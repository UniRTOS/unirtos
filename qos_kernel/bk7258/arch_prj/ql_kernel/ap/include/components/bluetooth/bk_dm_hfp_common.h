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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HF feature masks reported by the HF to the AG in AT+BRSF */
typedef enum {
    BK_HF_CLIENT_FEAT_ECNR              = (1 << 0),       /* Echo cancellation and/or noise reduction */
    BK_HF_CLIENT_FEAT_3WAY              = (1 << 1),       /* Three-way calling */
    BK_HF_CLIENT_FEAT_CLI_PRESENTATION  = (1 << 2),       /* CLI presentation capability */
    BK_HF_CLIENT_FEAT_VREC              = (1 << 3),       /* Voice recognition activation */
    BK_HF_CLIENT_FEAT_REMOTE_VOLUME     = (1 << 4),       /* Remote audio volume control */
    BK_HF_CLIENT_FEAT_ECS               = (1 << 5),       /* Enhanced call status */
    BK_HF_CLIENT_FEAT_ECC               = (1 << 6),       /* Enhanced call control */
    BK_HF_CLIENT_FEAT_CODEC             = (1 << 7),       /* Codec Negotiation */
    /* HFP 1.7+ */
    BK_HF_CLIENT_FEAT_HF_IND            = (1 << 8),       /* HF Indicators */
    BK_HF_CLIENT_FEAT_ESCO_S4           = (1 << 9),       /* eSCO S4 Settings Supported */
    /* HFP 1.8+ */
    BK_HF_CLIENT_FEAT_ENH_VR_STATUS     = (1 << 10),      /* Enhanced Voice Recognition Status */
    BK_HF_CLIENT_FEAT_VR_TEXT           = (1 << 11),      /* Voice Recognition Text */
} bk_hf_client_brsf_feat_t;

/* HF Client SDP feature mask */
typedef enum {
    BK_HF_CLIENT_SDP_FEAT_ECNR              = (1 << 0),   /* Echo cancellation and/or noise reduction */
    BK_HF_CLIENT_SDP_FEAT_3WAY              = (1 << 1),   /* Call waiting or three-way calling */
    BK_HF_CLIENT_SDP_FEAT_CLI_PRESENTATION  = (1 << 2),   /* CLI presentation capability */
    BK_HF_CLIENT_SDP_FEAT_VREC              = (1 << 3),   /* Voice recognition activation */
    BK_HF_CLIENT_SDP_FEAT_REMOTE_VOLUME     = (1 << 4),   /* Remote audio volume control */
    BK_HF_CLIENT_SDP_FEAT_WBS               = (1 << 5),   /* Wideband Speech */
    BK_HF_CLIENT_SDP_FEAT_ENH_VR_STATUS     = (1 << 6),   /* Enhanced Voice Recognition Status */
    BK_HF_CLIENT_SDP_FEAT_VR_TEXT           = (1 << 7),   /* Voice Recognition Text */
    BK_HF_CLIENT_SDP_FEAT_SWBS              = (1 << 8),   /* Super Wideband Speech */
} bk_hf_client_sdp_feat_t;


/* AG feature masks reported by the AG to the HF in the +BRSF response */
typedef enum {
    BK_HF_AG_FEAT_3WAY             = (1 << 0),        /* Three-way calling */
    BK_HF_AG_FEAT_ECNR             = (1 << 1),        /* Echo cancellation and/or noise reduction */
    BK_HF_AG_FEAT_VREC             = (1 << 2),        /* Voice recognition */
    BK_HF_AG_FEAT_INBAND           = (1 << 3),        /* In-band ring tone */
    BK_HF_AG_FEAT_VTAG             = (1 << 4),        /* Attach a phone number to a voice tag */
    BK_HF_AG_FEAT_REJECT           = (1 << 5),        /* Ability to reject incoming call */
    BK_HF_AG_FEAT_ECS              = (1 << 6),        /* Enhanced Call Status */
    BK_HF_AG_FEAT_ECC              = (1 << 7),        /* Enhanced Call Control */
    BK_HF_AG_FEAT_EXTERR           = (1 << 8),        /* Extended error codes */
    BK_HF_AG_FEAT_CODEC            = (1 << 9),        /* Codec Negotiation */
    /* HFP 1.7+ */
    BK_HF_AG_FEAT_HF_IND           = (1 << 10),       /* HF Indicators */
    BK_HF_AG_FEAT_ESCO_S4          = (1 << 11),       /* eSCO S4 Setting Supported */
    /* HFP 1.8+ */
    BK_HF_AG_FEAT_ENH_VR_STATUS    = (1 << 12),       /* Enhanced Voice Recognition Status */
    BK_HF_AG_FEAT_VR_TEXT          = (1 << 13),       /* Voice Recognition Text */
    /* HFP 1.10+ */
    BK_HF_AG_FEAT_CALL_DUR_INFO    = (1 << 14),       /* Call Duration Information */
} bk_hf_ag_brsf_feat_t;


/* HF AG SDP feature mask */
typedef enum {
    BK_HF_AG_SDP_FEAT_3WAY             = (1 << 0),        /* Three-way calling */
    BK_HF_AG_SDP_FEAT_ECNR             = (1 << 1),        /* Echo cancellation and/or noise reduction */
    BK_HF_AG_SDP_FEAT_VREC             = (1 << 2),        /* Voice recognition */
    BK_HF_AG_SDP_FEAT_INBAND           = (1 << 3),        /* In-band ring tone */
    BK_HF_AG_SDP_FEAT_VTAG             = (1 << 4),        /* Attach a phone number to a voice tag */
    BK_HF_AG_SDP_FEAT_WBS              = (1 << 5),        /* Wideband Speech */
    BK_HF_AG_SDP_FEAT_ENH_VR_STATUS    = (1 << 6),        /* Enhanced Voice Recognition Status */
    BK_HF_AG_SDP_FEAT_VR_TEXT          = (1 << 7),        /* Voice Recognition Text */
    BK_HF_AG_SDP_FEAT_SWBS             = (1 << 8),        /* Super Wideband Speech */
} bk_hf_ag_sdp_feat_t;

/* -------------------------------------------------------------------------- */
/*  Shared HFP enums - used by BOTH the HF-client and AG roles                 */
/* -------------------------------------------------------------------------- */

/// Bluetooth Assigned Numbers for HFP HF Indicators (AT+BIND / AT+BIEV)
typedef enum {
    BK_HF_IND_ID_ENHANCED_SAFETY = 1,              /*!< Enhanced Safety indicator */
    BK_HF_IND_ID_BATTERY_LEVEL   = 2,              /*!< Battery Level indicator */
} bk_hf_indicator_id_t;

/// AG call-hold / multiparty capabilities reported by AT+CHLD=?
typedef enum {
    BK_HF_CHLD_FEAT_REL          = (1 << 0),        /*!< 0: release waiting call or all held calls */
    BK_HF_CHLD_FEAT_REL_ACC      = (1 << 1),        /*!< 1: release active calls and accept waiting/held call */
    BK_HF_CHLD_FEAT_REL_X        = (1 << 2),        /*!< 1x: release the specified active call */
    BK_HF_CHLD_FEAT_HOLD_ACC     = (1 << 3),        /*!< 2: hold active calls and accept waiting/held call */
    BK_HF_CHLD_FEAT_PRIV_X       = (1 << 4),        /*!< 2x: private mode with the specified call */
    BK_HF_CHLD_FEAT_MERGE        = (1 << 5),        /*!< 3: add held call to multiparty */
    BK_HF_CHLD_FEAT_MERGE_DETACH = (1 << 6),        /*!< 4: connect two calls and leave multiparty */
} bk_hf_chld_feat_t;

/// AT+CHLD command values
typedef enum {
    BK_HF_CHLD_TYPE_REL = 0,               /*!< <0>, Terminate all held or set UDUB("busy") to a waiting call */
    BK_HF_CHLD_TYPE_REL_ACC,               /*!< <1>, Terminate all active calls and accepts a waiting/held call */
    BK_HF_CHLD_TYPE_HOLD_ACC,              /*!< <2>, Hold all active calls and accepts a waiting/held call */
    BK_HF_CHLD_TYPE_MERGE,                 /*!< <3>, Add all held calls to a conference */
    BK_HF_CHLD_TYPE_MERGE_DETACH,          /*!< <4>, connect the two calls and disconnects the subscriber from both calls */
} bk_hf_chld_type_t;

/// AT+BTRH response-and-hold action
typedef enum {
    BK_HF_BTRH_CMD_HOLD = 0,                        /*!< put the incoming call on hold */
    BK_HF_BTRH_CMD_ACCEPT,                          /*!< accept a held incoming call */
    BK_HF_BTRH_CMD_REJECT,                          /*!< reject a held incoming call */
} bk_hf_btrh_cmd_t;

/// +BTRH response-and-hold status
typedef enum {
    BK_HF_BTRH_STATUS_HELD = 0,                     /*!< incoming call is held in the AG */
    BK_HF_BTRH_STATUS_ACCEPTED,                     /*!< held incoming call was accepted */
    BK_HF_BTRH_STATUS_REJECTED,                     /*!< held incoming call was rejected */
} bk_hf_btrh_status_t;

/// Codec Type (air-mode codec reported by the controller)
typedef enum {
    CODEC_VOICE_CVSD = 0,     /*!< cvsd */
    CODEC_VOICE_MSBC,         /*!< msbc*/
    CODEC_VOICE_LC3,          /*!< lc3*/
    CODEC_VOICE_MAX,
} bk_hf_codec_type_t;

/// voice recognition state
typedef enum {
    BK_HF_VR_STATE_DISABLED = 0,           /*!< voice recognition disabled */
    BK_HF_VR_STATE_ENABLED,                /*!< voice recognition enabled */
} bk_hf_vr_state_t;

/// +CIND call status indicator values
typedef enum {
    BK_HF_CALL_STATUS_NO_CALLS = 0,                  /*!< no call in progress  */
    BK_HF_CALL_STATUS_CALL_IN_PROGRESS = 1,          /*!< call is present(active or held) */
} bk_hf_call_status_t;

/// +CIND call setup status indicator values
typedef enum {
    BK_HF_CALL_SETUP_STATUS_IDLE = 0,                /*!< no call setup in progress */
    BK_HF_CALL_SETUP_STATUS_INCOMING = 1,            /*!< incoming call setup in progress */
    BK_HF_CALL_SETUP_STATUS_OUTGOING_DIALING = 2,    /*!< outgoing call setup in dialing state */
    BK_HF_CALL_SETUP_STATUS_OUTGOING_ALERTING = 3,   /*!< outgoing call setup in alerting state */
} bk_hf_call_setup_status_t;

/// +CIND call held indicator values
typedef enum {
    BK_HF_CALL_HELD_STATUS_NONE = 0,                 /*!< no calls held */
    BK_HF_CALL_HELD_STATUS_HELD_AND_ACTIVE = 1,      /*!< both active and held call */
    BK_HF_CALL_HELD_STATUS_HELD = 2,                 /*!< call on hold, no active call*/
} bk_hf_call_held_status_t;

/// +CIND network service availability status
typedef enum
{
    BK_HF_NETWORK_STATE_NOT_AVAILABLE = 0,
    BK_HF_NETWORK_STATE_AVAILABLE
} bk_hf_network_state_t;

/// +CIND roaming status indicator values
typedef enum {
    BK_HF_ROAMING_STATUS_INACTIVE = 0,               /*!< roaming is not active */
    BK_HF_ROAMING_STATUS_ACTIVE,                     /*!< a roaming is active */
} bk_hf_roaming_status_t;

/// +CLCC status of the call
typedef enum {
    BK_HF_CURRENT_CALL_STATUS_ACTIVE = 0,            /*!< active */
    BK_HF_CURRENT_CALL_STATUS_HELD = 1,              /*!< held */
    BK_HF_CURRENT_CALL_STATUS_DIALING = 2,           /*!< dialing (outgoing calls only) */
    BK_HF_CURRENT_CALL_STATUS_ALERTING = 3,          /*!< alerting (outgoing calls only) */
    BK_HF_CURRENT_CALL_STATUS_INCOMING = 4,          /*!< incoming (incoming calls only) */
    BK_HF_CURRENT_CALL_STATUS_WAITING = 5,           /*!< waiting (incoming calls only) */
    BK_HF_CURRENT_CALL_STATUS_HELD_BY_RBK_HOLD = 6, /*!< call held by response and hold */
} bk_hf_current_call_status_t;

/// +CLCC direction of the call
typedef enum {
    BK_HF_CURRENT_CALL_DIRECTION_OUTGOING = 0,       /*!< outgoing */
    BK_HF_CURRENT_CALL_DIRECTION_INCOMING = 1,       /*!< incoming */
} bk_hf_current_call_direction_t;

/// +CLCC multi-party call flag
typedef enum {
    BK_HF_CURRENT_CALL_MPTY_TYPE_SINGLE = 0,         /*!< not a member of a multi-party call */
    BK_HF_CURRENT_CALL_MPTY_TYPE_MULTI = 1,          /*!< member of a multi-party call */
} bk_hf_current_call_mpty_type_t;

/// +CLCC mode (current call mode)
typedef enum {
    BK_HF_CURRENT_CALL_MODE_VOICE = 0,              /*!< voice call */
    BK_HF_CURRENT_CALL_MODE_DATA = 1,               /*!< data call */
    BK_HF_CURRENT_CALL_MODE_FAX = 2,                /*!< fax call */
} bk_hf_current_call_mode_t;

/// Number address type (3GPP Type-of-Address); HFP covers values 128-175.
typedef uint16_t bk_hf_call_addr_type_t;

enum {
    BK_HF_CALL_ADDR_TYPE_UNKNOWN_MIN       = 128,
    BK_HF_CALL_ADDR_TYPE_UNKNOWN           = 129,   /*!< common unknown/ISDN value */
    BK_HF_CALL_ADDR_TYPE_UNKNOWN_MAX       = 143,
    BK_HF_CALL_ADDR_TYPE_INTERNATIONAL_MIN = 144,
    BK_HF_CALL_ADDR_TYPE_INTERNATIONAL     = 145,   /*!< common international/ISDN value */
    BK_HF_CALL_ADDR_TYPE_INTERNATIONAL_MAX = 159,
    BK_HF_CALL_ADDR_TYPE_NATIONAL_MIN      = 160,
    BK_HF_CALL_ADDR_TYPE_NATIONAL          = 161,   /*!< common national/ISDN value */
    BK_HF_CALL_ADDR_TYPE_NATIONAL_MAX      = 175,
};

/// AT+NREC state
typedef enum {
    BK_HF_NREC_STOP = 0,                               /*!< NREC stop */
    BK_HF_NREC_START,                                  /*!< NREC start */
} bk_hf_nrec_t;

/// Bluetooth HFP audio volume control target
typedef enum {
    BK_HF_VOLUME_CONTROL_TARGET_SPK = 0,             /*!< speaker */
    BK_HF_VOLUME_CONTROL_TARGET_MIC,                 /*!< microphone */
} bk_hf_volume_control_target_t;

/* AT response code - OK/Error */
typedef enum {
    BK_HF_AT_RESPONSE_CODE_OK = 0,              /*!< acknowledges execution of a command line */
    BK_HF_AT_RESPONSE_CODE_ERR,                 /*!< command not accepted */
    BK_HF_AT_RESPONSE_CODE_NO_CARRIER,          /*!< connection terminated */
    BK_HF_AT_RESPONSE_CODE_BUSY,                /*!< busy signal detected */
    BK_HF_AT_RESPONSE_CODE_NO_ANSWER,           /*!< connection completion timeout */
    BK_HF_AT_RESPONSE_CODE_DELAYED,             /*!< delayed */
    BK_HF_AT_RESPONSE_CODE_BLACKLISTED,         /*!< blacklisted */
    BK_HF_AT_RESPONSE_CODE_CME,                 /*!< CME error */
    BK_HF_AT_RESPONSE_CODE_REMOVE_FROM_NETWORK, /*!< remove from network */
} bk_hf_at_response_code_t;

/// Extended Audio Gateway Error Result Code Response
typedef enum {
    BK_HF_CME_AG_FAILURE = 0,                           /*!< ag failure */
    BK_HF_CME_NO_CONNECTION_TO_PHONE = 1,               /*!< no connection to phone */
    BK_HF_CME_OPERATION_NOT_ALLOWED = 3,                /*!< operation not allowed */
    BK_HF_CME_OPERATION_NOT_SUPPORTED = 4,              /*!< operation not supported */
    BK_HF_CME_PH_SIM_PIN_REQUIRED = 5,                  /*!< PH-SIM PIN Required */
    BK_HF_CME_SIM_NOT_INSERTED = 10,                    /*!< SIM not inserted */
    BK_HF_CME_SIM_PIN_REQUIRED = 11,                    /*!< SIM PIN required */
    BK_HF_CME_SIM_PUK_REQUIRED = 12,                    /*!< SIM PUK required */
    BK_HF_CME_SIM_FAILURE = 13,                         /*!< SIM failure */
    BK_HF_CME_SIM_BUSY = 14,                            /*!< SIM busy */
    BK_HF_CME_INCORRECT_PASSWORD = 16,                  /*!< incorrect password */
    BK_HF_CME_SIM_PIN2_REQUIRED = 17,                   /*!< SIM PIN2 required */
    BK_HF_CME_SIM_PUK2_REQUIRED = 18,                   /*!< SIM PUK2 required */
    BK_HF_CME_MEMORY_FULL = 20,                         /*!< memory full */
    BK_HF_CME_INVALID_INDEX = 21,                       /*!< invalid index */
    BK_HF_CME_MEMORY_FAILURE = 23,                      /*!< memory failure */
    BK_HF_CME_TEXT_STRING_TOO_LONG = 24,                /*!< test string too long */
    BK_HF_CME_INVALID_CHARACTERS_IN_TEXT_STRING = 25,   /*!< invalid characters in text string */
    BK_HF_CME_DIAL_STRING_TOO_LONG = 26,                /*!< dial string too long*/
    BK_HF_CME_INVALID_CHARACTERS_IN_DIAL_STRING = 27,   /*!< invalid characters in dial string */
    BK_HF_CME_NO_NETWORK_SERVICE = 30,                  /*!< no network service */
    BK_HF_CME_NETWORK_TIMEOUT = 31,                     /*!< network timeout */
    BK_HF_CME_NETWORK_NOT_ALLOWED = 32,                 /*!< network not allowed --emergency calls only */
} bk_hf_cme_err_t;

/// +CNUM service type of the phone number
typedef enum {
    BK_HF_SUBSCRIBER_SERVICE_TYPE_UNKNOWN = 0,      /*!< unknown */
    BK_HF_SUBSCRIBER_SERVICE_TYPE_VOICE,            /*!< voice service */
    BK_HF_SUBSCRIBER_SERVICE_TYPE_FAX,              /*!< fax service */
} bk_hf_subscriber_service_type_t;

/// Audio coding format, used in air payload
typedef enum {
    BK_BT_AUDIO_CODING_FORMAT_ULAW = 0,
    BK_BT_AUDIO_CODING_FORMAT_ALAW,
    BK_BT_AUDIO_CODING_FORMAT_CVSD,
    BK_BT_AUDIO_CODING_FORMAT_TRANSPARENT,
    BK_BT_AUDIO_CODING_FORMAT_LINEAR_PCM,
    BK_BT_AUDIO_CODING_FORMAT_MSBC, // hfp msbc use BK_BT_AUDIO_CODING_FORMAT_TRANSPARENT instead of this.
    BK_BT_AUDIO_CODING_FORMAT_LC3,  // hfp lc3 use BK_BT_AUDIO_CODING_FORMAT_TRANSPARENT instead of this.
    BK_BT_AUDIO_CODING_FORMAT_G729A,
    BK_BT_AUDIO_CODING_FORMAT_VENDOR = 0xff,
} bk_bt_audio_coding_format_t;

#ifdef __cplusplus
}
#endif
