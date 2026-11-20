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

#pragma once

/*
 * AP-side bridge context singleton.
 *
 * Centralises every AP-app-layer global the bridge subsystem mutates so we
 * have ONE place to grep, ONE place to add tracing, and a foundation for
 * the explicit bridge FSM.
 *
 * The fields we actively own here:
 *   - state      : bk_bridge_state_t, mirrored on CP via bk_wifi_sync_bridge_state
 *   - config     : sta_config + br_info + hostname snapshot from bk_bridge_start()
 *
 * Symbols that *logically* belong to the context but physically live elsewhere
 * (so we don't drag lwip/net.c globals across a module boundary):
 *   - bridge_ip_start_flag       -> net.c (bridge_ip_is_start / bridge_set_ip_start_flag)
 *   - g_bk_ap_connected          -> net.c
 *   - bridge_sap_port/sta_port   -> port/bk_bridge.c (set via bk_bridge_hook_port_attach)
 *   - bridgeif_netif_client_id   -> bridgeif.c (lwip-owned)
 *
 * Thread-safety: state/config are mutated only from non-ISR contexts
 * (wifi-event task / cli task). state is volatile so the CP fast-path can
 * race-free observe transitions. config is mutated only during
 * START/STOP control flow (single writer at a time).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "common/bk_include.h"

#if CONFIG_BRIDGE

#include <modules/wifi_types.h>

typedef struct {
    volatile bk_bridge_state_t state;
    bk_bridge_config_t         config;
} bk_bridge_ctx_t;

/* ---- state ----------------------------------------------------------- */

bk_bridge_state_t bk_bridge_ctx_get_state(void);

/*
 * Set state LOCAL ONLY — does NOT sync to CP.
 *
 * This is internal plumbing for bk_bridge_fsm_transition() and
 * MUST NOT be called from application code.  Use bk_bridge_fsm_transition()
 * instead; the FSM does the local write + CP IPC atomically under its
 * mutex and validates the transition against the legal table.
 */
void bk_bridge_ctx_set_state_local(bk_bridge_state_t st);

/* ---- config ---------------------------------------------------------- */

bk_bridge_config_t *bk_bridge_ctx_get_config(void);

/*
 * Deep-copy sta_config/br_info; strdup hostname if set.  Returns BK_OK on success.
 */
bk_err_t bk_bridge_ctx_save_config(const bk_bridge_config_t *src);

#endif /* CONFIG_BRIDGE */

#ifdef __cplusplus
}
#endif
