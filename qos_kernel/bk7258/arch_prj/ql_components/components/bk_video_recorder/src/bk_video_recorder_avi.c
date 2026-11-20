// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "bk_video_recorder_ctlr.h"
#include "bk_video_recorder_avi.h"
#include "modules/avilib.h"
#include "components/bk_video_recorder_types.h"

#define TAG "video_recorder_avi"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// AVI recording: start recording
avdk_err_t avi_record_start(private_video_recorder_ctlr_t *controller, char *file_path)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");

    // AVI recording
    avi_t *avi = NULL;
    AVI_open_output_file(&avi, file_path, AVI_MEM_PSRAM);
    if (avi == NULL)
    {
        LOGE("%s: Failed to open AVI output file: %s\n", __func__, file_path);
        return AVDK_ERR_IO;
    }

    controller->avi_handle = avi;
    // Initialize mutex for thread-safe AVI write operations
    // Note: we keep avilib.c unchanged; concurrency is handled here at the caller layer.
    if (rtos_init_mutex(&controller->avi_write_mutex) != kNoErr)
    {
        LOGE("%s: Failed to initialize AVI write mutex\n", __func__);
        AVI_close(avi);
        controller->avi_handle = NULL;
        return AVDK_ERR_IO;
    }
    LOGI("%s: AVI write mutex initialized successfully\n", __func__);

    // Set video parameters
    char compressor[8] = {0};
    if (controller->config.record_format == VIDEO_RECORDER_FORMAT_H264)
    {
        os_memcpy(compressor, "H264", 4);
    }
    else if (controller->config.record_format == VIDEO_RECORDER_FORMAT_MJPEG)
    {
        os_memcpy(compressor, "MJPG", 4);
    }
    else
    {
        // Unsupported format, report error and stop recording
        LOGE("%s: Unsupported video format: %d, only H264 and MJPEG are supported for AVI\n", 
             __func__, controller->config.record_format);
        rtos_deinit_mutex(&controller->avi_write_mutex);
        AVI_close(avi);
        controller->avi_handle = NULL;
        return AVDK_ERR_INVAL;
    }

    double fps = (controller->config.record_framerate > 0) ?
                 (double)controller->config.record_framerate : 30.0;
    uint32_t width = (controller->config.video_width > 0) ?
                     controller->config.video_width : 640;
    uint32_t height = (controller->config.video_height > 0) ?
                      controller->config.video_height : 480;

    AVI_set_video(avi, width, height, fps, compressor);

    // Set audio parameters if audio is enabled
    if (controller->config.audio_channels > 0)
    {
        uint32_t audio_format = (controller->config.audio_format > 0) ?
                               controller->config.audio_format : WAVE_FORMAT_PCM;
        uint32_t audio_rate = (controller->config.audio_rate > 0) ?
                             controller->config.audio_rate : 8000;
        uint32_t audio_bits = (controller->config.audio_bits > 0) ?
                              controller->config.audio_bits : 16;

        // AVI container audio uses WAVE_FORMAT_* values.
        // For formats not suitable for AVI in this project, we skip the audio track (record video-only).
        bool avi_audio_supported = true;

        if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_PCM || audio_format == WAVE_FORMAT_PCM)
        {
            audio_format = WAVE_FORMAT_PCM;
        }
        else if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_AAC || audio_format == WAVE_FORMAT_AAC)
        {
            audio_format = WAVE_FORMAT_AAC;
            audio_bits = 0;
        }
        else if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_ALAW || audio_format == WAVE_FORMAT_ALAW)
        {
            audio_format = WAVE_FORMAT_ALAW;
            audio_bits = 8;
        }
        else if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_MULAW || audio_format == WAVE_FORMAT_MULAW)
        {
            audio_format = WAVE_FORMAT_MULAW;
            audio_bits = 8;
        }
        else if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_G722 || audio_format == WAVE_FORMAT_G722)
        {
            audio_format = WAVE_FORMAT_G722;
            audio_bits = 0;
        }
        else
        {
            avi_audio_supported = false;
        }

        if (!avi_audio_supported)
        {
            LOGE("%s: Unsupported audio_format=0x%08x for AVI\n", __func__, (unsigned int)audio_format);
            rtos_deinit_mutex(&controller->avi_write_mutex);
            AVI_close(avi);
            controller->avi_handle = NULL;
            return AVDK_ERR_INVAL;
        }
        AVI_set_audio(avi, controller->config.audio_channels, audio_rate, audio_bits, audio_format);
    }

    LOGI("%s: AVI recording started, file: %s, %dx%d@%.2ffps\n",
         __func__, file_path, width, height, fps);

    return AVDK_ERR_OK;
}

// AVI recording: stop recording
avdk_err_t avi_record_stop(private_video_recorder_ctlr_t *controller)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");

    // Close AVI recording if active
    if (controller->avi_handle != NULL)
    {
        avi_t *avi = (avi_t *)controller->avi_handle;
        // Serialize close with possible concurrent write operations
        // Note: lock must not be used when interrupts are disabled.
        if (rtos_lock_mutex(&controller->avi_write_mutex) != kNoErr)
        {
            LOGE("%s: Failed to lock AVI write mutex before close\n", __func__);
            // Best effort: continue close without lock
        }
        
        // Calculate actual frame rate based on recorded frames and time
        if (controller->video_frame_count > 1)
        {
            uint32_t total_time_ms = 0;

            // Prefer saved-frame timestamps for fps estimation (see MP4 path for rationale).
            if (controller->first_saved_frame_time_ms > 0 && controller->last_saved_frame_time_ms > 0)
            {
                if (controller->last_saved_frame_time_ms >= controller->first_saved_frame_time_ms)
                {
                    total_time_ms = controller->last_saved_frame_time_ms - controller->first_saved_frame_time_ms;
                }
                else
                {
                    total_time_ms = (0xFFFFFFFFU - controller->first_saved_frame_time_ms) + controller->last_saved_frame_time_ms + 1;
                }
            }
            else if (controller->recording_start_time_ms > 0)
            {
                uint32_t current_time_ms = rtos_get_time();
                // Handle 32-bit overflow
                if (current_time_ms >= controller->recording_start_time_ms)
                {
                    total_time_ms = current_time_ms - controller->recording_start_time_ms;
                }
                else
                {
                    // Time has wrapped around
                    total_time_ms = (0xFFFFFFFFU - controller->recording_start_time_ms) + current_time_ms + 1;
                }
            }
            
            // Update frame rate based on actual duration (frame count tracked internally).
            if (total_time_ms > 0)
            {
                double actual_fps = (double)controller->video_frame_count * 1000.0 / (double)total_time_ms;
                AVI_update_video_frame_rate_by_duration(avi, total_time_ms);
                LOGI("%s: Updated frame rate: %u frames in %u ms (%.2f fps)\n",
                     __func__, controller->video_frame_count, total_time_ms, actual_fps);
            }
        }

        int avi_ret = AVI_close(avi);
        if (avi_ret != 0)
        {
            LOGE("%s: AVI_close failed, ret=%d\n", __func__, avi_ret);
            ret = AVDK_ERR_IO;
        }
        controller->avi_handle = NULL;

        // Unlock and destroy mutex after closing AVI file
        // Always unlock first (if it was locked successfully)
        (void)rtos_unlock_mutex(&controller->avi_write_mutex);
        rtos_deinit_mutex(&controller->avi_write_mutex);
        LOGI("%s: AVI write mutex destroyed\n", __func__);
    }

    return ret;
}

// AVI recording: close recording (cleanup)
avdk_err_t avi_record_close(private_video_recorder_ctlr_t *controller)
{
    // Same as stop for AVI
    return avi_record_stop(controller);
}

// AVI recording: write video frame
// Frame rate limiting: skip frames if actual frame rate exceeds configured max frame rate
avdk_err_t avi_write_video_frame(private_video_recorder_ctlr_t *controller, uint8_t *data, uint32_t length)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(data, AVDK_ERR_INVAL, TAG, "data is NULL");

    avi_t *avi = (avi_t *)controller->avi_handle;
    if (avi == NULL)
    {
        LOGE("%s: AVI handle is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // For H264, wait until the first IDR frame before starting the recording timeline.
    if (controller->config.record_format == VIDEO_RECORDER_FORMAT_H264 &&
        !controller->video_idr_started)
    {
        if (!controller->last_is_key_frame)
        {
            controller->last_frame_time_ms = rtos_get_time();
            return AVDK_ERR_OK;
        }
        controller->video_idr_started = true;
        controller->recording_start_time_ms = rtos_get_time();
        controller->video_frame_count = 0;
        controller->first_saved_frame_time_ms = 0;
        controller->last_saved_frame_time_ms = 0;
    }

    // Frame rate limiting: check if frame should be saved based on max frame rate
    uint32_t current_time_ms = rtos_get_time();
    bool should_save_frame = false;
    
    // Get configured max frame rate (0 means no limit)
    uint32_t max_fps = controller->config.record_framerate;
    
    if (max_fps == 0)
    {
        // No frame rate limit, save all frames
        should_save_frame = true;
    }
    else if (controller->last_saved_frame_time_ms == 0)
    {
        // First frame, always save
        should_save_frame = true;
    }
    else
    {
        // Calculate minimum frame interval based on max frame rate
        // min_interval_ms = 1000 / max_fps (rounded up)
        uint32_t min_interval_ms = (1000 + max_fps - 1) / max_fps; // Round up
        
        // Calculate time since last saved frame
        uint32_t time_since_last_saved = 0;
        if (current_time_ms >= controller->last_saved_frame_time_ms)
        {
            // Normal case: no overflow
            time_since_last_saved = current_time_ms - controller->last_saved_frame_time_ms;
        }
        else
        {
            // Time has wrapped around (32-bit overflow)
            time_since_last_saved = (0xFFFFFFFFU - controller->last_saved_frame_time_ms) + current_time_ms + 1;
        }
        
        // Save frame only if enough time has passed since last saved frame
        if (time_since_last_saved >= min_interval_ms)
        {
            should_save_frame = true;
        }
        else
        {
            // Check if this is an H264 I-frame - I-frames should never be skipped
            if (controller->config.record_format == VIDEO_RECORDER_FORMAT_H264)
            {
                if (controller->last_is_key_frame)
                {
                    // This is an I-frame, must save it even if interval is too short
                    LOGV("%s: Saving H264 I-frame despite short interval (interval=%u ms, min=%u ms)\n",
                         __func__, time_since_last_saved, min_interval_ms);
                    should_save_frame = true;
                }
                else
                {
                    // Skip this P/B-frame to maintain max frame rate
                    LOGV("%s: Skipping H264 P/B-frame to maintain max frame rate (interval=%u ms, min=%u ms)\n",
                         __func__, time_since_last_saved, min_interval_ms);
                    return AVDK_ERR_OK; // Return OK but don't write frame
                }
            }
            else
            {
                // For non-H264 formats (MJPEG, etc.), skip frame normally
                LOGV("%s: Skipping frame to maintain max frame rate (interval=%u ms, min=%u ms)\n",
                     __func__, time_since_last_saved, min_interval_ms);
                return AVDK_ERR_OK; // Return OK but don't write frame
            }
        }
    }
    
    // Save frame if it passed the rate limiting check
    if (should_save_frame)
    {
        // Lock mutex for thread-safe AVI write operation
        if (rtos_lock_mutex(&controller->avi_write_mutex) != kNoErr)
        {
            LOGE("%s: Failed to lock AVI write mutex\n", __func__);
            return AVDK_ERR_IO;
        }

        avdk_err_t write_ret = AVDK_ERR_OK;
        int ret = AVI_write_frame(avi, (char *)data, length);
        if (ret < 0)
        {
            LOGE("%s: AVI_write_frame failed, ret=%d\n", __func__, ret);
            write_ret = AVDK_ERR_IO;
        }
        else
        {
            // Track frame timing for dynamic frame rate calculation
            // Only count frames that were actually saved
            controller->video_frame_count++;
            if (controller->first_saved_frame_time_ms == 0)
            {
                controller->first_saved_frame_time_ms = current_time_ms;
            }
            controller->last_saved_frame_time_ms = current_time_ms;
        }

        // Unlock mutex
        if (rtos_unlock_mutex(&controller->avi_write_mutex) != kNoErr)
        {
            LOGE("%s: Failed to unlock AVI write mutex\n", __func__);
        }

        if (write_ret != AVDK_ERR_OK)
        {
            return write_ret;
        }
    }
    
    // Always update last_frame_time_ms to track all frames (including skipped ones)
    controller->last_frame_time_ms = current_time_ms;

    return AVDK_ERR_OK;
}

// AVI recording: write audio data
avdk_err_t avi_write_audio_data(private_video_recorder_ctlr_t *controller, uint8_t *data, uint32_t length)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(data, AVDK_ERR_INVAL, TAG, "data is NULL");

    avi_t *avi = (avi_t *)controller->avi_handle;
    if (avi == NULL)
    {
        LOGE("%s: AVI handle is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // If AVI audio track is disabled/unsupported, ignore audio data (record video-only).
    if (avi->a_chans == 0)
    {
        return AVDK_ERR_OK;
    }

    // Lock mutex for thread-safe AVI write operation
    if (rtos_lock_mutex(&controller->avi_write_mutex) != kNoErr)
    {
        LOGE("%s: Failed to lock AVI write mutex\n", __func__);
        return AVDK_ERR_IO;
    }

    avdk_err_t write_ret = AVDK_ERR_OK;

    int ret = AVI_write_audio(avi, (char *)data, length);
    if (ret < 0)
    {
        LOGE("%s: AVI_write_audio failed, ret=%d\n", __func__, ret);
        write_ret = AVDK_ERR_IO;
        goto out;
    }

out:
    // Unlock mutex
    if (rtos_unlock_mutex(&controller->avi_write_mutex) != kNoErr)
    {
        LOGE("%s: Failed to unlock AVI write mutex\n", __func__);
    }

    return write_ret;
}
