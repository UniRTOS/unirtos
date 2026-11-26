/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: uart_device.h
*
*  Description:
*
*  History: 2021/1/19 created by xuwang
*
*  Notes:
*
******************************************************************************/
#ifndef UART_DEVICE_H
#define UART_DEVICE_H

/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/
#include "Driver_USART.h"
#include "ccio_pub.h"

#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/
#if defined CHIP_EC618 || defined CHIP_EC618_Z0
#define UART_DEV_DTR_WKUP_ENABLE   0
#else
#ifdef CONFIG_QOSA_DTR_FUNC
#define UART_DEV_DTR_WKUP_ENABLE   1
#else /* CONFIG_QOSA_DTR_FUNC */
#define UART_DEV_DTR_WKUP_ENABLE   0
#endif /* CONFIG_QOSA_DTR_FUNC */
#endif


/*----------------------------------------------------------------------------*
 *                   DATA TYPE DEFINITION                                     *
 *----------------------------------------------------------------------------*/
typedef enum
{
    UART_DEV_RX_SLIDER = 0, /* to recv data based on current remaining size of the slider */
    UART_DEV_RX_BATCH       /* to recv data based on current length of incoming data */
}UartDevRxMethod_e;

/** \brief configuration struct for initializing UART hardware */
typedef struct
{
    ARM_POWER_STATE    powerMode;       /**< UART power mode, ARM_POWER_FULL or ARM_POWER_LOW */
    uint32_t           ctrlSetting;     /**< setting value passed to ARM Control API */
    uint32_t           baudRate;        /**< baudrate value */
}UartHwConf_t;

/**
 * configuration about a uart device.
 */
typedef struct
{
    UartHwConf_t       hwConf;
    ARM_DRIVER_USART  *drvHandler;      /**< uart driver handler */
    uint8_t            mainUsage;       /**< refer to 'CsioDevType_e' */
    uint8_t            speedType;       /**< refer to 'CcioSpeedType_e' */
    uint8_t            bmCreateFlag;    /**< refer to 'CcioTaskOperFlag_e', bitmap type */
    uint8_t            rxMethod;        /**< refer to 'UartDevRxMethod_e' */
    uint32_t           isDftAtPort :1;  /**< default uartIdx for AT or not */
    uint32_t           rbufFlags   :4;  /**< which rbuf will be used? refer to 'CcioRbufUsage_e' */
    uint32_t           custFlags   :4;  /**< flags for customers' private purpose */
    uint32_t           custExtras  :16; /**< extra info for customers' private purpose */
    uint32_t           rsvdBits    :7;
}UartDevConf_t;


/*----------------------------------------------------------------------------*
 *                    GLOBAL FUNCTIONS DECLEARATION                           *
 *----------------------------------------------------------------------------*/
/**
 * @brief uartDevCreate(uint32_t uartIdx, UartDevConf_t *uartConf)
 * @details create a uart device of 'uartIdx'
 *
 * @param uartIdx   The index of uart port
 * @param uartConf  The configuration about a uart device
 * @return NULL failure; !NULL the created uart device.
 */
CcioDevice_t* uartDevCreate(uint32_t uartIdx, UartDevConf_t *uartConf);

#ifdef UNIRTOS_COMMON_MODIFY
/**
 * @brief uartDevCreate_ex with callback support
 * @details create a uart device with create completion callback
 *
 * @param uartIdx   The index of uart port
 * @param uartConf  The configuration about a uart device
 * @param uart_create_callback  Callback function called when create completes
 * @param argv      User data passed to callback
 * @return NULL failure; !NULL the created uart device.
 */
CcioDevice_t* uartDevCreate_ex(uint32_t uartIdx, UartDevConf_t *uartConf, chdevDestroyCallback uart_create_callback, void *argv);
#endif /* UNIRTOS_COMMON_MODIFY */

/**
 * @brief uartDevDestroy(uint32_t uartIdx)
 * @details destroy/delete a uart device of 'uartIdx'
 *
 * @param uartIdx index of uart port
 * @return 0 succ; < 0 failure with errno.
 */
int32_t uartDevDestroy(uint32_t uartIdx);

#ifdef UNIRTOS_COMMON_MODIFY
int32_t uartDevDestroy_ex(uint32_t uartIdx, chdevDestroyCallback uart_destroy_callback, void *argv);
#endif /* UNIRTOS_COMMON_MODIFY */
/**
 * @brief uartDevTransform(uint32_t uartIdx, CsioDevType_e newType, uint32_t extras)
 * @details transform a uart device into a new type
 *
 * @param uartIdx   The index of uart port
 * @param newType   The new stype of the uart device
 * @param extras    The extra info for new device
 * @return NULL failure; !NULL the transformed uart device.
 */
CcioDevice_t* uartDevTransform(uint32_t uartIdx, CsioDevType_e newType, uint32_t extras);

#ifdef __cplusplus
}
#endif
#endif

