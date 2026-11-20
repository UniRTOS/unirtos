#include <stdint.h>
#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "mux_pipeline.h"
#include "uvc_pipeline_act.h"
#include "components/media_types.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_sw.h"
#include "bk_jpeg_decode_ctlr.h"

#define TAG "bk_jdec_sw"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

avdk_err_t bk_jpeg_decode_sw_open(bk_jpeg_decode_sw_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->open, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->open(handler);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "open failed");

    return ret;
}

avdk_err_t bk_jpeg_decode_sw_close(bk_jpeg_decode_sw_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->close, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->close(handler);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "close failed");

    return ret;
}

avdk_err_t bk_jpeg_decode_sw_decode(bk_jpeg_decode_sw_handle_t handler, frame_buffer_t *in_frame, frame_buffer_t *out_frame)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->decode, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(in_frame, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(out_frame, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->decode(handler, in_frame, out_frame);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "decode failed");

    return ret;
}

avdk_err_t bk_jpeg_decode_sw_decode_async(bk_jpeg_decode_sw_handle_t handler, frame_buffer_t *in_frame)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->decode_async, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(in_frame, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->decode_async(handler, in_frame);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "decode_async failed");

    return ret;
}

avdk_err_t bk_jpeg_decode_sw_delete(bk_jpeg_decode_sw_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->delete, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->delete(handler);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "delete failed");

    return ret;
}

avdk_err_t bk_jpeg_decode_sw_set_config(bk_jpeg_decode_sw_handle_t handler, bk_jpeg_decode_sw_out_frame_info_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->set_config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->set_config(handler, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "set_config failed");

    return ret;
}

avdk_err_t bk_jpeg_decode_sw_get_img_info(bk_jpeg_decode_sw_handle_t handler, bk_jpeg_decode_img_info_t *info)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->get_img_info, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->get_img_info(handler, info);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "get_img_info failed");

    return ret;
}

avdk_err_t bk_jpeg_decode_sw_ioctl(bk_jpeg_decode_sw_handle_t handler, bk_jpeg_decode_sw_ioctl_cmd_t cmd, void *param)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->ioctl, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->ioctl(handler, cmd, param);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "ioctl failed");

    return ret;
}

avdk_err_t bk_software_jpeg_decode_new(bk_jpeg_decode_sw_handle_t *handle, bk_jpeg_decode_sw_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL");

    ret = bk_software_jpeg_decode_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed");

    return ret;
}

avdk_err_t bk_software_jpeg_decode_on_multi_core_new(bk_jpeg_decode_sw_handle_t *handle, bk_jpeg_decode_sw_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL");

    ret = bk_software_jpeg_decode_on_multi_core_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed");

    return ret;
}
