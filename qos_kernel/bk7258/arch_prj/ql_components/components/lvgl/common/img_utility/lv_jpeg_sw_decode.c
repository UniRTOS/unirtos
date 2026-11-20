#include "os/os.h"
#include "os/mem.h"
#include "driver/dma2d.h"
#include "lv_jpeg_sw_decode.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_sw.h"
#include "components/media_types.h"

#define TAG "lv_sw_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static frame_buffer_t *g_dec_out_frame = NULL;
static bk_jpeg_decode_sw_handle_t lv_jpeg_decode_handle = NULL;

static bk_err_t lv_jpeg_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame);

static bk_jpeg_decode_sw_config_t lv_jpeg_decode_config = {
    .decode_cbs = {.out_complete = lv_jpeg_decode_complete,},
    .out_format = JPEG_DECODE_SW_OUT_FORMAT_RGB565,
};

static bk_err_t lv_jpeg_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    if (result == BK_OK) {
        LOGD("%s, %d, jpeg decode success! format_type: %d, out_frame: %p\n", __func__, __LINE__, format_type, out_frame);
    } else {
        LOGE("%s, %d, jpeg decode failed! format_type: %d, result: %d, out_frame: %p\n", __func__, __LINE__, format_type, result, out_frame);
    }

    return BK_OK;
}

bk_err_t lv_jpeg_sw_decode_init(void)
{
    bk_software_jpeg_decode_new(&lv_jpeg_decode_handle, &lv_jpeg_decode_config);
    bk_jpeg_decode_sw_open(lv_jpeg_decode_handle);

    g_dec_out_frame = os_malloc(sizeof(frame_buffer_t));
    if (!g_dec_out_frame) {
        LOGE("[%s][%d] g_dec_out_frame malloc fail\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
    }
    os_memset(g_dec_out_frame, 0, sizeof(frame_buffer_t));

    return BK_OK;
}

bk_err_t lv_jpeg_sw_decode_deinit(void)
{
    bk_jpeg_decode_sw_close(lv_jpeg_decode_handle);

    bk_jpeg_decode_sw_delete(lv_jpeg_decode_handle);
    lv_jpeg_decode_handle = NULL;

    if (g_dec_out_frame) {
        os_free(g_dec_out_frame);
        g_dec_out_frame = NULL;
    }

    return BK_OK;
}

static inline uint16_t bswap16_self(uint16_t x)
{
    uint32_t result;
    __asm__ volatile (
        "eor   %1, %1, %1, ror #16 \n"
        "mov   %1, %1, ror #8      \n"
        : "=r" (result)
        : "0" ((uint32_t)x << 16)
    );

    return (uint16_t)(result >> 16);
}

#if CONFIG_LVGL_V8
bk_err_t lv_jpeg_sw_decode_start(frame_buffer_t *jpeg_frame, lv_img_dsc_t *img_dst, bool byte_swap)
#else
bk_err_t lv_jpeg_sw_decode_start(frame_buffer_t *jpeg_frame, lv_image_dsc_t *img_dst, bool byte_swap)
#endif
{
    bk_err_t ret = BK_FAIL;
    bk_jpeg_decode_img_info_t img_info = {0};

    if (jpeg_frame == NULL) {
        LOGE("[%s][%d] jpeg_frame is null\r\n", __func__, __LINE__);
        return ret;
    }

    if (img_dst == NULL) {
        LOGE("[%s][%d] img_dst is null\r\n", __func__, __LINE__);
        return ret;
    }

    img_info.frame = jpeg_frame;
    ret = bk_jpeg_decode_sw_get_img_info(lv_jpeg_decode_handle, &img_info);
    if (ret != BK_OK) {
        LOGE("[%s][%d] get img info failed, ret: %d\r\n", __func__, __LINE__, ret);
        return ret;
    }

#if CONFIG_LVGL_V8
    img_dst->header.always_zero = 0;
    img_dst->header.cf = LV_IMG_CF_TRUE_COLOR;
#else
    img_dst->header.cf = LV_COLOR_FORMAT_RGB565;
    img_dst->header.magic = LV_IMAGE_HEADER_MAGIC;
    img_dst->header.stride = img_info.width * 2;
#endif
    img_dst->header.w = img_info.width;
    img_dst->header.h = img_info.height;
    img_dst->data_size = img_info.width * img_info.height * 2;
    img_dst->data = psram_malloc(img_dst->data_size);
    if (!img_dst->data) {
        LOGE("[%s][%d] psram malloc fail\r\n", __FUNCTION__, __LINE__);
        ret = BK_FAIL;
        return ret;
    }

    g_dec_out_frame->frame = (uint8_t *)img_dst->data;
    g_dec_out_frame->size = img_dst->data_size;
    ret = bk_jpeg_decode_sw_decode(lv_jpeg_decode_handle, jpeg_frame, g_dec_out_frame);
    if (ret != BK_OK) {
        LOGE("[%s][%d] sw decoder error\r\n", __FUNCTION__, __LINE__);
        return ret;
    }

    if (byte_swap) {
        for (int i = 0; i < img_dst->data_size; i += 2) {
            uint16_t *p = (uint16_t *)&img_dst->data[i];
            *p = bswap16_self(*p);
        }
    }

    return ret;
}
