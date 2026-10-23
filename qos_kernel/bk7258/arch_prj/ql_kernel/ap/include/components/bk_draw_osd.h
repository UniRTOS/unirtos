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

#ifndef __BK_DRAW_OSD_H__
#define __BK_DRAW_OSD_H__

#include "components/bk_draw_osd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************
 * component name: draw_osd
 * description: Public API (open interface)
 *******************************************************************/

/**
 * @brief draw osd array
 * 
 * @param handle osd controller handle
 * @param bg_info osd background info
 * @param info osd info
 * @return avdk_err_t 
 */
avdk_err_t bk_draw_osd_array(bk_draw_osd_ctlr_handle_t handle, osd_bg_info_t *bg_info, const blend_info_t *info);

/**
 * @brief draw osd image
 * 
 * @param handle osd controller handle
 * @param bg_info osd background info
 * @param info osd info
 * @return avdk_err_t 
 */
/**
 * @brief draw osd image
 * 
 * @param handle osd controller handle
 * @param bg_info osd background info
 * @param info osd info
 * @return avdk_err_t 
 */
avdk_err_t bk_draw_osd_image(bk_draw_osd_ctlr_handle_t handle, osd_bg_info_t *bg_info,  const blend_info_t *info);

/**
 * @brief draw osd font
 * 
 * @param handle osd controller handle
 * @param bg_info osd background info
 * @param info osd info
 * @return avdk_err_t 
 */
avdk_err_t bk_draw_osd_font(bk_draw_osd_ctlr_handle_t handle, osd_bg_info_t *bg_info,  const blend_info_t *info);

/**
 * @brief add or updata osd info
 * 
 * @param handle osd controller handle
 * @param name osd name
 * @param content osd content
 * @return avdk_err_t 
 */
avdk_err_t bk_draw_osd_add_or_updata(bk_draw_osd_ctlr_handle_t handle, const char *name, const char* content);

/**
 * @brief osd controller ioctl
 * 
 * @param handle osd controller handle
 * @param ioctl_cmd ioctl command
 * @param param1 ioctl param1
 * @param param2 ioctl param2
 * @param param3 ioctl param3
 * @return avdk_err_t 
 */
avdk_err_t bk_draw_osd_ioctl(bk_draw_osd_ctlr_handle_t handle, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3);

/**
 * @brief delete osd controller
 * 
 * @param handle osd controller handle
 * @return avdk_err_t 
 */ 
avdk_err_t bk_draw_osd_delete(bk_draw_osd_ctlr_handle_t handle);

/**
 * @brief create osd controller
 * 
 * @param handle osd controller handle
 * @param config osd controller config
 * @return avdk_err_t 
 */
avdk_err_t bk_draw_osd_new(bk_draw_osd_ctlr_handle_t *handle, osd_ctlr_config_t *config);

/**
 * @brief remove osd info
 * 
 * @param handle osd controller handle
 * @param name osd name
 * @return avdk_err_t 
 */
avdk_err_t bk_draw_osd_remove(bk_draw_osd_ctlr_handle_t handle, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __BK_DRAW_OSD_H__ */
