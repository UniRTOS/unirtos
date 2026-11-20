#include <os/os.h>
#include <stdint.h>
#include "components/bk_display.h"
#include "bk_display_ctlr.h"

#define TAG "bk_display"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG,  ##__VA_ARGS__)


avdk_err_t bk_display_open(bk_display_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->open, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->open(handle);
}

avdk_err_t bk_display_close(bk_display_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->close, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->close(handle);
}

avdk_err_t bk_display_delete(bk_display_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->delete, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->delete(handle);
}


avdk_err_t bk_display_flush(bk_display_ctlr_handle_t handle, frame_buffer_t *frame, bk_err_t (*free_t)(void *args))
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->flush, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(frame, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(free_t, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    return handle->flush(handle, frame, free_t);
}

avdk_err_t bk_display_ioctl(bk_display_ctlr_handle_t handle, uint32_t cmd, void *arg)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->ioctl(handle, cmd, arg);
}

avdk_err_t bk_display_mcu_new(bk_display_ctlr_handle_t *handle, bk_display_mcu_ctlr_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;

    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL\n");
    AVDK_RETURN_ON_FALSE(config->lcd_device, AVDK_ERR_INVAL, TAG, "lcd_device is NULL\n");

    ret = bk_display_mcu_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed\n");

    return ret;
}

avdk_err_t bk_display_rgb_new(bk_display_ctlr_handle_t *handle, bk_display_rgb_ctlr_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;

    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL\n");
    AVDK_RETURN_ON_FALSE(config->lcd_device, AVDK_ERR_INVAL, TAG, "lcd_device is NULL\n");

    ret = bk_display_rgb_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed \n");

    return ret;
}

avdk_err_t bk_display_spi_new(bk_display_ctlr_handle_t *handle, bk_display_spi_ctlr_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;

    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL\n");
    AVDK_RETURN_ON_FALSE(config->lcd_device, AVDK_ERR_INVAL, TAG, "lcd_device is NULL\n");

    ret = bk_display_spi_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed \n");

    return ret;
}

avdk_err_t bk_display_qspi_new(bk_display_ctlr_handle_t *handle, bk_display_qspi_ctlr_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;

    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL\n");
    AVDK_RETURN_ON_FALSE(config->lcd_device, AVDK_ERR_INVAL, TAG, "lcd_device is NULL\n");

    ret = bk_display_qspi_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed \n");

    return ret;
}

avdk_err_t bk_display_dual_qspi_new(bk_display_ctlr_handle_t *handle, bk_display_dual_qspi_ctlr_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;

    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL\n");
    AVDK_RETURN_ON_FALSE(config->lcd0_config.lcd_device, AVDK_ERR_INVAL, TAG, "lcd0_config.lcd_device is NULL\n");
    AVDK_RETURN_ON_FALSE(config->lcd1_config.lcd_device, AVDK_ERR_INVAL, TAG, "lcd1_config.lcd_device is NULL\n");

    ret = bk_display_dual_qspi_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed \n");

    return ret;
}

avdk_err_t bk_display_dual_spi_new(bk_display_ctlr_handle_t *handle, bk_display_dual_spi_ctlr_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;

    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL\n");
    AVDK_RETURN_ON_FALSE(config->lcd0_config.lcd_device, AVDK_ERR_INVAL, TAG, "lcd0_config.lcd_device is NULL\n");
    AVDK_RETURN_ON_FALSE(config->lcd1_config.lcd_device, AVDK_ERR_INVAL, TAG, "lcd1_config.lcd_device is NULL\n");

    ret = bk_display_dual_spi_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed \n");

    return ret;
}
