/*****************************************************************/ /**
* @file qosa_device.h
* @brief
* @author larson.li@quectel.com
* @date 2023-04-20
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-04-20 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"
#include "qosa_watermark.h"
#include "qosa_object.h"
#include "qosa_buffer_block.h"

/**
 * device flags defitions
 */
#define RT_DEVICE_FLAG_DEACTIVATE       0x000 /**< device is not not initialized */

#define RT_DEVICE_FLAG_RDONLY           0x001 /**< read only */
#define RT_DEVICE_FLAG_WRONLY           0x002 /**< write only */
#define RT_DEVICE_FLAG_RDWR             0x003 /**< read and write */

#define RT_DEVICE_FLAG_REMOVABLE        0x004 /**< removable device */
#define RT_DEVICE_FLAG_STANDALONE       0x008 /**< standalone device */
#define RT_DEVICE_FLAG_ACTIVATED        0x010 /**< device is activated */
#define RT_DEVICE_FLAG_SUSPENDED        0x020 /**< device is suspended */
#define RT_DEVICE_FLAG_STREAM           0x040 /**< stream mode */

#define RT_DEVICE_FLAG_INT_RX           0x100 /**< INT mode on Rx */
#define RT_DEVICE_FLAG_DMA_RX           0x200 /**< DMA mode on Rx */
#define RT_DEVICE_FLAG_INT_TX           0x400 /**< INT mode on Tx */
#define RT_DEVICE_FLAG_DMA_TX           0x800 /**< DMA mode on Tx */

#define RT_DEVICE_OFLAG_CLOSE           0x000 /**< device is closed */
#define RT_DEVICE_OFLAG_RDONLY          0x001 /**< read only access */
#define RT_DEVICE_OFLAG_WRONLY          0x002 /**< write only access */
#define RT_DEVICE_OFLAG_RDWR            0x003 /**< read and write */
#define RT_DEVICE_OFLAG_OPEN            0x008 /**< device is opened */
#define RT_DEVICE_OFLAG_MASK            0xf0f /**< mask of open flag */

/**
  * general device commands
  */
#define RT_DEVICE_CTRL_RESUME           0x01 /**< resume device */
#define RT_DEVICE_CTRL_SUSPEND          0x02 /**< suspend device */
#define RT_DEVICE_CTRL_CONFIG           0x03 /**< configure device */
#define RT_DEVICE_CTRL_CLOSE            0x04 /**< close device */
#define RT_DEVICE_CTRL_OPEN             0x07 /**< open device */

/* UniRTOS ADD by Larson.li*/
#define RT_DEVICE_CTRL_ESCAPE_FUNC      0x05 /**< config RAW_DATA recv +++ after process handler */
#define RT_DEVICE_CTRL_SET_TRANMIT_INFO 0x06 /**< config driver transmit info, watermark pointer and escape check info*/
#define RT_DEVICE_CTRL_DTR_FUNC         0x08 /**< config RAW_DATA recv DTR after process handler */
/* UniRTOS ADD end*/

#define RT_DEVICE_CTRL_SET_INT          0x10 /**< set interrupt */
#define RT_DEVICE_CTRL_CLR_INT          0x11 /**< clear interrupt */
#define RT_DEVICE_CTRL_GET_INT          0x12 /**< get interrupt status */

                                             /**
   * special device commands
   */
#define RT_DEVICE_CTRL_CHAR_STREAM      0x10 /**< stream mode on char device */
#define RT_DEVICE_CTRL_BLK_GETGEOME     0x10 /**< get geometry information   */
#define RT_DEVICE_CTRL_BLK_SYNC         0x11 /**< flush data to block device */
#define RT_DEVICE_CTRL_BLK_ERASE        0x12 /**< erase block on block device */
#define RT_DEVICE_CTRL_BLK_AUTOREFRESH  0x13 /**< block device : enter/exit auto refresh mode */
#define RT_DEVICE_CTRL_NETIF_GETMAC     0x10 /**< get mac address */
#define RT_DEVICE_CTRL_MTD_FORMAT       0x10 /**< format a MTD device */
#define RT_DEVICE_CTRL_RTC_GET_TIME     0x10 /**< get time */
#define RT_DEVICE_CTRL_RTC_SET_TIME     0x11 /**< set time */
#define RT_DEVICE_CTRL_RTC_GET_ALARM    0x12 /**< get alarm */
#define RT_DEVICE_CTRL_RTC_SET_ALARM    0x13 /**< set alarm */

/* UniRTOS ADD */
#define RT_DEVICE_CTRL_DCD_GET          0x14 /**< get DCD */
#define RT_DEVICE_CTRL_DCD_SET          0x15 /**< set DCD */
#define RT_DEVICE_CTRL_RI_GET           0x16 /**< get RI */
#define RT_DEVICE_CTRL_RI_SET           0x17 /**< set RI */
#define RT_DEVICE_CTRL_RTS_GET          0x18 /**< get RTS */
#define RT_DEVICE_CTRL_RTS_SET          0x19 /**< set RTS */
#define RT_DEVICE_CTRL_CTS_GET          0x20 /**< get CTS */
#define RT_DEVICE_CTRL_DTR_GET          0x21 /**< get DTR */
#define RT_DEVICE_CTRL_DCB_SET          0x22 /**< set DCB */

/* UniRTOS ADD end*/

/** osa error code base */
#define QOSA_ERRCODE_RT_DEV_BASE        (QOSA_COMPONENT_RT_DEVICE << 16)

/*! RTT error code definitions */
typedef enum
{
    QOSA_EOK = 0,                                             /*!< success operation */
    QOSA_RT_DEV_ERROR_GENERIC = 1 | QOSA_ERRCODE_RT_DEV_BASE, /*!< Message queue creation failed */
    QOSA_RT_DEV_ETIMEOUT,                                     /**< Timed out */
    QOSA_RT_DEV_EFULL,                                        /**< The resource is full */
    QOSA_RT_DEV_EEMPTY,                                       /**< The resource is empty */
    QOSA_RT_DEV_ENOMEM,                                       /**< No memory */
    QOSA_RT_DEV_ENOSYS,                                       /**< No system */
    QOSA_RT_DEV_EBUSY,                                        /**< Busy */
    QOSA_RT_DEV_EIO,                                          /**< IO error */
    QOSA_RT_DEV_EINTR,                                        /**< Interrupted system call */
    QOSA_RT_DEV_EINVAL,                                       /**< Invalid argument */
} qosa_rt_dev_error_e;

typedef struct rt_device* rt_device_t;

/**
 * device (I/O) class type
 */
enum rt_device_class_type
{
    RT_Device_Class_Char = 0,      /**< character device */
    RT_Device_Class_Block,         /**< block device */
    RT_Device_Class_NetIf,         /**< net interface */
    RT_Device_Class_MTD,           /**< memory device */
    RT_Device_Class_CAN,           /**< CAN device */
    RT_Device_Class_RTC,           /**< RTC device */
    RT_Device_Class_Sound,         /**< Sound device */
    RT_Device_Class_Graphic,       /**< Graphic device */
    RT_Device_Class_I2CBUS,        /**< I2C bus device */
    RT_Device_Class_USBDevice,     /**< USB slave device */
    RT_Device_Class_USBHost,       /**< USB host bus */
    RT_Device_Class_SPIBUS,        /**< SPI bus device */
    RT_Device_Class_SPIDevice,     /**< SPI device */
    RT_Device_Class_SDIO,          /**< SDIO bus device */
    RT_Device_Class_PM,            /**< PM pseudo device */
    RT_Device_Class_Pipe,          /**< Pipe device */
    RT_Device_Class_Portal,        /**< Portal device */
    RT_Device_Class_Timer,         /**< Timer device */
    RT_Device_Class_Miscellaneous, /**< Miscellaneous device */
    RT_Device_Class_Sensor,        /**< Sensor device */
    RT_Device_Class_Touch,         /**< Touch device */
    RT_Device_Class_PHY,           /**< PHY device */
    RT_Device_Class_Unknown        /**< unknown device */
};
/**
 * operations set for device object
 */
struct rt_device_ops
{
    /* common device interface */
    int (*init)(rt_device_t dev);
    int (*open)(rt_device_t dev, qosa_uint16_t oflag);
    int (*close)(rt_device_t dev);
    qosa_size_t (*read)(rt_device_t dev, qosa_size_t pos, void* buffer, qosa_size_t size);
    qosa_size_t (*write)(rt_device_t dev, qosa_size_t pos, const void* buffer, qosa_size_t size);
    int (*control)(rt_device_t dev, int cmd, void* args);
    void (*transmit)(rt_device_t dev, qosa_buffer_block_t* tx_ptr);
};
/**
 * Device structure
 */
struct rt_device
{
    struct rt_object parent;             /**< inherit from rt_object */

    enum rt_device_class_type type;      /**< device type */
    qosa_uint16_t             flag;      /**< device flag */
    qosa_uint16_t             open_flag; /**< device open flag */

    qosa_uint8_t  ref_count;             /**< reference count */
    qosa_uint32_t port_id;               /**< 0 - 255 */
    /* device call back */
    int (*rx_indicate)(rt_device_t dev, qosa_size_t size);
    int (*tx_complete)(rt_device_t dev, void* buffer);

    const struct rt_device_ops* ops;

#if defined(RT_USING_POSIX)
    const struct dfs_file_ops* fops;
    struct rt_wqueue           wait_queue;
#endif

    void* user_data; /**< device private data */
};

/**
 * This function registers a device driver with specified name.
 *
 * @param dev the pointer of device driver structure
 * @param name the device driver's name
 * @param flags the capabilities flag of device
 *
 * @return the error code, QOSA_OK on initialization successfully.
 */
int rt_device_register(rt_device_t dev, const char* name, qosa_uint16_t flags);

/**
 * This function removes a previously registered device driver
 *
 * @param dev the pointer of device driver structure
 *
 * @return the error code, QOSA_OK on successfully.
 */
int rt_device_unregister(rt_device_t dev);

/**
 * This function finds a device driver by specified name.
 *
 * @param name the device driver's name
 *
 * @return the registered device driver on successful, or QOSA_NULL on failure.
 */
rt_device_t rt_device_find(const char* name);

/**
 * This function will initialize the specified device
 *
 * @param dev the pointer of device driver structure
 *
 * @return the result
 */
int rt_device_init(rt_device_t dev);

/**
 * This function will open a device
 *
 * @param dev the pointer of device driver structure
 * @param oflag the flags for device open
 *
 * @return the result
 */
int rt_device_open(rt_device_t dev, qosa_uint16_t oflag);

/**
 * This function will close a device
 *
 * @param dev the pointer of device driver structure
 *
 * @return the result
 */
int rt_device_close(rt_device_t dev);

/**
 * This function will read some data from a device.
 *
 * @param dev the pointer of device driver structure
 * @param pos the position of reading
 * @param buffer the data buffer to save read data
 * @param size the size of buffer
 *
 * @return the actually read size on successful, otherwise negative returned.
 *
 * @note since 0.4.0, the unit of size/pos is a block for block device.
 */
qosa_size_t rt_device_read(rt_device_t dev, int pos, void* buffer, qosa_size_t size);

/**
 * This function will write some data to a device.
 *
 * @param dev the pointer of device driver structure
 * @param pos the position of written
 * @param buffer the data buffer to be written to device
 * @param size the size of buffer
 *
 * @return the actually written size on successful, otherwise negative returned.
 *
 * @note since 0.4.0, the unit of size/pos is a block for block device.
 */
qosa_size_t rt_device_write(rt_device_t dev, int pos, const void* buffer, qosa_size_t size);

/**
 * This function will transmit a packet or if transmitter is busy, schedule
 * the transmission of a packet.
 *
 * @param dev the pointer of device driver structure
 * @param tx_ptr memory item type ptr
 *
 * @return None
 */
void rt_device_transmit(rt_device_t dev, qosa_buffer_block_t* tx_ptr);

/**
 * This function will perform a variety of control functions on devices.
 *
 * @param dev the pointer of device driver structure
 * @param cmd the command sent to device
 * @param arg the argument of command
 *
 * @return the result
 */
int rt_device_control(rt_device_t dev, int cmd, void* arg);

rt_device_t rt_device_find_id(qosa_uint32_t decive_id);

#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __DEVICE_H__ */
