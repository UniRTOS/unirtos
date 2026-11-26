/*****************************************************************/ /**
* @file unirtos_flash_partion_718pm_open.h
* @brief
* @author sharon.li@quectel.com
* @date 2025-12-27
*
* @copyright Copyright (c) 2025 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2025-12-27 <td>1.0 <td>sharon.Li <td> Init
* </table>
**********************************************************************/

#ifndef __UNIRTOS_MEM_PARTION_718HM_OPEN_H__
#define __UNIRTOS_MEM_PARTION_718HM_OPEN_H__
#include "QosaPrjName.h"
#include "quec_mem_map_718hm.h"

/* 4MB flash + 4MB psram*/

/**********************************************************************************
flash layout, toatl 4MB
flash raw address: 0x00000000---0x00400000
flash xip address(from both ap/cp view): 0x00800000---0x00c00000

注意:
1、当前CSDK的默认分区配置已占满整个内置FLASH空间，且客户自定义分区（cust partition）的初始大小为0KB。
2、若需调整分区，目前仅支持在内核分区（ap partition）、应用分区（app partition）、轻量文件系统分区（lfs partition）和客户自定义分区（cust partition）之间进行容量转换。
这意味着这四个分区的总大小保持不变，增大其中一个分区时，必须相应减少其他分区的容量。
例如，若要扩大自定义分区（即增大 UNIRTOS_CUST_FLASH_SIZE），则需相应调减另外两个分区的大小。
3、可以使用预处理器错误指令 #error  来确定拿到的CSDK走哪个宏控制后，再在对应的宏下面修改。


        |---------------------------------|
		|      cp img                     |
        |---------------------------------|
		|      ap img                     |(UNIR_AP_IMG_SIZE)
		|---------------------------------|
		|      app img partition          |(UNIR_APP_OPEN_CPU_SIZE)
		|---------------------------------|
		|      cust partition(0KB)        |(UNIRTOS_CUST_FLASH_SIZE)
		|---------------------------------|
		|      lfs partition              |(UNIR_LFS_SIZE)
		|---------------------------------|
		|      updater img                |(UNIR_UPDATER_SIZE)
		|---------------------------------|

**********************************************************************************/

/**********************************************************************************
 * First 分配各个FLASH分区大小
 */
#if defined(QL_AP_LIB_IMS)
#define UNIR_AP_IMG_SIZE        (0x577000)
#define UNIR_APP_OPEN_CPU_SIZE  (0xC8000)
#define UNIRTOS_CUST_FLASH_SIZE (0x0)
#define UNIR_LFS_SIZE           (0x90000)
#define UNIR_UPDATER_SIZE       (0X24000)
#define UNIR_HIB_BACKUP_SIZE    (0x18000)
#elif defined(QL_AP_LIB_OC)
#define UNIR_AP_IMG_SIZE        (0x5B3000)
#define UNIR_APP_OPEN_CPU_SIZE  (0xC8000)
#define UNIRTOS_CUST_FLASH_SIZE (0x0)
#define UNIR_LFS_SIZE           (0x90000)
#define UNIR_UPDATER_SIZE       (0X24000)
#define UNIR_HIB_BACKUP_SIZE    (0x18000)
#else
#error "please define AP type in GccBuild_ec7xx.bat-->Makefile_Path.vars-->quec_partion_7xx.h"
#endif
// 以下部分为不可调整分区大小的FLASH分区 START
#define UNIR_AP_IMG_UNZIP_SIZE       (0x61E000)
#define UNIR_FOTA_INFO_SIZE          (0X400)
#define UNIR_FLASH_ERASE_RECORD_SIZE (0xC00)
#define UNIR_FOTA_BACKUP_SIZE        (0XB000)
#define UNIR_FLASH_DUMP_SIZE         (0X4000)
// 不可调整分区大小的FLASH分区 END
/************************************************************************************
 * Second：
 * 1、分配APP的RAM分区大小,针对此平台而言,APP的Ram分区在Psram的最后,PSRAM的结束地址为0x0c400000
 * 2、此SIZE要求大于app的Ram Code+RW+ZI的和;
 * 3、由于采用XIP,Text段在Flash分区里;
 * 4、增大UNIRTOS_OCPU_APP_RAM_SIZE，系统会自动配置减少总Heap分区的大小;反之增加
 * 5、默认APP的RAM的起始地址为(UNIRTOS_PSRAM_END_ADDR - UNIRTOS_OCPU_APP_RAM_SIZE)
 */
#define UNIRTOS_PSRAM_END_ADDR       0x0c800000
#define UNIRTOS_OCPU_APP_RAM_SIZE    0x100000
// 双app分区 ram大小分配举例，客户如不使用双app跳转功能，可忽略以下宏定义 START
#define UNIRTOS_OCPU_APP1_RAM_ADDR   (UNIRTOS_PSRAM_END_ADDR - UNIRTOS_OCPU_APP_RAM_SIZE) /* 默认为0x0c300000*/
#define UNIRTOS_OCPU_APP1_RAM_SIZE   0x080000

#define UNIRTOS_OCPU_APP2_RAM_ADDR   (UNIRTOS_OCPU_APP1_RAM_ADDR + UNIRTOS_OCPU_APP1_RAM_SIZE)
#define UNIRTOS_OCPU_APP2_RAM_SIZE   (UNIRTOS_OCPU_APP_RAM_SIZE - UNIRTOS_OCPU_APP1_RAM_SIZE)

// app分区大小检查，用于双app 跳转时app1 与 app2的大小检查
#if ((UNIRTOS_OCPU_APP1_RAM_SIZE + UNIRTOS_OCPU_APP2_RAM_SIZE) > (UNIRTOS_OCPU_APP_RAM_SIZE))
#error "QUEC_FLASH_MEM_PLAT_INFO Partion Error!!!."
#endif
// 双app分区 ram大小分配举例，客户如不使用双app跳转功能，可忽略的宏定义 END

/**********************************************************************************
 * Second： 由于AP分区起始是固定的,再根据上面确定的分区大小，确定各个分区的FLASH起始地址
 */
#define UNIR_AP_IMG_ADDR             (UNIR_AP_PARTION_START)
#define UNIR_APP_OPEN_CPU_ADDR       (UNIR_AP_IMG_ADDR + UNIR_AP_IMG_SIZE)
#define UNIRTOS_CUST_FLASH_ADDR      (UNIR_APP_OPEN_CPU_ADDR + UNIR_APP_OPEN_CPU_SIZE)
#define UNIR_LFS_ADDR                (UNIRTOS_CUST_FLASH_ADDR + UNIRTOS_CUST_FLASH_SIZE)
#define UNIR_UPDATER_ADDR            (UNIR_LFS_ADDR + UNIR_LFS_SIZE)
#define UNIR_HIB_BACKUP_ADDR         (UNIR_UPDATER_ADDR + UNIR_UPDATER_SIZE)
#define UNIR_FOTA_INFO_ADDR          (UNIR_HIB_BACKUP_ADDR + UNIR_HIB_BACKUP_SIZE)
#define UNIR_FLASH_ERASE_RECORD_ADDR (UNIR_FOTA_INFO_ADDR + UNIR_FOTA_INFO_SIZE)
#define UNIR_FOTA_BACKUP_ADDR        (UNIR_FLASH_ERASE_RECORD_ADDR + UNIR_FLASH_ERASE_RECORD_SIZE)
#define UNIR_FLASH_DUMP_ADDR         (UNIR_FOTA_BACKUP_ADDR + UNIR_FOTA_BACKUP_SIZE - UNIR_FLASH_DUMP_SIZE)

#endif /* __UNIRTOS_FLASH_PARTION_718HM_OPEN_H__ */
