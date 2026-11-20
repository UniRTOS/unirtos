#include <stdint.h>
#include <os/os.h>
#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/media_types.h"

#include "components/bk_jpeg_decode/bk_jpeg_decode_sw.h"

#include "bk_jpeg_decode_ctlr.h"
#include "sw_jpeg_decode_cp1.h"
#include "sw_jpeg_decode_cp2.h"
#include "sw_jpeg_decode_dual_core.h"

#define TAG "dec_ctlr"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

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

static void sw_jpeg_decode_out_complete(bk_jpeg_decode_sw_out_format_t format_type, uint32_t result, frame_buffer_t *out_frame, private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
    LOGV("%s %d out_frame %p\n", __func__, __LINE__, out_frame);
    if (controller->config.decode_cbs.out_complete != NULL)
    {
        controller->config.decode_cbs.out_complete(format_type, result, out_frame);
    }
}

static void sw_jpeg_decode_in_complete(frame_buffer_t *in_frame, private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
    LOGV("%s %d in_frame %p\n", __func__, __LINE__, in_frame);
    if (controller->config.decode_cbs.in_complete != NULL)
    {
        controller->config.decode_cbs.in_complete(in_frame);
    }
    else
    {
        LOGE("%s %d in_complete is NULL\n", __func__, __LINE__);
    }
}

static frame_buffer_t *sw_jpeg_decode_out_malloc(private_jpeg_decode_sw_multi_core_ctlr_t *controller, uint32_t size)
{
    frame_buffer_t *out_frame = NULL;
    if (controller->config.decode_cbs.out_malloc != NULL)
    {
        out_frame = controller->config.decode_cbs.out_malloc(size);
    }
    LOGV("%s %d %p\n", __func__, __LINE__, out_frame);
    return out_frame;
}

static bk_err_t cp1_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame, frame_buffer_t *in_frame, void *context);
static bk_err_t cp2_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame, frame_buffer_t *in_frame, void *context);

static bk_err_t start_next_decode_cp1(private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
    bk_err_t ret = BK_OK;

    frame_buffer_t *in_frame = NULL;
    frame_buffer_t *out_frame = NULL;
    bk_jpeg_decode_img_info_t img_info = {0};
    uint32_t frame_ptr = 0;

	ret = rtos_pop_from_queue(&controller->input_queue, &frame_ptr, BEKEN_NO_WAIT);
	if (ret != BK_OK) {
		return ret;
	}

    in_frame = (frame_buffer_t *)frame_ptr;
    img_info.frame = in_frame;
    ret = bk_get_jpeg_data_info(&img_info);
    if (ret != AVDK_ERR_OK)
    {
        LOGE(" %s %d bk_get_jpeg_data_info failed %d\n", __func__, __LINE__, ret);
        sw_jpeg_decode_in_complete(in_frame, controller);
        return ret;
    }

    in_frame->width = img_info.width;
    in_frame->height = img_info.height;

    if (controller->config.out_format == JPEG_DECODE_SW_OUT_FORMAT_GRAY)
    {
        out_frame = sw_jpeg_decode_out_malloc(controller, img_info.width * img_info.height);
    }
    else if (controller->config.out_format == JPEG_DECODE_SW_OUT_FORMAT_RGB888)
    {
        out_frame = sw_jpeg_decode_out_malloc(controller, img_info.width * img_info.height * 3);
    }
    else
    {
        out_frame = sw_jpeg_decode_out_malloc(controller, img_info.width * img_info.height * 2);
    }

    if (out_frame == NULL)
    {
        LOGE(" %s %d out_malloc failed\n", __func__, __LINE__);
        ret = BK_ERR_NO_MEM;
        sw_jpeg_decode_in_complete(in_frame, controller);
        return ret;
    }
    out_frame->width = img_info.width;
    out_frame->height = img_info.height;
    controller->sw_dec_info[0].in_frame = in_frame;
    controller->sw_dec_info[0].out_frame = out_frame;
    controller->sw_dec_info[0].complete = cp1_decode_complete;
    controller->cp1_busy = 1;

    ret = software_decode_task_send_msg_cp1(SOFTWARE_DECODE_START, (uint32_t)&controller->sw_dec_info[0]);
    if (ret != AVDK_ERR_OK)
    {
        LOGE(" %s %d software_decode_task_send_msg_cp1 failed %d\n", __func__, __LINE__, ret);
        sw_jpeg_decode_in_complete(in_frame, controller);
        sw_jpeg_decode_out_complete(out_frame->fmt, BK_FAIL, out_frame, controller);
        controller->sw_dec_info[0].in_frame = NULL;
        controller->sw_dec_info[0].out_frame = NULL;
        controller->sw_dec_info[0].complete = NULL;
        controller->cp1_busy = 0;
        return ret;
    }
    return ret;
}

static bk_err_t start_next_decode_cp2(private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
    bk_err_t ret = BK_OK;

    frame_buffer_t *in_frame = NULL;
    frame_buffer_t *out_frame = NULL;
    bk_jpeg_decode_img_info_t img_info = {0};
    uint32_t frame_ptr = 0;

	ret = rtos_pop_from_queue(&controller->input_queue, &frame_ptr, BEKEN_NO_WAIT);
	if (ret != BK_OK) {
		return ret;
	}
    in_frame = (frame_buffer_t *)frame_ptr;
    img_info.frame = in_frame;
    ret = bk_get_jpeg_data_info(&img_info);
    if (ret != AVDK_ERR_OK)
    {
        LOGE(" %s %d bk_get_jpeg_data_info failed %d\n", __func__, __LINE__, ret);
        sw_jpeg_decode_in_complete(in_frame, controller);
        return ret;
    }

    // Set the image dimensions from parsed JPEG info to input and output frame buffers
    // Note: If rotation is applied, the output width and height will be reconfigured internally after rotation
    in_frame->width = img_info.width;
    in_frame->height = img_info.height;

    if (controller->config.out_format == JPEG_DECODE_SW_OUT_FORMAT_GRAY)
    {
        out_frame = sw_jpeg_decode_out_malloc(controller, img_info.width * img_info.height);
    }
    else if (controller->config.out_format == JPEG_DECODE_SW_OUT_FORMAT_RGB888)
    {
        out_frame = sw_jpeg_decode_out_malloc(controller, img_info.width * img_info.height * 3);
    }
    else
    {
        out_frame = sw_jpeg_decode_out_malloc(controller, img_info.width * img_info.height * 2);
    }
    if (out_frame == NULL)
    {
        LOGE(" %s %d out_malloc failed\n", __func__, __LINE__);
        ret = BK_ERR_NO_MEM;
        sw_jpeg_decode_in_complete(in_frame, controller);
        return ret;
    }
    out_frame->width = img_info.width;
    out_frame->height = img_info.height;
    controller->sw_dec_info[1].in_frame = in_frame;
    controller->sw_dec_info[1].out_frame = out_frame;
    controller->sw_dec_info[1].complete = cp2_decode_complete;
    controller->cp2_busy = 1;
    ret = software_decode_task_send_msg_cp2(SOFTWARE_DECODE_START, (uint32_t)&controller->sw_dec_info[1]);
    if (ret != AVDK_ERR_OK)
    {
        LOGE(" %s %d software_decode_task_send_msg_cp2 failed %d\n", __func__, __LINE__, ret);
        sw_jpeg_decode_in_complete(in_frame, controller);
        sw_jpeg_decode_out_complete(out_frame->fmt, BK_FAIL, out_frame, controller);
        controller->sw_dec_info[1].in_frame = NULL;
        controller->sw_dec_info[1].out_frame = NULL;
        controller->sw_dec_info[1].complete = NULL;
        controller->cp2_busy = 0;
        return ret;
    }

    return ret;
}

/**
 * @brief CP1 decoder callback function
 *
 * @param format_type Format type
 * @param result Decoding result
 * @param out_frame Output frame
 * @return bk_err_t Operation result
 */
static bk_err_t cp1_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame, frame_buffer_t *in_frame, void *context)
{
    LOGV("CP decode complete 1, format: %d, result: %d %p\n", format_type, result, out_frame);
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = (private_jpeg_decode_sw_multi_core_ctlr_t *)context;

    if (controller->config.core_id == JPEG_DECODE_CORE_ID_1)
    {
        sw_jpeg_decode_in_complete(in_frame, controller);
        sw_jpeg_decode_out_complete(format_type, result, out_frame, controller);

        controller->sw_dec_info[0].in_frame = NULL;
        controller->sw_dec_info[0].out_frame = NULL;
        controller->sw_dec_info[0].complete = NULL;
        controller->cp1_busy = 0;

        rtos_lock_mutex(&controller->lock);
        start_next_decode_cp1(controller);
        rtos_unlock_mutex(&controller->lock);
    }
    else
    {
        LOGE("%s %d core_id %d is not support\n", __func__, __LINE__, controller->config.core_id);
    }

    return BK_OK;
}

/**
 * @brief CP2 decoder callback function
 *
 * @param format_type Format type
 * @param result Decoding result
 * @param out_frame Output frame
 * @return bk_err_t Operation result
 */
static bk_err_t cp2_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame, frame_buffer_t *in_frame, void *context)
{
    LOGV("CP decode complete 2, format: %d, result: %d %p\n", format_type, result, out_frame);
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = (private_jpeg_decode_sw_multi_core_ctlr_t *)context;

    if (controller->config.core_id == JPEG_DECODE_CORE_ID_2)
    {
        sw_jpeg_decode_in_complete(in_frame, controller);
        sw_jpeg_decode_out_complete(format_type, result, out_frame, controller);

        controller->sw_dec_info[1].in_frame = NULL;
        controller->sw_dec_info[1].out_frame = NULL;
        controller->sw_dec_info[1].complete = NULL;
        controller->cp2_busy = 0;
        rtos_lock_mutex(&controller->lock);
        start_next_decode_cp2(controller);
        rtos_unlock_mutex(&controller->lock);    }
    else
    {
        LOGE("%s %d core_id %d is not support\n", __func__, __LINE__, controller->config.core_id);
    }
    return BK_OK;
}

/**
 * @brief 同步解码完成回调函数
 *
 * @param format_type 输出格式类型
 * @param result 解码结果
 * @param out_frame 输出帧
 * @param in_frame 输入帧
 * @param context 上下文指针，指向控制器结构体
 * @return bk_err_t 操作结果
 */
static bk_err_t sw_jpeg_decode_sync_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame, frame_buffer_t *in_frame, void *context)
{
    bk_err_t ret = BK_OK;
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = (private_jpeg_decode_sw_multi_core_ctlr_t *)context;

    LOGV("sync decode complete, format: %d, result: %d %p\n", format_type, result, out_frame);

    // 先调用原始的回调函数处理业务逻辑
    if (controller->config.core_id == JPEG_DECODE_CORE_ID_1)
    {
        sw_jpeg_decode_in_complete(in_frame, controller);
        sw_jpeg_decode_out_complete(format_type, result, out_frame, controller);
    }
    else if (controller->config.core_id == JPEG_DECODE_CORE_ID_2)
    {
        sw_jpeg_decode_in_complete(in_frame, controller);
        sw_jpeg_decode_out_complete(format_type, result, out_frame, controller);
    }
    else
    {
        LOGE("%s %d core_id %d is not support\n", __func__, __LINE__, controller->config.core_id);
        return BK_FAIL;
    }

    // 调用信号量同步
    ret = rtos_set_semaphore(&controller->sem);
    if (ret != BK_OK)
    {
        LOGE("%s %d semaphore set failed: %d\n", __func__, __LINE__, ret);
    }

    return BK_OK;
}

static avdk_err_t software_jpeg_decode_ctlr_open(bk_jpeg_decode_sw_handle_t handler)
{
    bk_err_t ret = BK_OK;
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_multi_core_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->config.core_id <= (JPEG_DECODE_CORE_ID_1 | JPEG_DECODE_CORE_ID_2), AVDK_ERR_INVAL, TAG, "core_id is not 1 or 2");

    if (controller->config.core_id == JPEG_DECODE_CORE_ID_1)
    {
        do
        {
            // 初始化同步信号量
            ret = rtos_init_semaphore(&controller->sem, 1);
            if(ret != BK_OK)
            {
                LOGE("%s %d rtos_init_semaphore failed\n", __func__, __LINE__);
                break;
            }
            ret = rtos_init_queue(&controller->input_queue, "input_queue", sizeof(uint32_t), 10);
            if(ret != BK_OK)
            {
                LOGE("%s %d rtos_init_queue failed\n", __func__, __LINE__);
                break;
            }
            ret = rtos_init_mutex(&controller->lock);
            if(ret != BK_OK)
            {
                LOGE("%s %d rtos_init_mutex failed\n", __func__, __LINE__);
                break;
            }

            ret = software_decode_task_open_cp1(controller);
            if(ret != BK_OK)
            {
                LOGE("%s %d software_decode_task_open_cp1 failed\n", __func__, __LINE__);
                break;
            }
        } while (0);
        if (ret != BK_OK)
        {
            if (controller->sem)
            {
                rtos_deinit_semaphore(&controller->sem);
                controller->sem = NULL;
            }
            if (controller->lock)
            {
                rtos_deinit_mutex(&controller->lock);
                controller->lock = NULL;
            }
            if (controller->input_queue)
            {
                while (!rtos_is_queue_empty(&controller->input_queue))
                {
                    uint32_t frame_ptr = 0;
                    ret = rtos_pop_from_queue(&controller->input_queue, &frame_ptr, BEKEN_NO_WAIT);
                    if (ret != BK_OK) {
                        continue;
                    }
                    sw_jpeg_decode_in_complete((frame_buffer_t *)frame_ptr, controller);
                }
                rtos_deinit_queue(&controller->input_queue);
                controller->input_queue = NULL;
            }
            return ret;
        }
    }
    else if (controller->config.core_id == JPEG_DECODE_CORE_ID_2)
    {
        do
        {
            // 初始化同步信号量
            ret = rtos_init_semaphore(&controller->sem, 1);
            if(ret != BK_OK)
            {
                LOGE("%s %d rtos_init_semaphore failed\n", __func__, __LINE__);
                break;
            }
            ret = rtos_init_queue(&controller->input_queue, "input_queue", sizeof(uint32_t), 10);
            if(ret != BK_OK)
            {
                LOGE("%s %d rtos_init_queue failed\n", __func__, __LINE__);
                break;
            }
            ret = rtos_init_mutex(&controller->lock);
            if(ret != BK_OK)
            {
                LOGE("%s %d rtos_init_mutex failed\n", __func__, __LINE__);
                break;
            }

            ret = software_decode_task_open_cp2(controller);
            if(ret != BK_OK)
            {
                LOGE("%s %d software_decode_task_open_cp2 failed\n", __func__, __LINE__);
                break;
            }
        } while (0);
        if (ret != BK_OK)
        {
            if (controller->sem)
            {
                rtos_deinit_semaphore(&controller->sem);
                controller->sem = NULL;
            }
            if (controller->lock)
            {
                rtos_deinit_mutex(&controller->lock);
                controller->lock = NULL;
            }
            if (controller->input_queue)
            {
                while (!rtos_is_queue_empty(&controller->input_queue))
                {
                    uint32_t frame_ptr = 0;
                    ret = rtos_pop_from_queue(&controller->input_queue, &frame_ptr, BEKEN_NO_WAIT);
                    if (ret != BK_OK) {
                        continue;
                    }
                    sw_jpeg_decode_in_complete((frame_buffer_t *)frame_ptr, controller);
                }
                rtos_deinit_queue(&controller->input_queue);
                controller->input_queue = NULL;
            }
            return ret;
        }
    }
    else if (controller->config.core_id == (JPEG_DECODE_CORE_ID_1 | JPEG_DECODE_CORE_ID_2))
    {
        ret = software_decode_task_dual_core_open(controller);
        AVDK_RETURN_ON_FALSE(ret == BK_OK, AVDK_ERR_INVAL, TAG, "software_decode_task_open failed");
    }
    else
    {
        ret = BK_FAIL;
        LOGE("%s %d Invalid core id\n", __func__, __LINE__);
        return ret;
    }

    bk_jpeg_decode_sw_out_format_t out_format = controller->config.out_format;
    bk_jpeg_decode_byte_order_t byte_order = controller->config.byte_order;

    controller->rotate_info.rotate_angle = sw_jpeg_decode_get_rotate_angle(out_format);

    if (controller->config.core_id == JPEG_DECODE_CORE_ID_1)
    {
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_ROTATE, (uint32_t)&controller->rotate_info);
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_OUT_FORMAT, out_format);
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_BYTE_ORDER, byte_order);
        controller->module_status[0].status = JPEG_DECODE_ENABLED;
    }
    else if (controller->config.core_id == JPEG_DECODE_CORE_ID_2)
    {
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_ROTATE, (uint32_t)&controller->rotate_info);
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_OUT_FORMAT, out_format);
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_BYTE_ORDER, byte_order);
        controller->module_status[1].status = JPEG_DECODE_ENABLED;
    }
    else if (controller->config.core_id == (JPEG_DECODE_CORE_ID_1 | JPEG_DECODE_CORE_ID_2))
    {
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_ROTATE, (uint32_t)&controller->rotate_info);
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_ROTATE, (uint32_t)&controller->rotate_info);
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_OUT_FORMAT, out_format);
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_OUT_FORMAT, out_format);
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_BYTE_ORDER, byte_order);
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_BYTE_ORDER, byte_order);
    }
    else
    {

    }

    return ret;
}

static avdk_err_t software_jpeg_decode_ctlr_close(bk_jpeg_decode_sw_handle_t handler)
{
    bk_err_t ret = BK_OK;
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_multi_core_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (controller->config.core_id == JPEG_DECODE_CORE_ID_1)
    {
        ret = software_decode_task_close_cp1();
        if (controller->sem)
        {
            rtos_deinit_semaphore(&controller->sem);
            controller->sem = NULL;
        }
        if (controller->lock)
        {
            rtos_deinit_mutex(&controller->lock);
            controller->lock = NULL;
        }
        if (controller->input_queue)
        {
            while (!rtos_is_queue_empty(&controller->input_queue))
            {
                uint32_t frame_ptr = 0;
                ret = rtos_pop_from_queue(&controller->input_queue, &frame_ptr, BEKEN_NO_WAIT);
                if (ret != BK_OK) {
                    continue;
                }
                sw_jpeg_decode_in_complete((frame_buffer_t *)frame_ptr, controller);
            }
            rtos_deinit_queue(&controller->input_queue);
            controller->input_queue = NULL;
        }

        controller->module_status[0].status = JPEG_DECODE_DISABLED;
    }
    else if (controller->config.core_id == JPEG_DECODE_CORE_ID_2)
    {
        ret = software_decode_task_close_cp2();
        if (controller->sem)
        {
            rtos_deinit_semaphore(&controller->sem);
            controller->sem = NULL;
        }
        if (controller->lock)
        {
            rtos_deinit_mutex(&controller->lock);
            controller->lock = NULL;
        }

        while (!rtos_is_queue_empty(&controller->input_queue))
        {
            uint32_t frame_ptr = 0;
            ret = rtos_pop_from_queue(&controller->input_queue, &frame_ptr, BEKEN_NO_WAIT);
            if (ret != BK_OK) {
                continue;
            }
            sw_jpeg_decode_in_complete((frame_buffer_t *)frame_ptr, controller);
        }
        rtos_deinit_queue(&controller->input_queue);
        controller->input_queue = NULL;

        controller->module_status[1].status = JPEG_DECODE_DISABLED;
    }
    else if (controller->config.core_id == (JPEG_DECODE_CORE_ID_1 | JPEG_DECODE_CORE_ID_2))
    {
        ret = software_decode_task_dual_core_close(controller);
    }
    else
    {
        ret = BK_FAIL;
        LOGE("%s %d Invalid core id\n", __func__, __LINE__);
        return ret;
    }

    AVDK_RETURN_ON_FALSE(ret == BK_OK, AVDK_ERR_INVAL, TAG, "software_decode_task_close failed");

    return ret;
}

static avdk_err_t software_jpeg_decode_ctlr_decode(bk_jpeg_decode_sw_handle_t handler, frame_buffer_t *in_frame, frame_buffer_t *out_frame)
{
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_multi_core_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(in_frame, AVDK_ERR_INVAL, TAG, "in_frame is NULL");
    AVDK_RETURN_ON_FALSE(out_frame, AVDK_ERR_INVAL, TAG, "out_frame is NULL");
    AVDK_RETURN_ON_FALSE(in_frame->frame, AVDK_ERR_INVAL, TAG, "in_frame frame is NULL");
    AVDK_RETURN_ON_FALSE(out_frame->frame, AVDK_ERR_INVAL, TAG, "out_frame frame is NULL");

    if (controller->config.core_id == JPEG_DECODE_CORE_ID_1)
    {
        // 使用同步回调函数替代原来的异步回调函数
        controller->sw_dec_info[0].in_frame = in_frame;
        controller->sw_dec_info[0].out_frame = out_frame;
        controller->sw_dec_info[0].complete = sw_jpeg_decode_sync_complete;
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_START, (uint32_t)&controller->sw_dec_info[0]);

        // 等待解码完成信号
        rtos_get_semaphore(&controller->sem, BEKEN_WAIT_FOREVER);
    }
    else if (controller->config.core_id == JPEG_DECODE_CORE_ID_2)
    {
        // 使用同步回调函数替代原来的异步回调函数
        controller->sw_dec_info[1].in_frame = in_frame;
        controller->sw_dec_info[1].out_frame = out_frame;
        controller->sw_dec_info[1].complete = sw_jpeg_decode_sync_complete;
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_START, (uint32_t)&controller->sw_dec_info[1]);

        // 等待解码完成信号
        rtos_get_semaphore(&controller->sem, BEKEN_WAIT_FOREVER);
    }
    else if (controller->config.core_id == (JPEG_DECODE_CORE_ID_1 | JPEG_DECODE_CORE_ID_2))
    {
        LOGE("%s %d core_id %d is not support sync decode\n", __func__, __LINE__, controller->config.core_id);
    }
    else
    {
        LOGE("%s %d Invalid core id\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t software_jpeg_decode_ctlr_decode_async(bk_jpeg_decode_sw_handle_t handler, frame_buffer_t *in_frame)
{
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_multi_core_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(in_frame, AVDK_ERR_INVAL, TAG, "in_frame is NULL");
    AVDK_RETURN_ON_FALSE(in_frame->frame, AVDK_ERR_INVAL, TAG, "in_frame frame is NULL");
    bk_err_t ret = BK_OK;

    if (controller->config.core_id == JPEG_DECODE_CORE_ID_1)
    {
        ret = rtos_push_to_queue(&controller->input_queue, &in_frame, BEKEN_NO_WAIT);
        if(ret != AVDK_ERR_OK)
        {
            sw_jpeg_decode_in_complete(in_frame, controller);
            return ret;
        }
        if (controller->cp1_busy == 1)
        {
            return ret;
        }
        rtos_lock_mutex(&controller->lock);
        start_next_decode_cp1(controller);
        rtos_unlock_mutex(&controller->lock);
    }
    else if (controller->config.core_id == JPEG_DECODE_CORE_ID_2)
    {
        ret = rtos_push_to_queue(&controller->input_queue, &in_frame, BEKEN_NO_WAIT);
        if(ret != AVDK_ERR_OK)
        {
            sw_jpeg_decode_in_complete(in_frame, controller);
            return ret;
        }
        if (controller->cp2_busy == 1)
        {
            return ret;
        }
        rtos_lock_mutex(&controller->lock);
        start_next_decode_cp2(controller);
        rtos_unlock_mutex(&controller->lock);    }
    else if (controller->config.core_id == (JPEG_DECODE_CORE_ID_1 | JPEG_DECODE_CORE_ID_2))
    {
        avdk_err_t ret = AVDK_ERR_OK;
        ret = software_decode_task_dual_core_send_frame(controller, in_frame);
        if (ret != AVDK_ERR_OK)
        {
            LOGE(" %s %d software_decode_task_dual_core_send_frame failed %d\n", __func__, __LINE__, ret);
            sw_jpeg_decode_in_complete(in_frame, controller);
            return ret;
        }
    }
    else
    {
        LOGE("%s %d Invalid core id\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t software_jpeg_decode_ctlr_delete(bk_jpeg_decode_sw_handle_t handler)
{
    private_jpeg_decode_sw_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    os_free(controller);
    return AVDK_ERR_OK;
}

static avdk_err_t software_jpeg_decode_ctlr_set_config(bk_jpeg_decode_sw_handle_t handler, bk_jpeg_decode_sw_out_frame_info_t *out_frame_info)
{
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_multi_core_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(out_frame_info, AVDK_ERR_INVAL, TAG, "out_frame_info is NULL");

    if (controller->config.out_format == out_frame_info->out_format && controller->config.byte_order == out_frame_info->byte_order)
    {
        return BK_OK;
    }
    controller->config.out_format = out_frame_info->out_format;
    controller->config.byte_order = out_frame_info->byte_order;

    bk_jpeg_decode_sw_out_format_t out_format = controller->config.out_format;
    bk_jpeg_decode_byte_order_t byte_order = controller->config.byte_order;
    controller->rotate_info.rotate_angle = sw_jpeg_decode_get_rotate_angle(out_format);
    controller->rotate_info.rotate_buf = NULL;

    if (controller->config.core_id == 1)
    {
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_ROTATE, (uint32_t)&controller->rotate_info);
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_OUT_FORMAT, out_format);
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_BYTE_ORDER, byte_order);
    }
    if (controller->config.core_id == 2)
    {
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_ROTATE, (uint32_t)&controller->rotate_info);
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_OUT_FORMAT, out_format);
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_BYTE_ORDER, byte_order);
    }
    else if (controller->config.core_id == (JPEG_DECODE_CORE_ID_1 | JPEG_DECODE_CORE_ID_2))
    {
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_ROTATE, (uint32_t)&controller->rotate_info);
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_OUT_FORMAT, out_format);
        software_decode_task_send_msg_cp1(SOFTWARE_DECODE_SET_BYTE_ORDER, byte_order);
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_ROTATE, (uint32_t)&controller->rotate_info);
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_OUT_FORMAT, out_format);
        software_decode_task_send_msg_cp2(SOFTWARE_DECODE_SET_BYTE_ORDER, byte_order);
    }
    else
    {

    }
    return AVDK_ERR_OK;
}

static avdk_err_t software_jpeg_decode_ctlr_get_img_info(bk_jpeg_decode_sw_handle_t handler, bk_jpeg_decode_img_info_t *img_info)
{
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_multi_core_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(img_info, AVDK_ERR_INVAL, TAG, "img_info is NULL");
    AVDK_RETURN_ON_FALSE(img_info->frame, AVDK_ERR_INVAL, TAG, "img_info->frame is NULL");

    return bk_get_jpeg_data_info(img_info);
}

static avdk_err_t software_jpeg_decode_ctlr_ioctl(bk_jpeg_decode_sw_handle_t handler, bk_jpeg_decode_sw_ioctl_cmd_t cmd, void *param)
{
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = __containerof(handler, private_jpeg_decode_sw_multi_core_ctlr_t, ops);
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

avdk_err_t bk_software_jpeg_decode_on_multi_core_ctlr_new(bk_jpeg_decode_sw_handle_t *handle, bk_jpeg_decode_sw_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config->core_id <= (JPEG_DECODE_CORE_ID_1 | JPEG_DECODE_CORE_ID_2), AVDK_ERR_INVAL, TAG, "core_id is not support");

    private_jpeg_decode_sw_multi_core_ctlr_t *controller = os_malloc(sizeof(private_jpeg_decode_sw_multi_core_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_jpeg_decode_sw_multi_core_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_jpeg_decode_sw_config_t));
    controller->ops.open = software_jpeg_decode_ctlr_open;
    controller->ops.close = software_jpeg_decode_ctlr_close;
    controller->ops.decode = software_jpeg_decode_ctlr_decode;
    controller->ops.decode_async = software_jpeg_decode_ctlr_decode_async;
    controller->ops.delete = software_jpeg_decode_ctlr_delete;
    controller->ops.set_config = software_jpeg_decode_ctlr_set_config;
    controller->ops.get_img_info = software_jpeg_decode_ctlr_get_img_info;
    controller->ops.ioctl = software_jpeg_decode_ctlr_ioctl;

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}
