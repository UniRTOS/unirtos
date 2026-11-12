
/**  @file
  quec_mem_map_718hm.h

  @brief
  TODO

*/

/*================================================================
  Copyright (c) 2025 Quectel Wireless Solution, Co., Ltd.  All Rights Reserved.
  Quectel Wireless Solution Proprietary and Confidential.
=================================================================*/
/*=================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN              WHO         WHAT, WHERE, WHY
------------     -------     -------------------------------------------------------------------------------

=================================================================*/
#ifndef __QUEC_MEM_MAP_718HM_H__
#define __QUEC_MEM_MAP_718HM_H__

#include "QosaPrjName.h"
#if defined QL_UPDATER_BUILD || defined QL_BOOTLOADER_BUILD
#include "bl_link_mem_map.h"
#endif /* QL_UPDATER_BUILD */
/***********************************************************************************************************
 * 1、由于AP分区前面的分区包含CP分区,且AP分区前面的分区不可调整大小,因此,要根据CP分区的大小，来确定AP分区的起始地址;
 * 2、不同平台的CP分区大小不同,同平台不同类型的CP分区也大小不同;
*/
#if defined QL_CP_OC_BIN_MODE
#define UNIR_AP_PARTION_START (0x91000)
#elif defined QL_CP_AUDIO_BIN_MODE
#define UNIR_AP_PARTION_START (0xcd000)
#else
#error "please define cpbin type in GccBuild_ec7xx.bat-->Makefile_Path.vars-->quec_partion_7xx.h"
#endif

/***********************************************************************************************************
 * 客户在以下两个文件里自定义分区
*/
#ifdef UNIRTOS_OPEN_MODE_SUPPORT
#include "unirtos_mem_partion_718hm_open.h"
#else  /* UNIRTOS_OPEN_MODE_SUPPORT */
#include "unirtos_mem_partion_718hm_std.h"
#endif /* UNIRTOS_OPEN_MODE_SUPPORT */

#ifdef UNIR_NORFLASH_ERASE_OPTIMIZE
#define UNIR_FLASH_EARSE_RECORD_ADDR              (UNIR_FLASH_ERASE_RECORD_ADDR)
#define UNIR_FLASH_EARSE_RECORD_SECTOR_BEGIN_ADDR (UNIR_FOTA_INFO_ADDR)  // 4K split fota flag(1K) and FLASH erase record (3k)
#define UNIR_FLASH_EARSE_RECORD_SIZE              0xBF4  // erase 3060byte not 3k ,It must be an integer multiple quec_flashErase_record_s(20byte)
#endif                                                   /* UNIR_NORFLASH_ERASE_OPTIMIZE */

#define AP_FLASH_LOAD_ADDR       (AP_FLASH_XIP_ADDR + UNIR_AP_IMG_ADDR)
#define AP_FLASH_LOAD_SIZE       (UNIR_AP_IMG_SIZE)
#define AP_FLASH_LOAD_UNZIP_SIZE (UNIR_AP_IMG_UNZIP_SIZE)

//flash_dump

#define FLASH_FS_REGION_START    (UNIR_LFS_ADDR)
#define FLASH_FS_REGION_END      (UNIR_LFS_ADDR + UNIR_LFS_SIZE)
#define FLASH_FS_REGION_SIZE     (UNIR_LFS_SIZE)  // 48k

#define UPDATER_FLASH_LOAD_ADDR  (UNIR_UPDATER_ADDR + AP_FLASH_XIP_ADDR)
#define UPDATER_FLASH_LOAD_SIZE  (UNIR_UPDATER_SIZE)  //144k
#define UPDATER_FLASH_LOAD_END   (UPDATER_FLASH_LOAD_ADDR + UNIR_UPDATER_SIZE)

#if defined (QOSA_EXT_UPDATER_SUPPORT)
#if defined (QL_BOOTLOADER_BUILD) || defined (QL_UPDATER_BUILD)
#define UPDATER_PSRAM_EXEC_ADDR  (PSRAM_FOTA_MUXMEM_END_ADDR)
#else
#define UPDATER_PSRAM_EXEC_ADDR  (0x0)
#endif /* QL_BOOTLOADER_BUILD || QL_UPDATER_BUILD */
#define UPDATER_EXT_FLASH_LOAD_ADDR (EF_EXT_UPDATER_IMG_BURNADDR_LNA)
#define UPDATER_EXT_FLASH_LOAD_SIZE (EF_EXT_UPDATER_IMG_LIMIT_SIZE)
#define UPDATER_EXT_FLASH_LOAD_END   (UPDATER_EXT_FLASH_LOAD_ADDR + UPDATER_EXT_FLASH_LOAD_SIZE)
#endif /* QOSA_EXT_UPDATER_SUPPORT */

#if defined (QOSA_EXT_UPDATER_SUPPORT)
#define UPDATER_CODE_LOAD_ADDR  (UPDATER_PSRAM_EXEC_ADDR)
#define UPDATER_CODE_LOAD_SIZE  (UPDATER_EXT_FLASH_LOAD_SIZE)
#else
#define UPDATER_CODE_LOAD_ADDR  (UPDATER_FLASH_LOAD_ADDR)
#define UPDATER_CODE_LOAD_SIZE  (UPDATER_FLASH_LOAD_SIZE)
#endif /* QOSA_EXT_UPDATER_SUPPORT  */

#if (UNIR_HIB_BACKUP_SIZE == 0x0)
//hib bakcup addr and size
#define FLASH_HIB_BACKUP_EXIST       (0)
#define FLASH_MEM_BACKUP_ADDR        0x0
#define FLASH_MEM_BACKUP_NONXIP_ADDR 0x0
#define FLASH_MEM_BLOCK_SIZE         0x0
#define FLASH_MEM_BLOCK_CNT          0x0
#define FLASH_MEM_BACKUP_SIZE        0x0
#else
//hib bakcup addr and size
#define FLASH_HIB_BACKUP_EXIST       (1)
#define FLASH_MEM_BACKUP_ADDR        (AP_FLASH_XIP_ADDR + UNIR_HIB_BACKUP_ADDR)
#define FLASH_MEM_BACKUP_NONXIP_ADDR (UNIR_HIB_BACKUP_ADDR)
#define FLASH_MEM_BACKUP_SIZE        (UNIR_HIB_BACKUP_SIZE)  //96KB
#define FLASH_MEM_BLOCK_SIZE         (0x6000)
#define FLASH_MEM_BLOCK_CNT          (0x4)
#endif

#ifdef FEATURE_FOTAPAR_ENABLE
//fota addr and size
#define FLASH_FOTA_REGION_START (UNIR_FOTA_BACKUP_ADDR)
#define FLASH_FOTA_REGION_LEN   (UNIR_FOTA_BACKUP_SIZE)
#define FLASH_FOTA_REGION_END   (UNIR_FOTA_BACKUP_ADDR + UNIR_FOTA_BACKUP_SIZE)
#endif

#ifdef UNIRTOS_OPEN_MODE_SUPPORT
#define UNIRTOS_OCPU_APP_XIP_ADDR   (AP_FLASH_XIP_ADDR + UNIR_APP_OPEN_CPU_ADDR)
#define UNIRTOS_OCPU_APP_FLASH_LEN  (UNIR_APP_OPEN_CPU_SIZE)
#define UNIRTOS_OCPU_APP_END_ADDR   (UNIRTOS_OCPU_APP_XIP_ADDR + UNIRTOS_OCPU_APP_FLASH_LEN)

#define UNIRTOS_OCPU_CUST_XIP_ADDR  (UNIRTOS_OCPU_APP_END_ADDR)
#define UNIRTOS_OCPU_CUST_FLASH_LEN (UNIRTOS_CUST_FLASH_SIZE)
#define UNIRTOS_OCPU_CUST_END_ADDR  (UNIRTOS_OCPU_CUST_XIP_ADDR + UNIRTOS_OCPU_CUST_FLASH_LEN)
#endif /* UNIRTOS_OPEN_MODE_SUPPORT */

#if defined(CONFIG_QOSA_SECURITY_BOOT_KERNEL) || defined(CONFIG_QOSA_SECURITY_BOOT_BL)
#ifdef UNIRTOS_OPEN_MODE_SUPPORT
// APP Secure header
#define UNIRTOS_APP_SEC_HEAD_ADDR (APP_SECUREBOOT_LOAD_ADDR + AP_FLASH_XIP_ADDR)
#define UNIRTOS_APP_SEC_HEAD_LEN  (APP_SECUREBOOT_LOAD_SIZE)  //4K
#define UNIRTOS_APP_SEC_END_ADDR  (UNIRTOS_APP_SEC_HEAD_ADDR + UNIRTOS_APP_SEC_HEAD_LEN)
#endif /* UNIRTOS_OPEN_MODE_SUPPORT*/
#endif /* defined (CONFIG_QOSA_SECURITY_BOOT_KERNEL) || defined (CONFIG_QOSA_SECURITY_BOOT_BL)*/
/* ======================== MEM_MAP DEF CHECK TABLE ========================*/

// chaim 2024.10.1 flash partion check
/*
will check the macro between UNIR_PARTION_START---UNIR_PARTION_END.
the prartion need rank sequence
if add one partion that not in origin partion,we need add it between  UNIR_PARTION_START---UNIR_PARTION_END.
otherwise not need modify these
*/
#ifdef UNIRTOS_OPEN_MODE_SUPPORT
#define UNIR_PARTION_START                        (0x0)

#define QOS_PARTION_CHECK_AP_FLASH_ADDR           AP_FLASH_LOAD_ADDR
#define QOS_PARTION_CHECK_AP_FLASH_SIZE           AP_FLASH_LOAD_SIZE

#define QOS_PARTION_CHECK_APP_OPEN_CPU_ADDR       (AP_FLASH_XIP_ADDR + UNIR_APP_OPEN_CPU_ADDR)
#define QOS_PARTION_CHECK_APP_OPEN_CPU_SIZE       UNIR_APP_OPEN_CPU_SIZE

#define QOS_PARTION_CHECK_CUST_OPEN_CPU_ADDR      (AP_FLASH_XIP_ADDR + UNIRTOS_CUST_FLASH_ADDR)
#define QOS_PARTION_CHECK_CUST_OPEN_CPU_SIZE      UNIRTOS_OCPU_CUST_FLASH_LEN

#define QOS_PARTION_CHECK_FS_ADDR                 (AP_FLASH_XIP_ADDR + FLASH_FS_REGION_START)
#define QOS_PARTION_CHECK_FS_SIZE                 FLASH_FS_REGION_SIZE

#define QOS_PARTION_CHECK_UPDATER_ADDR            (UPDATER_FLASH_LOAD_ADDR)
#define QOS_PARTION_CHECK_UPDATER_SIZE            (UPDATER_FLASH_LOAD_SIZE)

//HIB
#define QOS_PARTION_CHECK_HIB_BACKUP_ADDR         FLASH_MEM_BACKUP_ADDR
#define QOS_PARTION_CHECK_HIB_BACKUP_SIZE         FLASH_MEM_BACKUP_SIZE

#define QOS_PARTION_CHECK_FOTA_INFO_ADDR          (AP_FLASH_XIP_ADDR + UNIR_FOTA_INFO_ADDR)
#define QOS_PARTION_CHECK_FOTA_INFO_SIZE          (UNIR_FOTA_INFO_SIZE)

#define QOS_PARTION_CHECK_FLASH_ERASE_RECORD_ADDR (AP_FLASH_XIP_ADDR + UNIR_FLASH_ERASE_RECORD_ADDR)
#define QOS_PARTION_CHECK_FLASH_ERASE_RECORD_SIZE (UNIR_FLASH_ERASE_RECORD_SIZE)

#define QOS_PARTION_CHECK_FOTA_BACKUP_ADDR        (AP_FLASH_XIP_ADDR + FLASH_FOTA_REGION_START)
#define QOS_PARTION_CHECK_FOTA_BACKUP_SIZE        FLASH_FOTA_REGION_LEN

#define UNIR_PARTION_END                          (0x1)

//FLASH_DUMP位于FOTA_BACKUP内部最后16K，不参与连续分区检查
#define QOS_PARTION_CHECK_FLASH_DUMP_ADDR         (QOS_PARTION_CHECK_FOTA_BACKUP_ADDR + QOS_PARTION_CHECK_FOTA_BACKUP_SIZE - QOS_PARTION_CHECK_FLASH_DUMP_SIZE)
#define QOS_PARTION_CHECK_FLASH_DUMP_SIZE         (UNIR_FLASH_DUMP_SIZE)

#else

#define UNIR_PARTION_START                        (0x0)

#define QOS_PARTION_CHECK_AP_FLASH_ADDR           AP_FLASH_LOAD_ADDR
#define QOS_PARTION_CHECK_AP_FLASH_SIZE           AP_FLASH_LOAD_SIZE

#define QOS_PARTION_CHECK_FS_ADDR                 (AP_FLASH_XIP_ADDR + FLASH_FS_REGION_START)
#define QOS_PARTION_CHECK_FS_SIZE                 FLASH_FS_REGION_SIZE

#define QOS_PARTION_CHECK_UPDATER_ADDR            (UPDATER_FLASH_LOAD_ADDR)
#define QOS_PARTION_CHECK_UPDATER_SIZE            (UPDATER_FLASH_LOAD_SIZE)

#define QOS_PARTION_CHECK_FOTA_INFO_ADDR          (AP_FLASH_XIP_ADDR + UNIR_FOTA_INFO_ADDR)
#define QOS_PARTION_CHECK_FOTA_INFO_SIZE          (UNIR_FOTA_INFO_SIZE)

#define QOS_PARTION_CHECK_FLASH_ERASE_RECORD_ADDR (AP_FLASH_XIP_ADDR + UNIR_FLASH_ERASE_RECORD_ADDR)
#define QOS_PARTION_CHECK_FLASH_ERASE_RECORD_SIZE (UNIR_FLASH_ERASE_RECORD_SIZE)

//HIB
#define QOS_PARTION_CHECK_HIB_BACKUP_ADDR         FLASH_MEM_BACKUP_ADDR
#define QOS_PARTION_CHECK_HIB_BACKUP_SIZE         FLASH_MEM_BACKUP_SIZE

#define QOS_PARTION_CHECK_FOTA_BACKUP_ADDR        (AP_FLASH_XIP_ADDR + FLASH_FOTA_REGION_START)
#define QOS_PARTION_CHECK_FOTA_BACKUP_SIZE        FLASH_FOTA_REGION_LEN

#define UNIR_PARTION_END                          (0x1)

//FLASH_DUMP位于FOTA_BACKUP内部最后16K，不参与连续分区检查
#define QOS_PARTION_CHECK_FLASH_DUMP_ADDR         (QOS_PARTION_CHECK_FOTA_BACKUP_ADDR + QOS_PARTION_CHECK_FOTA_BACKUP_SIZE - QOS_PARTION_CHECK_FLASH_DUMP_SIZE)
#define QOS_PARTION_CHECK_FLASH_DUMP_SIZE         (UNIR_FLASH_DUMP_SIZE)

#endif

/* ======================== MEM_DEF DEF TABLE ========================*/

#define AP_PKGIMG_LNA        (AP_FLASH_LOAD_ADDR)
#define AP_PKGIMG_LIMIT_SIZE (AP_FLASH_LOAD_SIZE)

#ifdef UNIRTOS_OPEN_MODE_SUPPORT
//application bin
#define PKGFLX_APP_PKGIMG_LNA         (UNIRTOS_OCPU_APP_XIP_ADDR)
#define PKGFLX_APP_PKGIMG_LIMIT_SIZE  (UNIRTOS_OCPU_APP_FLASH_LEN)

#define PKGFLX_CUST_PKGIMG_LNA        (UNIRTOS_OCPU_CUST_XIP_ADDR)
#define PKGFLX_CUST_PKGIMG_LIMIT_SIZE (UNIRTOS_OCPU_CUST_FLASH_LEN)
#endif

//updater bin
#define PKGFLX_UPDATER_PKGIMG_LNA        (UPDATER_FLASH_LOAD_ADDR)
#define PKGFLX_UPDATER_PKGIMG_LIMIT_SIZE (UPDATER_FLASH_LOAD_SIZE)

#ifdef CONFIG_QOSA_SECURITY_BOOT_KERNEL
// Updater Secure header
#define PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LNA        (UPDATER_SECUREBOOT_LOAD_ADDR + AP_FLASH_XIP_ADDR)
#define PKGFLX_UPDATER_SEC_HEAD_PKGIMG_LIMIT_SIZE (UPDATER_SECUREBOOT_LOAD_SIZE)  //4K
#ifdef UNIRTOS_OPEN_MODE_SUPPORT
// APP Secure header
#define PKGFLX_APP_SEC_HEAD_PKGIMG_LNA        (APP_SECUREBOOT_LOAD_ADDR + AP_FLASH_XIP_ADDR)
#define PKGFLX_APP_SEC_HEAD_PKGIMG_LIMIT_SIZE (APP_SECUREBOOT_LOAD_SIZE)  //4K
#endif /* UNIRTOS_OPEN_MODE_SUPPORT*/
#endif /* CONFIG_QOSA_SECURITY_BOOT_KERNEL*/
// External flash start address
#define EF_EXT_FLASH_BASE_ADDR  (0x80000000)
#define EFID01_EXT_FLASH_BASE_ADDR  (0x80000000)

// EFS start address, currently set to external flash start address, can be modified
#define EF_EFS_IMG_BURNADDR_LNA (0x80000000)
#define EFID01_EFS_IMG_BURNADDR_LNA (0x80000000)
// EFS image size, needs to be less than or equal to external Flash size
#define EF_EFS_IMG_LIMIT_SIZE   (0x200000)
#define EFID01_EFS_IMG_LIMIT_SIZE   (0x200000)

// EXT UPDATER start address, currently set to external flash start address, can be modified
#define EF_EXT_UPDATER_IMG_BURNADDR_LNA (EF_EXT_FLASH_BASE_ADDR + EF_EFS_IMG_LIMIT_SIZE)
#define EFID01_EXT_UPDATER_IMG_BURNADDR_LNA (EFID01_EXT_FLASH_BASE_ADDR + EFID01_EFS_IMG_LIMIT_SIZE)
// EXT UPDATER image size, needs to be less than or equal to external Flash size
#define EF_EXT_UPDATER_IMG_LIMIT_SIZE   (0x24000)
#define EFID01_EXT_UPDATER_IMG_LIMIT_SIZE   (0x24000)

#endif /* __QUEC_MEM_MAP_718HM_H__ */
