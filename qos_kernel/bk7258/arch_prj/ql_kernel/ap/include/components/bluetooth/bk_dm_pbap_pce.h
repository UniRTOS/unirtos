/*
 * Copyright 2020-2021 Beken
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
 *
 * PBAP PCE (Phone Book Access Profile - Phone Book Client Equipment) API.
 * Aligned with Bluetooth PBAP specification v1.2.3.
 */

#pragma once

#include "bk_dm_bluetooth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants                                                                   */
/* -------------------------------------------------------------------------- */

#define BK_PBAP_PCE_MAX_OBJECT_NAME_LEN    64U   /*!< Max length of phonebook/vcard object path */
#define BK_PBAP_PCE_MAX_PIN_LEN            16U   /*!< Max OBEX PIN size */
#define BK_PBAP_PCE_MAX_USER_ID_LEN        32U   /*!< Max user ID length for auth */
#define BK_PBAP_PCE_DEFAULT_MAX_RECV_SIZE  600U  /*!< Default OBEX max receive size */

/* -------------------------------------------------------------------------- */
/* Supported Repositories (Uint8) – PBAP server supported repositories        */
/* -------------------------------------------------------------------------- */
#define BK_PBAP_PCE_REPO_LOCAL_PHONEBOOK   (1U << 0)  /*!< Bit 0: Local Phonebook */
#define BK_PBAP_PCE_REPO_SIM               (1U << 1)  /*!< Bit 1: SIM card */
#define BK_PBAP_PCE_REPO_SPEED_DIAL        (1U << 2)  /*!< Bit 2: Speed dial */
#define BK_PBAP_PCE_REPO_FAVORITES         (1U << 3)  /*!< Bit 3: Favorites */
/* Bits 4–7 reserved for future use */

/* -------------------------------------------------------------------------- */
/* PbapSupportedFeatures (Uint32) – PBAP server supported features            */
/* -------------------------------------------------------------------------- */
#define BK_PBAP_PCE_FEAT_DOWNLOAD              (1U << 0)   /*!< Bit 0: Download */
#define BK_PBAP_PCE_FEAT_BROWSING              (1U << 1)   /*!< Bit 1: Browsing */
#define BK_PBAP_PCE_FEAT_DATABASE_IDENTIFIER   (1U << 2)   /*!< Bit 2: Database Identifier */
#define BK_PBAP_PCE_FEAT_FOLDER_VERSION_COUNTERS (1U << 3) /*!< Bit 3: Folder Version Counters */
#define BK_PBAP_PCE_FEAT_VCARD_SELECTING       (1U << 4)   /*!< Bit 4: vCard Selecting */
#define BK_PBAP_PCE_FEAT_ENHANCED_MISSED_CALLS (1U << 5)   /*!< Bit 5: Enhanced Missed Calls */
#define BK_PBAP_PCE_FEAT_X_BT_UCI_VCARD       (1U << 6)   /*!< Bit 6: X-BT-UCI vCard Property */
#define BK_PBAP_PCE_FEAT_X_BT_UID_VCARD       (1U << 7)   /*!< Bit 7: X-BT-UID vCard Property */
#define BK_PBAP_PCE_FEAT_CONTACT_REFERENCING  (1U << 8)   /*!< Bit 8: Contact Referencing */
#define BK_PBAP_PCE_FEAT_DEFAULT_CONTACT_IMAGE (1U << 9)   /*!< Bit 9: Default Contact Image Format */
/* Bits 10–31 reserved for future use */

/* -------------------------------------------------------------------------- */
/* Status                                                                     */
/* -------------------------------------------------------------------------- */

typedef enum {
    BK_PBAP_PCE_SUCCESS           = 0,   /*!< Operation successful (OBEX Success) */
    BK_PBAP_PCE_FAILURE,                /*!< Generic failure */
    BK_PBAP_PCE_BUSY,                  /*!< Temporarily cannot handle this request */
    BK_PBAP_PCE_NO_RESOURCE,           /*!< No more resource */
    BK_PBAP_PCE_NEED_INIT,             /*!< PBAP PCE module must be initialized first */
    BK_PBAP_PCE_NEED_DEINIT,           /*!< PBAP PCE module must be deinitialized first */
    BK_PBAP_PCE_NO_CONNECTION,         /*!< Not connected to PSE */
    BK_PBAP_PCE_INVALID_STATE,         /*!< Invalid state */
    BK_PBAP_PCE_INVALID_PARAM,         /*!< Invalid parameter */
    BK_PBAP_PCE_CONTINUE,              /*!< Continue (OBEX Continue - more data to follow) */
    BK_PBAP_PCE_BAD_REQUEST,           /*!< Bad request (OBEX) */
    BK_PBAP_PCE_UNAUTHORIZED,          /*!< Unauthorized - authentication required (OBEX) */
    BK_PBAP_PCE_NOT_FOUND,             /*!< Not found (OBEX) */
    BK_PBAP_PCE_FORBIDDEN,             /*!< Forbidden (OBEX) */
    BK_PBAP_PCE_IDLE_TIMEOUT,          /*!< Idle timeout */
} bk_pbap_pce_status_t;

/* -------------------------------------------------------------------------- */
/* Callback events (PBAP PCE notifications)                                   */
/* -------------------------------------------------------------------------- */

typedef enum {
    BK_PBAP_PCE_INIT_EVT                = 0,   /*!< PCE module initialized */
    BK_PBAP_PCE_DEINIT_EVT              = 1,   /*!< PCE module deinitialized */
    BK_PBAP_PCE_CONNECT_CFM_EVT         = 2,   /*!< Connect to PSE confirmed */
    BK_PBAP_PCE_DISCONNECT_CFM_EVT      = 3,   /*!< Disconnect confirmed */
    BK_PBAP_PCE_GET_PHONEBOOK_CFM_EVT   = 4,   /*!< Get phonebook confirmed; body/params in param */
    BK_PBAP_PCE_GET_VCARD_LIST_CFM_EVT  = 5,   /*!< Get vcard list confirmed */
    BK_PBAP_PCE_GET_VCARD_CFM_EVT       = 6,   /*!< Get vcard confirmed */
    BK_PBAP_PCE_SET_PHONEBOOK_CFM_EVT   = 7,   /*!< Set phonebook (path) confirmed */
    BK_PBAP_PCE_GET_SIZE_CFM_EVT        = 8,   /*!< Get size confirmed */
    BK_PBAP_PCE_ABORT_CFM_EVT          = 9,   /*!< Abort confirmed */
} bk_pbap_pce_cb_event_t;

/* -------------------------------------------------------------------------- */
/* Connect parameters                                                         */
/* -------------------------------------------------------------------------- */

/**
 * Connect parameters (filled by application before bk_bt_pbap_pce_connect).
 *   bd_addr        - Remote PSE device address (e.g. from pairing list).
 *   auth_required  - 1 if PSE requires OBEX auth, else 0.
 *   pin / pin_len  - OBEX PIN when auth_required == 1 (e.g. "0000").
 *   user_id / len  - Optional User ID for auth (NULL if not needed).
 *   max_recv_size  - OBEX max receive size; use BK_PBAP_PCE_DEFAULT_MAX_RECV_SIZE if unsure.
 */
typedef struct {
    bk_bd_addr_t     bd_addr;           /*!< Remote PSE Bluetooth address */
    uint8_t          auth_required;     /*!< 1 = OBEX auth required, 0 = no auth */
    const uint8_t   *pin;               /*!< OBEX PIN (optional if auth_required == 0) */
    uint16_t         pin_len;           /*!< Length of pin (max BK_PBAP_PCE_MAX_PIN_LEN) */
    const uint8_t   *user_id;           /*!< User ID for auth (optional) */
    uint16_t         user_id_len;       /*!< Length of user_id (max BK_PBAP_PCE_MAX_USER_ID_LEN) */
    uint16_t         max_recv_size;     /*!< OBEX max receive size (e.g. BK_PBAP_PCE_DEFAULT_MAX_RECV_SIZE) */
} bk_pbap_pce_connect_param_t;

/* -------------------------------------------------------------------------- */
/* Set path (SetPhonebook)                                                    */
/* -------------------------------------------------------------------------- */

typedef enum {
    BK_PBAP_PCE_SET_PATH_ROOT   = 0,    /*!< Set path to root (e.g. "telecom") */
    BK_PBAP_PCE_SET_PATH_PARENT = 1,    /*!< Set path to parent folder */
    BK_PBAP_PCE_SET_PATH_CHILD  = 2,    /*!< Set path to child folder; use path_name */
} bk_pbap_pce_set_path_type_t;

typedef struct {
    bk_pbap_pce_set_path_type_t path_type;  /*!< Root, parent, or child */
    const char                 *path_name;  /*!< Folder/object name (required for CHILD only) */
} bk_pbap_pce_set_path_param_t;

/* -------------------------------------------------------------------------- */
/* Get phonebook / vcard list / vcard parameters                              */
/* -------------------------------------------------------------------------- */

/**
 * vCard property filter bits (filter / vcard_selector).
 * Use these when setting get_phonebook_param.filter, get_vcard_param.filter,
 * or get_vcard_list_param.vcard_selector (0 = no filter / use default).
 *
 * Bit 0  VERSION     - vCard Version
 * Bit 1  FN          - Formatted Name
 * Bit 2  N           - Structured Presentation of Name
 * Bit 3  PHOTO       - Associated Image or Photo
 * Bit 4  BDAY        - Birthday
 * Bit 5  ADR         - Delivery Address
 * Bit 6  LABEL       - Delivery
 * Bit 7  TEL         - Telephone Number
 * Bit 8  EMAIL       - Electronic Mail Address
 * Bit 9  MAILER      - Electronic Mail
 * Bit 10 TZ          - Time Zone
 * Bit 11 GEO         - Geographic Position
 * Bit 12 TITLE       - Job
 * Bit 13 ROLE        - Role within the Organization
 * Bit 14 LOGO        - Organization Logo
 * Bit 15 AGENT       - vCard of Person Representing
 * Bit 16 ORG         - Name of Organization
 * Bit 17 NOTE        - Comments
 * Bit 18 REV         - Revision
 * Bit 19 SOUND       - Pronunciation of Name
 * Bit 20 URL         - Uniform Resource Locator
 * Bit 21 UID         - Unique ID
 * Bit 22 KEY         - Public Encryption Key
 * Bit 23 NICKNAME    - Nickname
 * Bit 24 CATEGORIES  - Categories
 * Bit 25 PROID       - Product ID
 * Bit 26 CLASS       - Class information
 * Bit 27 SORT_STRING - String used for sorting operations
 * Bit 28 X_IRMC_CALL_DATETIME - Time stamp
 * Bit 29 X_BT_SPEEDDIALKEY - Speed-dial shortcut
 * Bit 30 X_BT_UCI    - Uniform Caller Identifier
 * Bit 31 X_BT_UID    - Bluetooth Contact Unique Identifier
 * Bit 32~38          - Reserved for future use
 * Bit 39             - Proprietary Filter Indicates the usage of a proprietary filter
 * Bit 40~63          - Reserved for proprietary filter usage
 */
#define BK_PBAP_PCE_FILTER_VCARD_VERSION       (1ULL << 0)   /* VERSION */
#define BK_PBAP_PCE_FILTER_FN                 (1ULL << 1)   /* Formatted Name */
#define BK_PBAP_PCE_FILTER_N                  (1ULL << 2)  /* Structured Name */
#define BK_PBAP_PCE_FILTER_PHOTO              (1ULL << 3)   /* Photo */
#define BK_PBAP_PCE_FILTER_BDAY               (1ULL << 4)   /* Birthday */
#define BK_PBAP_PCE_FILTER_ADR                (1ULL << 5)   /* Delivery Address */
#define BK_PBAP_PCE_FILTER_LABEL              (1ULL << 6)   /* Label */
#define BK_PBAP_PCE_FILTER_TEL                (1ULL << 7)   /* Telephone Number */
#define BK_PBAP_PCE_FILTER_EMAIL              (1ULL << 8)   /* Email */
#define BK_PBAP_PCE_FILTER_MAILER             (1ULL << 9)   /* Mailer */
#define BK_PBAP_PCE_FILTER_TZ                 (1ULL << 10)  /* Time Zone */
#define BK_PBAP_PCE_FILTER_GEO                (1ULL << 11)  /* Geographic Position */
#define BK_PBAP_PCE_FILTER_TITLE              (1ULL << 12)  /* Job/Title */
#define BK_PBAP_PCE_FILTER_ROLE               (1ULL << 13)  /* Role */
#define BK_PBAP_PCE_FILTER_LOGO               (1ULL << 14)  /* Organization Logo */
#define BK_PBAP_PCE_FILTER_AGENT              (1ULL << 15)  /* Agent */
#define BK_PBAP_PCE_FILTER_ORG                (1ULL << 16)  /* Organization */
#define BK_PBAP_PCE_FILTER_NOTE               (1ULL << 17)  /* Note */
#define BK_PBAP_PCE_FILTER_REV                (1ULL << 18)  /* Revision */
#define BK_PBAP_PCE_FILTER_SOUND              (1ULL << 19)  /* Pronunciation */
#define BK_PBAP_PCE_FILTER_URL                (1ULL << 20)  /* URL */
#define BK_PBAP_PCE_FILTER_UID                (1ULL << 21)  /* Unique ID */
#define BK_PBAP_PCE_FILTER_KEY                (1ULL << 22)  /* Public Key */
#define BK_PBAP_PCE_FILTER_NICKNAME           (1ULL << 23)  /* Nickname */
#define BK_PBAP_PCE_FILTER_CATEGORIES         (1ULL << 24)  /* Categories */
#define BK_PBAP_PCE_FILTER_PROID              (1ULL << 25)  /* Product ID */
#define BK_PBAP_PCE_FILTER_CLASS              (1ULL << 26)  /* Class */
#define BK_PBAP_PCE_FILTER_SORT_STRING        (1ULL << 27)  /* Sort string */
#define BK_PBAP_PCE_FILTER_X_IRMC_CALL_DATETIME (1ULL << 28) /* Call datetime */
#define BK_PBAP_PCE_FILTER_X_BT_SPEEDDIALKEY  (1ULL << 29)  /* Speed-dial */
#define BK_PBAP_PCE_FILTER_X_BT_UCI           (1ULL << 30)  /* Uniform Caller ID */
#define BK_PBAP_PCE_FILTER_X_BT_UID           (1ULL << 31)  /* BT Contact UID */
#define BK_PBAP_PCE_FILTER_PROPRIETARY        (1ULL << 39)  /* Proprietary filter usage */

/** Protocol default when Filter is omitted (PBAP 1.2.3): VERSION, FN, N, TEL, X-IRMC-CALL-DATETIME */
#define BK_PBAP_PCE_FILTER_DEFAULT            (0x10000087ULL)

/** vCard selector operator (vcard_selector_operator) */
#define BK_PBAP_PCE_VCARD_SELECTOR_OP_OR      0  /* OR */
#define BK_PBAP_PCE_VCARD_SELECTOR_OP_AND     1  /* AND */

/** vCard format (format field) */
#define BK_PBAP_PCE_FORMAT_2_1                0
#define BK_PBAP_PCE_FORMAT_3_0                1

/** Sorting order (order field, get_vcard_list_param) */
#define BK_PBAP_PCE_ORDER_INDEXED             0
#define BK_PBAP_PCE_ORDER_ALPHABETICAL        1
#define BK_PBAP_PCE_ORDER_PHONETICAL          2

/** Reset new missed calls (reset_new_missed_calls, for mch/cch) */
#define BK_PBAP_PCE_RESET_NEW_MISSED_CALLS_NO  0
#define BK_PBAP_PCE_RESET_NEW_MISSED_CALLS_YES 1

/**
 * Get-request parameters (PBAP 1.2.3 aligned).
 * Use the union member matching the API: get_phonebook (PullPhonebook), get_vcard_list (GetvCardListing),
 * get_vcard (GetvCard), get_size (GetPhonebookSize). Each struct contains only the parameters valid for that operation.
 */
typedef union {
    struct {
        const char  *object_name;              /*!< Object path, e.g. "telecom/pb.vcf", "SIM1/telecom/ich.vcf" */
        uint16_t     max_list_count;           /*!< Max entries. 0 = default; else max count to return */
        uint16_t     list_start_offset;        /*!< Start offset (0 = from start) */
        uint64_t     filter;                   /*!< Property selector. 0 = use default (BK_PBAP_PCE_FILTER_DEFAULT). Use BK_PBAP_PCE_FILTER_* */
        uint8_t      format;                   /*!< vCard format. 0 = vCard 2.1 (default), 1 = vCard 3.0 */
        uint8_t      reset_new_missed_calls;   /*!< Reset new missed calls (mch/cch): _NO / _YES , shall only be used for the folders mch and cch*/
        uint64_t     vcard_selector;           /*!< vCard selector (0 = none). Use BK_PBAP_PCE_FILTER_* */
        uint8_t      vcard_selector_operator;  /*!< BK_PBAP_PCE_VCARD_SELECTOR_OP_OR / _AND */
    } get_phonebook_param;
    struct {
        const char  *object_name;              /*!< Name: folder name relative to current path. E.g. "pb", "ich" */
        uint16_t     max_list_count;           /*!< Max entries. 0 = default; else max count to return */
        uint16_t     list_start_offset;        /*!< Start offset (0 = from start) */
        uint8_t      order;                    /*!< Sort order: BK_PBAP_PCE_ORDER_* */
        uint8_t      reset_new_missed_calls;   /*!< Reset new missed calls (mch/cch): _NO / _YES */
        uint64_t     vcard_selector;           /*!< vCard selector (0 = none). Use BK_PBAP_PCE_FILTER_* */
        uint8_t      vcard_selector_operator;  /*!< BK_PBAP_PCE_VCARD_SELECTOR_OP_OR / _AND */
    } get_vcard_list_param;
    struct {
        const char  *object_name;              /*!< Object name, e.g. "0.vcf" or "X-BT-UID:*" */
        uint64_t     filter;                   /*!< Property selector. 0 = use default (BK_PBAP_PCE_FILTER_DEFAULT). Use BK_PBAP_PCE_FILTER_* */
        uint8_t      format;                   /*!< vCard format. 0 = vCard 2.1 (default, BK_PBAP_PCE_FORMAT_2_1), 1 = vCard 3.0 */
    } get_vcard_param;
    struct {
        const char  *object_name;              /*!< Object path for size query, e.g. "telecom/pb.vcf", "telecom/mch.vcf" */
    } get_size_param;
} bk_pbap_pce_get_param_t;

/* -------------------------------------------------------------------------- */
/* Callback parameter union                                                   */
/* -------------------------------------------------------------------------- */

typedef union {
    struct {
        bk_pbap_pce_status_t status;
    } init;

    struct {
        bk_pbap_pce_status_t status;
    } uninit;

    struct {
        bk_pbap_pce_status_t status;
        bk_bd_addr_t        bd_addr;
        uint8_t             remote_supported_repositories; /*!< Remote PSE supported repositories */
        uint32_t            remote_supported_features; /*!< Remote PSE supported features */
    } connect_cfm;

    struct {
        bk_pbap_pce_status_t status;
        bk_bd_addr_t        bd_addr;      /*!< Remote PSE address pointer */
    } disconnect_cfm;

    struct {
        bk_pbap_pce_status_t status;
        bk_bd_addr_t        bd_addr;      /*!< Remote PSE address pointer */
        uint8_t             *data;        /*!< Phonebook data */
        uint16_t            data_len;     /*!< Phonebook data length */
    } get_phonebook_cfm;

    struct {
        bk_pbap_pce_status_t status;
        bk_bd_addr_t        bd_addr;
        uint8_t             *data;        /*!< vCard list data */
        uint16_t            data_len;     /*!< vCard list data length */
    } get_vcard_list_cfm;

    struct {
        bk_pbap_pce_status_t status;
        bk_bd_addr_t        bd_addr;
        uint8_t             *data;        /*!< vCard data */
        uint16_t            data_len;     /*!< vCard data length */
    } get_vcard_cfm;

    struct {
        bk_pbap_pce_status_t status;
        bk_bd_addr_t        bd_addr;
    } set_phonebook_cfm;

    struct {
        bk_pbap_pce_status_t status;
        bk_bd_addr_t        bd_addr;
        uint16_t            phonebook_size;  /*!< Number of entries (from PSE) */
    } get_size_cfm;

    struct {
        bk_pbap_pce_status_t status;
        bk_bd_addr_t        bd_addr;
    } abort_cfm;
} bk_pbap_pce_cb_param_t;

/**
 * @brief PBAP PCE event callback type.
 *
 * @param event  Event type (bk_pbap_pce_cb_event_t).
 * @param param  Event-specific parameters.
 */
typedef void (*bk_pbap_pce_cb_t)(bk_pbap_pce_cb_event_t event, bk_pbap_pce_cb_param_t *param);

/* -------------------------------------------------------------------------- */
/* API: Lifecycle                                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Register the PBAP PCE event callback.
 *        Should be called before bk_bt_pbap_pce_init().
 *
 * @param callback  Callback for PCE events.
 * @return bt_err_t BK_OK on success, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_register_callback(bk_pbap_pce_cb_t callback);

/**
 * @brief Initialize the PBAP PCE module.
 *        Call after bluetooth stack is enabled. On completion, callback is
 *        invoked with BK_PBAP_PCE_INIT_EVT.
 *
 * @return bt_err_t BK_OK on success, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_init(void);

/**
 * @brief Deinitialize the PBAP PCE module.
 *        Closes all connections; callback is invoked with BK_PBAP_PCE_DEINIT_EVT.
 *
 * @return bt_err_t BK_OK on success, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_deinit(void);

/* -------------------------------------------------------------------------- */
/* API: Connection                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Connect to a remote PBAP PSE. ACL must exist; SDK obtains RFCOMM channel
 *        internally via SDP. On result, callback receives BK_PBAP_PCE_CONNECT_CFM_EVT.
 *        If status is BK_PBAP_PCE_UNAUTHORIZED, application should call
 *        bk_bt_pbap_pce_send_auth_response() with PIN/user ID.
 *
 * @param param    Connection parameters (bd_addr, auth, pin, max_recv_size, etc.).
 * @return bt_err_t BK_OK if connect request accepted, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_connect(const bk_pbap_pce_connect_param_t *param);

/**
 * @brief Disconnect the PBAP session from the PSE.
 *        Callback receives BK_PBAP_PCE_DISCONNECT_CFM_EVT.
 *
 * @param bd_addr  Remote PSE address pointer (6 bytes).
 * @return bt_err_t BK_OK on success, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_disconnect(const uint8_t *bd_addr);

/* -------------------------------------------------------------------------- */
/* API: Pull operations (PBAP 1.2.3)                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Pull phonebook object (e.g. pb.vcf, ich.vcf, och.vcf, mch.vcf).
 *        Callback receives BK_PBAP_PCE_GET_PHONEBOOK_CFM_EVT with body/params in param->headers.
 *
 * @param bd_addr    Remote PSE address pointer (6 bytes).
 * @param get_param  Object name and optional max_list_count, offset, filter.
 * @return bt_err_t BK_OK if request accepted, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_get_phonebook(const uint8_t *bd_addr, const bk_pbap_pce_get_param_t *get_param);

/**
 * @brief Set current path (SetPhonebook) to root, parent, or child folder.
 *        Callback receives BK_PBAP_PCE_SET_PHONEBOOK_CFM_EVT.
 *
 * @param bd_addr    Remote PSE address pointer (6 bytes).
 * @param set_param  Path type and optional path_name for child.
 * @return bt_err_t BK_OK if request accepted, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_set_phonebook(const uint8_t *bd_addr, const bk_pbap_pce_set_path_param_t *set_param);

/**
 * @brief Pull vCard listing (e.g. telecom/ich.vcf listing).
 *        Callback receives BK_PBAP_PCE_GET_VCARD_LIST_CFM_EVT.
 *
 * @param bd_addr    Remote PSE address pointer (6 bytes).
 * @param get_param  Object name and optional list/filter params.
 * @return bt_err_t BK_OK if request accepted, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_get_vcard_list(const uint8_t *bd_addr, const bk_pbap_pce_get_param_t *get_param);

/**
 * @brief Pull a single vCard (e.g. by handle or path).
 *        Callback receives BK_PBAP_PCE_GET_VCARD_CFM_EVT.
 *
 * @param bd_addr    Remote PSE address pointer (6 bytes).
 * @param get_param  Object name (path/handle) and optional filter.
 * @return bt_err_t BK_OK if request accepted, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_get_vcard(const uint8_t *bd_addr, const bk_pbap_pce_get_param_t *get_param);

/**
 * @brief Abort the current pull/set operation.
 *        Callback receives BK_PBAP_PCE_ABORT_CFM_EVT.
 *
 * @param bd_addr  Remote PSE address pointer (6 bytes).
 * @return bt_err_t BK_OK on success, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_abort(const uint8_t *bd_addr);

/**
 * @brief Get the size of the phonebook or vcard list.
 *        Callback receives BK_PBAP_PCE_GET_SIZE_CFM_EVT.
 *
 * @param bd_addr    Remote PSE address pointer (6 bytes).
 * @param get_param  get_size_param: object_name only (e.g. "telecom/pb.vcf", "telecom/mch.vcf").
 * @return bk_pbap_pce_status_t BK_PBAP_PCE_SUCCESS if request accepted, otherwise error.
 */
bk_pbap_pce_status_t bk_bt_pbap_pce_get_size(const uint8_t *bd_addr, const bk_pbap_pce_get_param_t *get_param);

/* -------------------------------------------------------------------------- */
/* API: Authentication (response to UNAUTHORIZED)                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Send authentication response after BK_PBAP_PCE_UNAUTHORIZED in
 *        BK_PBAP_PCE_CONNECT_CFM_EVT. Pass PIN and optionally user_id.
 *
 * @param bd_addr      Remote PSE address pointer (6 bytes).
 * @param pin          OBEX PIN (binary, up to BK_PBAP_PCE_MAX_PIN_LEN).
 * @param pin_len      Length of pin.
 * @param user_id      Optional user ID (can be NULL).
 * @param user_id_len  Length of user_id (0 if user_id is NULL).
 * @return bt_err_t BK_OK on success, otherwise error code.
 */
bt_err_t bk_bt_pbap_pce_send_auth_response(const uint8_t *bd_addr,
                                        const uint8_t *pin,
                                        uint16_t pin_len,
                                        const uint8_t *user_id,
                                        uint16_t user_id_len);

#ifdef __cplusplus
}
#endif
