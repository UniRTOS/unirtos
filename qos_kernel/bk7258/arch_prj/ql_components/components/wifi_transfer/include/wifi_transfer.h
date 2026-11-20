
#pragma once

#include <os/os.h>
#include <common/bk_include.h>
#include <components/video_types.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uint8_t id;
	uint8_t eof;
	uint8_t cnt;
	uint8_t size;
	uint8_t data[];
} transfer_data_t;

typedef int (*media_transfer_send_cb)(uint8_t *data, uint32_t length);
typedef int (*media_transfer_prepare_cb)(uint8_t *data, uint32_t length);
typedef void* (*media_transfer_get_tx_buf_cb)(void);
typedef int (*media_transfer_get_tx_size_cb)(void);
typedef bool (*media_transfer_drop_check_cb)(frame_buffer_t *frame,uint32_t count, uint16_t ext_size);

typedef struct {
	media_transfer_send_cb send;
	media_transfer_prepare_cb prepare;
	media_transfer_drop_check_cb drop_check;
	media_transfer_get_tx_buf_cb get_tx_buf;
	media_transfer_get_tx_size_cb get_tx_size;
	frame_buffer_t *(*read)(image_format_t format, uint32_t timeout);
	void (*free)(image_format_t format, frame_buffer_t *frame);
} media_transfer_cb_t;

#define wifi_transfer_data_check(data,length) wifi_transfer_data_check_caller((const char*)__FUNCTION__,__LINE__,data,length)

void wifi_transfer_data_check_caller(const char *func_name, int line,uint8_t *data, uint32_t length);

bk_err_t bk_wifi_transfer_frame_open(const media_transfer_cb_t *cb, uint16_t img_format);
bk_err_t bk_wifi_transfer_frame_close(void);

#ifdef __cplusplus
}
#endif

