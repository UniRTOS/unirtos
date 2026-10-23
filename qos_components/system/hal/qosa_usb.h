/*****************************************************************/ /**
* @file qosa_usb.h
* @brief Usb operation interface
* @author bronson.zhan@quectel.com
* @date 2025-10-22
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-10-22 <td>1.0 <td>Bronson.Zhan <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_USB_H__
#define __QOSA_USB_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"

/*===========================================================================
 * Macro Definition
 ===========================================================================*/

#define QOSA_USB_ERRCODE_BASE (QOSA_COMPONENT_API_USB << 16)

/*===========================================================================
 * Enum
===========================================================================*/
/**
 * @enum qosa_usb_error_e
 * @brief usb error code.
 */
typedef enum
{
    QOSA_USB_SUCCESS = 0,

    QOSA_USB_EXECUTE_ERR = 1 | QOSA_USB_ERRCODE_BASE, /*!< USB execution error */
    QOSA_USB_MEM_ADDR_NULL_ERR,                       /*!< Memory address is null */
    QOSA_USB_INVALID_PARAM,                           /*!< Invalid input param  */
    QOSA_USB_SYS_ERROR,                               /*!< System error  */
    QOSA_USB_NO_SPACE,                                /*!< No space to store data  */
    QOSA_USB_NOT_SUPPORT,                             /*!< Current operation not support  */
    QOSA_USB_REOPEN_ERR,                              /*!< Usb reopen  */
} qosa_usb_error_e;

/**
 * @enum qosa_usb_vbus_state_e
 * @brief VBUS pin connect state.
 */
typedef enum
{
    QOSA_USB_VBUS_NOT_SUPPORT = -1,/*!< VBUS NOT SUPPORT */
    QOSA_USB_VBUS_DISCONNECT = 0, /*!< VBUS pin disconnected */
    QOSA_USB_VBUS_CONNECT,        /*!< VBUS pin connected */
} qosa_usb_vbus_state_e;

/**
 * @enum qosa_usb_state_e
 * @brief USB connect state.
 */
typedef enum
{
    QOSA_USB_STATE_NOT_SUPPORT = -1,  /*!< USB NOT SUPPORT */
    QOSA_USB_DISCONNECT = 0,          /*!< USB disconnected */
    QOSA_USB_CONNECT,                 /*!< USB connected */
} qosa_usb_state_e;

/**
 * @enum qosa_usb_hid_mode_e
 * @brief USB mode selection.
 */
typedef enum 
{
    QOSA_USB_HID_MODE_KEYBOARD = 1,     /*!< USB Just list the keyboard */
    QOSA_USB_HID_MODE_MOUSE,            /*!< USB Just list the mouse. */
    QOSA_USB_HID_MODE_MOUSE_AND_KEYBOARD,
} qosa_usb_hid_mode_e;

/*===========================================================================
 *  Struct
===========================================================================*/

/*===========================================================================
 * callback
===========================================================================*/
typedef qosa_uint32_t (*qosa_usb_vbus_cb)(qosa_usb_vbus_state_e state, void *ctx);

/*===========================================================================
 * Function
===========================================================================*/

/**
 * @brief Initialize USB
 *
 * @return qosa_usb_error_e
 *          - QOSA_USB_SUCCESS: Success
 *          - other: Error code
 */
qosa_usb_error_e qosa_usb_init(void);

/**
 * @brief Deinitialize USB
 *
 * @return qosa_usb_error_e
 *          - QOSA_USB_SUCCESS: Success
 *          - other: Error code
 */
qosa_usb_error_e qosa_usb_deinit(void);

/**
 * @brief Bind VBUS pin interrupt callback
 *
 * @return qosa_usb_error_e
 *          - QOSA_USB_SUCCESS: Success
 *          - other: Error code
 */
qosa_usb_error_e qosa_usb_bind_vbus_cb(qosa_usb_vbus_cb vbus_callback);

/**
 * @brief Get the VBUS pin status
 *
 * @return qosa_usb_error_e
 *          - QOSA_USB_SUCCESS: Success
 *          - other: Error code
 */
qosa_usb_vbus_state_e qosa_usb_get_vbus_state(void);

/**
 * @brief Get the usb connect state
 *
 * @return qosa_usb_error_e
 *          - QOSA_USB_SUCCESS: Success
 *          - other: Error code
 */
qosa_usb_state_e qosa_usb_get_connect_state(void);

/**
 * @brief Send usb hid data
 *
 * @return qosa_int32_t
 *          - QOSA_USB_SUCCESS: Success
 *          - other: Error code
 */
qosa_int32_t qosa_usb_hid_send_proc(qosa_uint8_t hid_num, qosa_uint8_t* sndbuf_ptr, qosa_uint32_t size);

/**
 * @brief hidx is connect
 *
 * @return qosa_int32_t
 *          - QOSA_USB_SUCCESS: Success
 *          - other: Error code
 */
qosa_int32_t qosa_usb_hidx_isconnect(qosa_uint8_t hid_num);

/**
 * @brief Select the option to enable the HID function (for mouse or keyboard)
 *
 * @param[in] qosa_usb_hid_mode_e mode
 *          - mode selection
 *
 * @return qosa_int32_t
 *          - QOSA_USB_SUCCESS: Success
 *          - other: Error code
 */
qosa_int32_t qosa_usb_config_hid(qosa_usb_hid_mode_e mode);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_USB_H__ */
