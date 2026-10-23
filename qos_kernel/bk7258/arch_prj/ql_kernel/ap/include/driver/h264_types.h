// Copyright 2022-2023 Beken
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
#include <driver/hal/hal_h264_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ERR_H264_DRIVER_NOT_INIT             (BK_ERR_H264_BASE - 1)
#define BK_ERR_H264_ISR_INVALID_ID				(BK_ERR_H264_BASE - 2)
#define BK_ERR_H264_INVALID_RESOLUTION_TYPE		(BK_ERR_H264_BASE - 3)
#define BK_ERR_H264_INVALID_PFRAME_NUMBER		(BK_ERR_H264_BASE - 4)
#define BK_ERR_H264_INVALID_QP					(BK_ERR_H264_BASE - 5)
#define BK_ERR_H264_INVALID_IMB_BITS			(BK_ERR_H264_BASE - 6)
#define BK_ERR_H264_INVALID_PMB_BITS			(BK_ERR_H264_BASE - 7)
#define BK_ERR_H264_INVALID_CONFIG_PARAM		(BK_ERR_H264_BASE - 8)
#define BK_ERR_H264_INVALID_PIXEL_HEIGHT		(BK_ERR_H264_BASE - 9)

#define INT_SKIP_FRAME      0x1
#define INT_FINAL_OUT       0x2
#define INT_LINE_DONE       0x4

#define	H264_CLK_SEL_480M       1
#define H264_AHB_CLK_DIV        3
#define H264_AHB_CLK_SEL		3
#define H264_INT_ENABLE			0x7
#define H264_INT_DISABLE		0

#define MAX_QP					51
#define MIN_QP					0
#define MAX_PFRAME				1023
#define MIN_PFRAME				0
#define MAX_IFRAME_BITS			3412
#define MAX_PFRAME_BITS			4095
#define MIN_FRAME_BITS			0
#define DEFAULT_SKIP_MODE		1

#ifdef CONFIG_H264_P_FRAME_CNT
#define H264_GOP_FRAME_CNT    (CONFIG_H264_P_FRAME_CNT + 1)
#define H265_GOP_FRAME_CNT    (H264_GOP_FRAME_CNT)
#else
#define H264_GOP_FRAME_CNT    (8)
#define H265_GOP_FRAME_CNT    (8)
#endif

typedef enum {
	H264_SKIP_FRAME = 0,
	H264_FINAL_OUT,
	H264_LINE_DONE,
	H264_ISR_MAX,
} h264_isr_type_t;

typedef struct
{
	uint8_t h264_state;
	uint8_t p_frame_cnt;
	uint8_t profile_id;
	uint8_t qp;
	uint16_t num_imb_bits;
	uint16_t num_pmb_bits;
	uint16_t width;
	uint16_t height;
} h264_base_config_t;

typedef struct {
	h264_qp_t qp;
	uint8_t  enable;
	uint16_t imb_bits;
	uint16_t pmb_bits;
} h264_compress_ratio_t;

typedef enum {
    H264_NAL_UNSPECIFIED     = 0,
    H264_NAL_SLICE           = 1,
    H264_NAL_DPA             = 2,
    H264_NAL_DPB             = 3,
    H264_NAL_DPC             = 4,
    H264_NAL_IDR_SLICE       = 5,
    H264_NAL_SEI             = 6,
    H264_NAL_SPS             = 7,
    H264_NAL_PPS             = 8,
    H264_NAL_AUD             = 9,
    H264_NAL_END_SEQUENCE    = 10,
    H264_NAL_END_STREAM      = 11,
    H264_NAL_FILLER_DATA     = 12,
    H264_NAL_SPS_EXT         = 13,
    H264_NAL_PREFIX          = 14,
    H264_NAL_SUB_SPS         = 15,
    H264_NAL_DPS             = 16,
    H264_NAL_RESERVED17      = 17,
    H264_NAL_RESERVED18      = 18,
    H264_NAL_AUXILIARY_SLICE = 19,
    H264_NAL_EXTEN_SLICE     = 20,
    H264_NAL_DEPTH_EXTEN_SLICE = 21,
    H264_NAL_B_FRAME         = 22,
    H264_NAL_P_FRAME         = 23,
    H264_NAL_I_FRAME         = 24,
    H264_NAL_UNSPECIFIED25   = 25,
    H264_NAL_UNSPECIFIED26   = 26,
    H264_NAL_UNSPECIFIED27   = 27,
    H264_NAL_UNSPECIFIED28   = 28,
    H264_NAL_UNSPECIFIED29   = 29,
    H264_NAL_UNSPECIFIED30   = 30,
    H264_NAL_UNSPECIFIED31   = 31,
} h264_type_t;

/**
 * @brief H264 compress ratio default config
 *
 * This is the default configuration for the H264 compress ratio.
 * The init_qp/i_min_qp/i_max_qp/p_min_qp/p_max_qp range is [15, 51].
 * The imb_bits/pmb_bits range is [50, 4095].
 *
 * @return h264_compress_ratio_t
 */
#define H264_COMPRESS_RATIO_DEFAULT_CONFIG() { \
    .qp = {                               \
        .init_qp = 25,                   \
        .i_min_qp = 25,                  \
        .i_max_qp = 51,                  \
        .p_min_qp = 25,                  \
        .p_max_qp = 51,                  \
    },                                \
    .enable = true,                    \
    .imb_bits = 160,                    \
    .pmb_bits = 60,                    \
}

#ifdef __cplusplus
}
#endif