#include <avdk_types.h>
#include <avdk_check.h>
#include <driver/yuv_buf.h>
#include <driver/jpeg_enc.h>
#include <driver/dma.h>
#include <driver/pwr_clk.h>
#include "private_jpeg_encode_ctlr.h"

#define TAG "jpeg_encode_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static uint32_t s_jpeg_dma_lenght = 0;

static avdk_err_t jpeg_encode_ctlr_msg_send(private_jpeg_encode_driver_t *driver_config, private_jpeg_encode_event_t event, uint32_t param)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    private_jpeg_encode_msg_t msg = {0};
    msg.event = event;
    msg.param = param;

    if (driver_config && driver_config->task_running && driver_config->queue)
    {
        ret = rtos_push_to_queue(&driver_config->queue, &msg, BEKEN_NO_WAIT);
    }

    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s push failed\n", __func__);
    }
    return ret;
}

static void jpeg_encode_ctlr_line_done_handler(void *param)
{
    private_jpeg_encode_driver_t *driver_config = (private_jpeg_encode_driver_t *)param;
    driver_config->line_done_index++;

    if (driver_config->line_done_index < driver_config->line_done_cnt)
    {
        bk_yuv_buf_rencode_start();
    }

    if (driver_config->line_done_index >= driver_config->line_done_cnt)
    {
        return;
    }

    uint32_t node_length = driver_config->encode_node_length;
    uint8_t *src_data = driver_config->yuv_buffer->frame + node_length * (driver_config->line_done_index + 1);
    if (driver_config->line_done_index % 2 == 1)
    {
        os_memcpy(driver_config->yuv_cache, src_data, node_length);
    }
    else
    {
        os_memcpy(driver_config->yuv_cache + node_length, src_data, node_length);
    }
}

static void jpeg_encode_ctlr_encode_complete_handler(void *param)
{
    private_jpeg_encode_driver_t *driver_config = (private_jpeg_encode_driver_t *)param;
    frame_buffer_t *jpeg_frame = driver_config->jpeg_buffer;

    bk_yuv_buf_stop(JPEG_MODE);
    bk_yuv_buf_soft_reset();
    if (driver_config->encode_error)
    {
        LOGE("%s, encode error\n", __func__);
        bk_dma_stop(driver_config->dma_channel);
        rtos_set_semaphore(&driver_config->sem_encode_done);
        return;
    }

    bk_dma_flush_src_buffer(driver_config->dma_channel);
    uint32_t real_length = bk_jpeg_enc_get_frame_size();
    uint32_t recv_length = BK_JPEGENC_DMA_CACHE_SIZE - bk_dma_get_remain_len(driver_config->dma_channel);
    bk_dma_stop(driver_config->dma_channel);

    s_jpeg_dma_lenght += recv_length - BK_JPEGENC_CRC_SIZE;
    if (s_jpeg_dma_lenght != real_length)
    {
        uint32_t left_length = real_length - s_jpeg_dma_lenght;
        if (left_length != BK_JPEGENC_DMA_CACHE_SIZE)
        {
            LOGE("%s, size no match(reg-dma):%d-%d=%d\n", __func__, real_length, s_jpeg_dma_lenght, left_length);
            driver_config->encode_error = true;
        }
    }

    jpeg_frame->length = real_length;
    //jpeg_frame->timestamp = get_current_timestamp();
    //jpeg_frame->sequence = driver_config->sequence++;
    jpeg_frame->width = driver_config->width;
    jpeg_frame->height = driver_config->height;
    jpeg_frame->fmt = IMAGE_MJPEG;

    rtos_set_semaphore(&driver_config->sem_encode_done);
}

static void jpeg_encode_ctlr_task_entry(beken_thread_arg_t arg)
{
    private_jpeg_encode_driver_t *driver_config = (private_jpeg_encode_driver_t *)arg;
    driver_config->task_running = true;
    rtos_set_semaphore(&driver_config->sem);

    while (driver_config->task_running)
    {
        private_jpeg_encode_msg_t msg;
        int ret = rtos_pop_from_queue(&driver_config->queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == AVDK_ERR_OK)
        {
            switch (msg.event)
            {
                case JPEG_ENCODE_EVENT_LINE_DONE:
                    jpeg_encode_ctlr_line_done_handler(driver_config);
                    break;
                case JPEG_ENCODE_EVENT_FRAME_DONE:
                    jpeg_encode_ctlr_encode_complete_handler(driver_config);
                    break;
                case JPEG_ENCODE_EVENT_EXIT:
                    driver_config->task_running = false;
                    break;
                default:
                    break;
            }
        }
    }

    rtos_set_semaphore(&driver_config->sem);
    driver_config->thread = NULL;
    rtos_delete_thread(NULL);
}

static void jpeg_encode_ctlr_dma_finish_callback(dma_id_t id)
{
    s_jpeg_dma_lenght += BK_JPEGENC_DMA_CACHE_SIZE;
}

static void jpeg_encode_ctlr_head_output_callback(jpeg_unit_t id, void *param)
{
    // TODO: handle head output, begin to encode
    bk_yuv_buf_rencode_start();
}

static void jpeg_encode_ctlr_line_clear_callback(jpeg_unit_t id, void *param)
{
    private_jpeg_encode_driver_t *driver_config = (private_jpeg_encode_driver_t *)param;
    jpeg_encode_ctlr_msg_send(driver_config, JPEG_ENCODE_EVENT_LINE_DONE, 0);
    //LOGI("%s, %d, line done index: %d\n", __func__, __LINE__, driver_config->line_done_index);
}

static void jpeg_encode_ctlr_eof_callback(jpeg_unit_t id, void *param)
{
    private_jpeg_encode_driver_t *driver_config = (private_jpeg_encode_driver_t *)param;
    jpeg_encode_ctlr_msg_send(driver_config, JPEG_ENCODE_EVENT_FRAME_DONE, 0);
}

static void jpeg_encode_ctlr_frame_err_callback(jpeg_unit_t id, void *param)
{
    private_jpeg_encode_driver_t *driver_config = (private_jpeg_encode_driver_t *)param;
    driver_config->encode_error = true;
}

static avdk_err_t jpeg_encode_ctlr_dma_config(dma_id_t dma_channel)
{
    avdk_err_t ret = AVDK_ERR_OK;
    uint32_t encode_fifo_addr;
    bk_jpeg_enc_get_fifo_addr(&encode_fifo_addr);
    LOGI("%s, channel: %d, encode fifo addr: %d\n", __func__, dma_channel, encode_fifo_addr);

    uint32_t psram_addr = 0x60000000;// for temp init, when start encode, will be set to the actual psram addr

    dma_config_t dma_config = {0};
    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 0;
    dma_config.src.dev = DMA_DEV_JPEG;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.src.start_addr = encode_fifo_addr;
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr = psram_addr;
    dma_config.dst.end_addr = psram_addr + BK_JPEGENC_DMA_CACHE_SIZE;

    ret = bk_dma_init(dma_channel, &dma_config);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "init dma failed");
    ret = bk_dma_set_transfer_len(dma_channel, BK_JPEGENC_DMA_CACHE_SIZE);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "set transfer len failed");
    ret = bk_dma_register_isr(dma_channel, NULL, jpeg_encode_ctlr_dma_finish_callback);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "register isr failed");
    ret = bk_dma_enable_finish_interrupt(dma_channel);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "enable finish interrupt failed");

#if (CONFIG_SPE)
    ret = bk_dma_set_src_burst_len(dma_channel, BURST_LEN_SINGLE);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "set src burst len failed");
    ret = bk_dma_set_dest_burst_len(dma_channel, BURST_LEN_INC16);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "set dest burst len failed");
    bk_dma_set_src_sec_attr(dma_channel, DMA_ATTR_SEC);
    bk_dma_set_dest_sec_attr(dma_channel, DMA_ATTR_SEC);
#endif

    return ret;
}

static avdk_err_t jpeg_encode_ctlr_open(bk_jpeg_encode_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_jpeg_encode_ctlr_t *controller = __containerof(handle, private_jpeg_encode_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->state == JPEG_ENCODE_STATE_INIT, AVDK_ERR_INVAL, TAG, "control state err");
    bk_jpeg_encode_ctlr_config_t *config = &controller->config;

    private_jpeg_encode_driver_t *driver_config = os_malloc(sizeof(private_jpeg_encode_driver_t));
    AVDK_RETURN_ON_FALSE(driver_config, AVDK_ERR_NOMEM, TAG, "malloc driver failed");
    os_memset(driver_config, 0, sizeof(private_jpeg_encode_driver_t));

    driver_config->dma_channel = bk_fixed_dma_alloc(DMA_DEV_JPEG, DMA_ID_8);
    AVDK_GOTO_ON_FALSE(driver_config->dma_channel != DMA_ID_MAX, AVDK_ERR_INVAL, error, TAG, "dma channel alloc failed");

    driver_config->width = config->width;
    driver_config->height = config->height;

    driver_config->yuv_cache = controller->yuv_cache;
    driver_config->encode_node_length = config->width * BK_JPEGENC_FLEXA_LINE * 2;

    ret = rtos_init_semaphore(&driver_config->sem, 1);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, error, TAG, "init sem failed");

    ret = rtos_init_semaphore(&driver_config->sem_encode_done, 1);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, error, TAG, "init sem_encode_done failed");

    ret = rtos_init_queue(&driver_config->queue, "jpeg_encode_queue", sizeof(private_jpeg_encode_msg_t), BK_JPEGENC_QUEUE_SIZE);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, error, TAG, "init queue failed");

    ret = rtos_create_thread(&driver_config->thread, BEKEN_DEFAULT_WORKER_PRIORITY, "jpeg_encode_thread", (beken_thread_function_t)jpeg_encode_ctlr_task_entry, 1024 * 2, driver_config);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, error, TAG, "create thread failed");

    rtos_get_semaphore(&driver_config->sem, BEKEN_NEVER_TIMEOUT);

    // init yuv_buf
    yuv_buf_config_t yuv_buf_config = {0};
    yuv_buf_config.x_pixel = config->width / BK_JPEGENC_FLEXA_LINE;
    yuv_buf_config.y_pixel = config->height / BK_JPEGENC_FLEXA_LINE;
    yuv_buf_config.work_mode = JPEG_MODE;
    yuv_buf_config.base_addr = NULL;
    yuv_buf_config.yuv_mode_cfg.yuv_format = config->yuv_format;

    ret = bk_yuv_buf_init(&yuv_buf_config);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, error, TAG, "init yuv_buf failed");

    ret = bk_yuv_buf_enable_nosensor_encode_mode();
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, error, TAG, "enable nosensor encode mode failed");

    // init jpeg
    jpeg_config_t jpeg_config = {0};
    jpeg_config.x_pixel = config->width / BK_JPEGENC_FLEXA_LINE;
    jpeg_config.y_pixel = config->height / BK_JPEGENC_FLEXA_LINE;
    jpeg_config.mode = JPEG_MODE;
    ret = bk_jpeg_enc_init(&jpeg_config);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, error, TAG, "init jpeg failed");

    ret = bk_yuv_buf_set_em_base_addr((uint32_t)driver_config->yuv_cache);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, error, TAG, "set em base addr failed");

    /* register jpeg callback */
    bk_jpeg_enc_register_isr(JPEG_HEAD_OUTPUT, jpeg_encode_ctlr_head_output_callback, driver_config);
    bk_jpeg_enc_register_isr(JPEG_LINE_CLEAR, jpeg_encode_ctlr_line_clear_callback, driver_config);
    bk_jpeg_enc_register_isr(JPEG_EOF, jpeg_encode_ctlr_eof_callback, driver_config);
    bk_jpeg_enc_register_isr(JPEG_FRAME_ERR, jpeg_encode_ctlr_frame_err_callback, driver_config);

    // init dma
    ret = jpeg_encode_ctlr_dma_config(driver_config->dma_channel);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, error, TAG, "init dma failed");

    controller->driver = driver_config;

    controller->state = JPEG_ENCODE_STATE_OPENED;

    return AVDK_ERR_OK;

error:
    if (driver_config)
    {
        if (driver_config->dma_channel != DMA_ID_MAX)
        {
            bk_dma_stop(driver_config->dma_channel);
            bk_dma_deinit(driver_config->dma_channel);
            bk_dma_free(DMA_DEV_JPEG, driver_config->dma_channel);
        }
        if (driver_config->queue)
        {
            rtos_deinit_queue(&driver_config->queue);
        }
        if (driver_config->sem)
        {
            rtos_deinit_semaphore(&driver_config->sem);
        }
        if (driver_config->sem_encode_done)
        {
            rtos_deinit_semaphore(&driver_config->sem_encode_done);
        }
        os_free(driver_config);
    }

    return ret;
}

static avdk_err_t jpeg_encode_ctlr_close(bk_jpeg_encode_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_jpeg_encode_ctlr_t *controller = __containerof(handle, private_jpeg_encode_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->state == JPEG_ENCODE_STATE_OPENED, AVDK_ERR_INVAL, TAG, "control state err");

    private_jpeg_encode_driver_t *driver_config = controller->driver;
    AVDK_RETURN_ON_FALSE(driver_config, AVDK_ERR_INVAL, TAG, "driver is NULL");

    ret = jpeg_encode_ctlr_msg_send(driver_config, JPEG_ENCODE_EVENT_EXIT, 0);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "send exit msg failed");

    rtos_get_semaphore(&driver_config->sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_queue(&driver_config->queue);
    rtos_deinit_semaphore(&driver_config->sem);
    rtos_deinit_semaphore(&driver_config->sem_encode_done);

    bk_dma_stop(driver_config->dma_channel);
    bk_dma_deinit(driver_config->dma_channel);
    bk_dma_free(DMA_DEV_JPEG, driver_config->dma_channel);

    bk_jpeg_enc_deinit();
    bk_yuv_buf_deinit();

    os_free(driver_config);
    controller->driver = NULL;

    controller->state = JPEG_ENCODE_STATE_CLOSED;

    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_encode_ctlr_encode(bk_jpeg_encode_ctlr_handle_t handle, frame_buffer_t *in_frame, frame_buffer_t *out_frame)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_jpeg_encode_ctlr_t *controller = __containerof(handle, private_jpeg_encode_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->state == JPEG_ENCODE_STATE_OPENED, AVDK_ERR_INVAL, TAG, "control state err");
    AVDK_RETURN_ON_FALSE(in_frame, AVDK_ERR_INVAL, TAG, "in_frame is NULL");
    AVDK_RETURN_ON_FALSE(out_frame, AVDK_ERR_INVAL, TAG, "out_frame is NULL");
    AVDK_RETURN_ON_FALSE(in_frame->frame, AVDK_ERR_INVAL, TAG, "in_frame frame is NULL");
    AVDK_RETURN_ON_FALSE(out_frame->frame, AVDK_ERR_INVAL, TAG, "out_frame frame is NULL");

    private_jpeg_encode_driver_t *driver_config = controller->driver;
    AVDK_RETURN_ON_FALSE(driver_config, AVDK_ERR_INVAL, TAG, "driver is NULL");

    // wait for the semaphore to be set
    rtos_get_semaphore(&driver_config->sem_encode_done, BEKEN_NO_WAIT);

    driver_config->yuv_buffer = in_frame;
    driver_config->jpeg_buffer = out_frame;
    driver_config->line_done_index = 0;
    driver_config->line_done_cnt = driver_config->height / BK_JPEGENC_FLEXA_LINE;
    s_jpeg_dma_lenght = 0;

    ret = bk_dma_set_dest_addr(driver_config->dma_channel, (uint32_t)(out_frame->frame),
        (uint32_t)(out_frame->frame + out_frame->size));
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "set dest addr failed");

    ret = bk_dma_start(driver_config->dma_channel);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, ret, TAG, "start dma failed");

    // soft reset yuv buf and jpeg enc
    if (driver_config->encode_error)
    {
        bk_yuv_buf_stop(JPEG_MODE);
        bk_yuv_buf_soft_reset();
        bk_jpeg_enc_soft_reset();
        driver_config->encode_error = false;
    }

    // yuv data copy to yuv cache, yuv422 data, 2 bytes per pixel
    uint32_t encode_node_length = driver_config->width * BK_JPEGENC_FLEXA_LINE * 2;
    // first copy two flexa lines
    os_memcpy(driver_config->yuv_cache, driver_config->yuv_buffer->frame, encode_node_length * 2);

    controller->state = JPEG_ENCODE_STATE_ENCODING;
    // start jpeg enc
    bk_yuv_buf_start(JPEG_MODE);

    rtos_get_semaphore(&driver_config->sem_encode_done, BEKEN_NEVER_TIMEOUT);
    if (driver_config->encode_error)
    {
        LOGE("%s, encode error\n", __func__);
        ret = AVDK_ERR_HWERROR;
    }

    // change encode state to opened
    controller->state = JPEG_ENCODE_STATE_OPENED;

    return ret;
}

static avdk_err_t jpeg_encode_ctlr_ioctl(bk_jpeg_encode_ctlr_handle_t handle, uint32_t cmd, void *param)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_jpeg_encode_ctlr_t *controller = __containerof(handle, private_jpeg_encode_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    bk_jpeg_encode_frame_compress_t *compress = (bk_jpeg_encode_frame_compress_t *)param;
    AVDK_RETURN_ON_FALSE(compress, AVDK_ERR_INVAL, TAG, "compress is NULL");

    switch (cmd)
    {
        case JPEG_ENCODE_IOCTL_CMD_SET_COMPRESS_PARAM:
            AVDK_RETURN_ON_FALSE(compress->min_size_bytes < compress->max_size_bytes,
                                 AVDK_ERR_INVAL, TAG, "min_size_bytes must be less than max_size_bytes");
            AVDK_RETURN_ON_FALSE(compress->min_size_bytes <= 0xFFFFu && compress->max_size_bytes <= 0xFFFFu,
                                 AVDK_ERR_INVAL, TAG, "min/max size must be <= 65535 bytes");
            while (controller->state == JPEG_ENCODE_STATE_ENCODING)
            {
                rtos_delay_milliseconds(10);
            }
            ret = bk_jpeg_enc_encode_config(true, compress->min_size_bytes, compress->max_size_bytes);
            break;

        case JPEG_ENCODE_IOCTL_CMD_GET_COMPRESS_PARAM:
            ret = bk_jpeg_enc_get_encode_config(&compress->min_size_bytes, &compress->max_size_bytes);
            break;

        case JPEG_ENCODE_IOCTL_CMD_BASE:
        default:
            ret = AVDK_ERR_UNSUPPORTED;
            break;
    }

    return ret;
}

static avdk_err_t jpeg_encode_ctlr_delete(bk_jpeg_encode_ctlr_handle_t handle)
{
    private_jpeg_encode_ctlr_t *controller = __containerof(handle, private_jpeg_encode_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (controller->yuv_cache)
    {
        os_free(controller->yuv_cache);
    }

    os_free(controller);

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_EN,PM_POWER_MODULE_STATE_OFF);

    return AVDK_ERR_OK;
}

avdk_err_t bk_jpeg_encode_ctlr_new(bk_jpeg_encode_ctlr_handle_t *handle, bk_jpeg_encode_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config->width > 0 && config->height > 0, AVDK_ERR_INVAL, TAG, "width and height must be greater than 0");
    AVDK_RETURN_ON_FALSE(config->width % BK_JPEGENC_FLEXA_LINE == 0, AVDK_ERR_INVAL, TAG, "width must be multiple of 8");
    AVDK_RETURN_ON_FALSE(config->height % BK_JPEGENC_FLEXA_LINE == 0, AVDK_ERR_INVAL, TAG, "height must be multiple of 8");

    private_jpeg_encode_ctlr_t *controller = os_malloc(sizeof(private_jpeg_encode_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_jpeg_encode_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_jpeg_encode_ctlr_config_t));
    controller->yuv_cache = os_malloc(config->width * BK_JPEGENC_FLEXA_LINE * 2 * 2);
    if (controller->yuv_cache == NULL)
    {
        LOGE("%s, malloc yuv cache failed\n", __func__);
        os_free(controller);
        return AVDK_ERR_NOMEM;
    }

    LOGI("yuv cache: %p, width: %d, height: %d, yuv format: %d\n", controller->yuv_cache, config->width, config->height, config->yuv_format);

    controller->state = JPEG_ENCODE_STATE_INIT;

    controller->ops.open = jpeg_encode_ctlr_open;
    controller->ops.close = jpeg_encode_ctlr_close;
    controller->ops.encode = jpeg_encode_ctlr_encode;
    controller->ops.ioctl = jpeg_encode_ctlr_ioctl;
    controller->ops.del = jpeg_encode_ctlr_delete;

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_EN,PM_POWER_MODULE_STATE_ON);

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}
