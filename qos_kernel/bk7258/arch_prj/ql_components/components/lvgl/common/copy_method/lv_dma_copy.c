#include <os/os.h>
#include <driver/dma.h>
#include "bk_general_dma.h"
#include "lv_vendor.h"

#define TAG "DMA_CPY"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


static dma_id_t lvgl_dma_id = DMA_ID_MAX;
static uint8_t g_dma_use_flag = 0;
static bool lv_dma_cpy_is_init = false;
extern lv_vnd_config_t vendor_config;

static bk_err_t lv_get_dma_repeat_once_len(uint32_t total_len)
{
    uint32_t len = 0;
    uint32_t value = 0;
    uint8_t i = 0;

    for (i = 2; i < 30; i++)
    {
        len = total_len / i;
        if (len <= 0x10000)
        {
            value = total_len % i;
            if (!value)
            {
                return len;
            }
        }
    }
    LOGE("%s Error dma length\r\n", __func__);

    return len;
}

void lv_dma_memcpy_init(void)
{
    bk_err_t ret;
    uint32_t dma_repeat_once_len = 0;

    if (lv_dma_cpy_is_init) {
        LOGW("%s already init\r\n", __func__);
        return;
    }

    ret = bk_dma_driver_init();
    if (ret != BK_OK) {
        LOGE("dma driver init failed!\r\n");
        return;
    }

    lvgl_dma_id = bk_dma_alloc(DMA_DEV_DTCM);
    if ((lvgl_dma_id < DMA_ID_0) || (lvgl_dma_id >= DMA_ID_MAX)) {
        LOGE("lvgl dma malloc failed!\r\n");
        return;
    }

#if (CONFIG_SPE)
    bk_dma_set_src_sec_attr(lvgl_dma_id, DMA_ATTR_SEC);
    bk_dma_set_dest_sec_attr(lvgl_dma_id, DMA_ATTR_SEC);
    bk_dma_set_dest_burst_len(lvgl_dma_id, BURST_LEN_INC16);
    bk_dma_set_src_burst_len(lvgl_dma_id, BURST_LEN_INC16);
#endif

    dma_repeat_once_len = lv_get_dma_repeat_once_len(vendor_config.width * vendor_config.height * sizeof(bk_color_t));
    bk_dma_set_transfer_len(lvgl_dma_id, dma_repeat_once_len);

    lv_dma_cpy_is_init = true;
}

void lv_dma_memcpy_deinit(void)
{
    bk_err_t ret = BK_OK;

    if (!lv_dma_cpy_is_init) {
        LOGW("%s already init\r\n", __func__);
        return;
    }

    bk_dma_stop(lvgl_dma_id);

    ret = bk_dma_deinit(lvgl_dma_id);
    if (ret != BK_OK) {
        LOGE("%s dma deinit fail!\r\n", __func__);
        return;
    }

    ret = bk_dma_free(DMA_DEV_DTCM, lvgl_dma_id);
    if ( ret != BK_OK)
    {
        LOGE("lvgl dma free failed!\r\n");
        return;
    }

    // ret = bk_dma_driver_deinit();
    // if (ret != BK_OK)
    // {
    //     LOGE("dma driver deinit failed!\r\n");
    //     return;
    // }

    g_dma_use_flag = 0;
    lv_dma_cpy_is_init = false;
}

void lv_dma_stop_memcpy_last_frame(void)
{
    if (g_dma_use_flag) {
        bk_dma_stop(lvgl_dma_id);
        g_dma_use_flag = 0;
    }
}

void lv_dma_memcpy_wait_transfer_finish(void)
{
    while (g_dma_use_flag) {
        if (bk_dma_get_repeat_wr_pause(lvgl_dma_id)) {
            bk_dma_stop(lvgl_dma_id);
            break;
        }
    }
    g_dma_use_flag = 0;
}

void lv_dma_memcpy_last_frame(void *Psrc, void *Pdst, uint32_t xsize, uint32_t ysize)
{
    uint32_t frame_len = 0;

#if CONFIG_LVGL_V8
#if (LV_COLOR_DEPTH == 16)
    frame_len = xsize * ysize * 2;
#elif (LV_COLOR_DEPTH == 32)
    frame_len = xsize * ysize * 3;
#endif
#else
    frame_len = xsize * ysize * sizeof(bk_color_t);
#endif

    bk_dma_stateless_judgment_configuration(Pdst, Psrc, frame_len, lvgl_dma_id, NULL);
    dma_set_src_pause_addr(lvgl_dma_id, (uint32_t)Psrc + frame_len);
    dma_set_dst_pause_addr(lvgl_dma_id, (uint32_t)Pdst + frame_len);

    g_dma_use_flag = 1;
    bk_dma_start(lvgl_dma_id);
}