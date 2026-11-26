/**
  @file
  ql_api_common.h

  @brief
  quectel common definition.

*/
/*================================================================
  Copyright (c) 2020 Quectel Wireless Solution, Co., Ltd.  All Rights Reserved.
  Quectel Wireless Solution Proprietary and Confidential.
=================================================================*/
/*=================================================================

						EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN              WHO         WHAT, WHERE, WHY
------------     -------     -------------------------------------------------------------------------------

=================================================================*/

#ifndef QL_API_COMMON_H
#define QL_API_COMMON_H

#include "commontypedef.h"
#include "mem_map.h"
#ifdef __cplusplus
extern "C" {
#endif


/*========================================================================
 *  Macro Definition
 *========================================================================*/

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define	QOSA_FLASH_HEADER1_ADDR                 BLS_SEC_HAED_ADDR
#define	QOSA_FLASH_HEADER2_ADDR                 SYS_SEC_HAED_ADDR
#define	QOSA_FLASH_FUSEMIRROR_ADDR              FUSE_FLASH_MIRROR_ADDR
#define	QOSA_FLASH_BOOTLOADER_ADDR              (BOOTLOADER_FLASH_LOAD_ADDR&(~CP_FLASH_XIP_ADDR))
#define	QOSA_FLASH_FACTORY_ADDR                 NVRAM_FACTORY_PHYSICAL_BASE
#define	QOSA_FLASH_CP_IMG_ADDR                  (CP_FLASH_LOAD_ADDR&(~CP_FLASH_XIP_ADDR))
#define	QOSA_FLASH_AP_IMG_ADDR                  UNIR_AP_IMG_ADDR
#define	QOSA_FLASH_LFS_ADDR                     FLASH_FS_REGION_START
#define	QOSA_FLASH_UPDATER_ADDR                 UNIR_UPDATER_ADDR
#define	QOSA_FLASH_FOTA_ADDR                    FLASH_FOTA_REGION_START
#define	QOSA_FLASH_REL_ADDR                     NVRAM_PHYSICAL_BASE
#define	QOSA_FLASH_PLAT_INFO_ADDR               FLASH_MEM_PLAT_INFO_NONXIP_ADDR


#define	QOSA_FLASH_HEADER1_SIZE                 BLS_FLASH_LOAD_SIZE
#define	QOSA_FLASH_HEADER2_SIZE                 SYS_FLASH_LOAD_SIZE
#define	QOSA_FLASH_FUSEMIRROR_SIZE              FUSE_FLASH_MIRROR_SIZE
#define	QOSA_FLASH_BOOTLOADER_SIZE              BOOTLOADER_FLASH_LOAD_SIZE
#define	QOSA_FLASH_FACTORY_SIZE                 NVRAM_FACTORY_PHYSICAL_SIZE
#define	QOSA_FLASH_CP_IMG_SIZE                  CP_FLASH_LOAD_SIZE
#define	QOSA_FLASH_AP_IMG_SIZE                  UNIR_AP_IMG_SIZE
#define	QOSA_FLASH_LFS_SIZE                     FLASH_FS_REGION_SIZE
#define	QOSA_FLASH_UPDATER_SIZE                 UPDATER_FLASH_LOAD_SIZE
#define	QOSA_FLASH_FOTA_SIZE                    FLASH_FOTA_REGION_LEN
#define	QOSA_FLASH_REL_SIZE                     NVRAM_PHYSICAL_SIZE
#define	QOSA_FLASH_PLAT_INFO_SIZE               FLASH_MEM_PLAT_INFO_SIZE

/*========================================================================
*	Enumeration Definition
*========================================================================*/

/*
*		errcode definition
*
* 		4 bytes, little endian, signed type, as below:
*
*		----------------------------------------------------------------------------------------------------
*		|			 4				|			  3 		   |		  2 		  | 		1		   |
*		----------------------------------------------------------------------------------------------------
*		| High byte of component ID | low byte of component ID | High byte of errcode | low byte of errcode|
*		----------------------------------------------------------------------------------------------------
*
* component ID defined at enum type <ql_errcode_component_e>.
* detail errcode defined by each component.
*
*/
//common detail errcode, 2 bytes
typedef enum {
	QL_NO_ERROR = 0,
	QL_SUCCESS = 0,
	QL_INVALID_PARAM_ERR,
	QL_READ_NVM_ERR,
	QL_STORAGE_NVM_ERR,


	QL_GENERAL_ERROR = 0xFFFF,
} ql_errcode_e;

/*========================================================================
 *  Type Definition
 *========================================================================*/
#ifndef BOOLEAN
typedef unsigned char BOOLEAN;
#endif

#ifndef int8
typedef signed char int8;
#endif

#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef UINT8
typedef unsigned char UINT8;
#endif

#ifndef int16
typedef signed short int16;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef int32
typedef signed int int32;
#endif

#ifndef uint
typedef unsigned int uint;
#endif

#ifndef UINT
typedef unsigned int UINT;
#endif

#ifndef uint32
typedef unsigned int uint32;
#endif

#ifndef PVOID
typedef void *PVOID;
#endif

#ifndef TCHAR
typedef char TCHAR;
#endif

#ifndef int64
typedef long long   int64;
#endif

#ifndef uint64
typedef unsigned long long  uint64;
#endif


#ifndef size_t
typedef unsigned int size_t;
#endif
/*========================================================================
 *  Variable Definition
 *========================================================================*/
#define CHIP_SECTOR_SIZE 		  (0x1000)    	//erase 4k each times
#define CHIP_BLOCK_SIZE 		  (0x10000)    	//erase 64k each times


#define ERASE_SIZE_4K 		    	  (0x1000)    	//erase 4k each times
#define ERASE_SIZE_32K 		  	      (0x8000)    	//erase 32k each times
#define ERASE_SIZE_64K 		    	  (0x10000-4096)    	//erase 64k each times
#define ERASE_SIZE_128K 		      (0x20000)    	//erase 128k each times

/*========================================================================
*  function Definition
*========================================================================*/
typedef void (*ql_callback)(void *ctx);

#if defined CHIP_EC618 || defined CHIP_EC618_Z0
extern uint8_t  BSP_QSPI_Erase_Sector_Safe(uint32_t SectorAddress);
extern uint8_t  BSP_QSPI_Erase_32KBlk_Safe(uint32_t BlockAddress);
extern uint8_t  BSP_QSPI_Erase_Sector(uint32_t SectorAddress);
extern uint8_t  BSP_QSPI_Erase_Block(uint32_t BlockAddress);
extern uint8_t  BSP_QSPI_Write(uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
extern uint8_t  BSP_QSPI_XIP_Read(uint8_t *pData, uint32_t ReadAddr, uint32_t Size);

extern void     CPXIP_Enable(void);
extern void     CPXIP_DeInit(void);
extern uint8_t  CPXIP_QSPI_Erase_32Kblk(uint32_t SectorAddress);
extern uint8_t  CPXIP_QSPI_Erase_Sector(uint32_t SectorAddress);
extern uint8_t  CPXIP_QSPI_Write(uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
extern uint32_t CPXIP_QSPI_Read(uint8_t *pData, uint32_t ReadAddr, uint32_t Size);

#else

extern uint8_t FLASH_XIPRead(uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
extern uint8_t FLASH_write(uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
extern uint8_t FLASH_eraseSectorSafe(uint32_t SectorAddress);
extern uint8_t FLASH_erase32KBlkSafe(uint32_t SectorAddress);
extern uint8_t CPFLASH_read(uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
extern uint8_t CPFLASH_write(uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
extern uint8_t CPFLASH_eraseSector(uint32_t SectorAddress);
extern uint8_t CPFLASH_eraseBlock(uint32_t BlockAddress);
extern uint8_t CPFLASH_xipInit(void);


#endif


uint8_t FLASH_eraseSafe(uint32_t SectorAddress, uint32_t Size);
uint8_t FLASH_writeSafe(uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
uint8_t FLASH_readSafe(uint8_t *pData, uint32_t ReadAddr, uint32_t Size);



#ifdef QL_BOOTLOADER_BUILD
//ap flash operation
#if defined CHIP_EC618 || defined CHIP_EC618_Z0
#define QL_BSP_QSPI_ERASE_AP_FLASH(addr, size)        BSP_QSPI_Erase_Sector_Safe(addr)
#define QL_BSP_QSPI_ERASE_AP_FLASH_32K(addr, size)    BSP_QSPI_Erase_32KBlk_Safe(addr)
#define QL_BSP_QSPI_WRITE_AP_FLASH(buf, addr, size)   BSP_QSPI_Write(buf, addr, size)
#define QL_BSP_QSPI_READ_AP_FLASH(buf, addr, size)    BSP_QSPI_XIP_Read(buf, addr, size)

#else//ec71xx
#define QL_BSP_QSPI_ERASE_AP_FLASH(addr, size)        FLASH_eraseSectorSafe(addr)
#define QL_BSP_QSPI_ERASE_AP_FLASH_32K(addr, size)    FLASH_erase32KBlkSafe(addr)
#define QL_BSP_QSPI_WRITE_AP_FLASH(buf, addr, size)   FLASH_write(buf, addr, size)
#define QL_BSP_QSPI_READ_AP_FLASH(buf, addr, size)    FLASH_XIPRead(buf, addr, size)
#endif


#else

//ap flash operation
#if defined CHIP_EC618 || defined CHIP_EC618_Z0
#define QL_BSP_QSPI_ERASE_AP_FLASH(addr, size)        BSP_QSPI_Erase_Safe(addr, size)
#define QL_BSP_QSPI_ERASE_AP_FLASH_32K(addr, size)    BSP_QSPI_Erase_Safe(addr, size)
#define QL_BSP_QSPI_WRITE_AP_FLASH(buf, addr, size)   BSP_QSPI_Write_Safe(buf, addr, size)
#define QL_BSP_QSPI_READ_AP_FLASH(buf, addr, size)    BSP_QSPI_Read_Safe(buf, addr, size)
#else//ec71x
#define QL_BSP_QSPI_ERASE_AP_FLASH(addr, size)        FLASH_eraseSafe(addr, size)
#define QL_BSP_QSPI_ERASE_AP_FLASH_32K(addr, size)    FLASH_eraseSafe(addr, size)
#define QL_BSP_QSPI_WRITE_AP_FLASH(buf, addr, size)   FLASH_writeSafe(buf, addr, size)
#define QL_BSP_QSPI_READ_AP_FLASH(buf, addr, size)    FLASH_XIPRead(buf, addr, size)
#endif


#endif



#ifdef __cplusplus
} /*"C" */
#endif

#endif /* QL_API_COMMON_H */

