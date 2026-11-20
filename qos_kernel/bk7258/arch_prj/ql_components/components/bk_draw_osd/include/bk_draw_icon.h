#ifndef __BK_DRAW_ICON_H__
#define __BK_DRAW_ICON_H__

#include <components/media_types.h>
#include "modules/lcd_font.h"

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************
 * component name: bk_draw_icon
 * description: Private API (internal interface)
 ******************************************************************/

/* 图像混合配置结构体 */
typedef struct {
    uint8_t *pfg_addr;         /* 前景图像地址 */
    uint8_t *pbg_addr;         /* 背景图像地址 */
    uint32_t xsize;            /* 前景图像宽度 */
    uint32_t ysize;            /* 前景图像高度 */
    uint32_t bg_width;         /* 背景图像宽度 */
    uint32_t bg_height;        /* 背景图像高度 */
    uint32_t visible_width;        /* 可见宽度 */
    uint32_t visible_height;       /* 可见高度 */
    uint16_t xpos;             /* 绘制起始X坐标 */
    uint16_t ypos;             /* 绘制起始Y坐标 */
    pixel_format_t bg_data_format; /* 背景数据格式 PIXEL_FMT_YUYV, PIXEL_FMT_RGB565_LE,PIXEL_FMT_RGB565, PIXEL_FMT_RGB888 */
    media_rotate_t blend_rotate;  /* 旋转角度 only support ROTATE_270*/
} icon_image_blend_cfg_t;


/* 字符串绘制配置 */
typedef struct {
    const char *str;                     /* 字符串内容 */
    uint32_t font_color;           /* 字体颜色 */
    const gui_font_digit_struct *font_digit_type; /* 字体数字类型 */
    uint16_t x_pos;                /* X坐标 */
    uint16_t y_pos;                /* Y坐标 */
} icon_font_str_cfg_t;

/* 字体混合配置结构体 */
typedef struct {
    uint8_t *pbg_addr;             /* 背景图像地址 */
    uint32_t xsize;                /* 绘制区域宽度 */
    uint32_t ysize;                /* 绘制区域高度 */
    uint32_t bg_width;             /* 背景图像宽度 */
    uint32_t bg_height;            /* 背景图像高度 */
    uint32_t visible_width;            /* 可见宽度 */
    uint32_t visible_height;           /* 可见高度 */
    uint16_t xpos;                 /* 绘制起始X坐标 */
    uint16_t ypos;                 /* 绘制起始Y坐标 */
    pixel_format_t bg_data_format; /* 背景数据格式 */
    font_format_t font_format;     /* 字体格式 */
    media_rotate_t font_rotate;   /* 字体旋转角度 only support PIXEL_FMT_YUYV bg data*/
    icon_font_str_cfg_t str[2];       /* 字符串配置数组 */
    uint8_t str_num;               /* 字符串数量 */
} icon_font_blend_cfg_t;

/* 内存配置结构体 */
typedef struct {
    bool draw_in_psram; /* true: 使用PSRAM, false: 使用SRAM */

} icon_ctlr_config_t;

typedef enum {
    DRAW_ICON_CTLR_CMD_SET_PSRAM_USAGE = 0,
} draw_icon_ctlr_cmd_t;

/* 控制器句柄前向声明 */
typedef struct bk_draw_icon_ctlr_t *bk_draw_icon_ctlr_handle_t;
typedef struct bk_draw_icon_ctlr_t bk_draw_icon_ctlr_t;

struct bk_draw_icon_ctlr_t
{
    avdk_err_t (*delete)(bk_draw_icon_ctlr_t *controller);
    avdk_err_t (*draw_image)(bk_draw_icon_ctlr_t *controller, icon_image_blend_cfg_t *cfg);
    avdk_err_t (*draw_font)(bk_draw_icon_ctlr_t *controller, icon_font_blend_cfg_t *cfg);
    avdk_err_t (*ioctl)(bk_draw_icon_ctlr_t *controller, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3);
};


avdk_err_t bk_draw_icon_delete(bk_draw_icon_ctlr_handle_t handle);
avdk_err_t bk_draw_icon_image(bk_draw_icon_ctlr_handle_t handle, icon_image_blend_cfg_t *cfg);
avdk_err_t bk_draw_icon_font(bk_draw_icon_ctlr_handle_t handle, icon_font_blend_cfg_t *cfg);
avdk_err_t bk_draw_icon_ioctl(bk_draw_icon_ctlr_handle_t handle, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3);
avdk_err_t bk_draw_icon_new(bk_draw_icon_ctlr_handle_t *handle, icon_ctlr_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* __BK_DRAW_icon_H__ */