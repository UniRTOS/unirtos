#include "os/os.h"
#include "os/mem.h"
#include "driver/lcd.h"
#include "components/media_types.h"
#include "modules/jpeg_decode_sw.h"
#include "driver/psram.h"
#include "lv_jpeg_hw_decode.h"
#include "lv_jpeg_sw_decode.h"
#include "lvgl.h"
#include "bk_posix.h"
#if CONFIG_LVGL_V9
#include "lvgl_private.h"
#endif


#define TAG "lv_img_utility"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


static bk_err_t lv_img_read_file_to_mem(char *filename, uint32 *paddr)
{
    uint8 *sram_addr = NULL;
    uint32 once_read_len = 1024 * 4;
    int fd = -1;
    int read_len = 0;
    bk_err_t ret = BK_FAIL;

    do {
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            LOGE("[%s][%d] open fail:%s\r\n", __FUNCTION__, __LINE__, filename);
            break;
        }

        sram_addr = os_malloc(once_read_len);
        if (sram_addr == NULL) {
            LOGE("[%s][%d] malloc fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        while(1)
        {
            read_len = read(fd, sram_addr, once_read_len);
            if (read_len < 0) {
                LOGD("[%s][%d] read file fail.\r\n", __FUNCTION__, __LINE__);
                break;
            }

            if (read_len == 0) {
                ret = BK_OK;
                break;
            }

            if (once_read_len != read_len) {
                if (read_len % 4) {
                    read_len = (read_len / 4 + 1) * 4;
                }
                bk_psram_word_memcpy(paddr, sram_addr, read_len);
            } else {
                bk_psram_word_memcpy(paddr, sram_addr, once_read_len);
                paddr += (once_read_len / 4);
            }
        }
    } while(0);

    if (sram_addr) {
        os_free(sram_addr);
        sram_addr = NULL;
    }

    if (fd > 0) {
        close(fd);
    }

    return ret;
}

int lv_img_get_filelen(char *filename)
{
    int ret = BK_FAIL;
    struct stat statbuf;

    do {
        if (!filename) {
            LOGE("[%s][%d]param is null.\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        ret = stat(filename, &statbuf);
        if (BK_OK != ret) {
            LOGE("[%s][%d] sta fail:%s\r\n", __FUNCTION__, __LINE__, filename);
            break;
        }

        ret = statbuf.st_size;
        LOGD("[%s][%d] %s size:%d\r\n", __FUNCTION__, __LINE__, filename, ret);
    } while(0);

    return ret;
}

static frame_buffer_t *lv_img_read_file(char *file_name)
{
    frame_buffer_t *jpeg_frame = NULL;
    int file_len = 0;
    int ret = 0;

    do {
        file_len = lv_img_get_filelen(file_name);
        if (file_len <= 0) {
            LOGE("[%s][%d] %s don't exit in fatfs\r\n", __FUNCTION__, __LINE__, file_name);
            break;
        }

        jpeg_frame = os_malloc(sizeof(frame_buffer_t));
        if (!jpeg_frame) {
            LOGE("[%s][%d] malloc fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        memset(jpeg_frame, 0, sizeof(frame_buffer_t));
        jpeg_frame->frame = psram_malloc(file_len);
        jpeg_frame->length = file_len;
        if (!jpeg_frame->frame) {
            os_free(jpeg_frame);
            jpeg_frame = NULL;
            LOGE("[%s][%d] psram malloc fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        ret = lv_img_read_file_to_mem((char *)file_name, (uint32 *)jpeg_frame->frame);
        if (BK_OK != ret) {
            psram_free(jpeg_frame->frame);
            jpeg_frame->frame = NULL;

            os_free(jpeg_frame);
            jpeg_frame = NULL;
        }
    } while(0);

    return jpeg_frame;
}

#if CONFIG_LVGL_V8
static bk_err_t lv_img_file_jpeg_sw_dec(char *file_name, lv_img_dsc_t *img_dst, bool byte_swap)
#else
static bk_err_t lv_img_file_jpeg_sw_dec(char *file_name, lv_image_dsc_t *img_dst, bool byte_swap)
#endif
{
    int ret = BK_FAIL;
    frame_buffer_t *jpeg_frame = NULL;

    do {
        jpeg_frame = lv_img_read_file(file_name);
        if (jpeg_frame == NULL) {
            LOGE("[%s][%d]jpeg_frame is null\r\n", __FUNCTION__, __LINE__);
            break;
        }

        ret = lv_jpeg_sw_decode_start(jpeg_frame, img_dst, byte_swap);
        if (BK_OK == ret) {
            LOGD("[%s][%d] decode success, width:%d, height:%d, size:%d\r\n", __FUNCTION__, __LINE__,
                                            img_dst->header.w, img_dst->header.h, img_dst->data_size);
        }
    } while(0);

    if (jpeg_frame) {
        if (jpeg_frame->frame) {
            psram_free(jpeg_frame->frame);
            jpeg_frame->frame = NULL;
        }

        os_free(jpeg_frame);
        jpeg_frame = NULL;
    }

    return ret;
}

#if CONFIG_LVGL_V8
static bk_err_t lv_img_file_jpeg_hw_dec(char *file_name, lv_img_dsc_t *img_dst, bool byte_swap)
#else
static bk_err_t lv_img_file_jpeg_hw_dec(char *file_name, lv_image_dsc_t *img_dst, bool byte_swap)
#endif
{
    int ret = BK_FAIL;
    frame_buffer_t *jpeg_frame = NULL;

    do {
        jpeg_frame = lv_img_read_file(file_name);
        if (jpeg_frame == NULL) {
            LOGE("[%s][%d]jpeg_frame is null\r\n", __FUNCTION__, __LINE__);
            break;
        }

        sw_jpeg_dec_res_t result;
        ret = bk_jpeg_get_img_info(jpeg_frame->length, jpeg_frame->frame, &result, NULL);
        if (ret != BK_OK) {
            LOGE("[%s][%d] get img info fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }

#if CONFIG_LVGL_V8
        img_dst->header.always_zero = 0;
        img_dst->header.cf = LV_IMG_CF_TRUE_COLOR;
#else
        img_dst->header.cf = LV_COLOR_FORMAT_RGB565;
        img_dst->header.magic = LV_IMAGE_HEADER_MAGIC;
        img_dst->header.stride = result.pixel_x * 2;
#endif
        img_dst->header.w = result.pixel_x;
        img_dst->header.h = result.pixel_y;
        img_dst->data_size = img_dst->header.w * img_dst->header.h * 2;
        img_dst->data = psram_malloc(img_dst->data_size);
        if (!img_dst->data) {
            LOGE("[%s][%d] psram malloc fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        ret = lv_jpeg_hw_decode_init(img_dst->header.w);
        if (ret != BK_OK) {
            LOGE("[%s][%d] lv_jpeg_hw_decode_init fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        ret = lv_jpeg_hw_decode_start(jpeg_frame, img_dst, byte_swap);
        if (BK_OK == ret) {
            LOGD("[%s][%d] hw decode success, width:%d, height:%d, size:%d\r\n", __FUNCTION__, __LINE__,
                                                img_dst->header.w, img_dst->header.h, img_dst->data_size);
        } else {
            LOGE("[%s][%d] hw decode fail\r\n", __FUNCTION__, __LINE__);
        }

        ret = lv_jpeg_hw_decode_deinit();
        if (ret != BK_OK) {
            LOGE("[%s][%d] lv_jpeg_hw_decode_deinit fail\r\n", __FUNCTION__, __LINE__);
            break;
        }
    } while(0);

    if (jpeg_frame) {
        if (jpeg_frame->frame) {
            psram_free(jpeg_frame->frame);
            jpeg_frame->frame = NULL;
        }

        os_free(jpeg_frame);
        jpeg_frame = NULL;
    }

    return ret;
}

#if CONFIG_LVGL_V8
bk_err_t lv_jpeg_img_load_with_sw_dec(char *filename, lv_img_dsc_t *img_dst, bool byte_swap)
#else
bk_err_t lv_jpeg_img_load_with_sw_dec(char *filename, lv_image_dsc_t *img_dst, bool byte_swap)
#endif
{
    int ret = BK_FAIL;

    do {
        if (!filename || !img_dst) {
            LOGE("[%s][%d]filename or img_dst is null\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_NULL_PARAM;
            break;
        }

        ret = lv_jpeg_sw_decode_init();
        if (ret != BK_OK) {
            LOGE("[%s][%d] lv_jpeg_sw_decode_init fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        ret = lv_img_file_jpeg_sw_dec(filename, img_dst, byte_swap);
        if (ret != BK_OK) {
            LOGE("%s jpeg sw decode fail\r\n", __func__);
            lv_jpeg_sw_decode_deinit();
            break;
        }

        ret = lv_jpeg_sw_decode_deinit();
        if (ret != BK_OK) {
            LOGE("[%s][%d] lv_jpeg_sw_decode_deinit fail\r\n", __FUNCTION__, __LINE__);
            break;
        }
    } while(0);

    return ret;
}

#if CONFIG_LVGL_V8
bk_err_t lv_jpeg_img_load_with_hw_dec(char *filename, lv_img_dsc_t *img_dst, bool byte_swap)
#else
bk_err_t lv_jpeg_img_load_with_hw_dec(char *filename, lv_image_dsc_t *img_dst, bool byte_swap)
#endif
{
    int ret = BK_FAIL;

    do {
        if (!filename || !img_dst) {
            LOGE("[%s][%d]filename or img_dst is null\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_NULL_PARAM;
            break;
        }

        ret = lv_img_file_jpeg_hw_dec(filename, img_dst, byte_swap);
        if (ret != BK_OK) {
            LOGE("%s jpeg hw decode fail\r\n", __func__);
            break;
        }
    } while(0);

    return ret;
}

#if CONFIG_LVGL_V8
bk_err_t lv_png_img_load(char *filename, lv_img_dsc_t *img_dst)
#else
bk_err_t lv_png_img_load(char *filename, lv_image_dsc_t *img_dst)
#endif
{
    int ret = BK_FAIL;

    if (!filename || !img_dst) {
        ret = BK_ERR_NULL_PARAM;
        LOGE("[%s][%d]param invalid\r\n", __FUNCTION__, __LINE__);
        return ret;
    }

#if CONFIG_LVGL_V8
    lv_img_decoder_dsc_t img_decoder_dsc;

    os_memset(img_dst, 0, sizeof(lv_img_dsc_t));
    os_memset((char *)&img_decoder_dsc, 0, sizeof(lv_img_decoder_dsc_t));
    img_decoder_dsc.src_type = LV_IMG_SRC_FILE;
    ret = lv_img_decoder_open(&img_decoder_dsc, filename, img_decoder_dsc.color, img_decoder_dsc.frame_id);
    if (ret != LV_RES_OK) {
        LOGE("[%s][%d] decoder open fail:%d\r\n", __FUNCTION__, __LINE__, ret);
        ret = BK_FAIL;
        return ret;
    }

    memcpy(&img_dst->header, &img_decoder_dsc.header, sizeof(lv_img_header_t));
    img_dst->data_size = img_decoder_dsc.header.w * img_decoder_dsc.header.h * 4;
    img_dst->data = img_decoder_dsc.img_data;
    lv_mem_free((void *)img_decoder_dsc.src);
#else
    lv_image_decoder_dsc_t img_decoder_dsc;
    const lv_draw_buf_t *decoded = NULL;

    os_memset(img_dst, 0, sizeof(lv_image_dsc_t));
    os_memset((char *)&img_decoder_dsc, 0, sizeof(lv_image_decoder_dsc_t));

    ret = lv_image_decoder_open(&img_decoder_dsc, filename, NULL);
    if (ret != LV_RESULT_OK) {
        LOGE("[%s][%d] decoder open fail:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    decoded = img_decoder_dsc.decoded;
    if (decoded == NULL || decoded->data == NULL) {
        LOGE("[%s][%d] decoded data is null\r\n", __FUNCTION__, __LINE__);
        lv_image_decoder_close(&img_decoder_dsc);
        ret = BK_FAIL;
        return ret;
    }

    img_dst->header.cf = decoded->header.cf;
    img_dst->header.magic = decoded->header.magic;
    img_dst->header.w = decoded->header.w;
    img_dst->header.h = decoded->header.h;
    img_dst->data_size = decoded->data_size;
    img_dst->data = decoded->data;

    LOGD("[%s][%d] decode success, width:%d, height:%d, size:%d\r\n", __FUNCTION__, __LINE__,
         img_dst->header.w, img_dst->header.h, img_dst->data_size);
#endif

    return ret;
}

#if CONFIG_LVGL_V8
void lv_img_decode_unload(lv_img_dsc_t *img_dst)
#else
void lv_img_decode_unload(lv_image_dsc_t *img_dst)
#endif
{
    if (img_dst) {
        if (img_dst->data) {
            psram_free((void *)img_dst->data);
            img_dst->data = NULL;
        }
    }
}

