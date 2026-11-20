#include "os/os.h"
#include "os/mem.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "bk_video_player_ctlr.h"
#include "bk_video_player_video_decode.h"
#include <stdint.h>

#define TAG "video_decode"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#include "bk_video_player_buffer_pool.h"
#include "bk_video_player_pipeline.h"

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

static void video_player_update_current_time_ms(private_video_player_ctlr_t *controller, uint64_t time_ms)
{
    if (controller == NULL || controller->time_mutex == NULL)
    {
        return;
    }

    rtos_lock_mutex(&controller->time_mutex);
    if (time_ms > controller->current_time_ms)
    {
        controller->current_time_ms = time_ms;
        controller->last_time_update_tick_ms = rtos_get_time();
    }
    rtos_unlock_mutex(&controller->time_mutex);
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

// Video decode thread (pipeline stage 2 for video)
// Pipeline: Container parse -> Video decode -> Output
static void bk_video_player_video_decode_thread(void *arg)
{
    private_video_player_ctlr_t *controller = (private_video_player_ctlr_t *)arg;
    avdk_err_t ret = AVDK_ERR_OK;
    uint64_t last_video_pts = 0;
    uint32_t last_video_time_ms = rtos_get_time();
    uint32_t last_seen_session_id = 0;
    uint64_t delivered_frame_index = 0;

    LOGI("%s: Video decode thread started\n", __func__);

    rtos_set_semaphore(&controller->video_decode_sem);
    while (!controller->video_decode_thread_exit)
    {
        if (!controller->video_decode_thread_running)
        {
            rtos_delay_milliseconds(10);
            continue;
        }

        video_player_buffer_node_t *in_buffer_node = buffer_pool_get_filled(&controller->video_pipeline.parser_to_decode_pool);
        if (in_buffer_node == NULL)
        {
            if (controller->video_decode_thread_exit)
            {
                break;
            }
            // Check if playback is paused
            if (controller->is_paused)
            {
                rtos_delay_milliseconds(10);
            }
            continue;
        }

        if (controller->video_decode_thread_exit)
        {
            buffer_pool_put_filled(&controller->video_pipeline.parser_to_decode_pool, in_buffer_node);
            break;
        }

        // Capture current session id for this iteration.
        // If stop/start happens while decoding, controller->play_session_id will change and we must drop output safely.
        uint32_t iter_session_id = controller->play_session_id;
        if (iter_session_id != last_seen_session_id)
        {
            /*
             * IMPORTANT:
             * Decode threads are persistent across play sessions.
             * Reset per-session pacing state here; otherwise, after stop/start or seek restart,
             * last_video_pts from previous session can be larger than new out_buffer.pts and cause
             * unsigned underflow in pts_diff, leading to long waits and "no video output" (lcd_fps=0).
             */
            last_video_pts = 0;
            last_video_time_ms = rtos_get_time();
            last_seen_session_id = iter_session_id;
            delivered_frame_index = 0;
        }

        // Drop stale packets from previous playback sessions (e.g. stop/start or seek restart).
        uint32_t pkt_session_id = (uint32_t)(uintptr_t)in_buffer_node->buffer.user_data;
        if (pkt_session_id != 0 && pkt_session_id != controller->play_session_id)
        {
            if (controller->config.video.packet_buffer_free_cb != NULL && in_buffer_node->buffer.data != NULL)
            {
                controller->config.video.packet_buffer_free_cb(controller->config.user_data, &in_buffer_node->buffer);
            }
            buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, in_buffer_node);
            continue;
        }

        // Check if playback is paused
        if (controller->is_paused)
        {
            buffer_pool_put_filled(&controller->video_pipeline.parser_to_decode_pool, in_buffer_node);
            rtos_delay_milliseconds(10);
            continue;
        }

        // Check active decoder existence under lock to avoid races with stop/close.
        bool has_active_decoder = false;
        rtos_lock_mutex(&controller->active_mutex);
        has_active_decoder = (controller->active_video_decoder != NULL);
        rtos_unlock_mutex(&controller->active_mutex);
        if (!has_active_decoder)
        {
            buffer_pool_put_filled(&controller->video_pipeline.parser_to_decode_pool, in_buffer_node);
            rtos_delay_milliseconds(10);
            continue;
        }

        uint64_t in_pts_ms = in_buffer_node->buffer.pts;

        // Seek preroll drop control (shared by video/audio threads):
        // During seek restart, video parse thread enables video_seek_drop_enable and sets the target PTS.
        // Audio decode thread may gate audio output until this flag is cleared, so video decode must NOT
        // wait on audio clock while the flag is enabled, otherwise it can deadlock (audio waits for video,
        // video waits for audio).
        bool seek_drop_enable = false;
        uint64_t seek_drop_until_pts_ms = 0;
        if (controller->time_mutex != NULL)
        {
            rtos_lock_mutex(&controller->time_mutex);
            seek_drop_enable = controller->video_seek_drop_enable;
            seek_drop_until_pts_ms = controller->video_seek_drop_until_pts_ms;
            rtos_unlock_mutex(&controller->time_mutex);
        }

        if (controller->clock_source == VIDEO_PLAYER_CLOCK_AUDIO && in_pts_ms > 0)
        {
            const uint64_t drop_threshold_ms = 200; // tolerate small jitter without dropping
            uint64_t cur_time_ms = video_player_get_effective_audio_time_ms_for_video(controller);

            if (!(seek_drop_enable && in_pts_ms < seek_drop_until_pts_ms) &&
                cur_time_ms > 0 && in_pts_ms + drop_threshold_ms < cur_time_ms)
            {
                LOGI("%s: Dropping packet before decode, pts=%llu, cur_time_ms=%llu\n",
                     __func__, (unsigned long long)in_pts_ms, (unsigned long long)cur_time_ms);
                if (controller->config.video.packet_buffer_free_cb != NULL && in_buffer_node->buffer.data != NULL)
                {
                    controller->config.video.packet_buffer_free_cb(controller->config.user_data, &in_buffer_node->buffer);
                }
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, in_buffer_node);
                continue;
            }
        }

        video_player_buffer_t out_buffer = {0};
        if (controller->config.video.buffer_alloc_cb != NULL)
        {
            // Output buffer format is decided by upper layer and passed via controller->config.video.output_format.
            // We pass this information to decoder via out_buffer.frame_buffer->fmt (frame_buffer_t),
            // and allocate buffer size based on bytes-per-pixel of target format.
            uint32_t bytes_per_pixel = 2; // default YUYV/RGB565
            if (controller->config.video.output_format == PIXEL_FMT_RGB888)
            {
                bytes_per_pixel = 3;
            }
            out_buffer.length = controller->current_media_info.video.width * controller->current_media_info.video.height * bytes_per_pixel;
            ret = controller->config.video.buffer_alloc_cb(controller->config.user_data, &out_buffer);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: Failed to allocate video output buffer, ret=%d\n", __func__, ret);
                if (controller->config.video.packet_buffer_free_cb != NULL && in_buffer_node->buffer.data != NULL)
                {
                    controller->config.video.packet_buffer_free_cb(controller->config.user_data, &in_buffer_node->buffer);
                }
                buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, in_buffer_node);
                continue;
            }
        }
        else
        {
            LOGE("%s: Video buffer alloc callback is NULL, cannot proceed\n", __func__);
            if (controller->config.video.packet_buffer_free_cb != NULL && in_buffer_node->buffer.data != NULL)
            {
                controller->config.video.packet_buffer_free_cb(controller->config.user_data, &in_buffer_node->buffer);
            }
            buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, in_buffer_node);
            controller->video_decode_thread_running = false;
            controller->video_parse_thread_running = false;
            controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;
            break;
        }

        if (in_buffer_node->buffer.data == NULL)
        {
            LOGE("%s: Input buffer data is NULL, cannot decode\n", __func__);
            if (controller->config.video.packet_buffer_free_cb != NULL)
            {
                controller->config.video.packet_buffer_free_cb(controller->config.user_data, &in_buffer_node->buffer);
            }
            buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, in_buffer_node);
            continue;
        }

        // Pass expected output format to decoder via out_buffer.frame_buffer->fmt when possible.
        if (out_buffer.frame_buffer != NULL)
        {
            frame_buffer_t *fb = (frame_buffer_t *)out_buffer.frame_buffer;
            fb->fmt = controller->config.video.output_format;
        }

        // For now, video decode is handled synchronously in this thread (no async decode).
        rtos_lock_mutex(&controller->active_mutex);
        video_player_video_decoder_ops_t *active_decoder = controller->active_video_decoder;
        if (active_decoder == NULL)
        {
            // Decoder was deinitialized concurrently (e.g. stop), drop this frame safely.
            rtos_unlock_mutex(&controller->active_mutex);
            ret = AVDK_ERR_INVAL;
        }
        else
        {
            LOGV("%s: Decoding frame, pts=%llu, session_id=%u\n", __func__, (unsigned long long)in_buffer_node->buffer.pts, iter_session_id);
            ret = active_decoder->decode(active_decoder,
                                         &in_buffer_node->buffer, &out_buffer,
                                         controller->config.video.output_format);
        }

        // Runtime decoder fallback is intentionally not supported here.
        // Decoder selection should be done during play/open. If decoder returns UNSUPPORTED at runtime,
        // stop playback to avoid infinite loops and inconsistent resource states.
        if (ret == AVDK_ERR_UNSUPPORTED &&
            controller->current_media_info.video.format == VIDEO_PLAYER_VIDEO_FORMAT_MJPEG)
        {
            LOGE("%s: MJPEG decoder returned UNSUPPORTED at runtime, stopping playback\n", __func__);
            controller->video_decode_thread_running = false;
            controller->video_parse_thread_running = false;
            controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;
        }

        rtos_unlock_mutex(&controller->active_mutex);

        // Always release encoded packet buffer after decode (or retry) completes.
        if (controller->config.video.packet_buffer_free_cb != NULL && in_buffer_node->buffer.data != NULL)
        {
            controller->config.video.packet_buffer_free_cb(controller->config.user_data, &in_buffer_node->buffer);
        }

        buffer_pool_put_empty(&controller->video_pipeline.parser_to_decode_pool, in_buffer_node);

        bool handed_to_user = false;

        if (ret == AVDK_ERR_OK)
        {
            // Preserve PTS from input packet if decoder didn't set it.
            if (out_buffer.pts == 0 && in_pts_ms > 0)
            {
                out_buffer.pts = in_pts_ms;
            }

            // If session has changed or playback is no longer running, do not output frames.
            if (iter_session_id != controller->play_session_id || !controller->video_decode_thread_running)
            {
                if (!handed_to_user && controller->config.video.buffer_free_cb != NULL)
                {
                    controller->config.video.buffer_free_cb(controller->config.user_data, &out_buffer);
                }
                continue;
            }

            if (out_buffer.pts > 0)
            {
                if (controller->clock_source == VIDEO_PLAYER_CLOCK_AUDIO)
                {
                    // Audio-driven: display when audio clock reaches this video PTS.
                    // - If video is too far behind audio clock, drop it to catch up.
                    // - If video is ahead of audio clock, wait until clock catches up.
                    uint64_t cur_time_ms = video_player_get_effective_audio_time_ms_for_video(controller);
                    const uint64_t drop_threshold_ms = 200; // tolerate small jitter without dropping

                    // IMPORTANT:
                    // If seek preroll drop is enabled, do NOT drop frames based on audio clock before
                    // reaching the seek target. Dropping them here would prevent clearing the drop flag,
                    // and audio thread would keep gating until timeout, causing A/V desync.
                    if (!(seek_drop_enable && out_buffer.pts < seek_drop_until_pts_ms) &&
                        cur_time_ms > 0 && out_buffer.pts + drop_threshold_ms < cur_time_ms)
                    {
                        LOGI("%s %d: Dropping frame, pts=%llu, cur_time_ms=%llu\n", __func__, __LINE__, (unsigned long long)out_buffer.pts, (unsigned long long)cur_time_ms);
                        if (!handed_to_user && controller->config.video.buffer_free_cb != NULL)
                        {
                            controller->config.video.buffer_free_cb(controller->config.user_data, &out_buffer);
                        }
                        continue;
                    }

                    bool switched_to_video_clock = false;

                    // During seek preroll, do NOT wait on audio clock. Audio output may be gated by
                    // video_seek_drop_enable, so waiting here would deadlock (audio waits for video,
                    // video waits for audio).
                    while (!seek_drop_enable &&
                           out_buffer.pts > cur_time_ms &&
                           controller->video_decode_thread_running &&
                           !controller->is_paused &&
                           iter_session_id == controller->play_session_id)
                    {
                        uint64_t diff = out_buffer.pts - cur_time_ms;
                        if (diff > 50)
                        {
                            diff = 50;
                        }
                        rtos_delay_milliseconds((uint32_t)diff);
                        cur_time_ms = video_player_get_effective_audio_time_ms_for_video(controller);

                        // If audio clock stops updating (e.g., audio decode failure or missing PTS),
                        // video decode will deadlock here forever. Detect and fall back to VIDEO clock.
                        uint32_t now_tick = rtos_get_time();
                        uint32_t last_update_tick = 0;
                        if (controller->time_mutex != NULL)
                        {
                            rtos_lock_mutex(&controller->time_mutex);
                            last_update_tick = controller->last_time_update_tick_ms;
                            rtos_unlock_mutex(&controller->time_mutex);
                        }
                        if (last_update_tick > 0 && (now_tick - last_update_tick) > 500)
                        {
                            LOGW("%s: Audio clock stalled, switch clock source to VIDEO (video_pts=%llu, audio_sync_time=%llu)\n",
                                 __func__, (unsigned long long)out_buffer.pts, (unsigned long long)cur_time_ms);
                            controller->clock_source = VIDEO_PLAYER_CLOCK_VIDEO;
                            switched_to_video_clock = true;
                            break;
                        }
                    }

                    if (iter_session_id != controller->play_session_id || !controller->video_decode_thread_running)
                    {
                        if (!handed_to_user && controller->config.video.buffer_free_cb != NULL)
                        {
                            controller->config.video.buffer_free_cb(controller->config.user_data, &out_buffer);
                        }
                        continue;
                    }

                    // If seek target has been reached, clear seek drop flag to unblock audio gating.
                    // This must NOT depend on whether decode_complete_cb is provided.
                    if (controller->time_mutex != NULL && seek_drop_enable)
                    {
                        rtos_lock_mutex(&controller->time_mutex);
                        if (controller->video_seek_drop_enable &&
                            controller->video_seek_drop_until_pts_ms > 0 &&
                            out_buffer.pts >= controller->video_seek_drop_until_pts_ms)
                        {
                            controller->video_seek_drop_enable = false;
                            controller->video_seek_drop_until_pts_ms = 0;
                        }
                        rtos_unlock_mutex(&controller->time_mutex);
                    }

                    // If we switched to VIDEO clock during waiting, run VIDEO-driven pacing and clock update.
                    if (switched_to_video_clock || controller->clock_source == VIDEO_PLAYER_CLOCK_VIDEO)
                    {
                        uint32_t current_time_ms = rtos_get_time();
                        if (last_video_pts > 0)
                        {
                            uint64_t pts_diff = out_buffer.pts - last_video_pts;
                            uint32_t elapsed_time_ms = 0;
                            if (current_time_ms >= last_video_time_ms)
                            {
                                elapsed_time_ms = current_time_ms - last_video_time_ms;
                            }
                            else
                            {
                                elapsed_time_ms = (0xFFFFFFFFU - last_video_time_ms) + current_time_ms + 1;
                            }

                            if (pts_diff > elapsed_time_ms)
                            {
                                uint32_t wait_time_ms = (uint32_t)(pts_diff - elapsed_time_ms);
                                if (wait_time_ms > 1000)
                                {
                                    wait_time_ms = 1000;
                                }
                                if (wait_time_ms > 0)
                                {
                                    LOGV("%s: Waiting %u ms for PTS sync (pts_diff=%llu, elapsed=%u)\n",
                                         __func__, wait_time_ms, pts_diff, elapsed_time_ms);
                                    rtos_delay_milliseconds(wait_time_ms);
                                    current_time_ms = rtos_get_time();
                                }
                            }
                        }
                        last_video_pts = out_buffer.pts;
                        last_video_time_ms = current_time_ms;

                        if (iter_session_id == controller->play_session_id && controller->video_decode_thread_running)
                        {
                            video_player_update_current_time_ms(controller, out_buffer.pts);
                        }
                    }
                }
                else
                {
                    // Video-driven: keep original pacing based on video PTS and system time.
                    uint32_t current_time_ms = rtos_get_time();
                    if (last_video_pts > 0)
                    {
                        uint64_t pts_diff = out_buffer.pts - last_video_pts;
                        uint32_t elapsed_time_ms = 0;
                        if (current_time_ms >= last_video_time_ms)
                        {
                            elapsed_time_ms = current_time_ms - last_video_time_ms;
                        }
                        else
                        {
                            elapsed_time_ms = (0xFFFFFFFFU - last_video_time_ms) + current_time_ms + 1;
                        }

                        if (pts_diff > elapsed_time_ms)
                        {
                            uint32_t wait_time_ms = (uint32_t)(pts_diff - elapsed_time_ms);
                            if (wait_time_ms > 1000)
                            {
                                wait_time_ms = 1000;
                            }
                            if (wait_time_ms > 0)
                            {
                                LOGV("%s: Waiting %u ms for PTS sync (pts_diff=%llu, elapsed=%u)\n",
                                     __func__, wait_time_ms, pts_diff, elapsed_time_ms);
                                rtos_delay_milliseconds(wait_time_ms);
                                current_time_ms = rtos_get_time();
                            }
                        }
                    }
                    last_video_pts = out_buffer.pts;
                    last_video_time_ms = current_time_ms;

                    /*
                     * Update the single current playback time point if video is the clock source.
                     *
                     * IMPORTANT:
                     * Session id may change during pacing/waiting (e.g. stop/start racing with decode).
                     * Re-check session id here to avoid stale frames from the previous session pushing
                     * current_time_ms and causing next playback to start from an old timestamp.
                     */
                    if (iter_session_id == controller->play_session_id && controller->video_decode_thread_running)
                    {
                        // Do not let preroll keyframe (pts < seek target) move current_time_ms backward.
                        bool allow_time_update = true;
                        if (controller->time_mutex != NULL)
                        {
                            rtos_lock_mutex(&controller->time_mutex);
                            if (controller->video_seek_drop_enable &&
                                out_buffer.pts < controller->video_seek_drop_until_pts_ms)
                            {
                                allow_time_update = false;
                            }
                            else if (controller->video_seek_drop_enable &&
                                     out_buffer.pts >= controller->video_seek_drop_until_pts_ms)
                            {
                                // First decoded frame reaches seek target, disable dropping globally.
                                controller->video_seek_drop_enable = false;
                                controller->video_seek_drop_until_pts_ms = 0;
                            }
                            rtos_unlock_mutex(&controller->time_mutex);
                        }

                        if (allow_time_update)
                        {
                            video_player_update_current_time_ms(controller, out_buffer.pts);
                        }
                    }
                }
            }

            if (controller->config.video.decode_complete_cb != NULL)
            {
                // Seek preroll output dropping:
                // When seeking to a non-keyframe, parser may feed a keyframe packet first.
                // We decode it for reference establishment but must NOT deliver it to upper layer.
                bool drop_output = false;
                uint64_t drop_until_pts = 0;
                if (controller->time_mutex != NULL)
                {
                    rtos_lock_mutex(&controller->time_mutex);
                    if (controller->video_seek_drop_enable)
                    {
                        drop_until_pts = controller->video_seek_drop_until_pts_ms;
                        if (out_buffer.pts < drop_until_pts)
                        {
                            drop_output = true;
                        }
                    }
                    rtos_unlock_mutex(&controller->time_mutex);
                }

                if (!drop_output)
                {
                    // First delivered frame at/after target pts disables dropping.
                    if (controller->time_mutex != NULL)
                    {
                        rtos_lock_mutex(&controller->time_mutex);
                        if (controller->video_seek_drop_enable && out_buffer.pts >= controller->video_seek_drop_until_pts_ms)
                        {
                            controller->video_seek_drop_enable = false;
                            controller->video_seek_drop_until_pts_ms = 0;
                        }
                        rtos_unlock_mutex(&controller->time_mutex);
                    }

                    delivered_frame_index++;

                    video_player_video_frame_meta_t meta;
                    os_memset(&meta, 0, sizeof(meta));
                    meta.video = controller->current_media_info.video;
                    meta.frame_index = delivered_frame_index;
                    meta.pts_ms = out_buffer.pts;

                    controller->config.video.decode_complete_cb(controller->config.user_data, &meta, &out_buffer);
                    handed_to_user = true;
                }
            }
        }

        // Buffer ownership rule:
        // - If we handed the buffer to user callback, user must free it.
        // - If we did NOT hand it out (decode failed / no callback), we must free it here.
        if (!handed_to_user && controller->config.video.buffer_free_cb != NULL)
        {
            controller->config.video.buffer_free_cb(controller->config.user_data, &out_buffer);
        }
    }

    LOGI("%s: Video decode thread exiting\n", __func__);
    rtos_set_semaphore(&controller->video_decode_sem);
    rtos_delete_thread(NULL);
}

avdk_err_t bk_video_player_video_decode_init(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        LOGE("%s: Controller is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    if (rtos_init_semaphore(&controller->video_decode_sem, 1) != BK_OK)
    {
        LOGE("%s: Failed to init video decode semaphore\n", __func__);
        return AVDK_ERR_IO;
    }

    controller->video_decode_thread_running = false;
    controller->video_decode_thread_exit = false;

    beken_thread_t video_thread = NULL;
    avdk_err_t ret = rtos_create_thread(&video_thread, BK_VP_VIDEO_DECODE_THREAD_PRIORITY, "video_decode",
                                        (beken_thread_function_t)bk_video_player_video_decode_thread,
                                        BK_VP_VIDEO_DECODE_THREAD_STACK_SIZE,
                                        (beken_thread_arg_t)controller);
    if (ret != BK_OK)
    {
        LOGE("%s: Failed to create video decode thread, ret=%d\n", __func__, ret);
        rtos_deinit_semaphore(&controller->video_decode_sem);
        return AVDK_ERR_IO;
    }
    // Wait until decode thread is started.
    rtos_get_semaphore(&controller->video_decode_sem, BEKEN_WAIT_FOREVER);
    controller->video_decode_thread = video_thread;

    LOGI("%s: Video decode resources initialized successfully\n", __func__);
    return AVDK_ERR_OK;
}

void bk_video_player_video_decode_deinit(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    if (controller->video_decode_thread != NULL)
    {
        controller->video_decode_thread_exit = true;
        controller->video_decode_thread_running = false;
        rtos_get_semaphore(&controller->video_decode_sem, BEKEN_WAIT_FOREVER);
        controller->video_decode_thread = NULL;
        rtos_deinit_semaphore(&controller->video_decode_sem);
        controller->video_decode_sem = NULL;
    }
}

avdk_err_t bk_video_player_video_decoder_list_add(private_video_player_ctlr_t *controller,
                                                  video_player_video_decoder_ops_t *decoder_ops)
{
    if (controller == NULL || decoder_ops == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&controller->decoder_list_mutex);

    video_player_video_decoder_node_t *new_node = os_malloc(sizeof(video_player_video_decoder_node_t));
    if (new_node == NULL)
    {
        rtos_unlock_mutex(&controller->decoder_list_mutex);
        return AVDK_ERR_NOMEM;
    }

    new_node->ops = decoder_ops;
    new_node->next = NULL;

    // Insert at tail to maintain registration order (first registered decoder should be tried first)
    if (controller->video_decoder_list == NULL)
    {
        controller->video_decoder_list = new_node;
    }
    else
    {
        video_player_video_decoder_node_t *current = controller->video_decoder_list;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_node;
    }

    rtos_unlock_mutex(&controller->decoder_list_mutex);
    return AVDK_ERR_OK;
}

void bk_video_player_video_decoder_list_clear(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    rtos_lock_mutex(&controller->decoder_list_mutex);

    video_player_video_decoder_node_t *node = controller->video_decoder_list;
    while (node != NULL)
    {
        video_player_video_decoder_node_t *next = node->next;
        os_free(node);
        node = next;
    }
    controller->video_decoder_list = NULL;

    rtos_unlock_mutex(&controller->decoder_list_mutex);
}

