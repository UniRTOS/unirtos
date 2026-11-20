#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/avdk_utils/avdk_check.h>
#include "bk_draw_icon_ctlr.h"


#define TAG "draw_icon"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


/**
 * @brief Delete draw icon controller handle
 */
avdk_err_t bk_draw_icon_delete(bk_draw_icon_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->delete, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->delete(handle);
}

avdk_err_t bk_draw_icon_ioctl(bk_draw_icon_ctlr_handle_t handle, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->ioctl(handle, ioctl_cmd, param1, param2, param3);
}

/**
 * @brief Draw and blend image
 */
avdk_err_t bk_draw_icon_image(bk_draw_icon_ctlr_handle_t handle, icon_image_blend_cfg_t *cfg)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->draw_image, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(cfg, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    return handle->draw_image(handle, cfg);
}

/**
 * @brief Draw font
 */
avdk_err_t bk_draw_icon_font(bk_draw_icon_ctlr_handle_t handle, icon_font_blend_cfg_t *cfg)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->draw_font, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(cfg, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    return handle->draw_font(handle, cfg);
}

/**
 * @brief Draw icon controller
 * @param handle Output parameter, used to store the created draw icon controller handle
 * @param config Draw icon controller configuration parameters
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_draw_icon_new(bk_draw_icon_ctlr_handle_t *handle, icon_ctlr_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL");

    ret = bk_draw_icon_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed");

    return ret;
}