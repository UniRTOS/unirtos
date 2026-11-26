/*****************************************************************/ /**
* @file qosa_flash_config.h
* @brief
* @author larson.li@quectel.com
* @date 2023-06-01
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-06-01 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_FLASH_CONFIG_H__
#define __QOSA_FLASH_CONFIG_H__

#if 1
// spi0 
#define QOSA_EXT_FLASH_PORT_NUM  0
#define QOSA_EXT_FLASH_CLK_SET   6500000
#define QOSA_EXT_FLASH_MOSI_PIN  67
#define QOSA_EXT_FLASH_MOSI_FUNC 1
#define QOSA_EXT_FLASH_MISO_PIN  28
#define QOSA_EXT_FLASH_MISO_FUNC 1
#define QOSA_EXT_FLASH_SCLK_PIN  29
#define QOSA_EXT_FLASH_SCLK_FUNC 1
#define QOSA_EXT_FLASH_CS_PIN    66
#define QOSA_EXT_FLASH_CS_FUNC   0
#else
// spi1
#define QOSA_EXT_FLASH_PORT_NUM  1
#define QOSA_EXT_FLASH_CLK_SET   6500000
#define QOSA_EXT_FLASH_MOSI_PIN  63
#define QOSA_EXT_FLASH_MOSI_FUNC 1
#define QOSA_EXT_FLASH_MISO_PIN  62
#define QOSA_EXT_FLASH_MISO_FUNC 1
#define QOSA_EXT_FLASH_SCLK_PIN  49
#define QOSA_EXT_FLASH_SCLK_FUNC 1
#define QOSA_EXT_FLASH_CS_PIN    64
#define QOSA_EXT_FLASH_CS_FUNC   0 
#endif


/**
 * @struct qosa_ext_flash_cfg_t
 * @brief 外挂flash配置结构体
 */
typedef struct
{
    unsigned int port_num;            /*!< 使用的SPI端口号 */
    unsigned int clk_set;             /*!< SPI时钟频率 */
    unsigned int ext_flash_mosi_pin;  /*!< MOSI引脚 */
    unsigned int ext_flash_mosi_func; /*!< MOSI引脚功能 */
    unsigned int ext_flash_miso_pin;  /*!< MISO 引脚 */
    unsigned int ext_flash_miso_func; /*!< MISO 引脚功能 */
    unsigned int ext_flash_sclk_pin;  /*!< SCLK引脚 */
    unsigned int ext_flash_sclk_func; /*!< SCLK引脚功能 */
    unsigned int ext_flash_cs_pin;    /*!< CS引脚 */
    unsigned int ext_flash_cs_func;   /*!< CS引脚功能 */
    unsigned int ext_flash_fs_offset; /*!< FS镜像分区偏移 */
    unsigned int ext_flash_fs_size;   /*!< FS镜像分区大小 */
    unsigned int ext_flash_updater_offset; /*!< UPDATER镜像分区偏移 */
    unsigned int ext_flash_updater_size;   /*!< UPDATER镜像分区大小 */
} qosa_ext_flash_cfg_t;

/**
 * @brief 外挂flash配置全局变量
 */
extern qosa_ext_flash_cfg_t g_qosa_ext_flash_config;

/**
 * @brief 获取APP secure header镜像分区起始地址
 *
 * @return unsigned int
 *          - 分区起始地址xip地址
 */
unsigned int qosa_get_appsec_addr();

/**
 * @brief 获取AP镜像分区大小
 *
 * @return unsigned int
 *          - 分区大小
 */
unsigned int qosa_get_ap_size();

/**
 * @brief 获取AP镜像分区起始地址
 *
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_ap_addr();

/**
 * @brief 获取CP镜像分区大小
 *
 * @return unsigned int
 *          - 分区大小
 */
unsigned int qosa_get_cp_size();

/**
 * @brief 获取CP镜像分区起始地址
 *
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_cp_addr();

/**
 * @brief 获取UPDATER镜像分区起始地址
 *
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_updater_addr();

/**
 * @brief 获取UPDATER镜像分区大小
 *
 * @return unsigned int
 *          - 分区大小
 */
unsigned int qosa_get_updater_size();
/**
 * @brief 获取UPDATER INFO镜像分区起始地址
 *
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_updater_info_addr();

/**
 * @brief 获取当前UPDATER镜像目标地址
 *
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_updater_target_addr();

/**
 * @brief 获取当前UPDATER镜像目标偏移
 *
 * @return unsigned int
 *          - 存储设备内偏移地址
 */
unsigned int qosa_get_updater_target_offset();

/**
 * @brief 获取当前UPDATER镜像目标分区大小
 *
 * @return unsigned int
 *          - 分区大小
 */
unsigned int qosa_get_updater_target_size();

/**
 * @brief 擦除当前UPDATER镜像目标分区
 *
 * @param addr 存储设备内偏移地址
 * @param size 擦除大小
 *
 * @return int
 *          - 0表示成功，其他表示失败
 */
int qosa_updater_target_erase(unsigned int offset, unsigned int size);

/**
 * @brief 写入当前UPDATER镜像目标分区
 *
 * @param data 写入数据
 * @param addr 存储设备内偏移地址
 * @param size 写入大小
 *
 * @return int
 *          - 0表示成功，其他表示失败
 */
int qosa_updater_target_write(unsigned char *data, unsigned int offset, unsigned int size);

#ifdef QOSA_EXT_UPDATER_SUPPORT
/**
 * @brief 获取外挂UPDATER镜像分区起始地址
 *
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_ext_updater_addr();

/**
 * @brief 获取UPDATER镜像分区大小
 *
 * @return unsigned int
 *          - 分区大小
 */
unsigned int qosa_get_ext_updater_size();
/**
 * @brief 获取UPDATER INFO镜像分区起始地址
 *
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_ext_updater_info_addr();
/**
 * @brief 获取外挂UPDATER镜像运行地址
 *
 * @return unsigned int
 *          - 分区地址
 */
unsigned int qosa_get_ext_updater_exec_addr();
#endif /* QOSA_EXT_UPDATER_SUPPORT */

#ifdef UNIRTOS_OPEN_MODE_SUPPORT
/**
 * @brief 获取APP镜像分区起始地址
 *
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_app_addr();

/**
 * @brief 获取APP镜像分区ram起始地址
 *
 * @return unsigned int
 *          - ram分区起始地址
 */
unsigned int qosa_get_app_ram_addr();

/**
 * @brief 获取APP镜像分区大小
 *
 * @return unsigned int
 *          - 分区大小
 */
unsigned int qosa_get_app_size();
#endif /* UNIRTOS_OPEN_MODE_SUPPORT */
/**
 * @brief 获取FS镜像分区起始地址
 *
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_fs_addr();

/**
 * @brief 获取FS镜像分区大小
 *
 * @return unsigned int
 *          - 分区大小
 */
unsigned int qosa_get_fs_size();

/**
 * @brief 获取FOTA INFO镜像分区起始地址
 * 
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_fota_info_addr();

/**
 * @brief 获取FLASH ERASE RECORD镜像分区起始地址
 * 
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_flash_erase_record_addr();

/**
 * @brief 获取FLASH ERASE RECORD镜像分区大小
 * 
 * @return unsigned int
 *          - 分区大小
 */
unsigned int qosa_get_flash_erase_record_size();

/**
 * @brief 获取可以被擦写的FLASH分区个数
 *
 * @return unsigned int
 *          - 分区个数
 */
unsigned int qosa_get_flash_limit_partion_cnt();

/**
 * @brief 获取需要记录擦写次数的分区个数
 *
 * @return unsigned int
 *          - 分区个数
 */
unsigned int qosa_get_flash_limit_record_cnt();

/**
 * @brief 获取GNSS IMG分区起始地址
 *
 * @return qosa_uint32_t
 *       - 如果不存在则返回0
 *       - 存在则返回固件地址
 */
unsigned int qosa_get_gnss_img_addr(void);

/**
 * @brief 获取GNSS IMG 分区大小
 *
 * @return qosa_uint32_t
 *       - 如果不存在则返回0
 *       - 存在则返回分区大小
 */
unsigned int qosa_get_gnss_img_size(void);

/**
 * @brief 获取FLASH DUMP镜像起始地址
 * 
 * @return unsigned int
 *          - 分区起始地址
 */
unsigned int qosa_get_flash_dump_addr(void);

#endif /* __QOSA_FLASH_CONFIG_H__ */
