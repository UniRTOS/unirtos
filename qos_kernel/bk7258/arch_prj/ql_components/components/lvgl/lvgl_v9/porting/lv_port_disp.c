/**
 * @file lv_port_disp.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_port_disp.h"
#include <modules/image_scale.h>
#include "lv_vendor.h"
#include "frame_buffer.h"

#include <driver/lcd_types.h>
#include "lv_copy_method.h"

#define TAG "LVGL_DISP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_deinit(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
extern lv_vnd_config_t vendor_config;
extern media_debug_t *media_debug;
static void *rotate_buffer = NULL;
static frame_buffer_t *disp_buf = NULL;
static frame_buffer_t *copy_buf = NULL;
static bool lv_new_frame_flag = true;
static beken_semaphore_t lv_disp_sem = NULL;


void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = NULL;
    disp = lv_display_create(vendor_config.width, vendor_config.height);

    #if (CONFIG_LV_COLOR_DEPTH == 16 && CONFIG_LV_COLOR_16_SWAP)
        lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
    #endif

    lv_display_set_flush_cb(disp, disp_flush);

    lv_display_set_rotation(disp, vendor_config.rotation);

#if 0
    /* Example 1
     * One buffer for partial rendering*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_1_1[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];            /*A buffer for 10 rows*/
    lv_display_set_buffers(disp, buf_1_1, NULL, sizeof(buf_1_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 2
     * Two buffers for partial rendering
     * In flush_cb DMA or similar hardware should be used to update the display in the background.*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_1[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_2[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];
    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 3
     * Two buffers screen sized buffer for double buffering.
     * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];
#else
    if (vendor_config.render_mode == RENDER_PARTIAL_MODE) {
        lv_display_set_buffers(disp, vendor_config.draw_buf_2_1, vendor_config.draw_buf_2_2, vendor_config.draw_pixel_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    } else if (vendor_config.render_mode == RENDER_DIRECT_MODE) {
        lv_display_set_buffers(disp, vendor_config.draw_buf_2_1, vendor_config.draw_buf_2_2, vendor_config.draw_pixel_size, LV_DISPLAY_RENDER_MODE_DIRECT);
    } else {
        lv_display_set_buffers(disp, vendor_config.draw_buf_2_1, vendor_config.draw_buf_2_2, vendor_config.draw_pixel_size, LV_DISPLAY_RENDER_MODE_FULL);
    }

    LOGI("LVGL addr1:%x, addr2:%x, pixel size:%d, fb1:%x, fb2:%x\r\n", vendor_config.draw_buf_2_1, vendor_config.draw_buf_2_2,
                                        vendor_config.draw_pixel_size, vendor_config.frame_buffer[0], vendor_config.frame_buffer[1]);

    if (vendor_config.render_mode == RENDER_PARTIAL_MODE && vendor_config.rotation != ROTATE_NONE) {
        rotate_buffer = os_malloc(vendor_config.draw_pixel_size);
        if (rotate_buffer == NULL) {
            LOGE("%s lvgl rotate buffer malloc fail!\n", __func__);
            return;
        }
    }
#endif
}

void lv_port_disp_deinit(void)
{
    if (vendor_config.render_mode == RENDER_PARTIAL_MODE && vendor_config.rotation != ROTATE_NONE) {
        if (rotate_buffer) {
            os_free(rotate_buffer);
            rotate_buffer = NULL;
        }
    }

    lv_display_delete(lv_disp_get_default());

    disp_deinit();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void lv_memcpy_one_line(void *dest_buf, const void *src_buf, uint32_t point_num)
{
    os_memcpy(dest_buf, src_buf, point_num * sizeof(bk_color_t));
}

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
    if (vendor_config.render_mode != RENDER_PARTIAL_MODE) {
        bk_err_t ret = rtos_init_semaphore_ex(&lv_disp_sem, 1, 0);
        if (BK_OK != ret) {
            LOGE("%s lv_disp_sem init failed\n", __func__);
            return;
        }
    } else {
        #if CONFIG_LV_FRAME_DMA2D_COPY
            lv_dma2d_memcpy_init();
        #else
            lv_dma_memcpy_init();
            if (vendor_config.draw_buf_2_2 != NULL) {
                lv_dma2d_memcpy_init();
            }
        #endif
    }
}

static void disp_deinit(void)
{
    if (vendor_config.render_mode != RENDER_PARTIAL_MODE) {
        bk_err_t ret = rtos_deinit_semaphore(&lv_disp_sem);
        if (BK_OK != ret) {
            LOGE("%s lv_disp_sem deinit failed\n", __func__);
            return;
        }
    } else {
        #if CONFIG_LV_FRAME_DMA2D_COPY
            lv_dma2d_memcpy_deinit();
        #else
            lv_dma_memcpy_deinit();
            if (vendor_config.draw_buf_2_2 != NULL) {
                lv_dma2d_memcpy_deinit();
            }
        #endif
    }
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

static bk_err_t lvgl_frame_buffer_free_cb(void *frame)
{
    if (vendor_config.render_mode != RENDER_PARTIAL_MODE) {
        rtos_set_semaphore(&lv_disp_sem);
    } else {
        lv_vendor_set_ready_frame_buffer(frame);
    }

    return BK_OK;
}

static void lv_disp_flush_for_partial_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    lv_coord_t lv_hor = 0;
    lv_coord_t lv_ver = 0;

    if (vendor_config.rotation == ROTATE_NONE || vendor_config.rotation == ROTATE_180) {
        lv_hor = LV_HOR_RES;
        lv_ver = LV_VER_RES;
    } else {
        lv_hor = LV_VER_RES;
        lv_ver = LV_HOR_RES;
    }

    bk_color_t *color_ptr = NULL;
    int y = 0, offset = 0;

    lv_coord_t width = lv_area_get_width(area);
    lv_coord_t height = lv_area_get_height(area);

    if (CONFIG_LVGL_FRAME_BUFFER_NUM > 1) {
        #if CONFIG_LV_FRAME_DMA2D_COPY
            lv_dma2d_memcpy_wait_transfer_finish();
        #else
            lv_dma_memcpy_wait_transfer_finish();
        #endif

        if (lv_new_frame_flag) {
            if (disp_buf == NULL) {
                do {
                    disp_buf = lv_vendor_get_ready_frame_buffer();
                    if (disp_buf != NULL) {
                        break;
                    }
                } while (disp_buf == NULL);
            } else {
                disp_buf = copy_buf;
                copy_buf = NULL;
            }
            lv_new_frame_flag = false;
        }
    } else {
        if (lv_new_frame_flag) {
            disp_buf = vendor_config.frame_buffer[0];
            lv_new_frame_flag = false;
        }
    }

    if (vendor_config.rotation != ROTATE_NONE) {
        lv_color_format_t cf = lv_display_get_color_format(disp_drv);
        uint32_t w_stride = lv_draw_buf_width_to_stride(width, cf);
        uint32_t h_stride = lv_draw_buf_width_to_stride(height, cf);

        if (vendor_config.rotation == ROTATE_180) {
            lv_draw_sw_rotate(px_map, rotate_buffer, width, height, w_stride, w_stride, vendor_config.rotation, cf);
        } else {
            lv_draw_sw_rotate(px_map, rotate_buffer, width, height, w_stride, h_stride, vendor_config.rotation, cf);
        }
        color_ptr = rotate_buffer;

        lv_area_t rotated_area = *area;
        lv_display_rotate_area(disp_drv, &rotated_area);
        area = &rotated_area;

        if (vendor_config.rotation != ROTATE_180) {
            width = lv_area_get_width(area);
            height = lv_area_get_height(area);
        }
    } else {
        color_ptr = (bk_color_t *)px_map;
    }

    if (vendor_config.draw_buf_2_2) {
        lv_dma2d_memcpy_double_draw_buffer(color_ptr, width, height, disp_buf->frame, area->x1, area->y1);
    } else {
        offset = area->y1 * lv_hor + area->x1;
        for (y = area->y1; y <= area->y2; y++) {
            lv_memcpy_one_line(disp_buf->frame + offset * sizeof(bk_color_t), color_ptr, width);
            offset += lv_hor;
            color_ptr += width;
        }
    }

    if (lv_disp_flush_is_last(disp_drv)) {
        media_debug->lvgl_draw++;
        #if (!CONFIG_LV_USE_DEMO_BENCHMARK)
            if (vendor_config.draw_buf_2_2) {
                lv_dma2d_memcpy_wait_transfer_finish();
            }
        #endif

        bk_display_flush(vendor_config.handle, disp_buf, lvgl_frame_buffer_free_cb);
        lv_new_frame_flag = true;

        if (CONFIG_LVGL_FRAME_BUFFER_NUM > 1) {
            if (copy_buf == NULL) {
                do {
                    copy_buf = lv_vendor_get_ready_frame_buffer();
                    if (copy_buf != NULL) {
                        break;
                    }
                } while(copy_buf == NULL);
            }

            #if CONFIG_LV_FRAME_DMA2D_COPY
                lv_dma2d_memcpy_last_frame(disp_buf->frame, copy_buf->frame, lv_hor, lv_ver, 0, 0);
            #else
                lv_dma_memcpy_last_frame(disp_buf->frame, copy_buf->frame, lv_hor, lv_ver);
            #endif
        }
    }
}

static void lv_disp_flush_for_direct_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    static bool first_flush = true;

    if (lv_disp_flush_is_last(disp_drv)) {
        media_debug->lvgl_draw++;
        if (px_map == vendor_config.draw_buf_2_1) {
            bk_display_flush(vendor_config.handle, vendor_config.frame_buffer[0], lvgl_frame_buffer_free_cb);
        } else {
            bk_display_flush(vendor_config.handle, vendor_config.frame_buffer[1], lvgl_frame_buffer_free_cb);
        }
        if (first_flush) {
            first_flush = false;
        } else {
            bk_err_t ret = rtos_get_semaphore(&lv_disp_sem, 1000);
            if (ret != BK_OK) {
                LOGE("%s rtos_get_semaphore failed\n", __func__);
            }
        }
    }
}

static void lv_disp_flush_for_full_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    media_debug->lvgl_draw++;

    if (px_map == vendor_config.draw_buf_2_1) {
        bk_display_flush(vendor_config.handle, vendor_config.frame_buffer[0], lvgl_frame_buffer_free_cb);
    } else {
        bk_display_flush(vendor_config.handle, vendor_config.frame_buffer[1], lvgl_frame_buffer_free_cb);
    }   
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    if (disp_flush_enabled) {
        if (vendor_config.render_mode == RENDER_PARTIAL_MODE) {
            lv_disp_flush_for_partial_mode(disp_drv, area, px_map);
        } else if (vendor_config.render_mode == RENDER_DIRECT_MODE) {
            lv_disp_flush_for_direct_mode(disp_drv, area, px_map);
        } else {
            lv_disp_flush_for_full_mode(disp_drv, area, px_map);
        }
    }

    lv_disp_flush_ready(disp_drv);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
