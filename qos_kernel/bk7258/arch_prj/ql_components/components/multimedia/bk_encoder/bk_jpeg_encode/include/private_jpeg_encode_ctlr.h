#pragma once

#include <os/os.h>
#include <components/media_types.h>
#include <components/bk_jpeg_encode_ctlr_types.h>

#define BK_JPEGENC_CRC_SIZE (5)
#define BK_JPEGENC_QUEUE_SIZE 5
#define BK_JPEGENC_FLEXA_LINE 8
#define BK_JPEGENC_DMA_CACHE_SIZE (1024 * 10)

 /**
 * @brief JPEG encoder status enumeration
 *
 * This enumeration defines the operational states of the JPEG encoder.
 */
typedef enum
{
    JPEG_ENCODE_STATE_INIT = 0,  /*!< Encoder is disabled and cannot perform encoding operations */
    JPEG_ENCODE_STATE_OPENED,   /*!< Encoder is opened and ready for encoding operations */
    JPEG_ENCODE_STATE_ENCODING, /*!< Encoder is encoding and ready for encoding operations */
    JPEG_ENCODE_STATE_CLOSED,   /*!< Encoder is closed and cannot perform encoding operations */
} private_jpeg_encode_ctlr_state_t;

typedef enum
{
    JPEG_ENCODE_EVENT_LINE_DONE = 0,
    JPEG_ENCODE_EVENT_FRAME_DONE,
    JPEG_ENCODE_EVENT_EXIT,
} private_jpeg_encode_event_t;

typedef struct
{
    private_jpeg_encode_event_t event;
    uint32_t param;
} private_jpeg_encode_msg_t;

typedef struct
{
    uint8_t task_running;                 /**< Task running */
    uint8_t encode_error;                 /**< Encode error */
    uint8_t dma_channel;                  /**< DMA channel */
    uint8_t line_done_index;              /**< Line done index */
    uint16_t line_done_cnt;               /**< Line done count */
    uint16_t encode_node_length;          /**< Encode node length */
    uint16_t width;                       /**< Width */
    uint16_t height;                      /**< Height */
    uint8_t *yuv_cache;                   /**< Ping-pong YUV cache buffer for encoder */
    frame_buffer_t *yuv_buffer;           /**< YUV buffer */
    frame_buffer_t *jpeg_buffer;          /**< JPEG buffer */
    beken_thread_t thread;                /**< Thread */
    beken_queue_t queue;                  /**< Queue */
    beken_semaphore_t sem;                /**< Semaphore */
    beken_semaphore_t sem_encode_done;    /**< Semaphore for encode done */
} private_jpeg_encode_driver_t;

typedef struct
{
    private_jpeg_encode_ctlr_state_t state;  /**< Current state of the JPEG encoder */
    bk_jpeg_encode_ctlr_config_t config;     /**< JPEG encoder configuration */
    bk_jpeg_encode_ctlr_t ops;               /**< JPEG encoder operations interface */
    private_jpeg_encode_driver_t* driver;    /**< JPEG encoder driver */
    uint8_t *yuv_cache;                     /**< Ping-pong YUV cache buffer for encoder */
}  private_jpeg_encode_ctlr_t;