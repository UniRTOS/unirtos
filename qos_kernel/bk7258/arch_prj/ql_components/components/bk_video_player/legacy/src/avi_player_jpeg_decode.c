#include "os/os.h"
#include "os/mem.h"
#include "driver/dma2d.h"
#include "avi_player_jpeg_decode.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_hw.h"
#include "components/media_types.h"

#define TAG "avi_player_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static beken_semaphore_t avi_player_dma2d_sem = NULL;
static frame_buffer_t *g_jpeg_frame = NULL;
static frame_buffer_t *g_dec_out_frame = NULL;
static bk_jpeg_decode_hw_handle_t avi_player_jpeg_decode_handle = NULL;

static bk_err_t avi_player_jpeg_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame);
static bk_err_t avi_player_jpeg_decode_in_complete(frame_buffer_t *in_frame);

#if defined(CONFIG_AVI_PLAYER_JPEG_DECODE_OPT) && (CONFIG_AVI_PLAYER_JPEG_DECODE_OPT)
static bk_jpeg_decode_hw_opt_config_t avi_player_jpeg_decode_opt_config = {
    .decode_cbs = {
        .in_complete = avi_player_jpeg_decode_in_complete,
        .out_complete = avi_player_jpeg_decode_complete,},
    .image_max_width = 0,
    .lines_per_block = JPEG_DECODE_OPT_LINES_PER_BLOCK_8,
    .is_pingpong = 1,
    .copy_method = JPEG_DECODE_OPT_COPY_METHOD_MEMCPY,
};
#else
static bk_jpeg_decode_hw_config_t avi_player_jpeg_decode_config = {
    .decode_cbs = {
        .in_complete = avi_player_jpeg_decode_in_complete,
        .out_complete = avi_player_jpeg_decode_complete,},
};
#endif

static void avi_player_dma2d_config_error(void *arg)
{
    LOGD("%s \n", __func__);
}

static void avi_player_dma2d_transfer_error(void *arg)
{
    LOGE("%s \n", __func__);
}

static void avi_player_dma2d_transfer_complete(void *arg)
{
    rtos_set_semaphore(&avi_player_dma2d_sem);
}

static bk_err_t avi_player_dma2d_yuyv2rgb565_init(void)
{
    bk_err_t ret;

    ret = rtos_init_semaphore_ex(&avi_player_dma2d_sem, 1, 0);
    if (BK_OK != ret) {
        LOGE("%s %d avi_player_dma2d_sem init failed\n", __func__, __LINE__);
        return ret;
    }

    bk_dma2d_driver_init();
    bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, avi_player_dma2d_config_error, NULL);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, avi_player_dma2d_transfer_error, NULL);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, avi_player_dma2d_transfer_complete, NULL);
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 1);

    return ret;
}

static bk_err_t avi_player_dma2d_yuyv2rgb565_deinit(void)
{
    bk_err_t ret;

    bk_dma2d_stop_transfer();
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 0);
    bk_dma2d_driver_deinit();
    ret = rtos_deinit_semaphore(&avi_player_dma2d_sem);
    if (BK_OK != ret) {
        LOGE("%s %d avi_player_dma2d_sem deinit failed\n", __func__, __LINE__);
    }

    return ret;
}

static void avi_player_dma2d_yuyv2rgb565(void *src, const void *dst, uint16_t width, uint16_t height, bool byte_swap, bk_avi_player_format_t output_format)
{
    dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};

    dma2d_memcpy_pfc.input_addr = (char *)src;
    dma2d_memcpy_pfc.output_addr = (char *)dst;
    dma2d_memcpy_pfc.mode = DMA2D_M2M_PFC;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_YUYV;
    dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;

    if (output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB565) {
        dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB565;
        dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;
    } else if (output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB888) {
        dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB888;
        dma2d_memcpy_pfc.dst_pixel_byte = THREE_BYTES;
    }

    dma2d_memcpy_pfc.dma2d_width = width;
    dma2d_memcpy_pfc.dma2d_height = height;
    dma2d_memcpy_pfc.src_frame_width = width;
    dma2d_memcpy_pfc.src_frame_height = height;
    dma2d_memcpy_pfc.dst_frame_width = width;
    dma2d_memcpy_pfc.dst_frame_height = height;
    dma2d_memcpy_pfc.src_frame_xpos = 0;
    dma2d_memcpy_pfc.src_frame_ypos = 0;
    dma2d_memcpy_pfc.dst_frame_xpos = 0;
    dma2d_memcpy_pfc.dst_frame_ypos = 0;
    dma2d_memcpy_pfc.input_red_blue_swap = 0;
    dma2d_memcpy_pfc.output_red_blue_swap = 0;

    if (byte_swap) {
        dma2d_memcpy_pfc.out_byte_by_byte_reverse = 1;
    } else {
        dma2d_memcpy_pfc.out_byte_by_byte_reverse = 0;
    }

    bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
    bk_dma2d_start_transfer();

    rtos_get_semaphore(&avi_player_dma2d_sem, BEKEN_NEVER_TIMEOUT);
}

static bk_err_t avi_player_jpeg_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    if (result == BK_OK) {
        LOGV("%s, %d, jpeg decode success! format_type: %d, out_frame: %p\n", __func__, __LINE__, format_type, out_frame);
    } else {
        LOGE("%s, %d, jpeg decode failed! format_type: %d, result: %d, out_frame: %p\n", __func__, __LINE__, format_type, result, out_frame);
    }

    return BK_OK;
}

static bk_err_t avi_player_jpeg_decode_in_complete(frame_buffer_t *in_frame)
{
    LOGV("%s %d in_frame: %p\n", __func__, __LINE__, in_frame);
    return BK_OK;
}

bk_err_t avi_player_jpeg_hw_decode_init(bk_avi_player_format_t output_format, uint32_t image_width)
{
    bk_err_t ret = BK_OK;

    g_dec_out_frame = os_malloc(sizeof(frame_buffer_t));
    if (g_dec_out_frame == NULL) {
        LOGE("%s %d g_dec_out_frame malloc failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    os_memset(g_dec_out_frame, 0x00, sizeof(frame_buffer_t));

    if (output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB565 || output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB888) {
        ret = avi_player_dma2d_yuyv2rgb565_init();
        if (ret != BK_OK) {
            LOGE("%s %d avi_player_dma2d_yuyv2rgb565_init failed\n", __func__, __LINE__);
            return ret;
        }
    }

    g_jpeg_frame = os_malloc(sizeof(frame_buffer_t));
    if (g_jpeg_frame == NULL) {
        LOGE("%s %d g_jpeg_frame malloc failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memset(g_jpeg_frame, 0x00, sizeof(frame_buffer_t));

#if defined(CONFIG_AVI_PLAYER_JPEG_DECODE_OPT) && (CONFIG_AVI_PLAYER_JPEG_DECODE_OPT)
    avi_player_jpeg_decode_opt_config.image_max_width = image_width;
    bk_hardware_jpeg_decode_opt_new(&avi_player_jpeg_decode_handle, &avi_player_jpeg_decode_opt_config);
#else
    bk_hardware_jpeg_decode_new(&avi_player_jpeg_decode_handle, &avi_player_jpeg_decode_config);
#endif
    bk_jpeg_decode_hw_open(avi_player_jpeg_decode_handle);

    return ret;
}

bk_err_t avi_player_jpeg_hw_decode_deinit(bk_avi_player_format_t output_format)
{
    bk_err_t ret = BK_OK;

    bk_jpeg_decode_hw_close(avi_player_jpeg_decode_handle);

    bk_jpeg_decode_hw_delete(avi_player_jpeg_decode_handle);
    avi_player_jpeg_decode_handle = NULL;

    if (output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB565 || output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB888) {
        ret = avi_player_dma2d_yuyv2rgb565_deinit();
        if (ret != BK_OK) {
            LOGE("%s %d avi_player_dma2d_yuyv2rgb565_deinit failed\n", __func__, __LINE__);
            return ret;
        }

        if (g_dec_out_frame->frame) {
            psram_free(g_dec_out_frame->frame);
            g_dec_out_frame->frame = NULL;
        }
    }

    if (g_dec_out_frame != NULL) {
        os_free(g_dec_out_frame);
        g_dec_out_frame = NULL;
    }

    if (g_jpeg_frame != NULL) {
        os_free(g_jpeg_frame);
        g_jpeg_frame = NULL;
    }

    return ret;
}

bk_err_t avi_player_jpeg_hw_decode_start(bk_avi_player_t *avi_player)
{
    bk_err_t ret = BK_FAIL;

    if (avi_player == NULL) {
        LOGE("[%s][%d] avi_player is null\r\n", __func__, __LINE__);
        return ret;
    }

    if (avi_player->video_frame == NULL) {
        LOGE("[%s][%d] video_frame is null\r\n", __func__, __LINE__);
        return ret;
    }

    if (avi_player->framebuffer == NULL) {
        LOGE("[%s][%d] framebuffer is null\r\n", __func__, __LINE__);
        return ret;
    }

    g_jpeg_frame->length = avi_player->video_len;
    g_jpeg_frame->frame = avi_player->video_frame;

    g_dec_out_frame->size = avi_player->frame_size;
    if (avi_player->output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB565 || avi_player->output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB888) {
        if (g_dec_out_frame->frame == NULL) {
            g_dec_out_frame->frame = psram_malloc(g_dec_out_frame->size);
            if (g_dec_out_frame->frame == NULL) {
                LOGE("%s %d g_dec_out_frame->frame malloc failed\n", __func__, __LINE__);
                return BK_FAIL;
            }
        }
    } else {
        g_dec_out_frame->frame = avi_player->framebuffer;
    }

    ret = bk_jpeg_decode_hw_decode(avi_player_jpeg_decode_handle, g_jpeg_frame, g_dec_out_frame);
    if (ret != BK_OK) {
        LOGE("%s bk_jpeg_decode_hw_decode fail %d\n", __func__, ret);
        return ret;
    }

    if (avi_player->output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB565 || avi_player->output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB888) {
        avi_player_dma2d_yuyv2rgb565(g_dec_out_frame->frame, avi_player->framebuffer, avi_player->avi->width, avi_player->avi->height, avi_player->swap_flag, avi_player->output_format);
    }

    return ret;
}
