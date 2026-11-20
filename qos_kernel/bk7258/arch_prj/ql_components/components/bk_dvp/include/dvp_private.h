#ifndef __DVP_PRIVATE_H__
#define __DVP_PRIVATE_H__

#define H264_SELF_DEFINE_SEI_SIZE (96)
#define FRAME_BUFFER_CACHE (1024 * 10)
#define DVP_FRAME_OK            (0)
#define DVP_FRAME_ERR           (-1)

//#define DVP_DIAG_DEBUG

#ifdef DVP_DIAG_DEBUG

#define DVP_DIAG_DEBUG_INIT()                   \
    do {                                        \
        GPIO_DOWN(2);                           \
        GPIO_DOWN(3);                           \
        GPIO_DOWN(4);                           \
        GPIO_DOWN(5);                           \
        GPIO_DOWN(44);                          \
        GPIO_DOWN(45);                          \
        GPIO_DOWN(46);                          \
    } while (0)

#define DVP_VSYNC_ENTRY()          GPIO_UP(2)
#define DVP_VSYNC_OUT()            GPIO_DOWN(2)

#define DVP_JPEG_EOF_ENTRY()       GPIO_UP(3)
#define DVP_JPEG_EOF_OUT()         GPIO_DOWN(3)

#define DVP_H264_EOF_ENTRY()       GPIO_UP(3)
#define DVP_H264_EOF_OUT()         GPIO_DOWN(3)

#define DVP_YUV_EOF_ENTRY()        GPIO_UP(4)
#define DVP_YUV_EOF_OUT()          GPIO_DOWN(4)

#define DVP_PPI_ERROR_ENTRY()       GPIO_UP(5)
#define DVP_PPI_ERROR_OUT()         GPIO_DOWN(5)

#define DVP_SIZE_ERROR_ENTRY()      GPIO_UP(44)
#define DVP_SIZE_ERROR_OUT()        GPIO_DOWN(44)

#define DVP_RESET_ENTRY()           GPIO_UP(45)
#define DVP_RESET_OUT()             GPIO_DOWN(45)

#define DVP_SOFT_REST_ENTRY()  GPIO_UP(46)
#define DVP_SOFT_REST_OUT()    GPIO_DOWN(46)

#else
#define DVP_DIAG_DEBUG_INIT()

#define DVP_VSYNC_ENTRY()
#define DVP_VSYNC_OUT()

#define DVP_JPEG_EOF_ENTRY()
#define DVP_JPEG_EOF_OUT()

#define DVP_H264_EOF_ENTRY()
#define DVP_H264_EOF_OUT()

#define DVP_YUV_EOF_ENTRY()
#define DVP_YUV_EOF_OUT()

#define DVP_SIZE_ERROR_ENTRY()
#define DVP_SIZE_ERROR_OUT()

#define DVP_RESET_ENTRY()
#define DVP_RESET_OUT()

#define DVP_PPI_ERROR_ENTRY()
#define DVP_PPI_ERROR_OUT()

#define DVP_SOFT_REST_ENTRY()
#define DVP_SOFT_REST_OUT()

#define DVP_VSYNC_ENTRY()
#define DVP_VSYNC_OUT()

#endif

typedef struct
{
    uint32_t yuv_em_addr;
    uint32_t yuv_pingpong_length;
    uint32_t yuv_data_offset;
    uint8_t dma_collect_yuv;
} encode_yuv_config_t;

// DVP event type
typedef enum {
    DVP_EVENT_NONE = 0,
    DVP_EVENT_JPEG_EOF,
    DVP_EVENT_H264_EOF,
    DVP_EVENT_YUV_EOF,
    DVP_EVENT_EXIT,
} dvp_event_type_t;

// DVP event message
typedef struct {
    dvp_event_type_t type;
    uint32_t param1;
    uint32_t param2;
} dvp_event_msg_t;

typedef struct
{
    uint8_t eof;
    uint8_t error;
    uint8_t i_frame;
    uint8_t not_free;
    uint8_t regenerate_idr;
    uint8_t sequence;
    uint8_t dma_channel;
    uint32_t frame_id;
    uint32_t dma_length;
    uint8_t *encode_buffer;
    media_state_t dvp_state;
    beken_semaphore_t sem;
    frame_buffer_t *encode_frame;
    frame_buffer_t *yuv_frame;
    const bk_dvp_callback_t *callback;
    const dvp_sensor_config_t *sensor;
    bk_dvp_config_t *config;
    encode_yuv_config_t yuv_config;

    // Thread processing related
    beken_thread_t dvp_thread;
    beken_queue_t dvp_msg_queue;
    beken_semaphore_t thread_sem;  // Thread semaphore
    volatile bool thread_should_exit;   // Thread exit flag

#if (MEDIA_DEBUG_TIMER_ENABLE)
    beken_timer_t timer;
    uint32_t curr_length;
    uint32_t later_seq;
    uint32_t later_kbps;
    uint32_t latest_kbps;
#endif
#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
    uint8_t sei[H264_SELF_DEFINE_SEI_SIZE]; // save frame infomation
#endif
} dvp_driver_handle_t;

/**
 * @brief dvp camera enable mclk
 * 
 * @param mclk mclk freq
 */
void dvp_camera_mclk_enable(mclk_freq_t mclk);

/**
 * @brief dvp camera disable mclk
 * 
 */
void dvp_camera_mclk_disable(void);

/**
 * @brief dvp camera io init
 * 
 * @param io_config dvp io config
 */
void dvp_camera_io_init(bk_dvp_io_config_t *io_config);

/**
 * @brief dvp camera io deinit
 * 
 * @param io_config dvp io config
 */
void dvp_camera_io_deinit(bk_dvp_io_config_t *io_config);


#endif