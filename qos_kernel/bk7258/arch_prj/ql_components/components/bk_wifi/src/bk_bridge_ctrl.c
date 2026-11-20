/*
 * Copyright 2020-2026 Beken
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * AP-side bridge control: context singleton + FSM transitions (merged from
 * bk_bridge_ctx.c and bk_bridge_fsm.c).
 */

#include "common/bk_include.h"
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <common/bk_err.h>
#include <stdbool.h>

#include "bk_private/bk_bridge_ctx.h"
#include "bk_private/bk_bridge_fsm.h"
#include "bk_private/bk_wifi.h"
#include "wifi_log.h"
#include "wifi_api.h"

/* ====================================================================
 * Context (state + config)
 * ==================================================================== */

static bk_bridge_ctx_t s_ctx = {
    .state = BRIDGE_STATE_DISABLED,
};

bk_bridge_state_t bk_bridge_ctx_get_state(void)
{
    return s_ctx.state;
}

void bk_bridge_ctx_set_state_local(bk_bridge_state_t st)
{
    s_ctx.state = st;
}

bk_bridge_config_t *bk_bridge_ctx_get_config(void)
{
    return &s_ctx.config;
}

static inline void ctx_free_str(char **slot)
{
    if (*slot) {
        os_free(*slot);
        *slot = NULL;
    }
}

bk_err_t bk_bridge_ctx_save_config(const bk_bridge_config_t *src)
{
    if (src == NULL)
        return BK_ERR_PARAM;

    os_memcpy(&s_ctx.config.sta_config, &src->sta_config, sizeof(wifi_sta_config_t));
    os_memcpy(&s_ctx.config.br_info, &src->br_info, sizeof(wifi_ap_config_t));

    ctx_free_str(&s_ctx.config.hostname);
    if (src->hostname)
        s_ctx.config.hostname = os_strdup(src->hostname);

    s_ctx.config.keep_sta_on_close = src->keep_sta_on_close ? 1 : 0;

    return BK_OK;
}

/* ====================================================================
 * FSM (Finite State Machine) — AP-side bridge lifecycle
 * ====================================================================
 *
 * States (bk_bridge_state_t):
 *   DISABLED  — bridge off; no br0 / softap bring-up in progress.
 *   DISABLING — tear-down in progress (stop AP, bridge netif, optional STA).
 *   ENABLING  — bring-up in progress (STA assoc, softap, br0, port attach).
 *   ENABLED   — bridge running; CP fast-path may forward while in this state.
 *
 * bk_bridge_fsm_transition(to) is the only supported way to change state.  It:
 *   - validates from->to against s_legal (illegal jumps return BK_ERR_STATE);
 *   - treats same-state calls as a no-op (no CP IPC);
 *   - under one mutex: updates s_ctx.state, then bk_wifi_sync_bridge_state()
 *     so AP and CP never observe a half-applied transition from other threads;
 *   - optionally notifies a single observer (stats / debug).
 *
 * Legal edges (see s_legal matrix below):
 *   DISABLED  -> ENABLING
 *   ENABLING  -> DISABLING | ENABLED
 *   ENABLED   -> DISABLING
 *   DISABLING -> DISABLED
 *
 * CP does not run this FSM; it only stores the state AP pushes via IPC.
 * Callers: CLI bridge open/close, wifi event handler, br_start_thread.
 */

#define FSM_NSTATES 4
static const bool s_legal[FSM_NSTATES][FSM_NSTATES] = {
    /*  to:     DISABLED DISABLING ENABLING ENABLED */
    /* DISABLED  */ { false, false, true,  false },
    /* DISABLING */ { true,  false, false, false },
    /* ENABLING  */ { false, true,  false, true  },
    /* ENABLED   */ { false, true,  false, false },
};

static volatile bool      s_lock_inited = false;
static beken_mutex_t      s_lock;
static bk_bridge_fsm_observer_t s_observer;

static void fsm_ensure_lock(void)
{
    if (s_lock_inited)
        return;
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    if (!s_lock_inited) {
        if (rtos_init_mutex(&s_lock) == kNoErr)
            s_lock_inited = true;
        else
            WIFI_LOGE("bridge fsm: rtos_init_mutex failed\r\n");
    }
    GLOBAL_INT_RESTORE();
}

static const char *fsm_name(bk_bridge_state_t st)
{
    switch (st) {
    case BRIDGE_STATE_DISABLED:  return "DISABLED";
    case BRIDGE_STATE_DISABLING: return "DISABLING";
    case BRIDGE_STATE_ENABLING:  return "ENABLING";
    case BRIDGE_STATE_ENABLED:   return "ENABLED";
    default:                     return "???";
    }
}

void bk_bridge_fsm_set_observer(bk_bridge_fsm_observer_t cb)
{
    s_observer = cb;
}

bk_err_t bk_bridge_fsm_transition(bk_bridge_state_t to)
{
    bk_bridge_state_t         from;
    bk_err_t                  ret = BK_OK;
    bk_bridge_fsm_observer_t  obs;

    if ((unsigned)to >= FSM_NSTATES) {
        WIFI_LOGE("bridge fsm: bad target state %u\r\n", (unsigned)to);
        return BK_ERR_PARAM;
    }

    fsm_ensure_lock();
    if (!s_lock_inited)
        return BK_FAIL;

    rtos_lock_mutex(&s_lock);

    from = bk_bridge_ctx_get_state();
    if (from == to) {
        ret = BK_OK;
        goto done;
    }

    if (!s_legal[from][to]) {
        WIFI_LOGE("bridge fsm: ILLEGAL %s -> %s, ignored\r\n",
                  fsm_name(from), fsm_name(to));
        ret = BK_ERR_STATE;
        goto done;
    }

    bk_bridge_ctx_set_state_local(to);
    ret = bk_wifi_sync_bridge_state(to);
    if (ret != BK_OK) {
        WIFI_LOGW("bridge fsm: %s -> %s, CP sync failed ret=%d (cores may disagree)\r\n",
                  fsm_name(from), fsm_name(to), ret);
    } else {
        WIFI_LOGD("bridge fsm: %s -> %s\r\n", fsm_name(from), fsm_name(to));
    }

done:
    obs = s_observer;
    rtos_unlock_mutex(&s_lock);

    if (obs)
        obs(from, to, ret);
    return ret;
}
