
#ifndef MEM_MAP_718HM_H
#define MEM_MAP_718HM_H


/*
flash layout, toatl 8MB
flash raw address: 0x00000000---0x00800000
flash xip address(from both ap/cp view): 0x00800000---0x01000000

0x00000000          |---------------------------------|
                    |      header1 4KB                |
0x00001000          |---------------------------------|
                    |      header2 4KB                |
0x00002000          |---------------------------------|
                    |      fuse mirror 4KB            |
0x00003000          |---------------------------------|
                    |      bl 128KB                   |
0x00023000          |---------------------------------|
                    |      rel data(factory)40KB      |
0x0002D000          |---------------------------------|

#if defined (FEATURE_AMR_CP_ENABLE) || defined (FEATURE_VEM_CP_ENABLE)
                    |      cp img 640KB               |
0x000CD000          |---------------------------------|
                    |      app img 6396KB             |
#else
                    |      cp img 400KB               |
0x00091000          |---------------------------------|
                    |      app img 6636KB             |
#endif
0x0070C000          |---------------------------------|
                    |      hib backup 96KB            |
0x00724000          |---------------------------------|
#if defined (FEATURE_FOTA_FS_ENABLE)
                    |      lfs  720KB                 |
0x007D8000          |---------------------------------|
                    |      fota rsvd 48KB             |
#else
                    |      lfs  256KB                 |
0x00764000          |---------------------------------|
                    |      fota 512KB                 |
#endif
0x007E4000          |---------------------------------|
                    |      rel data 104KB             |
0x007fe000          |---------------------------------|
                    |      plat config 8KB            |
0x00800000          |---------------------------------|


*/



#include "pkg_718hm_mapdef.h"


/* -------------------------------flash  address define-------------------------*/


#define AP_FLASH_XIP_ADDR               (0x00800000)


//for fuse mirror, due to limitation of fuse read times
#define FUSE_FLASH_MIRROR_XIP_ADDR      (AP_FLASH_XIP_ADDR+0x2000)
#define FUSE_FLASH_MIRROR_ADDR          (0x2000)
#define FUSE_FLASH_MIRROR_SIZE          (0x1000)


//bl addr and size
#define BOOTLOADER_FLASH_LOAD_ADDR              (0x00803000)
#define BOOTLOADER_FLASH_LOAD_UNZIP_SIZE        (0x22000)//136KB ,for ld

#if defined (CONFIG_QOSA_SECURITY_BOOT_BL) || defined (CONFIG_QOSA_SECURITY_BOOT_KERNEL)
#ifdef UNIRTOS_OPEN_MODE_SUPPORT
//updater/application secboot header addr and size
#define UPDATER_SECUREBOOT_LOAD_ADDR            ((BOOTLOADER_FLASH_LOAD_ADDR + BOOTLOADER_FLASH_PARTITION_SIZE - 2*SECUREBOOT_HEAD_LOAD_SIZE) - AP_FLASH_XIP_ADDR) // used the end of the bootloader to save Updater header
#define UPDATER_SECUREBOOT_LOAD_SIZE            (SECUREBOOT_HEAD_LOAD_SIZE)//4kB

#define APP_SECUREBOOT_LOAD_ADDR                ((BOOTLOADER_FLASH_LOAD_ADDR + BOOTLOADER_FLASH_PARTITION_SIZE - SECUREBOOT_HEAD_LOAD_SIZE) - AP_FLASH_XIP_ADDR) // used the end of the bootloader to save Updater header
#define APP_SECUREBOOT_LOAD_SIZE                (SECUREBOOT_HEAD_LOAD_SIZE)//4kB
#else /* UNIRTOS_OPEN_MODE_SUPPORT*/
//updater secboot header addr and size
#define UPDATER_SECUREBOOT_LOAD_ADDR            ((BOOTLOADER_FLASH_LOAD_ADDR + BOOTLOADER_FLASH_PARTITION_SIZE - SECUREBOOT_HEAD_LOAD_SIZE) - AP_FLASH_XIP_ADDR) // used the end of the bootloader to save Updater header
#define UPDATER_SECUREBOOT_LOAD_SIZE            (SECUREBOOT_HEAD_LOAD_SIZE)//4kB
#endif/* UNIRTOS_OPEN_MODE_SUPPORT*/
#define BOOTLOADER_FLASH_LOAD_SIZE              (UPDATER_SECUREBOOT_LOAD_ADDR+AP_FLASH_XIP_ADDR-BOOTLOADER_FLASH_LOAD_ADDR)
#else /* defined (CONFIG_QOSA_SECURITY_BOOT_BL) || defined (CONFIG_QOSA_SECURITY_BOOT_KERNEL)*/
#define BOOTLOADER_FLASH_LOAD_SIZE              (BOOTLOADER_FLASH_PARTITION_SIZE)// 128kB, real region size, tool will check when zip TODO:ZIP
#endif /* defined (CONFIG_QOSA_SECURITY_BOOT_BL) || defined (CONFIG_QOSA_SECURITY_BOOT_KERNEL)	*/

#ifdef UNIRTOS_PLATFORM_MODIFY
#include "quec_mem_map_718hm.h"
#else /* QUECTEL_PLATFORM_SUPPORT */

//ap image addr and size
#if defined (FEATURE_AMR_CP_ENABLE) || defined (FEATURE_VEM_CP_ENABLE)
#define AP_FLASH_LOAD_ADDR              (0x008CD000)

#define AP_FLASH_LOAD_SIZE              (0x63F000)//6396KB  
#define AP_FLASH_LOAD_UNZIP_SIZE        (0x6D6000)//7000KB ,for ld
#else
#define AP_FLASH_LOAD_ADDR              (0x00891000)

#define AP_FLASH_LOAD_SIZE              (0x67B000)//6636KB  
#define AP_FLASH_LOAD_UNZIP_SIZE        (0x6D6000)//7000KB ,for ld
#endif

//hib bakcup addr and size
#define FLASH_HIB_BACKUP_EXIST          (1)
#define FLASH_MEM_BACKUP_ADDR           (AP_FLASH_XIP_ADDR+FLASH_MEM_BACKUP_NONXIP_ADDR)
#define FLASH_MEM_BACKUP_NONXIP_ADDR    (0x70C000)
#define FLASH_MEM_BACKUP_SIZE           (0x18000)//96KB
#define FLASH_MEM_BLOCK_SIZE            (0x6000)
#define FLASH_MEM_BLOCK_CNT             (0x4)

//fs addr and size
#define FLASH_FS_REGION_START           (0x724000)
#define FLASH_FS_REGION_SIZE            (FLASH_FS_REGION_END-FLASH_FS_REGION_START)
#if defined (FEATURE_FOTA_FS_ENABLE)
#define FLASH_FS_REGION_END             (0x7D8000)  // 720KB

//fota addr and size
#define FLASH_FOTA_REGION_START         (0x7D8000)
#define FLASH_FOTA_REGION_LEN           (0xc000)//48KB
#else
#define FLASH_FS_REGION_END             (0x764000)

//fota addr and size
#define FLASH_FOTA_REGION_START         (0x764000)
#define FLASH_FOTA_REGION_LEN           (0x80000)//512KB
#endif
#define FLASH_FOTA_REGION_END           (0x7E4000)

#endif /* QUECTEL_PLATFORM_SUPPORT */

//ap reliable addr and size
#define NVRAM_FACTORY_PHYSICAL_BASE     (0x23000)
#define NVRAM_FACTORY_PHYSICAL_SIZE     (0xA000)//40KB   
#define NVRAM_PHYSICAL_BASE             (0x7E4000)
#define NVRAM_PHYSICAL_SIZE             (0x1A000)//104KB   

//plat config addr and size
#define FLASH_MEM_PLAT_INFO_ADDR        (AP_FLASH_XIP_ADDR+FLASH_MEM_PLAT_INFO_NONXIP_ADDR)
#define FLASH_MEM_PLAT_INFO_NONXIP_ADDR (0x7fe000)
#define FLASH_MEM_PLAT_INFO_SIZE        (0x1000)//4KB

#ifdef UNIRTOS_PLATFORM_MODIFY
/*quec plat config addr and size . This final 4K space has been confirmed by EIGEN that they will not use it.
Therefore, quectel uses this area to store important parameters, and this region cannot be erased frequently. */
#define QUEC_FLASH_MEM_PLAT_INFO_ADDR        (AP_FLASH_XIP_ADDR+QUEC_FLASH_MEM_PLAT_INFO_NONXIP_ADDR)
#define QUEC_FLASH_MEM_PLAT_INFO_NONXIP_ADDR (0x7ff000)
#define QUEC_FLASH_MEM_PLAT_INFO_SIZE        (0x1000)//4KB

#if ((FLASH_MEM_PLAT_INFO_NONXIP_ADDR+FLASH_MEM_PLAT_INFO_SIZE)!=(QUEC_FLASH_MEM_PLAT_INFO_NONXIP_ADDR)) 
#error "QUEC_FLASH_MEM_PLAT_INFO Partion Error!!!."
#endif

#endif /* UNIRTOS_PLATFORM_MODIFY */

#define CP_FLASH_XIP_ADDR               (0x00800000)

//cp img
#define CP_FLASH_LOAD_ADDR              (0x0082D000)
#if defined (FEATURE_AMR_CP_ENABLE) || defined (FEATURE_VEM_CP_ENABLE)
//cp img
#define CP_FLASH_LOAD_SIZE              (0xA0000)//640KB,real region size, tool will check when zip
#define CP_FLASH_LOAD_UNZIP_SIZE        (0xC8000)//800KB, for ld
#else
//cp img
#define CP_FLASH_LOAD_SIZE              (0x64000)//400KB,real region size, tool will check when zip
#define CP_FLASH_LOAD_UNZIP_SIZE        (0x80000)//512KB, for ld
#endif


//cp reliable addr and size, cp nvm write by ap
#define CP_NVRAM_FACTORY_PHYSICAL_BASE  (NVRAM_FACTORY_PHYSICAL_BASE + 0x1000)
#define CP_NVRAM_FACTORY_PHYSICAL_SIZE  (0x9000)//36KB      
#define CP_NVRAM_PHYSICAL_BASE          (NVRAM_PHYSICAL_BASE + 0x1000)
#define CP_NVRAM_PHYSICAL_SIZE          (0x19000)//100KB    


//add for img merge tool,should fix as AP_xx/CP_xx/BL_xx, tool will extract img type from it
//#define AP_IMG_MERGE_ADDR               (0x0007c000)
//#define CP_IMG_MERGE_ADDR               (0x00018000)
//#define BL_IMG_MERGE_ADDR               (0x00003000)

// Flash Dump Macros
#define FLASH_EXCEP_DUMP_ADDR            (FLASH_FOTA_REGION_END - FLASH_EXCEP_DUMP_SIZE)
#define FLASH_EXCEP_DUMP_SIZE            0x4000
#define FLASH_EXCEP_DUMP_SECTOR_NUM      0x4
#define FLASH_EXCEP_KEY_INFO_ADDR        0x0
#define FLASH_EXCEP_KEY_INFO_LEN         0x0

#if ((FLASH_EXCEP_DUMP_ADDR)>=(FLASH_FOTA_REGION_START)) && ((FLASH_EXCEP_DUMP_ADDR)<(FLASH_FOTA_REGION_END-0xB000))
#error "Vaild excep dump area in FOTA region is [FLASH_FOTA_REGION_END-0xB000,FLASH_FOTA_REGION_END]."
#endif

#ifdef FEATURE_EXCEPTION_FLASH_DUMP_ENABLE
/*
 *	BaseAddress is FLASH_EXCEP_DUMP_ADDR and the dump space is FLASH_EXCEP_DUMP_SIZE.
 *	offset 0                   offset 0x1000	          offset 0x3000
 *	|--------------------------|--------------------------|--------------------------|
 *	| 1K PLAT + 1K PHY + 2K PS |         8K UNILOG        |         4K CUST          |
 *	|--------------------------|--------------------------|--------------------------|
 */
#define FLASH_EXCEP_DUMP_ADDR_OFFSET_PLAT    0x0000
#define FLASH_EXCEP_DUMP_ADDR_OFFSET_PHY     0x0400
#define FLASH_EXCEP_DUMP_ADDR_OFFSET_PS      0x0800
#define FLASH_EXCEP_DUMP_ADDR_OFFSET_UNILOG  0x1000
#define FLASH_EXCEP_DUMP_ADDR_OFFSET_CUST    0x3000
#define FLASH_EXCEP_DUMP_SPACE_LIMIT_PLAT    (FLASH_EXCEP_DUMP_ADDR_OFFSET_PHY - FLASH_EXCEP_DUMP_ADDR_OFFSET_PLAT)
#define FLASH_EXCEP_DUMP_SPACE_LIMIT_PHY     (FLASH_EXCEP_DUMP_ADDR_OFFSET_PS - FLASH_EXCEP_DUMP_ADDR_OFFSET_PHY)
#define FLASH_EXCEP_DUMP_SPACE_LIMIT_PS      (FLASH_EXCEP_DUMP_ADDR_OFFSET_UNILOG - FLASH_EXCEP_DUMP_ADDR_OFFSET_PS)
#define FLASH_EXCEP_DUMP_SPACE_LIMIT_UNILOG  (FLASH_EXCEP_DUMP_ADDR_OFFSET_CUST - FLASH_EXCEP_DUMP_ADDR_OFFSET_UNILOG)
#define FLASH_EXCEP_DUMP_CUST_SPACE_LIMIT    (0x1000) /* Customer can change this macro depend on its real needs */
#endif



//for secure boot
#define BLS_SEC_HAED_ADDR               (0x0)
#define BLS_FLASH_LOAD_SIZE             (0x1000)
#define SYS_SEC_HAED_ADDR               (0x1000)
#define SYS_FLASH_LOAD_SIZE             (0x1000)

/* external storage device */
//e.g. external flash data addr and size
#define EF_FLASH_XIP_ADDR               (0x80000000)
#define EF_DATA_LOAD_ADDR               (0x00000000)
#define EF_DATA_LOAD_SIZE               (0x100000)//1MB

//e.g. external SD card data addr and size
#define SD_CARD_XIP_ADDR                (0x40000000)
#define SD_DATA_LOAD_ADDR               (0x00000000)
#define SD_DATA_LOAD_SIZE               (0x100000)//1MB

/* -----------ram address define, TODO: need modify according to ram lauout-------------*/

//csmb start
#define CSMB_START_ADDR                 (0x0)
#define CSMB_END_ADDR                   (0x10000)
#define CSMB_TOTAL_LENGTH               (CSMB_END_ADDR-CSMB_START_ADDR)
#define CSMB_RAMCODE_START_ADDR         (0x20)
#define CSMB_PHY_RET_DATA               (0xE400)
#define CSMB_PHY_AONMEM_ADDR            (0xF000)

//csmb end


//msmb start

/*
0x00400000          |---------------------------------|
                    |      LOAD_AP_FIRAM_MSMB         |
                    |---------------------------------|
                    |      LOAD_APOS                  |
                    |---------------------------------|
                    |      LOAD_AP_PS_FIRAM_MSMB      |
                    |---------------------------------|
                    |      LOAD_AP_MSMB_DATA          |
                    |---------------------------------|
                    |      LOAD_AP_MSMB_BSS           |
0x00418000          |---------------------------------|   <---MSMB_APMEM_END_ADDR
                    |      LOAD_CP_FIRAM_MSMB         |
                    |---------------------------------|
                    |      LOAD_CPOS                  |
                    |---------------------------------|
                    |      LOAD_CP_MSMB_BSS           |
0x00428000          |---------------------------------|   <---MSMB_CPMEM_END_ADDR

*/


#define MSMB_START_ADDR                 (0x00400000)
#define MSMB_END_ADDR                   (0x00428000)
#define MSMB_TOTAL_LENGTH               (MSMB_END_ADDR-MSMB_START_ADDR)

#define MSMB_PLAT_END_ADDR              (0x0040a000)

#define MSMB_PS_START_ADDR              (0x0040a000)
#define MSMB_PS_END_ADDR                (0x00418000)
#define MSMB_PHY_END_ADDR               (MSMB_END_ADDR)

#define MSMB_APMEM_END_ADDR             (0x00418000)
#define MSMB_CPMEM_START_ADDR           (0x00418000)
#define MSMB_CPDATA_START_ADDR          (0x00424000)

#define MSMB_CPMEM_LENGTH               (MSMB_END_ADDR-MSMB_CPMEM_START_ADDR)

#define EXCEPTION_INFO_MEM_END_ADDR     (MSMB_END_ADDR)

//msmb end


//asmb start
/*
0x00000000          |---------------------------------|
                    |      bootcode                   |
0x00001000          |---------------------------------|
                    |      LOAD_AP_PIRAM_ASMB         |
                    |---------------------------------|
                    |      LOAD_AP_FIRAM_ASMB         |
0x0000A800          |---------------------------------|   <---CP_AONMEMBACKUP_START_ADDR
                    |      UNLOAD_CPAON               |
0x0000B000          |---------------------------------|
                    |      LOAD_RRCMEM                |
0x0000C000          |---------------------------------|
                    |      LOAD_FLASHMEM              |
0x00010000          |---------------------------------|
*/
#define ASMB_START_ADDR                 (0x00000000)
#define ASMB_END_ADDR                   (0x00010000)
#define ASMB_TOTAL_LENGTH               (ASMB_END_ADDR-ASMB_START_ADDR)
#define ASMB_IRAM_START_ADDR            (0x00001000)
#define ASMB_RRCMEM_START_ADDR          (0x0000B000)
#define ASMB_FLASHMEM_START_ADDR        (0x0000C000)
#define ASMB_START_ADDR_FOR_CP          (ASMB_START_ADDR+0x200000)
#define CP_AONMEMBACKUP_START_ADDR      (0x0000A800)
#define ASMB_UNREMAP_START_ADDR         (0x00100000)
#define AMSB_CPAON_START_ADDR_FOR_CP    (CP_AONMEMBACKUP_START_ADDR+0x200000)

//asmb end



// ps ram
/*
    718PM PSRAM is a 4M Bytes physical memory (ranges are 0x08000000 ~ 0x08400000) but with three caches, 
    three caches are uesd by accessing PSRAM with a prerequisite as descripted below, accessing address 
    0x08xx_xxxx is mapped to Pcache0, and 0x0axx_xxxx is maped to Pcache1, and 0x0cxx_xxxx is mapping to Pcache2.

    PSRAM is divided as the following descripted:
0x08000000       |---------------------------------|
                 |           SHARED MEM            |
0x08002000       |---------------------------------|
                 |           CP PSRAM MEM          |
0x08042000       |---------------------------------|
                 |          AP PSRAM P0            |
0x08xxxxxx       |---------------------------------|
0x0axxxxxx       |---------------------------------|
                 |          AP PSRAM P1            |
0x0a140000       |---------------------------------|
0x0c140000       |---------------------------------|
                 |           PSRAM P2              |  <---  Reserved to customer.
0x0c400000       |---------------------------------|
*/

#define PSRAM_EXIST                     (1)
#define PSRAM_START_ADDR                (0x08000000)
#define PSRAM_END_ADDR                  (0x0c800000)
#define PSRAM_TOTAL_LENGTH              ((PSRAM_END_ADDR - PSRAM_START_ADDR) & 0x00ffffff)

#define PSRAM_PCACHE0_BASE              (0x0)
#define PSRAM_PCACHE1_BASE              (0x02000000)
#define PSRAM_PCACHE2_BASE              (0x04000000)

#define PSRAM_AREA_P0_OFFSET            (0)
#define PSRAM_AREA_APCP_SHARED_MEM      (0x2000)


#ifdef OPEN_CPU_MODE
#if FEATURE_IMS_ENABLE
#if defined(FEATURE_IMS_CC_ENABLE) || defined(FEATURE_AUDIO_ENABLE)
#define PSRAM_AREA_P0_CP_OFFSET         (0x4e400)
#define PSRAM_AREA_P1_OFFSET            (0x4e400)
#define PSRAM_AREA_P2_OFFSET            (0x00140000)
#else
#define PSRAM_AREA_P0_CP_OFFSET         (0x34400)
#define PSRAM_AREA_P1_OFFSET            (0x34400)
#define PSRAM_AREA_P2_OFFSET            (0x00140000)
#endif

#else //FEATURE_IMS_ENABLE

#if defined (FEATURE_AMR_CP_ENABLE) && defined (FEATURE_VEM_CP_ENABLE)
#define PSRAM_AREA_P0_CP_OFFSET         (0x4e400)
#define PSRAM_AREA_P1_OFFSET            (0x4e400)

#if defined(FEATURE_IMS_CC_ENABLE)
#define PSRAM_AREA_P2_OFFSET            (0x00140000)
#else
#define PSRAM_AREA_P2_OFFSET            (0x000e0000)
#endif

#else // defined (FEATURE_AMR_CP_ENABLE) && defined (FEATURE_VEM_CP_ENABLE)

#ifdef FEATURE_IMS_SMS_ENABLE
#define PSRAM_AREA_P0_CP_OFFSET         (0x34400)
#define PSRAM_AREA_P1_OFFSET            (0x34400)
#define PSRAM_AREA_P2_OFFSET            (0x00140000)
#else
#define PSRAM_AREA_P0_CP_OFFSET         (0x34400)
#define PSRAM_AREA_P1_OFFSET            (0x34400)
#define PSRAM_AREA_P2_OFFSET            (0x000e0000)
#endif //FEATURE_IMS_SMS_ENABLE
#endif //defined (FEATURE_AMR_CP_ENABLE) && defined (FEATURE_VEM_CP_ENABLE)
#endif // FEATURE_IMS_ENABLE
#else
#define PSRAM_AREA_P0_CP_OFFSET         (0x34400)
#define PSRAM_AREA_P1_OFFSET            (0x34400)
#define PSRAM_AREA_P2_OFFSET            (0x00140000)
#endif // OPEN_CPU_MODE

#define PSRAM_AREA_END_OFFSET           (0x00800000)

#define PSRAM_P0_START_ADDR            (PSRAM_START_ADDR|PSRAM_AREA_P1_OFFSET)
#define PSRAM_P1_START_ADDR            (PSRAM_START_ADDR|PSRAM_AREA_P1_OFFSET)
#define PSRAM_P2_START_ADDR            (PSRAM_START_ADDR|PSRAM_AREA_P2_OFFSET)

#define PSRAM_P0_LENGTH                (PSRAM_AREA_P2_OFFSET - PSRAM_AREA_P1_OFFSET)
#define PSRAM_P1_LENGTH                (PSRAM_AREA_P2_OFFSET - PSRAM_AREA_P1_OFFSET)
#define PSRAM_P2_LENGTH                (PSRAM_AREA_END_OFFSET - PSRAM_AREA_P2_OFFSET)


#define CP_PSRAM_P0_START_ADDR         (PSRAM_START_ADDR|PSRAM_PCACHE0_BASE|PSRAM_AREA_APCP_SHARED_MEM)
#define CP_PSRAM_P0_END_ADDR           (PSRAM_START_ADDR|PSRAM_PCACHE0_BASE|PSRAM_AREA_P0_CP_OFFSET)
#define CP_PSRAM_P0_LENGTH             (CP_PSRAM_P0_END_ADDR - CP_PSRAM_P0_START_ADDR)

#define SHARED_MEM_START_ADDR          (PSRAM_START_ADDR|PSRAM_AREA_P0_OFFSET)
#define SHARED_MEM_LENGTH              (PSRAM_AREA_APCP_SHARED_MEM-PSRAM_AREA_P0_OFFSET)

#define PSRAM_APMEM_END_ADDR           (PSRAM_START_ADDR|PSRAM_PCACHE1_BASE|PSRAM_AREA_P2_OFFSET)//(0x0a140000)

#define XP_SHAREINFO_BASE_ADDR          (0x08000000)
#define XP_DBGRESERVED_BASE_ADDR        (0x08000f00)
#define IPC_SHAREDMEM_START_ADDR        (0x08001000)

#ifdef OPEN_CPU_MODE
#if FEATURE_IMS_ENABLE
#ifdef FEATURE_IMS_USE_PSRAM_ENABLE
#define min_heap_size_threshold 0x57800//ims heap(250KB) will also use heap
#else
#define min_heap_size_threshold 0x5F000//ims heap(280KB) will also use heap
#endif
#if FEATURE_SUPPORT_APP_PCM_MEM_POOL//hal app mem pool 640*3+8align to 2K
#define heap_end_addr PSRAM_APMEM_END_ADDR  // should be 4 byte align
#else
#if defined(FEATURE_IMS_CC_ENABLE) || defined(FEATURE_AUDIO_ENABLE)
#define heap_end_addr PSRAM_APMEM_END_ADDR  // should be 4 byte align
#else
#define heap_end_addr PSRAM_APMEM_END_ADDR  // should be 4 byte align
#endif

#endif
#else

#define min_heap_size_threshold 0x19000
#if defined (FEATURE_AMR_CP_ENABLE) && defined (FEATURE_VEM_CP_ENABLE)
#if FEATURE_SUPPORT_APP_PCM_MEM_POOL//hal app mem pool 640*3+8align to 2K
#define heap_end_addr PSRAM_APMEM_END_ADDR  // should be 4 byte align
#else
#define heap_end_addr PSRAM_APMEM_END_ADDR  // should be 4 byte align
#endif
#else
#define heap_end_addr PSRAM_APMEM_END_ADDR  // should be 4 byte align
#endif
#endif

#define UP_BUF_MAX_SIZE 0x3CA00//only upbuf size, need another 512B for other buf also in this region

#else
#define min_heap_size_threshold 0x20000
#define heap_end_addr PSRAM_APMEM_END_ADDR  // should be 4 byte align
#define UP_BUF_MAX_SIZE 0x66D00//only upbuf size, need another 512B for other buf also in this region
#endif

#define heap_boundary_psram PSRAM_END_ADDR
#define cust_min_heap_size_threshold   (0x32000) // min heap size threshold is 200K.

#define LZMA_DECOMPRESS_THRESHOLD_SIZE  (0x32000)
#if ((UP_BUF_MAX_SIZE)<(LZMA_DECOMPRESS_THRESHOLD_SIZE))
#error "UP_BUF_MAX_SIZE can't less than LZMA decompress threshold size."
#endif
// TODO: need re-design excption dump

//配置UniRtos app的ram 
#ifdef UNIRTOS_OPEN_MODE_SUPPORT
#define APP_RAM_RESERVED_LENGTH        (UNIRTOS_OCPU_APP_RAM_SIZE) // Reserve for the application

#define APP_RAM_START_ADDR             (PSRAM_END_ADDR - APP_RAM_RESERVED_LENGTH)
#define APP_RAM_END_ADDR               (PSRAM_END_ADDR)
#define APP_RAM_TOTAL_LENGTH           (APP_RAM_END_ADDR - APP_RAM_START_ADDR)
#endif /* UNIRTOS_OPEN_MODE_SUPPORT */

// AP Section define
// AP Section define
#define SECTIONAP_LOAD_BOOTCODE                   1
#define SECTIONAP_LOAD_AP_PIRAM_ASMB              2
#define SECTIONAP_LOAD_AP_FIRAM_ASMB              3
#define SECTIONAP_LOAD_AP_RWDATA_ASMB             4
#define SECTIONAP_LOAD_PS_RWDATA_ASMB             5
#define SECTIONAP_LOAD_AP_PIRAM_MSMB              6
#define SECTIONAP_LOAD_AP_FIRAM_MSMB              7
#define SECTIONAP_LOAD_AP_DATA_MSMB               8
#define SECTIONAP_LOAD_APOS                       9
#define SECTIONAP_LOAD_PPSRAM_P0_BSP_DATA          10
#define SECTIONAP_LOAD_AP_PS_FIRAM_MSMB           11
#define SECTIONAP_LOAD_AP_FPSRAMP0_RAMCODE_PLAT    12
#define SECTIONAP_LOAD_AP_FPSRAMP0_RAMCODE_PS      13
#define SECTIONAP_LOAD_AP_FPSRAMP0_DATA            14
#define SECTIONAP_LOAD_AP_FPSRAMP1_DATA            15

#define SECTIONAP_HASH_CHECK_MAX_INDEX    SECTIONAP_LOAD_AP_FPSRAMP1_DATA

// CP Section define
#define SECTIONCP_LOAD_CP_PIRAM_CSMB             1
#define SECTIONCP_LOAD_CP_FIRAM_CSMB             2
#define SECTIONCP_LOAD_CP_SLP2PIRAM_MSMB         3
#define SECTIONCP_LOAD_CP_PIRAM_MSMB             4
#define SECTIONCP_LOAD_CP_FIRAM_MSMB_PART2       5
#define SECTIONCP_LOAD_CP_FIRAM_MSMB_PART1       6
#define SECTIONCP_LOAD_CPOS                      7
#define SECTIONCP_LOAD_CPDRAM_BSP_DATA           8
#define SECTIONCP_LOAD_CPDRAM_SHARED_DATA        9
#define SECTIONCP_LOAD_CP_PSRAM_P0_RAMCODE       10
#define SECTIONCP_LOAD_CP_PSRAM_P0_DATA          11
#define SECTIONCP_LOAD_CP_PSRAM_P0_RAMCODE_PART2 12

#define SECTIONCP_HASH_CHECK_MAX_INDEX    SECTIONCP_LOAD_CP_PSRAM_P0_RAMCODE_PART2


// BL Section define
#define SECTIONBL_LOAD_ASMB_SHARED            1
#define SECTIONBL_LOAD_AIRAM_OTHER_RAMCODE    2
#define SECTIONBL_LOAD_MIRAM_USB              3
#define SECTIONBL_LOAD_MIRAM_USB_DATA         4
#define SECTIONBL_LOAD_CIRAM_RAMCODE          5


#include "pkg_718hm_mapchk.h"

#endif
