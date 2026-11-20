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

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Media transmission lengths and timing parameters */
#define MEDIA_UDP_TRAN_LEN              (1472)  /**< UDP transmission length in bytes */
#define MEDIA_TCP_TRAN_LEN              (1460)  /**< TCP transmission length in bytes */
#define MEDIA_NET_TRAN_MAX_LEN          (1024)  /**< Maximum network transmission length in bytes */
#define MEDIA_RETRY_DELAY_TIME          (2000)  /**< Retry delay time in milliseconds */
#define MEDIA_TRAN_DELAY_TIME_MS        (10)    /**< Transmission delay time in milliseconds */

/**
 * @brief video sample module protocol type
 */
typedef enum {
    TVIDEO_OPEN_NONE         = 0, /**< not sample module */
    TVIDEO_OPEN_SCCB,             /**< sample module follow sccb protocol */
    TVIDEO_OPEN_SPIDMA,           /**< sample module follow spidma protocol */
    TVIDEO_OPEN_RTSP,           /**< sample module follow rtsp protocol */
} video_open_type_t;

/**
 * @brief video transfer network comunication protocol type
 */
typedef enum {
    TVIDEO_SND_NONE         = 0,  /**< not transfer */
    TVIDEO_SND_UDP,               /**< follow udp protocol */
    TVIDEO_SND_TCP,               /**< follow tcp protocol */
    TVIDEO_SND_INTF,              /**< transfer to inter frame */
    TVIDEO_SND_BUFFER,            /**< transfer to buffer */
} video_send_type_t;

/**
 * @brief Video configuration structure
 * Contains configuration parameters for video data reception and handling
 */
typedef struct {
    uint8_t *rxbuf; /**< Buffer to save camera data */

    /**
     * @brief Node full handler callback
     *
     * This function is called when transfer of node_len JPEG data is finished.
     * It transfers camera data to upper layer API.
     *
     * @param curptr The start address of transfer data
     * @param newlen The transfer data length
     * @param is_eof 0/1: whether this packet data is the last packet of this frame, 
     *               will be called in jpeg_end_frame ISR
     * @param frame_len The complete JPEG frame size. If is_eof=1, frame_len is the true 
     *                  value of JPEG frame size. If is_eof=0, frame_len=0. In other words, 
     *                  frame_len is only transferred at the last packet in jpeg_end_frame ISR.
     */
    void (*node_full_handler)(void *curptr, uint32_t newlen, uint32_t is_eof, uint32_t frame_len);

    /**
     * @brief Data end handler callback
     *
     * This API is used to inform the video transfer thread to process transferred camera data
     */
    void (*data_end_handler)(void);

    uint16_t rxbuf_len;  /**< Length of the receiving camera data buffer */
    uint16_t rx_read_len;/**< Manages the node_full_handler callback function input parameters */
    uint32_t node_len;   /**< Video transfer network communication protocol length at a time */
} video_config_t;

/**
 * @brief Video packet structure
 * Contains information about a single video packet
 */
typedef struct {
    uint8_t *ptk_ptr;    /**< Pointer to the packet data */
    uint32_t ptklen;     /**< The current packet length */
    uint32_t frame_id;   /**< The current packet frame ID */
    uint32_t is_eof;     /**< The current packet is the last packet (1 = last packet) */
    uint32_t frame_len;  /**< The frame length */
} video_packet_t;

/** @brief Function pointer type for adding packet headers */
typedef void (*tvideo_add_pkt_header)(video_packet_t *param);

/** @brief Function pointer type for video transfer send function */
typedef int (*video_transfer_send_func)(uint8_t *data, uint32_t len);

/** @brief Function pointer type for video transfer start callback */
typedef void (*video_transfer_start_cb)(void);

/** @brief Function pointer type for video transfer end callback */
typedef void (*video_transfer_end_cb)(void);

/**
 * @brief Video setup structure
 * Contains configuration parameters for video transfer setup
 */
typedef struct {
    uint16_t open_type;                  /**< Video sample module protocol type (video_open_type_t) */
    uint16_t send_type;                  /**< Video transfer network communication protocol type (video_send_type_t) */
    uint16_t pkt_header_size;            /**< Packet header size in bytes */
    uint16_t pkt_size;                   /**< Packet size in bytes */
    video_transfer_send_func send_func;  /**< Function pointer for sending data to upper layer */
    video_transfer_start_cb start_cb;    /**< Function pointer for starting data transfer to upper layer */
    video_transfer_start_cb end_cb;      /**< Function pointer for ending data transfer to upper layer */
    tvideo_add_pkt_header add_pkt_header;/**< Function pointer for adding packet headers */
} video_setup_t;


/**
 * @brief Video header structure
 * Contains header information for video frames
 */
typedef struct {
    uint8_t id;      /**< The frame ID */
    uint8_t is_eof;  /**< End of frame flag (1 = end) */
    uint8_t pkt_cnt; /**< Packet count of one frame */
    uint8_t pkt_seq; /**< Packet header's count/sequence of one frame */
} video_header_t;

/**
 * @brief Video buffer structure
 * Contains buffer information for video data reception
 */
typedef struct {
    beken_semaphore_t aready_semaphore; /**< Semaphore indicating video data reception is complete */
    uint8_t *buf_base;                  /**< Received video data buffer (allocated by user) */
    uint32_t buf_len;                   /**< Video buffer length (allocated by user) */
    uint32_t frame_id;                  /**< Frame ID */
    uint32_t frame_pkt_cnt;             /**< Packet count of one frame */
    uint8_t *buf_ptr;                   /**< Buffer pointer recording each video packet reception */
    uint32_t frame_len;                 /**< Length of received frame */
    uint32_t start_buf;                 /**< Video buffer receive state */
} video_buff_t;

/**
 * @brief Video buffer states
 * Represents the different states of a video buffer
 */
typedef enum {
    BUF_STA_INIT = 0,    /**< Video buffer initialized */
    BUF_STA_COPY,        /**< Video buffer copying data */
    BUF_STA_GET,         /**< Video frame received */
    BUF_STA_FULL,        /**< Video buffer full */
    BUF_STA_DEINIT,      /**< Video buffer deinitialized */
    BUF_STA_ERR,         /**< Video buffer error */
} video_buff_state_t;


#ifdef __cplusplus
}
#endif
