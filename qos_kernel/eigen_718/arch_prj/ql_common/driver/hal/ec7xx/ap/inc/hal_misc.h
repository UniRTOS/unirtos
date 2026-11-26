/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    hal_misc.h
 * Description:  EC618 hal for misc header file
 * History:      Rev1.0   2019-12-12
 *
 ****************************************************************************/

#ifndef _HAL_MISC_H
#define _HAL_MISC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
  \brief  HAL_Get_ChipID enum
 */
typedef enum _chip_id_sel
{
   CHIP_ID_ONLYID = 0,  /**< only read chip ID */
   CHIP_ID_REVID,       /**< only read revison ID */
   CHIP_ID_FULLID         /**< read both  chip ID & revison ID */
} chip_id_sel;

/**
  \brief  HAL_Get_ChipVER enum
 */
typedef enum _chip_version
{
   CHIP_VER_UNKNOWN = 0,
   CHIP_VER_R1,      /**< 718M2 ORG */
   CHIP_VER_R2,      /**< 718M2 ECO1 */
   CHIP_VER_R3,      /**< 718M2 ECO2 */
} chip_version;

/**
  \brief  HAL_Get_ChipVER enum
 */
typedef enum _efuse_bit_type
{
   DOWNLOAD_PIN_DIS = 0,/**< disable USB download triggered by boot pin press */
  DOWNLOAD_DIS,        /**< disable both UART and USB download functions */
   DEBUG_DIS,           /**< disable UART0 log */
   SECURE_BOOT_EN,      /**< enable secure boot */
   SECURE_DOWNLOAD_EN,  /**< enable agentboot verification */
   USB_VPID_FLAG,       /**< use the VID/PIP burned in eFUSE */
   ANTI_ROLLBACK_EN,    /**< enable BL anti rollback */
   TAG_DIS,             /**< disable jtag/swd */
} efuse_bit_type;

/**
  \brief Get chip id
  \param[in] sel       HAL_Get_ChipID enum
  \return              chip id value
 */
uint32_t HAL_Get_ChipID(chip_id_sel sel);

/**
  \brief Get chip version
  \return  chip_version enum
 */
uint32_t HAL_Get_ChipVER(void);

/**
  \fn        HAL_burn_aes_key(void)
  \brief     hardware AES key programming.
 */
void HAL_burn_aes_key(void);

/**
  \fn           HAL_set_fuse_bit(en_fuse_bit_type typ)
  \brief        burn efuse bit.
  \param[in]    en_fuse_bit_type type
  \returns      true if bit has burn, false the type cannot be burn or burn failed
 */
bool HAL_set_fuse_bit(efuse_bit_type type);

/**
  \fn           HAL_get_fuse_bit(en_fuse_bit_type typ)
  \brief        read related efuse bit.
  \param[in]    en_fuse_bit_type type
  \returns      0:the bit not set or no such type; 1:the bit has set
 */
uint32_t HAL_get_fuse_bit(efuse_bit_type type);

/**
  \fn        delay_us(uint32_t us)
  \brief     delay time in microseconds.
  \param[in] us number of us
  \note       Given the maximum cpu frequency is 204.8MHz and the limit in calculation,
 *            the maximum time can be delayed is 0xFFFFFFFF / 2048 = 2097151 us = 2097 m   
 */
void delay_us(uint32_t us);

/**
  \fn        apmuBootDbgGPIOSet(bool level)
  \brief     set gpio level in boot flow, for boot time dbg
  \param[in] level
  \note       should set register directly, do not use api
 */
void apmuBootDbgGPIOSet(bool level);
/**
  \fn        bool apmuGetLongSlpCfg(void)
  \brief     config the maximum sleep length.
             1. We suggest return false to sleep no more than 36hour
             2. But when usb is disabled and you do not care about time accuracy, 
             return true to set maximum sleep time to 1165 hour
  \note      false: sleep no more than 36.4 hour, true:  can sleep 1165 hour
 */
bool apmuGetLongSlpCfg(void);


/**
  \fn        halCrc32(void* input, int len)
  \brief     caculate the crc 32
  \param[in] input pointer to input buffer
             len buffer len
  \note
 */
uint32_t halCrc32(void* input, int len);


#ifdef __cplusplus
}
#endif

#endif /* _HAL_MISC_H */
