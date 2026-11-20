#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/bk_video_recorder_types.h"
#include "bk_video_recorder_ctlr.h"

#define TAG "bk_video_recorder"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

avdk_err_t bk_video_recorder_open(bk_video_recorder_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->open, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->open(handler);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "open failed");

    return ret;
}

avdk_err_t bk_video_recorder_close(bk_video_recorder_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->close, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->close(handler);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "close failed");

    return ret;
}

avdk_err_t bk_video_recorder_start(bk_video_recorder_handle_t handler, char *file_path, uint32_t record_type)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->start, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->start(handler, file_path, record_type);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "start failed");

    return ret;
}

avdk_err_t bk_video_recorder_stop(bk_video_recorder_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->stop, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->stop(handler);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "stop failed");

    return ret;
}

avdk_err_t bk_video_recorder_delete(bk_video_recorder_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->delete_video_recorder, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->delete_video_recorder(handler);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "delete failed");

    return ret;
}

avdk_err_t bk_video_recorder_ioctl(bk_video_recorder_handle_t handler, bk_video_recorder_ioctl_cmd_t cmd, void *param)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->ioctl, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->ioctl(handler, cmd, param);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "ioctl failed");

    return ret;
}

avdk_err_t bk_video_recorder_new(bk_video_recorder_handle_t *handle, bk_video_recorder_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL, it may have already been created");

    ret = bk_video_recorder_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "new failed");

    return ret;
}
