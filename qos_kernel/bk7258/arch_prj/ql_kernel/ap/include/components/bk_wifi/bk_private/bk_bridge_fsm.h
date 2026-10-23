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
 * Explicit FSM for AP-side bridge state transitions.
 *
 * Previously the codebase open-coded
 *     bk_bridge_ctx_set_state_local(X);
 *     bk_wifi_sync_bridge_state(X);
 * at every transition point.  That had three problems:
 *
 *   1. Two-step pattern is easy to forget the IPC sync (which is exactly
 *      what produced the race we fixed in switch_bridge_to_sta).
 *   2. Nothing rejects an illegal jump (e.g. DISABLED -> ENABLED).
 *   3. No place to add observability — tracing, stats, watchpoints.
 *
 * bk_bridge_fsm_transition() is the only sanctioned way to mutate the
 * AP-side bridge state.  It:
 *
 *   - serializes concurrent transitions (CLI thread vs wifi-event task)
 *     through an internal mutex;
 *   - rejects illegal source->target pairs against a static table;
 *   - writes the local ctx and dispatches the synchronous IPC to CP as
 *     one atomic step under the lock — so the AP-local view never gets
 *     ahead of CP's view across the IPC barrier;
 *   - invokes the registered observer (no-op by default).
 *
 * CP is a passive follower: it stores whatever AP tells it through
 * SET_BRIDGE_SYNC_STATE.  We do NOT replicate this FSM on CP.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "common/bk_include.h"

#if CONFIG_BRIDGE

#include <modules/wifi_types.h>

/* Try to drive AP-side bridge state from `current` to `to`.
 *
 * Returns:
 *   BK_OK              transition (or no-op same-state) accepted & synced.
 *   BK_ERR_STATE       transition is not in the legal table — state is
 *                      left untouched, no IPC is sent.  Caller MUST treat
 *                      this as a programming bug; the FSM also logs.
 *   <other>            return from bk_wifi_sync_bridge_state(); state was
 *                      written locally but the CP-side ACK failed.  The
 *                      cores are now potentially out of sync — caller's
 *                      job to recover (typically by tearing the bridge
 *                      down via a series of further transitions).
 */
bk_err_t bk_bridge_fsm_transition(bk_bridge_state_t to);

/* Optional tracing hook.  Called from inside the transition mutex AFTER
 * the local write and IPC have completed (or been rejected).  Passing
 * NULL clears the hook.  Single global slot — late writer wins.
 *
 * A per-transition stats counter can be plugged in here without touching
 * any call site.
 */
typedef void (*bk_bridge_fsm_observer_t)(bk_bridge_state_t from,
                                          bk_bridge_state_t to,
                                          bk_err_t          result);
void bk_bridge_fsm_set_observer(bk_bridge_fsm_observer_t cb);

#endif /* CONFIG_BRIDGE */

#ifdef __cplusplus
}
#endif
