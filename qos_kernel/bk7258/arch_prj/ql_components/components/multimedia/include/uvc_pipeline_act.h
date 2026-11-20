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

#pragma once
#include <common/bk_include.h>
#include <components/media_types.h>
#include <driver/lcd_types.h>
#include <components/bk_video_pipeline/bk_video_pipeline.h>

#ifdef __cplusplus
extern "C" {
#endif

//this function is called by h264_jdec_pipeline_open and lcd_jdec_pipeline_open
bk_err_t uvc_pipeline_init(void);

//this function call uvc_pipeline_init when the h264 pipeline is opened
bk_err_t h264_jdec_pipeline_open(bk_video_pipeline_h264e_config_t *config, const bk_h264e_callback_t *cb,
	 				const jpeg_callback_t *jpeg_cbs, const decode_callback_t *decode_cbs);
//this function don't call uvc_pipeline_deinit when the h264 pipeline is closed
bk_err_t h264_jdec_pipeline_close(void);
bk_err_t h264_jdec_pipeline_regenerate_idr_frame(void);

//this function call uvc_pipeline_init when the lcd pipeline is opened
bk_err_t lcd_jdec_pipeline_open(bk_video_pipeline_decode_config_t *config, const jpeg_callback_t *jpeg_cbs, const decode_callback_t *decode_cbs);
//this function don't call uvc_pipeline_deinit when the lcd pipeline is closed
bk_err_t lcd_jdec_pipeline_close(void);

uint8_t *get_mux_sram_decode_buffer(void);
uint8_t *get_mux_sram_rotate_buffer(void);
uint8_t *get_mux_sram_scale_buffer(void);

//this two functions should be called when the uvc pipeline is not used
bk_err_t uvc_pipeline_deinit(void);
bk_err_t deinit_all_buffer(void);

#ifdef __cplusplus
}
#endif


