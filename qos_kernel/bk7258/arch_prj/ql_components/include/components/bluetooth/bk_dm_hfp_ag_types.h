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
 * bk_dm_hfp_ag_types.h
 *
 * HFP AG (Audio Gateway) public type definitions.
 * Enums shared with the HF client role are defined in bk_dm_hfp_common.h.
 */

#pragma once

#include <stdint.h>
#include "bk_dm_hfp_common.h"

#ifdef __cplusplus
extern "C" {
#endif


/// Bluetooth HFP RFCOMM connection and service level connection status
typedef enum {
    BK_HF_AG_CONNECTION_STATE_DISCONNECTED = 0,        /*!< RFCOMM data link channel released */
    BK_HF_AG_CONNECTION_STATE_CONNECTING,              /*!< connecting remote device on the RFCOMM data link */
    BK_HF_AG_CONNECTION_STATE_CONNECTED,               /*!< RFCOMM connection established */
    BK_HF_AG_CONNECTION_STATE_SLC_CONNECTED,           /*!< service level connection established */
    BK_HF_AG_CONNECTION_STATE_DISCONNECTING,           /*!< disconnecting with remote device on the RFCOMM data link */
} bk_hf_ag_connection_state_t;

/// Bluetooth HFP audio connection status
typedef enum {
    BK_HF_AG_AUDIO_STATE_DISCONNECTED = 0,             /*!< audio connection released */
    BK_HF_AG_AUDIO_STATE_CONNECTING,                   /*!< audio connection has been initiated */
    BK_HF_AG_AUDIO_STATE_CONNECTED,                    /*!< audio connection is established (CVSD) */
    BK_HF_AG_AUDIO_STATE_CONNECTED_MSBC,               /*!< audio connection is established (mSBC) */
} bk_hf_ag_audio_state_t;

/// in-band ring tone state
typedef enum {
    BK_HF_AG_IN_BAND_RINGTONE_NOT_PROVIDED = 0,        /*!< in-band ring tone not provided */
    BK_HF_AG_IN_BAND_RINGTONE_PROVIDED,                /*!< in-band ring tone provided */
} bk_hf_ag_in_band_ring_state_t;

/// HF-originated dial request type
typedef enum {
    BK_HF_AG_DIAL_TYPE_NUMBER = 0,                     /*!< ATD<number>: dial supplied number */
    BK_HF_AG_DIAL_TYPE_MEMORY,                         /*!< ATD><location>: dial phonebook location */
    BK_HF_AG_DIAL_TYPE_REDIAL,                         /*!< AT+BLDN: redial last number */
} bk_hf_ag_dial_type_t;

/// HFP AG feature-config API method (see bk_bt_hf_ag_sdp_feature_operation / bk_bt_hf_ag_brsf_feature_operation)
typedef enum {
    BK_HF_AG_FEATURE_API_METHOD_GET_ALLOWED = 0,       /*!< all features the stack can advertise, immutable */
    BK_HF_AG_FEATURE_API_METHOD_GET_CURRENT_ENABLE,    /*!< features currently configured */
    BK_HF_AG_FEATURE_API_METHOD_SET,                   /*!< set features; must be called before bk_bt_hf_ag_init() */
    BK_HF_AG_FEATURE_API_METHOD_MAX,
} bk_hf_ag_feature_api_method_t;

/// +CIND / +CIEV indicator positions (1-based), in the order the AG advertises them in the
/// AT+CIND=? test response. The AG (host) owns this ordering; apps must use these indices
/// for unsolicited +CIEV updates instead of hard-coding numbers.
typedef enum {
    BK_HF_AG_CIND_IDX_SERVICE       = 1,               /*!< network service availability */
    BK_HF_AG_CIND_IDX_CALL          = 2,               /*!< call (active) status */
    BK_HF_AG_CIND_IDX_CALLSETUP     = 3,               /*!< call setup status */
    BK_HF_AG_CIND_IDX_CALLHELD      = 4,               /*!< call held status */
    BK_HF_AG_CIND_IDX_SIGNAL        = 5,               /*!< signal strength */
    BK_HF_AG_CIND_IDX_ROAM          = 6,               /*!< roaming status */
    BK_HF_AG_CIND_IDX_BATTCHG       = 7,               /*!< battery charge */
    BK_HF_AG_CIND_IDX_CALL_FORWARD  = 8,               /*!< call forward status */
    BK_HF_AG_CIND_IDX_MAX,                             /*!< one past the last valid indicator index */
} bk_hf_ag_cind_idx_t;

/// HFP AG callback events.
///
/// Reply obligation (from the AG application's point of view):
///   [reply]   the app MUST answer the HF. Send the final OK / +CME ERROR with
///             bk_bt_hf_ag_cmee_send(), or use the dedicated *_response() API noted per event.
///   [update]  the HF pushed an unsolicited value; the stack already acked, no reply needed.
///   [notify]  a local/state notification; the stack owns the wire reply, no reply needed.
typedef enum {
    BK_HF_AG_CONNECTION_STATE_EVT = 0,              /*!< [notify] RFCOMM/SLC connection state changed */
    BK_HF_AG_AUDIO_STATE_EVT,                       /*!< [notify] audio (SCO) connection state changed */
    BK_HF_AG_BVRA_REQ_EVT,                          /*!< [reply]  HF voice-recognition request (AT+BVRA) -> bk_bt_hf_ag_cmee_send() */
    BK_HF_AG_VOLUME_CONTROL_EVT,                    /*!< [notify] HF volume control (+VGS/+VGM); stack sends OK */

    BK_HF_AG_UNAT_REQ_EVT,                          /*!< [reply]  unknown AT from HF -> bk_bt_hf_ag_unknown_at_send() and/or bk_bt_hf_ag_cmee_send() */
    BK_HF_AG_CIND_REQ_EVT,                          /*!< [reply]  HF indicator query (AT+CIND?) -> bk_bt_hf_ag_cind_response() */
    BK_HF_AG_COPS_REQ_EVT,                          /*!< [reply]  HF operator query (AT+COPS?) -> bk_bt_hf_ag_cops_response() */
    BK_HF_AG_CLCC_REQ_EVT,                          /*!< [reply]  HF current-calls query (AT+CLCC) -> bk_bt_hf_ag_clcc_response() */
    BK_HF_AG_CNUM_REQ_EVT,                          /*!< [reply]  HF subscriber-number query (AT+CNUM) -> bk_bt_hf_ag_cnum_response() */
    BK_HF_AG_VTS_REQ_EVT,                           /*!< [reply]  DTMF code from HF (AT+VTS) -> bk_bt_hf_ag_cmee_send() */
    BK_HF_AG_NREC_REQ_EVT,                          /*!< [reply]  HF disable-NREC request (AT+NREC) -> bk_bt_hf_ag_cmee_send() */

    BK_HF_AG_ATA_REQ_EVT,                           /*!< [reply]  HF answers a call (ATA) -> bk_bt_hf_ag_cmee_send() then update call state */
    BK_HF_AG_CHUP_REQ_EVT,                          /*!< [reply]  HF ends/rejects a call (AT+CHUP) -> bk_bt_hf_ag_cmee_send() then update call state */
    BK_HF_AG_DIAL_REQ_EVT,                          /*!< [reply]  HF dials/redials (ATD / AT+BLDN) -> bk_bt_hf_ag_cmee_send() then update call state */
    BK_HF_AG_CODEC_EVT,                             /*!< [notify] peer codec list (AT+BAC); stack sends OK */
    BK_HF_AG_BCS_RESPONSE_EVT,                      /*!< [notify] HF confirmed the codec (AT+BCS); stack handles SCO setup */
    BK_HF_AG_BRSF_EVT,                              /*!< [notify] HF supported features (AT+BRSF); stack sends +BRSF and OK */
    BK_HF_AG_BIEV_UPDATE_EVT,                       /*!< [update] HF indicator value (AT+BIEV); stack sends OK */
    BK_HF_AG_CHLD_REQ_EVT,                          /*!< [reply]  HF call-hold/multiparty request (AT+CHLD) -> bk_bt_hf_ag_cmee_send() */
    BK_HF_AG_BTRH_REQ_EVT,                          /*!< [reply]  HF response-and-hold request (AT+BTRH) -> bk_bt_hf_ag_btrh_response() then bk_bt_hf_ag_cmee_send() */
} bk_hf_ag_cb_event_t;

/// HFP AG callback parameters
typedef struct {
    uint8_t remote_addr[6];                         /*!< remote bluetooth device address; valid for every event */

    union {
        /**
         * @brief  BK_HF_AG_CONNECTION_STATE_EVT
         */
        struct hf_ag_conn_stat_param {
            bk_hf_ag_connection_state_t state;             /*!< connection state */
            uint32_t peer_feat;                         /*!< HF supported features */
            uint32_t chld_feat;                         /*!< AG CHLD capability mask, see BK_HF_CHLD_FEAT_* */
        } conn_stat;                                    /*!< AG callback param of BK_HF_AG_CONNECTION_STATE_EVT */

        /**
         * @brief BK_HF_AG_AUDIO_STATE_EVT
         */
        struct hf_ag_audio_stat_param {
            bk_hf_ag_audio_state_t state;                  /*!< audio connection state (connected / disconnected) */
            bk_hf_codec_type_t codec;                   /*!< negotiated SCO codec when connected */
            uint8_t interval;                        /*!< tx interval in 625us */
            uint16_t tx_packet_len;                  /*!< tx packet len in byte */
            uint16_t rx_packet_len;                  /*!< rx packet len in byte */
            uint8_t packet_type;                    /*!< coding packet type, see bk_bt_audio_coding_format_t */
        } audio_stat;                                   /*!< AG callback param of BK_HF_AG_AUDIO_STATE_EVT */

        /**
         * @brief BK_HF_AG_BVRA_REQ_EVT
         */
        struct hf_ag_vra_req_param {
            bk_hf_vr_state_t value;                     /*!< voice recognition state */
        } vra_req;                                      /*!< AG callback param of BK_HF_AG_BVRA_REQ_EVT */

        /**
         * @brief BK_HF_AG_VOLUME_CONTROL_EVT
         */
        struct hf_ag_volume_control_param {
            bk_hf_volume_control_target_t type;         /*!< volume control target, speaker or microphone */
            int volume;                                 /*!< gain, ranges from 0 to 15 */
        } volume_control;                               /*!< AG callback param of BK_HF_AG_VOLUME_CONTROL_EVT */

        /**
         * @brief BK_HF_AG_UNAT_REQ_EVT
         */
        struct hf_ag_unat_req_param {
            char *unat;                                 /*!< unknown AT command string */
            uint8_t app_response;                       /*!< app response to the unknown AT command */
        } unat_req;                                     /*!< AG callback param of BK_HF_AG_UNAT_REQ_EVT */

        /**
         * @brief BK_HF_AG_DIAL_REQ_EVT
         */
        struct hf_ag_out_call_param {
            bk_hf_ag_dial_type_t type;                  /*!< number, memory-location, or redial request */
            char *num_or_loc;                           /*!< number string of the call, or location for memory dial; NULL means re-dial */
        } out_call;                                     /*!< AG callback param of BK_HF_AG_DIAL_REQ_EVT */

        /**
         * @brief BK_HF_AG_VTS_REQ_EVT
         */
        struct hf_ag_vts_req_param {
            char *code;                                 /*!< DTMF code from HF */
        } vts_req;                                      /*!< AG callback param of BK_HF_AG_VTS_REQ_EVT */

        /**
         * @brief BK_HF_AG_NREC_REQ_EVT
         */
        struct hf_ag_nrec_param {
            bk_hf_nrec_t state;                            /*!< NREC enabled or disabled */
        } nrec;                                         /*!< AG callback param of BK_HF_AG_NREC_REQ_EVT */

        /**
         * @brief BK_HF_AG_CODEC_EVT
         */
        struct hf_ag_codec_info_param {
            uint8_t            num;                          /*!< number of codecs the peer (HF) supports */
            bk_hf_codec_type_t codecs[CODEC_VOICE_MAX];      /*!< codecs the peer supports (CVSD always included) */
        } codec_info;                                   /*!< AG callback param of BK_HF_AG_CODEC_EVT */

        /**
         * @brief BK_HF_AG_BCS_RESPONSE_EVT
         */
        struct hf_ag_bcs_rep_param {
            bk_hf_codec_type_t codec;                   /*!< final negotiated codec */
        } bcs_rep;                                      /*!< AG callback param of BK_HF_AG_BCS_RESPONSE_EVT */

        /**
         * @brief BK_HF_AG_BRSF_EVT
         */
        struct hf_ag_brsf_param {
            bk_hf_client_brsf_feat_t peer_feat;         /*!< HF supported features bitmask (AT+BRSF) */
        } brsf;                                         /*!< AG callback param of BK_HF_AG_BRSF_EVT */

        /**
         * @brief BK_HF_AG_BIEV_UPDATE_EVT
         */
        struct hf_ag_biev_param {
            bk_hf_indicator_id_t ind_id;                /*!< HF indicator assigned number */
            uint32_t value;                             /*!< HF indicator value */
        } biev;                                         /*!< AG callback param of BK_HF_AG_BIEV_UPDATE_EVT */

        /**
         * @brief BK_HF_AG_CHLD_REQ_EVT
         */
        struct hf_ag_chld_param {
            bk_hf_chld_type_t type;                     /*!< requested CHLD operation */
            int index;                                  /*!< call index for 1x/2x, or -1 when absent */
        } chld;                                         /*!< AG callback param of BK_HF_AG_CHLD_REQ_EVT */

        /**
         * @brief BK_HF_AG_BTRH_REQ_EVT
         */
        struct hf_ag_btrh_param {
            bk_hf_btrh_cmd_t action;                    /*!< hold, accept, or reject action */
        } btrh;                                         /*!< AG callback param of BK_HF_AG_BTRH_REQ_EVT */
    };
} bk_hf_ag_cb_param_t;                              /*!< HFP AG callback parameters */

/**
 * @brief           HFP AG incoming data callback function, useful in case of Voice Over HCI.
 *
 * @param[in]       buf : pointer to incoming data(payload of HCI synchronous data packet)
 * @param[in]       len : size(in bytes) in buf
 */
typedef void (* bk_bt_hf_ag_incoming_data_cb_t)(const uint8_t *buf, uint32_t len);

/**
 * @brief           HFP AG outgoing data callback function, useful in case of Voice Over HCI.
 *
 * @param[in]       buf : pointer to the buffer to fill outgoing data into
 * @param[in]       len : size(in bytes) in buf
 *
 * @return          length of data successfully written into buf
 */
typedef uint32_t (* bk_bt_hf_ag_outgoing_data_cb_t)(uint8_t *buf, uint32_t len);

/**
 * @brief           HFP AG callback function type
 *
 * @param           event : Event type
 * @param           param : Pointer to callback parameter
 */
typedef void (* bk_bt_hf_ag_cb_t)(bk_hf_ag_cb_event_t event, bk_hf_ag_cb_param_t *param);

#ifdef __cplusplus
}
#endif
