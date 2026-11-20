#include "private_camera_ctlr.h"
#include "components/avdk_utils/avdk_check.h"
#include <components/uvc_camera.h>

#define TAG "uvc_ctlr"

static avdk_err_t uvc_camera_ctlr_open(bk_camera_ctlr_t *controller)
{
    private_camera_uvc_ctlr_t *uvc_controller = __containerof(controller, private_camera_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(uvc_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(uvc_controller->state == CAMERA_STATE_INIT, AVDK_ERR_INVAL, TAG, "control state err");

    avdk_err_t ret = bk_uvc_power_on(uvc_controller->config.config.img_format, 40000);
    if (ret == AVDK_ERR_OK)
    {
        ret = bk_uvc_open(&uvc_controller->handle, &uvc_controller->config.config, uvc_controller->config.cbs);

        if (ret == AVDK_ERR_OK)
        {
            uvc_controller->state = CAMERA_STATE_OPENED;
        }
        else
        {
            bk_uvc_power_off();
        }
    }
    return ret;
}

static avdk_err_t uvc_camera_ctlr_close(bk_camera_ctlr_t *controller)
{
    private_camera_uvc_ctlr_t *uvc_controller = __containerof(controller, private_camera_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(uvc_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(uvc_controller->state == CAMERA_STATE_OPENED, AVDK_ERR_INVAL, TAG, "control state err");
    avdk_err_t ret = bk_uvc_close(uvc_controller->handle);
    if (ret == AVDK_ERR_OK)
    {
        bk_uvc_power_off();
        uvc_controller->state = CAMERA_STATE_INIT;
    }

    return ret;
}

static avdk_err_t uvc_camera_ctlr_delete(bk_camera_ctlr_t *controller)
{
    private_camera_uvc_ctlr_t *uvc_controller = __containerof(controller, private_camera_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(uvc_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(uvc_controller->state == CAMERA_STATE_INIT, AVDK_ERR_INVAL, TAG, "control state err");

    //TODO
    os_free(uvc_controller);
    return AVDK_ERR_OK;
}

static avdk_err_t uvc_camera_ctlr_suspend(bk_camera_ctlr_t *controller)
{
    private_camera_uvc_ctlr_t *uvc_controller = __containerof(controller, private_camera_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(uvc_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(uvc_controller->state == CAMERA_STATE_OPENED, AVDK_ERR_INVAL, TAG, "control state err");
    return bk_uvc_suspend(uvc_controller->handle);
}

static avdk_err_t uvc_camera_ctlr_resume(bk_camera_ctlr_t *controller)
{
    private_camera_uvc_ctlr_t *uvc_controller = __containerof(controller, private_camera_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(uvc_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(uvc_controller->state == CAMERA_STATE_OPENED, AVDK_ERR_INVAL, TAG, "control state err");
    return bk_uvc_resume(uvc_controller->handle);
}

static avdk_err_t uvc_camera_ctlr_ioctlr(bk_camera_ctlr_t *controller, uint32_t cmd, void *arg)
{
    private_camera_uvc_ctlr_t *uvc_controller = __containerof(controller, private_camera_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(uvc_controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(uvc_controller->state == CAMERA_STATE_OPENED, AVDK_ERR_INVAL, TAG, "control state err");
    // to do
    return AVDK_ERR_OK;
}

avdk_err_t bk_camera_uvc_ctlr_new(bk_camera_ctlr_handle_t *handle, bk_uvc_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config->cbs, AVDK_ERR_INVAL, TAG, "cbs is NULL");
    AVDK_RETURN_ON_FALSE(config->cbs->malloc, AVDK_ERR_INVAL, TAG, "cbs->malloc is NULL");
    AVDK_RETURN_ON_FALSE(config->cbs->complete, AVDK_ERR_INVAL, TAG, "cbs->complete is NULL");
    private_camera_uvc_ctlr_t *controller = os_malloc(sizeof(private_camera_uvc_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_camera_uvc_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_uvc_ctlr_config_t));
    controller->config.cbs = config->cbs;
    controller->ops.open = uvc_camera_ctlr_open;
    controller->ops.close = uvc_camera_ctlr_close;
    controller->ops.suspend = uvc_camera_ctlr_suspend;
    controller->ops.resume = uvc_camera_ctlr_resume;
    controller->ops.ioctlr = uvc_camera_ctlr_ioctlr;
    controller->ops.del = uvc_camera_ctlr_delete;

    *handle = &(controller->ops);
    controller->state = CAMERA_STATE_INIT;

    return AVDK_ERR_OK;
}
