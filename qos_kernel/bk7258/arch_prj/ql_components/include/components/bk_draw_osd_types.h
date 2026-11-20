// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <components/avdk_utils/avdk_types.h>
#include <components/avdk_utils/avdk_check.h>
#include <components/avdk_utils/avdk_error.h>
#include "frame_buffer.h"
#include "modules/lcd_font.h"

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************
 * component name: draw_osd
 * description: Public API (open interface)
 *******************************************************************/

#define MAX_BLEND_NAME_LEN    20
#define MAX_BLEND_CONTENT_LEN 31

typedef enum {
    OSD_CTLR_CMD_SET_PSRAM_USAGE,        /**< set use psram */
    OSD_CTLR_CMD_GET_PSRAM_USAGE,        /**< get psram usage status */
    OSD_CTLR_CMD_GET_ALL_ASSETS,         /**< get all blend_info */
    OSD_CTLR_CMD_GET_DRAW_INFO,          /**< get current blend osd info */
}osd_ioctl_cmd_t;

typedef enum
{
     BLEND_TYPE_IMAGE = 0,               /**< image type */
     BLEND_TYPE_FONT,                    /**< font type */
}blend_type_t;

typedef struct {
    uint8_t format;             /**< data_format_t, should be ARGB8888                    */
    uint32_t data_len;          /**< ARGB8888 image size, should be: (xsize * ysize * 4) (no used)  */
    const uint8_t *data;        /**< ARGB8888 image data                                  */
}blend_image_t;

typedef struct {
    const gui_font_digit_struct * font_digit_type;   /**< character database */
    uint32_t color;            /**< font color value used by RGB565 date*/
}blend_font_t;

typedef struct 
{
    uint8_t version;               /**< version */
    blend_type_t blend_type;       /**< 0: image, 1:font */
    const char name[MAX_BLEND_NAME_LEN];        /**< image name like "wifi","clock", "weather" */
    uint32_t width;                 /**< icon width   */
    uint32_t height;                /**< icon height  */
    uint32_t icon_width;                 /**< icon width   */
    uint32_t icon_height;                /**< icon height  */
    uint32_t bg_width;                 /**< background window width   */
    uint32_t bg_height;                /**< background window height  */
    uint16_t xpos;                  /**< icon x pos based on background window */
    uint16_t ypos;                  /**< icon y pos based on background window */
    union
    {
        blend_image_t image;
        blend_font_t font;
    };
}bk_blend_t;


typedef struct 
{
    char name[MAX_BLEND_NAME_LEN];        /**<  name like "wifi","clock", "weather" */
    const bk_blend_t *addr;               /**< the pointer, pointer to the struct */
    char content[MAX_BLEND_CONTENT_LEN];  /**< content like "wifi0", "12:00","v 1.0.0" */
}blend_info_t;

typedef struct{
    blend_info_t *entry;                /**< the pointer, pointer to the struct array */
    size_t size;                        /**< the size of the struct array */
    size_t capacity;                    /**< the capacity of the struct array */
}dynamic_array_t;


/* osd controller config */
typedef struct {
    const blend_info_t *blend_assets;    /**< the pointer, pointer to the struct all blend assetsarray */
    const blend_info_t *blend_info;      /**< the pointer, pointer to current blend info */
    bool draw_in_psram;                 /**< true: use PSRAM, false: use SRAM */

} osd_ctlr_config_t;

typedef struct{
    frame_buffer_t *frame;      /**< the pointer, pointer to the struct frame buffer */
    uint16_t width;             /**< osd draw visible width */
    uint16_t height;            /**< osd draw visible height */
}osd_bg_info_t;

/**< osd controller handle */
typedef struct bk_draw_osd_ctlr *bk_draw_osd_ctlr_handle_t;


typedef struct bk_draw_osd_ctlr
{
    avdk_err_t (*draw_image)(bk_draw_osd_ctlr_handle_t controller, osd_bg_info_t *bg_info,  const blend_info_t *info);
    avdk_err_t (*draw_font)(bk_draw_osd_ctlr_handle_t controller, osd_bg_info_t *bg_info,  const blend_info_t *info);
    avdk_err_t (*draw_osd_array)(bk_draw_osd_ctlr_handle_t controller, osd_bg_info_t *bg_info, const blend_info_t *info);
    avdk_err_t (*add_or_updata)(bk_draw_osd_ctlr_handle_t controller, const char *name, const char* content);
    avdk_err_t (*ioctl)(bk_draw_osd_ctlr_handle_t controller, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3);
    avdk_err_t (*delete)(bk_draw_osd_ctlr_handle_t controller);
    avdk_err_t (*remove)(bk_draw_osd_ctlr_handle_t controller, const char *name);
    avdk_err_t (*close)(bk_draw_osd_ctlr_handle_t controller);
}bk_draw_osd_ctlr_t;




#ifdef __cplusplus
}
#endif
