/*****************************************************************/ /**
* @file qosa_center_task.h
* @brief
* @author larson.li@quectel.com
* @date 2023-05-09
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-05-09 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __AT_TASK_H__
#define __AT_TASK_H__

#include "qosa_def.h"

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 

#include "qosa_sys.h"
#include "qosa_queue_list.h"

/** Maximum number of events that the current AT task can store */
#define QOSA_CENTER_EVENT_MAC_CMD_BUFF        50
#define QOSA_CENTER_EVENT_APPS_RESERVER_START 0x100

/**
 * Event types registered to AT task
 */
typedef enum
{
    QOSA_CENTER_EVENT_MIN_CMD = 0,

    /*!< AT parser new data arrival notification */
    QOSA_CENTER_EVENT_AT_PARSER_RECV_CMD,

    /*!< AT parser maximum timeout event notification for waiting \r character */
    QOSA_CENTER_EVENT_AT_PARSER_LF_TIMEOUT_CMD,

    /*!< AT parser AT command execution result maximum wait event timeout notification */
    QOSA_CENTER_EVENT_AT_PARSER_RESP_TIMEOUT_CMD,

    /*!< AT parser receives +++ exit event notification from driver layer */
    QOSA_CENTER_EVENT_AT_PARSER_ESCAPE_CMD,

    /*!< AT parser receives dtr event notification from driver layer */
    QOSA_CENTER_EVENT_AT_PARSER_DTR_CMD,

    /*!< Used for operating URC to call SIO for URC output */
    QOSA_CENTER_EVENT_URC_FLUSH_CMD,

    /*!< Used for UniRTOS FILE AT command execution result return */
    QOSA_CENTER_EVENT_FILETASK_RESULT_CMD,

    /*!< Used for UniRTOS TCPIP AT command execution result return */
    QOSA_CENTER_EVENT_TCPIP_RESULT_CMD,

    /*!< Used for UniRTOS TCPIP NB AT command execution result return */
    QOSA_CENTER_EVENT_TCPIP_NB_RESULT_CMD,

    /*!< Used for UniRTOS PING AT command execution result return */
    QOSA_CENTER_EVENT_PING_RESULT_CMD,

    /*!< Used for UniRTOS HTTP AT command execution result return */
    QOSA_CENTER_EVENT_HTTP_RESULT_CMD,

    /*!< Used for UniRTOS FTP AT command execution result return */
    QOSA_CENTER_EVENT_FTP_RESULT_CMD,

    /*!< Used for UniRTOS SMTP AT command execution result return */
    QOSA_CENTER_EVENT_SMTP_RESULT_CMD,

    /*!< Used for UniRTOS LPM AT command execution result return */
    QOSA_CENTER_EVENT_LPM_RESULT_CMD,

    QOSA_CENTER_EVENT_TCPIP_FROM_SIO_DATA,
    /*!< Write data to SIO */
    QOSA_CENTER_EVENT_TCPIP_TO_SIO_DATA,
    /*!< SIO mode switch */
    QOSA_CENTER_EVENT_TCPIP_SWITCH_MODE_CHANGE,

    /*!< Used for UniRTOS PPP AT command execution result return */
    QOSA_CENTER_EVENT_PPP_RESULT_CMD,

    /*!< MODEM asynchronous message passing */
    QOSA_CENTER_EVENT_MODEM_ASYNC_CMD,

    /*!< AT side handles events reported by SIM module */
    QOSA_CENTER_EVENT_SIM_IND_CMD,

    /*!< AT side handles events reported by SMS module */
    QOSA_CENTER_EVENT_SMS_IND_CMD,

    /*!< Used for UniRTOS modem AT command execution result return */
    QOSA_CENTER_EVENT_MODEM_AT_RESULT_CMD,

    /*!< AT side handles events reported by network module */
    QOSA_CENTER_EVENT_NW_IND_CMD,

    /*!< AT side handles events reported by network module */
    QOSA_CENTER_EVENT_USBNET_IND_CMD,

    /*!< AT side handles events reported by device module */
    QOSA_CENTER_EVENT_DEVICE_IND_CMD,

    /*!< Used for UniRTOS lwm2m AT command execution result return processing */
    QOSA_CENTER_EVENT_LWM2M_RESULT_CMD,

    /*!< Used for onenet lwm2m AT command execution result return processing */
    QOSA_CENTER_EVENT_MIPL_LWM2M_RESULT_CMD,

    /*!< Used for aep lwm2m AT command execution result return processing */
    QOSA_CENTER_EVENT_AEP_LWM2M_RESULT_CMD,

    /*!< Used for UniRTOS MMS AT command execution result return */
    QOSA_CENTER_EVENT_MMS_RESULT_CMD,

    /*!< Used for UniRTOS MQTT AT command execution result return */
    QOSA_CENTER_EVENT_MQTT_RESULT_CMD,

    /*!< NTP AT instruction command execution result return */
    QOSA_CENTER_EVENT_NTP_RESULT_CMD,

    /*!< FOTA download instruction command result return */
    QOSA_CENTER_EVENT_QFOTADL_RESULT_CMD,

    /*!< LBS AT instruction command execution result return */
    QOSA_CENTER_EVENT_LBS_RESULT_CMD,

    /*!< SBFOTA AT instruction command execution result return */
    QOSA_CENTER_EVENT_SBFOTA_RESULT_CMD,

    /*!< Serial port configuration processing */
    QOSA_CENTER_EVENT_UART_CFG_CMD,

    /*!< websocket AT instruction return result */
    QOSA_CENTER_EVENT_WEB_RESULT_CMD,

    /*!< DMHTTP AT instruction return result */
    QOSA_CENTER_EVENT_DMHTTP_RESULT_CMD,

    /*!< GNSS AT instruction return result */
    QOSA_CENTER_EVENT_GNSS_RESULT_CMD,

    /*!< Used for UniRTOS COAP AT command execution result return */
    QOSA_CENTER_EVENT_COAP_RESULT_CMD,

    /*!< Used for UniRTOS CAMERA AT command execution result return */
    QOSA_CENTER_EVENT_CAMERA_TASK_RESULT_CMD,

    /*!< Used for UniRTOS ftm command execution result return */
    QOSA_CENTER_EVENT_FTM_RESULT_CMD,

    /*!< Used for UniRTOS CAMERA application to write data to SIO */
    QOSA_CENTER_EVENT_CAMERA_TO_SIO_DATA,

    /*!< Used for UniRTOS FLASH TEST AT command execution result return */
    QOSA_CENTER_EVENT_FLASHTEST_TASK_RESULT_CMD,

    /*!< Used for UniRTOS FLASH TEST AT command execution result return */
    QOSA_CENTER_EVENT_QESIM_RESULT_CMD,

    /*!< AT side handles events reported by jamm_detect */
    QOSA_CENTER_EVENT_JD_IND_CMD,

    /*!< Used for UniRTOS CIND AT command execution result return */
    QOSA_CENTER_EVENT_CIND_RESULT_CMD,

    /*!< Used for UniRTOS QNWSCAN AT command execution result return */
    QOSA_CENTER_EVENT_QNWSCAN_RESULT_CMD,

    /*!< Used for UniRTOS ims event reported */
    QOSA_CENTER_EVENT_IMS_IND_CMD,

    /*!< Place the system app event before this event */
    QOSA_CENTER_EVENT_SYSTEM_MAX_CMD,
    /*!< APPS customer-defined use of core AT task business, only for AT result return, cannot do time-consuming actions */
    QOSA_CENTER_EVENT_APPS_USE_RESERVER0_CMD = QOSA_CENTER_EVENT_APPS_RESERVER_START,
    QOSA_CENTER_EVENT_APPS_USE_RESERVER1_CMD,
    QOSA_CENTER_EVENT_APPS_USE_RESERVER2_CMD,
    QOSA_CENTER_EVENT_APPS_USE_RESERVER3_CMD,
    QOSA_CENTER_EVENT_APPS_USE_RESERVER4_CMD,
    QOSA_CENTER_EVENT_APPS_USE_RESERVER5_CMD,
    QOSA_CENTER_EVENT_APPS_USE_RESERVER6_CMD,
    QOSA_CENTER_EVENT_APPS_USE_RESERVER7_CMD,
    QOSA_CENTER_EVENT_APPS_USE_RESERVER8_CMD,
    QOSA_CENTER_EVENT_APPS_USE_RESERVER9_CMD,

    QOSA_CENTER_EVENT_MAX_CMD
} qosa_center_cmd_event_enum_e;

/**
 * Function registration callback type
 */
typedef void (*qosa_center_cmd_handler_ptr)(qosa_center_cmd_event_enum_e cmd, void *user_data_ptr);

/**
* @brief Return AT parser thread handle
*
* @return qosa_task_t
*/
qosa_task_t qosa_center_get_task_ptr(void);

/**
* @brief Used to send corresponding cmd event to AT task
*
* @param[in] qosa_center_cmd_event_enum_e cmd - Corresponding enumeration type event ID
* @param[in] void * user_data_prt - User parameters to be sent and carried
*
*/
void qosa_center_send_cmd(qosa_center_cmd_event_enum_e cmd, void *user_data_prt);

/**
* @brief Used for external modules to set registration functions themselves
*
* @param[in] qosa_center_cmd_event_enum_e cmd - Registered AT command command word
* @param[in] qosa_center_cmd_handler_ptr cmd_handler - Registered user function
*
* @return qosa_center_cmd_handler_ptr Return old registration function pointer
*
*/
qosa_center_cmd_handler_ptr qosa_center_set_cmd_handler(qosa_center_cmd_event_enum_e cmd, qosa_center_cmd_handler_ptr cmd_handler);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __AT_TASK_H__ */
