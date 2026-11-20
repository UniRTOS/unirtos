#include <stdint.h>
#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "mux_pipeline.h"
#include "uvc_pipeline_act.h"
#include "components/media_types.h"
#include "components/bk_video_pipeline/bk_video_pipeline.h"
#include "bk_video_pipeline_ctlr.h"

#define TAG "video_pipeline"

#define LOGE(fmt, ...) BK_LOGE(TAG, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define LOGW(fmt, ...) BK_LOGW(TAG, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define LOGI(fmt, ...) BK_LOGI(TAG, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define LOGD(fmt, ...) BK_LOGD(TAG, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define LOGV(fmt, ...) BK_LOGV(TAG, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

avdk_err_t bk_video_pipeline_open_h264e(bk_video_pipeline_handle_t handler, bk_video_pipeline_h264e_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handler && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->open_h264e, AVDK_ERR_INVAL, TAG, "open_h264e callback is NULL");

    return handler->open_h264e(handler, config);
}

avdk_err_t bk_video_pipeline_close_h264e(bk_video_pipeline_handle_t handler)
{
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->close_h264e, AVDK_ERR_INVAL, TAG, "close_h264e callback is NULL");

    return handler->close_h264e(handler);
}

avdk_err_t bk_video_pipeline_open_rotate(bk_video_pipeline_handle_t handler, bk_video_pipeline_decode_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handler && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->open_rotate, AVDK_ERR_INVAL, TAG, "open_rotate callback is NULL");

    // Validate rotation mode
    if (config->rotate_mode != NONE_ROTATE && 
        config->rotate_mode != HW_ROTATE && 
        config->rotate_mode != SW_ROTATE) {
        LOGE("Invalid rotate mode: %d\n", config->rotate_mode);
        return AVDK_ERR_INVAL;
    }

    // Validate rotation angle
    if (config->rotate_angle != ROTATE_NONE && config->rotate_angle != ROTATE_90 && 
        config->rotate_angle != ROTATE_180 && config->rotate_angle != ROTATE_270) {
        LOGE("Invalid rotate angle: %d\n", config->rotate_angle);
        return AVDK_ERR_INVAL;
    }

    return handler->open_rotate(handler, config);
}

avdk_err_t bk_video_pipeline_close_rotate(bk_video_pipeline_handle_t handler)
{
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->close_rotate, AVDK_ERR_INVAL, TAG, "close_rotate callback is NULL");

    return handler->close_rotate(handler);
}

avdk_err_t bk_video_pipeline_get_module_status(bk_video_pipeline_handle_t handler, video_pipeline_module_t module, video_pipeline_module_status_t *status)
{
    AVDK_RETURN_ON_FALSE(handler && status, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->get_module_status, AVDK_ERR_INVAL, TAG, "get_module_status callback is NULL");

    return handler->get_module_status(handler, module, status);
}

avdk_err_t bk_video_pipeline_reset_decode(bk_video_pipeline_handle_t handler)
{
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->reset_decode, AVDK_ERR_INVAL, TAG, "reset_decode callback is NULL");

    return handler->reset_decode(handler);
}

avdk_err_t bk_video_pipeline_delete(bk_video_pipeline_handle_t handler)
{
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->delete, AVDK_ERR_INVAL, TAG, "delete callback is NULL");

    return handler->delete(handler);
}

avdk_err_t bk_video_pipeline_ioctl(bk_video_pipeline_handle_t handler, video_pipeline_ioctl_cmd_t cmd, void *param)
{
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->ioctl, AVDK_ERR_INVAL, TAG, "ioctl callback is NULL");

    return handler->ioctl(handler, cmd, param);
}

avdk_err_t bk_video_pipeline_new(bk_video_pipeline_handle_t *handle, bk_video_pipeline_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL, it may have already been created");

    return bk_video_pipeline_ctlr_new(handle, config);
}