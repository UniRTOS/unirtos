#include <stdint.h>
#include <driver/lcd_qspi.h>
#include <driver/pwr_clk.h>
#include <components/media_types.h>
#include "bk_display_ctlr.h"

#define TAG "qspi_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef enum {
    LCD_QSPI_DISP_REQUEST = 0,
    LCD_QSPI_DISP_EXIT,
} lcd_qspi_display_msg_type_t;

typedef struct {
    uint32_t event;
    uint32_t param0;
    uint32_t param1;
} lcd_qspi_display_msg_t;

static bk_err_t lcd_qspi_display_task_send_msg(private_display_qspi_context_t *context, uint8_t type, uint32_t param0, uint32_t param1)
{
    bk_err_t ret = BK_OK;
    lcd_qspi_display_msg_t msg;

    if (context && context->disp_task_running) {
        msg.event = type;
        msg.param0 = param0;
        msg.param1 = param1;

        ret = rtos_push_to_queue(&context->queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret != BK_OK) {
            LOGE("%s push failed\n", __func__);
        }
    }

    return ret;
}

static void lcd_qspi_display_complete_handler(frame_buffer_t *frame, flush_complete_t frame_complete_cb)
{
    if (frame && frame_complete_cb) {
        frame_complete_cb(frame);
    }
}

static void lcd_qspi_display_task_entry(beken_thread_arg_t arg)
{
    private_display_qspi_context_t *context = (private_display_qspi_context_t *)arg;
    context->disp_task_running = true;
    rtos_set_semaphore(&context->disp_task_sem);

    while (context->disp_task_running)
    {
        lcd_qspi_display_msg_t msg;
        int ret = rtos_pop_from_queue(&context->queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == BK_OK) {
            switch (msg.event) {
                case LCD_QSPI_DISP_REQUEST:
                    if (context->lcd_display_flag) {
                        bk_lcd_qspi_wait_display_complete(context->qspi_id, context->device);
                        lcd_qspi_display_complete_handler(context->display_frame, context->display_frame_cb);
                        context->lcd_display_flag = false;
                    }
                    context->display_frame = (frame_buffer_t *)msg.param0;
                    context->display_frame_cb = (flush_complete_t)msg.param1;
                    bk_lcd_qspi_frame_display(context->qspi_id, context->device, (uint32_t *)context->display_frame->frame, context->display_frame->size);
                    if (context->device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_LINE) {
                        bk_lcd_qspi_wait_display_complete(context->qspi_id, context->device);
                        while(1) {
                            ret = rtos_pop_from_queue(&context->queue, &msg, 4);
                            if (ret == BK_OK) {
                                if (msg.event == LCD_QSPI_DISP_EXIT) {
                                    lcd_qspi_display_task_send_msg(context, LCD_QSPI_DISP_EXIT, 0, 0);
                                    break;
                                } else {
                                    lcd_qspi_display_complete_handler(context->display_frame, context->display_frame_cb);
                                    context->display_frame = (frame_buffer_t *)msg.param0;
                                    context->display_frame_cb = (flush_complete_t)msg.param1;
                                }
                            }
                            bk_lcd_qspi_frame_display(context->qspi_id, context->device, (uint32_t *)context->display_frame->frame, context->display_frame->size);
                            bk_lcd_qspi_wait_display_complete(context->qspi_id, context->device);
                        }
                    } else {
                        context->lcd_display_flag = true;
                    }
                    break;

                case LCD_QSPI_DISP_EXIT:
                    if (context->lcd_display_flag) {
                        bk_lcd_qspi_wait_display_complete(context->qspi_id, context->device);
                        lcd_qspi_display_complete_handler(context->display_frame, context->display_frame_cb);
                        context->lcd_display_flag = false;
                    }
                    context->disp_task_running = false;
                    do {
                        ret = rtos_pop_from_queue(&context->queue, &msg, BEKEN_NO_WAIT);
                        if (ret == BK_OK) {
                            if (msg.event == DISPLAY_FRAME_REQUEST) {
                                if (msg.param1 != 0) {
                                    ((flush_complete_t)msg.param1)((frame_buffer_t *)msg.param0);
                                }
                            }
                        }
                    } while (ret == BK_OK);
                    break;
            }
        } else {
            if (context->lcd_display_flag) {
                bk_lcd_qspi_wait_display_complete(context->qspi_id, context->device);
                lcd_qspi_display_complete_handler(context->display_frame, context->display_frame_cb);
                context->lcd_display_flag = false;
            }
        }
    }

    context->disp_task = NULL;
    rtos_set_semaphore(&context->disp_task_sem);
    rtos_delete_thread(NULL);
}

static bk_err_t bk_lcd_qspi_display_open(private_display_qspi_context_t *context, bk_display_qspi_ctlr_config_t *config)
{
    bk_err_t ret = BK_OK;

    if (context->disp_task_running) {
        LOGW("%s have opened\r\n", __func__);
        return BK_OK;
    }

    context->qspi_id = config->qspi_id;
    context->device = config->lcd_device;
    context->reset_pin = config->reset_pin;
    ret = bk_lcd_qspi_init(context->qspi_id, context->device, context->reset_pin);
    if (ret != BK_OK) {
        LOGE("%s bk_lcd_qspi_init failed\n", __func__);
        return ret;
    }

    ret = rtos_init_semaphore(&context->disp_task_sem, 1);
    if (ret != BK_OK) {
        LOGE("%s disp_task_sem init failed: %d\n", __func__, ret);
        goto out;
    }

    ret = rtos_init_mutex(&context->lock);
    if (ret != BK_OK) {
        LOGE("%s lock init failed: %d\n", __func__, ret);
        goto out;
    }

    ret = rtos_init_queue(&context->queue,
                          "qspi_disp_queue",
                          sizeof(lcd_qspi_display_msg_t),
                          20);

    if (ret != BK_OK) {
        LOGE("%s, init display_queue failed\r\n", __func__);
        goto out;
    }

    ret = rtos_create_thread(&context->disp_task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "qspi_disp_thread",
                             (beken_thread_function_t)lcd_qspi_display_task_entry,
                             1024 * 2,
                             (beken_thread_arg_t)context);

    if (BK_OK != ret) {
        LOGE("%s lcd_display_thread init failed\n", __func__);
        goto out;
    }

    ret = rtos_get_semaphore(&context->disp_task_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret) {
        LOGE("%s disp_task_sem get failed\n", __func__);
        goto out;
    }

    return ret;

out:
    bk_lcd_qspi_deinit(context->qspi_id, context->reset_pin);

    if (context->disp_task_sem) {
        rtos_deinit_semaphore(&context->disp_task_sem);
        context->disp_task_sem = NULL;
    }

    if (context->queue) {
        rtos_deinit_queue(&context->queue);
        context->queue = NULL;
    }

    if (context->lock) {
        rtos_deinit_mutex(&context->lock);
        context->lock = NULL;
    }

    if (context->disp_task) {
        rtos_delete_thread(&context->disp_task);
        context->disp_task = NULL;
    }

    return ret;
}

static void bk_lcd_qspi_display_close(private_display_qspi_context_t *context)
{
    bk_err_t ret = BK_OK;

    if (context->disp_task_running == false) {
        LOGW("%s have closed\r\n", __func__);
        return;
    }

    lcd_qspi_display_task_send_msg(context, LCD_QSPI_DISP_EXIT, 0, 0);

    ret = rtos_get_semaphore(&context->disp_task_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret)
    {
        LOGE("%s disp_task_sem get failed\n", __func__);
        return;
    }

    bk_lcd_qspi_deinit(context->qspi_id, context->reset_pin);

    if (context->queue) {
        rtos_deinit_queue(&context->queue);
        context->queue = NULL;
    }

    if (context->lock) {
        rtos_deinit_mutex(&context->lock);
        context->lock = NULL;
    }

    if (context->disp_task_sem) {
        rtos_deinit_semaphore(&context->disp_task_sem);
        context->disp_task_sem = NULL;
    }
}

static avdk_err_t qspi_display_ctlr_open(bk_display_ctlr_t *controller)
{
    private_display_qspi_ctlr_t *qspi_controller = __containerof(controller, private_display_qspi_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(qspi_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_480M);

    bk_lcd_qspi_display_open(&qspi_controller->qspi_context, &qspi_controller->config);

    return AVDK_ERR_OK;
}

static avdk_err_t qspi_display_ctlr_close(bk_display_ctlr_t *controller)
{
    private_display_qspi_ctlr_t *qspi_controller = __containerof(controller, private_display_qspi_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(qspi_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    bk_lcd_qspi_display_close(&qspi_controller->qspi_context);

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);

    return AVDK_ERR_OK;
}

static avdk_err_t qspi_display_ctlr_delete(bk_display_ctlr_t *controller)
{
    private_display_qspi_ctlr_t *qspi_controller = __containerof(controller, private_display_qspi_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(qspi_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    os_free(qspi_controller);

    return AVDK_ERR_OK;
}

static avdk_err_t qspi_display_ctlr_flush(bk_display_ctlr_t *controller, frame_buffer_t *frame, bk_err_t (*free_t)(void *args))
{
    private_display_qspi_ctlr_t *qspi_controller = __containerof(controller, private_display_qspi_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(qspi_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    return lcd_qspi_display_task_send_msg(&qspi_controller->qspi_context, LCD_QSPI_DISP_REQUEST, (uint32_t)frame, (uint32_t)free_t);
}

static avdk_err_t qspi_display_ctlr_ioctl(bk_display_ctlr_t *controller, uint32_t cmd, void *arg)
{
    return AVDK_ERR_OK;
}

avdk_err_t bk_display_qspi_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_qspi_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    private_display_qspi_ctlr_t *controller = os_malloc(sizeof(private_display_qspi_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_display_qspi_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_display_qspi_ctlr_config_t));
    controller->ops.open = qspi_display_ctlr_open;
    controller->ops.close = qspi_display_ctlr_close;
    controller->ops.delete = qspi_display_ctlr_delete;
    controller->ops.flush = qspi_display_ctlr_flush;
    controller->ops.ioctl = qspi_display_ctlr_ioctl;

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}