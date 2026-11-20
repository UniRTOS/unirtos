#include "bk_draw_osd_ctlr.h"
#include "components/bk_draw_osd_types.h"

#define TAG "draw_osd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


/**
 * @brief Delete OSD controller handle
 */
avdk_err_t bk_draw_osd_delete(bk_draw_osd_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->delete, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->delete(handle);
}

avdk_err_t bk_draw_osd_ioctl(bk_draw_osd_ctlr_handle_t handle, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->ioctl(handle, ioctl_cmd, param1, param2, param3);
}

/**
 * @brief Draw and blend image
 */
avdk_err_t bk_draw_osd_image(bk_draw_osd_ctlr_handle_t handle, osd_bg_info_t *bg_info, const blend_info_t *img_info)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->draw_image, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(bg_info, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(img_info, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    return handle->draw_image(handle, bg_info, img_info);
}

/**
 * @brief Draw font
 */
avdk_err_t bk_draw_osd_font(bk_draw_osd_ctlr_handle_t handle, osd_bg_info_t *bg_info, const blend_info_t *font_info)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->draw_font, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(bg_info, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(font_info, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    return handle->draw_font(handle, bg_info, font_info);
}

avdk_err_t bk_draw_osd_array(bk_draw_osd_ctlr_handle_t handle, osd_bg_info_t *bg_info, const blend_info_t *osd_array)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->draw_osd_array, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(bg_info, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    ///AVDK_RETURN_ON_FALSE(osd_array, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    return handle->draw_osd_array(handle, bg_info, osd_array);
}
avdk_err_t bk_draw_osd_add_or_updata(bk_draw_osd_ctlr_handle_t handle, const char *name, const char* content)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(name, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->add_or_updata, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->add_or_updata(handle, name, content);
}

avdk_err_t bk_draw_osd_remove(bk_draw_osd_ctlr_handle_t handle, const char *name)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(name, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->remove, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->remove(handle, name);
}

avdk_err_t bk_draw_osd_new(bk_draw_osd_ctlr_handle_t *handle, osd_ctlr_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL\n");

    ret = osd_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed");

    return ret;
}