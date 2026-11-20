#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/bk_video_recorder.h"
#include "bk_video_recorder_ctlr.h"
#include "bk_video_recorder_avi.h"
#include "bk_video_recorder_mp4.h"


#define TAG "video_recorder_ctlr"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


#define VIDEO_RECORDER_AUDIO_BURST_MAX    (8)

void *video_recorder_psram_malloc(uint32_t size)
{
    return psram_malloc(size);
}

void video_recorder_psram_free(void *ptr)
{
    psram_free(ptr);
}

// Recording thread function
static void video_recorder_thread(void *arg)
{
    private_video_recorder_ctlr_t *controller = (private_video_recorder_ctlr_t *)arg;
    avdk_err_t ret = AVDK_ERR_OK;
    video_recorder_frame_data_t frame_data = {0};
    video_recorder_audio_data_t audio_data = {0};
    
    LOGI("%s: Recording thread started\n", __func__);
    
    // Main thread loop - wait for start signals and process recording
    while (!controller->thread_exit)
    {
        // Wait for start signal
        rtos_get_semaphore(&controller->thread_sem, BEKEN_WAIT_FOREVER);
        
        // Check if thread should exit (in case close was called)
        if (controller->thread_exit)
        {
            LOGI("%s: Recording thread exiting (close called)\n", __func__);
            break;
        }
        
        // Check if thread should run
        if (!controller->thread_running)
        {
            // Thread was woken but not running, wait for next start signal
            continue;
        }
        
        LOGI("%s: Recording thread activated\n", __func__);
        
        // Main recording loop
        while (controller->thread_running && !controller->thread_exit)
        {
            bool processed = false;

            // Prefer audio: drain a bounded number of audio packets first.
            if (controller->config.get_audio_cb != NULL)
            {
                for (uint32_t i = 0; i < VIDEO_RECORDER_AUDIO_BURST_MAX; i++)
                {
                    ret = controller->config.get_audio_cb(controller->config.user_data, &audio_data);
                    if (ret != 0 || audio_data.data == NULL || audio_data.length == 0)
                    {
                        // No more audio available at the moment.
                        break;
                    }

                    // For H264, drop audio until the first IDR frame is written.
                    if (controller->config.record_format == VIDEO_RECORDER_FORMAT_H264 &&
                        !controller->video_idr_started)
                    {
                        if (controller->config.release_audio_cb != NULL)
                        {
                            controller->config.release_audio_cb(controller->config.user_data, &audio_data);
                        }
                        os_memset(&audio_data, 0, sizeof(audio_data));
                        processed = true;
                        continue;
                    }

                    // For H264, drop audio until the first IDR frame is written.
                    if (controller->config.record_format == VIDEO_RECORDER_FORMAT_H264 &&
                        !controller->video_idr_started)
                    {
                        if (controller->config.release_audio_cb != NULL)
                        {
                            controller->config.release_audio_cb(controller->config.user_data, &audio_data);
                        }
                        os_memset(&audio_data, 0, sizeof(audio_data));
                        processed = true;
                        continue;
                    }

                    // Write audio data
                    avdk_err_t write_ret = AVDK_ERR_OK;
                    LOGV("%s: Writing audio data, length=%d\n", __func__, audio_data.length);
                    if (controller->config.record_type == VIDEO_RECORDER_TYPE_AVI)
                    {
                        write_ret = avi_write_audio_data(controller, audio_data.data, audio_data.length);
                    }
                    else if (controller->config.record_type == VIDEO_RECORDER_TYPE_MP4)
                    {
                        if (controller->mp4_audio_packetize_enable)
                        {
                            // Split variable-size fixed-frame audio packets into fixed 20ms frames.
                            // This avoids a first 40ms packet (e.g., 320 bytes) breaking MP4 STTS.
                            if (controller->mp4_audio_packetize_partial_buf == NULL ||
                                controller->mp4_audio_packetize_partial_buf_size < controller->mp4_audio_packetize_frame_bytes)
                            {
                                LOGE("%s: mp4 audio packetize buffer is not ready, buf=%p, buf_size=%u, frame=%u\n",
                                     __func__,
                                     controller->mp4_audio_packetize_partial_buf,
                                     (unsigned)controller->mp4_audio_packetize_partial_buf_size,
                                     (unsigned)controller->mp4_audio_packetize_frame_bytes);
                                write_ret = AVDK_ERR_NOMEM;
                            }

                            uint32_t off = 0;
                            while (write_ret == AVDK_ERR_OK && off < audio_data.length)
                            {
                                uint32_t need = controller->mp4_audio_packetize_frame_bytes - controller->mp4_audio_packetize_partial_used;
                                uint32_t copy = (audio_data.length - off < need) ? (audio_data.length - off) : need;
                                os_memcpy(controller->mp4_audio_packetize_partial_buf + controller->mp4_audio_packetize_partial_used,
                                          audio_data.data + off,
                                          copy);
                                controller->mp4_audio_packetize_partial_used += copy;
                                off += copy;

                                if (controller->mp4_audio_packetize_partial_used == controller->mp4_audio_packetize_frame_bytes)
                                {
                                    write_ret = mp4_write_audio_data(controller,
                                                                    controller->mp4_audio_packetize_partial_buf,
                                                                    controller->mp4_audio_packetize_frame_bytes);
                                    if (write_ret != AVDK_ERR_OK)
                                    {
                                        break;
                                    }
                                    controller->mp4_audio_packetize_partial_used = 0;
                                }
                            }
                        }
                        else
                        {
                            write_ret = mp4_write_audio_data(controller, audio_data.data, audio_data.length);
                        }
                    }

                    if (write_ret != AVDK_ERR_OK)
                    {
                        LOGE("%s: Failed to write audio data, ret=%d\n", __func__, write_ret);
                    }

                    // Release audio buffer after writing (always release, even if write failed)
                    if (controller->config.release_audio_cb != NULL)
                    {
                        controller->config.release_audio_cb(controller->config.user_data, &audio_data);
                    }

                    // Clear audio_data after release to avoid reusing old data
                    os_memset(&audio_data, 0, sizeof(audio_data));
                    processed = true;
                }
            }

            // Get video frame if callback is registered
            if (controller->config.get_frame_cb != NULL)
            {
                ret = controller->config.get_frame_cb(controller->config.user_data, &frame_data);
                if (ret == 0 && frame_data.data != NULL && frame_data.length > 0)
                {
                    controller->last_is_key_frame = (frame_data.is_key_frame != 0);
                    if (controller->config.record_format == VIDEO_RECORDER_FORMAT_H264 &&
                        !controller->video_idr_started)
                    {
                        if (!controller->last_is_key_frame)
                        {
                            if (controller->config.release_frame_cb != NULL)
                            {
                                controller->config.release_frame_cb(controller->config.user_data, &frame_data);
                            }
                            os_memset(&frame_data, 0, sizeof(frame_data));
                            processed = true;
                            continue;
                        }

                        controller->video_idr_started = true;
                        controller->recording_start_time_ms = rtos_get_time();
                        controller->video_frame_count = 0;
                        controller->mp4_video_frame_count = 0;
                        controller->first_saved_frame_time_ms = 0;
                        controller->last_saved_frame_time_ms = 0;
                    }

                    // Write video frame
                    avdk_err_t write_ret = AVDK_ERR_OK;
                    if (controller->config.record_type == VIDEO_RECORDER_TYPE_AVI)
                    {
                        write_ret = avi_write_video_frame(controller, frame_data.data, frame_data.length);
                    }
                    else if (controller->config.record_type == VIDEO_RECORDER_TYPE_MP4)
                    {
                        write_ret = mp4_write_video_frame(controller, frame_data.data, frame_data.length);
                    }
                    
                    if (write_ret != AVDK_ERR_OK)
                    {
                        LOGE("%s: Failed to write video frame, ret=%d\n", __func__, write_ret);
                    }
                    
                    // Release frame buffer after writing (always release, even if write failed)
                    if (controller->config.release_frame_cb != NULL)
                    {
                        controller->config.release_frame_cb(controller->config.user_data, &frame_data);
                    }
                    
                    // Clear frame_data after release to avoid reusing old data
                    os_memset(&frame_data, 0, sizeof(frame_data));
                    processed = true;
                }
            }

            if (!processed)
            {
                rtos_delay_milliseconds(5);
            }
        }
        
        // Recording stopped, notify stop function that current operation is finished
        LOGI("%s: Recording thread stopped, waiting for next start\n", __func__);
        rtos_set_semaphore(&controller->stop_sem);
    }
    
    LOGI("%s: Recording thread exiting\n", __func__);
    
    // Thread will exit, rtos_thread_join in close() will detect it
    rtos_delete_thread(NULL);
}

static avdk_err_t video_recorder_ctlr_open(bk_video_recorder_ctlr_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_video_recorder_ctlr_t *controller = __containerof(handler, private_video_recorder_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == VIDEO_RECORDER_STATUS_NONE, AVDK_ERR_INVAL, TAG, "video record is not opened");

    // Initialize recording handles
    controller->avi_handle = NULL;
    controller->mp4_handle = NULL;
    controller->mp4_video_frame_count = 0;
    controller->mp4_audio_sample_count = 0;
    controller->mp4_mdat_offset = 0;
    controller->mp4_mdat_size = 0;
    controller->record_thread = NULL;
    controller->thread_running = false;
    controller->thread_exit = false;
    // Initialize dynamic frame rate tracking
    controller->video_frame_count = 0;
    controller->recording_start_time_ms = 0;
    controller->last_frame_time_ms = 0;
    controller->first_saved_frame_time_ms = 0;
    controller->last_saved_frame_time_ms = 0; // 0 means first frame should be saved
    controller->mp4_audio_packetize_enable = false;
    controller->mp4_audio_packetize_frame_bytes = 0;
    controller->mp4_audio_packetize_partial_used = 0;
    controller->mp4_audio_packetize_partial_buf = NULL;
    controller->mp4_audio_packetize_partial_buf_size = 0;

    // Initialize semaphore for thread control
    ret = rtos_init_semaphore(&controller->thread_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s: Failed to init thread semaphore, ret=%d\n", __func__, ret);
        return AVDK_ERR_IO;
    }

    // Initialize semaphore for stop synchronization
    ret = rtos_init_semaphore(&controller->stop_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s: Failed to init stop semaphore, ret=%d\n", __func__, ret);
        rtos_deinit_semaphore(&controller->thread_sem);
        return AVDK_ERR_IO;
    }

    // Create recording thread (thread will wait for start signal)
    beken_thread_t thread = NULL;

    ret = rtos_create_thread(&thread, CONFIG_VIDEO_RECORDER_TASK_PRIORITY, "video_recorder",
                             (beken_thread_function_t)video_recorder_thread,
                             CONFIG_VIDEO_RECORDER_TASK_STACK_SIZE,
                             (beken_thread_arg_t)controller);
    if (ret != BK_OK)
    {
        LOGE("%s: Failed to create thread, ret=%d\n", __func__, ret);
        rtos_deinit_semaphore(&controller->thread_sem);
        return AVDK_ERR_IO;
    }

    controller->record_thread = thread;
    LOGI("%s: Recording thread created\n", __func__);

    controller->module_status.status = VIDEO_RECORDER_STATUS_OPENED;

    return AVDK_ERR_OK;
}

static avdk_err_t video_recorder_ctlr_close(bk_video_recorder_ctlr_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_video_recorder_ctlr_t *controller = __containerof(handler, private_video_recorder_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == VIDEO_RECORDER_STATUS_OPENED || 
                         controller->module_status.status == VIDEO_RECORDER_STATUS_STARTED ||
                         controller->module_status.status == VIDEO_RECORDER_STATUS_STOPPED, 
                         AVDK_ERR_INVAL, TAG, "video record status is invalid");

    // Stop thread if running
    if (controller->record_thread != NULL)
    {
        // Set exit flag to signal thread to exit
        controller->thread_exit = true;
        controller->thread_running = false;
        
        // Wake up thread if it's waiting on semaphore
        rtos_set_semaphore(&controller->thread_sem);
        
        // Wait for thread to exit using rtos_thread_join (more reliable than semaphore)
        // This will poll until thread actually exits
        rtos_thread_join(controller->record_thread);
        LOGI("%s: Recording thread stopped\n", __func__);
        
        // Free thread handle
        controller->record_thread = NULL;
        
        // Deinit semaphores
        rtos_deinit_semaphore(&controller->thread_sem);
        rtos_deinit_semaphore(&controller->stop_sem);
    }

    // Close AVI recording if active
    if (controller->avi_handle != NULL)
    {
        avdk_err_t avi_ret = avi_record_close(controller);
        if (avi_ret != AVDK_ERR_OK)
        {
            LOGE("%s: avi_record_close failed, ret=%d\n", __func__, avi_ret);
            ret = avi_ret;
        }
    }
    
    // Close MP4 recording if active
    if (controller->mp4_handle != NULL)
    {
        avdk_err_t mp4_ret = mp4_record_close(controller);
        if (mp4_ret != AVDK_ERR_OK)
        {
            LOGE("%s: mp4_record_close failed, ret=%d\n", __func__, mp4_ret);
            ret = mp4_ret;
        }
    }

    if (controller->mp4_audio_packetize_partial_buf != NULL)
    {
        os_free(controller->mp4_audio_packetize_partial_buf);
        controller->mp4_audio_packetize_partial_buf = NULL;
        controller->mp4_audio_packetize_partial_buf_size = 0;
    }

    controller->module_status.status = VIDEO_RECORDER_STATUS_CLOSED;

    return ret;
}

static avdk_err_t video_recorder_ctlr_start(bk_video_recorder_ctlr_handle_t handler, char *file_path, uint32_t record_type)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_video_recorder_ctlr_t *controller = __containerof(handler, private_video_recorder_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == VIDEO_RECORDER_STATUS_OPENED || 
                         controller->module_status.status == VIDEO_RECORDER_STATUS_STOPPED, 
                         AVDK_ERR_INVAL, TAG, "video record is not in valid state for start");

    controller->file_path = file_path;
    
    // MP4 + fixed-frame audio (G.711/G.722): enforce fixed 20ms packetization at recording layer.
    // This keeps mp4lib write-mode (single STTS entry) consistent and avoids "first packet=320" issues.
    controller->mp4_audio_packetize_enable = false;
    controller->mp4_audio_packetize_frame_bytes = 0;
    controller->mp4_audio_packetize_partial_used = 0;

    if (controller->config.record_type == VIDEO_RECORDER_TYPE_MP4 &&
        (controller->config.audio_format == VIDEO_RECORDER_AUDIO_FORMAT_ALAW ||
         controller->config.audio_format == VIDEO_RECORDER_AUDIO_FORMAT_MULAW ||
         controller->config.audio_format == VIDEO_RECORDER_AUDIO_FORMAT_G722) &&
        controller->config.audio_channels > 0)
    {
        uint32_t ch = controller->config.audio_channels;
        if (ch == 0 || ch > 2)
        {
            LOGE("%s: invalid audio params, channels=%u\n", __func__, (unsigned)ch);
            return AVDK_ERR_INVAL;
        }

        uint32_t frame_bytes = 0;
        if (controller->config.audio_format == VIDEO_RECORDER_AUDIO_FORMAT_G722)
        {
            // G.722 encoder output is fixed-rate 64kbps, so:
            // bytes_per_20ms_per_channel = 64000bps / 8 / 50 = 160 bytes.
            frame_bytes = 160u * ch;
        }
        else
        {
            // For G.711, output is 8-bit per sample per channel.
            // Use 20ms framing: samples_per_20ms = sample_rate / 50.
            uint32_t sr = controller->config.audio_rate;
            if (sr == 0)
            {
                LOGE("%s: invalid G711 sample_rate=%u\n", __func__, (unsigned)sr);
                return AVDK_ERR_INVAL;
            }
            if ((sr % 50u) != 0u)
            {
                LOGE("%s: unsupported G711 sample_rate=%u for 20ms framing\n", __func__, (unsigned)sr);
                return AVDK_ERR_INVAL;
            }
            frame_bytes = (sr / 50u) * ch;
        }

        if (frame_bytes == 0)
        {
            LOGE("%s: invalid audio frame_bytes=%u\n", __func__, (unsigned)frame_bytes);
            return AVDK_ERR_INVAL;
        }

        // Reuse the packetize buffer across multiple start/stop cycles to avoid frequent malloc/free.
        // Only (re)allocate when the required frame_bytes grows beyond current buffer size.
        if (controller->mp4_audio_packetize_partial_buf == NULL ||
            controller->mp4_audio_packetize_partial_buf_size < frame_bytes)
        {
            if (controller->mp4_audio_packetize_partial_buf != NULL)
            {
                os_free(controller->mp4_audio_packetize_partial_buf);
                controller->mp4_audio_packetize_partial_buf = NULL;
                controller->mp4_audio_packetize_partial_buf_size = 0;
            }

            controller->mp4_audio_packetize_partial_buf = (uint8_t *)os_malloc(frame_bytes);
            if (controller->mp4_audio_packetize_partial_buf == NULL)
            {
                LOGE("%s: allocate mp4 audio packetize buffer failed, size=%u\n", __func__, (unsigned)frame_bytes);
                return AVDK_ERR_NOMEM;
            }
            controller->mp4_audio_packetize_partial_buf_size = frame_bytes;
        }

        controller->mp4_audio_packetize_enable = true;
        controller->mp4_audio_packetize_frame_bytes = frame_bytes;
        controller->mp4_audio_packetize_partial_used = 0;
    }

    // Initialize recording based on record type
    if (controller->config.record_type == VIDEO_RECORDER_TYPE_AVI)
    {
        ret = avi_record_start(controller, file_path);
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }
    }
    else if (controller->config.record_type == VIDEO_RECORDER_TYPE_MP4)
    {
        ret = mp4_record_start(controller, file_path);
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }
    }
    else
    {
        LOGE("%s: Unsupported record type: %d\n", __func__, controller->config.record_type);
        return AVDK_ERR_INVAL;
    }
    
    // Initialize dynamic frame rate tracking
    controller->video_frame_count = 0;
    controller->recording_start_time_ms = rtos_get_time();
    controller->last_frame_time_ms = controller->recording_start_time_ms;
    controller->first_saved_frame_time_ms = 0;
    controller->last_saved_frame_time_ms = 0; // 0 means first frame should be saved
    // For H264, delay writing until first IDR frame is detected.
    controller->wait_for_video_idr = (controller->config.record_format == VIDEO_RECORDER_FORMAT_H264);
    controller->video_idr_started = !controller->wait_for_video_idr;
    controller->last_is_key_frame = false;
    
    // Start recording thread
    if (controller->record_thread != NULL)
    {
        controller->thread_running = true;
        rtos_set_semaphore(&controller->thread_sem);  // Signal thread to start
        LOGI("%s: Recording thread started\n", __func__);
    }
    
    controller->module_status.status = VIDEO_RECORDER_STATUS_STARTED;
    
    return AVDK_ERR_OK;
}

static avdk_err_t video_recorder_ctlr_stop(bk_video_recorder_ctlr_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_video_recorder_ctlr_t *controller = __containerof(handler, private_video_recorder_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == VIDEO_RECORDER_STATUS_STARTED, AVDK_ERR_INVAL, TAG, "video record is not started");

    // Stop recording thread
    if (controller->record_thread != NULL)
    {
        controller->thread_running = false;
        LOGI("%s: Stopping recording thread\n", __func__);
        
        // Wait for thread to finish current operation using semaphore (no delay)
        // Thread will signal stop_sem when it exits the recording loop
        bk_err_t sem_ret = rtos_get_semaphore(&controller->stop_sem, BEKEN_WAIT_FOREVER);
        if (sem_ret != BK_OK)
        {
            LOGE("%s: Failed to wait for thread to stop, ret=%d\n", __func__, sem_ret);
        }
        else
        {
            LOGI("%s: Recording thread stopped successfully\n", __func__);
        }
    }

    // Close AVI recording if active
    if (controller->avi_handle != NULL)
    {
        avdk_err_t avi_ret = avi_record_stop(controller);
        if (avi_ret != AVDK_ERR_OK)
        {
            ret = avi_ret;
        }
        controller->avi_handle = NULL;
    }
    
    // Close MP4 recording if active
    if (controller->mp4_handle != NULL)
    {
        avdk_err_t mp4_ret = mp4_record_stop(controller);
        if (mp4_ret != AVDK_ERR_OK)
        {
            ret = mp4_ret;
        }
        controller->mp4_handle = NULL;
    }

    controller->mp4_audio_packetize_enable = false;
    controller->mp4_audio_packetize_frame_bytes = 0;
    controller->mp4_audio_packetize_partial_used = 0;
    if (controller->mp4_audio_packetize_partial_buf != NULL)
    {
        os_free(controller->mp4_audio_packetize_partial_buf);
        controller->mp4_audio_packetize_partial_buf = NULL;
        controller->mp4_audio_packetize_partial_buf_size = 0;
    }

    controller->module_status.status = VIDEO_RECORDER_STATUS_STOPPED;

    return ret;
}

static avdk_err_t video_recorder_ctlr_delete(bk_video_recorder_ctlr_handle_t handler)
{
    private_video_recorder_ctlr_t *controller = __containerof(handler, private_video_recorder_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    /*
     * Allow delete after close/stop to match typical lifecycle:
     * start -> stop -> close -> delete
     *
     * Also allow delete when status is NONE (open not called) so that callers can
     * clean up the handle after a partial initialization failure.
     */
    avdk_err_t ret = AVDK_ERR_OK;

    if (controller->module_status.status == VIDEO_RECORDER_STATUS_STARTED)
    {
        /* Best-effort stop before close to avoid leaving file handles open. */
        avdk_err_t stop_ret = video_recorder_ctlr_stop(handler);
        if (stop_ret != AVDK_ERR_OK)
        {
            LOGW("%s: stop before delete failed, ret=%d\n", __func__, stop_ret);
            ret = stop_ret;
        }
    }

    if (controller->module_status.status != VIDEO_RECORDER_STATUS_NONE &&
        controller->module_status.status != VIDEO_RECORDER_STATUS_CLOSED)
    {
        /* Best-effort close to free thread/semaphores and container resources. */
        avdk_err_t close_ret = video_recorder_ctlr_close(handler);
        if (close_ret != AVDK_ERR_OK)
        {
            LOGW("%s: close before delete failed, ret=%d\n", __func__, close_ret);
            ret = close_ret;
        }
    }

    os_free(controller);
    return ret;
}

static avdk_err_t video_recorder_ctlr_ioctl(bk_video_recorder_ctlr_handle_t handler, bk_video_recorder_ioctl_cmd_t cmd, void *param)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_video_recorder_ctlr_t *controller = __containerof(handler, private_video_recorder_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    switch (cmd)
    {
    case BK_VIDEO_RECORDER_IOCTL_CMD_BASE:
        break;

    default:
        LOGE("%s: Unknown ioctl command: %d\n", __func__, cmd);
        ret = AVDK_ERR_INVAL;
        break;
    }

    return ret;
}

avdk_err_t bk_video_recorder_ctlr_new(bk_video_recorder_ctlr_handle_t *handle, bk_video_recorder_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    private_video_recorder_ctlr_t *controller = os_malloc(sizeof(private_video_recorder_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_video_recorder_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_video_recorder_ctlr_config_t));
    controller->ops.open = video_recorder_ctlr_open;
    controller->ops.close = video_recorder_ctlr_close;
    controller->ops.start = video_recorder_ctlr_start;
    controller->ops.stop = video_recorder_ctlr_stop;
    controller->ops.delete = video_recorder_ctlr_delete;
    controller->ops.ioctl = video_recorder_ctlr_ioctl;

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}
