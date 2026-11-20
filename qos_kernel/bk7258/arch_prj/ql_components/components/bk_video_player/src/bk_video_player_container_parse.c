#include "os/os.h"
#include "os/mem.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "bk_video_player_ctlr.h"
#include "bk_video_player_container_parse.h"
#include "bk_video_player_pipeline.h"
#include <stdint.h>

#define TAG "container_parse"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Stream type definitions (should match with container parser return values)
#define STREAM_TYPE_AUDIO  0x01
#define STREAM_TYPE_VIDEO  0x02

#include "bk_video_player_buffer_pool.h"

// Fallback defaults for build environments that don't yet have the new Kconfig symbols in sdkconfig.h.
// Defaults match the previous hard-coded behavior.
#ifndef CONFIG_BK_VIDEO_PLAYER_VIDEO_PARSE_THREAD_PRIORITY
#define CONFIG_BK_VIDEO_PLAYER_VIDEO_PARSE_THREAD_PRIORITY (BEKEN_DEFAULT_WORKER_PRIORITY)
#endif
#ifndef CONFIG_BK_VIDEO_PLAYER_VIDEO_PARSE_THREAD_STACK_SIZE
#define CONFIG_BK_VIDEO_PLAYER_VIDEO_PARSE_THREAD_STACK_SIZE (8 * 1024)
#endif
#ifndef CONFIG_BK_VIDEO_PLAYER_AUDIO_PARSE_THREAD_PRIORITY
#define CONFIG_BK_VIDEO_PLAYER_AUDIO_PARSE_THREAD_PRIORITY (BEKEN_DEFAULT_WORKER_PRIORITY)
#endif
#ifndef CONFIG_BK_VIDEO_PLAYER_AUDIO_PARSE_THREAD_STACK_SIZE
#define CONFIG_BK_VIDEO_PLAYER_AUDIO_PARSE_THREAD_STACK_SIZE (8 * 1024)
#endif

#include "bk_video_player_thread_config.h"

static uint64_t video_player_get_current_time_ms(private_video_player_ctlr_t *controller)
{
    if (controller == NULL || controller->time_mutex == NULL)
    {
        return 0;
    }

    rtos_lock_mutex(&controller->time_mutex);
    uint64_t t = controller->current_time_ms;
    rtos_unlock_mutex(&controller->time_mutex);
    return t;
}

static uint64_t video_player_get_effective_audio_time_ms_for_video(private_video_player_ctlr_t *controller)
{
    if (controller == NULL || controller->time_mutex == NULL)
    {
        return 0;
    }

    uint64_t base_ms = 0;
    int32_t offset_ms = 0;
    rtos_lock_mutex(&controller->time_mutex);
    base_ms = controller->current_time_ms;
    offset_ms = controller->av_sync_offset_ms;
    rtos_unlock_mutex(&controller->time_mutex);

    return bk_video_player_apply_av_sync_offset_ms(base_ms, offset_ms);
}

static void bk_video_player_container_video_parse_thread(void *arg)
{
    private_video_player_ctlr_t *controller = (private_video_player_ctlr_t *)arg;
    avdk_err_t ret = AVDK_ERR_OK;
    uint32_t last_seen_session_id = 0;
    uint64_t last_video_pts_ms = 0;
    bool session_seek_pending = false;
    uint64_t session_seek_pts_ms = 0;

    LOGI("%s: Container video parse thread started\n", __func__);

    while (!controller->video_parse_thread_exit)
    {
        rtos_get_semaphore(&controller->video_parse_sem, BEKEN_WAIT_FOREVER);

        if (controller->video_parse_thread_exit)
        {
            break;
        }

        if (!controller->video_parse_thread_running)
        {
            continue;
        }

        LOGI("%s: Container video parse thread activated\n", __func__);

        while (controller->video_parse_thread_running && !controller->video_parse_thread_exit)
        {
            // Capture session id at loop entry to prevent cross-session packet pollution.
            // play_session_id may change asynchronously during seek/stop/start.
            uint32_t iter_session_id = controller->play_session_id;

            if (controller->play_session_id != last_seen_session_id)
            {
                last_seen_session_id = controller->play_session_id;
                last_video_pts_ms = 0;
                /*
                 * Scheme B:
                 * On every new playback session start, seek ONCE to the controller's start time (current_time_ms),
                 * then switch to sequential reading for the rest of the session.
                 *
                 * This avoids per-packet "seek-to-audio-time" oscillation (AVI is sensitive due to rounding/PTS gaps),
                 * while still allowing explicit seek/ff/fb (which restarts playback session with updated current_time_ms).
                 */
                session_seek_pts_ms = video_player_get_current_time_ms(controller);
                session_seek_pending = true;

                // Enable seek preroll output dropping for video.
                // The parser may output a keyframe first (pts < seek target) and then jump to target frame.
                if (controller->time_mutex != NULL)
                {
                    rtos_lock_mutex(&controller->time_mutex);
                    controller->video_seek_drop_enable = true;
                    controller->video_seek_drop_until_pts_ms = session_seek_pts_ms;
                    rtos_unlock_mutex(&controller->time_mutex);
                }
            }

            // Check if playback is paused
            if (controller->is_paused)
            {
                rtos_delay_milliseconds(10);
                continue;
            }

            // If video track is disabled, stop parsing video and mark EOF so that audio-only playback can finish.
            if (!controller->video_track_enabled)
            {
                controller->video_parse_thread_running = false;
                controller->video_eof_reached = true;
                break;
            }

            if (controller->active_container_parser == NULL || controller->current_media_info.video.width == 0)
            {
                rtos_delay_milliseconds(10);
                continue;
            }

            /*
             * Pace sequential video parsing under AUDIO clock to avoid reading too far ahead of audio time.
             * Video decode thread will still do A/V sync (drop/wait), but throttling here reduces wasted I/O.
             */
            if (!session_seek_pending && controller->clock_source == VIDEO_PLAYER_CLOCK_AUDIO &&
                last_video_pts_ms > 0)
            {
                uint64_t audio_time_ms = video_player_get_effective_audio_time_ms_for_video(controller);
                const uint64_t max_video_lead_ms = 200;
                if (audio_time_ms > 0 && last_video_pts_ms > (audio_time_ms + max_video_lead_ms))
                {
                    rtos_delay_milliseconds(5);
                    continue;
                }
            }

            // Wait a short time for an empty node to avoid busy-loop when consumer is slow.
            video_player_buffer_node_t *buffer_node = buffer_pool_get_empty(&controller->video_pipeline.parser_to_decode_pool);
            if (buffer_node == NULL)
            {
                if (controller->video_parse_thread_exit)
                {
                    break;
                }
                // LOGI("%s: No empty buffer node, waiting for next signal\n", __func__);
                rtos_delay_milliseconds(1);
                continue;
            }

            uint32_t estimated_buffer_size = 0;
            
            rtos_lock_mutex(&controller->active_mutex);
            video_player_container_parser_ops_t *parser = controller->active_container_parser;
            if (parser == NULL || controller->current_media_info.video.width == 0)
            {
                rtos_unlock_mutex(&controller->active_mutex);
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                rtos_delay_milliseconds(1);
                continue;
            }

            if (parser->get_video_packet_size == NULL)
            {
                LOGE("%s: Container parser does not support get_video_packet_size, cannot proceed\n", __func__);
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                controller->video_parse_thread_running = false;
                controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;
                rtos_unlock_mutex(&controller->active_mutex);
                break;
            }
            
            /*
             * Scheme B target_pts selection:
             * - Seek ONCE at session start (target_pts=session_seek_pts_ms)
             * - Then always read sequentially (target_pts=VIDEO_PLAYER_PTS_INVALID)
             *
             * Container parsers (AVI/MP4) treat VIDEO_PLAYER_PTS_INVALID as "read next frame".
             * NOTE: 0 is a valid PTS (seek to beginning), so MUST NOT use 0 as the sequential sentinel.
             */
            uint64_t target_pts = VIDEO_PLAYER_PTS_INVALID;
            if (session_seek_pending)
            {
                target_pts = session_seek_pts_ms;
                session_seek_pending = false;
            }
            
            ret = parser->get_video_packet_size(parser,
                                                &estimated_buffer_size,
                                                target_pts);
            rtos_unlock_mutex(&controller->active_mutex);
            LOGV("%s: get_video_packet_size returned size=%u, ret=%d\r\n", __func__, estimated_buffer_size, ret);
            if (ret != AVDK_ERR_OK || estimated_buffer_size == 0)
            {
                if (ret == AVDK_ERR_EOF)
                {
                    if (!controller->video_eof_reached)
                    {
                        LOGI("%s %d: Video EOF detected\n", __func__, __LINE__);
                        controller->video_eof_reached = true;
                    }
                    // Check if both video and audio have reached EOF before executing action
                    if (controller->video_eof_reached && controller->audio_eof_reached)
                    {
                        avdk_err_t eof_ret = bk_video_player_handle_play_mode(controller);
                        // IMPORTANT: Always return the acquired empty node before leaving/looping.
                        // Otherwise, empty_sem will permanently lose a token across play sessions and
                        // the next playback can get stuck waiting for an empty buffer.
                        buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                        buffer_node = NULL;
                        if (eof_ret != AVDK_ERR_OK)
                        {
                            break;
                        }
                        if (controller->video_parse_thread_running)
                        {
                            rtos_delay_milliseconds(10);
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                        rtos_delay_milliseconds(10);
                        continue;
                    }
                }
                LOGE("%s: Failed to get video packet size from container parser, ret=%d, size=%u\n",
                     __func__, ret, estimated_buffer_size);
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                rtos_delay_milliseconds(1);
                continue;
            }
            
            if (controller->config.video.packet_buffer_alloc_cb != NULL)
            {
                buffer_node->buffer.length = estimated_buffer_size;
                LOGV("%s: Requesting buffer allocation, size=%u\r\n", __func__, estimated_buffer_size);
                ret = controller->config.video.packet_buffer_alloc_cb(controller->config.user_data, &buffer_node->buffer);
                if (ret != AVDK_ERR_OK)
                {
                    LOGE("%s: Failed to allocate video packet buffer, ret=%d\n", __func__, ret);
                    buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                    rtos_delay_milliseconds(1);
                    continue;
                }
                
                if (buffer_node->buffer.data == NULL)
                {
                    LOGE("%s: Buffer allocation callback returned OK but buffer->data is NULL\n", __func__);
                    buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                    rtos_delay_milliseconds(1);
                    continue;
                }
                
                LOGV("%s: Buffer allocated, requested=%u, actual=%u\r\n", __func__, estimated_buffer_size, buffer_node->buffer.length);
                if (buffer_node->buffer.length < estimated_buffer_size)
                {
                    LOGW("%s: Allocated buffer is smaller than requested! requested=%u, actual=%u\r\n", 
                         __func__, estimated_buffer_size, buffer_node->buffer.length);
                }
            }
            else
            {
                LOGE("%s: Video packet buffer alloc callback is NULL, cannot proceed\n", __func__);
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                controller->video_parse_thread_running = false;
                controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;
                break;
            }

            buffer_node->buffer.pts = 0;
            
            rtos_lock_mutex(&controller->active_mutex);
            parser = controller->active_container_parser;
            if (parser == NULL)
            {
                rtos_unlock_mutex(&controller->active_mutex);
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                rtos_delay_milliseconds(1);
                continue;
            }

            if (parser->read_video_packet == NULL)
            {
                LOGE("%s: Container parser does not support read_video_packet, cannot proceed\n", __func__);
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                controller->video_parse_thread_running = false;
                controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;
                rtos_unlock_mutex(&controller->active_mutex);
                break;
            }

            LOGV("%s: Reading video packet, target_pts=%llu\n", __func__, (unsigned long long)target_pts);
            ret = parser->read_video_packet(parser,
                                            &buffer_node->buffer,
                                            target_pts);
            rtos_unlock_mutex(&controller->active_mutex);
            // Tag packet with the session id captured at loop entry.
            // If session changes during read_video_packet(), this packet belongs to the OLD session and must be dropped.
            buffer_node->buffer.user_data = (void *)(uintptr_t)iter_session_id;
            
            // Note: remove per-frame buffer hex dump logs (too noisy for normal playback).
            
            if (ret != AVDK_ERR_OK)
            {
                if (controller->config.video.packet_buffer_free_cb != NULL && buffer_node->buffer.data != NULL)
                {
                    controller->config.video.packet_buffer_free_cb(controller->config.user_data, &buffer_node->buffer);
                }
                buffer_node->buffer.data = NULL;
                buffer_node->buffer.length = 0;
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                
                if (ret == AVDK_ERR_EOF)
                {
                    if (!controller->video_eof_reached)
                    {
                        LOGI("%s %d: Video EOF detected\n", __func__, __LINE__);
                        controller->video_eof_reached = true;
                    }
                    // Check if both video and audio have reached EOF before executing action
                    if (controller->video_eof_reached && controller->audio_eof_reached)
                    {
                        avdk_err_t eof_ret = bk_video_player_handle_play_mode(controller);
                        if (eof_ret != AVDK_ERR_OK)
                        {
                            break;
                        }
                        if (controller->video_parse_thread_running)
                        {
                            rtos_delay_milliseconds(10);
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        LOGI("%s %d: Waiting for audio EOF (video_eof=%d, audio_eof=%d)\n",
                             __func__, __LINE__, controller->video_eof_reached, controller->audio_eof_reached);
                        rtos_delay_milliseconds(10);
                        continue;
                    }
                }
                rtos_delay_milliseconds(1);
                continue;
            }

            // If session changed during this loop iteration, drop the packet to avoid mixing sessions.
            if (iter_session_id != controller->play_session_id)
            {
                if (controller->config.video.packet_buffer_free_cb != NULL && buffer_node->buffer.data != NULL)
                {
                    controller->config.video.packet_buffer_free_cb(controller->config.user_data, &buffer_node->buffer);
                }
                buffer_node->buffer.data = NULL;
                buffer_node->buffer.length = 0;
                buffer_node->buffer.pts = 0;
                buffer_node->buffer.user_data = NULL;
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                continue;
            }

            // Verify buffer data is still valid after decode
            if (buffer_node->buffer.data == NULL)
            {
                LOGE("%s: Container parser returned OK but buffer->data is NULL after read\n", __func__);
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                rtos_delay_milliseconds(1);
                continue;
            }

            if (buffer_node->buffer.pts == VIDEO_PLAYER_PTS_INVALID)
            {
                /*
                 * Root fix:
                 * Some container streams may not provide valid PTS for every video packet.
                 * Generate a monotonic PTS using fps and last_video_pts_ms to keep A/V sync stable.
                 *
                 * NOTE: 0 is a valid PTS. Container parsers should use VIDEO_PLAYER_PTS_INVALID to indicate "missing PTS".
                 */
                uint32_t fps = (controller->current_media_info.video.fps > 0) ? controller->current_media_info.video.fps : 30;
                uint64_t frame_ms = 1000ULL / (uint64_t)fps;
                if (frame_ms == 0)
                {
                    frame_ms = 33;
                }

                uint64_t pts_ms = 0;
                if (last_video_pts_ms > 0)
                {
                    pts_ms = last_video_pts_ms + frame_ms;
                }
                else
                {
                    pts_ms = video_player_get_current_time_ms(controller);
                    if (pts_ms == 0)
                    {
                        pts_ms = frame_ms;
                    }
                }

                buffer_node->buffer.pts = pts_ms;
                last_video_pts_ms = pts_ms;
                LOGW("%s: Video packet PTS missing, patched to %llu ms (fps=%u)\n",
                     __func__, (unsigned long long)pts_ms, fps);
            }
            else
            {
                last_video_pts_ms = buffer_node->buffer.pts;
            }

            // Do not update any global sync state here.
            // Sync and time advancement are handled by the decode threads via controller->current_time_ms.

            if (buffer_node->buffer.data == NULL || buffer_node->buffer.length == 0)
            {
                LOGE("%s: Buffer data is NULL or length is 0 before putting to pool, data=%p, length=%u\n",
                     __func__, buffer_node->buffer.data, buffer_node->buffer.length);
                if (controller->config.video.packet_buffer_free_cb != NULL && buffer_node->buffer.data != NULL)
                {
                    controller->config.video.packet_buffer_free_cb(controller->config.user_data, &buffer_node->buffer);
                }
                buffer_node->buffer.data = NULL;
                buffer_node->buffer.length = 0;
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
                continue;
            }

            buffer_pool_put_filled(&controller->video_pipeline.parser_to_decode_pool, buffer_node);
        }

        LOGI("%s: Container video parse thread stopped, waiting for next play signal\n", __func__);
    }

    LOGI("%s: Container video parse thread exiting\n", __func__);
    rtos_delete_thread(NULL);
}

void bk_video_player_container_audio_parse_thread(void *arg)
{
    private_video_player_ctlr_t *controller = (private_video_player_ctlr_t *)arg;
    avdk_err_t ret = AVDK_ERR_OK;
    uint32_t last_seen_session_id = 0;
    uint64_t last_audio_pts_ms = 0;
    bool session_seek_pending = false;
    uint64_t session_seek_pts_ms = 0;

    LOGI("%s: Container audio parse thread started\n", __func__);

    while (!controller->audio_parse_thread_exit)
    {
        rtos_get_semaphore(&controller->audio_parse_sem, BEKEN_WAIT_FOREVER);

        if (controller->audio_parse_thread_exit)
        {
            break;
        }

        if (!controller->audio_parse_thread_running)
        {
            LOGV("%s: Thread running flag is false, waiting for next signal\n", __func__);
            continue;
        }

        LOGI("%s: Container audio parse thread activated\n", __func__);

        uint32_t consecutive_failures = 0;
        const uint32_t max_consecutive_failures = 10;
        
        while (controller->audio_parse_thread_running && !controller->audio_parse_thread_exit)
        {
            // Capture session id at loop entry to prevent cross-session packet pollution.
            uint32_t iter_session_id = controller->play_session_id;

            if (controller->play_session_id != last_seen_session_id)
            {
                last_seen_session_id = controller->play_session_id;
                last_audio_pts_ms = 0;
                /*
                 * Keep audio behavior consistent with video parse thread (Scheme B):
                 * - Seek ONCE at session start (target_pts=session_seek_pts_ms)
                 * - Then always read sequentially (target_pts=VIDEO_PLAYER_PTS_INVALID)
                 *
                 * This ensures target_pts is effective for audio (real seek),
                 * while avoiding per-packet seek oscillation.
                 */
                session_seek_pts_ms = video_player_get_current_time_ms(controller);
                session_seek_pending = true;
            }

            // Check if playback is paused
            if (controller->is_paused)
            {
                rtos_delay_milliseconds(10);
                continue;
            }

            // If audio track is disabled, stop parsing audio and mark EOF so that video-only playback can finish.
            if (!controller->audio_track_enabled)
            {
                controller->audio_parse_thread_running = false;
                controller->audio_eof_reached = true;
                break;
            }

            if (controller->active_container_parser == NULL)
            {
                LOGV("%s: Waiting for parser ready (active_container_parser=%p)\n",
                     __func__, controller->active_container_parser);
                rtos_delay_milliseconds(10);
                continue;
            }
            
            // If decoder is ready but channels is 0, file has no audio stream, exit thread
            if (controller->current_media_info.audio.channels == 0)
            {
                LOGI("%s: File has no audio stream (channels=0), exiting audio parse thread\n", __func__);
                controller->audio_parse_thread_running = false;
                controller->audio_eof_reached = true; // Mark audio as EOF
                break;
            }

            video_player_buffer_node_t *buffer_node = buffer_pool_get_empty(&controller->audio_pipeline.parser_to_decode_pool);
            if (buffer_node == NULL)
            {
                if (controller->audio_parse_thread_exit)
                {
                    break;
                }
                continue;
            }

            uint32_t estimated_buffer_size = 0;
            
            rtos_lock_mutex(&controller->active_mutex);
            video_player_container_parser_ops_t *aparser = controller->active_container_parser;
            if (aparser == NULL)
            {
                rtos_unlock_mutex(&controller->active_mutex);
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                rtos_delay_milliseconds(1);
                continue;
            }

            if (aparser->get_audio_packet_size == NULL)
            {
                LOGE("%s: Container parser does not support get_audio_packet_size, cannot proceed\n", __func__);
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                controller->audio_parse_thread_running = false;
                controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;
                rtos_unlock_mutex(&controller->active_mutex);
                break;
            }

            uint64_t target_pts = VIDEO_PLAYER_PTS_INVALID;
            if (session_seek_pending)
            {
                target_pts = session_seek_pts_ms;
                session_seek_pending = false;
            }
            
            ret = aparser->get_audio_packet_size(aparser,
                                                 &estimated_buffer_size,
                                                 target_pts);
            rtos_unlock_mutex(&controller->active_mutex);
            if (ret != AVDK_ERR_OK || estimated_buffer_size == 0)
            {
                consecutive_failures++;
                
                if (ret == AVDK_ERR_EOF || consecutive_failures >= max_consecutive_failures)
                {
                    if (ret == AVDK_ERR_EOF)
                    {
                        if (!controller->audio_eof_reached)
                        {
                            LOGI("%s: Audio EOF detected\n", __func__);
                            controller->audio_eof_reached = true;
                        }

                        if (controller->clock_source == VIDEO_PLAYER_CLOCK_AUDIO)
                        {
                            if (controller->video_track_enabled)
                            {
                                controller->clock_source = VIDEO_PLAYER_CLOCK_VIDEO;
                                LOGI("%s: Audio EOF reached, switch clock source to VIDEO\n", __func__);
                            }
                        }

                        controller->audio_parse_thread_running = false;

                        if (controller->video_eof_reached && controller->audio_eof_reached)
                        {
                            avdk_err_t eof_ret = bk_video_player_handle_play_mode(controller);
                            // IMPORTANT: Always return the acquired empty node before leaving/looping.
                            // Otherwise, empty_sem will permanently lose a token across play sessions and
                            // the next playback can get stuck waiting for an empty buffer.
                            buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                            buffer_node = NULL;
                            if (eof_ret != AVDK_ERR_OK)
                            {
                                break;
                            }
                            if (controller->audio_parse_thread_running)
                            {
                                rtos_delay_milliseconds(10);
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                            break;
                        }
                    }
                    else
                    {
                        LOGI("%s: Too many consecutive failures (failures=%u), stopping playback\n",
                             __func__, consecutive_failures);
                        controller->audio_parse_thread_running = false;
                        controller->audio_decode_thread_running = false; // Also stop audio decode thread
                        controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;
                        buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                        break;
                    }
                }
                LOGE("%s: Failed to get audio packet size from container parser, ret=%d, size=%u (failures=%u)\n",
                     __func__, ret, estimated_buffer_size, consecutive_failures);
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                rtos_delay_milliseconds(1);
                continue;
            }
            
            consecutive_failures = 0;

            if (controller->config.audio.buffer_alloc_cb != NULL)
            {
                buffer_node->buffer.length = estimated_buffer_size;
                ret = controller->config.audio.buffer_alloc_cb(controller->config.user_data, &buffer_node->buffer);
                if (ret != AVDK_ERR_OK)
                {
                    LOGE("%s: Failed to allocate audio packet buffer, ret=%d\n", __func__, ret);
                    buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                    rtos_delay_milliseconds(1);
                    continue;
                }
            }
            else
            {
                LOGE("%s: Audio buffer alloc callback is NULL, cannot proceed\n", __func__);
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                controller->audio_parse_thread_running = false;
                controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;
                break;
            }

            buffer_node->buffer.pts = 0;
            
            rtos_lock_mutex(&controller->active_mutex);
            aparser = controller->active_container_parser;
            if (aparser == NULL)
            {
                rtos_unlock_mutex(&controller->active_mutex);
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                rtos_delay_milliseconds(1);
                continue;
            }

            if (aparser->read_audio_packet == NULL)
            {
                LOGE("%s: Container parser does not support read_audio_packet, cannot proceed\n", __func__);
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                controller->audio_parse_thread_running = false;
                controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;
                rtos_unlock_mutex(&controller->active_mutex);
                break;
            }
            
            ret = aparser->read_audio_packet(aparser,
                                             &buffer_node->buffer,
                                             target_pts);
            rtos_unlock_mutex(&controller->active_mutex);
            // Tag packet with the session id captured at loop entry.
            buffer_node->buffer.user_data = (void *)(uintptr_t)iter_session_id;

            if (ret != AVDK_ERR_OK)
            {
                if (controller->config.audio.buffer_free_cb != NULL && buffer_node->buffer.data != NULL)
                {
                    controller->config.audio.buffer_free_cb(controller->config.user_data, &buffer_node->buffer);
                }
                buffer_node->buffer.data = NULL;
                buffer_node->buffer.length = 0;
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                
                if (ret == AVDK_ERR_EOF)
                {
                    if (!controller->audio_eof_reached)
                    {
                        LOGI("%s: Audio EOF detected\n", __func__);
                        controller->audio_eof_reached = true;
                    }
                    if (controller->video_eof_reached && controller->audio_eof_reached)
                    {
                        avdk_err_t eof_ret = bk_video_player_handle_play_mode(controller);
                        if (eof_ret != AVDK_ERR_OK)
                        {
                            break;
                        }
                        if (controller->audio_parse_thread_running)
                        {
                            rtos_delay_milliseconds(10);
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        // Avoid log flooding while waiting for video EOF.
                        LOGV("%s: Waiting for video EOF (video_eof=%d, audio_eof=%d)\n",
                             __func__, controller->video_eof_reached, controller->audio_eof_reached);
                        rtos_delay_milliseconds(10);
                        continue;
                    }
                }
                LOGE("%s: Container audio parse failed, ret=%d\n", __func__, ret);
                rtos_delay_milliseconds(1);
                continue;
            }

            // If session changed during this loop iteration, drop the packet to avoid mixing sessions.
            if (iter_session_id != controller->play_session_id)
            {
                if (controller->config.audio.buffer_free_cb != NULL && buffer_node->buffer.data != NULL)
                {
                    controller->config.audio.buffer_free_cb(controller->config.user_data, &buffer_node->buffer);
                }
                buffer_node->buffer.data = NULL;
                buffer_node->buffer.length = 0;
                buffer_node->buffer.pts = 0;
                buffer_node->buffer.user_data = NULL;
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                continue;
            }

            // Verify buffer data is still valid after read
            if (buffer_node->buffer.data == NULL)
            {
                LOGE("%s: Container parser returned OK but buffer->data is NULL after read\n", __func__);
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
                rtos_delay_milliseconds(1);
                continue;
            }

            if (buffer_node->buffer.pts == VIDEO_PLAYER_PTS_INVALID)
            {
                /*
                 * Root fix:
                 * If parser didn't provide audio PTS (VIDEO_PLAYER_PTS_INVALID), generate a monotonic PTS for A/V sync.
                 * For packetized audio codecs, derive per-packet duration from codec rules/bitrate when possible.
                 *
                 * NOTE: 0 is a valid PTS. Container parsers should use VIDEO_PLAYER_PTS_INVALID to indicate "missing PTS".
                 */
                uint32_t sr = (controller->current_media_info.audio.sample_rate > 0) ? controller->current_media_info.audio.sample_rate : 8000;
                uint64_t frame_ms = 20;
                const uint32_t fmt = controller->current_media_info.audio.format;
                const uint32_t ch = (controller->current_media_info.audio.channels > 0) ? controller->current_media_info.audio.channels : 1;
                const uint32_t pkt_len = buffer_node->buffer.length;

                if (fmt == VIDEO_PLAYER_AUDIO_FORMAT_AAC)
                {
                    frame_ms = (1024ULL * 1000ULL + (uint64_t)sr / 2ULL) / (uint64_t)sr;
                }
                else if (fmt == VIDEO_PLAYER_AUDIO_FORMAT_G711A || fmt == VIDEO_PLAYER_AUDIO_FORMAT_G711U)
                {
                    // G.711 is 8-bit PCM companded. One byte per sample per channel.
                    // Duration(ms) = bytes / (sr * ch) seconds.
                    uint64_t denom = (uint64_t)sr * (uint64_t)ch;
                    if (pkt_len > 0 && denom > 0)
                    {
                        frame_ms = ((uint64_t)pkt_len * 1000ULL + denom / 2ULL) / denom;
                        if (frame_ms == 0)
                        {
                            frame_ms = 1;
                        }
                    }
                }
                else if (fmt == VIDEO_PLAYER_AUDIO_FORMAT_G722)
                {
                    // G.722 in this project uses 64kbps bitstream (no explicit bitrate signaled in AVI header).
                    // Duration(ms) = (bytes * 8) / bitrate seconds.
                    const uint64_t bitrate = 64000ULL;
                    if (pkt_len > 0)
                    {
                        frame_ms = ((uint64_t)pkt_len * 8ULL * 1000ULL + bitrate / 2ULL) / bitrate;
                        if (frame_ms == 0)
                        {
                            frame_ms = 1;
                        }
                    }
                }

                uint64_t pts_ms = 0;
                if (last_audio_pts_ms > 0)
                {
                    pts_ms = last_audio_pts_ms + frame_ms;
                }
                else
                {
                    pts_ms = video_player_get_current_time_ms(controller);
                    if (pts_ms == 0)
                    {
                        pts_ms = frame_ms;
                    }
                }

                buffer_node->buffer.pts = pts_ms;
                last_audio_pts_ms = pts_ms;
                LOGW("%s: Audio packet PTS missing, patched to %llu ms (sr=%u, fmt=%u)\n",
                     __func__, (unsigned long long)pts_ms, sr, controller->current_media_info.audio.format);
            }
            else
            {
                last_audio_pts_ms = buffer_node->buffer.pts;
            }

            audio_pipeline_update_pts(&controller->audio_pipeline, buffer_node->buffer.pts);

            buffer_pool_put_filled(&controller->audio_pipeline.parser_to_decode_pool, buffer_node);
        }

        LOGI("%s: Container audio parse thread stopped\n", __func__);
        rtos_set_semaphore(&controller->stop_sem);
    }

    LOGI("%s: Container audio parse thread exiting\n", __func__);
    rtos_delete_thread(NULL);
}

avdk_err_t bk_video_player_container_parse_init(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        LOGE("%s: Controller is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    if (rtos_init_semaphore(&controller->video_parse_sem, 1) != BK_OK)
    {
        LOGE("%s: Failed to init video parse semaphore\n", __func__);
        return AVDK_ERR_IO;
    }

    if (rtos_init_semaphore(&controller->audio_parse_sem, 1) != BK_OK)
    {
        LOGE("%s: Failed to init audio parse semaphore\n", __func__);
        rtos_deinit_semaphore(&controller->video_parse_sem);
        return AVDK_ERR_IO;
    }

    controller->video_parse_thread_running = false;
    controller->audio_parse_thread_running = false;
    controller->video_parse_thread_exit = false;
    controller->audio_parse_thread_exit = false;

    beken_thread_t video_parse_thread = NULL;
    avdk_err_t ret = rtos_create_thread(&video_parse_thread, BK_VP_VIDEO_PARSE_THREAD_PRIORITY, "vp_video_parse",
                                        (beken_thread_function_t)bk_video_player_container_video_parse_thread,
                                        BK_VP_VIDEO_PARSE_THREAD_STACK_SIZE,
                                        (beken_thread_arg_t)controller);
    if (ret != BK_OK)
    {
        LOGE("%s: Failed to create video parse thread, ret=%d\n", __func__, ret);
        rtos_deinit_semaphore(&controller->video_parse_sem);
        rtos_deinit_semaphore(&controller->audio_parse_sem);
        return AVDK_ERR_IO;
    }
    controller->video_parse_thread = video_parse_thread;

    beken_thread_t audio_parse_thread = NULL;
    ret = rtos_create_thread(&audio_parse_thread, BK_VP_AUDIO_PARSE_THREAD_PRIORITY, "vp_audio_parse",
                            (beken_thread_function_t)bk_video_player_container_audio_parse_thread,
                            BK_VP_AUDIO_PARSE_THREAD_STACK_SIZE,
                            (beken_thread_arg_t)controller);
    if (ret != BK_OK)
    {
        LOGE("%s: Failed to create audio parse thread, ret=%d\n", __func__, ret);
        controller->video_parse_thread_exit = true;
        rtos_set_semaphore(&controller->video_parse_sem);
        rtos_thread_join(controller->video_parse_thread);
        controller->video_parse_thread = NULL;
        rtos_deinit_semaphore(&controller->video_parse_sem);
        rtos_deinit_semaphore(&controller->audio_parse_sem);
        return AVDK_ERR_IO;
    }
    controller->audio_parse_thread = audio_parse_thread;

    LOGI("%s: Container parse resources initialized successfully\n", __func__);
    return AVDK_ERR_OK;
}

void bk_video_player_container_parse_deinit(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    if (controller->video_parse_thread != NULL)
    {
        controller->video_parse_thread_exit = true;
        controller->video_parse_thread_running = false;
        rtos_set_semaphore(&controller->video_parse_sem);
        rtos_thread_join(controller->video_parse_thread);
        controller->video_parse_thread = NULL;
    }

    if (controller->audio_parse_thread != NULL)
    {
        controller->audio_parse_thread_exit = true;
        controller->audio_parse_thread_running = false;
        rtos_set_semaphore(&controller->audio_parse_sem);
        rtos_thread_join(controller->audio_parse_thread);
        controller->audio_parse_thread = NULL;
    }

    if (controller->video_parse_sem != NULL)
    {
        rtos_deinit_semaphore(&controller->video_parse_sem);
    }

    if (controller->audio_parse_sem != NULL)
    {
        rtos_deinit_semaphore(&controller->audio_parse_sem);
    }

    LOGI("%s: Container parse resources deinitialized\n", __func__);
}

avdk_err_t bk_video_player_container_parser_list_add(private_video_player_ctlr_t *controller,
                                                     video_player_container_parser_ops_t *parser_ops)
{
    if (controller == NULL || parser_ops == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&controller->decoder_list_mutex);

    video_player_container_parser_node_t *new_node = os_malloc(sizeof(video_player_container_parser_node_t));
    if (new_node == NULL)
    {
        rtos_unlock_mutex(&controller->decoder_list_mutex);
        return AVDK_ERR_NOMEM;
    }

    new_node->ops = parser_ops;
    new_node->next = NULL;

    // Insert at tail to maintain registration order (first registered parser should be tried first).
    if (controller->container_parser_list == NULL)
    {
        controller->container_parser_list = new_node;
    }
    else
    {
        video_player_container_parser_node_t *cur = controller->container_parser_list;
        while (cur->next != NULL)
        {
            cur = cur->next;
        }
        cur->next = new_node;
    }

    rtos_unlock_mutex(&controller->decoder_list_mutex);
    return AVDK_ERR_OK;
}

void bk_video_player_container_parser_list_clear(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    rtos_lock_mutex(&controller->decoder_list_mutex);

    video_player_container_parser_node_t *node = controller->container_parser_list;
    while (node != NULL)
    {
        video_player_container_parser_node_t *next = node->next;
        os_free(node);
        node = next;
    }
    controller->container_parser_list = NULL;

    rtos_unlock_mutex(&controller->decoder_list_mutex);
}
