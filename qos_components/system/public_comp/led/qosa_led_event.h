/*****************************************************************/ /**
* @file qosa_led_event.h
* @brief LED Event Definitions and Interfaces
* @author
* @date 2025-09-18
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2025-09-18 <td>1.0 <td>Initial <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_LED_EVENT_H__
#define __QOSA_LED_EVENT_H__

#include "qosa_def.h"

/** Mark the event to indicate reverse matching of this event, for example, when not camped on the network, use QOSA_LED_EVENT_NEG(QOSA_LED_EVENT_NETWORK_REGISTERED). Priority inherits from the original event. */
#define QOSA_LED_EVENT_NEG(ev)         (0x80 | (ev))  //  Invert the high-order flag
/** Forward matching */
#define QOSA_LED_EVENT_POS_MATCH(ev)   ((0x80 & (ev)) == 0x00)
/** Reverse matching */
#define QOSA_LED_EVENT_NEG_MATCH(ev)   ((0x80 & (ev)) == 0x80)
/** Get the real event value */
#define QOSA_LED_GET_REL_EVENT_VAL(ev) (0x7f & (ev))
/** Unspecified event */
#define QOSA_LED_EVENT_NOT_SPECIFIC    (0xFF)
/** Driver mapping size */
#define QOSA_LED_DRIVER_MAPPING_SIZE   (2)
/**
 * @brief LED event ID definitions
 */
typedef enum
{
    QOSA_LED_EVENT_POWERON = 0,            /*!< Power on */
    QOSA_LED_EVENT_POWERON_10S = 1,        /*!< Power on for 10 seconds, currently not supported */
    QOSA_LED_EVENT_SIM_NOT_INSERTED = 2,   /*!< SIM not inserted */
    QOSA_LED_EVENT_SIM_INITING = 3,        /*!< SIM card initializing */
    QOSA_LED_EVENT_SIM_LOCKED = 4,         /*!< SIM card lock PIN */
    QOSA_LED_EVENT_SIM_READY = 5,          /*!< SIM card ready */
    QOSA_LED_EVENT_NETWORK_CAMP = 6,       /*!< Camp on the cell, currently not supported */
    QOSA_LED_EVENT_NETWORK_SEARCHING = 7,  /*!< In network search */
    QOSA_LED_EVENT_NETWORK_REGISTERED = 8, /*!< Network is registered */
    QOSA_LED_EVENT_PDN_ACTIVATING = 9,     /*!< PDN activation in progress, currently not supported */
    QOSA_LED_EVENT_PDN_CONNECTED = 10,     /*!< PDN connected */
    QOSA_LED_EVENT_SOCKET_CONNECTED = 11,  /*!< SOCKET is connected */
    QOSA_LED_EVENT_DATA_TRANSMIT = 12,     /*!< Data transmission in progress */
    QOSA_LED_EVENT_PDN_DEACTIVATING = 13,  /*!< PDN deactivation in progress, currently not supported */
    QOSA_LED_EVENT_DIALING = 14,           /*!< In a call, currently not supported */
    QOSA_LED_EVENT_DATA_TRANSMIT_IDLE_WITH_AT = 15,/*!< uart data transmit idle status with first at*/
    QOSA_LED_EVENT_DATA_TRANSMIT_BUSY_WITH_AT = 16,/*!< uart data transmit busy status with first at*/
    QOSA_LED_EVENT_UART_NOT_SYN_WITH_AT = 17,/*!< Uart failed to complete synchronization within the specified time.*/
    QOSA_LED_EVENT_FILE_DOWNLOADING = 18,/*!< During FTP normal file download or fota file downlaod.*/
    #ifdef CONFIG_QOSA_OEM_RWR_FUNC
    QOSA_LED_EVENT_MQTT_CONNECTED = 19,/*!< MQTT protocol connection (CONNACK) established*/
    #endif
    QOSA_LED_EVENT_MAX
} qosa_led_event_e;

/**
 * @brief The platform layer uses this interface to trigger LED events.
 * QOSA_LED_UPDATE_SIM_STATUS: Used to report SIM card status. It directly affects the determination of SIM_NOT_INSERTED, SIM_INITING, SIM_LOCKED, and SIM_READY states in the LED.
 * QOSA_LED_UPDATE_REG_STATUS: Used to report network registration status. It directly affects the determination of NETWORK_SEARCHING and NETWORK_REGISTERED states in the LED.
 * QOSA_LED_UPDATE_NETIF_STATUS: Used to report network interface status. It indirectly affects the determination of the DATA_TRANSMIT state in the LED.
 * QOSA_LED_UPDATE_PPP_STATUS: Used to report PPP connection status. It indirectly affects the determination of the DATA_TRANSMIT state in the LED.
 * QOSA_LED_UPDATE_USBNET_STATUS: Used to report USB network connection status. It indirectly affects the determination of the DATA_TRANSMIT state in the LED.
 * QOSA_LED_UPDATE_DATA_TRANSMIT_STATUS: Used to report data transmission status. It indirectly affects the determination of the DATA_TRANSMIT state in the LED.
 * QOSA_LED_UPDATE_SOCKET_STATUS: Used to report Socket connection status. It directly affects the determination of the SOCKET_CONNECTED state in the LED.
 *
 */
#ifdef CONFIG_QOSA_LED_FUNC
#define QOSA_LED_EVENT_TRIGGER(_simid, _event_id)                           qosa_led_set_event(_simid, _event_id)
#define QOSA_LED_UPDATE_SIM_STATUS(_simid, _pin_status)                     qosa_led_update_sim_status(_simid, _pin_status)
#define QOSA_LED_UPDATE_REG_STATUS(_simid, _reg_status)                     qosa_led_update_reg_status(_simid, _reg_status)
#define QOSA_LED_UPDATE_PDP_STATUS(_simid, _pdp_status)                     qosa_led_update_pdp_status(_simid, _pdp_status)
#define QOSA_LED_UPDATE_NETIF_STATUS(_simid, _pdpid, _netif_status)         qosa_led_update_netif_status(_simid, _pdpid, _netif_status)
#define QOSA_LED_UPDATE_PPP_STATUS(_simid, _ppp_status)                     qosa_led_update_ppp_status(_simid, _ppp_status)
#define QOSA_LED_UPDATE_USBNET_STATUS(_simid, _usbnet_status)               qosa_led_update_usbnet_status(_simid, _usbnet_status)
#define QOSA_LED_UPDATE_DATA_TRANSMIT_STATUS(_simid, _data_transmit_status) qosa_led_update_data_transmit_status(_simid, _data_transmit_status)
#define QOSA_LED_UPDATE_SOCKET_STATUS(_simid, _idx, _socket_status)         qosa_led_update_socket_status(_simid, _idx, _socket_status)
#ifdef CONFIG_QOSA_OEM_RWR_FUNC
#define QOSA_LED_UPDATE_MQTT_STATUS(_simid, _client_id, _mqtt_status)       qosa_led_update_mqtt_status(_simid, _client_id, _mqtt_status)
#endif
#define QOSA_LED_UPDATE_RECEIVE_FIRST_AT_STATUS(_simid)                     qosa_led_update_receive_first_at_status(_simid)
#define QOSA_LED_UPDATE_UART_TRANSMIT_STATUS(_simid, _uart_transmit_status) qosa_led_update_uart_transmit_status(_simid, _uart_transmit_status)
#define QOSA_LED_UPDATE_FILE_DOWNLOAD_STATUS(_simid, _file_download_status) qosa_led_update_file_download_status(_simid, _file_download_status)

#else /* CONFIG_QOSA_LED_FUNC */
#define QOSA_LED_EVENT_TRIGGER(_simid, _event_id)
#define QOSA_LED_UPDATE_SIM_STATUS(_simid, _pin_status)
#define QOSA_LED_UPDATE_REG_STATUS(_simid, _reg_status)
#define QOSA_LED_UPDATE_PDP_STATUS(_simid, _pdp_status)
#define QOSA_LED_UPDATE_NETIF_STATUS(_simid, _pdpid, _netif_status)
#define QOSA_LED_UPDATE_PPP_STATUS(_simid, _ppp_status)
#define QOSA_LED_UPDATE_USBNET_STATUS(_simid, _usbnet_status)
#define QOSA_LED_UPDATE_DATA_TRANSMIT_STATUS(_simid, _data_transmit_status)
#define QOSA_LED_UPDATE_SOCKET_STATUS(_simid, _idx, _socket_status)
#ifdef CONFIG_QOSA_OEM_RWR_FUNC
#define QOSA_LED_UPDATE_MQTT_STATUS(_simid, _client_id, _mqtt_status)
#endif
#define QOSA_LED_UPDATE_RECEIVE_FIRST_AT_STATUS(_simid)
#define QOSA_LED_UPDATE_UART_TRANSMIT_STATUS(_simid, _uart_transmit_status)
#define QOSA_LED_UPDATE_FILE_DOWNLOAD_STATUS(_simid, _file_download_status)

#endif /* CONFIG_QOSA_LED_FUNC */

int qosa_led_set_event(qosa_uint8_t simid, qosa_led_event_e event_id);

void qosa_led_update_sim_status(qosa_uint8_t simid, qosa_uint8_t pin_status);
void qosa_led_update_reg_status(qosa_uint8_t simid, qosa_uint8_t reg_status);
void qosa_led_update_pdp_status(qosa_uint8_t simid, qosa_uint8_t pdp_status);
void qosa_led_update_netif_status(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_uint8_t netif_status);
void qosa_led_update_ppp_status(qosa_uint8_t simid, qosa_uint8_t ppp_status);
void qosa_led_update_usbnet_status(qosa_uint8_t simid, qosa_uint8_t usbnet_status);
void qosa_led_update_data_transmit_status(qosa_uint8_t simid, qosa_uint8_t data_transmit_status);
void qosa_led_update_socket_status(qosa_uint8_t simid, qosa_int8_t idx, qosa_uint8_t socket_status);
#ifdef CONFIG_QOSA_OEM_RWR_FUNC
void qosa_led_update_mqtt_status(qosa_uint8_t simid, qosa_uint8_t client_id, qosa_uint8_t mqtt_status);
#endif
void qosa_led_update_receive_first_at_status(qosa_uint8_t simid);
void qosa_led_update_uart_transmit_status(qosa_uint8_t simid, qosa_uint8_t uart_transmit_status);
void qosa_led_update_file_download_status(qosa_uint8_t simid, qosa_uint8_t file_download_status);
/**
 * @brief Get LED status bits
 *
 * Retrieves the current LED status bits, where each bit corresponds to an LED event status.
 *
 * @param[in] simid
 *          - SIM card ID (currently unused, reserved parameter)
 * @param[out] status
 *          - LED status bits, each bit corresponds to an LED event status
 * @return void
 */
void qosa_led_get_status(qosa_uint8_t simid, qosa_uint32_t* status);

/**
 * @brief LED platform module initialization
 *
 * Initializes the mutex for the LED platform module
 *
 * @return int 0 indicates success, non-zero indicates failure
 */
int qosa_led_platform_init(void);

#endif /* __QOSA_LED_EVENT_H__ */
