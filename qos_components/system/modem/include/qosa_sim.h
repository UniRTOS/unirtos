/*****************************************************************/ /**
* @file qosa_sim.h
* @brief SIM card operation interface
* @author Joe.tu@quectel.com
* @date 2024-03-14
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-03-14 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_SIM_H__
#define __QOSA_SIM_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sim_config.h"
#include "qosa_modem.h"

/**
 * @note Explanation of the correspondence between application layer simid and slotid in dual card mode
 *       By default, the correspondence between application layer simid and slotid is as follows:
 *       +-------------------+     +-------------------+
 *       |    Application layer simid 0  |     |   Application layer simid 1   |
 *       +-------------------+     +-------------------+
 *               |                       |
 *               v                       v
 *       +-------------------+     +-------------------+
 *       | Card slot 1 (slotid 0)  |     | Card slot 2 (slotid 1) |
 *       +-------------------+     +-------------------+
 *       The correspondence between application layer simid and card slot slotid is one-to-one, application layer simid 0 corresponds to card slot slotid 0, application layer simid 1 corresponds to card slot slotid 1.
 *

 *       If card slot switching occurs, usually through the osa_sim_set_slot_id(0,1) function for card slot switching operation, the correspondence between application layer simid and card slot slotid will also change.
 *       +-------------------+     +-------------------+
 *       |    Application layer simid 0  |     |   Application layer simid 1   |
 *       +-------------------+     +-------------------+
 *               |                       |
 *               v                       v
 *       +-------------------+     +-------------------+
 *       | Card slot 2 (slotid 1)  |     | Card slot 1 (slotid 0) |
 *       +-------------------+     +-------------------+
 *       The correspondence between application layer simid 0 and card slot slotid 1 has changed, application layer simid 0 now corresponds to card slot slotid 1, application layer simid 1 corresponds to card slot slotid 0.
 *
 * @note It should be noted that in dual card single standby scenarios, application layer cannot see simid 1, that is, application layer can only use simid 0, in this case, if you want to get the presence status of the other card, you can access the other card's status through slot related interfaces.
 */

/** Maximum length of SIM code */
#define QOSA_SIM_PIN_LEN_MAX               (8)

/** Maximum length of C-APDU */
#define QOSA_SIM_C_APDU_LEN_MAX            (261)

/** Minimum length of C-APDU */
#define QOSA_SIM_C_APDU_LEN_MIN            (4)

/** Maximum length of C-APDU data field */
#define QOSA_SIM_C_APDU_DATA_LEN_MAX       (255)

/** Maximum length of R-APDU */
#define QOSA_SIM_R_APDU_LEN_MAX            (1026)

/** Maximum length of R-APDU data field */
#define QOSA_SIM_R_APDU_DATA_LEN_MAX       (1024)

/** Minimum length of R-APDU */
#define QOSA_SIM_R_APDU_LEN_MIN            (2)

/** Maximum length of path id */
#define QOSA_SIM_PATH_ID_LEN_MAX           (8)

/** the length of iccid */
#define QOSA_SIM_ICCID_LEN                 (20)

/** Maximum length of IMSI, although specification documents define it as 15 digits, considering potential risks, here it is defined as 16 digits based on the size of SIM card files */
#define QOSA_SIM_IMSI_LEN_MAX              (16)

/** Maximum number of phone numbers in SIM card */
#define QOSA_SIM_PHONENUMBER_MAX_NUM       (4)

/** Maximum length of phone number in SIM card */
#define QOSA_SIM_PHONENUMBER_MAX_LEN       (24)

/** Maximum length of AID */
#define QOSA_SIM_AID_LEN_MAX               (16)

/** SIM card function block initialization status */
#define QOSA_SIMINI_STAT_START             (0x00) /*!< SIM card initialization start status */
#define QOSA_SIMINI_STAT_CPIN_READY        (0x01) /*!< SIM card PIN code ready status */
#define QOSA_SIMINI_STAT_SMS_DONE          (0x02) /*!< SMS function initialization completion status */
#define QOSA_SIMINI_STAT_PB_DONE           (0x04) /*!< Phone book function initialization completion status */

/** Hot swap detection GPIO cannot be modified */
#define QOSA_SIM_HOT_SWAP_UNSPECIFIED_GPIO (0xffffffff) /*!< Hot swap detection uses product default configuration pins, cannot be modified */

/** The maximum number of files queried by writesim */
#define QOSA_SIM_WRITE_SIM_FILE_COUNT_MAX  (8)

/**
 * @brief SIM card callback function pointer type definition
 *
 * This type defines the general callback function format for SIM card related operations, used for result notification after asynchronous operations are completed.
 *
 * @param [in] ctx
 *           - User context pointer, passed in by the caller
 * @param [in] argv
 *           - Callback parameter pointer, pointing to the structure containing operation results
 */
typedef void (*sim_callback_ptr)(void *ctx, void *argv);

/** SIM card error code base value */
#define QOSA_ERRCODE_SIM_BASE (QOSA_COMPONENT_SIM << 16) /*!< SIM card module error code base value, used to generate specific error codes */

// clang-format off
/**
 * @enum qosa_sim_err_e
 * @brief SIM error codes
 *
 */
typedef enum sim_err
{
    QOSA_SIM_ERR_OK = 0,

    QOSA_SIM_ERR_OPERATION_NOT_ALLOWED   = 3 | QOSA_ERRCODE_SIM_BASE, /*!< operation not allowed */
    QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED = 4 | QOSA_ERRCODE_SIM_BASE, /*!< operation not supported */
    QOSA_SIM_ERR_NOT_INSERTED            = 10 | QOSA_ERRCODE_SIM_BASE, /*!< SIM not inserted */
    QOSA_SIM_ERR_PUK_REQUIRED            = 12 | QOSA_ERRCODE_SIM_BASE, /*!< SIM PUK required */
    QOSA_SIM_ERR_FAILURE                 = 13 | QOSA_ERRCODE_SIM_BASE, /*!< SIM failure */
    QOSA_SIM_ERR_BUSY                    = 14 | QOSA_ERRCODE_SIM_BASE, /*!< SIM busy */
    QOSA_SIM_ERR_WRONG                   = 15 | QOSA_ERRCODE_SIM_BASE, /*!< SIM wrong */
    QOSA_SIM_ERR_INCORRECT_PASSWORD      = 16 | QOSA_ERRCODE_SIM_BASE, /*!< incorrect password */
    QOSA_SIM_ERR_PIN2_REQUIRED           = 17 | QOSA_ERRCODE_SIM_BASE, /*!< SIM PIN2 required */
    QOSA_SIM_ERR_PUK2_REQUIRED           = 18 | QOSA_ERRCODE_SIM_BASE, /*!< SIM PUK2 required */
    QOSA_SIM_ERR_MEMORY_FULL             = 20 | QOSA_ERRCODE_SIM_BASE, /*!< memory full */
    QOSA_SIM_ERR_NOT_FOUND               = 22 | QOSA_ERRCODE_SIM_BASE, /*!< not found */
    QOSA_SIM_ERR_MEMORY_FAILURE          = 23 | QOSA_ERRCODE_SIM_BASE, /*!< memory failure */
    QOSA_SIM_ERR_INVALID_PARAM           = 53 | QOSA_ERRCODE_SIM_BASE, /*!< invalid parameter */
    QOSA_SIM_ERR_EXECUTE                 = 1 | (QOSA_ERRCODE_SIM_BASE + QOSA_AT_ERR_OFS),
} qosa_sim_err_e;
// clang-format on

/**
 * @enum qosa_sim_type_e
 * @brief OSA SIM types
 *
 */
typedef enum sim_type
{
    QOSA_SIM_TYPE_INVALID = -1, /*!< Invalid SIM type */
    QOSA_SIM,  /*!< Subscriber Identity Module */
    QOSA_USIM, /*!< Universal Subscriber Identity Module */
} qosa_sim_type_e;

/**
 * @enum qosa_sim_status_e
 * @brief SIM card status definitions
 *
 */
typedef enum sim_status
{
    QOSA_SIM_STATUS_READY,                  /*!< MT is not pending for any password */
    QOSA_SIM_STATUS_SIM_PIN,                /*!< MT waiting SIM PIN to be given */
    QOSA_SIM_STATUS_SIM_PUK,                /*!< MT waiting SIM PUK to be given */
    QOSA_SIM_STATUS_PHONE_TO_SIM_PIN,       /*!< MT is waiting phone-to-SIM card password to be given */
    QOSA_SIM_STATUS_PHONE_TO_FIRST_SIM_PIN, /*!< MT is waiting phone-to-very first SIM card password to be given */
    QOSA_SIM_STATUS_PHONE_TO_FIRST_SIM_PUK, /*!< MT is waiting phone-to-very first SIM card unblocking password to be given */
    QOSA_SIM_STATUS_SIM_PIN2,               /*!< MT is waiting SIM PIN2 to be given */
    QOSA_SIM_STATUS_SIM_PUK2,               /*!< MT is waiting SIM PUK2 to be given */
    QOSA_SIM_STATUS_NETWORK_PIN,            /*!< MT is waiting network personalization password to be given */
    QOSA_SIM_STATUS_NETWORK_PUK,            /*!< MT is waiting network personalization unblocking password to be given */
    QOSA_SIM_STATUS_NETWORK_SUBSET_PIN,     /*!< MT is waiting network subset personalization password to be given */
    QOSA_SIM_STATUS_NETWORK_SUBSET_PUK,     /*!< MT is waiting network subset personalization unblocking password to be given */
    QOSA_SIM_STATUS_SERVICE_PROVIDER_PIN,   /*!< MT is waiting service provider personalization password to be given */
    QOSA_SIM_STATUS_SERVICE_PROVIDER_PUK,   /*!< MT is waiting service provider personalization unblocking password to be given */
    QOSA_SIM_STATUS_CORPORATE_PIN,          /*!< MT is waiting corporate personalization password to be given */
    QOSA_SIM_STATUS_CORPORATE_PUK,          /*!< MT is waiting corporate personalization unblocking password to be given */
    QOSA_SIM_STATUS_NOT_INSERTED,           /*!< SIM card is not inserted */
    QOSA_SIM_STATUS_BUSY,                   /*!< SIM busy */
    QOSA_SIM_STATUS_BLOCKED,                /*!< SIM blocked */
    QOSA_SIM_STATUS_UNKNOWN,                /*!< unknown status */
    QOSA_SIM_STATUS_ESIM_NO_PROFILE,        /*!< eSIM has no profile */
} qosa_sim_status_e;

/**
 * @enum qosa_sim_facility_e
 * @brief Enumerates the facility lock types for SIM cards
 *
 */
typedef enum sim_facility
{
    QOSA_SIM_FACILITY_AB, /*!< all barring service */
    QOSA_SIM_FACILITY_AC, /*!< all incoming barring services */
    QOSA_SIM_FACILITY_FD, /*!< SIM card or active application in the UICC (GSM or USIM) fixed dialling memory feature (if PIN2 authentication has not been done during the current session, PIN2 is required as <passwd>)*/
    QOSA_SIM_FACILITY_AG, /*!< all outing barring services */
    QOSA_SIM_FACILITY_AI, /*!< barr all incoming calls */
    QOSA_SIM_FACILITY_AO, /*!< barr all outgoing calls */
    QOSA_SIM_FACILITY_IR, /*!< barr incoming calls when roaming outside the nome country */
    QOSA_SIM_FACILITY_OI, /*!< barr outgoing international calls */
    QOSA_SIM_FACILITY_OX, /*!< barr outgoing international calls except to home country */
    QOSA_SIM_FACILITY_SC, /*!< lock SIM/UICC CARD INSTALLED in the currently selected card slot */
    QOSA_SIM_FACILITY_P2, /*!< sim pin2 */
} qosa_sim_facility_e;

/**
 * @enum qosa_sim_restricted_access_command_e
 * @brief Enumerates instruction codes for command APDU
 *
 * This enumeration defines various command codes used to perform restricted access operations with the SIM card.
 */
typedef enum sim_restricted_access_command
{
    QOSA_SIM_READ_BINARY = 176,   /*!< Reads a string of bytes from the current transparent EF */
    QOSA_SIM_READ_RECORD = 178,   /*!< Reads one complete record in the current linear fixed or cyclic EF */
    QOSA_SIM_GET_RESPONSE = 192,  /*!< Retrieves response data from the SIM card */
    QOSA_SIM_UPDATE_BINARY = 214, /*!< Updates the current transparent EF with a string of bytes */
    QOSA_SIM_UPDATE_RECORD = 220, /*!< Updates one specific, complete record in the current linear fixed or cyclic EF */
    QOSA_SIM_STATUS = 242,        /*!< Retrieves information about the current directory or current application */
} qosa_sim_restricted_access_command_e;

/**
 * @enum qosa_sim_insert_level_e
 * @brief When inserting a SIM card, the level of the hot swap detection pin.
 *
 */
typedef enum
{
    QOSA_SIM_INSERT_LEVEL_LOW,  /*!< When SIM card is inserted into card slot, hot swap detection pin level is low */
    QOSA_SIM_INSERT_LEVEL_HIGH, /*!< When SIM card is inserted into card slot, hot swap detection pin level is high */
} qosa_sim_insert_level_e;

/**
 * @enum qosa_sim_insert_stat_e
 * @brief SIM card insertion status
 *
 */
typedef enum
{
    QOSA_SIM_INSERT_STAT_REMOVED = 0,  /*!< SIM card is not inserted */
    QOSA_SIM_INSERT_STAT_INSERTED = 1, /*!< SIM card insertion status */
    QOSA_SIM_INSERT_STAT_UNKNOWN = 2,  /*!< SIM card status unknown */
} qosa_sim_insert_stat_e;

/**
 * @enum qosa_sim_ds_type_e
 * @brief Dual SIM type
 *
 */
typedef enum
{
    QOSA_SIM_TYPE_DSSS = 0,    /*!< Dual SIM Single Standby */
    QOSA_SIM_TYPE_DSDS = 1,    /*!< Dual SIM Dual Standby */
    QOSA_SIM_TYPE_DSDS_FP = 2, /*!< Dual SIM Dual Standby with Full Performance */
    QOSA_SIM_TYPE_SSSS = 3,    /*!< Single SIM Single Standby */
    QOSA_SIM_TYPE_MAX          /*!< Dual SIM invalid type */
} qosa_sim_ds_type_e;

/**
 * @enum qosa_sim_ds_type_e 0: 1: 2:active , bitmap invalid; the file protection list uses the default value
 * @brief Dual SIM type
 *
 */
typedef enum
{
    QOSA_SIM_SIMWPROTECT_DISABLE            = 0,    /*!< not active */
    QOSA_SIM_SIMWPROTECT_ENABLE_BITMAP      = 1,    /*!< active, bitmap valid*/
    QOSA_SIM_SIMWPROTECT_ENABLE_DEFAULT     = 2,    /*!< active , bitmap invalid; the file protection list uses the default value */
    QOSA_SIM_SIMWPROTECT_MAX                        /*!< invalid mode */
} qosa_simwprotect_enable_mode_e;


/*-------------------------------------- basic struct --------------------------------------*/
/**
 * @struct qosa_sim_pin_t
 * @brief PIN code
 *
 */
typedef struct sim_pin
{
    qosa_uint8_t length;                             /*!< pin code data length */
    qosa_uint8_t pin_data[QOSA_SIM_PIN_LEN_MAX + 1]; /*!< pin code data, max to QOSA_SIM_PIN_LEN_MAX */
} qosa_sim_pin_t;

/**
 * @struct qosa_sim_iccid_t
 * @brief type for iccid
 *
 */
typedef struct
{
    char id[QOSA_SIM_ICCID_LEN + 1]; /*!< iccid, in string format */
} qosa_sim_iccid_t;

/**
 * @struct qosa_sim_imsi_t
 * @brief type for imsi
 *
 */
typedef struct
{
    char imsi[QOSA_SIM_IMSI_LEN_MAX + 1]; /*!< imsi, in string format */
} qosa_sim_imsi_t;

/**
 * @struct qosa_sim_dfname_t
 * @brief dfname is the Application Identifier
 *
 */
typedef struct
{
    qosa_uint8_t length;                        /*!< the length of aid */
    qosa_uint8_t aid[QOSA_SIM_AID_LEN_MAX + 1]; /*!< aid, hexadecimal data*/
} qosa_sim_dfname_t;

/**
 * @struct qosa_sim_ef_info_t
 * @brief Update count information of the file
 *
 */
typedef struct sim_ef_info
{
    qosa_uint16_t ef_id;       /*!< File id */
    qosa_uint16_t retry_count; /*!< Operation retry count */
    qosa_uint16_t real_count;  /*!< The actual update count value of the file  */
    qosa_uint16_t reserve;     /*!< Retain parameters */
} qosa_sim_ef_info_t;

/*-------------------------------------- event --------------------------------------*/

/**
 * @struct qosa_sim_status_event_t
 * @brief Data content carried by event EVENT_MODEM_SIM_STATUS report
 *
 */
typedef struct sim_status_event
{
    qosa_uint8_t      simid;  /*!< SIM card ID */
    qosa_sim_status_e status; /*!< SIM card status */
    qosa_uint32_t     cause;  /*!< 0:normal; 1: status change caused by cfun0 */
} qosa_sim_status_event_t;

/**
 * @struct qosa_sim_insert_status_event_t
 * @brief Data content carried by event EVENT_MODEM_SIM_INSERT_STATUS report
 * @note Event EVENT_MODEM_SIM_INSERT_STATUS is mainly used to report SIM card insertion status in single card single standby and dual card dual standby scenarios;
 *       If you need to get the insertion status of non-activated SIM card in dual card single standby scenario, please use SLOT_INSERT related interfaces.
 *
 */
typedef struct sim_insert_status_event
{
    qosa_uint8_t           slotid; /*!< slotID */
    qosa_sim_insert_stat_e stat;   /*!< SIM card insertion status */
} qosa_sim_insert_status_event_t;

/*-------------------------------------- pa --------------------------------------*/

/**
 * @struct qosa_sim_cmd_data_t
 * @brief command APDU data field
 *
 */
typedef struct sim_cmd_data
{
    qosa_uint8_t length;                                 /*!< the length of command APDU data field */
    qosa_uint8_t cmd_data[QOSA_SIM_C_APDU_DATA_LEN_MAX]; /*!< command APDU data field */
} qosa_sim_cmd_data_t;

/**
 * @struct qosa_sim_path_id_t
 * @brief sim path id
 *
 */
typedef struct sim_path_id
{
    qosa_uint8_t length;                                 /*!< the length of path id */
    qosa_uint8_t path_id_data[QOSA_SIM_PATH_ID_LEN_MAX]; /*!< path id data */
} qosa_sim_path_id_t;

/**
 * @struct qosa_sim_hot_swap_cfg_t
 * @brief Used to configure the hot swap function of SIM card, including whether to enable hot swap detection, the detection pin level when SIM card is inserted, and the GPIO pin used for hot swap detection (optional).
 *
 */
typedef struct sim_hot_swap_cfg
{
    qosa_bool_t             enable;       /*!< Whether to enable SIM card hot swap detection */
    qosa_sim_insert_level_e insert_level; /*!< Detection pin detection level when SIM card is inserted */
    qosa_uint32_t           gpio; /*!< to be implemented, configured as QOSA_SIM_HOT_SWAP_UNSPECIFIED_GPIO means using product default configuration pins */
} qosa_sim_hot_swap_cfg_t;

/**
 * @struct qosa_sim_phonenumber_data_t
 * @brief SIM card phone number content.
 *
 */
typedef struct sim_phone_number_data
{
    qosa_uint8_t type; /*!< Eight-bit byte address type, e.g.: 129-unknown type (ISDN format), 145-international number (ISDN format), 161-national number*/
    char         phone_number[QOSA_SIM_PHONENUMBER_MAX_LEN]; /*!< Content of the number */
} qosa_sim_phonenumber_data_t;

/**
 * @struct qosa_sim_phonenumber_t
 * @brief Result of getting phone numbers stored in SIM card.
 *
 */
typedef struct sim_phone_number
{
    qosa_uint8_t                num;                                /*!< Number of phone numbers */
    qosa_sim_phonenumber_data_t list[QOSA_SIM_PHONENUMBER_MAX_NUM]; /*!< List of phone numbers */
} qosa_sim_phonenumber_t;

/**
 * @struct qosa_sim_simwprotect_t
 * @brief Used to configure write protection for sim files
 * @note ASR1903 bitmap: bit0: epsnsc, 6fe4; bit1: keys, 6f08; bit2: keyps, 6f09; bit3: kc, 4f20; bit4: kcGprs, 4f52; bit5: loci, 6f7e; bit6: psloci, 6f73; bit7: epsloci,6fe3
 * @note UIS8852 bitmap: bit0: 6F7B;bit1: 6F7E;bit2: 6F73;bit3: 6F08; bit4: 6F09; bit5: 6FE4; bit6: 6FE3; bit7: 6F5B
 * @note ec718 configuration via bitmap is not supported; default optimization is applied to 6FE3 and 6FE4 file writes.
 */
typedef struct qosa_sim_simwprotect
{
    qosa_simwprotect_enable_mode_e  enable;      /*!< 0:not active 1:active, bitmap valid 2:active , bitmap invalid; the file protection list uses the default value. */
    qosa_uint8_t                    bit_map;     /*!< Files that need to be write protected */
} qosa_sim_simwprotect_t;

/*-------------------------------------- cnf --------------------------------------*/

/* All cnf results need to ensure the first two parameters of the structure are simid and err_code in order, which is OSA_MODEM_COMMON_CNF_HDR, user layer does not need to release parameter content, after executing the callback, internally it will be released directly */

/**
 * @struct qosa_sim_general_cnf_t
 * @brief SIM card general result return, used to return simple success or failure, no additional result return type
 *
 */
typedef qosa_modem_general_cnf_t qosa_sim_general_cnf_t;

/**
 * @struct qosa_sim_get_imsi_cnf_t
 * @brief the result of qosa_sim_get_imsi API
 *
 */
typedef struct sim_get_imsi_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR; /*!< common header, simid and err_code */
    qosa_sim_imsi_t imsi;      /*!< imsi data, in string format */
} qosa_sim_get_imsi_cnf_t;

/**
 * @struct qosa_sim_get_iccid_cnf_t
 * @brief the result of qosa_sim_get_iccid API
 *
 */
typedef struct sim_get_iccid_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR; /*!< common header, simid and err_code */
    qosa_sim_iccid_t iccid;    /*!< iccid in string format */
} qosa_sim_get_iccid_cnf_t;

/**
 * @struct qosa_sim_get_status_cnf_t
 * @brief the result of qosa_sim_get_status API
 *
 */
typedef struct sim_get_status_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR; /*!< common header, simid and err_code */
    qosa_sim_status_e status;  /*!< sim status */
} qosa_sim_get_status_cnf_t;

/**
 * @struct qosa_sim_generic_access_cnf_t
 * @brief the result of qosa_sim_generic_access API
 *
 */
typedef struct sim_generic_access_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR;                       /*!< common header, simid and err_code */
    qosa_uint16_t data_len;                          /*!< the length of data */
    qosa_uint8_t  data[QOSA_SIM_R_APDU_LEN_MAX + 1]; /*!< response APDU, include sw */
} qosa_sim_generic_access_cnf_t;

/**
 * @struct qosa_sim_generic_logical_channel_access_cnf_t
 * @brief the result of qosa_sim_generic_logical_channel_access API
 *
 */
typedef struct sim_generic_logical_channel_access_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR;                       /*!< common header, simid and err_code */
    qosa_uint16_t data_len;                          /*!< the length of data */
    qosa_uint8_t  data[QOSA_SIM_R_APDU_LEN_MAX + 1]; /*!< response APDU, include sw */
} qosa_sim_generic_logical_channel_access_cnf_t;

/**
 * @struct qosa_sim_restricted_access_cnf_t
 * @brief the result of qosa_sim_restricted_access API
 *
 */
typedef struct sim_restricted_access_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR;                            /*!< common header, simid and err_code */
    qosa_uint8_t  sw1;                                    /*!< sw1 */
    qosa_uint8_t  sw2;                                    /*!< sw2 */
    qosa_uint16_t data_len;                               /*!< the length of data */
    qosa_uint8_t  data[QOSA_SIM_R_APDU_DATA_LEN_MAX + 1]; /*!< response APDU data field, not include sw */
} qosa_sim_restricted_access_cnf_t;

/**
 * @struct qosa_sim_open_logical_channel_cnf_t
 * @brief the result of qosa_sim_open_logical_channel API
 *
 */
typedef struct sim_open_logical_channel_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR;     /*!< common header, simid and err_code */
    qosa_uint32_t channel_session; /*!< link between the card and the external world during a card session on a given logical channel */
} qosa_sim_open_logical_channel_cnf_t;

/**
 * @struct osa_sim_pin_remain_retries_cnf_t
 * @brief the result of qosa_sim_get_pin_remain_retries API
 *
 */
typedef struct sim_pin_remain_retries_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR; /*!< common header, simid and err_code */
    qosa_uint8_t pin1_remain;  /*!< PIN1 remain retry counts */
    qosa_uint8_t puk1_remain;  /*!< PUK1 remain retry counts */
    qosa_uint8_t pin2_remain;  /*!< PIN2 remain retry counts */
    qosa_uint8_t puk2_remain;  /*!< PUK2 remain retry counts */
} qosa_sim_get_pin_remain_retries_cnf_t;

/**
 * @struct qosa_sim_get_facility_lock_cnf_t
 * @brief the result of qosa_sim_get_facility_lock API
 *
 */
typedef struct sim_get_facility_lock_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR; /*!< common header, simid and err_code */
    qosa_uint8_t status;       /*!< 0:not active 1:active */
} qosa_sim_get_facility_lock_cnf_t;

/**
 * @struct qosa_sim_get_file_update_count_cnf_t
 * @brief the result of qosa_sim_get_file_update_count API
 *
 */
typedef struct sim_get_file_update_count_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR;                                     /*!< common header, simid and err_code */
    qosa_sim_ef_info_t ef_info[QOSA_SIM_WRITE_SIM_FILE_COUNT_MAX]; /*!< response EF info */
} qosa_sim_get_file_update_count_cnf_t;

/**
 * @struct qosa_sim_get_simwprotect_mode_cnf_t
 * @brief the result of qosa_sim_get_simwprotect_mode API
 *
 */
typedef struct sim_get_simwprotect_mode_cnf
{
    QOSA_MODEM_COMMON_CNF_HDR;                  /*!< common header, simid and err_code */
    qosa_sim_simwprotect_t simwprotect_cfg;     /*!< SIM Write protect configure */
} qosa_sim_get_simwprotect_mode_cnf_t;

/*-------------------------------------- api --------------------------------------*/

/**
 * @brief Read SIM card type information
 *
 * This function is used to synchronously read the card type information of the specified SIM card, including SIM card and USIM card types.
 * Through this function, you can get the specific type of the current SIM card, used to distinguish 2G SIM card and 3G/4G/5G USIM card.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to read
 *
 * @return qosa_sim_type_e
 *           - Return SIM card type, possible values include:
 *           - QOSA_SIM: Normal SIM card (2G)
 *           - QOSA_USIM: USIM card (3G/4G/5G)
 *
 * @note This function can only be used after SIM card initialization is completed, otherwise it may return wrong results
 * @note This function is a synchronous operation and will return results immediately
 */
qosa_sim_type_e qosa_sim_read_sim_type(qosa_uint8_t simid);

/**
 * @brief Asynchronously get SIM card IMSI information
 *
 * This function is used to asynchronously read the IMSI (International Mobile Subscriber Identity) information of the SIM card.
 * IMSI is the unique identifier of the SIM card, used to identify user identity in the mobile network.
 * The operation is executed asynchronously, and the result is returned through the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to read
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_get_imsi_cnf_t
 *           - Return IMSI string on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_FAILURE: Read failed
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note IMSI is usually a 15-digit number string, format: MCC (3 digits) + MNC (2-3 digits) + MSIN (9-10 digits)
 * @note Before calling this function, it is recommended to check if the SIM card status is ready (QOSA_SIM_STATUS_READY)
 */
qosa_sim_err_e qosa_sim_get_imsi(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);

/**
 * @brief Synchronously read SIM card IMSI information
 *
 * This function is used to synchronously read the IMSI (International Mobile Subscriber Identity) information of the SIM card.
 * IMSI is the unique identifier of the SIM card, used to identify user identity in the mobile network.
 * This function will directly return the read result, suitable for scenarios that require immediate acquisition of IMSI.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to read
 *
 * @param [out] imsi
 *           - Pointer to IMSI structure, used to receive the read IMSI information
 *           - IMSI is stored in string format in the structure
 *           - Caller needs to ensure a valid pointer is passed in
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_FAILURE: Read failed
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is a synchronous operation and will block the current thread until the operation is completed
 * @note IMSI is usually a 15-digit number string, format: MCC (3 digits) + MNC (2-3 digits) + MSIN (9-10 digits)
 * @note Before calling this function, it is recommended to check if the SIM card status is ready (QOSA_SIM_STATUS_READY)
 */
qosa_sim_err_e qosa_sim_read_imsi(qosa_uint8_t simid, qosa_sim_imsi_t *imsi);

/**
 * @brief Asynchronously get SIM card status information
 *
 * This function is used to asynchronously query the current status information of the SIM card, including PIN code status, lock status, etc.
 * Depending on platform characteristics, this operation may need to interact with the CP side, and the result is returned through the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to query
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_get_status_cnf_t
 *           - Return SIM card status information on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_FAILURE: Query failed
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note The returned status information includes the current status of the SIM card (such as READY, SIM PIN, SIM PUK, etc.)
 * @note In dual card scenarios, you need to specify the correct simid to query the status of the corresponding SIM card
 */
qosa_sim_err_e qosa_sim_get_status(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);

/**
 * @brief Read buffered SIM card status information
 *
 * This function is used to synchronously read the cached SIM card status information, without needing to interact with the underlying layer.
 * It returns the most recently updated SIM card status, suitable for scenarios that require quick access to status information.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to query
 *
 * @param [out] status
 *           - Pointer to status enumeration variable, used to receive the read SIM card status
 *           - Status values refer to qosa_sim_status_e enumeration definition
 *           - Caller needs to ensure a valid pointer is passed in
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_FAILURE: Read failed
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is a synchronous operation and will return results immediately
 * @note The returned status is the cached SIM card status, which may not be the real-time status
 * @note If you need to get real-time status, please use the qosa_sim_get_status asynchronous interface
 * @note Common statuses include: READY, SIM PIN, SIM PUK, NOT_INSERTED, etc.
 */
qosa_sim_err_e qosa_sim_read_status(qosa_uint8_t simid, qosa_sim_status_e *status);

/**
 * @brief Get SIM card function block initialization status
 *
 * This function is used to query the initialization status of the specified SIM card function module, and the return value is a combination of multiple status flag bits.
 * You can check whether each function module has completed initialization through bitwise operations.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to query
 *
 * @return int
 *           - Return value is a combination of the following status flag bits (bitwise OR):
 *           - QOSA_SIMINI_STAT_START: SIM card initialization start status
 *           - QOSA_SIMINI_STAT_CPIN_READY: SIM card PIN code ready status
 *           - QOSA_SIMINI_STAT_SMS_DONE: SMS function initialization completion status
 *           - QOSA_SIMINI_STAT_PB_DONE: Phone book function initialization completion status
 *           - You can use bitwise AND operation (&) to check if a specific status is set
 *
 * @note This function is a synchronous operation and will return results immediately
 * @note The return value is a combination of multiple status flag bits, which need to be parsed using bitwise operations
 * @note Different status flag bits may be set at different time points, reflecting different stages of SIM card initialization
 */
int qosa_sim_read_init_stat(qosa_uint8_t simid);

/**
 * @brief Get SIM card insertion status
 *
 * This function is used to query the insertion status of the specified SIM card, mainly used in single card single standby and dual card dual standby scenarios.
 * In dual card single standby scenarios, if you need to get the insertion status of non-activated SIM card, please use SLOT_INSERT related interfaces.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to query
 *
 * @return qosa_sim_insert_stat_e
 *           - Return SIM card insertion status, possible values include:
 *           - QOSA_SIM_INSERT_STAT_REMOVED: SIM card has been removed
 *           - QOSA_SIM_INSERT_STAT_INSERTED: SIM card has been inserted
 *           - QOSA_SIM_INSERT_STAT_UNKNOWN: SIM card status is unknown
 *
 * @note This function is a synchronous operation and will return results immediately
 * @note In dual card single standby scenarios, this interface can only get the insertion status of the currently activated SIM card
 * @note For non-activated SIM card status queries, please use the qosa_sim_read_slot_stat interface
 */
qosa_sim_insert_stat_e qosa_sim_read_insert_stat(qosa_uint8_t simid);

/**
 * @brief Get the SIM card insertion status of the physical card slot
 *
 * This function is used to query the SIM card presence status of the specified physical card slot, independent of the currently activated SIM card.
 * It can be used to get the SIM card insertion status of any physical card slot, including non-activated card slots.
 *
 * @param [in] slotid
 *           - Physical card slot ID, identifying the physical card slot to query
 *           - 0: Card slot 1 (defined according to hardware design document)
 *           - 1: Card slot 2 (defined according to hardware design document)
 *
 * @return qosa_sim_insert_stat_e
 *           - Return the SIM card insertion status of the physical card slot, possible values include:
 *           - QOSA_SIM_INSERT_STAT_REMOVED: SIM card has been removed
 *           - QOSA_SIM_INSERT_STAT_INSERTED: SIM card has been inserted
 *           - QOSA_SIM_INSERT_STAT_UNKNOWN: SIM card status is unknown
 *
 * @note This function is a synchronous operation and will return results immediately
 * @note This interface queries the status of the physical card slot, independent of the currently activated SIM card
 * @note In dual card single standby scenarios, you can use this interface to get the status of non-activated card slots
 * @note Card slot numbering is related to hardware design, please refer to specific hardware documentation
 */
qosa_sim_insert_stat_e qosa_sim_read_slot_stat(qosa_uint8_t slotid);

/**
 * @brief Set SIM card hot swap detection function
 *
 * This function is used to configure the SIM card hot swap detection function of the specified physical card slot, including enable status, insertion detection level, and GPIO pin configuration.
 * The hot swap detection function allows the device to automatically detect status changes and trigger corresponding events when the SIM card is inserted or removed.
 *
 * @param [in] slotid
 *           - Physical card slot ID, identifying the physical card slot to configure
 *           - 0: Card slot 1 (defined according to hardware design document)
 *           - 1: Card slot 2 (defined according to hardware design document)
 *
 * @param [in] hot_swap_config
 *           - Pointer to hot swap configuration structure, including the following configuration parameters:
 *           - enable: Whether to enable SIM card hot swap detection function
 *           - insert_level: Detection pin level status when SIM card is inserted (high level or low level)
 *           - gpio: GPIO pin number used for hot swap detection (if configuration is supported, otherwise use default configuration)
 *           - Caller needs to ensure a valid pointer is passed in
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note 1. Depending on product platform characteristics, this interface may take effect immediately or after restart
 * @note 2. Note that in dual card single standby/dual card dual standby, if card slots are exchanged through osa_sim_set_slot_id, pay attention to the change in the correspondence between application layer simid and actual physical card slot
 * @note 3. Note that on most platforms, the enable and detection level configuration of hot swap does not distinguish slotid, the last one takes precedence, that is, if you first configure slotid 0 through this api to enable hot swap function, detection level is high, and then again configure slotid 1 through this api to enable hot swap function, detection level is low, then the actual hot swap detection level is low
 * @note This function is a synchronous operation and will return results immediately
 */
qosa_sim_err_e qosa_sim_set_sim_hot_swap(qosa_uint8_t slotid, qosa_sim_hot_swap_cfg_t *hot_swap_config);

/**
 * @brief Get SIM card hot swap detection function configuration
 *
 * This function is used to read the SIM card hot swap detection function configuration information of the specified physical card slot.
 * You can get the current enable status, insertion detection level, and GPIO pin configuration of the hot swap function.
 *
 * @param [in] slotid
 *           - Physical card slot ID, identifying the physical card slot to query
 *           - 0: Card slot 1 (defined according to hardware design document)
 *           - 1: Card slot 2 (defined according to hardware design document)
 *
 * @param [out] hot_swap_config
 *           - Pointer to hot swap configuration structure, used to receive configuration information
 *           - Includes the following configuration parameters:
 *           - enable: Whether to enable hot swap detection function
 *           - insert_level: Detection pin level status when SIM card is inserted
 *           - gpio: GPIO pin number used for hot swap detection (if configuration is supported)
 *           - Caller needs to ensure a valid pointer is passed in
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is a synchronous operation and will return results immediately
 * @note Hot swap detection configuration may vary depending on the product platform, some platforms may not support GPIO pin configuration
 * @note In multi-card scenarios, pay attention to the correspondence between application layer simid and physical card slot
 */
qosa_sim_err_e qosa_sim_get_sim_hot_swap(qosa_uint8_t slotid, qosa_sim_hot_swap_cfg_t *hot_swap_config);

/**
 * @brief Get the physical card slot ID corresponding to the application layer SIM ID
 *
 * This function is used to query the physical card slot ID currently used by the specified application layer SIM ID.
 * Mainly used in dual card single standby scenarios to help the application layer understand the mapping relationship between SIM cards and physical card slots.
 *
 * @param [in] simid
 *           - Application layer SIM ID, identifying the SIM card to query
 *           - In dual card single standby mode, this parameter is usually 0 (only supports single card)
 *
 * @param [out] slotid
 *           - Pointer to physical card slot ID, used to receive query results
 *           - Return value is the physical card slot number currently used by the SIM card:
 *           - 0: Card slot 1 (defined according to hardware design document)
 *           - 1: Card slot 2 (defined according to hardware design document)
 *           - Caller needs to ensure a valid pointer is passed in
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is a synchronous operation and will return results immediately
 * @note In dual card single standby mode, the application layer can usually only see simid 0
 * @note Physical card slot numbering is related to hardware design, please refer to specific hardware documentation
 */
qosa_sim_err_e qosa_sim_get_slot_id(qosa_uint8_t simid, qosa_uint8_t *slotid);

/**
 * @brief Set the physical card slot used by application layer SIM ID
 *
 * This function is used to configure the physical card slot used by the specified application layer SIM ID, supporting card slot switching in dual card single standby scenarios.
 * In dual card dual standby scenarios, this operation will trigger the underlying exchange of physical card slots used by simid 0/1.
 *
 * @param [in] simid
 *           - Application layer SIM ID, identifying the SIM card to configure
 *           - In dual card single standby mode, this parameter is usually 0 (only supports single card)
 *
 * @param [in] slotid
 *           - Target physical card slot ID, specifying the physical card slot to use
 *           - 0: Card slot 1 (defined according to hardware design document)
 *           - 1: Card slot 2 (defined according to hardware design document)
 *           - Value range: 0 to CONFIG_QOSA_SIM_HAL_SLOT_NUM-1
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note Important: Before performing card slot switching, you need to call cfun0 to enter flight mode, and after switching is completed, call cfun4 or cfun1 to restore
 * @note Switching process: cfun0 → switch slot id → cfun4/cfun1
 * @note This operation may affect the current network connection status, please use it with caution
 * @note In dual card dual standby scenarios, this operation will exchange the physical card slot mapping relationship of the two SIM cards
 */
qosa_sim_err_e qosa_sim_set_slot_id(qosa_uint8_t simid, qosa_uint8_t slotid);

/**
 * @brief Get application layer dual SIM type
 *
 * This function is only applicable to devices that support dual SIM dual standby.
 *
 * @param [in] simid
 *           - Application layer SIM ID, identifying the SIM card to configure
 *
 * @param [out] type
 *           - Dual SIM mode
 *           - QOSA_SIM_TYPE_DSSS: Dual SIM Single Standby
 *           - QOSA_SIM_TYPE_DSDS: Dual SIM Dual Standby
 *           - QOSA_SIM_TYPE_DSDS_FP: Dual SIM Dual Standby with Full Performance
 *           - QOSA_SIM_TYPE_SSSS: Single SIM Single Standby
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 */
qosa_sim_err_e qosa_sim_get_ds_type(qosa_uint8_t simid, qosa_sim_ds_type_e *type);

/**
 * @brief Set application layer dual SIM type
 *
 * This function is only applicable to devices that support dual SIM dual standby.
 *
 * @param [in] simid
 *           - Application layer SIM ID, identifying the SIM card to configure
 *
 * @param [in] type
 *           - Dual SIM mode
 *           - QOSA_SIM_TYPE_DSSS: Dual SIM Single Standby
 *           - QOSA_SIM_TYPE_DSDS: Dual SIM Dual Standby
 *           - QOSA_SIM_TYPE_DSDS_FP: Dual SIM Dual Standby with Full Performance
 *           - QOSA_SIM_TYPE_SSSS: Single SIM Single Standby
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 */
qosa_sim_err_e qosa_sim_set_ds_type(qosa_uint8_t simid, qosa_sim_ds_type_e type);

/**
 * @brief Verify SIM card PIN code
 *
 * This function is used to asynchronously verify the PIN code of the SIM card, confirming whether the user-entered PIN code is correct.
 * The operation is executed asynchronously, and the result is returned through the callback function, indicating whether the verification was successful.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to verify
 *
 * @param [in] pin
 *           - Pointer to PIN code structure, containing the PIN code information to verify
 *           - PIN code must be 4 to 8 digit number string
 *           - For PIN1 verification, usually use 4 digits
 *           - Caller needs to ensure a valid pointer is passed in
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_general_cnf_t
 *           - Return operation success status on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter (such as PIN code length error)
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note If PIN code verification fails, the SIM card may be locked and need to be unlocked with PUK code
 * @note It is recommended to query the remaining PIN code retry attempts before verification
 */
qosa_sim_err_e qosa_sim_verify_pin(qosa_uint8_t simid, qosa_sim_pin_t *pin, sim_callback_ptr cb, void *ctx);

/**
 * @brief Reset SIM card PIN code using PUK code
 *
 * This function is used to asynchronously unlock the locked SIM card using PUK code and set a new PIN code.
 * When the number of incorrect PIN code entries exceeds the limit causing the SIM card to be locked, you need to use the PUK code to unlock it.
 * The operation is executed asynchronously, and the result is returned through the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to operate on
 *
 * @param [in] unblock_pin
 *           - Pointer to PUK code structure, containing the PUK code used for unlocking
 *           - PUK code must be 8 digit number string
 *           - Caller needs to ensure a valid pointer is passed in
 *
 * @param [in] new_pin
 *           - Pointer to new PIN code structure, containing the new PIN code to set
 *           - New PIN code must be 4 to 8 digit number string
 *           - Caller needs to ensure a valid pointer is passed in
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_general_cnf_t
 *           - Return operation success status on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter (such as PUK code or new PIN code length error)
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_PUK_REQUIRED: PUK code is required
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note PUK code is usually printed on the SIM card holder, if the number of incorrect PUK code entries exceeds the limit, the SIM card will be permanently locked
 * @note It is recommended to query the remaining PUK code retry attempts before operation
 */
qosa_sim_err_e qosa_sim_unblock_pin(qosa_uint8_t simid, qosa_sim_pin_t *unblock_pin, qosa_sim_pin_t *new_pin, sim_callback_ptr cb, void *ctx);

/**
 * @brief Change SIM card PIN code
 *
 * This function is used to asynchronously change the PIN code for a specific
 * facility on the SIM card, such as PIN1, PIN2, etc.
 * The old PIN code is required for verification, then a new PIN code is set.
 * The operation is executed asynchronously, and the result is returned through
 * the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to operate on
 *
 * @param [in] fac
 *           - Facility type, refer to qosa_sim_facility_e enumeration
 *           - Common facilities include: SC (SIM card PIN1), P2 (PIN2), etc.
 *           - Supported facility types depend on SIM card and platform capabilities
 *
 * @param [in] old_pin
 *           - Pointer to the old PIN code structure containing the current valid PIN
 *           - Used to verify that the user has permission to change the PIN
 *           - Caller must ensure a valid pointer is passed
 *
 * @param [in] new_pin
 *           - Pointer to the new PIN code structure containing the new PIN to set
 *           - The new PIN must be a 4 to 8 digit numeric string
 *           - Caller must ensure a valid pointer is passed
 *
 * @param [in] cb
 *           - Callback function pointer for receiving operation results
 *           - Callback parameter type: qosa_sim_general_cnf_t
 *           - Returns operation success status on success, error code on failure
 *
 * @param [in] ctx
 *           - User context pointer that will be passed back unchanged in the callback
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter (e.g., incorrect PIN length)
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card not inserted
 *           - QOSA_SIM_ERR_INCORRECT_PASSWORD: Incorrect password
 *           - For other error codes, please refer to qosa_sim_err_e enumeration
 *
 * @note This function is an asynchronous operation, actual execution result is
 *       returned through the callback function
 * @note Changing facilities like PIN2 may require PIN1 verification first
 * @note Regular PIN code changes are recommended for better security
 */
qosa_sim_err_e
qosa_sim_change_pin(qosa_uint8_t simid, qosa_sim_facility_e fac, qosa_sim_pin_t *old_pin, qosa_sim_pin_t *new_pin, sim_callback_ptr cb, void *ctx);

/**
 * @brief Remaining attempts to retrieve SIM card PIN code
 *
 * This function is used to asynchronously retrieve the remaining number of uses for the SIM card PIN code.
 * This operation is executed asynchronously, and the result is returned via a callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to operate on
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_get_pin_remain_retries_cnf_t
 *           - Return operation success status on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter (such as PIN code length error)
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note The returned remaining times information includes:
 *           - PIN1 remaining retry attempts
 *           - PUK1 remaining retry attempts
 *           - PIN2 remaining retry attempts
 *           - PUK2 remaining retry attempts
 * @note When the remaining times is 0, the corresponding PIN code will be locked and needs to be unlocked with PUK code
 */
qosa_sim_err_e qosa_sim_get_pin_remain_retries(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);

/**
 * @brief Get SIM card function lock status
 *
 * This function is used to asynchronously query the current lock status of a specific function on the SIM card, such as SIM card lock, fixed dialing lock, etc.
 * The operation is executed asynchronously, and the result is returned through the callback function, containing the lock status information of the specified function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to query
 *
 * @param [in] fac
 *           - Function type, refer to qosa_sim_facility_e enumeration definition
 *           - Common functions include: SC (SIM card lock), FD (fixed dialing lock), P2 (PIN2 lock), etc.
 *           - Specific supported function types depend on SIM card and platform capabilities
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_get_facility_lock_cnf_t
 *           - Return function lock status (0-not activated, 1-activated) on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter (such as unsupported function type)
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note Lock status return values:
 *           - 0: Function not locked (not activated)
 *           - 1: Function locked (activated)
 * @note Before setting function lock status, it is recommended to query the current status first
 */
qosa_sim_err_e qosa_sim_get_facility_lock(qosa_uint8_t simid, qosa_uint8_t fac, sim_callback_ptr cb, void *ctx);

/**
 * @brief Set SIM card function lock status
 *
 * This function is used to set or unlock the lock status of a specific function on the SIM card, such as PIN code lock, fixed dialing lock, etc.
 * Supports multiple function types (facility) and operation modes, the operation is executed asynchronously, and the result is returned through the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to operate on
 *
 * @param [in] fac
 *           - Function type, refer to qosa_sim_facility_e enumeration definition
 *           - Common functions include: SC (SIM card lock), FD (fixed dialing lock), P2 (PIN2 lock), etc.
 *
 * @param [in] pin
 *           - Pointer to PIN code structure, containing PIN code information used for verification
 *           - For some functions (such as PIN2 related operations), corresponding PIN code needs to be provided
 *           - PIN code length must be within valid range (4 to 8 digits)
 *
 * @param [in] mode
 *           - Operation mode, specifying the type of operation to execute
 *           - 0: Unlock function lock (correct PIN code needs to be provided)
 *           - 1: Set function lock (PIN code needs to be provided for verification)
 *
 * @param [in] classx
 *           - Data category, specifying the communication type applicable to the function lock
 *           - 1: Voice communication
 *           - 2: Data communication
 *           - 4: Fax communication
 *           - 7: All telephone communications except short messages
 *           - 8: Short message communication
 *           - 16: Data line synchronous communication
 *           - 32: Data line asynchronous communication
 *           - Multiple categories can be combined using bitwise OR operation
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_general_cnf_t
 *           - Return operation result on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter (such as PIN code length error)
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note Different function types may have different requirements for PIN code, please refer to specific function specifications
 * @note Before operation, it is recommended to query the current lock status through qosa_sim_get_facility_lock first
 */
qosa_sim_err_e qosa_sim_set_facility_lock(
    qosa_uint8_t        simid,
    qosa_sim_facility_e fac,
    qosa_sim_pin_t     *pin,
    qosa_uint8_t        mode,
    qosa_uint8_t        classx,
    sim_callback_ptr    cb,
    void               *ctx
);

/**
 * @brief SIM card general access interface
 *
 * This function is used to send raw APDU commands to the SIM card and receive responses, providing the lowest level SIM card access capability.
 * Supports sending any valid APDU commands, the operation is executed asynchronously, and the result is returned through the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to operate on
 *
 * @param [in] cmd_data
 *           - Pointer to APDU command data, containing hexadecimal data to be sent
 *           - Data format example: 0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00 (select master file)
 *           - Must ensure data length matches data_len parameter
 *
 * @param [in] data_len
 *           - Actual length of APDU command data (in bytes)
 *           - Must be within valid range: QOSA_SIM_C_APDU_LEN_MIN to QOSA_SIM_C_APDU_LEN_MAX
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_generic_access_cnf_t
 *           - Return complete response APDU (including status words) on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter (such as data length out of range)
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note Response data contains complete R-APDU, including status words (SW1/SW2)
 * @note Using this interface requires familiarity with SIM card APDU command format and specifications
 * @note For standard operations, it is recommended to prioritize the restricted access interface (qosa_sim_restricted_access)
 */
qosa_sim_err_e qosa_sim_generic_access(qosa_uint8_t simid, qosa_uint8_t *cmd_data, qosa_uint16_t data_len, sim_callback_ptr cb, void *ctx);

/**
 * @brief Access SIM card application through logical channel
 *
 * This function is used to send APDU commands and receive responses through an opened SIM card logical channel.
 * Logical channels allow communication with specific applications on the SIM card, each channel works independently.
 * The operation is executed asynchronously, and the result is returned through the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to operate on
 *
 * @param [in] session_id
 *           - Logical channel session ID, obtained through qosa_sim_open_logical_channel function
 *           - Value range: 1 to CONFIG_QOSA_SIM_LOGICAL_CHANNEL_NUM
 *
 * @param [in] cmd_data
 *           - Pointer to APDU command data, containing hexadecimal data to be sent
 *           - Data format example: 0x00, 0x88, 0x00, 0x81, 0x22, 0x10
 *           - Must ensure data length matches data_len parameter
 *
 * @param [in] data_len
 *           - Actual length of APDU command data (in bytes)
 *           - Must be within valid range: QOSA_SIM_C_APDU_LEN_MIN to QOSA_SIM_C_APDU_LEN_MAX
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_generic_logical_channel_access_cnf_t
 *           - Return response APDU data on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter (such as data length out of range)
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note Before use, you must successfully open a logical channel through qosa_sim_open_logical_channel
 * @note Response data contains complete R-APDU, including status words (SW1/SW2)
 */
qosa_sim_err_e qosa_sim_generic_logical_channel_access(
    qosa_uint8_t     simid,
    qosa_uint32_t    session_id,
    qosa_uint8_t    *cmd_data,
    qosa_uint16_t    data_len,
    sim_callback_ptr cb,
    void            *ctx
);

/**
 * @brief SIM card restricted access interface
 *
 * This function is used to execute restricted access operations on the SIM card, supporting multiple standard SIM card commands.
 * Restricted access provides a standardized way to access the SIM card file system, including reading binary data,
 * reading records, getting responses, updating data, etc.
 * The operation is executed asynchronously, and the result is returned through the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to operate on
 *
 * @param [in] cmd
 *           - Command code, refer to qosa_sim_restricted_access_command_e enumeration definition
 *           - Common commands include: READ_BINARY, READ_RECORD, GET_RESPONSE, UPDATE_BINARY, etc.
 *
 * @param [in] fileid
 *           - File identifier, specifying the SIM card file to access
 *           - For STATUS command, this parameter can be ignored
 *
 * @param [in] p1
 *           - Command parameter 1, specific meaning depends on the type of command being executed
 *           - For commands other than GET_RESPONSE and STATUS, this parameter is required
 *
 * @param [in] p2
 *           - Command parameter 2, specific meaning depends on the type of command being executed
 *           - For commands other than GET_RESPONSE and STATUS, this parameter is required
 *
 * @param [in] p3
 *           - Command parameter 3, usually represents data length or offset
 *           - For commands other than GET_RESPONSE and STATUS, this parameter is required
 *
 * @param [in] data
 *           - Pointer to command data structure, containing data content to be sent
 *           - For write operations (such as UPDATE_BINARY), this parameter cannot be NULL
 *           - For read operations, this parameter is usually NULL
 *
 * @param [in] path
 *           - Pointer to path identifier structure, specifying file access path
 *           - If no specific path is needed, NULL can be passed in
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_restricted_access_cnf_t
 *           - Return operation result and response data on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note Response data does not include status words (SW1/SW2), status words are returned through separate fields
 * @note For specific command parameter meanings, please refer to GSM 11.11 specification document
 */
qosa_sim_err_e qosa_sim_restricted_access(
    qosa_uint8_t         simid,
    qosa_uint8_t         cmd,
    qosa_uint16_t        fileid,
    qosa_uint8_t         p1,
    qosa_uint8_t         p2,
    qosa_uint8_t         p3,
    qosa_sim_cmd_data_t *data,
    qosa_sim_path_id_t  *path,
    sim_callback_ptr     cb,
    void                *ctx
);

/**
 * @brief Asynchronously get SIM card ICCID information
 *
 * This function is used to asynchronously read the Integrated Circuit Card Identifier (ICCID) of the SIM card.
 * ICCID is the unique identifier of the SIM card, usually printed on the SIM card surface, used to identify the SIM card.
 * The operation is executed asynchronously, and the result is returned through the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to read
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_get_iccid_cnf_t
 *           - Return ICCID string on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_FAILURE: Read failed
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note ICCID is usually a 19-20 digit number string, format: MMCC IINN NNNN NNNN NN C
 */
qosa_sim_err_e qosa_sim_get_iccid(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);

/**
 * @brief Synchronously read SIM card ICCID information
 *
 * This function is used to synchronously read the Integrated Circuit Card Identifier (ICCID) of the SIM card.
 * ICCID is the unique identifier of the SIM card, usually printed on the SIM card surface, used to identify the SIM card.
 * This function will directly return the read result, suitable for scenarios that require immediate acquisition of ICCID.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to read
 *
 * @param [out] iccid
 *           - Pointer to ICCID structure, used to receive the read ICCID information
 *           - ICCID is stored in string format in the structure
 *           - Caller needs to ensure a valid pointer is passed in
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_FAILURE: Read failed
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note ICCID is usually a 19-20 digit number string, format: MMCC IINN NNNN NNNN NN C
 * @note This function is a synchronous operation and will block the current thread until the operation is completed
 */
qosa_sim_err_e qosa_sim_read_iccid(qosa_uint8_t simid, qosa_sim_iccid_t *iccid);

/**
 * @brief Open SIM card logical channel
 *
 * This function is used to open a new logical channel on the SIM card, so as to communicate with specific applications through this channel.
 * Logical channels allow simultaneous communication with multiple applications on the SIM card, each channel works independently.
 * The open operation is executed asynchronously, and the result is returned through the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to operate on
 *
 * @param [in] dfname
 *           - Pointer to DF name structure, containing Application Identifier (AID) to access
 *           - AID is used to identify specific applications on the SIM card
 *           - AID length must be within valid range (1 to QOSA_SIM_AID_LEN_MAX bytes)
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_open_logical_channel_cnf_t
 *           - Return logical channel session ID on success, return error code on failure
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter (such as AID length out of range)
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_OPERATION_NOT_SUPPORTED: Operation not supported
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note The opened logical channel must be closed through qosa_sim_close_logical_channel after use
 * @note The number of logical channels is limited by platform (CONFIG_QOSA_SIM_LOGICAL_CHANNEL_NUM)
 */
qosa_sim_err_e qosa_sim_open_logical_channel(qosa_uint8_t simid, qosa_sim_dfname_t *dfname, sim_callback_ptr cb, void *ctx);

/**
 * @brief Close SIM card logical channel
 *
 * This function is used to close the SIM card logical channel previously opened through qosa_sim_open_logical_channel.
 * The close operation is executed asynchronously, and the result is returned through the callback function.
 *
 * @param [in] simid
 *           - SIM card ID, identifying the SIM card to operate on
 *
 * @param [in] sessionid
 *           - Logical channel session ID, obtained through qosa_sim_open_logical_channel function
 *           - Value range: 1 to CONFIG_QOSA_SIM_LOGICAL_CHANNEL_NUM
 *
 * @param [in] cb
 *           - Callback function pointer, used to receive operation results
 *           - Callback parameter type: qosa_sim_general_cnf_t
 *
 * @param [in] ctx
 *           - User context pointer, will be passed as-is when calling back
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation has been successfully initiated
 *           - QOSA_SIM_ERR_INVALID_PARAM: Invalid parameter
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 *
 * @note This function is an asynchronous operation, actual execution results are returned through the callback function
 * @note Closing a non-existent logical channel will return an error
 */
qosa_sim_err_e qosa_sim_close_logical_channel(qosa_uint8_t simid, qosa_uint32_t sessionid, sim_callback_ptr cb, void *ctx);

/**
 * @brief Get phone number information stored in SIM card
 *
 * This function is used to read the local phone number information stored in the SIM card, including number content and type information.
 * Supports reading multiple phone numbers (up to 4), each number contains number content and address type information.
 *
 * @param [in] simid
 *           - Application layer SIM ID, identifying the SIM card to read
 *           - Always 0 in dual card single standby mode
 *
 * @param [out] phonenumber
 *           - Pointer to phone number structure, used to receive the read phone number information
 *           - Contains number count, number list (number content and type information)
 *           - Type information includes: 129-unknown type (ISDN format), 145-international number (ISDN format), 161-national number, etc.
 *
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card is not inserted
 *           - QOSA_SIM_ERR_FAILURE: Read failed
 *           - QOSA_SIM_ERR_BUSY: SIM card is busy
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 */
qosa_sim_err_e qosa_sim_get_phonenumber(qosa_uint8_t simid, qosa_sim_phonenumber_t *phonenumber);

/**
 * @brief Get update count information of specific files on SIM card, result is returned through qosa_sim_get_file_update_count_cnf_t
 *
 * @param[in] simid
 *          - Application layer SIM ID, identifying the SIM card to read
 *          - Always 0 in dual card single standby mode
 * @param[in] ef_id
 *          - List of specific file IDs
 *          - For example {0x6F5B, 0x6F7B, 0x6F7E, 0x6F73, 0x6F08, 0x6F09, 0x6FE4, 0x6FE3}
 * @param[in] ef_id_cnt
 *          - Number of specific files
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - User context
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card not inserted
 *           - QOSA_SIM_ERR_FAILURE: Read failed
 *           - QOSA_SIM_ERR_BUSY: SIM card busy
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 */
qosa_sim_err_e qosa_sim_get_file_update_count(qosa_uint8_t simid, qosa_uint16_t *ef_id, qosa_uint8_t ef_id_cnt, sim_callback_ptr cb, void *ctx);

/**
 * @brief Get SIM Write protect configure , result is returned through qosa_sim_get_simwprotect_mode_cnf_t
 *
 * @param[in] simid
 *          - Application layer SIM ID, identifying the SIM card to read
 *          - Always 0 in dual card single standby mode
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - User context
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card not inserted
 *           - QOSA_SIM_ERR_FAILURE: Read failed
 *           - QOSA_SIM_ERR_BUSY: SIM card busy
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 */
qosa_sim_err_e qosa_sim_get_simwprotect_mode(qosa_uint8_t simid, sim_callback_ptr cb, void *ctx);

/**
 * @brief Set SIM Write protect configure , result is returned through qosa_sim_general_cnf_t
 *
 * @param[in] simid
 *          - Application layer SIM ID, identifying the SIM card to read
 *          - Always 0 in dual card single standby mode
  * @param[in] simwprotect_cfg
 *          - SIM Write protect config
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - User context
 * @return qosa_sim_err_e
 *           - QOSA_SIM_ERR_OK: Operation successful
 *           - QOSA_SIM_ERR_NOT_INSERTED: SIM card not inserted
 *           - QOSA_SIM_ERR_FAILURE: Read failed
 *           - QOSA_SIM_ERR_BUSY: SIM card busy
 *           - For other error codes, please refer to qosa_sim_err_e enumeration definition
 */
qosa_sim_err_e qosa_sim_set_simwprotect_mode(qosa_uint8_t simid, qosa_sim_simwprotect_t simwprotect_cfg, sim_callback_ptr cb, void *ctx);

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_SIM_H__ */
