// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * bk_dm_hfp_ag.h
 *
 * HFP AG (Audio Gateway) public API, in the Beken bk_dm_hfp.h style
 * (bk_bt_hf_ag_* functions, bt_err_t return codes, uint8_t remote_bda[6]).
 *
 * The command path only posts a request to the single ui_ethermind_ctx_thread,
 * which is the sole owner of the EtherMind stack and the AG shared state; hence
 * every API below returns after the request "is sent to lower layer".
 */

#pragma once

#include "bk_dm_bluetooth_types.h"
#include "bk_dm_hfp_ag_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief           Register application callback function to HFP AG module.
 *
 * @param[in]       callback: HFP AG event callback function
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_register_callback(bk_bt_hf_ag_cb_t callback);

/**
 * @brief           Initialize the bluetooth HFP AG module.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: the initialization request is sent successfully
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_init(void);

/**
 * @brief           De-initialize for HFP AG module.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_deinit(void);

/**
 * @brief           Get/Set the AG SDP SupportedFeatures attribute (0x0311).
 *                  Call after bk_bt_hf_ag_init() and before establishing an SLC.
 *
 * @param[in]       method: BK_HF_AG_FEATURE_API_METHOD_GET_ALLOWED / GET_CURRENT_ENABLE / SET
 * @param[in,out]   feat: output when method is GET_*, input when method is SET.
 *                        AG feature bitmask (BK_HF_AG_FEAT_* from bk_dm_hfp_common.h).
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_sdp_feature_operation(bk_hf_ag_feature_api_method_t method, uint16_t *feat);

/**
 * @brief           Get/Set the AG features advertised to the HF in the +BRSF response.
 *                  Call after bk_bt_hf_ag_init() and before establishing an SLC.
 *
 * @param[in]       method: BK_HF_AG_FEATURE_API_METHOD_GET_ALLOWED / GET_CURRENT_ENABLE / SET
 * @param[in,out]   feat: output when method is GET_*, input when method is SET.
 *                        AG feature bitmask (BK_HF_AG_FEAT_* from bk_dm_hfp_common.h).
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_brsf_feature_operation(bk_hf_ag_feature_api_method_t method, uint32_t *feat);

/**
 * @brief           Get/Set the AG capabilities returned for AT+CHLD=?.
 *                  Call after bk_bt_hf_ag_init() and before establishing an SLC.
 *
 * @param[in]       method: BK_HF_AG_FEATURE_API_METHOD_GET_ALLOWED / GET_CURRENT_ENABLE / SET
 * @param[in,out]   feat: output when method is GET_*, input when method is SET.
 *                        CHLD capability bitmask (BK_HF_CHLD_FEAT_*).
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_chld_feature_operation(bk_hf_ag_feature_api_method_t method, uint32_t *feat);

/**
 * @brief           Establish a Service Level Connection to remote bluetooth HFP client device.
 *                  This function must be called after bk_bt_hf_ag_init() and before bk_bt_hf_ag_deinit().
 *
 * @param[in]       remote_bda: remote bluetooth HFP client device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: connect request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_slc_connect(uint8_t *remote_bda);

/**
 * @brief           Disconnect from the remote HFP client.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: disconnect request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_slc_disconnect(uint8_t *remote_bda);

/**
 * @brief           Create audio connection with remote HFP client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: connect request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_audio_connect(uint8_t *remote_bda);

/**
 * @brief           Release the established audio connection with remote HFP client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: disconnect request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_audio_disconnect(uint8_t *remote_bda);

/**
 * @brief           Select the SCO codec the AG uses for subsequent audio connections.
 *                  The peer's codec capability is reported to the application via
 *                  BK_HF_AG_CODEC_EVT (codec_info.codec) when the HF sends AT+BAC;
 *                  the application may then call this to choose the codec. If the
 *                  selected codec is not supported by the peer, CVSD is used.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       codec: desired codec (CODEC_VOICE_CVSD / CODEC_VOICE_MSBC / ...)
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_set_codec(uint8_t *remote_bda, bk_hf_codec_type_t codec);

/**
 * @brief           Response of Voice Recognition Command(AT+BVRA) from HFP client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_bda: the device address of voice recognition initiator
 * @param[in]       value: voice recognition state, disabled or enabled
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_vra_control(uint8_t *remote_bda, bk_hf_vr_state_t value);

/**
 * @brief           Volume synchronization with HFP client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       type: volume control target, speaker or microphone
 * @param[in]       volume: gain of the speaker or microphone, ranges 0 to 15
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_volume_control(uint8_t *remote_bda, bk_hf_volume_control_target_t type, int volume);

/**
 * @brief           Handle Unknown AT command from HFP client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       unat: user AT command response to HF. It will respond "ERROR" by default if unat is NULL.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_unknown_at_send(uint8_t *remote_addr, char *unat);

/**
 * @brief           Unsolicited send extend AT error code to HFP client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       response_code: AT command response code
 * @param[in]       error_code: CME error code
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_cmee_send(uint8_t *remote_bda, bk_hf_at_response_code_t response_code, bk_hf_cme_err_t error_code);

/**
 * @brief           Unsolicited send device status notification to HFP client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       ntk_state: network service state
 * @param[in]       signal: signal strength from 0 to 5
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_devices_status_indchange(uint8_t *remote_addr, bk_hf_call_status_t call_state,
                                              bk_hf_call_setup_status_t call_setup_state,
                                              bk_hf_network_state_t ntk_state, int signal);

/**
 * @brief           Send ONE unsolicited indicator event (+CIEV: <idx>,<value>) to the HF and
 *                  update the AG's cached indicator so a later AT+CIND? read stays consistent.
 *                  This is the generic, low-level primitive; the app need not build raw AT
 *                  strings. Use it mainly for indicators the purpose-built APIs don't cover
 *                  (e.g. BK_HF_AG_CIND_IDX_ROAM, BK_HF_AG_CIND_IDX_BATTCHG).
 *
 *                  Relationship to the other indicator / call APIs:
 *                  - bk_bt_hf_ag_devices_status_indchange(): higher-level convenience that
 *                    reports the service/call/callsetup/signal indicators together (each is
 *                    effectively a ciev_report, sent only when the value changed). Prefer it
 *                    for those four; use ciev_report for a single indicator or for roam/battchg.
 *                  - bk_bt_hf_ag_cind_response(): the *reply* to the HF's AT+CIND? query (a full
 *                    snapshot of all indicators), NOT an unsolicited update. It seeds the initial
 *                    values; ciev_report pushes subsequent changes.
 *                  - bk_bt_hf_ag_answer_call()/reject_call()/out_call()/end_call(): these drive
 *                    the call and callsetup indicators as part of the call state machine. Do NOT
 *                    push call/callsetup via ciev_report during an active call flow, or it will
 *                    conflict with them; let the call-control APIs own those two indicators.
 *
 *                  As a precondition, a Service Level Connection shall exist with the HF.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       idx: indicator position (bk_hf_ag_cind_idx_t)
 * @param[in]       value: new indicator value (clamped to the indicator's valid range by the AG)
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_ciev_report(uint8_t *remote_addr, bk_hf_ag_cind_idx_t idx, int value);

/**
 * @brief           Response to device individual indicators(AT+CIND?) to HFP client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       ntk_state: network service state
 * @param[in]       signal: signal strength from 0 to 5
 * @param[in]       roam: roam state
 * @param[in]       batt_lev: battery level from 0 to 5
 * @param[in]       call_held_status: call held status
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_cind_response(uint8_t *remote_addr,
                                   bk_hf_call_status_t call_state,
                                   bk_hf_call_setup_status_t call_setup_state,
                                   bk_hf_network_state_t ntk_state, int signal, bk_hf_roaming_status_t roam, int batt_lev,
                                   bk_hf_call_held_status_t call_held_status);

/**
 * @brief           Response for AT+COPS command from HF client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       name: current operator name
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_cops_response(uint8_t *remote_addr, char *name);

/**
 * @brief           Response to AT+CLCC command from HFP client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       index: the index of current call
 * @param[in]       dir: call direction (incoming/outgoing)
 * @param[in]       current_call_state: current call state
 * @param[in]       mode: current call mode (voice/data/fax)
 * @param[in]       mpty: single or multi type
 * @param[in]       number: current call number
 * @param[in]       type: international type or unknown
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_clcc_response(uint8_t *remote_addr, int index, bk_hf_current_call_direction_t dir,
                                   bk_hf_current_call_status_t current_call_state, bk_hf_current_call_mode_t mode,
                                   bk_hf_current_call_mpty_type_t mpty, char *number, bk_hf_call_addr_type_t type);

/**
 * @brief           Response for AT+CNUM command from HF client.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       number: registration number
 * @param[in]       type: service type (unknown/voice/fax)
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_cnum_response(uint8_t *remote_addr, char *number, bk_hf_subscriber_service_type_t type);

/**
 * @brief           Inform HF client that AG provided in-band ring tone or not.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       state: in-band ring tone state
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_bsir(uint8_t *remote_addr, bk_hf_ag_in_band_ring_state_t state);

/**
 * @brief           Send +BTRH status in response to AT+BTRH or AT+BTRH?.
 */
bt_err_t bk_bt_hf_ag_btrh_response(uint8_t *remote_addr, bk_hf_btrh_status_t status);

/**
 * @brief           Send an unsolicited +CLIP caller-ID report.
 *                  The report is sent only after the HF enables it with AT+CLIP=1.
 */
bt_err_t bk_bt_hf_ag_clip_report(uint8_t *remote_addr, const char *number,
                                 bk_hf_call_addr_type_t type);

/**
 * @brief           Send an unsolicited +CCWA call-waiting report.
 *                  The report is sent only after the HF enables it with AT+CCWA=1.
 */
bt_err_t bk_bt_hf_ag_ccwa_report(uint8_t *remote_addr, const char *number,
                                 bk_hf_call_addr_type_t type);

/**
 * @brief           Send an unsolicited RING indication to the HF.
 */
bt_err_t bk_bt_hf_ag_ring(uint8_t *remote_addr);

/**
 * @brief           Answer incoming call from AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       num_active: the number of active call
 * @param[in]       num_held: the number of held call
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       number: number of the incoming call
 * @param[in]       call_addr_type: call address type
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_answer_call(uint8_t *remote_addr, int num_active, int num_held,
                                 bk_hf_call_status_t call_state, bk_hf_call_setup_status_t call_setup_state,
                                 char *number, bk_hf_call_addr_type_t call_addr_type);

/**
 * @brief           Reject incoming call from AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       num_active: the number of active call
 * @param[in]       num_held: the number of held call
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       number: number of the incoming call
 * @param[in]       call_addr_type: call address type
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_reject_call(uint8_t *remote_addr, int num_active, int num_held,
                                 bk_hf_call_status_t call_state, bk_hf_call_setup_status_t call_setup_state,
                                 char *number, bk_hf_call_addr_type_t call_addr_type);

/**
 * @brief           Initiate a call from AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       num_active: the number of active call
 * @param[in]       num_held: the number of held call
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       number: number of the outgoing call
 * @param[in]       call_addr_type: call address type
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_out_call(uint8_t *remote_addr, int num_active, int num_held,
                              bk_hf_call_status_t call_state, bk_hf_call_setup_status_t call_setup_state,
                              char *number, bk_hf_call_addr_type_t call_addr_type);

/**
 * @brief           End an ongoing call.
 *                  As a precondition to use this API, Service Level Connection shall exist with HFP client.
 *
 * @param[in]       remote_addr: remote bluetooth device address
 * @param[in]       num_active: the number of active call
 * @param[in]       num_held: the number of held call
 * @param[in]       call_state: call state
 * @param[in]       call_setup_state: call setup state
 * @param[in]       number: number of the call
 * @param[in]       call_addr_type: call address type
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_end_call(uint8_t *remote_addr, int num_active, int num_held,
                              bk_hf_call_status_t call_state, bk_hf_call_setup_status_t call_setup_state,
                              char *number, bk_hf_call_addr_type_t call_addr_type);

/**
 * @brief           Register AG data output function. Only used when Voice Over HCI is enabled.
 *
 * @param[in]       recv: HFP client incoming data callback function
 * @param[in]       send: HFP client outgoing data callback function
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_register_data_callback(bk_bt_hf_ag_incoming_data_cb_t recv, bk_bt_hf_ag_outgoing_data_cb_t send);

/**
 * @brief           Send one SCO voice (PCM) frame to the remote HFP client (Voice over HCI).
 *                  As a precondition, an audio (SCO) connection shall exist with the HFP client.
 *                  For the CVSD codec the payload is raw 16-bit PCM at 8 kHz.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       data: PCM data to transmit over SCO
 * @param[in]       len: length in bytes of data
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_ag_voice_out_write(uint8_t *remote_bda, uint8_t *data, uint16_t len);

/**
 * @brief           Trigger the lower-layer to fetch and send audio data.
 *                  Only used when Voice Over HCI is enabled. As a precondition, Service Level
 *                  Connection shall exist with HFP client.
 */
void bk_bt_hf_ag_outgoing_data_ready(void);

#ifdef __cplusplus
}
#endif
