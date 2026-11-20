#include <avdk_check.h>
#include <components/media_types.h>
#include <components/bk_jpeg_encode_ctlr.h>

#define TAG "bk_jenc"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

avdk_err_t bk_jpeg_encode_open(bk_jpeg_encode_ctlr_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->open, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->open(handler);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "open failed");

    return ret;
}

avdk_err_t bk_jpeg_encode_close(bk_jpeg_encode_ctlr_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->close, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->close(handler);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "close failed");

    return ret;
}

avdk_err_t bk_jpeg_encode_encode(bk_jpeg_encode_ctlr_handle_t handler, frame_buffer_t *in_frame, frame_buffer_t *out_frame)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->encode, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(in_frame, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(out_frame, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->encode(handler, in_frame, out_frame);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "encode failed");

    return ret;
}

avdk_err_t bk_jpeg_encode_delete(bk_jpeg_encode_ctlr_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->del, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->del(handler);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "delete failed");

    return ret;
}

avdk_err_t bk_jpeg_encode_ioctl(bk_jpeg_encode_ctlr_handle_t handler, uint32_t cmd, void *param)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->ioctl, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->ioctl(handler, cmd, param);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "ioctl failed");

    return ret;
}
