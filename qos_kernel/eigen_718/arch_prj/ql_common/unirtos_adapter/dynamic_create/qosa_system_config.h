/* Copyright (C) 2020 UNIRTOS Technologies Limited and/or its affiliates("UNIRTOS").
 * All rights reserved.
 */

#ifndef _QOSA_SYSTEM_CONFIG_H_
#define _QOSA_SYSTEM_CONFIG_H_

/**
 * use linux platform config
 */
/* #undef CONFIG_QOSA_LINUX_PLATFORM_CFG */

/**
 * use rtos platform config
 */
/* #undef CONFIG_QOSA_RTOS_PLATFORM_FUNC */

/**
 * use rtos opencpu config
 */
/* #undef CONFIG_QOSA_OPENCPU_FUNC */

/**
 * Virtual file system function module
 */
#define CONFIG_QOSA_VFS_FILESYSTEM_FUNC

/**
 * Bidirectional linked table manipulation functions
 */
#define CONFIG_QOSA_LINK_LIST_FUNC

/**
 * Data service manger function
 */
#define CONFIG_QOSA_BUFFER_BLOCK_FUNC

/**
 * SSL third-party library support
 */
#define CONFIG_QOSA_SSL_FUNC

/**
 * UniRTOS OSA_LWM2M function of object
 */
/* #undef CONFIG_QOSA_LWM2M_OBJECT_4 */

/**
 * Provide synchronous and asynchronous DNS resolution capabilities
 */
#define CONFIG_QOSA_SOCKET_DNS_FUNC

/**
 * Provide registration notification capability
 */
#define CONFIG_QOSA_EVENT_NOTIFY_FUNC

/**
 * Provide log printing and output capabilities
 */
#define CONFIG_QOSA_LOG_DEBUG_FUNC

/**
 *  VSIM adapt function support
 */
#define CONFIG_QOSA_VSIM_ADAPT_FUNC

/**
 * Provide PDP management capabilities
 */
#define CONFIG_QOSA_DATACALL_FUNC

/**
 * Provide device management capabilities
 */
#define CONFIG_QOSA_MODEM_DEV_FUNC

/**
 * network parameter update and indication report function
 */
#define CONFIG_QOSA_NW_FUNC

/**
 *  * ims parameter update and indication report function
 */
#define CONFIG_QOSA_IMS_FUNC

/**
 * usbnet update and indication report function
 */
#define CONFIG_QOSA_USBNET_FUNC

/**
 * LAN management function support
 */
/* #undef CONFIG_QOSA_LAN_FUNC */

/**
 * QPPP for UniRTOS function
 */
/* #undef CONFIG_QOSA_PPP_FUNC */

/**
 * UniRTOS modem EDRX function support
 */
/* #undef CONFIG_QOSA_NW_EDRX_FUNC */

/**
 * UniRTOS modem antenna signal info support (AT+QANTINFO)
 */
/* #undef CONFIG_QOSA_NW_ANTENNA_INFO_FUNC */

/**
 * UniRTOS modem SIM/(U)SIM function support
 */
#define CONFIG_QOSA_SIM_FUNC

/**
 * UniRTOS modem SIM STK (SAT/TERMINAL PROFILE) function support
 */
/* #undef CONFIG_QOSA_SIM_STK_FUNC */

/**
 * UniRTOS modem SMS function support
 */
#define CONFIG_QOSA_SMS_FUNC

/**
 * UniRTOS modem SMS CBS function support
 */
/* #undef CONFIG_QOSA_SMS_CBS_FUNC */

/**
 * UniRTOS modem Dual SIM Dual Standby function support
 */
/* #undef CONFIG_QOSA_DSDS_FUNC */

/**
 * Number of board-level SIM card slots
 */
#define CONFIG_QOSA_SIM_HAL_SLOT_NUM 1
/**
 * mbedtls library types
 */
#define CONFIG_QOSA_MBEDTLS_LIBRARY_FUNC

/**
 * mbedtls3.x.x library suport TLS1.3
 */
/* #undef CONFIG_QOSA_MBEDTLS3_X_X_LIBRARY_FUNC */

/**
 * mbedtls2.28.x library
 */
/* #undef CONFIG_QOSA_MBEDTLS2_X_X_LIBRARY_FUNC */

/**
 * RTT device function module
 */
/* #undef CONFIG_QOSA_RT_DEVICE_FUNC */

/**
 * RTT device architecture serial function module
 */
/* #undef CONFIG_QOSA_RT_DEVICE_SERIAL_FUNC */

/**
 * SIO manager function module
 */
#define CONFIG_QOSA_WATERMARK_FUNC

/**
 * OSA_SYSTEM manager function module
 */
/* #undef CONFIG_QOSA_LINK_SYSTEM_LIBRARIES_FUNC */

/**
 * UniRTOS usb uart function
 */
/* #undef CONFIG_QOSA_UDP_UART_FUNC */

/**
 * UniRTOS wifi scan function
 */
#define CONFIG_QOSA_WIFISCAN_FUNC

/**
 * UniRTOS modem WIFI function support
 */
/* #undef CONFIG_QOSA_WIFI_FUNC */

/**
 * UniRTOS GNSS function support
 */
/* #undef CONFIG_QOSA_GNSS_FUNC */

/**
 * UniRTOS FTM function
 */
#define CONFIG_QOSA_FTM_FUNC

/**
 * UniRTOS WiFi RF factory test function
 */
/* #undef CONFIG_QOSA_WIFI_RF_TEST_FUNC */

/**
 * UniRTOS AT parser function
 */
#define CONFIG_QOSA_AT_PARSER_FUNC

/**
 * UniRTOS CLI function
 */
/* #undef CONFIG_QOSA_CLI_FUNC */

/**
 * UniRTOS secdata function
 */
/* #undef CONFIG_QOSA_SECDATA_FUNC */

/**
 * UniRTOS QGMR with app
 */
/* #undef CONFIG_QOSA_AT_QGMR_APP */

/**
 * UniRTOS LPM function
 */
#define CONFIG_QOSA_LPM_FUNC

/**
 * UniRTOS LED function support
 */
/* #undef CONFIG_QOSA_LED_FUNC */

/**
 * UniRTOS MIFI base function support
 */
/* #undef CONFIG_QOSA_MIFI_FUNC */

/**
 * UniRTOS LAN function support
 */
/* #undef CONFIG_QOSA_LAN_FUNC */

/**
 * UniRTOS MIFI LED function support
 */
/* #undef CONFIG_QOSA_MIFI_LED_FUNC */

/**
 * UniRTOS MIFI KEY function support
 */
/* #undef CONFIG_QOSA_MIFI_KEY_FUNC */


/**
 * UniRTOS LED_GPIO_BLINK function support
 */
/* #undef CONFIG_QOSA_LED_GPIO_BLINK_FUNC */

/**
 * use license function module
 */
#define CONFIG_QOSA_LICENSE_FUNC

/**
 * use license function support
 */
/* #undef CONFIG_QOSA_LICENSE_SUPPORT */

/**
 * UniRTOS CAMERA function
 */
#define CONFIG_QOSA_CAMERA_FUNC

/**
 * UniRTOS LCD function
 */
#define CONFIG_QOSA_LCD_FUNC

/**
 * UniRTOS IIC function
 */
#define CONFIG_QOSA_IIC_FUNC

/**
 * UniRTOS SPI function
 */
#define CONFIG_QOSA_SPI_FUNC

/**
 * UniRTOS AUDIO function
 */
#define CONFIG_QOSA_AUDIO_DRIVER

/**
 * UniRTOS FTP function
 */
/* #undef CONFIG_QOSA_QURL_FTP_FUNC */

/**
 * UniRTOS Virtual AT macro control
 */
#define CONFIG_QOSA_VIRT_AT_FUNC

/**
 * UniRTOS SMTP function
 */
/* #undef CONFIG_QOSA_QURL_SMTP_FUNC */

/**
 * UniRTOS TTS function
 */
#define CONFIG_QOSA_TTS_FUNC

/**
 * UniRTOS Code Decoder function
 */
#define CONFIG_QOSA_CODE_DECODER_FUNC

/**
 * UniRTOS Code Decoder control
 */
/* #undef CONFIG_QOSA_UNIRTOS_CODE_DECODER_FUNC */

/**
 * QINGYA Code Decoder control
 */
#define CONFIG_QOSA_QINGYA_CODE_DECODER_FUNC

/**
 * UniRTOS Flash function
 */
/* #undef CONFIG_QOSA_FLASH_TEST_FUNC */

/**
 * UniRTOS NBIOT customized applications
 */
/* #undef CONFIG_QOSA_NBIOT_CUST_FUNC */

/**
 * UniRTOS lpm psm mode support
 */
#define CONFIG_QOSA_LPM_PSM_SUPPORT_FUNC

/**
 * UniRTOS lpm sleep log mode support
 */
/* #undef CONFIG_QOSA_LPM_SLEEP_LOG_FUNC */

/**
 * UniRTOS lpm rfr legacy function support
 */
/* #undef CONFIG_QOSA_LPM_RFR_LEGACY_FUNC */

/**
 * UniRTOS TTS function
 */
#define CONFIG_QOSA_TTS_SUPPORT

/**
 * UniRTOS TTS resource store to fs function
 */
/* #undef CONFIG_QOSA_TTS_RESOURCE_TO_FS_SUPPORT */

/**
 * UniRTOS Code decoder function
 */
/* #undef CONFIG_QOSA_CODE_DECODER_SUPPORT */

/**
 * UniRTOS LCD function
 */
/* #undef CONFIG_QOSA_LCD_SUPPORT */

/**
 * Dual panel LCD support
 */
/* #undef CONFIG_QOSA_LCD_DUAL_PANEL */

/**
 * UniRTOS CAMERA function
 */
/* #undef CONFIG_QOSA_CAMERA_SUPPORT */

/**
 * UniRTOS CAN function
 */
/* #undef CONFIG_QOSA_CAN_SUPPORT */

/**
 * UniRTOS FOTA SIGN function
 */
#define CONFIG_QOSA_FOTA_SIGN_FUNC

/**
 * UniRTOS secboot function
 */
#define CONFIG_QOSA_SECBOOT_FUNC

/**
 * UniRTOS secboot support
 */
/* #undef CONFIG_QOSA_SECBOOT_SUPPORT */

/**
 * UniRTOS DTR support
 */
/* #undef CONFIG_QOSA_DTR_FUNC */

/**
 * UniRTOS OEM LT function
 */
/* #undef CONFIG_QOSA_OEM_LT_FUNC */

/**
 * UniRTOS OEM SE function
 */
/* #undef CONFIG_QOSA_OEM_SE_FUNC */

/**
 * UniRTOS OEM RWR function
 */
/* #undef CONFIG_QOSA_OEM_RWR_FUNC */

/**
 * UniRTOS OEM GW (State Grid / 国网) function
 */
/* #undef CONFIG_QOSA_OEM_DL_FUNC */

/**
 * UniRTOS OEM BSQ function
 */
/* #undef CONFIG_QOSA_OEM_BSQ_FUNC */

/**
 * UniRTOS OEM ALLIED function
 */
/* #undef CONFIG_QOSA_OEM_ALLIED_FUNC */

/**
 * UniRTOS OEM GENUS function
 */
/* #undef CONFIG_QOSA_OEM_GENUS_FUNC */

/**
 * UniRTOS CONTROL CALL Function
 */
#define CONFIG_QOSA_CALL_CONTROL_FUNC

/**
 * UniRTOS VOICE CALL Function
 */
#define CONFIG_QOSA_VOICE_CALL_FUNC

/**
 * UniRTOS CLAT Function
 */
#define CONFIG_QOSA_CLAT_FUNC

/**
 * UniRTOS JD Function
 */
#define CONFIG_QOSA_JD_FUNC

/**
 * UniRTOS Customer-customized AI recognition for cameras Function
 */
/* #undef CONFIG_QOSA_CAMERA_WATER_METER_AI_FUNC */

/**
 * UniRTOS Customer-customized AI recognition for cameras
 */
/* #undef CONFIG_QOSA_CAMERA_WATER_METER_AI_SUPPORT */

/**
 * UniRTOS secboot function
 */
#define CONFIG_QOSA_USBHID_FUNC
/**
 * UniRTOS autoapn function
 */
/* #undef CONFIG_QOSA_AUTOAPN_FUNC */

/**
 * UniRTOS MBEDTLS function
 */
/* #undef CONFIG_QOSA_MBEDTLS_SUPPORT */

/**
 * UniRTOS NON-ESIM VERSION EID QUERY FUNCTION
 */
/* #undef CONFIG_QOSA_ESIM_EID_QUERY_FUNC */

/**
 * UniRTOS basestation function
 */
#define CONFIG_QOSA_ABNORMAL_BASESTATION_FUNC

/**
 * UniRTOS netdev core function
 */
/* #undef CONFIG_QOSA_NETDEV_CORE_FUNC */
/* #undef CONFIG_QOSA_NETDEV_CORE_NAT_FUNC */
/* #undef CONFIG_QOSA_NETDEV_CORE_DHCPD_FUNC */

/**
 * UniRTOS netdev bus function
 */
/* #undef CONFIG_QOSA_NETDEV_BUS_FUNC */
/* #undef CONFIG_QOSA_NETDEV_BUS_SPI_FUNC */
/* #undef CONFIG_QOSA_NETDEV_BUS_GPIO_FUNC */
/* #undef CONFIG_QOSA_NETDEV_BUS_UART_FUNC */


/**
 * UniRTOS CMUX function
 */
/* #undef CONFIG_UNIRTOS_CMUX_SUPPORT */
/**********************************************************************/
/*       Platform related configuration macro definitions                                        */
/**********************************************************************/

/**
 * linux platform function support
 */
/* #undef CONFIG_QOSA_LINUX_PLATFORM_FUNC */

/**
 * RDA8910 platform function support
 */
/* #undef CONFIG_QOSA_RDA8910_PLATFORM_FUNC */

/**
 * RDA8910 R06 platform function support
 */
/* #undef CONFIG_QOSA_RDA8910_R06_PLATFORM_FUNC */

/**
 * QCX216 platform function support
 */
/* #undef CONFIG_QOSA_QCX216_PLATFORM_FUNC */

/**
 * QCX217 platform function support
 */
/* #undef CONFIG_QOSA_QCX217_PLATFORM_FUNC */

/**
 * ASR1903SR platform function support
 */
/* #undef CONFIG_QOSA_ASR1903_PLATFORM_FUNC */

/**
 * ASR1803 platform function support
 */
/* #undef CONFIG_QOSA_ASR1803_PLATFORM_FUNC */

/**
 * ASR1805 platform function support
 */
/* #undef CONFIG_QOSA_ASR1805_PLATFORM_FUNC */

/**
 * ASR160X platform function support
 */
/* #undef CONFIG_QOSA_ASR160X_PLATFORM_FUNC */

/**
 * YX718 platform function support
 */
#define CONFIG_QOSA_EIGEN718_PLATFORM_FUNC

/**
 * UNISOC 8852 platform function support
 */
/* #undef CONFIG_QOSA_UIS8852_PLATFORM_FUNC */

/**
 * UNISOC 8360S platform function support
 */
/* #undef CONFIG_QOSA_UIS8360S_PLATFORM_FUNC */

/**
 * XY4100 platform function support
 */
/* #undef CONFIG_QOSA_XY4100_PLATFORM_FUNC */

/**
 * UNISOC 8910 platform function support
 */
/* #undef CONFIG_QOSA_UIS8910_PLATFORM_FUNC */

/**
 * UniRTOS CPU Usage AT function
 */
#define CONFIG_QAPP_UNIRTOS_CPU_USAGE_FUNC

#endif /* _QOSA_SYSTEM_CONFIG_H_ */
