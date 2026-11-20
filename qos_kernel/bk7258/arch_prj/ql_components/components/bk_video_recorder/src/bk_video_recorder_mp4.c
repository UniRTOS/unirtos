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
#include "bk_video_recorder_mp4.h"
#include "modules/mp4lib.h"

#define TAG "video_recorder_mp4"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// MP4 recording: start recording
avdk_err_t mp4_record_start(private_video_recorder_ctlr_t *controller, char *file_path)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");

    mp4_t *mp4 = NULL;

    // Allocate MP4 index cache buffer to batch index records and reduce filesystem write次数.
    // The buffer size can be tuned based on expected recording length and available memory.
    // Larger buffer => fewer flushes to index file.
    if (controller->mp4_index_cache_buf == NULL)
    {
        const uint32_t idx_cache_size = 8 * 1024;
        controller->mp4_index_cache_buf = psram_malloc(idx_cache_size);
        if (controller->mp4_index_cache_buf == NULL)
        {
            LOGE("%s: Failed to allocate MP4 index cache buffer, size=%u\n", __func__, idx_cache_size);
            return AVDK_ERR_NOMEM;
        }
        controller->mp4_index_cache_size = idx_cache_size;
    }
    
    // Open MP4 file for writing using mp4lib (write-mode index is buffered and flushed in large chunks).
    MP4_open_output_file_with_index_cache(&mp4, file_path,
                                          controller->mp4_index_cache_buf,
                                          controller->mp4_index_cache_size,
                                          MP4_MEM_PSRAM);
    if (mp4 == NULL)
    {
        LOGE("%s: Failed to open MP4 output file: %s\n", __func__, file_path);
        psram_free(controller->mp4_index_cache_buf);
        controller->mp4_index_cache_buf = NULL;
        controller->mp4_index_cache_size = 0;
        return AVDK_ERR_IO;
    }

    controller->mp4_handle = (void *)mp4;
    controller->mp4_video_frame_count = 0;
    controller->mp4_audio_sample_count = 0;
    controller->mp4_mdat_size = 0;
    
    // Initialize mutex for thread-safe MP4 write operations
    if (rtos_init_mutex(&controller->mp4_write_mutex) != kNoErr) {
        LOGE("%s: Failed to initialize MP4 write mutex\n", __func__);
        MP4_close(mp4);
        controller->mp4_handle = NULL;
        psram_free(controller->mp4_index_cache_buf);
        controller->mp4_index_cache_buf = NULL;
        controller->mp4_index_cache_size = 0;
        return AVDK_ERR_IO;
    }
    LOGI("%s: MP4 write mutex initialized successfully\n", __func__);
    
    // Record start time for frame rate calculation
    controller->recording_start_time_ms = rtos_get_time();

    // Determine video codec type
    uint32_t video_codec = MP4_CODEC_H264;
    if (controller->config.record_format == VIDEO_RECORDER_FORMAT_MJPEG)
    {
        video_codec = MP4_CODEC_MJPEG;
    }
    else if (controller->config.record_format == VIDEO_RECORDER_FORMAT_H264)
    {
        video_codec = MP4_CODEC_H264;
    }

    // Set video parameters
    uint32_t width = (controller->config.video_width > 0) ? controller->config.video_width : 640;
    uint32_t height = (controller->config.video_height > 0) ? controller->config.video_height : 480;
    double fps = (controller->config.record_framerate > 0) ? (double)controller->config.record_framerate : 30.0;
    MP4_set_video(mp4, width, height, fps, video_codec);

    // Set audio parameters if audio is enabled
    if (controller->config.audio_channels > 0)
    {
        // Select MP4 audio codec based on config.audio_format (WAVE tags as hint).
        // Supported:
        // - PCM:   'sowt' (little-endian signed PCM)
        // - AAC:   'mp4a' (requires AudioSpecificConfig in 'esds')
        // - G.711: 'alaw' / 'ulaw'
        // - G722:  'g722'
        uint32_t audio_codec = MP4_CODEC_PCM_SOWT;
        uint16_t sample_bits = (uint16_t)controller->config.audio_bits;
        uint32_t audio_format = controller->config.audio_format;

        /*
         * Align MP4 audio behavior with AVI path:
         * - AAC/G722 are packetized/compressed: sample_size is not meaningful -> 0
         * - G.711 A-law/mu-law are PCM-like 8-bit -> 8
         * - PCM: keep configured bits, default to 16 if not specified
         */
        if (audio_format == 0 || audio_format == VIDEO_RECORDER_AUDIO_FORMAT_PCM)
        {
            audio_codec = MP4_CODEC_PCM_SOWT;
        }
        else if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_AAC)
        {
            audio_codec = MP4_CODEC_AAC;
            sample_bits = 0;
        }
        else if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_G722)
        {
            audio_codec = MP4_CODEC_G722;
            sample_bits = 0;
        }
        else if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_ALAW)
        {
            audio_codec = MP4_CODEC_ALAW;
            sample_bits = 8;
        }
        else if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_MULAW)
        {
            audio_codec = MP4_CODEC_ULAW;
            sample_bits = 8;
        }
        else
        {
            // Only PCM/AAC/G711A/G711U/G722 are supported for MP4.
            LOGE("%s: Unsupported MP4 audio_format=0x%04x\n", __func__, (unsigned)audio_format);
            return AVDK_ERR_INVAL;
        }

        if (MP4_set_audio(mp4,
                          (uint16_t)controller->config.audio_channels,
                          controller->config.audio_rate,
                          sample_bits,
                          audio_codec) != 0)
        {
            LOGE("%s: MP4_set_audio failed, rate=%u, channels=%u, codec=0x%08x\n",
                 __func__, controller->config.audio_rate, controller->config.audio_channels, (unsigned)audio_codec);
            return AVDK_ERR_INVAL;
        }
    }

    LOGI("%s: MP4 recording started, file: %s\n", __func__, file_path);

    return AVDK_ERR_OK;
}

// MP4 recording: stop recording
avdk_err_t mp4_record_stop(private_video_recorder_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");

    // Close MP4 recording if active
    if (controller->mp4_handle != NULL)
    {
        mp4_t *mp4 = (mp4_t *)controller->mp4_handle;

        // Calculate actual frame rate based on actual frame count and duration
        if (controller->mp4_video_frame_count > 0)
        {
            uint32_t total_duration_ms = 0;

            /*
             * Prefer video-frame based duration (first_saved -> last_saved) to estimate FPS.
             * This avoids distortion from:
             * - start/stop overhead (pipeline init/deinit)
             * - long stop latency
             * - gaps when frames are skipped by rate limiter
             */
            if (controller->mp4_video_frame_count > 1 &&
                controller->first_saved_frame_time_ms > 0 &&
                controller->last_saved_frame_time_ms > 0)
            {
                if (controller->last_saved_frame_time_ms >= controller->first_saved_frame_time_ms)
                {
                    total_duration_ms = controller->last_saved_frame_time_ms - controller->first_saved_frame_time_ms;
                }
                else
                {
                    total_duration_ms = (0xFFFFFFFFU - controller->first_saved_frame_time_ms) + controller->last_saved_frame_time_ms + 1;
                }
            }
            else if (controller->recording_start_time_ms > 0)
            {
                uint32_t current_time_ms = rtos_get_time();
                if (current_time_ms >= controller->recording_start_time_ms)
                {
                    // Normal case: no overflow
                    total_duration_ms = current_time_ms - controller->recording_start_time_ms;
                }
                else
                {
                    // Time has wrapped around (32-bit overflow)
                    total_duration_ms = (0xFFFFFFFFU - controller->recording_start_time_ms) + current_time_ms + 1;
                }
            }
            
            if (total_duration_ms > 0)
            {
                // Update frame rate based on actual duration (frame count tracked internally).
                MP4_update_video_frame_rate_by_duration(mp4, total_duration_ms);
                LOGI("%s: Updated frame rate: %u frames in %u ms (%.2f fps)\n",
                     __func__, controller->mp4_video_frame_count, total_duration_ms,
                     (double)controller->mp4_video_frame_count * 1000.0 / (double)total_duration_ms);
            }
        }

        // Close MP4 file (this will write moov box and update mdat size)
        if (MP4_close(mp4) != 0)
        {
            LOGE("%s: Failed to close MP4 file\n", __func__);
            controller->mp4_handle = NULL;
            // Destroy mutex even if close failed
            rtos_deinit_mutex(&controller->mp4_write_mutex);
            if (controller->mp4_index_cache_buf != NULL)
            {
                psram_free(controller->mp4_index_cache_buf);
                controller->mp4_index_cache_buf = NULL;
                controller->mp4_index_cache_size = 0;
            }
            return AVDK_ERR_IO;
        }

        controller->mp4_handle = NULL;
        
        // Destroy mutex after closing MP4 file
        rtos_deinit_mutex(&controller->mp4_write_mutex);
        LOGI("%s: MP4 write mutex destroyed\n", __func__);

        // Free MP4 index cache buffer after closing MP4 file.
        if (controller->mp4_index_cache_buf != NULL)
        {
            psram_free(controller->mp4_index_cache_buf);
            controller->mp4_index_cache_buf = NULL;
            controller->mp4_index_cache_size = 0;
        }
    }

    return AVDK_ERR_OK;
}

// MP4 recording: close recording (cleanup)
avdk_err_t mp4_record_close(private_video_recorder_ctlr_t *controller)
{
    // Same as stop for MP4
    return mp4_record_stop(controller);
}

// MP4 recording: write video frame
// Frame rate limiting: skip frames if actual frame rate exceeds configured max frame rate
avdk_err_t mp4_write_video_frame(private_video_recorder_ctlr_t *controller, uint8_t *data, uint32_t length)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(data, AVDK_ERR_INVAL, TAG, "data is NULL");

    mp4_t *mp4 = (mp4_t *)controller->mp4_handle;
    if (mp4 == NULL)
    {
        LOGE("%s: MP4 file handle is invalid\n", __func__);
        return AVDK_ERR_INVAL;
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
        // Use more precise calculation: min_interval_ms = 1000.0 / max_fps (rounded down to allow max fps)
        // Round down instead of up to allow frames at the configured rate
        uint32_t min_interval_ms = 1000 / max_fps; // Round down to allow max fps
        // Allow 1ms tolerance to account for timing jitter and processing delays
        // This helps maintain frame rate when processing is slightly delayed
        uint32_t tolerance_ms = 1;
        uint32_t effective_min_interval = (min_interval_ms > tolerance_ms) ? (min_interval_ms - tolerance_ms) : 0;
        
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
        
        // Save frame if enough time has passed (with tolerance to account for processing delays)
        if (time_since_last_saved >= effective_min_interval)
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
        mp4_t *mp4 = (mp4_t *)controller->mp4_handle;
        if (mp4 == NULL)
        {
            LOGE("%s: MP4 file handle is invalid\n", __func__);
            return AVDK_ERR_INVAL;
        }

        // Lock mutex for thread-safe write operation
        if (rtos_lock_mutex(&controller->mp4_write_mutex) != kNoErr) {
            LOGE("%s: Failed to lock MP4 write mutex\n", __func__);
            return AVDK_ERR_IO;
        }

        // Write video frame data using mp4lib
        avdk_err_t write_ret = AVDK_ERR_OK;
        if (MP4_write_frame(mp4, data, length) != 0)
        {
            LOGE("%s: Failed to write video frame data\n", __func__);
            write_ret = AVDK_ERR_IO;
        } else {
            // Only count frames that were actually saved
            controller->mp4_video_frame_count++;
            controller->mp4_mdat_size += length;
            if (controller->first_saved_frame_time_ms == 0)
            {
                controller->first_saved_frame_time_ms = current_time_ms;
            }
            controller->last_saved_frame_time_ms = current_time_ms;
        }

        // Unlock mutex
        rtos_unlock_mutex(&controller->mp4_write_mutex);

        if (write_ret != AVDK_ERR_OK) {
            return write_ret;
        }
    }
    
    // Always update last_frame_time_ms to track all frames (including skipped ones)
    controller->last_frame_time_ms = current_time_ms;

    return AVDK_ERR_OK;
}

// MP4 recording: write audio data
avdk_err_t mp4_write_audio_data(private_video_recorder_ctlr_t *controller, uint8_t *data, uint32_t length)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(data, AVDK_ERR_INVAL, TAG, "data is NULL");

    mp4_t *mp4 = (mp4_t *)controller->mp4_handle;
    if (mp4 == NULL)
    {
        LOGE("%s: MP4 file handle is invalid\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // Lock mutex for thread-safe write operation
    if (rtos_lock_mutex(&controller->mp4_write_mutex) != kNoErr) {
        LOGE("%s: Failed to lock MP4 write mutex\n", __func__);
        return AVDK_ERR_IO;
    }

    // Write audio data using mp4lib
    avdk_err_t write_ret = AVDK_ERR_OK;
    if (MP4_write_audio(mp4, data, length) != 0)
    {
        LOGE("%s: Failed to write audio data\n", __func__);
        write_ret = AVDK_ERR_IO;
    } else {
        controller->mp4_audio_sample_count++;
        controller->mp4_mdat_size += length;
    }

    // Unlock mutex
    rtos_unlock_mutex(&controller->mp4_write_mutex);

    return write_ret;
}
