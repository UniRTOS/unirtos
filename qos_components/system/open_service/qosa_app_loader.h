/*****************************************************************/ /**
* @file qosa_app_loader.h
* @brief
* @author mahat.xu@quectel.com
* @date 2026-03-10
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2026-03-10 <td>1.0 <td>Mahat.Xu <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_APP_LOADER_H__
#define __QOSA_APP_LOADER_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#ifdef __cplusplus
extern "C" {
#endif

/*
    User Guide:
    setp1 : call qosa_app_loader_init to init app loader information,each platform provides app loader information and guarantees its legality.

    setp2 : call qosa_sys_app_jump_boot to jump app boot function

    setp3 : call qosa_sys_app_jump_startup enter app main function

    The location of function calls is determined by each platform, but the initialization sequence must be guaranteed.
*/


#define QOSA_APP_IMG_MAGIC "QOS-APP"

// open app entry head structure 
// structure size is 96 bytes
typedef struct
{
	char	magic[16];
	int 	crc;                                    /*not used*/
	int     (*qosa_boot)(void *arvg);
	int     (*qosa_kernel)(void *arvg);
	int     (*qosa_app)(void *arvg);
	int     *app_config;
	int     *system_infomation;
	qosa_int32_t qos_app_rom_code_start_addr;       /* app rom code section start address*/
	qosa_int32_t qos_app_rom_rw_start_addr;         /* app rom rw section start address*/
	qosa_int32_t qos_app_ram_start_addr;            /* app ram  section start address*/
	qosa_int32_t qos_app_ram_rw_end_addr;           /* app ram rw section start addres*/
	qosa_int32_t qos_app_ram_end_addr;              /* app rom code section start address*/
	int  reserver[9];                               /* reserve for future*/ 
}qosa_app_entry_t;

// system static function typedef
typedef struct {
	int (*trace)(const char* fmt, ... );
	int (*sprintf)(char *, const char *, ...);
	int (*snprintf)(char*, size_t, const char*, ...);
	int (*sscanf)(const char*, const char*, ...);
	int (*printf)(const char* fmt, ... );
	int  reserver[16];
}qosa_sys_static_func_t;

typedef struct
{
    signed int hash;
    void * api_ptr;
}qosa_sys_api_list_t;

typedef struct
{
    //system api table list pointer , sys and user api table are in contiguous memory space.
    qosa_sys_api_list_t     *api_table_list;

    //system api table size , include user api table size
    qosa_uint32_t           api_table_size;

    // system static api function pointer
    qosa_sys_static_func_t  *static_func;

    //app entry pointer
    qosa_app_entry_t        *app_entry;

}qosa_app_load_info_t;

/*
* @brief init app loader information
* @param info app loader information reference qosa_app_load_info_t struct
*/
void qosa_app_loader_init(qosa_app_load_info_t *info);

/*
* @brief jump to applicable boot entry
* used user_boot_init macro in customer app code
*/
void qosa_sys_jump_app_boot(void);

/*
* @brief jump to applicable app entry
* used application_init macro in customer app code
*/
void qosa_sys_jump_app_startup(void);


#ifdef __cplusplus
extern "C" }
#endif

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif

