#pragma once
#include "sdkconfig.h"
#include "driver/flash.h"
#include "driver/flash_partition.h"
#include "common/bk_err.h"
#include "CheckSumUtils.h"

#define OTA_TAG "OTA"

#ifdef CONFIG_OTA_DEBUG_LOG_OPEN
#define OTA_LOGI(...)	BK_LOGI(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGW(...)	BK_LOGW(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGE(...)	BK_LOGE(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGD(...)	BK_LOGD(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGV(...)	BK_LOGV(OTA_TAG, ##__VA_ARGS__)
#else
#define OTA_LOGI(...)	BK_LOGI(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGW(...)
#define OTA_LOGE(...)	BK_LOGE(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGD(...)
#define OTA_LOGV(...)
#endif

#ifndef ERASE_TOUCH_TIMEOUT
#define ERASE_TOUCH_TIMEOUT          (3000)
#endif
#ifndef ERASE_FLASH_TIMEOUT
#define ERASE_FLASH_TIMEOUT          (56)
#endif
#ifndef WRITE_FLASH_TIMEOUT
#define WRITE_FLASH_TIMEOUT          (4)
#endif
#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE            (0x1000)
#endif
#define OTA_FLASH_BUFFER_LENGTH      (1024)
#define OTA_CRC_BUFFER_LENGTH        (32)
#define OTA_TEMP_FLASH_BUFFER_LENGTH (1024*5)
#define FD_DEST_PATH_LEN             (20)

#ifndef MIN
#define MIN(x, y)                  (((x) < (y)) ? (x) : (y))
#endif

#define OTA_CHECK_POINTER(ptr) do{ \
    if(ptr == NULL){ \
        return BK_FAIL; \
    } \
}while(0)

#define OTA_MALLOC(ptr, size) do{ \
    ptr = (void*)os_malloc(size); \
    if(ptr == NULL){ \
        OTA_LOGE("%s malloc fail \r\n",ptr); \
        return BK_FAIL; \
    } \
    os_memset(ptr,0 ,size); \
}while(0)

#define OTA_MALLOC_WITHOUT_RETURN(ptr, size) do{ \
    ptr = (void*)os_malloc(size); \
    if(ptr == NULL){ \
        OTA_LOGE("%s malloc fail \r\n",ptr); \
        return ; \
    } \
    os_memset(ptr,0 ,size); \
}while(0)

#define OTA_FREE(ptr) do{ \
    if(ptr != NULL){ \
        os_free(ptr); \
        ptr = NULL; \
    } \
}while(0)

typedef enum{
	OTA_TYPE_WIFI = 0,    /* OTA update via      WIFI   */
	OTA_TYPE_BLE,        /* OTA update via Bluetooth */
}ota_update_type_t  ;

typedef struct __attribute__((packed))
{
    /* data */
    uint8_t  *magic_code;                  /*ota magic code*/
    uint16_t new_sequence_number;          /*the new sequence number*/
    uint16_t curr_sequence_number;         /*the current sequence number*/
    uint16_t wr_last_len ;                 /*the remaining lenth*/
    uint32_t partition_length;             /*the partition length*/
    uint32_t wr_address;                   /*the written address*/
    uint32_t image_size;                   /*the ota image total size*/
    uint32_t received_total_size;          /*receive the total image size*/  
    uint8_t  *wr_buf;                      /*store data from apk*/
    uint8_t  *wr_tmp_buf;                  /*store data from app temporarily*/
    uint8_t  *rd_buf;                      /*store data from falsh */
    bk_logic_partition_t *pt;              /**/
    flash_protect_type_t  protect_type;    /**/
    uint8_t  wr_flash_flag;                /*the write flash flag*/
    int      fd;                           /*the file*/
    char dest_path_name[FD_DEST_PATH_LEN]; /*file path name*/
    uint8_t  init_flag;                    /*init flag*/ 
    ota_update_type_t ota_type;            /*ota update type*/
    CRC32_Context ota_crc;
}f_ota_t;

typedef int (*ota_wr_callback)(f_ota_t* ota_ptr, uint16_t len);
typedef struct
{
    /* data */
    int (*init)(f_ota_t* ota_ptr);
    int (*open)(f_ota_t* ota_ptr);
    void (*mount)(f_ota_t* ota_ptr);
    int (*read)(f_ota_t* ota_ptr, uint16_t len);
    int (*seek)(f_ota_t* ota_ptr, uint16_t len);
    int (*write)(f_ota_t* ota_ptr, uint16_t len);
    int (*wr_flash)(f_ota_t* ota_ptr, uint16_t len);
    int (*data_process)(f_ota_t* ota_ptr, uint16_t len, ota_update_type_t ota_type, ota_wr_callback wr_callback);
    int (*crc)(f_ota_t* ota_ptr, uint32_t in_crc);
    void (*umount)(f_ota_t* ota_ptr);
    int (*close)(f_ota_t* ota_ptr);
    int (*deinit)(f_ota_t* ota_ptr);
}f_ota_func_t;

extern const f_ota_func_t *f_ota_fun_ptr;