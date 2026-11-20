#include <stdint.h>
#include "bk_display_ctlr.h"
#include <driver/lcd_spi.h>
#include <driver/pwr_clk.h>
#include <components/media_types.h>
#include <components/bk_display.h>

#define TAG "dual_spi_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static avdk_err_t dual_spi_display_lcd0_free_cb(void *frame)
{
    // Nothing to do

    return AVDK_ERR_OK;
}

static avdk_err_t dual_spi_display_ctlr_open(bk_display_ctlr_t *controller)
{
    private_display_dual_spi_ctlr_t *dual_spi_controller = __containerof(controller, private_display_dual_spi_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dual_spi_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_480M);

    bk_display_open(dual_spi_controller->disp_handle0);
    bk_display_open(dual_spi_controller->disp_handle1);

    return AVDK_ERR_OK;
}

static avdk_err_t dual_spi_display_ctlr_close(bk_display_ctlr_t *controller)
{
    private_display_dual_spi_ctlr_t *dual_spi_controller = __containerof(controller, private_display_dual_spi_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dual_spi_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    bk_display_close(dual_spi_controller->disp_handle0);
    bk_display_close(dual_spi_controller->disp_handle1);

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);

    return AVDK_ERR_OK;
}

static avdk_err_t dual_spi_display_ctlr_delete(bk_display_ctlr_t *controller)
{
    private_display_dual_spi_ctlr_t *dual_spi_controller = __containerof(controller, private_display_dual_spi_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dual_spi_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    os_free(dual_spi_controller->disp_buffer1);
    os_free(dual_spi_controller->disp_buffer2);

    bk_display_delete(dual_spi_controller->disp_handle0);
    bk_display_delete(dual_spi_controller->disp_handle1);

    os_free(dual_spi_controller);

    return AVDK_ERR_OK;
}

static avdk_err_t dual_spi_display_ctlr_flush(bk_display_ctlr_t *controller, frame_buffer_t *frame, bk_err_t (*free_t)(void *args))
{
    private_display_dual_spi_ctlr_t *dual_spi_controller = __containerof(controller, private_display_dual_spi_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(dual_spi_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    frame_buffer_t *frame_buffer = frame;
    dual_spi_controller->disp_buffer1->frame = frame_buffer->frame;
    dual_spi_controller->disp_buffer1->size = frame_buffer->size / 2;
    dual_spi_controller->disp_buffer2->frame = frame_buffer->frame + frame_buffer->size / 2;
    dual_spi_controller->disp_buffer2->size = frame_buffer->size / 2;

    bk_display_flush(dual_spi_controller->disp_handle0, dual_spi_controller->disp_buffer1, dual_spi_display_lcd0_free_cb);
    bk_display_flush(dual_spi_controller->disp_handle1, dual_spi_controller->disp_buffer2, free_t);

    return AVDK_ERR_OK;
}

static avdk_err_t dual_spi_display_ctlr_ioctl(bk_display_ctlr_t *controller, uint32_t cmd, void *arg)
{
    return AVDK_ERR_OK;
}

avdk_err_t bk_display_dual_spi_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_dual_spi_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    private_display_dual_spi_ctlr_t *controller = os_malloc(sizeof(private_display_dual_spi_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_display_dual_spi_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_display_dual_spi_ctlr_config_t));

    controller->disp_buffer1 = os_malloc(sizeof(frame_buffer_t));
    controller->disp_buffer2 = os_malloc(sizeof(frame_buffer_t));
    AVDK_RETURN_ON_FALSE(controller->disp_buffer1, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    AVDK_RETURN_ON_FALSE(controller->disp_buffer2, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller->disp_buffer1, 0, sizeof(frame_buffer_t));
    os_memset(controller->disp_buffer2, 0, sizeof(frame_buffer_t));

    bk_display_spi_ctlr_new(&controller->disp_handle0, &controller->config.lcd0_config);
    bk_display_spi_ctlr_new(&controller->disp_handle1, &controller->config.lcd1_config);

    controller->ops.open = dual_spi_display_ctlr_open;
    controller->ops.close = dual_spi_display_ctlr_close;
    controller->ops.delete = dual_spi_display_ctlr_delete;
    controller->ops.flush = dual_spi_display_ctlr_flush;
    controller->ops.ioctl = dual_spi_display_ctlr_ioctl;

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}