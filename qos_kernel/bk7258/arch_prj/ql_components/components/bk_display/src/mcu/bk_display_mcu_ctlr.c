#include <driver/pwr_clk.h>
#include "components/avdk_utils/avdk_types.h"
#include "driver/lcd.h"
#include "bk_display_ctlr.h"
#include "frame_buffer.h"
#include "components/i8080_lcd_commands.h"
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
#endif

#define TAG "mcu_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifdef DISP_DIAG_DEBUG
#define DISPLAY_START()         do { GPIO_UP(GPIO_DVP_D6); } while (0)
#define DISPLAY_END()           do { GPIO_DOWN(GPIO_DVP_D6); } while (0)
#define DISPLAY_ISR_START()     do { GPIO_UP(GPIO_DVP_D7); } while (0)
#define DISPLAY_ISR_END()       do { GPIO_DOWN(GPIO_DVP_D7); } while (0)
#else
#define DISPLAY_START()
#define DISPLAY_END()
#define DISPLAY_ISR_START()
#define DISPLAY_ISR_END()
#endif

extern media_debug_t *media_debug;
extern uint32_t platform_is_in_interrupt_context(void);

#ifdef CONFIG_FREERTOS_SMP
static SPINLOCK_SECTION volatile spinlock_t display_spin_lock = SPIN_LOCK_INIT;
#endif

static inline uint32_t display_enter_critical()
{
    uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
   spin_lock(&display_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   return flags;
}

static inline void display_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
   spin_unlock(&display_spin_lock);
#endif // CONFIG_FREERTOS_SMP

   rtos_enable_int(flags);
}

// 任务消息发送函数
static bk_err_t lcd_display_task_send_msg(private_display_mcu_context_t *lcd_disp_config, uint8_t type, uint32_t param, uint32_t param2)
{
    int ret = BK_FAIL;
    display_msg_t msg;
    uint32_t isr_context = platform_is_in_interrupt_context();

    if (lcd_disp_config && lcd_disp_config->disp_task_running)
    {
        msg.event = type;
        msg.param = param;
        msg.param2 = param2;

        if (!isr_context)
        {
            rtos_lock_mutex(&lcd_disp_config->lock);
        }

        if (lcd_disp_config->disp_task_running)
        {
            ret = rtos_push_to_queue(&lcd_disp_config->queue, &msg, BEKEN_WAIT_FOREVER);

            if (ret != BK_OK)
            {
                LOGE("%s push failed\n", __func__);
            }
        }

        if (!isr_context)
        {
            rtos_unlock_mutex(&lcd_disp_config->lock);
        }

    }

    return ret;
}

static void lcd_driver_display_mcu_isr(void *args)
{
    media_debug->isr_lcd++;
    private_display_mcu_context_t *lcd_disp_config = (private_display_mcu_context_t *)args;
    if (lcd_disp_config->pingpong_frame != NULL)
    {
        media_debug->fps_lcd++;
        if (lcd_disp_config->display_frame)
        {
            if (lcd_disp_config->display_frame_cb)
            {
                lcd_disp_config->display_frame_cb(lcd_disp_config->display_frame);
            }
            lcd_disp_config->display_frame = NULL;
            lcd_disp_config->display_frame_cb = NULL;
        }
        uint32_t flag = 0;
        flag = display_enter_critical();
        lcd_disp_config->display_frame = lcd_disp_config->pingpong_frame;
        lcd_disp_config->display_frame_cb = lcd_disp_config->pingpong_frame_cb;
        lcd_disp_config->pingpong_frame = NULL;
        lcd_disp_config->pingpong_frame_cb = NULL;
        display_exit_critical(flag);
        bk_lcd_8080_start_transfer(0);

        rtos_set_semaphore(&lcd_disp_config->disp_sem);
    }
}

// 显示帧处理函数
static bk_err_t lcd_display_frame(private_display_mcu_context_t *lcd_disp_config, frame_buffer_t *frame, flush_complete_t cb)
{
    bk_err_t ret = BK_FAIL;
    DISPLAY_START();
    if (lcd_disp_config->display_frame == NULL)
    {
        lcd_driver_ppi_set(frame->width, frame->height);
        bk_lcd_set_yuv_mode(frame->fmt);
        uint32_t flag = 0;
        flag = display_enter_critical();
        lcd_disp_config->pingpong_frame = frame;
        lcd_disp_config->pingpong_frame_cb = cb;
        display_exit_critical(flag);

        lcd_driver_set_display_base_addr((uint32_t)frame->frame);
        lcd_driver_display_enable();
        if (lcd_disp_config->lcd_device->mcu->start_transfer)
        {
            lcd_disp_config->lcd_device->mcu->start_transfer(lcd_disp_config->i80_bus_handle);
        }
        else
        {
            bk_lcd_8080_send_cmd(I8080_LCD_CMD_RAMWR, NULL, 0);
        }
        LOGD("display start, frame width, height %d, %d\n", frame->width, frame->height);
    }
    else
    {
        uint32_t flag = 0;
        flag = display_enter_critical();
        if (lcd_disp_config->pingpong_frame != NULL)
        {
            if (lcd_disp_config->pingpong_frame_cb != NULL)
            {
                lcd_disp_config->pingpong_frame_cb(lcd_disp_config->pingpong_frame);
            }
            lcd_disp_config->pingpong_frame = NULL;
            lcd_disp_config->pingpong_frame_cb = NULL;
        }
        display_exit_critical(flag);

        lcd_disp_config->pingpong_frame = frame;
        lcd_disp_config->pingpong_frame_cb = cb;

        lcd_driver_set_display_base_addr((uint32_t)frame->frame);
        lcd_driver_display_continue();
        if (lcd_disp_config->lcd_device->mcu->continue_transfer)
        {
            lcd_disp_config->lcd_device->mcu->continue_transfer(lcd_disp_config->i80_bus_handle);
        }
        else
        {
            bk_lcd_8080_send_cmd(I8080_LCD_CMD_RAMWR, NULL, 0);
        }
    }
    ret = rtos_get_semaphore(&lcd_disp_config->disp_sem, BEKEN_NEVER_TIMEOUT);

    if (ret != BK_OK)
    {
        LOGE("%s semaphore get failed: %d\n", __func__, ret);
    }
    DISPLAY_END();

    return ret;
}

// 显示任务入口函数
static void lcd_display_task_entry(beken_thread_arg_t context)
{
    private_display_mcu_context_t *lcd_disp_config = (private_display_mcu_context_t *)context;
    lcd_disp_config->disp_task_running = true;
    rtos_set_semaphore(&lcd_disp_config->disp_task_sem);

    while (lcd_disp_config->disp_task_running)
    {
        display_msg_t msg;
        int ret = rtos_pop_from_queue(&lcd_disp_config->queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == BK_OK)
        {
            switch (msg.event)
            {
                case DISPLAY_FRAME_REQUEST:
                    lcd_display_frame(lcd_disp_config, (frame_buffer_t *)msg.param, (flush_complete_t)msg.param2);
                    break;
                case DISPLAY_FRAME_FREE:
                {
                    frame_buffer_t *frame = (frame_buffer_t *)msg.param;
                    if (msg.param2 != 0)
                    {
                        ((flush_complete_t)msg.param2)(frame);
                    }
                }
                    break;

                case DISPLAY_FRAME_EXIT:
                {
                    rtos_lock_mutex(&lcd_disp_config->lock);
                    lcd_disp_config->disp_task_running = false;
                    rtos_unlock_mutex(&lcd_disp_config->lock);
                    do
                    {
                        ret = rtos_pop_from_queue(&lcd_disp_config->queue, &msg, BEKEN_NO_WAIT);

                        if (ret == BK_OK)
                        {
                            if (msg.event == DISPLAY_FRAME_REQUEST || msg.event == DISPLAY_FRAME_FREE)
                            {
                                if (msg.param2 != 0)
                                {
                                    ((flush_complete_t)msg.param2)((frame_buffer_t *)msg.param);
                                }
                            }
                        }
                    }
                    while (ret == BK_OK);
                }
                goto exit;
            }
        }
    }

exit:

    lcd_disp_config->disp_task = NULL;
    rtos_set_semaphore(&lcd_disp_config->disp_task_sem);
    rtos_delete_thread(NULL);
}

// 配置释放函数
static bk_err_t lcd_display_config_free(private_display_mcu_context_t *lcd_disp_config)
{
    int ret = BK_OK;

    if (lcd_disp_config)
    {
        if (lcd_disp_config->disp_task_sem)
        {
            rtos_deinit_semaphore(&lcd_disp_config->disp_task_sem);
            lcd_disp_config->disp_task_sem = NULL;
        }

        lcd_driver_deinit();

        if (lcd_disp_config->disp_sem)
        {
            rtos_deinit_semaphore(&lcd_disp_config->disp_sem);
            lcd_disp_config->disp_sem = NULL;
        }

        if (lcd_disp_config->pingpong_frame)
        {
            LOGD("%s pingpong_frame free\n", __func__);
            if (lcd_disp_config->pingpong_frame_cb)
            {
                lcd_disp_config->pingpong_frame_cb(lcd_disp_config->pingpong_frame);
            }
            lcd_disp_config->pingpong_frame = NULL;
            lcd_disp_config->pingpong_frame_cb = NULL;
        }

        if (lcd_disp_config->display_frame)
        {
            LOGD("%s display_frame free\n", __func__);
            if (lcd_disp_config->display_frame_cb)
            {
                lcd_disp_config->display_frame_cb(lcd_disp_config->display_frame);
            }
            lcd_disp_config->display_frame = NULL;
            lcd_disp_config->display_frame_cb = NULL;
        }

        if (lcd_disp_config->lock)
        {
            rtos_deinit_mutex(&lcd_disp_config->lock);
            lcd_disp_config->lock = NULL;
        }
    }
    LOGV("%s %d\n", __func__, __LINE__);
    return ret;
}

// 任务启动函数
static bk_err_t lcd_display_task_start(private_display_mcu_context_t *context)
{
    int ret = BK_OK;

    ret = rtos_init_queue(&context->queue, "display_queue", sizeof(display_msg_t), 15);

    if (ret != BK_OK)
    {
        LOGE("%s, init display_queue failed\r\n", __func__);
        return ret;
    }

    ret = rtos_create_thread(&context->disp_task,
                            BEKEN_DEFAULT_WORKER_PRIORITY, 
                            "display_thread",
                            (beken_thread_function_t)lcd_display_task_entry,
                            1024 * 2, 
                            (beken_thread_arg_t)context);

    if (BK_OK != ret)
    {
        LOGE("%s lcd_display_thread init failed\n", __func__);
        return ret;
    }

    ret = rtos_get_semaphore(&context->disp_task_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret)
    {
        LOGE("%s disp_task_sem get failed\n", __func__);
    }

    return ret;
}

// 任务停止函数
static bk_err_t lcd_display_task_stop(private_display_mcu_context_t *context)
{
    bk_err_t ret = BK_OK;

    if (!context || context->disp_task_running == false)
    {
        LOGD("%s already stop\n", __func__);
        return ret;
    }

    lcd_display_task_send_msg(context, DISPLAY_FRAME_EXIT, 0, 0);

    ret = rtos_get_semaphore(&context->disp_task_sem, BEKEN_NEVER_TIMEOUT);

    if (BK_OK != ret)
    {
        LOGE("%s jpeg_display_sem get failed\n", __func__);
    }

    if (context->queue)
    {
        rtos_deinit_queue(&context->queue);
        context->queue = NULL;
    }

    LOGD("%s complete\n", __func__);

    return ret;
}

// 打开函数
static avdk_err_t mcu_display_ctlr_open(bk_display_ctlr_t *controller)
{
    bk_err_t ret = BK_OK;

    private_display_mcu_ctlr_t *mcu_controller = __containerof(controller, private_display_mcu_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(mcu_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
    private_display_mcu_context_t *context = &mcu_controller->mcu_context;

    // 获取LCD设备
    if (mcu_controller->config.lcd_device != NULL)
    {
        context->lcd_device = (const lcd_device_t *)mcu_controller->config.lcd_device;
    }
    else
    {
        LOGE("%s: lcd device not found\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // 设置LCD分辨率
    context->lcd_width = context->lcd_device->width;
    context->lcd_height = context->lcd_device->height;

    context->lcd_type = context->lcd_device->type;
    LOGD("%s %d lcd '%s' %d X %d \n", __func__, __LINE__, context->lcd_device->name, context->lcd_width, context->lcd_height);

    // 初始化LCD驱动
    ret = lcd_driver_init(context->lcd_device);
    if (ret != BK_OK)
    {
        LOGE("%s: lcd_driver_init fail\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    // 验证是否为MCU类型LCD
    if (context->lcd_device->type != LCD_TYPE_MCU8080)
    {
        LOGE("%s: device type is not MCU\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_driver_display_mcu_isr, context);

    context->i80_bus_handle = lcd_i80_bus_io_register(NULL);
    if (context->i80_bus_handle && context->lcd_device->init)
    {
        LOGD("%s init lcd device %s\n", __func__, context->lcd_device->name);
        context->lcd_device->init(context->i80_bus_handle);
    }

    ret = rtos_init_semaphore(&context->disp_task_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s disp_task_sem init failed: %d\n", __func__, ret);
        goto out;
    }

    ret = rtos_init_semaphore(&context->disp_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s disp_sem init failed: %d\n", __func__, ret);
        goto out;
    }

    ret = rtos_init_mutex(&context->lock);
    if (ret != BK_OK)
    {
        LOGE("%s, init mutex failed\n", __func__);
        goto out;
    }

    // 启动显示任务
    ret = lcd_display_task_start(context);
    if (ret != BK_OK)
    {
        LOGE("%s lcd_display_task_start failed: %d\n", __func__, ret);
        goto out;
    }
    media_debug->isr_lcd = 0;
    media_debug->fps_lcd = 0;
    LOGD("%s %d complete\n", __func__, __LINE__);
    return AVDK_ERR_OK;

out:
    lcd_display_config_free(context);
    LOGE("%s failed\r\n", __func__);
    return ret;
}

// 关闭函数
static avdk_err_t mcu_display_ctlr_close(bk_display_ctlr_t *controller)
{
    private_display_mcu_ctlr_t *mcu_controller = __containerof(controller, private_display_mcu_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(mcu_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    private_display_mcu_context_t *lcd_disp_config = &mcu_controller->mcu_context;
    if (lcd_disp_config == NULL)
    {
        LOGE("%s, have been closed!\r\n", __func__);
        return BK_OK;
    }

    // 停止显示任务
    lcd_display_task_stop(lcd_disp_config);

    if (lcd_disp_config->i80_bus_handle)
    {
        if (lcd_disp_config->lcd_device->off)
        {
            lcd_disp_config->lcd_device->off(lcd_disp_config->i80_bus_handle);
        }
        if (lcd_disp_config->i80_bus_handle->delete)
        {
            lcd_disp_config->i80_bus_handle->delete(lcd_disp_config->i80_bus_handle);
        }
        lcd_disp_config->i80_bus_handle = NULL;
    }
    // 释放配置资源
    lcd_display_config_free(lcd_disp_config);

    LOGD("%s, close complete\r\n", __func__);
    return AVDK_ERR_OK;
}

// 删除函数
static avdk_err_t mcu_display_ctlr_delete(bk_display_ctlr_t *controller)
{
    private_display_mcu_ctlr_t *mcu_controller = __containerof(controller, private_display_mcu_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(mcu_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    // 释放控制器内存
    os_free(mcu_controller);
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);
    return AVDK_ERR_OK;
}

// 刷新函数
static avdk_err_t mcu_display_ctlr_flush(bk_display_ctlr_t *controller, frame_buffer_t *frame, bk_err_t (*free_t)(void *args))
{
    private_display_mcu_ctlr_t *mcu_controller = __containerof(controller, private_display_mcu_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(mcu_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    private_display_mcu_context_t *lcd_disp_config = &mcu_controller->mcu_context;
    return lcd_display_task_send_msg(lcd_disp_config, DISPLAY_FRAME_REQUEST, (uint32_t)frame, (uint32_t)free_t);
}

static avdk_err_t mcu_display_ctlr_ioctl(bk_display_ctlr_t *controller, uint32_t cmd, void *arg)
{
    //private_display_mcu_ctlr_t *mcu_controller = __containerof(controller, private_display_mcu_ctlr_t, ops);
    //AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    //private_display_mcu_context_t *lcd_disp_config = &mcu_controller->mcu_context;

    switch (cmd)
    {
        default:
            LOGD("%s, no cmd: %d \n", __func__, cmd);
            break;
    }

    return AVDK_ERR_OK;
}

// 创建控制器函数
avdk_err_t bk_display_mcu_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_mcu_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    private_display_mcu_ctlr_t *controller = os_malloc(sizeof(private_display_mcu_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_display_mcu_ctlr_t));

    // 复制配置参数
    os_memcpy(&controller->config, config, sizeof(bk_display_mcu_ctlr_config_t));

    // 初始化操作集
    controller->ops.open = mcu_display_ctlr_open;
    controller->ops.close = mcu_display_ctlr_close;
    controller->ops.delete = mcu_display_ctlr_delete;
    controller->ops.flush = mcu_display_ctlr_flush;
    controller->ops.ioctl = mcu_display_ctlr_ioctl;

    // 返回控制器句柄
    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}