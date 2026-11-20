#pragma once

#include <os/os.h>
#include <os/mem.h>
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/video_types.h>
#include "network_transfer.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_NTWK_USE_PSARM_MEM
#define ntwk_malloc   psram_malloc
#else
#define ntwk_malloc   os_malloc
#endif

#if CONFIG_NTWK_PKT_USE_PSARM_MEM
#define ntwk_packet_malloc   psram_malloc
#else
#define ntwk_packet_malloc   os_malloc
#endif

typedef bk_err_t (*ntwk_in_start_cb_t)(void *user_data);
typedef bk_err_t (*ntwk_in_stop_cb_t)(void);

typedef struct
{
    beken_thread_t thd;
    beken_queue_t queue;
    ntwk_trans_msg_event_cb_t event_cb;

    //ntwk_trans_ctxt_t *s_ntwk_trans_ctxt;

    ntwk_in_start_cb_t ctrl_start;
    ntwk_in_stop_cb_t ctrl_stop;

    ntwk_in_start_cb_t video_start;
    ntwk_in_stop_cb_t video_stop;

    ntwk_in_start_cb_t audio_start;
    ntwk_in_stop_cb_t audio_stop;
} ntwk_in_cfg_t;


bk_err_t ntwk_msg_init(void);
bk_err_t ntwk_msg_deinit(void);

bk_err_t ntwk_msg_start(void);
bk_err_t ntwk_msg_stop(void);

void ntwk_msg_event_report(uint32_t event, uint32_t param, uint32_t chan_type);

bk_err_t ntwk_msg_register_event_cb(ntwk_trans_msg_event_cb_t cb);

bk_err_t ntwk_in_register_ctrl_start_cb(ntwk_in_start_cb_t cb);
bk_err_t ntwk_in_register_ctrl_stop_cb(ntwk_in_stop_cb_t cb);
bk_err_t ntwk_in_register_video_start_cb(ntwk_in_start_cb_t cb);
bk_err_t ntwk_in_register_video_stop_cb(ntwk_in_stop_cb_t cb);
bk_err_t ntwk_in_register_audio_start_cb(ntwk_in_start_cb_t cb);
bk_err_t ntwk_in_register_audio_stop_cb(ntwk_in_stop_cb_t cb);

bk_err_t ntwk_in_start(chan_type_t chan_type, void *param);
bk_err_t ntwk_in_stop(chan_type_t chan_type);

#ifdef __cplusplus
}
#endif