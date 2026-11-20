#include "os/os.h"
#include "os/mem.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "bk_video_player_ctlr.h"
#include "bk_video_player_audio_decode.h"
#include <stdint.h>

#define TAG "audio_decode"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#include "bk_video_player_buffer_pool.h"
#include "bk_video_player_pipeline.h"

// Fallback defaults for build environments that don't yet have the new Kconfig symbols in sdkconfig.h.
// Defaults match the previous hard-coded behavior.
#ifndef CONFIG_BK_VIDEO_PLAYER_AUDIO_DECODE_THREAD_PRIORITY
#define CONFIG_BK_VIDEO_PLAYER_AUDIO_DECODE_THREAD_PRIORITY (BEKEN_DEFAULT_WORKER_PRIORITY)
#endif
#ifndef CONFIG_BK_VIDEO_PLAYER_AUDIO_DECODE_THREAD_STACK_SIZE
#define CONFIG_BK_VIDEO_PLAYER_AUDIO_DECODE_THREAD_STACK_SIZE (8 * 1024)
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
        // Keep stall detection in video decode thread in sync with audio-driven updates.
        controller->last_time_update_tick_ms = rtos_get_time();
    }
    rtos_unlock_mutex(&controller->time_mutex);
}

static inline uint32_t video_player_tick_elapsed_ms(uint32_t now_ms, uint32_t last_ms)
{
    if (now_ms >= last_ms)
    {
        return now_ms - last_ms;
    }
    return (0xFFFFFFFFU - last_ms) + now_ms + 1;
}

static inline uint64_t video_player_calc_audio_clock_ms(uint64_t base_pts_ms,
                                                        uint32_t base_tick_ms,
                                                        uint32_t now_tick_ms,
                                                        bool base_valid)
{
    if (!base_valid || base_tick_ms == 0)
    {
        return 0;
    }
    return base_pts_ms + (uint64_t)video_player_tick_elapsed_ms(now_tick_ms, base_tick_ms);
}

#define BK_VP_AUDIO_PREFETCH_PACKETS 2
#define BK_VP_AUDIO_PREFETCH_LEAD_MS 40

typedef struct
{
    bool valid;
    uint32_t session_id;
    video_player_buffer_t buffer;
} bk_vp_audio_pending_t;

static void video_player_audio_flush_pending(private_video_player_ctlr_t *controller,
                                             bk_vp_audio_pending_t *pending,
                                             uint8_t pending_size,
                                             uint8_t *pending_head,
                                             uint8_t *pending_count)
{
    if (controller == NULL || pending == NULL || pending_head == NULL || pending_count == NULL || pending_size == 0)
    {
        return;
    }
    while (*pending_count > 0)
    {
        bk_vp_audio_pending_t *p = &pending[*pending_head];
        if (p->valid && controller->config.audio.buffer_free_cb != NULL)
        {
            controller->config.audio.buffer_free_cb(controller->config.user_data, &p->buffer);
        }
        p->valid = false;
        os_memset(&p->buffer, 0, sizeof(p->buffer));
        *pending_head = (uint8_t)((*pending_head + 1) % pending_size);
        (*pending_count)--;
    }
    *pending_head = 0;
    *pending_count = 0;
}

// Audio decode thread (pipeline stage 2 for audio)
// Pipeline: File audio decode -> Audio decode -> Output
static void bk_video_player_audio_decode_thread(void *arg)
{
    private_video_player_ctlr_t *controller = (private_video_player_ctlr_t *)arg;
    avdk_err_t ret = AVDK_ERR_OK;
    uint32_t alloc_fail_cnt = 0;
    uint32_t last_seen_session_id = 0;
    uint64_t delivered_packet_index = 0;
    uint32_t seek_gate_start_tick_ms = 0;
    uint64_t audio_pace_base_pts_ms = 0;
    uint32_t audio_pace_base_tick_ms = 0;
    bool audio_pace_base_valid = false;
    bool was_paused = false;
    uint32_t pause_tick_ms = 0;

    bk_vp_audio_pending_t pending[BK_VP_AUDIO_PREFETCH_PACKETS];
    uint8_t pending_head = 0;
    uint8_t pending_count = 0;
    os_memset(pending, 0, sizeof(pending));

    LOGI("%s: Audio decode thread started\n", __func__);

    rtos_set_semaphore(&controller->audio_decode_sem);
    while (!controller->audio_decode_thread_exit)
    {
        if (!controller->audio_decode_thread_running)
        {
            video_player_audio_flush_pending(controller, pending, BK_VP_AUDIO_PREFETCH_PACKETS, &pending_head, &pending_count);
            rtos_delay_milliseconds(10);
            continue;
        }

        if (controller->is_paused)
        {
            if (!was_paused)
            {
                pause_tick_ms = rtos_get_time();
                was_paused = true;
            }
            rtos_delay_milliseconds(10);
            continue;
        }
        if (was_paused)
        {
            uint32_t now_tick_ms = rtos_get_time();
            uint32_t paused_ms = video_player_tick_elapsed_ms(now_tick_ms, pause_tick_ms);
            if (audio_pace_base_tick_ms != 0)
            {
                audio_pace_base_tick_ms += paused_ms;
            }
            was_paused = false;
            pause_tick_ms = 0;
        }

        // Decode-ahead strategy:
        // - Keep a small queue of decoded packets (2 by default).
        // - Only throttle delivery when we are "too far ahead" of the audio clock.
        while (pending_count < BK_VP_AUDIO_PREFETCH_PACKETS && controller->audio_decode_thread_running && !controller->audio_decode_thread_exit)
        {
            video_player_buffer_node_t *in_buffer = buffer_pool_get_filled(&controller->audio_pipeline.parser_to_decode_pool);
            if (in_buffer == NULL)
            {
                break;
            }

            // Capture session id for this packet and drop stale packets from previous sessions.
            uint32_t iter_session_id = controller->play_session_id;
            if (iter_session_id != last_seen_session_id)
            {
                alloc_fail_cnt = 0;
                last_seen_session_id = iter_session_id;
                delivered_packet_index = 0;
                seek_gate_start_tick_ms = 0;
                audio_pace_base_pts_ms = 0;
                audio_pace_base_tick_ms = 0;
                audio_pace_base_valid = false;
                video_player_audio_flush_pending(controller, pending, BK_VP_AUDIO_PREFETCH_PACKETS, &pending_head, &pending_count);
            }

            uint32_t pkt_session_id = (uint32_t)(uintptr_t)in_buffer->buffer.user_data;
            if (pkt_session_id != 0 && pkt_session_id != controller->play_session_id)
            {
                if (controller->config.audio.buffer_free_cb != NULL && in_buffer->buffer.data != NULL)
                {
                    controller->config.audio.buffer_free_cb(controller->config.user_data, &in_buffer->buffer);
                }
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, in_buffer);
                continue;
            }

            // Seek gating: do not output audio until video preroll drop completes.
            if (controller->time_mutex != NULL)
            {
                if (seek_gate_start_tick_ms == 0)
                {
                    seek_gate_start_tick_ms = rtos_get_time();
                }

                while (controller->audio_decode_thread_running && !controller->audio_decode_thread_exit)
                {
                    bool gate_on = false;
                    uint64_t gate_pts = 0;

                    rtos_lock_mutex(&controller->time_mutex);
                    gate_on = controller->video_seek_drop_enable;
                    gate_pts = controller->video_seek_drop_until_pts_ms;
                    rtos_unlock_mutex(&controller->time_mutex);

                    if (!gate_on)
                    {
                        break;
                    }
                    if (gate_pts == 0)
                    {
                        rtos_lock_mutex(&controller->time_mutex);
                        controller->video_seek_drop_enable = false;
                        controller->video_seek_drop_until_pts_ms = 0;
                        rtos_unlock_mutex(&controller->time_mutex);
                        break;
                    }

                    uint32_t now = rtos_get_time();
                    uint32_t elapsed = video_player_tick_elapsed_ms(now, seek_gate_start_tick_ms);
                    if (elapsed > 1000)
                    {
                        LOGW("%s: Seek gating timeout (%u ms), allow audio output\n", __func__, elapsed);
                        rtos_lock_mutex(&controller->time_mutex);
                        controller->video_seek_drop_enable = false;
                        controller->video_seek_drop_until_pts_ms = 0;
                        rtos_unlock_mutex(&controller->time_mutex);
                        break;
                    }

                    if (iter_session_id != controller->play_session_id)
                    {
                        break;
                    }
                    rtos_delay_milliseconds(2);
                }
            }

            const bool is_pcm = (controller->current_media_info.audio.format == VIDEO_PLAYER_AUDIO_FORMAT_PCM);
            video_player_buffer_t out_buffer = {0};
            if (!is_pcm)
            {
                if (controller->config.audio.buffer_alloc_cb != NULL)
                {
                    uint32_t ch = (controller->current_media_info.audio.channels > 0) ? controller->current_media_info.audio.channels : 1;
                    uint32_t bps = (controller->current_media_info.audio.bits_per_sample > 0) ? controller->current_media_info.audio.bits_per_sample : 16;
                    uint32_t bytes_per_sample = (bps + 7U) / 8U;
                    if (bytes_per_sample == 0)
                    {
                        bytes_per_sample = 2;
                    }

                    const uint32_t in_len = in_buffer->buffer.length;
                    uint64_t out_len64 = 0;
                    bool format_supported = true;
                    switch (controller->current_media_info.audio.format)
                    {
                        case VIDEO_PLAYER_AUDIO_FORMAT_AAC:
                            out_len64 = 4096ULL * (uint64_t)ch;
                            break;
                        case VIDEO_PLAYER_AUDIO_FORMAT_MP3:
                            out_len64 = 4608ULL;
                            break;
                        case VIDEO_PLAYER_AUDIO_FORMAT_G711A:
                        case VIDEO_PLAYER_AUDIO_FORMAT_G711U:
                            out_len64 = (uint64_t)in_len * 2ULL;
                            break;
                        case VIDEO_PLAYER_AUDIO_FORMAT_ADPCM:
                            out_len64 = (uint64_t)in_len * 4ULL;
                            break;
                        case VIDEO_PLAYER_AUDIO_FORMAT_G722:
                            out_len64 = (uint64_t)in_len * 4ULL;
                            break;
                        case VIDEO_PLAYER_AUDIO_FORMAT_OPUS:
                        {
                            uint32_t sr = (controller->current_media_info.audio.sample_rate > 0) ? controller->current_media_info.audio.sample_rate : 48000;
                            uint64_t samples = ((uint64_t)sr * 120ULL) / 1000ULL;
                            out_len64 = samples * (uint64_t)ch * (uint64_t)bytes_per_sample;
                            break;
                        }
                        case VIDEO_PLAYER_AUDIO_FORMAT_AMR:
                        {
                            uint32_t sr = (controller->current_media_info.audio.sample_rate > 0) ? controller->current_media_info.audio.sample_rate : 8000;
                            uint64_t samples = ((uint64_t)sr * 20ULL) / 1000ULL;
                            out_len64 = samples * (uint64_t)ch * 2ULL;
                            break;
                        }
                        default:
                            format_supported = false;
                            break;
                    }

                    if (!format_supported)
                    {
                        LOGE("%s: Unsupported audio format: %d\n",
                             __func__, (int)controller->current_media_info.audio.format);

                        if (controller->config.audio.buffer_free_cb != NULL && in_buffer->buffer.data != NULL)
                        {
                            controller->config.audio.buffer_free_cb(controller->config.user_data, &in_buffer->buffer);
                        }
                        buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, in_buffer);

                        controller->audio_decode_thread_running = false;
                        controller->audio_parse_thread_running = false;
                        controller->video_decode_thread_running = false;
                        controller->video_parse_thread_running = false;
                        controller->module_status.status = VIDEO_PLAYER_STATUS_STOPPED;
                        ret = AVDK_ERR_UNSUPPORTED;
                        break;
                    }

                    if (out_len64 > 65536ULL)
                    {
                        out_len64 = 65536ULL;
                    }
                    out_buffer.length = (uint32_t)out_len64;
                    ret = controller->config.audio.buffer_alloc_cb(controller->config.user_data, &out_buffer);
                    if (ret != AVDK_ERR_OK)
                    {
                        alloc_fail_cnt++;
                        if (alloc_fail_cnt <= 3 || (alloc_fail_cnt % 50 == 0))
                        {
                            LOGW("%s: audio_buffer_alloc_cb failed, ret=%d, need_len=%u, in_len=%u\n",
                                 __func__, ret, out_buffer.length, in_buffer->buffer.length);
                        }
                        if (controller->config.audio.buffer_free_cb != NULL && in_buffer->buffer.data != NULL)
                        {
                            controller->config.audio.buffer_free_cb(controller->config.user_data, &in_buffer->buffer);
                        }
                        buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, in_buffer);
                        continue;
                    }
                    alloc_fail_cnt = 0;
                }
                else
                {
                    LOGE("%s: Audio buffer alloc callback is NULL, cannot proceed\n", __func__);
                    if (controller->config.audio.buffer_free_cb != NULL && in_buffer->buffer.data != NULL)
                    {
                        controller->config.audio.buffer_free_cb(controller->config.user_data, &in_buffer->buffer);
                    }
                    buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, in_buffer);
                    controller->audio_decode_thread_running = false;
                    controller->audio_parse_thread_running = false;
                    controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;
                    break;
                }

                // Preserve input PTS for the decoded output.
                // Most decoder implementations do not set out_buffer.pts, so the controller must carry it over.
                out_buffer.pts = in_buffer->buffer.pts;

                rtos_lock_mutex(&controller->active_mutex);
                video_player_audio_decoder_ops_t *decoder = controller->active_audio_decoder;
                if (decoder == NULL)
                {
                    rtos_unlock_mutex(&controller->active_mutex);
                    ret = AVDK_ERR_NODEV;
                }
                else
                {
                    ret = decoder->decode(decoder, &in_buffer->buffer, &out_buffer);
                    rtos_unlock_mutex(&controller->active_mutex);
                }

                if (controller->config.audio.buffer_free_cb != NULL && in_buffer->buffer.data != NULL)
                {
                    controller->config.audio.buffer_free_cb(controller->config.user_data, &in_buffer->buffer);
                }
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, in_buffer);
            }
            else
            {
                // PCM zero-copy path: transfer ownership to output.
                out_buffer = in_buffer->buffer;
                ret = AVDK_ERR_OK;
                in_buffer->buffer.data = NULL;
                in_buffer->buffer.length = 0;
                in_buffer->buffer.pts = 0;
                in_buffer->buffer.user_data = NULL;
                buffer_pool_put_empty(&controller->audio_pipeline.parser_to_decode_pool, in_buffer);
            }

            if (ret != AVDK_ERR_OK)
            {
                if (controller->config.audio.buffer_free_cb != NULL)
                {
                    controller->config.audio.buffer_free_cb(controller->config.user_data, &out_buffer);
                }
                continue;
            }

            // Drop audio that is behind the current playback clock.
            uint64_t cur_time_ms = 0;
            // IMPORTANT:
            // Do NOT use "wall time inferred audio clock" here. Using system tick can cause false "late"
            // decisions when the decode thread is temporarily stalled (e.g., SD I/O or other tasks),
            // resulting in dropping multiple packets and visible PTS jumps (20ms -> 60ms).
            //
            // Use the unified playback clock (current_time_ms) which is driven by delivered A/V PTS.
            cur_time_ms = video_player_get_current_time_ms(controller);

            if (out_buffer.pts > 0 && cur_time_ms > 0 && out_buffer.pts < cur_time_ms)
            {
                if (controller->config.audio.buffer_free_cb != NULL)
                {
                    controller->config.audio.buffer_free_cb(controller->config.user_data, &out_buffer);
                }
                continue;
            }

            uint8_t tail = (uint8_t)((pending_head + pending_count) % BK_VP_AUDIO_PREFETCH_PACKETS);
            pending[tail].valid = true;
            pending[tail].session_id = iter_session_id;
            pending[tail].buffer = out_buffer;
            pending_count++;
        }

        if (!controller->audio_decode_thread_running || controller->audio_decode_thread_exit)
        {
            continue;
        }

        if (pending_count == 0)
        {
            if (!controller->audio_parse_thread_running)
            {
                controller->audio_decode_thread_running = false;
                rtos_delay_milliseconds(10);
            }
            else
            {
                rtos_delay_milliseconds(1);
            }
            continue;
        }

        // Deliver the oldest pending packet.
        bk_vp_audio_pending_t *p = &pending[pending_head];
        if (!p->valid)
        {
            pending_head = (uint8_t)((pending_head + 1) % BK_VP_AUDIO_PREFETCH_PACKETS);
            pending_count--;
            continue;
        }

        // Drop pending packet if session changed.
        if (p->session_id != controller->play_session_id)
        {
            if (controller->config.audio.buffer_free_cb != NULL)
            {
                controller->config.audio.buffer_free_cb(controller->config.user_data, &p->buffer);
            }
            p->valid = false;
            os_memset(&p->buffer, 0, sizeof(p->buffer));
            pending_head = (uint8_t)((pending_head + 1) % BK_VP_AUDIO_PREFETCH_PACKETS);
            pending_count--;
            continue;
        }

        // Initialize audio clock base on first delivered PTS.
        uint64_t out_pts_ms = p->buffer.pts;

        // Pacing base for audio output (independent of current clock source).
        // This prevents flooding upper audio sink (voice_write) when clock source is VIDEO.
        if (!audio_pace_base_valid)
        {
            audio_pace_base_pts_ms = out_pts_ms;
            audio_pace_base_tick_ms = rtos_get_time();
            audio_pace_base_valid = true;
        }

        // Throttle delivery if we are too far ahead of the expected audio wall clock.
        // NOTE: This pacing must apply regardless of controller->clock_source, otherwise audio packets
        // can burst and overflow voice_write internal buffer, resulting in silence fill.
        if (audio_pace_base_valid && audio_pace_base_tick_ms > 0)
        {
            uint32_t now_tick_ms = rtos_get_time();
            uint64_t play_time_ms = video_player_calc_audio_clock_ms(audio_pace_base_pts_ms,
                                                                     audio_pace_base_tick_ms,
                                                                     now_tick_ms,
                                                                     audio_pace_base_valid);
            if (play_time_ms > 0 && out_pts_ms > play_time_ms + (uint64_t)BK_VP_AUDIO_PREFETCH_LEAD_MS)
            {
                uint64_t ahead = out_pts_ms - (play_time_ms + (uint64_t)BK_VP_AUDIO_PREFETCH_LEAD_MS);
                uint32_t wait_ms = (ahead > 10ULL) ? 10U : (uint32_t)ahead;
                if (wait_ms > 0)
                {
                    rtos_delay_milliseconds(wait_ms);
                    continue;
                }
            }
        }

        /*
         * Update the unified playback clock from the ACTUALLY delivered audio packet.
         *
         * IMPORTANT:
         * - av_sync_offset_ms is only applied when VIDEO syncs to AUDIO clock.
         * - Video decode thread detects "audio clock stalled" based on last_time_update_tick_ms.
         * - Some streams start at pts=0 and will deliver several packets with pts=0 before any
         *   non-zero PTS appears. If we only refresh last_time_update_tick_ms when PTS increases,
         *   video thread may falsely switch to VIDEO clock during track switching/startup, making
         *   avsync tuning appear ineffective.
         *
         * Policy:
         * - Keep current_time_ms monotonic (only advance when out_pts_ms increases).
         * - Always refresh last_time_update_tick_ms when an audio packet is successfully delivered
         *   while using AUDIO clock.
         */
        if (controller->clock_source == VIDEO_PLAYER_CLOCK_AUDIO &&
            controller->play_session_id == last_seen_session_id &&
            controller->audio_decode_thread_running &&
            controller->time_mutex != NULL)
        {
            rtos_lock_mutex(&controller->time_mutex);
            if (out_pts_ms > controller->current_time_ms)
            {
                controller->current_time_ms = out_pts_ms;
            }
            controller->last_time_update_tick_ms = rtos_get_time();
            rtos_unlock_mutex(&controller->time_mutex);
        }

        bool handed_to_user = false;
        if (controller->config.audio.decode_complete_cb != NULL)
        {
            delivered_packet_index++;
            video_player_audio_packet_meta_t meta;
            os_memset(&meta, 0, sizeof(meta));
            meta.audio = controller->current_media_info.audio;
            meta.packet_index = delivered_packet_index;
            meta.pts_ms = p->buffer.pts;
            controller->config.audio.decode_complete_cb(controller->config.user_data, &meta, &p->buffer);
            handed_to_user = true;
        }
        if (!handed_to_user && controller->config.audio.buffer_free_cb != NULL)
        {
            controller->config.audio.buffer_free_cb(controller->config.user_data, &p->buffer);
        }

        p->valid = false;
        os_memset(&p->buffer, 0, sizeof(p->buffer));
        pending_head = (uint8_t)((pending_head + 1) % BK_VP_AUDIO_PREFETCH_PACKETS);
        pending_count--;

        continue;
    }

    LOGI("%s: Audio decode thread exiting\n", __func__);
    rtos_set_semaphore(&controller->audio_decode_sem);
    rtos_delete_thread(NULL);
}

avdk_err_t bk_video_player_audio_decode_init(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        LOGE("%s: Controller is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    controller->audio_decode_thread_running = false;
    controller->audio_decode_thread_exit = false;

    if (rtos_init_semaphore(&controller->audio_decode_sem, 1) != BK_OK)
    {
        LOGE("%s: Failed to init audio decode semaphore\n", __func__);
        return AVDK_ERR_IO;
    }

    beken_thread_t audio_thread = NULL;
    avdk_err_t ret = rtos_create_thread(&audio_thread, BK_VP_AUDIO_DECODE_THREAD_PRIORITY, "audio_decode",
                                        (beken_thread_function_t)bk_video_player_audio_decode_thread,
                                        BK_VP_AUDIO_DECODE_THREAD_STACK_SIZE,
                                        (beken_thread_arg_t)controller);
    if (ret != BK_OK)
    {
        LOGE("%s: Failed to create audio decode thread, ret=%d\n", __func__, ret);
        rtos_deinit_semaphore(&controller->audio_decode_sem);
        return AVDK_ERR_IO;
    }
    // Wait until decode thread is started.
    rtos_get_semaphore(&controller->audio_decode_sem, BEKEN_WAIT_FOREVER);
    controller->audio_decode_thread = audio_thread;

    LOGI("%s: Audio decode resources initialized successfully\n", __func__);
    return AVDK_ERR_OK;
}

void bk_video_player_audio_decode_deinit(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    if (controller->audio_decode_thread != NULL)
    {
        controller->audio_decode_thread_exit = true;
        controller->audio_decode_thread_running = false;
        if (controller->audio_decode_sem != NULL)
        {
            rtos_get_semaphore(&controller->audio_decode_sem, BEKEN_WAIT_FOREVER);
        }
        controller->audio_decode_thread = NULL;
    }

    if (controller->audio_decode_sem != NULL)
    {
        rtos_deinit_semaphore(&controller->audio_decode_sem);
        controller->audio_decode_sem = NULL;
    }

    LOGI("%s: Audio decode resources deinitialized\n", __func__);
}

avdk_err_t bk_video_player_audio_decoder_list_add(private_video_player_ctlr_t *controller,
                                                  const video_player_audio_decoder_ops_t *decoder_ops)
{
    if (controller == NULL || decoder_ops == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&controller->decoder_list_mutex);

    video_player_audio_decoder_node_t *new_node = os_malloc(sizeof(video_player_audio_decoder_node_t));
    if (new_node == NULL)
    {
        rtos_unlock_mutex(&controller->decoder_list_mutex);
        return AVDK_ERR_NOMEM;
    }

    new_node->ops = decoder_ops;
    new_node->next = NULL;

    // Insert at tail to maintain registration order (first registered decoder should be tried first).
    if (controller->audio_decoder_list == NULL)
    {
        controller->audio_decoder_list = new_node;
    }
    else
    {
        video_player_audio_decoder_node_t *cur = controller->audio_decoder_list;
        while (cur->next != NULL)
        {
            cur = cur->next;
        }
        cur->next = new_node;
    }

    rtos_unlock_mutex(&controller->decoder_list_mutex);
    return AVDK_ERR_OK;
}

void bk_video_player_audio_decoder_list_clear(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    rtos_lock_mutex(&controller->decoder_list_mutex);

    video_player_audio_decoder_node_t *node = controller->audio_decoder_list;
    while (node != NULL)
    {
        video_player_audio_decoder_node_t *next = node->next;
        os_free(node);
        node = next;
    }
    controller->audio_decoder_list = NULL;

    rtos_unlock_mutex(&controller->decoder_list_mutex);
}

