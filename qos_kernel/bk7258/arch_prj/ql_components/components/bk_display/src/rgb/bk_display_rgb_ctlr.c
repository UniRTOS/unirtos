#include <driver/flash.h>
#include "components/avdk_utils/avdk_types.h"
#include "lcd_disp_hal.h"
#include "driver/lcd.h"
#include "bk_display_ctlr.h"
#include "frame_buffer.h"
#include <os/mem.h>
#include <modules/image_scale.h>
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
#endif
#include <driver/pwr_clk.h>
#define TAG "rgb_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifdef DISP_DIAG_DEBUG
#define DISPLAY_START()         do { GPIO_UP(GPIO_DVP_D6); } while (0)
#define DISPLAY_END()           do { GPIO_DOWN(GPIO_DVP_D6); } while (0)
#define DISPLAY_ISR_START()         do { GPIO_UP(GPIO_DVP_D7); } while (0)
#define DISPLAY_ISR_END()           do { GPIO_DOWN(GPIO_DVP_D7); } while (0)
#else
#define DISPLAY_START()
#define DISPLAY_END()
#define DISPLAY_ISR_START()
#define DISPLAY_ISR_END()
#endif

extern media_debug_t *media_debug;
#ifdef CONFIG_FREERTOS_SMP
static SPINLOCK_SECTION volatile spinlock_t display_spin_lock = SPIN_LOCK_INIT;
#endif

extern uint32_t  platform_is_in_interrupt_context(void);

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

static bk_err_t lcd_display_task_send_msg(private_display_rgb_context_t *lcd_disp_config, uint8_t type, uint32_t param, uint32_t param2)
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

#if CONFIG_LV_ATTRIBUTE_FAST_MEM
static void lcd_driver_display_rgb_isr(void *args)
#else
__attribute__((section(".itcm_sec_code"))) static void lcd_driver_display_rgb_isr(void *args)
#endif
{
    DISPLAY_ISR_START();
    private_display_rgb_context_t *lcd_disp_config = (private_display_rgb_context_t *)args;
    media_debug->isr_lcd++;
    uint32_t flag = 0;
    if (lcd_disp_config->pingpong_frame != NULL)
    {
        if (lcd_disp_config->display_frame != NULL)
        {
            frame_buffer_t *temp_buffer = NULL;
            flush_complete_t temp_cb = NULL;
            bk_err_t ret = BK_OK;
            media_debug->fps_lcd++;

            flag = display_enter_critical();

            if (lcd_disp_config->pingpong_frame != lcd_disp_config->display_frame)
            {
                if (lcd_disp_config->display_frame->width != lcd_disp_config->pingpong_frame->width
                    || lcd_disp_config->display_frame->height != lcd_disp_config->pingpong_frame->height)
                {
                    lcd_driver_ppi_set(lcd_disp_config->pingpong_frame->width, lcd_disp_config->pingpong_frame->height);
                }
                if (lcd_disp_config->display_frame->fmt != lcd_disp_config->pingpong_frame->fmt)
                {
                    bk_lcd_set_yuv_mode(lcd_disp_config->pingpong_frame->fmt);
#if (CONFIG_RGB_FLUSH_BY_SOF)
                    lcd_hal_soft_reset();
#endif
                }

                temp_buffer = lcd_disp_config->display_frame;
                temp_cb = lcd_disp_config->display_frame_cb;
                lcd_disp_config->display_frame = NULL;
                lcd_disp_config->display_frame_cb = NULL;
            }
            lcd_disp_config->display_frame = lcd_disp_config->pingpong_frame;
            lcd_disp_config->display_frame_cb = lcd_disp_config->pingpong_frame_cb;
            lcd_disp_config->pingpong_frame = NULL;
            lcd_disp_config->pingpong_frame_cb = NULL;
            lcd_driver_set_display_base_addr((uint32_t)lcd_disp_config->display_frame->frame);
            if (temp_buffer != NULL)
            {
                ret = lcd_display_task_send_msg(lcd_disp_config, DISPLAY_FRAME_FREE, (uint32_t)temp_buffer, (uint32_t)temp_cb);
                if (ret != BK_OK)
                {
                    if (temp_cb != NULL)
                    {
                        temp_cb(temp_buffer);
                    }
                }
            }
            display_exit_critical(flag);
            rtos_set_semaphore(&lcd_disp_config->disp_sem);
        }
        else
        {
            flag = display_enter_critical();
            lcd_disp_config->display_frame = lcd_disp_config->pingpong_frame;
            lcd_disp_config->pingpong_frame = NULL;
            lcd_disp_config->display_frame_cb = lcd_disp_config->pingpong_frame_cb;
            lcd_disp_config->pingpong_frame_cb = NULL;
            display_exit_critical(flag);
            rtos_set_semaphore(&lcd_disp_config->disp_sem);
        }
    }
    DISPLAY_ISR_END();
}

static bk_err_t lcd_display_frame(private_display_rgb_context_t *lcd_disp_config, frame_buffer_t *frame, flush_complete_t cb)
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
        }
        display_exit_critical(flag);

        lcd_disp_config->pingpong_frame = frame;
        lcd_disp_config->pingpong_frame_cb = cb;
    }

    ret = rtos_get_semaphore(&lcd_disp_config->disp_sem, BEKEN_NEVER_TIMEOUT);

    if (ret != BK_OK)
    {
        LOGE("%s semaphore get failed: %d\n", __func__, ret);
    }
    DISPLAY_END();

    return ret;
}

static void lcd_display_task_entry(beken_thread_arg_t context)
{
    private_display_rgb_context_t *lcd_disp_config = (private_display_rgb_context_t *)context;
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
                    frame_buffer_t * frame = (frame_buffer_t *)msg.param;
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

static bk_err_t lcd_display_config_free(private_display_rgb_context_t *lcd_disp_config)
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

static bk_err_t lcd_display_task_start(private_display_rgb_context_t *context)
{
    int ret = BK_OK;

    ret = rtos_init_queue(&context->queue,
                          "display_queue",
                          sizeof(display_msg_t),
                          15);

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

static avdk_err_t rgb_display_ctlr_open(bk_display_ctlr_t *controller)
{
    bk_err_t ret = BK_OK;

    private_display_rgb_ctlr_t *rgb_controller = __containerof(controller, private_display_rgb_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(rgb_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD,PM_POWER_MODULE_STATE_ON);
    bk_display_rgb_ctlr_config_t *config = &rgb_controller->config;
    private_display_rgb_context_t *context = &rgb_controller->rgb_context;

    if (rgb_controller->config.lcd_device != NULL)
    {
        context->lcd_device = (const lcd_device_t *)rgb_controller->config.lcd_device;
    }
    else
    {
        LOGE("%s: lcd device not found\n", __func__);
        return AVDK_ERR_INVAL;
    }

    context->lcd_width = context->lcd_device->width;
    context->lcd_height = context->lcd_device->height;

    context->lcd_type = context->lcd_device->type;
    LOGD("%s %d lcd '%s' %d X %d \n", __func__, __LINE__, context->lcd_device->name, context->lcd_width, context->lcd_height);
    // 初始化 LCD 驱动
    ret = lcd_driver_init(context->lcd_device);
    if (ret != BK_OK)
    {
        LOGE("%s: lcd_driver_init fail\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    // RGB 接口特定初始化
    if (context->lcd_device->type != LCD_TYPE_RGB && context->lcd_device->type != LCD_TYPE_RGB565)
    {
        LOGE("%s: device type is not RGB\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    // 注册中断处理函数
#if (CONFIG_RGB_FLUSH_BY_SOF)
    bk_lcd_isr_register(RGB_OUTPUT_SOF, lcd_driver_display_rgb_isr, context);
#else
    bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_driver_display_rgb_isr, context);
#endif

    if (config->clk_pin >= 0 && config->clk_pin < GPIO_NUM && \
        config->cs_pin >= 0 && config->cs_pin < GPIO_NUM && \
        config->sda_pin >= 0 && config->sda_pin < GPIO_NUM && \
        config->rst_pin >= 0 && config->rst_pin < GPIO_NUM)
    {
        lcd_spi_io_t io = {
                .clk = config->clk_pin,
                .cs = config->cs_pin,
                .sda = config->sda_pin,
                .rst = config->rst_pin,
            };
        context->spi_bus_handle = lcd_spi_bus_io_register(&io);
        if (context->spi_bus_handle)
        {
            if (context->lcd_device->init)
            {
                if (context->spi_bus_handle->init)
                {
                    context->spi_bus_handle->init(context->spi_bus_handle);
                }
                context->lcd_device->init(context->spi_bus_handle);
            }
        }
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

static bk_err_t lcd_display_task_stop(private_display_rgb_context_t *context)
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

static avdk_err_t rgb_display_ctlr_close(bk_display_ctlr_t *controller)
{
    private_display_rgb_ctlr_t *rgb_controller = __containerof(controller, private_display_rgb_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(rgb_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    private_display_rgb_context_t *lcd_disp_config = &rgb_controller->rgb_context;
    if (lcd_disp_config == NULL)
    {
        LOGE("%s, have been closed!\r\n", __func__);
        return BK_OK;
    }

    lcd_display_task_stop(lcd_disp_config);

    if (lcd_disp_config->spi_bus_handle)
    {
        if (lcd_disp_config->lcd_device->off)
        {
            lcd_disp_config->lcd_device->off(lcd_disp_config->spi_bus_handle);
        }
        if (lcd_disp_config->spi_bus_handle->deinit)
        {
            lcd_disp_config->spi_bus_handle->deinit(lcd_disp_config->spi_bus_handle);
        }
        lcd_disp_config->spi_bus_handle = NULL;
    }

    lcd_display_config_free(lcd_disp_config);
    return AVDK_ERR_OK;
}

static avdk_err_t rgb_display_ctlr_delete(bk_display_ctlr_t *controller)
{
    private_display_rgb_ctlr_t *rgb_controller = __containerof(controller, private_display_rgb_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(rgb_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

#ifdef CONFIG_DISPLAY_RGB888_HIGH_BIT_SHIFT
        os_free(rgb_controller->rgb888_bitshift_sram);
        rgb_controller->rgb888_bitshift_sram = NULL;
#endif
    os_free(rgb_controller);
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);
    return AVDK_ERR_OK;
}

#ifdef CONFIG_DISPLAY_RGB888_HIGH_BIT_SHIFT
// Wrapper functions to call image_scale.c functions
static void rgb888_bit_shift_process_wrapper(private_display_rgb_ctlr_t *rgb_controller, uint8_t *src_psram, uint8_t *dst_psram, uint32_t width, uint32_t height)
{
    if (rgb_controller == NULL || rgb_controller->rgb888_bitshift_sram == NULL || src_psram == NULL || dst_psram == NULL || width == 0 || height == 0) {
        LOGE("%s rgb_controller or rgb_controller->rgb888_bitshift_sram or src_psram or dst_psram is NULL or width or height is 0\n", __func__);
        return;
    }
    rgb888_bit_shift_process(rgb_controller->rgb888_bitshift_sram, src_psram, dst_psram, width, height);
}

static void rgb565_process_to_rgb888_bitshift_wrapper(private_display_rgb_ctlr_t *rgb_controller, uint8_t *src_psram, uint8_t *dst_psram, uint32_t width, uint32_t height)
{
    if (rgb_controller == NULL || rgb_controller->rgb888_bitshift_sram == NULL || src_psram == NULL || dst_psram == NULL || width == 0 || height == 0) {
        LOGE("%s rgb_controller or rgb_controller->rgb888_bitshift_sram or src_psram or dst_psram is NULL or width or height is 0\n", __func__);
        return;
    }
    rgb565_process_to_rgb888_bitshift(rgb_controller->rgb888_bitshift_sram, src_psram, dst_psram, width, height);
}

static void yuyv_to_rgb888_bitshift_wrapper(private_display_rgb_ctlr_t *rgb_controller, uint8_t *src_psram, uint8_t *dst_psram, uint32_t width, uint32_t height)
{
    if (rgb_controller == NULL || rgb_controller->rgb888_bitshift_sram == NULL || src_psram == NULL || dst_psram == NULL || width == 0 || height == 0) {
        LOGE("%s rgb_controller or rgb_controller->rgb888_bitshift_sram or src_psram or dst_psram is NULL or width or height is 0\n", __func__);
        return;
    }
    yuyv_to_rgb565_process_to_rgb888_bitshift(rgb_controller->rgb888_bitshift_sram, src_psram, dst_psram, width, height);
}
static bk_err_t private_rgb888_process_free_cb(void *frame)
{
    frame_buffer_display_free(frame);
    return BK_OK;
}
#endif

static avdk_err_t rgb_display_ctlr_flush(bk_display_ctlr_t *controller, frame_buffer_t *frame, bk_err_t (*free_t)(void *args))
{
    private_display_rgb_ctlr_t *rgb_controller = __containerof(controller, private_display_rgb_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(rgb_controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    private_display_rgb_context_t *lcd_disp_config = &rgb_controller->rgb_context;
#ifdef CONFIG_DISPLAY_RGB888_HIGH_BIT_SHIFT
    // Get destination frame buffer for pixel conversion
    frame_buffer_t *dst_frame = NULL;
    dst_frame = frame_buffer_display_malloc(frame->width * frame->height * 3);
    if (dst_frame == NULL) {
        LOGE("%s dst_frame malloc failed, release source frame\n", __func__);
        // Release source frame if dst_frame allocation failed
        if (free_t != NULL) {
            free_t(frame);
        }
        return AVDK_ERR_NOMEM;
    }

    // Initialize dst_frame format information
    dst_frame->width = frame->width;
    dst_frame->height = frame->height;
    //dst_frame->fmt = frame->fmt;
    // Convert pixels from source frame to dst_frame
    if (frame->fmt == PIXEL_FMT_RGB565) {
        rgb565_process_to_rgb888_bitshift_wrapper(rgb_controller, frame->frame, dst_frame->frame, frame->width, frame->height);
        dst_frame->fmt = PIXEL_FMT_RGB888;
    } else if (frame->fmt == PIXEL_FMT_RGB888) {
        rgb888_bit_shift_process_wrapper(rgb_controller, frame->frame, dst_frame->frame, frame->width, frame->height);
        dst_frame->fmt = PIXEL_FMT_RGB888;
    } else if (frame->fmt == PIXEL_FMT_YUYV) {
        yuyv_to_rgb888_bitshift_wrapper(rgb_controller, frame->frame, dst_frame->frame, frame->width, frame->height);
        dst_frame->fmt = PIXEL_FMT_RGB888;
    } else {
        LOGE("%s frame->fmt %d is not supported, release dst_frame\n", __func__, frame->fmt);
        // Release dst_frame if frame->fmt is not supported
        if (free_t != NULL) {
            free_t(frame);
        }
        private_rgb888_process_free_cb(dst_frame);
        return AVDK_ERR_INVAL;
    }

    // Release source frame immediately after conversion (no need to wait for display)
    if (free_t != NULL) {
        free_t(frame);
    }
    // Flush dst_frame to display, callback will release it after display completes
    bk_err_t ret = lcd_display_task_send_msg(lcd_disp_config, DISPLAY_FRAME_REQUEST, (uint32_t)dst_frame, (uint32_t)private_rgb888_process_free_cb);
    if (ret != BK_OK) {
        LOGE("%s lcd_display_task_send_msg failed, ret:%d, release dst_frame manually\n", __func__, ret);
        // If send failed, manually release dst_frame
        private_rgb888_process_free_cb(dst_frame);
        return AVDK_ERR_GENERIC;
    }
    return AVDK_ERR_OK;
#endif
    return lcd_display_task_send_msg(lcd_disp_config, DISPLAY_FRAME_REQUEST, (uint32_t)frame, (uint32_t)free_t);
}

static avdk_err_t rgb_display_ctlr_ioctl(bk_display_ctlr_t *controller, uint32_t cmd, void *arg)
{
    switch (cmd)
    {
        default:
            LOGD("%s, no cmd: %d \n", __func__, cmd);
            break;
    }

    return AVDK_ERR_OK;
}


avdk_err_t bk_display_rgb_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_rgb_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    private_display_rgb_ctlr_t *controller = os_malloc(sizeof(private_display_rgb_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_display_rgb_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_display_rgb_ctlr_config_t));
   
#ifdef CONFIG_DISPLAY_RGB888_HIGH_BIT_SHIFT
    controller->rgb888_bitshift_sram = os_malloc(config->lcd_device->width * 3);
    if (controller->rgb888_bitshift_sram == NULL) {
        LOGE("%s controller->rgb888_bitshift_sram malloc failed\n", __func__);
        os_free(controller);
        return AVDK_ERR_NOMEM;
    }
#endif

    controller->ops.open = rgb_display_ctlr_open;
    controller->ops.close = rgb_display_ctlr_close;
    controller->ops.delete = rgb_display_ctlr_delete;
    controller->ops.flush = rgb_display_ctlr_flush;
    controller->ops.ioctl = rgb_display_ctlr_ioctl;

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}
