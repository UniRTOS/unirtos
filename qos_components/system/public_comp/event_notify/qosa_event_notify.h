/*****************************************************************/ /**
* @file qosa_event_notify.h
* @brief
* @author larson.li@quectel.com
* @date 2023-05-30
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-05-30 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __EVENT_NOTIFY_H__
#define __EVENT_NOTIFY_H__

#include "qosa_def.h"
#include "qosa_sys.h"

#define QOSA_ERRCODE_EVENT_NOTIFY_BASE (QOSA_COMPONENT_EVENT_NOTIFY << 16)

typedef enum
{
    QOSA_EVENT_NOTIFY_OK = QOSA_OK,
    QOSA_EVENT_NOTIFY_REGISTER_FULL_ERROR = QOSA_ERRCODE_EVENT_NOTIFY_BASE, /*!< register full error*/
    QOSA_EVENT_NOTIFY_ILLEGAL_EVENT_ERROR,                                  /*!< illegal event error */
    QOSA_EVENT_NOTIFY_REGISTER_CB_IS_NULL,                                  /*!< register cb is null error*/
    QOSA_EVENT_NOTIFY_NO_REGISTER_ERROR,                                    /*!< EVENT type is no register */
    QOSA_EVENT_NOTIFY_CREATE_MUTEX_ERROR,                                   /*!< create mutex is error */
    QOSA_EVENT_NOTIFY_NO_INIT_ERROR,                                        /*!< No initialization error */
    QOSA_EVENT_NOTIFY_MUTEX_LOCK_ERROR,                                     /*!< Failed to Execute Mutex */

    QOSA_EVENT_NOTIFY_MAX_ERROR
} qosa_e_n_error_e;

typedef enum
{
    QOSA_EVENT_NONE = 0,
    QOSA_EVENT_NET_PDP_ACT = 1,          /*!< PDP activation status changed, @ref qosa_datacall_act_event_t */
    QOSA_EVENT_NW_PDN_DEACT = 2,         /*!< Network-initiated PDN deactivation needs to report QIURC deactive, @ref qosa_datacall_nw_deact_event_t */
    QOSA_EVENT_USBNET_STATUS,            /*!< USB network status change notification, @ref qosa_usbnet_status_event_t */
    QOSA_EVENT_ESCAPE_DATA_OR_EDIT_MODE, /*!< dev channel exits transparent transmission mode/edit mode */
    QOSA_EVENT_AT_TX_HIGH,               /*!< AT channel TX high water mark notification */
    QOSA_EVENT_AT_TX_LOW,                /*!< AT channel TX low water mark notification */
    QOSA_EVENT_MODEM_SIM_STATUS,         /*!< SIM card status change, @ref qosa_sim_status_event_t */
    QOSA_EVENT_MODEM_SIM_INSERT_STATUS,  /*!< SIM card insertion status change, @ref qosa_sim_insert_status_event_t */
    QOSA_EVENT_MODEM_SMS_STATUS,         /*!< SMS initialization OK status report */
    QOSA_EVENT_MODEM_SMS_NEW_MSG = 10,        /*!< New SMS arrival notification, @ref qosa_sms_new_msg_event_t */
    QOSA_EVENT_MODEM_SMS_STORAGE_FULL,   /*!< SMS storage space insufficient notification, @ref qosa_sms_storage_full_event_t */
    QOSA_EVENT_MODEM_NW_CS_REG_STATUS,   /*!< CS domain network status change, @ref qosa_nw_reg_status_event_t */
    QOSA_EVENT_MODEM_NW_PS_REG_STATUS,   /*!< PS domain network status change, @ref qosa_nw_reg_status_event_t */
    QOSA_EVENT_MODEM_NW_SIGNAL_QUALITY,  /*!< Report of network-side signal quality change, @ref qosa_nw_reg_signal_quality_event_t */
    QOSA_EVENT_MODEM_NW_RRC_STATUS,      /*!< RRC state change, @ref qosa_nw_rrc_conn_event_t */
    QOSA_EVENT_MODEM_NW_NITZ_INFO,       /*!< NITZ time change, @ref qosa_nw_nitz_event_t */
    QOSA_EVENT_MODEM_NW_NAS_EVENT,       /*!< NAS event reporting, @ref qosa_nw_nas_event_t */
    QOSA_EVENT_SOFTBANK_SIM_READY,       /*!< Temporarily record SIM card status changes */
    QOSA_EVENT_PSM_SLEEP_STATUS,         /*!< PSM sleep state notification, @ref qosa_lpm_psm_event_t */
    QOSA_EVENT_DEV_AIRPLANE_CTRL_EVENT = 20,  /*!< Flight mode status change notification */
    QOSA_EVENT_SYSTEM_COMPLETE,          /*!< Platform side initialization completed */
    QOSA_EVENT_MODEM_NW_REJECT_EVENT,    /*!< Network rejection event */
    QOSA_EVENT_LPM_SLEEP_STATUS,         /*!< Device sleep or wake event */
    QOSA_EVENT_MODEM_JAMM_DETECT_STATUS, /*!< Interference detection result report, @ref qosa_nw_jamm_detect_event_t */
    QOSA_EVENT_MODEM_NW_CELL_CHANGE_EVENT,     /*!< Cell change event, @ref qosa_nw_cell_change_event_t*/
    QOSA_EVENT_MODEM_NW_AS_EVENT,              /*!< AS event reporting, @ref qosa_nw_as_event_t */
    QOSA_EVENT_NW_DETACH,                /*!< Network detach event @ref qosa_nw_detach_event_t*/
    QOSA_EVENT_MODEM_IMS_RING_STATUS,    /*!< Ring reports that there is an incoming call */
    QOSA_EVENT_MODEM_IMS_DISCONNECT_STATUS,    /*!< NO CARRIER reports that there is an incoming call */
    QOSA_EVENT_MODEM_IMS_CONN_ID_EVNET,        /*!< CONN ID reports that there is an incoming call */
    QOSA_EVENT_MODEM_SMS_CBM,                  /*!< Cell broadcast or CMAS message notification, @ref qosa_sms_cbm_notify_event_t */
    QOSA_EVENT_URC_CNT_LOW,              /*!< URC CNT is a LOW notification. */
    QOSA_EVENT_MODEM_NW_EDRX_DYN_PARM,     /*!< EDRX status change notification, @ref qosa_nw_edrx_status_event_t */
    QOSA_EVENT_SECONDARY_PDP_ACT,        /*!< Secondary PDP activation status changed, @ref qosa_datacall_secondary_pdp_act_event_t */
#ifdef CONFIG_QOSA_SIM_STK_FUNC
    QOSA_EVENT_MODEM_SIM_STK_PROACTIVE_CMD,    /*!< SIM proactive command event, @ref qosa_sim_stk_proactive_event_t */
    QOSA_EVENT_MODEM_SIM_STK_PROACTIVE_END,    /*!< SIM proactive command session end event */
#endif
    QOSA_EVENT_MAX                       /* the max number of the event to be registered */
} qosa_notify_event_e;

typedef int (*event_callback_ptr)(void *user_argv, void *argv);

/**
 * @brief Used by external other function modules to register for event interfaces
 *
 * @param[in] qosa_notify_event_e event
 *           - Types of events that require registration.
 *
 * @param[in] event_callback_ptr event_cb
 *           - Callback function for registering event notifications.
 *
 * @param[in] void * user_argv
 *           - Callback function carrying user parameters.
 *
 * @return qosa_e_n_error_e
 *        - Returning QOSA_EVENT_NOTIFY_OK or QOSA_OK indicates success,
 *          while returning other values indicates execution failure.
 */
qosa_e_n_error_e qosa_event_notify_register(qosa_notify_event_e event, event_callback_ptr event_cb, void *user_argv);

/**
 * @brief Event notification to unregister function
 *
 * @param[in] qosa_notify_event_e event
 *           - unregister event type
 *
 * @param[in] event_callback_ptr event_cb
 *           - unregister event callback pointer
 *
 * @return qosa_e_n_error_e
 *        - Returning QOSA_EVENT_NOTIFY_OK or QOSA_OK indicates success,
 *          while returning other values indicates execution failure.
 *
 * @note Please note that there may be N different cb functions for
 *      - the same registered event type. Therefore, when unregistering
 *       an interface, you must carry the cb function pointer from the
 *        time of registration.
 */
qosa_e_n_error_e qosa_event_notify_unregister(qosa_notify_event_e event, event_callback_ptr event_cb);

/**
 * @brief event active notification function
 *
 * @param[in] qosa_notify_event_e event
 *           - active notification event type
 *
 * @param[in] void * notifyData
 *          -  active notification parameters
 *
 * @return qosa_e_n_error_e
 *        - Returning QOSA_EVENT_NOTIFY_OK or QOSA_OK indicates success,
 *          while returning other values indicates execution failure.
 */
qosa_e_n_error_e qosa_event_notify_report(qosa_notify_event_e event, void *notifyData);

#endif /* __EVENT_NOTIFY_H__ */
