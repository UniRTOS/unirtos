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

#pragma once
#include "bk_dm_hfp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Bluetooth HFP RFCOMM connection and service level connection status
typedef enum {
    BK_HF_CLIENT_CONNECTION_STATE_DISCONNECTED = 0,     /*!< RFCOMM data link channel released */
    BK_HF_CLIENT_CONNECTION_STATE_CONNECTING,           /*!< connecting remote device on the RFCOMM data link*/
    BK_HF_CLIENT_CONNECTION_STATE_CONNECTED,            /*!< RFCOMM connection established */
    BK_HF_CLIENT_CONNECTION_STATE_SLC_CONNECTED,        /*!< service level connection established */
    BK_HF_CLIENT_CONNECTION_STATE_DISCONNECTING,        /*!< disconnecting with remote device on the RFCOMM dat link*/
} bk_hf_client_connection_state_t;

/// Bluetooth HFP audio connection status
typedef enum {
    BK_HF_CLIENT_AUDIO_STATE_DISCONNECTED = 0,          /*!< audio connection released */
    BK_HF_CLIENT_AUDIO_STATE_CONNECTING,                /*!< audio connection has been initiated */
    BK_HF_CLIENT_AUDIO_STATE_CONNECTED,                 /*!< audio connection is established */
} bk_hf_client_audio_state_t;

/// HF CLIENT callback events
typedef enum {
    BK_HF_CLIENT_CONNECTION_STATE_EVT = 0,          /*!< connection state changed event */
    BK_HF_CLIENT_AUDIO_STATE_EVT,                   /*!< audio connection state change event */
    BK_HF_CLIENT_BVRA_EVT,                          /*!< voice recognition state change event */
    BK_HF_CLIENT_CIND_CALL_EVT,                     /*!< call indication */
    BK_HF_CLIENT_CIND_CALL_SETUP_EVT,               /*!< call setup indication */
    BK_HF_CLIENT_CIND_CALL_HELD_EVT,                /*!< call held indication */
    BK_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT,     /*!< network service availability indication */
    BK_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT,          /*!< signal strength indication */
    BK_HF_CLIENT_CIND_ROAMING_STATUS_EVT,           /*!< roaming status indication */
    BK_HF_CLIENT_CIND_BATTERY_LEVEL_EVT,            /*!< battery level indication */
    BK_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT,         /*!< current operator information */
    BK_HF_CLIENT_BTRH_EVT,                          /*!< call response and hold event */
    BK_HF_CLIENT_CLIP_EVT,                          /*!< Calling Line Identification notification */
    BK_HF_CLIENT_CCWA_EVT,                          /*!< call waiting notification */
    BK_HF_CLIENT_CLCC_EVT,                          /*!< list of current calls notification */
    BK_HF_CLIENT_VOLUME_CONTROL_EVT,                /*!< audio volume control command from AG, provided by +VGM or +VGS message */
    BK_HF_CLIENT_AT_RESPONSE_EVT,                   /*!< AT command response event */
    BK_HF_CLIENT_CNUM_EVT,                          /*!< subscriber information response from AG */
    BK_HF_CLIENT_BSIR_EVT,                          /*!< setting of in-band ring tone */
    BK_HF_CLIENT_BINP_EVT,                          /*!< requested number of last voice tag from AG */
    BK_HF_CLIENT_RING_IND_EVT,                      /*!< ring indication event */
    BK_HF_CLIENT_UNKNOWN_DATA_IND_EVT,              /*!< unknown data from AG */
} bk_hf_client_cb_event_t;

/// in-band ring tone state
typedef enum {
    BK_HF_CLIENT_IN_BAND_RINGTONE_NOT_PROVIDED = 0,
    BK_HF_CLIENT_IN_BAND_RINGTONE_PROVIDED,
} bk_hf_client_in_band_ring_state_t;

/// HFP client callback parameters
typedef struct{
    uint8_t remote_bda[6];                          /*!< remote bluetooth device address */
    union
    {
        /**
         * @brief  BK_HF_CLIENT_CONNECTION_STATE_EVT
         */
        struct hf_client_conn_stat_param
        {
            bk_hf_client_connection_state_t state;   /*!< HF connection state */
            uint32_t peer_feat;                      /*!< AG supported features */
            uint32_t chld_feat;                      /*!< AG supported features on call hold and multiparty services */
        } conn_state;                                 /*!< HF callback param of BK_HF_CLIENT_CONNECTION_STATE_EVT */

        /**
         * @brief BK_HF_CLIENT_AUDIO_STATE_EVT
         */
        struct hf_client_audio_stat_param
        {
            bk_hf_client_audio_state_t state;        /*!< audio connection state */
            bk_hf_codec_type_t codec_type;           /*!< cvsd or msbc */
            uint8_t interval;                        /*!< tx interval in 625us */
            uint16_t tx_packet_len;                  /*!< tx packet len in byte */
            uint16_t rx_packet_len;                  /*!< rx packet len in byte */
            uint8_t packet_type;                    /*!< coding packet type, see bk_bt_audio_coding_format_t */
        } audio_state;                               /*!< HF callback param of BK_HF_CLIENT_AUDIO_STATE_EVT */

        /**
         * @brief BK_HF_CLIENT_BVRA_EVT
         */
        struct hf_client_bvra_param
        {
            bk_hf_vr_state_t value;                 /*!< voice recognition state */
        } bvra;                                      /*!< HF callback param of BK_HF_CLIENT_BVRA_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_CALL_EVT
         */
        struct hf_client_call_ind_param
        {
            bk_hf_call_status_t status;             /*!< call status indicator */
        } call;                                      /*!< HF callback param of BK_HF_CLIENT_CIND_CALL_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_CALL_SETUP_EVT
         */
        struct hf_client_call_setup_ind_param
        {
            bk_hf_call_setup_status_t status;       /*!< call setup status indicator */
        } call_setup;                                /*!< HF callback param of BK_HF_CLIENT_BVRA_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_CALL_HELD_EVT
         */
        struct hf_client_call_held_ind_param
        {
            bk_hf_call_held_status_t status;        /*!< bluetooth proprietary call hold status indicator */
        } call_held;                                 /*!< HF callback param of BK_HF_CLIENT_CIND_CALL_HELD_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT
         */
        struct hf_client_service_availability_param
        {
            bk_hf_network_state_t status;           /*!< service availability status */
        } service_availability;                      /*!< HF callback param of BK_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT */


        /**
         * @brief BK_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT
         */
        struct hf_client_signal_strength_ind_param
        {
            int value;                               /*!< signal strength value, ranges from 0 to 5 */
        } signal_strength;                           /*!< HF callback param of BK_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_ROAMING_STATUS_EVT
         */
        struct hf_client_network_roaming_param
        {
            bk_hf_roaming_status_t status;          /*!< roaming status */
        } roaming;                                   /*!< HF callback param of BK_HF_CLIENT_CIND_ROAMING_STATUS_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_BATTERY_LEVEL_EVT
         */
        struct hf_client_battery_level_ind_param
        {
            int value;                               /*!< battery charge value, ranges from 0 to 5 */
        } battery_level;                             /*!< HF callback param of BK_HF_CLIENT_CIND_BATTERY_LEVEL_EVT */

        /**
         * @brief BK_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT
         */
        struct hf_client_current_operator_param
        {
            const char *name;                        /*!< name of the network operator */
        } cops;                                      /*!< HF callback param of BK_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT */

        /**
         * @brief BK_HF_CLIENT_BTRH_EVT
         */
        struct hf_client_btrh_param
        {
            bk_hf_btrh_status_t status;             /*!< call hold and response status result code */
        } btrh;                                      /*!< HF callback param of BK_HF_CLIENT_BRTH_EVT */

        /**
         * @brief BK_HF_CLIENT_CLIP_EVT
         */
        struct hf_client_clip_param
        {
            const char *name;                        /*!< name string of the call */
            const char *number;                      /*!< phone number string of call */
        } clip;                                      /*!< HF callback param of BK_HF_CLIENT_CLIP_EVT */

        /**
         * @brief BK_HF_CLIENT_CCWA_EVT
         */
        struct hf_client_ccwa_param
        {
            const char *number;                      /*!< phone number string of waiting call */
            const char *name;                        /*!< name string of waiting call */
        } ccwa;                                      /*!< HF callback param of BK_HF_CLIENT_BVRA_EVT */

        /**
         * @brief BK_HF_CLIENT_CLCC_EVT
         */
        struct hf_client_clcc_param
        {
            int idx;                                 /*!< numbering(starting with 1) of the call */
            bk_hf_current_call_direction_t dir;     /*!< direction of the call */
            bk_hf_current_call_status_t status;     /*!< status of the call */
            bk_hf_current_call_mpty_type_t mpty;    /*!< multi-party flag */
            char *number;                            /*!< phone number(optional) */
        } clcc;                                      /*!< HF callback param of BK_HF_CLIENT_CLCC_EVT */

        /**
         * @brief BK_HF_CLIENT_VOLUME_CONTROL_EVT
         */
        struct hf_client_volume_control_param
        {
            bk_hf_volume_control_target_t type;     /*!< volume control target, speaker or microphone */
            int volume;                              /*!< gain, ranges from 0 to 15 */
        } volume_control;                            /*!< HF callback param of BK_HF_CLIENT_VOLUME_CONTROL_EVT */

        /**
         * @brief BK_HF_CLIENT_AT_RESPONSE_EVT
         */
        struct hf_client_at_response_param
        {
            bk_hf_at_response_code_t code;          /*!< AT response code */
            bk_hf_cme_err_t cme;                    /*!< Extended Audio Gateway Error Result Code */
            uint16_t asso_cmd;
        } at_response;                               /*!< HF callback param of BK_HF_CLIENT_AT_RESPONSE_EVT */

        /**
         * @brief BK_HF_CLIENT_CNUM_EVT
         */
        struct hf_client_cnum_param
        {
            const char *number;                      /*!< phone number string */
            bk_hf_subscriber_service_type_t type;   /*!< service type that the phone number relates to */
        } cnum;                                      /*!< HF callback param of BK_HF_CLIENT_CNUM_EVT */

        /**
         * @brief BK_HF_CLIENT_BSIR_EVT
         */
        struct hf_client_bsirparam
        {
            bk_hf_client_in_band_ring_state_t state;  /*!< setting state of in-band ring tone */
        } bsir;                                        /*!< HF callback param of BK_HF_CLIENT_BSIR_EVT */

        /**
         * @brief bk_HF_CLIENT_BINP_EVT
         */
        struct hf_client_binp_param
        {
            const char *number;                      /*!< phone number corresponding to the last voice tag in the HF */
        } binp;                                      /*!< HF callback param of BK_HF_CLIENT_BINP_EVT */

        /**
         * @brief BK_HF_CLIENT_UNKNOWN_DATA_IND_EVT
         */
        struct hf_client_unknown_data_param
        {
            const char *data;                      /*!< unknown data */
            uint16_t data_len;                     /*!< the length of unknown data*/
        } unknown_data;                                    /*!< HF callback param of BK_HF_CLIENT_UNKNOWN_DATA_IND_EVT */

    };
}bk_hf_client_cb_param_t;                      /*!< HFP client callback parameters */

/**
 * @brief           HFP client callback function type
 *
 * @param           event : Event type
 *
 * @param           param : Pointer to callback parameter
 */
typedef void (* bk_bt_hf_client_cb_t)(bk_hf_client_cb_event_t event, bk_hf_client_cb_param_t *param);

/**
 * @brief           HFP client incoming data callback function
 *
 * @param[in]       buf : pointer to the data(payload of HCI synchronous data packet) received from HFP AG device
 *
 * @param[in]       len : size(in bytes) in buf
 */
typedef void (* bk_bt_hf_client_data_cb_t)(const uint8_t *buf, uint16_t len);


#ifdef __cplusplus
}
#endif
