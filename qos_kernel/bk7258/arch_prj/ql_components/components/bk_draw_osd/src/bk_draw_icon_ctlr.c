#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/avdk_utils/avdk_check.h>
#include "modules/image_scale.h"
#include "modules/lcd_font.h"
#include "bk_draw_icon_ctlr.h"

#define TAG "draw_icon"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/**
 * @brief Check and allocate memory for icon buffer
 */
static avdk_err_t icon_check_mem(draw_icon_context_t *context, uint32_t icon_x, uint32_t icon_y, uint8_t icon_rotate)
{
    static bool last_psram_setting = false;
    static bool initialized = false;
    uint32_t required_size = icon_x * icon_y * 2;

    // last_psram_setting
    if (!initialized) {
        last_psram_setting = context->draw_in_psram;
        initialized = true;
    }

    // 如果draw_in_psram标志变化或者需要更大的内存，则重新分配
    if (required_size > context->buf1.size || context->draw_in_psram != last_psram_setting) {
        // 更新上次使用的PSRAM设置
        last_psram_setting = context->draw_in_psram;
        context->buf1.size = required_size;

        if (context->buf1.addr != NULL) {
            os_free(context->buf1.addr);
            context->buf1.addr = NULL;
        }

        if (context->draw_in_psram) {
            context->buf1.addr = (uint8_t *)psram_malloc(context->buf1.size);
        } else {
            context->buf1.addr = (uint8_t *)os_malloc(context->buf1.size);
        }

        if (context->buf1.addr == NULL) {
            LOGE("Malloc icon buffer failed, size: %d, use_psram: %d \n", context->buf1.size, context->draw_in_psram);
            return BK_FAIL;
        }
        
        LOGD("Malloc icon buffer1 size: %d, addr: %p-%p, is draw_in_psram: %d \n", 
             context->buf1.size, context->buf1.addr, (char*)(context->buf1.addr + context->buf1.size), context->draw_in_psram);

        if (icon_rotate == ROTATE_270) {
            if (context->buf2.addr != NULL) {
                os_free(context->buf2.addr);
            }
            context->buf2.size = required_size;
            if (context->draw_in_psram) {
                context->buf2.addr = (uint8_t *)psram_malloc(context->buf2.size);
            } else {
                context->buf2.addr = (uint8_t *)os_malloc(context->buf2.size);
            }
            if (context->buf2.addr == NULL) {
                LOGE("Malloc buf2 failed, size: %d, is draw_in_psram: %d \n", context->buf2.size, context->draw_in_psram);
                return BK_FAIL;
            }
        }
    }
    
    return BK_OK;
}

/**
 * @brief IO control function
 */
// Modify ioctl function to support OSD_CTLR_CMD_SET_PSRAM_USAGE command
avdk_err_t bk_draw_icon_ctlr_ioctl(bk_draw_icon_ctlr_t *controller, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    private_draw_icon_ctlr_t *icon_ctlr = __containerof(controller, private_draw_icon_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(icon_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL");

    // Execute different operations according to command type
    switch (ioctl_cmd) {
        case DRAW_ICON_CTLR_CMD_SET_PSRAM_USAGE:
            icon_ctlr->context.draw_in_psram = param1;
            break;
        default:
            LOGE("Unsupported ioctl command: %d", ioctl_cmd);
            return BK_ERR_PARAM;
    }
    return BK_OK;
}

/**
 * @brief Delete controller handle
 */
avdk_err_t bk_draw_icon_ctlr_delete(bk_draw_icon_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL\n");
    private_draw_icon_ctlr_t *icon_ctlr = __containerof(controller, private_draw_icon_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(icon_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL");
    
    /* Free buffer */
    if (icon_ctlr->context.buf1.addr != NULL) {
        os_free(icon_ctlr->context.buf1.addr);
        icon_ctlr->context.buf1.addr = NULL;
    }
    icon_ctlr->context.buf1.size = 0;

    /* Free buffer 2 */
    if (icon_ctlr->context.buf2.addr != NULL) {
        os_free(icon_ctlr->context.buf2.addr);
        icon_ctlr->context.buf2.addr = NULL;
    }
    icon_ctlr->context.buf2.size = 0;

    /* Free controller handle */
    os_free(icon_ctlr);
    
    LOGD("Delete draw icon controller success");
    return BK_OK;
}

// Modify bk_draw_icon_ctlr_draw_image function to use draw_in_psram variable in cfg
avdk_err_t bk_draw_icon_ctlr_draw_image(bk_draw_icon_ctlr_t *controller, icon_image_blend_cfg_t *cfg)
{
    private_draw_icon_ctlr_t *icon_ctlr = __containerof(controller, private_draw_icon_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(icon_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(cfg, AVDK_ERR_INVAL, TAG, "config is NULL");

    /* Check memory */
    if (icon_check_mem(&icon_ctlr->context, cfg->xsize, cfg->ysize, cfg->blend_rotate) != BK_OK) {
        LOGE("Check memory failed");
        // Restore original settings
        return BK_FAIL;
    }

    int i = 0;
    uint16_t start_x_pos = 0;
    uint16_t start_y_pos = 0;
    uint8_t pixel_bit;
    uint8_t *p_yuv_src_temp = NULL;
    uint8_t *p_yuv_src = NULL;
    uint8_t *p_yuv_dst = NULL;
    uint8_t *p_yuv_temp = NULL;
    pixel_format_t bg_fmt = cfg->bg_data_format;

    /* Compute LCD start position */
    if ((cfg->visible_width < cfg->bg_width) || (cfg->visible_height < cfg->bg_height)) {
        if (cfg->visible_width < cfg->bg_width) {
            start_x_pos = (cfg->bg_width - cfg->visible_width) / 2;
        }
        if (cfg->visible_height < cfg->bg_height) {
            start_y_pos = (cfg->bg_height - cfg->visible_height) / 2;
        }
    }
    
    /* Set pixel bit */
    if (cfg->bg_data_format == PIXEL_FMT_RGB888) {
        pixel_bit = 3;
    } else {
        pixel_bit = 2;
    }
    
    /* Compute source address */
    p_yuv_src_temp = (uint8_t *)(cfg->pbg_addr + (((start_y_pos + cfg->ypos) * cfg->bg_width) + (start_x_pos + cfg->xpos)) * pixel_bit);
    p_yuv_src = p_yuv_src_temp;
    p_yuv_dst = icon_ctlr->context.buf1.addr;
    p_yuv_temp = icon_ctlr->context.buf2.addr;
    
    /* Step 1: Copy background YUV data */
    // For RGB565 format, pixel_bit=2, width may be odd, so bytes may not be 4-byte aligned
    // Use byte pointer instead of uint32_t* to avoid alignment issues
    for (i = 0; i < cfg->ysize; i++) {
        os_memcpy(p_yuv_dst, p_yuv_src, cfg->xsize * pixel_bit);
        p_yuv_dst += (cfg->xsize * pixel_bit);
        p_yuv_src += (cfg->bg_width * pixel_bit);
    }
    p_yuv_dst = icon_ctlr->context.buf1.addr;
    
    /* If need to rotate 270 degrees */
    if (cfg->blend_rotate == ROTATE_270) {
        p_yuv_dst = icon_ctlr->context.buf1.addr;
        
        if (PIXEL_FMT_VUYY == bg_fmt) {
            // Rotate area
            vuyy_rotate_degree90_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_temp, cfg->xsize, cfg->ysize);
        } else if (PIXEL_FMT_YUYV == bg_fmt) {
            yuyv_rotate_degree90_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_temp, cfg->xsize, cfg->ysize);
        }
        
        // Swap width and height
        i = cfg->xsize;
        cfg->xsize = cfg->ysize;
        cfg->ysize = i;
        bg_fmt = PIXEL_FMT_YUYV;
        p_yuv_dst = icon_ctlr->context.buf2.addr;
    }
    
    /* Step 2: Blend foreground image to background */
    if (PIXEL_FMT_VUYY == bg_fmt) {
        argb8888_to_vuyy_blend((uint8_t *)cfg->pfg_addr, p_yuv_dst, cfg->xsize, cfg->ysize);
    } else if (PIXEL_FMT_YUYV == bg_fmt) {
        argb8888_to_yuyv_blend((uint8_t *)cfg->pfg_addr, p_yuv_dst, cfg->xsize, cfg->ysize);
    } else if (PIXEL_FMT_RGB888 == bg_fmt) {
        argb8888_to_rgb888_blend((uint8_t *)cfg->pfg_addr, p_yuv_dst, cfg->xsize, cfg->ysize);
    } else if (PIXEL_FMT_RGB565_LE == bg_fmt){
        // PIXEL_FMT_RGB565_LE: pixel big endian format
        argb8888_to_rgb565_blend((uint8_t *)cfg->pfg_addr, p_yuv_dst, cfg->xsize, cfg->ysize);
    } else if (PIXEL_FMT_RGB565 == bg_fmt){
        // PIXEL_FMT_RGB565: pixel little endian format
        argb8888_to_rgb565le_blend((uint8_t *)cfg->pfg_addr, p_yuv_dst, cfg->xsize, cfg->ysize);
    } else {
        LOGE("bg_data_format is not support. only support PIXEL_FMT_YUYV, PIXEL_FMT_RGB888, PIXEL_FMT_RGB565\n");
        return AVDK_ERR_INVAL;
    }
    
    /* If need to rotate back 270 degrees */
    if (cfg->blend_rotate == ROTATE_270) {
        p_yuv_temp = icon_ctlr->context.buf1.addr;
        
        if (PIXEL_FMT_VUYY == cfg->bg_data_format) {
            // Rotate back
            yuyv_rotate_degree270_to_vuyy((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_temp, cfg->xsize, cfg->ysize);
        } else {
            yuyv_rotate_degree270_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_temp, cfg->xsize, cfg->ysize);
        }
        
        // Swap width and height
        i = cfg->xsize;
        cfg->xsize = cfg->ysize;
        cfg->ysize = i;
        p_yuv_dst = icon_ctlr->context.buf1.addr;
    }
    
    /* Step 3: Copy result back to background image */
    // For RGB565 format, pixel_bit=2, width may be odd, so bytes may not be 4-byte aligned
    // Use byte pointer instead of uint32_t* to avoid alignment issues
    p_yuv_src = p_yuv_src_temp;
    p_yuv_dst = icon_ctlr->context.buf1.addr;
    
    for (i = 0; i < cfg->ysize; i++) {
        os_memcpy(p_yuv_src, p_yuv_dst, cfg->xsize * pixel_bit);
        p_yuv_dst += (cfg->xsize * pixel_bit);
        p_yuv_src += (cfg->bg_width * pixel_bit);
    }
    return BK_OK;
}

/**
 * @brief Draw font
 */
avdk_err_t bk_draw_icon_ctlr_draw_font(bk_draw_icon_ctlr_t *controller, icon_font_blend_cfg_t *cfg)
{
    private_draw_icon_ctlr_t *icon_ctlr = __containerof(controller, private_draw_icon_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(icon_ctlr, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(cfg, AVDK_ERR_INVAL, TAG, "config is NULL");

    /* Check memory */
    if (icon_check_mem(&icon_ctlr->context, cfg->xsize, cfg->ysize, cfg->font_rotate) != BK_OK) {
        LOGE("Check memory failed");
        return BK_FAIL;
    }
    
    int ret = BK_OK;
    uint8_t pixel_bytes;
    uint16_t start_x_pos = 0;
    uint16_t start_y_pos = 0;
    register uint32_t i = 0;
    uint8_t *pbg_addr_temp = NULL;
    uint8_t *p_yuv_src = NULL;
    uint8_t *p_yuv_dst = NULL;
    uint8_t *p_yuv_rotate_temp = NULL;
    uint8_t *font_addr = NULL;

    /* Compute LCD start position */
    if ((cfg->visible_width < cfg->bg_width) || (cfg->visible_height < cfg->bg_height)) {
        if (cfg->visible_width < cfg->bg_width) {
            start_x_pos = (cfg->bg_width - cfg->visible_width) / 2;
        }
        if (cfg->visible_height < cfg->bg_height) {
            start_y_pos = (cfg->bg_height - cfg->visible_height) / 2;
        }
    }
    
    /* Set pixel byte number */
    if (cfg->bg_data_format == PIXEL_FMT_RGB888) {
        pixel_bytes = 3;
    } else {
        pixel_bytes = 2;
    }
    
    /* Compute background address */
    pbg_addr_temp = (uint8_t *)(cfg->pbg_addr + (((start_y_pos + cfg->ypos) * cfg->bg_width) + (start_x_pos + cfg->xpos)) * pixel_bytes);
    p_yuv_src = pbg_addr_temp;
    p_yuv_dst = icon_ctlr->context.buf1.addr;
    p_yuv_rotate_temp = icon_ctlr->context.buf2.addr;
    
    /* Copy background data to buffer */
    for (i = 0; i < cfg->ysize; i++) {
        os_memcpy((uint32_t *)p_yuv_dst, (uint32_t *)p_yuv_src, cfg->xsize * pixel_bytes);
        p_yuv_dst += (cfg->xsize * pixel_bytes);
        p_yuv_src += (cfg->bg_width * pixel_bytes);
    }
    p_yuv_dst = icon_ctlr->context.buf1.addr;
    
    /* If need to rotate 270 degrees */
    if (cfg->font_rotate == ROTATE_270) {
        if (PIXEL_FMT_VUYY == cfg->bg_data_format) {
            // Rotate area
            vuyy_rotate_degree90_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_rotate_temp, cfg->xsize, cfg->ysize);
        } else if (PIXEL_FMT_YUYV == cfg->bg_data_format) {
            yuyv_rotate_degree90_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_rotate_temp, cfg->xsize, cfg->ysize);
        }
        else{
            LOGE("bg_data_format is not support ROTATE_270. only support PIXEL_FMT_VUYY, PIXEL_FMT_YUYV\n");
            return AVDK_ERR_INVAL;
        }
        
        // Swap width and height
        i = cfg->xsize;
        cfg->xsize = cfg->ysize;
        cfg->ysize = i;
        cfg->font_format = FONT_YUYV;
        p_yuv_dst = icon_ctlr->context.buf2.addr;
    }
    
    /* Process font format */
    if (cfg->font_format == FONT_RGB565) {
        // Font RGB565 data to YUV background image
        font_addr = icon_ctlr->context.buf2.addr;

        if (PIXEL_FMT_VUYY == cfg->bg_data_format) {
            vuyy_to_rgb565_convert((unsigned char *)icon_ctlr->context.buf1.addr, (unsigned char *)icon_ctlr->context.buf2.addr, cfg->xsize, cfg->ysize);
        } else if (PIXEL_FMT_YUYV == cfg->bg_data_format) {
            yuyv_to_rgb565_convert((unsigned char *)icon_ctlr->context.buf1.addr, (unsigned char *)icon_ctlr->context.buf2.addr, cfg->xsize, cfg->ysize);
        } else if (PIXEL_FMT_RGB565_LE == cfg->bg_data_format) {
            // Background is already RGB565
            LOGI("bg_data_format is PIXEL_FMT_RGB565_LE, no need to convert\n");
            font_addr = icon_ctlr->context.buf1.addr;
            rgb565_to_rgb565le_convert(icon_ctlr->context.buf1.addr, cfg->xsize, cfg->ysize);
        } else if (PIXEL_FMT_RGB888 == cfg->bg_data_format) {
            rgb888_to_rgb565_convert(icon_ctlr->context.buf1.addr, (uint16_t *)icon_ctlr->context.buf2.addr, cfg->xsize, cfg->ysize);
        } else if (PIXEL_FMT_RGB565 == cfg->bg_data_format) {
            font_addr = icon_ctlr->context.buf1.addr;
        } else{
            LOGE("bg_data_format is not support. only support PIXEL_FMT_YUYV, PIXEL_FMT_RGB565_LE, PIXEL_FMT_RGB565, PIXEL_FMT_RGB888\n");
            return AVDK_ERR_INVAL;
        }
        
        // Draw font
        font_t font;
        font.info = (ui_display_info_struct){
            font_addr, 0, cfg->ysize, 0, {0}
        };
        font.width = cfg->xsize;
        font.height = cfg->ysize;
        font.font_fmt = cfg->font_format;
        
        for (int i = 0; i < cfg->str_num; i++) {
            font.digit_info = cfg->str[i].font_digit_type;
            font.s = cfg->str[i].str;
            font.font_color = cfg->str[i].font_color;
            font.x_pos = cfg->str[i].x_pos;
            font.y_pos = cfg->str[i].y_pos;
            lcd_draw_font(&font);
        }
        
        // Convert back to original format
        if (PIXEL_FMT_VUYY == cfg->bg_data_format) {
            rgb565_to_vuyy_convert((uint16_t *)icon_ctlr->context.buf2.addr, (uint16_t *)icon_ctlr->context.buf1.addr, cfg->xsize, cfg->ysize);
        } else if (PIXEL_FMT_YUYV == cfg->bg_data_format) {
            rgb565_to_yuyv_convert((uint16_t *)icon_ctlr->context.buf2.addr, (uint16_t *)icon_ctlr->context.buf1.addr, cfg->xsize, cfg->ysize);
        } else if (PIXEL_FMT_RGB565_LE == cfg->bg_data_format) {
            // Convert to RGB565_LE
            rgb565_to_rgb565le_convert(icon_ctlr->context.buf1.addr, cfg->xsize, cfg->ysize);
        } else if (PIXEL_FMT_RGB888 == cfg->bg_data_format) {
            rgb565_to_rgb888_convert((uint16_t *)icon_ctlr->context.buf2.addr, icon_ctlr->context.buf1.addr, cfg->xsize, cfg->ysize);
        }
    } else {
        // Font YUV data to YUV background image
        font_t font;
        font.info = (ui_display_info_struct){
            (unsigned char *)p_yuv_dst, 0, cfg->ysize, 0, {0}
        };
        font.width = cfg->xsize;
        font.height = cfg->ysize;
        font.font_fmt = cfg->font_format;
        
        for (int i = 0; i < cfg->str_num; i++) {
            font.digit_info = cfg->str[i].font_digit_type;
            font.s = cfg->str[i].str;
            font.font_color = cfg->str[i].font_color;
            font.x_pos = cfg->str[i].x_pos;
            font.y_pos = cfg->str[i].y_pos;
            lcd_draw_font(&font);
        }
    }

    /* If need to rotate back 270 degrees */
    if (cfg->font_rotate == ROTATE_270) {
        p_yuv_rotate_temp = icon_ctlr->context.buf1.addr;
        
        if (PIXEL_FMT_VUYY == cfg->bg_data_format) {
            yuyv_rotate_degree270_to_vuyy((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_rotate_temp, cfg->xsize, cfg->ysize);
        } else {
            yuyv_rotate_degree270_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_rotate_temp, cfg->xsize, cfg->ysize);
        }
        
        // Swap width and height
        i = cfg->xsize;
        cfg->xsize = cfg->ysize;
        cfg->ysize = i;
    }
    
    /* Copy result back to background image */
    p_yuv_src = icon_ctlr->context.buf1.addr;
    p_yuv_dst = pbg_addr_temp;
    
    for (i = 0; i < cfg->ysize; i++) {
        os_memcpy((uint32_t *)p_yuv_dst, (uint32_t *)p_yuv_src, cfg->xsize * pixel_bytes);
        p_yuv_src += (cfg->xsize * pixel_bytes);
        p_yuv_dst += (cfg->bg_width * pixel_bytes);
    }
    return ret;
}


/**
 * @brief Create icon controller handle
 */
avdk_err_t bk_draw_icon_ctlr_new(bk_draw_icon_ctlr_handle_t *handle, icon_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    private_draw_icon_ctlr_t *controller = os_malloc(sizeof(private_draw_icon_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_draw_icon_ctlr_t));

    controller->context.draw_in_psram = config->draw_in_psram;

    controller->ops.draw_font = bk_draw_icon_ctlr_draw_font;
    controller->ops.draw_image = bk_draw_icon_ctlr_draw_image;
    controller->ops.delete = bk_draw_icon_ctlr_delete;
    controller->ops.ioctl = bk_draw_icon_ctlr_ioctl;

    *handle = &(controller->ops);

    LOGI(TAG, "create dma2d controller success");
    return AVDK_ERR_OK;
}
