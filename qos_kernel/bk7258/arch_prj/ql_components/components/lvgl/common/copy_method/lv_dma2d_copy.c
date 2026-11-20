#include <os/os.h>
#include "lv_vendor.h"
#include "driver/dma2d.h"

#define TAG "DMA2D_CPY"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static beken_semaphore_t lv_dma2d_sem = NULL;
static uint8_t lv_dma2d_use_flag = 0;
static bool lv_dma2d_cpy_is_init = false;
extern lv_vnd_config_t vendor_config;

static void lv_dma2d_config_error(void *arg)
{
    LOGE("%s \n", __func__);
}

static void lv_dma2d_transfer_error(void *arg)
{
    LOGE("%s \n", __func__);
}

static void lv_dma2d_transfer_complete(void *arg)
{
    rtos_set_semaphore(&lv_dma2d_sem);
}

void lv_dma2d_memcpy_init(void)
{
    bk_err_t ret;

    if (lv_dma2d_cpy_is_init) {
        LOGW("%s already init\r\n", __func__);
        return;
    }

    ret = rtos_init_semaphore_ex(&lv_dma2d_sem, 1, 0);
    if (BK_OK != ret) {
        LOGE("%s %d lv_dma2d_sem init failed\n", __func__, __LINE__);
        return;
    }

    bk_dma2d_driver_init();
    bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, lv_dma2d_config_error, NULL);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, lv_dma2d_transfer_error, NULL);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, lv_dma2d_transfer_complete, NULL);
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 1);

    lv_dma2d_cpy_is_init = true;
}

void lv_dma2d_memcpy_deinit(void)
{
    if (!lv_dma2d_cpy_is_init) {
        LOGW("%s already deinit\r\n", __func__);
        return;
    }

    bk_dma2d_stop_transfer();
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 0);
    bk_dma2d_driver_deinit();
    rtos_deinit_semaphore(&lv_dma2d_sem);

    lv_dma2d_use_flag = 0;
    lv_dma2d_cpy_is_init = false;
}

static void lv_dma2d_memcpy(void *Psrc, uint32_t src_xsize, uint32_t src_ysize,
                                    void *Pdst, uint32_t dst_xsize, uint32_t dst_ysize,
                                    uint32_t dst_xpos, uint32_t dst_ypos)
{
    dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};

    dma2d_memcpy_pfc.input_addr = (char *)Psrc;
    dma2d_memcpy_pfc.output_addr = (char *)Pdst;

#if CONFIG_LVGL_V8
#if (LV_COLOR_DEPTH == 16)
    dma2d_memcpy_pfc.mode = DMA2D_M2M;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_RGB565;
    dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB565;
    dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;
#elif (LV_COLOR_DEPTH == 32)
    dma2d_memcpy_pfc.mode = DMA2D_M2M_PFC;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_ARGB8888;
    dma2d_memcpy_pfc.src_pixel_byte = FOUR_BYTES;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB888;
    dma2d_memcpy_pfc.dst_pixel_byte = THREE_BYTES;
#endif
#else
#if (LV_COLOR_DEPTH == 16)
    dma2d_memcpy_pfc.mode = DMA2D_M2M;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_RGB565;
    dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB565;
    dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;
#elif (LV_COLOR_DEPTH == 24)
    dma2d_memcpy_pfc.mode = DMA2D_M2M;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_RGB888;
    dma2d_memcpy_pfc.src_pixel_byte = THREE_BYTES;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB888;
    dma2d_memcpy_pfc.dst_pixel_byte = THREE_BYTES;
#elif (LV_COLOR_DEPTH == 32)
    dma2d_memcpy_pfc.mode = DMA2D_M2M;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_ARGB8888;
    dma2d_memcpy_pfc.src_pixel_byte = FOUR_BYTES;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_ARGB8888;
    dma2d_memcpy_pfc.dst_pixel_byte = FOUR_BYTES;
#endif
#endif

    dma2d_memcpy_pfc.dma2d_width = src_xsize;
    dma2d_memcpy_pfc.dma2d_height = src_ysize;
    dma2d_memcpy_pfc.src_frame_width = src_xsize;
    dma2d_memcpy_pfc.src_frame_height = src_ysize;
    dma2d_memcpy_pfc.dst_frame_width = dst_xsize;
    dma2d_memcpy_pfc.dst_frame_height = dst_ysize;
    dma2d_memcpy_pfc.src_frame_xpos = 0;
    dma2d_memcpy_pfc.src_frame_ypos = 0;
    dma2d_memcpy_pfc.dst_frame_xpos = dst_xpos;
    dma2d_memcpy_pfc.dst_frame_ypos = dst_ypos;

    bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
    bk_dma2d_start_transfer();
}

void lv_dma2d_memcpy_wait_transfer_finish(void)
{
    bk_err_t ret = BK_OK;

    if (lv_dma2d_sem && lv_dma2d_use_flag) {
        ret = rtos_get_semaphore(&lv_dma2d_sem, 1000);
        if (ret != kNoErr) {
            LOGE("%s lv_dma2d_sem get fail! ret = %d\r\n", __func__, ret);
        }
        lv_dma2d_use_flag = 0;
    }
}

void lv_dma2d_memcpy_last_frame(void *Psrc, void *Pdst, uint32_t xsize, uint32_t ysize, uint32_t src_offline, uint32_t dest_offline)
{
#if CONFIG_LVGL_V8
    dma2d_memcpy_psram(Psrc, Pdst, xsize, ysize, src_offline, dest_offline);
#else
    lv_dma2d_memcpy(Psrc, xsize, ysize, Pdst, xsize, ysize, src_offline, dest_offline);
#endif
    lv_dma2d_use_flag = 1;
}

void lv_dma2d_stop_memcpy_last_frame(void)
{
    if (lv_dma2d_use_flag) {
        bk_dma2d_stop_transfer();
        lv_dma2d_use_flag = 0;
    }
}

void lv_dma2d_memcpy_double_draw_buffer(void *Psrc, uint32_t src_xsize, uint32_t src_ysize, void *Pdst, uint32_t dst_xpos, uint32_t dst_ypos)
{
    lv_dma2d_memcpy_wait_transfer_finish();
    lv_dma2d_memcpy(Psrc, src_xsize, src_ysize, Pdst, vendor_config.width, vendor_config.height, dst_xpos, dst_ypos);
    lv_dma2d_use_flag = 1;
}

void lv_dma2d_memcpy_single_draw_buffer(void *Psrc, uint32_t src_xsize, uint32_t src_ysize, void *Pdst, uint32_t dst_xpos, uint32_t dst_ypos)
{
    lv_dma2d_memcpy(Psrc, src_xsize, src_ysize, Pdst, vendor_config.width, vendor_config.height, dst_xpos, dst_ypos);
    lv_dma2d_use_flag = 1;
    lv_dma2d_memcpy_wait_transfer_finish();
}