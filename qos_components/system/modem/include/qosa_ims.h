/*****************************************************************/ /**
* @file qosa_ims.h
* @brief IMS operation interface
* @author elmer.tang@quectel.com
* @date 2026-03-06
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2026-03-06 <td>1.0 <td>elmer.tang <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_IMS_H__
#define __QOSA_IMS_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_modem.h"

#define QOSA_ERRCODE_IMS_BASE            (QOSA_COMPONENT_IMS << 16)


// clang-format off
/**
 * @enum qosa_ims_err_e
 * @brief network error codes
 *
 */
typedef enum ims_err
{
    QOSA_IMS_ERR_OK = 0,

    QOSA_IMS_ERR_OPERATION_NOT_ALLOWED   = 3 | QOSA_ERRCODE_IMS_BASE, /*!< operation not allowed */
    QOSA_IMS_ERR_OPERATION_NOT_SUPPORTED = 4 | QOSA_ERRCODE_IMS_BASE, /*!< operation not supported */
    QOSA_IMS_ERR_MEMORY_FULL             = 20 | QOSA_ERRCODE_IMS_BASE, /*!< memory full */
    QOSA_IMS_ERR_MEMORY_FAILURE          = 23 | QOSA_ERRCODE_IMS_BASE, /*!< memory failure */
    QOSA_IMS_ERR_INVALID_PARAM           = 53 | QOSA_ERRCODE_IMS_BASE, /*!< invalid parameter */
    QOSA_IMS_ERR_EXECUTE                 = 1 | (QOSA_ERRCODE_IMS_BASE + QOSA_AT_ERR_OFS),
} qosa_ims_err_e;
// clang-format on

/*-------------------------------------- basic struct --------------------------------------*/




/*-------------------------------------- event --------------------------------------*/

/**
 * @struct qosa_ims_ring_event_t
 * @brief Call ring event parameters, corresponding to event ID QOSA_EVENT_MODEM_IMS_RING_STATUS
 *
 */
typedef struct
{
    qosa_uint8_t call_type; /*!< Used to indicate the URC format for ringing report. Currently, it has no practical effect. */
} qosa_ims_ring_event_t;

/**
 * @struct qosa_ims_disconnect_event_t
 * @brief Call disconnect event parameters, corresponding to event ID QOSA_EVENT_MODEM_IMS_DISCONNECT_STATUS
 *
 */
typedef struct
{
    qosa_uint8_t no_carrier; /*!< Used to indicate that the called party of the call did not answer or hung up. Currently, it has no practical effect. */
} qosa_ims_disconnect_event_t;

/**
 * @struct qosa_ims_conn_id_event_t
 * @brief Call connection event parameters, corresponding to event ID QOSA_EVENT_MODEM_IMS_CONN_ID_EVNET
 *
 */
typedef struct
{
    qosa_uint8_t type;           /*!< Type of address octet in integer format (refer 3GPP TS 24.008 subclause 10.5.4.7) */
    char         dialnumstr[80]; /*!< phone number of format specified by <type> */
} qosa_ims_conn_id_event_t;
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_IMS_H__ */
