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
/* #undef CONFIG_QOSA_VFS_FILESYSTEM_FUNC */

/**
 * Bidirectional linked table manipulation functions
 */
#define CONFIG_QOSA_LINK_LIST_FUNC

/**
 * Data service manger function
 */
/* #undef CONFIG_QOSA_BUFFER_BLOCK_FUNC */

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
 * mbedtls library types
 */
/* #undef CONFIG_QOSA_MBEDTLS_LIBRARY_FUNC */

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
/* #undef CONFIG_QOSA_WATERMARK_FUNC */

/**
 * OSA_SYSTEM manager function module
 */
/* #undef CONFIG_QOSA_LINK_SYSTEM_LIBRARIES_FUNC */

/**
 * UniRTOS wifi scan function
 */
/* #undef CONFIG_QOSA_WIFISCAN_FUNC */

/**
 * UniRTOS modem WIFI function support
 */
#define CONFIG_QOSA_WIFI_FUNC

/**
 * UniRTOS GNSS function support
 */
/* #undef CONFIG_QOSA_GNSS_FUNC */

/**
 * UniRTOS FTM function
 */
/* #undef CONFIG_QOSA_FTM_FUNC */

/**
 * UniRTOS WiFi RF factory test function
 */
/* #undef CONFIG_QOSA_WIFI_RF_TEST_FUNC */

/**
 * UniRTOS CLI function
 */
/* #undef CONFIG_QOSA_CLI_FUNC */

/**
 * UniRTOS secdata function
 */
/* #undef CONFIG_QOSA_SECDATA_FUNC */

/**
 * UniRTOS LPM function
 */
#define CONFIG_QOSA_LPM_FUNC

/**
 * UniRTOS LED function support
 */
/* #undef CONFIG_QOSA_LED_FUNC */

/**
 * UniRTOS LED_GPIO_BLINK function support
 */
/* #undef CONFIG_QOSA_LED_GPIO_BLINK_FUNC */

/**
 * use license function module
 */
/* #undef CONFIG_QOSA_LICENSE_FUNC */

/**
 * use license function support
 */
/* #undef CONFIG_QOSA_LICENSE_SUPPORT */

/**
 * UniRTOS CAMERA function
 */
/* #undef CONFIG_QOSA_CAMERA_FUNC */

/**
 * UniRTOS LCD function
 */
#define CONFIG_QOSA_LCD_FUNC

/**
 * UniRTOS IIC function
 */
#define CONFIG_QOSA_IIC_FUNC

/* UniRTOS ADC function */
#define CONFIG_QOSA_ADC_FUNC

/* UniRTOS PWM function */
#define CONFIG_QOSA_PWM_FUNC

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
 * UniRTOS Code Decoder function
 */
/* #undef CONFIG_QOSA_CODE_DECODER_FUNC */

/**
 * UniRTOS Code Decoder control
 */
/* #undef CONFIG_QOSA_UNIRTOS_CODE_DECODER_FUNC */

/**
 * QINGYA Code Decoder control
 */
/* #undef CONFIG_QOSA_QINGYA_CODE_DECODER_FUNC */

/**
 * UniRTOS Flash function
 */
/* #undef CONFIG_QOSA_FLASH_TEST_FUNC */

/**
 * UniRTOS lpm psm mode support
 */
/* #undef CONFIG_QOSA_LPM_PSM_SUPPORT_FUNC */

/**
 * UniRTOS lpm sleep log mode support
 */
/* #undef CONFIG_QOSA_LPM_SLEEP_LOG_FUNC */

/**
 * UniRTOS Code decoder function
 */
/* #undef CONFIG_QOSA_CODE_DECODER_SUPPORT */

/**
 * UniRTOS LCD function
 */
/* #undef CONFIG_QOSA_LCD_SUPPORT */

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
/* #undef CONFIG_QOSA_FOTA_SIGN_FUNC */

/**
 * UniRTOS secboot function
 */
/* #undef CONFIG_QOSA_SECBOOT_FUNC */

/**
 * UniRTOS secboot support
 */
/* #undef CONFIG_QOSA_SECBOOT_SUPPORT */

/**
 * UniRTOS DTR support
 */
/* #undef CONFIG_QOSA_DTR_FUNC */

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
/* #undef CONFIG_QOSA_USBHID_FUNC */
/**
 * UniRTOS autoapn function
 */
/* #undef CONFIG_QOSA_AUTOAPN_FUNC */

/**
 * UniRTOS MBEDTLS function
 */
/* #undef CONFIG_QOSA_MBEDTLS_SUPPORT */

/**
 * UniRTOS basestation function
 */
/* #undef CONFIG_QOSA_ABNORMAL_BASESTATION_FUNC */


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
/* #undef CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */

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
 * bk7258 platform function support
 */
#define CONFIG_QOSA_BK7258_PLATFORM_FUNC
/**
 * bk7238 platform function support
 */
/* #undef CONFIG_QOSA_BK7238_PLATFORM_FUNC */
/**
 * ecr6600 platform function support
 */
/* #undef CONFIG_QOSA_ECR6600_PLATFORM_FUNC */
/**
 * UniRTOS CPU Usage AT function
 */
/* #undef CONFIG_QAPP_UNIRTOS_CPU_USAGE_FUNC */

/* #undef CONFIG_QOSA_PROVIDE_ERRNO */

#endif /* _QOSA_SYSTEM_CONFIG_H_ */
