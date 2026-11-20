//#include <stdint.h>
#include "private_camera_ctlr.h"
#include <components/avdk_utils/avdk_check.h>
#include <components/dvp_camera.h>

#define TAG "camera_dvp_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern uint8_t *media_bt_share_buffer;

static avdk_err_t dvp_camera_ctlr_open(bk_camera_ctlr_t *controller)
{
    private_camera_dvp_ctlr_t *dvp_controller = __containerof(controller, private_camera_dvp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(dvp_controller->state == CAMERA_STATE_INIT, AVDK_ERR_INVAL, TAG, "control state err");

    avdk_err_t ret = bk_dvp_open(&dvp_controller->handle, &dvp_controller->config.config, dvp_controller->config.cbs, dvp_controller->encode_buffer);

    if (ret == AVDK_ERR_OK)
    {
        dvp_controller->state = CAMERA_STATE_OPENED;
    }

    return ret;
}

static avdk_err_t dvp_camera_ctlr_close(bk_camera_ctlr_t *controller)
{
    private_camera_dvp_ctlr_t *dvp_controller = __containerof(controller, private_camera_dvp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(dvp_controller->state == CAMERA_STATE_OPENED, AVDK_ERR_INVAL, TAG, "control state err");

    avdk_err_t ret = bk_dvp_close(dvp_controller->handle);
    if (ret == AVDK_ERR_OK)
    {
        dvp_controller->state = CAMERA_STATE_INIT;
        dvp_controller->handle = NULL;
    }

    return ret;
}

static avdk_err_t dvp_camera_ctlr_delete(bk_camera_ctlr_t *controller)
{
    private_camera_dvp_ctlr_t *dvp_controller = __containerof(controller, private_camera_dvp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(dvp_controller->state == CAMERA_STATE_INIT, AVDK_ERR_INVAL, TAG, "control state err");

    if (dvp_controller->encode_buffer) {
        os_free(dvp_controller->encode_buffer);
        dvp_controller->encode_buffer = NULL;
    }

    os_free(dvp_controller);
    return AVDK_ERR_OK;
}

static avdk_err_t dvp_camera_ctlr_suspend(bk_camera_ctlr_t *controller)
{
    private_camera_dvp_ctlr_t *dvp_controller = __containerof(controller, private_camera_dvp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(dvp_controller->state == CAMERA_STATE_OPENED, AVDK_ERR_INVAL, TAG, "control state err");

    return bk_dvp_suspend(dvp_controller->handle);
}

static avdk_err_t dvp_camera_ctlr_resume(bk_camera_ctlr_t *controller)
{
    private_camera_dvp_ctlr_t *dvp_controller = __containerof(controller, private_camera_dvp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(dvp_controller->state == CAMERA_STATE_OPENED, AVDK_ERR_INVAL, TAG, "control state err");

    return bk_dvp_resume(dvp_controller->handle);
}

static avdk_err_t dvp_camera_ctlr_ioctlr(bk_camera_ctlr_t *controller, uint32_t cmd, void *arg)
{
    avdk_err_t ret = AVDK_ERR_UNKNOWN;
    private_camera_dvp_ctlr_t *dvp_controller = __containerof(controller, private_camera_dvp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(dvp_controller->state == CAMERA_STATE_OPENED, AVDK_ERR_INVAL, TAG, "control state err");

    if (cmd == DVP_IOCTL_CMD_H264_IDR_RESET)
    {
        ret = bk_dvp_h264_idr_reset(dvp_controller->handle);
    }
    else if (cmd == DVP_IOCTL_CMD_SENSOR_WRITE_REGISTER)
    {
        ret = bk_dvp_sensor_write_register(dvp_controller->handle, (dvp_sensor_reg_val_t *)arg);
    }
    else if (cmd == DVP_IOCTL_CMD_SENSOR_READ_REGISTER)
    {
        ret = bk_dvp_sensor_read_register(dvp_controller->handle, (dvp_sensor_reg_val_t *)arg);
    }
    else
    {
        ret = AVDK_ERR_INVAL;
    }

    return ret;
}

avdk_err_t bk_camera_dvp_ctlr_new(bk_camera_ctlr_handle_t *handle, bk_dvp_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config->cbs, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config->cbs, AVDK_ERR_INVAL, TAG, "cbs is NULL");
    AVDK_RETURN_ON_FALSE(config->cbs->malloc, AVDK_ERR_INVAL, TAG, "cbs->malloc is NULL");
    AVDK_RETURN_ON_FALSE(config->cbs->complete, AVDK_ERR_INVAL, TAG, "cbs->complete is NULL");
    private_camera_dvp_ctlr_t *controller = os_malloc(sizeof(private_camera_dvp_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

    os_memset(controller, 0, sizeof(private_camera_dvp_ctlr_t));
    os_memcpy(&controller->config, config, sizeof(bk_dvp_ctlr_config_t));

    if (config->config.img_format & IMAGE_H264)
    {
        controller->encode_buffer = os_malloc(config->config.width * 32 * 2);
        if (controller->encode_buffer == NULL)
        {
            LOGE("%s, malloc h264 encode buffer failed\n", __func__);
            os_free(controller);
            return AVDK_ERR_NOMEM;
        }
    }
    else if (config->config.img_format & IMAGE_MJPEG)
    {
        controller->encode_buffer = os_malloc(config->config.width * 16 * 2);
        if (controller->encode_buffer == NULL)
        {
            LOGE("%s, malloc mjpeg encode buffer failed\n", __func__);
            os_free(controller);
            return AVDK_ERR_NOMEM;
        }
    }

    controller->ops.open = dvp_camera_ctlr_open;
    controller->ops.close = dvp_camera_ctlr_close;
    controller->ops.del = dvp_camera_ctlr_delete;
    controller->ops.suspend = dvp_camera_ctlr_suspend;
    controller->ops.resume = dvp_camera_ctlr_resume;
    controller->ops.ioctlr = dvp_camera_ctlr_ioctlr;

    *handle = &(controller->ops);

    controller->state = CAMERA_STATE_INIT;

    return AVDK_ERR_OK;
}
