// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __OTA_PRIVATE_H__
#define __OTA_PRIVATE_H__

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <components/log.h>
#include <os/os.h>
#include "modules/ota.h"
#include "ota_base_drv.h"


#define OTA_ASSERT_ERR(cond)                              \
    do {                                             \
        if (!(cond)) {                                \
            os_printf("%s,condition %s,line = %d\r\n",__func__, #cond, __LINE__);  \
        }                                             \
    } while(0)


#define RBL_HEAD_POS            (0x1000)
#define RT_OTA_HASH_FNV_SEED    (0x811C9DC5)

/** @brief   This macro defines the status of the download.  */
#if CONFIG_OTA_EVADE_METHOD
#define DOWNLOAD_STATUS_POS			(12)
#define DOWNLOAD_START_FLAG			(0xFE)
#define DOWNLOAD_SUCCESS_FLAG		(0xFC)
#endif
/**
 * OTA firmware encryption algorithm and compression algorithm
 */
enum ota_algo
{
    OTA_CRYPT_ALGO_NONE    = 0L,               /**< no encryption algorithm and no compression algorithm */
    OTA_CRYPT_ALGO_XOR     = 1L,               /**< XOR encryption */
    OTA_CRYPT_ALGO_AES256  = 2L,               /**< AES256 encryption */
    OTA_CMPRS_ALGO_GZIP    = 1L << 8,          /**< Gzip: zh.wikipedia.org/wiki/Gzip */
    OTA_CMPRS_ALGO_QUICKLZ = 2L << 8,          /**< QuickLZ: www.quicklz.com */
};
typedef enum ota_algo ota_algo_t;

struct ota_rbl_head
{
    char magic[4];

    ota_algo_t algo;
    uint32_t timestamp;
    char name[16];
    char version[24];

    char sn[24];

    /* crc32(aes(zip(rtt.bin))) */
    uint32_t crc32;
    /* hash(rtt.bin) */
    uint32_t hash;

    /* len(rtt.bin) */
    uint32_t size_raw;
    /* len(aes(zip(rtt.bin))) */
    uint32_t size_package;

    /* crc32(rbl_hdr - info_crc32) */
    uint32_t info_crc32;
};

typedef struct
{
	char* src_path_name;
	ota_wr_destination_t ota_dest;
	f_ota_t fota_dl_info;
}ota_device_into_t;

typedef enum{
	EVT_OTA_START = 0,
	EVT_OTA_FAIL,
	EVT_OTA_SUCCESS,
}evt_ota;

typedef uint8_t (*ota_event_callback_t)(evt_ota event_param);
typedef uint8_t (*ota_progress_callback_t)(uint32_t received, uint32_t total);
int ota_progress_callback_register(ota_progress_callback_t callback);
int ota_event_callback_register(ota_event_callback_t callback);
int ota_input_event_handler(evt_ota event_param);
int ota_progress_event_handler(uint32_t received, uint32_t total);

typedef int (*ota_process_data_callback_t) (char*buff_data, uint32_t len, uint32_t received, uint32_t total);
void register_ota_callback(ota_process_data_callback_t ota_callback);
int ota_extract_path_segment(char *in_name, char *out_name);
int bk_ota_process_data(char*receive_data, uint32_t len, uint32_t received, uint32_t total);
int ota_get_init_status(void);
int ota_do_init_operation(void);
void ota_do_deinit_operation(void);
int ota_do_open_sysfile(void);
void ota_do_umount_sysfile(void);
ota_wr_destination_t ota_get_dest_id(void);

uint32 http_get_sapp_partition_length(bk_partition_t partition);
int ota_update_with_display_open(void);
int bk_ota_update_partition_flag(int input_val);

int32_t ota_get_rbl_head(const bk_logic_partition_t *bk_ptr, struct ota_rbl_head *hdr, uint32_t partition_len);
int32_t ota_hash_verify(const bk_logic_partition_t *part, const struct ota_rbl_head *hdr);
int32_t ota_do_hash_check(void);

int bk_http_ota_download(const char *uri);
int bk_https_ota_download(const char *url);

#if CONFIG_OTA_DISPLAY_PICTURE_DEMO
extern int bk_sconf_trans_start(void);
extern bk_err_t media_app_ota_disp_open(void);
extern bk_err_t media_app_ota_disp_close(void);
extern bk_err_t bk_ota_reponse_state_to_audio(int ota_state);
extern int bk_sconf_get_channel_name(char *chan);
extern int ntwk_trans_stop(void *user_data);
extern int ntwk_trans_start(void *user_data);
extern bk_err_t bk_dual_screen_avi_player_stop(void);
extern int audio_engine_deinit(void);
extern void bk_ota_display_init(void);
extern void bk_ota_display_deinit(void);
extern bk_err_t bk_ota_image_display_open(char *filename);
extern bk_err_t bk_ota_image_display_close(void);
int ota_update_with_display_close(void);
#endif

#endif


