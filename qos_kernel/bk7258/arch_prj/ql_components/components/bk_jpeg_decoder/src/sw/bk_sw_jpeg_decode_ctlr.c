#include <stdint.h>
#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/media_types.h"

#include <driver/jpeg_dec.h>
#include <modules/jpeg_decode_sw.h>


#include "components/bk_jpeg_decode/bk_jpeg_decode_sw.h"

#include "bk_jpeg_decode_ctlr.h"

#define TAG "dec_ctlr"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static uint32_t sw_jpeg_decode_conver_out_format(bk_jpeg_decode_sw_out_format_t out_format)
{
    switch (out_format)
    {
    case JPEG_DECODE_SW_OUT_FORMAT_RGB565:
        return JD_FORMAT_RGB565;
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV:
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_90:
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_180:
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_270:
        return JD_FORMAT_YUYV;
    case JPEG_DECODE_SW_OUT_FORMAT_VUYY:
        return JD_FORMAT_VUYY;
    case JPEG_DECODE_SW_OUT_FORMAT_VYUY:
        return JD_FORMAT_VYUY;
    case JPEG_DECODE_SW_OUT_FORMAT_RGB888:
        return JD_FORMAT_RGB888;
    default:
        return JD_FORMAT_YUYV;
    }
}

static uint32_t sw_jpeg_decode_get_rotate_angle(bk_jpeg_decode_sw_out_format_t out_format)
{
    switch (out_format)
    {
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_90:
        return ROTATE_90;
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_180:
        return ROTATE_180;
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_270:
        return ROTATE_270;
    default:
        return ROTATE_NONE;
    }
}

static avdk_err_t software_jpeg_decode_ctlr_open(bk_jpeg_decode_sw_handle_t handler)
{
    bk_err_t ret = BK_OK;
    private_jpeg_decode_sw_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    ret = bk_jpeg_dec_sw_init_by_handle(&controller->jpeg_dec_handle, NULL, 0);
    if (ret != BK_OK)
    {
        LOGE("%s sw jpeg_dec_init failed: %d\n", __func__, ret);
        return ret;
    }

    controller->rotate_info.rotate_angle = sw_jpeg_decode_get_rotate_angle(controller->config.out_format);
    
    if (controller->rotate_info.rotate_angle != ROTATE_NONE)
    {
        if (controller->rotate_info.rotate_buf == NULL)
        {
            controller->rotate_info.rotate_buf = os_malloc(SW_DECODE_ROTATE_BUFFER_SIZE);
            AVDK_RETURN_ON_FALSE(controller->rotate_info.rotate_buf, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
        }
    }

    jd_set_rotate_by_handle(controller->jpeg_dec_handle,
        controller->rotate_info.rotate_angle,
        controller->rotate_info.rotate_buf);

    bk_jpeg_decode_sw_out_format_t out_format = controller->config.out_format;
    jd_set_format_by_handle(controller->jpeg_dec_handle, sw_jpeg_decode_conver_out_format(out_format));

    bk_jpeg_decode_byte_order_t byte_order = controller->config.byte_order;
    jd_set_byte_order_by_handle(controller->jpeg_dec_handle, byte_order);

    controller->module_status.status = JPEG_DECODE_ENABLED;
    return ret;
}

static avdk_err_t software_jpeg_decode_ctlr_close(bk_jpeg_decode_sw_handle_t handler)
{
    bk_err_t ret = BK_OK;
    private_jpeg_decode_sw_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    ret = bk_jpeg_dec_sw_deinit_by_handle(controller->jpeg_dec_handle);
    if (ret != BK_OK)
    {
        LOGE("%s sw jpeg_dec_deinit failed: %d\n", __func__, ret);
        return ret;
    }
    controller->module_status.status = JPEG_DECODE_DISABLED;
    return ret;
}

static avdk_err_t software_jpeg_decode_ctlr_decode(bk_jpeg_decode_sw_handle_t handler, frame_buffer_t *in_frame, frame_buffer_t *out_frame)
{
    private_jpeg_decode_sw_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == JPEG_DECODE_ENABLED, AVDK_ERR_INVAL, TAG, "jpeg decode is disabled");
    AVDK_RETURN_ON_FALSE(in_frame, AVDK_ERR_INVAL, TAG, "in_frame is NULL");
    AVDK_RETURN_ON_FALSE(out_frame, AVDK_ERR_INVAL, TAG, "out_frame is NULL");
    AVDK_RETURN_ON_FALSE(in_frame->frame, AVDK_ERR_INVAL, TAG, "in_frame frame is NULL");
    AVDK_RETURN_ON_FALSE(out_frame->frame, AVDK_ERR_INVAL, TAG, "out_frame frame is NULL");

    sw_jpeg_dec_res_t res = {0};
    bk_err_t ret = bk_jpeg_dec_sw_start_by_handle(controller->jpeg_dec_handle,
                                        JPEGDEC_BY_FRAME,
                                        in_frame->frame,
                                        out_frame->frame,
                                        in_frame->length,
                                        out_frame->size,
                                        &res);
    if (ret != BK_OK)
    {
        if (controller->config.decode_cbs.out_complete)
        {
            controller->config.decode_cbs.out_complete(0, BK_FAIL, out_frame);
        }
        LOGE("%s sw jpeg_dec_decode failed: %d\n", __func__, ret);
        return ret;
    }

    if (controller->config.decode_cbs.out_complete)
    {
        controller->config.decode_cbs.out_complete(0, BK_OK, out_frame);
    }
    return AVDK_ERR_OK;
}

static avdk_err_t software_jpeg_decode_ctlr_delete(bk_jpeg_decode_sw_handle_t handler)
{
    private_jpeg_decode_sw_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == JPEG_DECODE_DISABLED, AVDK_ERR_INVAL, TAG, "jpeg decode is enabled");

    if (controller->rotate_info.rotate_buf)
    {
        os_free(controller->rotate_info.rotate_buf);
        controller->rotate_info.rotate_buf = NULL;
    }

    os_free(controller);
    return AVDK_ERR_OK;
}

static avdk_err_t software_jpeg_decode_ctlr_set_config(bk_jpeg_decode_sw_handle_t handler, bk_jpeg_decode_sw_out_frame_info_t *out_frame_info)
{
    private_jpeg_decode_sw_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(out_frame_info, AVDK_ERR_INVAL, TAG, "out_frame_info is NULL");

    controller->config.out_format = out_frame_info->out_format;
    controller->config.byte_order = out_frame_info->byte_order;

    uint32_t new_rotate_angle = sw_jpeg_decode_get_rotate_angle(controller->config.out_format);

    // Allocate rotate buffer only if rotation is needed and buffer doesn't exist
    if (new_rotate_angle != ROTATE_NONE && controller->rotate_info.rotate_buf == NULL)
    {
        controller->rotate_info.rotate_buf = os_malloc(SW_DECODE_ROTATE_BUFFER_SIZE);
        AVDK_RETURN_ON_FALSE(controller->rotate_info.rotate_buf, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    }

    controller->rotate_info.rotate_angle = new_rotate_angle;

    jd_set_rotate_by_handle(controller->jpeg_dec_handle,
        controller->rotate_info.rotate_angle,
        controller->rotate_info.rotate_buf);

    bk_jpeg_decode_sw_out_format_t out_format = controller->config.out_format;
    jd_set_format_by_handle(controller->jpeg_dec_handle, sw_jpeg_decode_conver_out_format(out_format));

    bk_jpeg_decode_byte_order_t byte_order = controller->config.byte_order;
    jd_set_byte_order_by_handle(controller->jpeg_dec_handle, byte_order);

    return AVDK_ERR_OK;
}

static avdk_err_t software_jpeg_decode_ctlr_get_img_info(bk_jpeg_decode_sw_handle_t handler, bk_jpeg_decode_img_info_t *info)
{
    private_jpeg_decode_sw_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(info, AVDK_ERR_INVAL, TAG, "info is NULL");
    AVDK_RETURN_ON_FALSE(info->frame, AVDK_ERR_INVAL, TAG, "info frame is NULL");

    return bk_get_jpeg_data_info(info);
}

static avdk_err_t software_jpeg_decode_ctlr_ioctl(bk_jpeg_decode_sw_handle_t handler, bk_jpeg_decode_sw_ioctl_cmd_t cmd, void *param)
{
    private_jpeg_decode_sw_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    switch (cmd)
    {
    case JPEG_DECODE_SW_IOCTL_CMD_BASE:
        break;
    default:
        break;
    }

    return AVDK_ERR_OK;
}

avdk_err_t bk_software_jpeg_decode_ctlr_new(bk_jpeg_decode_sw_handle_t *handle, bk_jpeg_decode_sw_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    private_jpeg_decode_sw_ctlr_t *controller = os_malloc(sizeof(private_jpeg_decode_sw_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_jpeg_decode_sw_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_jpeg_decode_sw_config_t));
    controller->ops.open = software_jpeg_decode_ctlr_open;
    controller->ops.close = software_jpeg_decode_ctlr_close;
    controller->ops.decode = software_jpeg_decode_ctlr_decode;
    controller->ops.delete = software_jpeg_decode_ctlr_delete;
    controller->ops.set_config = software_jpeg_decode_ctlr_set_config;
    controller->ops.get_img_info = software_jpeg_decode_ctlr_get_img_info;
    controller->ops.ioctl = software_jpeg_decode_ctlr_ioctl;

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}
