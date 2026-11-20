#pragma once

#include "components/media_types.h"

#ifdef __cplusplus
extern "C" {
#endif


#define MEDIA_TX_BUFFER_THD_DEFAULT  (0)
#define MEDIA_H264_DOWN_DROP_LEVEL_MIN_PERIOD 10
#define MEDIA_H264_EVERY_DROP_LEVEL_MAX_STATUS_CNT 30
#define MEDIA_H264_MAX_DROP_LEVEL  6
#define MEDIA_H264_I_FRAME_MAX_NUM  2
#define MEDIA_H264_SUPPORT_MAX_P_FRAME_NUM  5

typedef bk_err_t (*ntwk_get_write_size_cb_t)(uint32_t *write_size);

typedef struct
{
	uint8_t I_num;
	uint8_t P_num;
	uint8_t drop_level;
	uint8_t drop_period;
	uint32_t status_total_size;
	uint32_t status_cnt;
	bool check_type;
	bool support_type;
	bool I_frame_droped;
	uint32_t buffer_thd;
	uint32_t start_drop_thd;
	uint32_t drop_level_interval;
	ntwk_get_write_size_cb_t write_size;
	bool initialized;
} ntwk_h264_drop_info_t;

bk_err_t ntwk_video_drop_init(void);
bk_err_t ntwk_video_drop_deinit(void);

bk_err_t ntwk_video_drop_start(uint32_t buffer_size);
bk_err_t ntwk_video_drop_stop(void);

bool ntwk_video_drop_check(frame_buffer_t *frame);

bk_err_t ntwk_register_get_drop_size_cb(ntwk_get_write_size_cb_t cb);

#ifdef __cplusplus
}
#endif
