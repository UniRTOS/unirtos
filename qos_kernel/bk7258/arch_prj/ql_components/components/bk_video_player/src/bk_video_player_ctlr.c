#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "bk_video_player_ctlr.h"
#include "bk_video_player_buffer_pool.h"
#include "bk_video_player_pipeline.h"
#include "bk_video_player_container_parse.h"
#include "bk_video_player_audio_decode.h"
#include "bk_video_player_video_decode.h"

// Fallback defaults for build environments that don't yet have the new Kconfig symbols in sdkconfig.h.
// Defaults match the previous hard-coded behavior.
#ifndef CONFIG_BK_VIDEO_PLAYER_EVENT_THREAD_PRIORITY
#define CONFIG_BK_VIDEO_PLAYER_EVENT_THREAD_PRIORITY (BEKEN_DEFAULT_WORKER_PRIORITY)
#endif
#ifndef CONFIG_BK_VIDEO_PLAYER_EVENT_THREAD_STACK_SIZE
#define CONFIG_BK_VIDEO_PLAYER_EVENT_THREAD_STACK_SIZE (4 * 1024)
#endif

#include "bk_video_player_thread_config.h"

#define TAG "video_player_ctlr"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// A/V sync offset is an external tuning knob, keep it bounded and explicit.
// Returning error is preferred over silently clamping to avoid hiding misconfiguration.
#define BK_VP_AV_SYNC_OFFSET_MS_MAX  (5000)

typedef enum
{
    VIDEO_PLAYER_EVT_NONE = 0,
    VIDEO_PLAYER_EVT_FINISHED = 1,
    VIDEO_PLAYER_EVT_EXIT = 2,
} video_player_evt_type_t;

typedef struct
{
    video_player_evt_type_t type;
    // Heap allocated by producer (optional), must be freed by consumer thread.
    // NULL means "no file path".
    char *file_path;
} video_player_evt_msg_t;

static void video_player_event_thread_entry(beken_thread_arg_t arg)
{
    private_video_player_ctlr_t *controller = (private_video_player_ctlr_t *)arg;
    if (controller == NULL)
    {
        rtos_delete_thread(NULL);
        return;
    }

    while (controller->event_thread_running)
    {
        video_player_evt_msg_t msg;
        os_memset(&msg, 0, sizeof(msg));

        bk_err_t ret = rtos_pop_from_queue(&controller->event_queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret != kNoErr)
        {
            continue;
        }

        char *file_path = msg.file_path;
        if (!controller->event_thread_running || msg.type == VIDEO_PLAYER_EVT_EXIT)
        {
            if (file_path != NULL)
            {
                os_free(file_path);
            }
            break;
        }

        if (msg.type == VIDEO_PLAYER_EVT_FINISHED && controller->config.playback_finished_cb != NULL)
        {
            controller->config.playback_finished_cb(controller->config.playback_finished_user_data,
                                                    file_path);
        }

        if (file_path != NULL)
        {
            os_free(file_path);
        }
    }

    rtos_set_semaphore(&controller->event_sem);
    rtos_delete_thread(NULL);
}

static void video_player_post_finished_event(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    if (controller->config.playback_finished_cb == NULL || controller->event_thread == NULL)
    {
        return;
    }

    video_player_evt_msg_t msg;
    os_memset(&msg, 0, sizeof(msg));
    msg.type = VIDEO_PLAYER_EVT_FINISHED;

    if (controller->current_file_path != NULL && controller->current_file_path[0] != '\0')
    {
        msg.file_path = os_strdup(controller->current_file_path);
        if (msg.file_path == NULL)
        {
            LOGW("%s: Failed to allocate event file_path\n", __func__);
        }
    }

    bk_err_t ret = rtos_push_to_queue(&controller->event_queue, &msg, BEKEN_NO_WAIT);
    if (ret != kNoErr)
    {
        if (msg.file_path != NULL)
        {
            os_free(msg.file_path);
            msg.file_path = NULL;
        }
        LOGW("%s: event queue full, drop FINISHED event\n", __func__);
    }
}

static void video_player_event_notify_init(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    // We use vp_evt thread for:
    // - playback_finished_cb delivery
    // - internal async tasks (e.g., async pixel conversion completion)
    if (controller->config.playback_finished_cb == NULL &&
        controller->config.video.decode_complete_cb == NULL)
    {
        return;
    }

    if (controller->event_thread != NULL)
    {
        return;
    }

    controller->event_thread_running = true;

    // vp_evt queue is used to deliver per-frame decode_complete_cb as well as FINISHED notifications.
    // Keep it large enough to avoid dropping TASK events under normal playback.
    uint32_t qlen = controller->config.video.decode_to_output_buffer_count + 4;
    if (qlen < 8)
    {
        qlen = 8;
    }
    if (qlen > 32)
    {
        qlen = 32;
    }
    bk_err_t ret = rtos_init_queue(&controller->event_queue,
                                  "vp_evt",
                                  sizeof(video_player_evt_msg_t),
                                  qlen);
    if (ret != kNoErr)
    {
        LOGE("%s: init event queue failed, ret=%d\n", __func__, ret);
        controller->event_thread_running = false;
        return;
    }

    ret = rtos_init_semaphore(&controller->event_sem, 1);
    if (ret != kNoErr)
    {
        LOGE("%s: init event sem failed, ret=%d\n", __func__, ret);
        rtos_deinit_queue(&controller->event_queue);
        controller->event_thread_running = false;
        return;
    }

    controller->event_thread = (beken_thread_t *)os_malloc(sizeof(beken_thread_t));
    if (controller->event_thread == NULL)
    {
        LOGE("%s: malloc event thread handle failed\n", __func__);
        rtos_deinit_semaphore(&controller->event_sem);
        rtos_deinit_queue(&controller->event_queue);
        controller->event_thread_running = false;
        return;
    }
    os_memset(controller->event_thread, 0, sizeof(beken_thread_t));

    ret = rtos_create_thread(controller->event_thread,
                             BK_VP_EVENT_THREAD_PRIORITY,
                             "vp_evt",
                             (beken_thread_function_t)video_player_event_thread_entry,
                             BK_VP_EVENT_THREAD_STACK_SIZE,
                             (beken_thread_arg_t)controller);
    if (ret != kNoErr)
    {
        LOGE("%s: create event thread failed, ret=%d\n", __func__, ret);
        os_free(controller->event_thread);
        controller->event_thread = NULL;
        rtos_deinit_semaphore(&controller->event_sem);
        rtos_deinit_queue(&controller->event_queue);
        controller->event_thread_running = false;
        return;
    }
}

static void video_player_event_notify_deinit(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    if (controller->event_thread == NULL)
    {
        return;
    }

    controller->event_thread_running = false;

    video_player_evt_msg_t msg;
    os_memset(&msg, 0, sizeof(msg));
    msg.type = VIDEO_PLAYER_EVT_EXIT;
    // Ensure EXIT message is delivered, otherwise vp_evt thread may block forever.
    (void)rtos_push_to_queue(&controller->event_queue, &msg, BEKEN_WAIT_FOREVER);

    (void)rtos_get_semaphore(&controller->event_sem, BEKEN_NEVER_TIMEOUT);

    // Drain remaining messages after thread exit to avoid leaking heap pointers (file_path).
    while (rtos_pop_from_queue(&controller->event_queue, &msg, 0) == kNoErr)
    {
        if (msg.file_path != NULL)
        {
            os_free(msg.file_path);
            msg.file_path = NULL;
        }
    }

    rtos_deinit_semaphore(&controller->event_sem);
    rtos_deinit_queue(&controller->event_queue);

    os_free(controller->event_thread);
    controller->event_thread = NULL;
}

static void deinit_active_decoders(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    if (controller->active_mutex != NULL)
    {
        rtos_lock_mutex(&controller->active_mutex);
    }

    video_player_container_parser_ops_t *parser = controller->active_container_parser;
    video_player_audio_decoder_ops_t *audio = controller->active_audio_decoder;
    video_player_video_decoder_ops_t *video = controller->active_video_decoder;
    controller->active_container_parser = NULL;
    controller->active_audio_decoder = NULL;
    controller->active_video_decoder = NULL;
    controller->audio_track_enabled = false;
    controller->video_track_enabled = false;
    os_memset(&controller->current_media_info, 0, sizeof(controller->current_media_info));

    if (controller->active_mutex != NULL)
    {
        rtos_unlock_mutex(&controller->active_mutex);
    }

    // Deinit/close first, then destroy (destroy must only free memory).
    if (parser != NULL)
    {
        if (parser->close != NULL)
        {
            (void)parser->close(parser);
        }
        if (parser->destroy != NULL)
        {
            parser->destroy(parser);
        }
    }

    if (audio != NULL)
    {
        if (audio->deinit != NULL)
        {
            (void)audio->deinit(audio);
        }
        if (audio->destroy != NULL)
        {
            audio->destroy(audio);
        }
    }

    if (video != NULL)
    {
        if (video->deinit != NULL)
        {
            (void)video->deinit(video);
        }
        if (video->destroy != NULL)
        {
            video->destroy(video);
        }
    }
}

// Clear (close+destroy) probe cache.
// Caller must hold decoder_list_mutex to serialize with select_decoders() and get_media_info() probe path.
static void video_player_prefetch_parser_clear_locked(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    video_player_container_parser_ops_t *parser = controller->prefetch_container_parser;
    char *path = controller->prefetch_file_path;
    controller->prefetch_container_parser = NULL;
    controller->prefetch_file_path = NULL;
    os_memset(&controller->prefetch_media_info, 0, sizeof(controller->prefetch_media_info));

    if (parser != NULL)
    {
        if (parser->close != NULL)
        {
            (void)parser->close(parser);
        }
        if (parser->destroy != NULL)
        {
            parser->destroy(parser);
        }
    }

    if (path != NULL)
    {
        os_free(path);
    }
}

static bool audio_decoder_supports_format(const video_player_audio_decoder_ops_t *decoder_template,
                                         video_player_audio_format_t format);
static bool video_decoder_supports_format(video_player_video_decoder_ops_t *decoder_template,
                                          video_player_video_format_t format);

// Try selecting decoders using an already-opened container parser.
// Caller must hold decoder_list_mutex.
// On success (*selected=true), controller->active_container_parser/decoders are set and kept.
// On failure, controller state is restored (active resources deinitialized) and *selected=false.
static avdk_err_t select_decoders_try_opened_parser_locked(private_video_player_ctlr_t *controller,
                                                          video_player_container_parser_ops_t *ops,
                                                          const char *file_path,
                                                          const video_player_media_info_t *prefetched_info,
                                                          bool *selected)
{
    if (selected != NULL)
    {
        *selected = false;
    }
    if (controller == NULL || ops == NULL || file_path == NULL || selected == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    controller->active_container_parser = ops;
    os_memset(&controller->current_media_info, 0, sizeof(controller->current_media_info));

    // Cache file size best-effort.
    uint64_t stat_file_size_bytes = 0;
    struct stat st;
    if (stat(file_path, &st) == 0 && st.st_size > 0)
    {
        stat_file_size_bytes = (uint64_t)st.st_size;
        controller->current_media_info.file_size_bytes = stat_file_size_bytes;
    }

    video_player_media_info_t media_info = {0};
    if (prefetched_info != NULL)
    {
        media_info = *prefetched_info;
    }
    else
    {
        if (ops->get_media_info == NULL || ops->get_media_info(ops, &media_info) != AVDK_ERR_OK)
        {
            deinit_active_decoders(controller);
            return AVDK_ERR_UNSUPPORTED;
        }
    }

    // Parser may not provide file_size_bytes; keep stat() result as fallback.
    if (media_info.file_size_bytes == 0 && stat_file_size_bytes > 0)
    {
        media_info.file_size_bytes = stat_file_size_bytes;
    }
    controller->current_media_info = media_info;

    controller->audio_track_enabled = false;
    controller->video_track_enabled = false;

    const bool audio_present = !(media_info.audio.channels == 0 || media_info.audio.sample_rate == 0);
    const bool video_present = !(media_info.video.width == 0 || media_info.video.height == 0);

    bool audio_decoder_found = false;
    bool video_decoder_found = false;
    avdk_err_t ret = AVDK_ERR_OK;

    // PCM does not require an audio decoder: deliver parser output directly to upper layer (zero-copy path).
    // Ownership of PCM buffer is transferred to upper layer; player will not free it.
    if (audio_present && media_info.audio.format == VIDEO_PLAYER_AUDIO_FORMAT_PCM)
    {
        controller->active_audio_decoder = NULL;
        audio_decoder_found = true;
        controller->audio_track_enabled = true;
    }
    else if (audio_present)
    {
        video_player_audio_decoder_ops_t *audio_decoder_ops = NULL;
        video_player_audio_decoder_node_t *audio_node = controller->audio_decoder_list;
        while (audio_node != NULL)
        {
            if (audio_node->ops == NULL || audio_node->ops->create == NULL)
            {
                audio_node = audio_node->next;
                continue;
            }

            if (!audio_decoder_supports_format(audio_node->ops, media_info.audio.format))
            {
                audio_node = audio_node->next;
                continue;
            }

            audio_decoder_ops = audio_node->ops->create();
            if (audio_decoder_ops == NULL)
            {
                audio_node = audio_node->next;
                continue;
            }
            ret = audio_decoder_ops->init(audio_decoder_ops, &media_info.audio);
            if (ret == AVDK_ERR_OK)
            {
                controller->active_audio_decoder = audio_decoder_ops;
                audio_decoder_found = true;
                controller->audio_track_enabled = true;
                break;
            }
            if (audio_decoder_ops->destroy != NULL)
            {
                audio_decoder_ops->destroy(audio_decoder_ops);
            }
            audio_decoder_ops = NULL;
            audio_node = audio_node->next;
        }

        if (!audio_decoder_found)
        {
            // Keep playing without audio.
            controller->active_audio_decoder = NULL;
            controller->audio_track_enabled = false;
            LOGW("%s: No suitable audio decoder, disable audio for file: %s\n", __func__, file_path);
        }
    }

    if (video_present)
    {
        video_player_video_decoder_ops_t *video_decoder_ops = NULL;
        video_player_video_decoder_node_t *video_node = controller->video_decoder_list;
        while (video_node != NULL)
        {
            if (video_node->ops == NULL || video_node->ops->create == NULL)
            {
                video_node = video_node->next;
                continue;
            }

            if (!video_decoder_supports_format(video_node->ops, media_info.video.format))
            {
                video_node = video_node->next;
                continue;
            }

            video_decoder_ops = video_node->ops->create();
            if (video_decoder_ops == NULL)
            {
                video_node = video_node->next;
                continue;
            }
            ret = video_decoder_ops->init(video_decoder_ops, &media_info.video);
            if (ret == AVDK_ERR_OK)
            {
                controller->active_video_decoder = video_decoder_ops;
                video_decoder_found = true;
                controller->video_track_enabled = true;
                break;
            }
            if (video_decoder_ops->destroy != NULL)
            {
                video_decoder_ops->destroy(video_decoder_ops);
            }
            video_decoder_ops = NULL;
            video_node = video_node->next;
        }

        if (!video_decoder_found)
        {
            // Keep playing without video.
            controller->active_video_decoder = NULL;
            controller->video_track_enabled = false;
            LOGW("%s: No suitable video decoder, disable video for file: %s\n", __func__, file_path);
        }
    }

    // If a stream does not exist, its track stays disabled.
    // If both tracks are disabled (no streams, or both decoders missing), treat as not playable.
    if ((!audio_present || !controller->audio_track_enabled) &&
        (!video_present || !controller->video_track_enabled))
    {
        // Parser is opened but no usable stream; let caller continue trying other parsers.
        deinit_active_decoders(controller);
        return AVDK_ERR_NODEV;
    }

    *selected = true;
    return AVDK_ERR_OK;
}

static bool audio_decoder_supports_format(const video_player_audio_decoder_ops_t *decoder_template,
                                         video_player_audio_format_t format)
{
    if (decoder_template == NULL)
    {
        return false;
    }

    if (decoder_template->get_supported_formats == NULL)
    {
        // Legacy decoders: controller will fallback to create()+init() probing.
        return true;
    }

    const video_player_audio_format_t *formats = NULL;
    uint32_t count = 0;
    avdk_err_t ret = decoder_template->get_supported_formats(decoder_template, &formats, &count);
    if (ret != AVDK_ERR_OK || formats == NULL || count == 0)
    {
        // Best-effort: do not block selection if decoder reports unexpected error.
        return true;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        if (formats[i] == format)
        {
            return true;
        }
    }
    return false;
}

static bool video_decoder_supports_format(video_player_video_decoder_ops_t *decoder_template,
                                         video_player_video_format_t format)
{
    if (decoder_template == NULL)
    {
        return false;
    }

    if (decoder_template->get_supported_formats == NULL)
    {
        // Legacy decoders: controller will fallback to create()+init() probing.
        return true;
    }

    const video_player_video_format_t *formats = NULL;
    uint32_t count = 0;
    avdk_err_t ret = decoder_template->get_supported_formats(decoder_template, &formats, &count);
    if (ret != AVDK_ERR_OK || formats == NULL || count == 0)
    {
        // Best-effort: do not block selection if decoder reports unexpected error.
        return true;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        if (formats[i] == format)
        {
            return true;
        }
    }
    return false;
}

static bool file_path_extension_match_ci(const char *file_path, const char *ext)
{
    if (file_path == NULL || ext == NULL || ext[0] != '.')
    {
        return false;
    }

    const char *dot = strrchr(file_path, '.');
    if (dot == NULL || dot[1] == '\0')
    {
        return false;
    }

    return (os_strcasecmp(dot, ext) == 0);
}

static bool container_parser_supports_file_path(const video_player_container_parser_ops_t *parser_template,
                                                const char *file_path)
{
    if (parser_template == NULL || file_path == NULL)
    {
        return false;
    }

    if (parser_template->get_supported_file_extensions != NULL)
    {
        const char * const *exts = NULL;
        uint32_t ext_count = 0;
        avdk_err_t ret = parser_template->get_supported_file_extensions(parser_template, &exts, &ext_count);
        if (ret == AVDK_ERR_OK && exts != NULL && ext_count > 0)
        {
            for (uint32_t i = 0; i < ext_count; i++)
            {
                if (exts[i] != NULL && file_path_extension_match_ci(file_path, exts[i]))
                {
                    return true;
                }
            }
            return false;
        }
        // If parser returns unexpected error, treat as "unknown" and try it in fallback pass.
    }

    return false;
}

static avdk_err_t select_decoders(private_video_player_ctlr_t *controller, const char *file_path)
{
    if (controller == NULL || file_path == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&controller->decoder_list_mutex);

    avdk_err_t last_open_err = AVDK_ERR_NODEV;
    // Default to disabled; will be set when a usable track is selected.
    controller->audio_track_enabled = false;
    controller->video_track_enabled = false;

    // Fast path: reuse an already-opened parser from get_media_info() probe cache.
    // This prevents parsing the same container twice for the same file (e.g. MP4 moov/stbl).
    if (controller->prefetch_container_parser != NULL && controller->prefetch_file_path != NULL)
    {
        if (os_strcmp(controller->prefetch_file_path, file_path) == 0)
        {
            video_player_container_parser_ops_t *ops = controller->prefetch_container_parser;
            video_player_media_info_t info = controller->prefetch_media_info;

            controller->prefetch_container_parser = NULL;
            os_free(controller->prefetch_file_path);
            controller->prefetch_file_path = NULL;
            os_memset(&controller->prefetch_media_info, 0, sizeof(controller->prefetch_media_info));

            bool selected = false;
            avdk_err_t ret = select_decoders_try_opened_parser_locked(controller, ops, file_path, &info, &selected);
            if (selected)
            {
                rtos_unlock_mutex(&controller->decoder_list_mutex);
                return ret;
            }

            // Not usable: ensure parser is released then continue normal selection.
            deinit_active_decoders(controller);
        }
        else
        {
            // Probe cache is for a different file; release it to avoid leaking file descriptors/memory.
            video_player_prefetch_parser_clear_locked(controller);
        }
    }

    // Two-pass strategy:
    // - Pass 0: try parsers that match file suffix (avoid noisy open failures).
    // - Pass 1: fallback to other parsers if all matched parsers failed (format mismatch).
    for (uint8_t pass = 0; pass < 2; pass++)
    {
        video_player_container_parser_node_t *parser_node = controller->container_parser_list;
        while (parser_node != NULL)
        {
            if (parser_node->ops == NULL || parser_node->ops->create == NULL)
            {
                parser_node = parser_node->next;
                continue;
            }

            bool matched = false;
            // Use template ops to match file path before create() to avoid unnecessary allocations.
            matched = container_parser_supports_file_path(parser_node->ops, file_path);

            video_player_video_decoder_ops_t *video_decoder_ops = NULL;
            video_player_audio_decoder_ops_t *audio_decoder_ops = NULL;

            if (pass == 0)
            {
                // Only try matched parsers in pass 0.
                if (!matched)
                {
                    parser_node = parser_node->next;
                    continue;
                }
            }
            else
            {
                // In fallback pass, skip parsers already tried in pass 0.
                if (matched)
                {
                    parser_node = parser_node->next;
                    continue;
                }
            }

            video_player_container_parser_ops_t *ops = parser_node->ops->create(controller->config.user_data,
                                                                                controller->config.video.packet_buffer_alloc_cb,
                                                                                controller->config.video.packet_buffer_free_cb);
            if (ops == NULL)
            {
                LOGE("%s: Failed to create container parser\n", __func__);
                parser_node = parser_node->next;
                continue;
            }

            avdk_err_t ret = ops->open(ops, file_path);
            if (ret != AVDK_ERR_OK)
            {
                last_open_err = ret;
                if (ops->destroy != NULL)
                {
                    ops->destroy(ops);
                }
                ops = NULL;
                parser_node = parser_node->next;
                continue;
            }

            controller->active_container_parser = ops;
            os_memset(&controller->current_media_info, 0, sizeof(controller->current_media_info));

            // Cache file size best-effort.
            uint64_t stat_file_size_bytes = 0;
            struct stat st;
            if (stat(file_path, &st) == 0 && st.st_size > 0)
            {
                stat_file_size_bytes = (uint64_t)st.st_size;
                controller->current_media_info.file_size_bytes = stat_file_size_bytes;
            }

            video_player_media_info_t media_info = {0};
            bool audio_decoder_found = false;
            bool video_decoder_found = false;

            if (ops->get_media_info != NULL &&
                ops->get_media_info(ops, &media_info) == AVDK_ERR_OK)
            {
                // Parser may not provide file_size_bytes; keep stat() result as fallback.
                if (media_info.file_size_bytes == 0 && stat_file_size_bytes > 0)
                {
                    media_info.file_size_bytes = stat_file_size_bytes;
                }
                controller->current_media_info = media_info;

                controller->audio_track_enabled = false;
                controller->video_track_enabled = false;
                const bool audio_present = !(media_info.audio.channels == 0 || media_info.audio.sample_rate == 0);

                // PCM does not require an audio decoder: deliver parser output directly to upper layer.
                // Ownership of PCM buffer is transferred to upper layer; player will not free it.
                if (audio_present && media_info.audio.format == VIDEO_PLAYER_AUDIO_FORMAT_PCM)
                {
                    controller->active_audio_decoder = NULL;
                    audio_decoder_found = true;
                    controller->audio_track_enabled = true;
                }
                else if (audio_present)
                {
                    video_player_audio_decoder_node_t *audio_node = controller->audio_decoder_list;
                    while (audio_node != NULL)
                    {
                        if (audio_node->ops == NULL || audio_node->ops->create == NULL)
                        {
                            audio_node = audio_node->next;
                            continue;
                        }

                        if (!audio_decoder_supports_format(audio_node->ops, media_info.audio.format))
                        {
                            audio_node = audio_node->next;
                            continue;
                        }

                        audio_decoder_ops = audio_node->ops->create();
                        if (audio_decoder_ops == NULL)
                        {
                            LOGE("%s: Failed to create audio decoder\n", __func__);
                            audio_node = audio_node->next;
                            break;
                        }
                        ret = audio_decoder_ops->init(audio_decoder_ops, &media_info.audio);
                        if (ret == AVDK_ERR_OK)
                        {
                            controller->active_audio_decoder = audio_decoder_ops;
                            audio_decoder_found = true;
                            controller->audio_track_enabled = true;
                            break;
                        }
                        if (audio_decoder_ops->destroy != NULL)
                        {
                            audio_decoder_ops->destroy(audio_decoder_ops);
                        }
                        audio_decoder_ops = NULL;
                        audio_node = audio_node->next;
                    }

                    if (!audio_decoder_found)
                    {
                        // Keep playing without audio.
                        controller->active_audio_decoder = NULL;
                        controller->audio_track_enabled = false;
                        LOGW("%s: No suitable audio decoder, disable audio for file: %s\n", __func__, file_path);
                    }
                }
            }

            // Video track selection.
            // Note: Use controller->current_media_info here because the media_info local is only valid
            // when get_media_info() succeeds above.
            if (controller->current_media_info.video.width != 0 && controller->current_media_info.video.height != 0)
            {
                // Select video decoder: try decoders in registration order (hardware decoder should be registered first)
                // Hardware decoder has strict requirements (width%16==0, height%8==0, YUV422 format)
                // If hardware decoder init fails (returns AVDK_ERR_UNSUPPORTED), try next decoder (software decoder)
                // Software decoder has no restrictions and can handle any JPEG frame
                video_player_video_decoder_node_t *video_node = controller->video_decoder_list;
                while (video_node != NULL)
                {
                    if (video_node->ops == NULL || video_node->ops->create == NULL)
                    {
                        video_node = video_node->next;
                        continue;
                    }

                    if (!video_decoder_supports_format(video_node->ops, media_info.video.format))
                    {
                        video_node = video_node->next;
                        continue;
                    }

                    video_decoder_ops = video_node->ops->create();
                    if (video_decoder_ops == NULL)
                    {
                        LOGE("%s: Failed to create video decoder\n", __func__);
                        video_node = video_node->next;
                        break;
                    }
                    ret = video_decoder_ops->init(video_decoder_ops, &media_info.video);
                    if (ret == AVDK_ERR_OK)
                    {
                        controller->active_video_decoder = video_decoder_ops;
                        video_decoder_found = true;
                        controller->video_track_enabled = true;
                        break;
                    }
                    if (video_decoder_ops->destroy != NULL)
                    {
                        video_decoder_ops->destroy(video_decoder_ops);
                    }
                    video_decoder_ops = NULL;
                    // If init returns AVDK_ERR_UNSUPPORTED, try next decoder in list
                    video_node = video_node->next;
                }

                if (!video_decoder_found)
                {
                    // Keep playing without video.
                    controller->active_video_decoder = NULL;
                    controller->video_track_enabled = false;
                    LOGW("%s: No suitable video decoder, disable video for file: %s\n", __func__, file_path);
                }
            }

            // If both tracks are disabled, this parser is not usable for playback.
            if (!controller->audio_track_enabled && !controller->video_track_enabled)
            {
                LOGE("%s: No suitable audio or video decoder found for file: %s\n", __func__, file_path);
                deinit_active_decoders(controller);
                parser_node = parser_node->next;
                continue;
            }

            rtos_unlock_mutex(&controller->decoder_list_mutex);
            return AVDK_ERR_OK;
        }
    }

    rtos_unlock_mutex(&controller->decoder_list_mutex);
    LOGE("%s: No suitable container parser found for: %s\n", __func__, file_path);
    return last_open_err;
}

avdk_err_t bk_video_player_handle_play_mode(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    // Avoid duplicate FINISHED handling (both audio/video threads may report EOF).
    if (controller->module_status.status == VIDEO_PLAYER_STATUS_FINISHED)
    {
        return AVDK_ERR_OK;
    }

    // Controller stops on EOF. Upper (app) layer may choose to restart playback via callback.
    LOGI("%s: Playback finished, stopping playback\n", __func__);
    controller->video_parse_thread_running = false;
    controller->audio_parse_thread_running = false;
    controller->video_decode_thread_running = false;
    controller->audio_decode_thread_running = false;
    controller->module_status.status = VIDEO_PLAYER_STATUS_FINISHED;

    // Post FINISHED event once per play session.
    if (controller->play_session_id != 0 && controller->eof_notified_session_id != controller->play_session_id)
    {
        controller->eof_notified_session_id = controller->play_session_id;
        video_player_post_finished_event(controller);
    }

    return AVDK_ERR_OK;
}

static void drain_filled_packet_pool(video_player_buffer_pool_t *pool,
                                     video_player_audio_buffer_free_cb_t free_cb,
                                     void *user_data)
{
    if (pool == NULL)
    {
        return;
    }

    // Drain all filled nodes to avoid carrying stale PTS/data across play sessions.
    // This must be non-blocking to prevent deadlocks when no filled node exists.
    while (1)
    {
        video_player_buffer_node_t *node = buffer_pool_try_get_filled(pool);
        if (node == NULL)
        {
            break;
        }

        if (free_cb != NULL && node->buffer.data != NULL)
        {
            free_cb(user_data, &node->buffer);
        }

        node->buffer.data = NULL;
        node->buffer.length = 0;
        node->buffer.pts = 0;
        node->buffer.frame_buffer = NULL;
        node->buffer.user_data = NULL;
        buffer_pool_put_empty(pool, node);
    }
}

static void drain_filled_packet_pool_video(video_player_buffer_pool_t *pool,
                                           video_player_video_packet_buffer_free_cb_t free_cb,
                                           void *user_data)
{
    if (pool == NULL)
    {
        return;
    }

    while (1)
    {
        video_player_buffer_node_t *node = buffer_pool_try_get_filled(pool);
        if (node == NULL)
        {
            break;
        }

        if (free_cb != NULL && node->buffer.data != NULL)
        {
            free_cb(user_data, &node->buffer);
        }

        node->buffer.data = NULL;
        node->buffer.length = 0;
        node->buffer.pts = 0;
        node->buffer.frame_buffer = NULL;
        node->buffer.user_data = NULL;
        buffer_pool_put_empty(pool, node);
    }
}

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

static void video_player_set_current_time_ms(private_video_player_ctlr_t *controller, uint64_t time_ms)
{
    if (controller == NULL || controller->time_mutex == NULL)
    {
        return;
    }

    rtos_lock_mutex(&controller->time_mutex);
    controller->current_time_ms = time_ms;
    controller->last_time_update_tick_ms = rtos_get_time();
    rtos_unlock_mutex(&controller->time_mutex);
}

// Restart a seek session without closing parser/decoders to reduce latency.
static avdk_err_t video_player_ctlr_seek_inplace(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    if (controller->active_container_parser == NULL)
    {
        LOGE("%s: active parser is NULL, cannot seek in-place\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // Invalidate in-flight packets/decodes and start a fresh seek session.
    controller->play_session_id++;
    if (controller->play_session_id == 0)
    {
        controller->play_session_id = 1;
    }

    // For disabled tracks, treat as "already EOF" so that the other track can drive FINISHED.
    controller->video_eof_reached = !controller->video_track_enabled;
    controller->audio_eof_reached = !controller->audio_track_enabled;
    controller->eof_notified_session_id = 0;
    controller->paused_seek_pending = false;
    controller->paused_seek_time_ms = 0;
    controller->is_paused = false;
    controller->pause_start_time_ms = 0;

    // Reset pipeline PTS to the new seek target for immediate sync.
    uint64_t start_pts_ms = video_player_get_current_time_ms(controller);
    video_pipeline_set_next_frame_pts(&controller->video_pipeline, start_pts_ms);
    video_pipeline_update_pts(&controller->video_pipeline, start_pts_ms);
    audio_pipeline_update_pts(&controller->audio_pipeline, start_pts_ms);

    // Drain stale packets from previous session.
    drain_filled_packet_pool(&controller->audio_pipeline.parser_to_decode_pool,
                             controller->config.audio.buffer_free_cb,
                             controller->config.user_data);
    drain_filled_packet_pool_video(&controller->video_pipeline.parser_to_decode_pool,
                                   controller->config.video.packet_buffer_free_cb,
                                   controller->config.user_data);

    // Best-effort reset audio output to drop stale buffered audio.
    if (controller->config.audio.audio_output_reset_cb != NULL)
    {
        void *reset_user_data = controller->config.audio.audio_output_reset_user_data;
        if (reset_user_data == NULL)
        {
            reset_user_data = controller->config.user_data;
        }
        avdk_err_t reset_ret = controller->config.audio.audio_output_reset_cb(reset_user_data);
        if (reset_ret != AVDK_ERR_OK)
        {
            LOGW("%s: audio_output_reset_cb failed, ret=%d\n", __func__, reset_ret);
        }
    }

    controller->video_parse_thread_running = controller->video_track_enabled;
    controller->audio_parse_thread_running = controller->audio_track_enabled;
    controller->audio_decode_thread_running = controller->audio_track_enabled;
    controller->video_decode_thread_running = controller->video_track_enabled;
    controller->module_status.status = VIDEO_PLAYER_STATUS_PLAYING;

    if (controller->video_parse_thread_running)
    {
        rtos_set_semaphore(&controller->video_parse_sem);
    }
    if (controller->audio_parse_thread_running)
    {
        rtos_set_semaphore(&controller->audio_parse_sem);
    }

    return AVDK_ERR_OK;
}

static void drain_all_packet_pool_nodes(video_player_buffer_pool_t *pool,
                                        video_player_audio_buffer_free_cb_t free_cb,
                                        void *user_data)
{
    if (pool == NULL || pool->nodes == NULL || pool->count == 0)
    {
        return;
    }

    for (uint32_t i = 0; i < pool->count; i++)
    {
        video_player_buffer_node_t *node = &pool->nodes[i];
        if (node->buffer.data != NULL)
        {
            if (free_cb != NULL)
            {
                free_cb(user_data, &node->buffer);
            }
            node->buffer.data = NULL;
        }
        node->buffer.length = 0;
        node->buffer.pts = 0;
        node->buffer.frame_buffer = NULL;
        node->buffer.user_data = NULL;
        node->in_use = false;
    }
}

static void drain_all_packet_pool_nodes_video(video_player_buffer_pool_t *pool,
                                              video_player_video_packet_buffer_free_cb_t free_cb,
                                              void *user_data)
{
    if (pool == NULL || pool->nodes == NULL || pool->count == 0)
    {
        return;
    }

    for (uint32_t i = 0; i < pool->count; i++)
    {
        video_player_buffer_node_t *node = &pool->nodes[i];
        if (node->buffer.data != NULL)
        {
            if (free_cb != NULL)
            {
                free_cb(user_data, &node->buffer);
            }
            node->buffer.data = NULL;
        }
        node->buffer.length = 0;
        node->buffer.pts = 0;
        node->buffer.frame_buffer = NULL;
        node->buffer.user_data = NULL;
        node->in_use = false;
    }
}

static void cleanup_open_resources(private_video_player_ctlr_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    bk_video_player_video_decode_deinit(controller);
    bk_video_player_audio_decode_deinit(controller);
    bk_video_player_container_parse_deinit(controller);

    rtos_deinit_semaphore(&controller->stop_sem);

    if (controller->active_mutex != NULL)
    {
        rtos_deinit_mutex(&controller->active_mutex);
        controller->active_mutex = NULL;
    }
    if (controller->time_mutex != NULL)
    {
        rtos_deinit_mutex(&controller->time_mutex);
        controller->time_mutex = NULL;
    }

    /*
     * IMPORTANT:
     * buffer_pool_deinit() only frees the node array and does NOT free node->buffer.data.
     * Best-effort release any leftover encoded packet buffers before destroying pool node arrays.
     */
    drain_filled_packet_pool(&controller->audio_pipeline.parser_to_decode_pool,
                             controller->config.audio.buffer_free_cb,
                             controller->config.user_data);
    drain_filled_packet_pool_video(&controller->video_pipeline.parser_to_decode_pool,
                                   controller->config.video.packet_buffer_free_cb,
                                   controller->config.user_data);
    if (controller->audio_pipeline.parser_to_decode_pool.nodes != NULL)
    {
        for (uint32_t i = 0; i < controller->audio_pipeline.parser_to_decode_pool.count; i++)
        {
            video_player_buffer_node_t *node = &controller->audio_pipeline.parser_to_decode_pool.nodes[i];
            if (node->buffer.data != NULL && controller->config.audio.buffer_free_cb != NULL)
            {
                controller->config.audio.buffer_free_cb(controller->config.user_data, &node->buffer);
            }
            node->buffer.data = NULL;
            node->buffer.length = 0;
            node->buffer.pts = 0;
            node->buffer.frame_buffer = NULL;
            node->buffer.user_data = NULL;
            node->in_use = false;
        }
    }
    if (controller->video_pipeline.parser_to_decode_pool.nodes != NULL)
    {
        for (uint32_t i = 0; i < controller->video_pipeline.parser_to_decode_pool.count; i++)
        {
            video_player_buffer_node_t *node = &controller->video_pipeline.parser_to_decode_pool.nodes[i];
            if (node->buffer.data != NULL && controller->config.video.packet_buffer_free_cb != NULL)
            {
                controller->config.video.packet_buffer_free_cb(controller->config.user_data, &node->buffer);
            }
            node->buffer.data = NULL;
            node->buffer.length = 0;
            node->buffer.pts = 0;
            node->buffer.frame_buffer = NULL;
            node->buffer.user_data = NULL;
            node->in_use = false;
        }
    }
    video_pipeline_deinit(&controller->video_pipeline);
    video_player_audio_pipeline_deinit(&controller->audio_pipeline);
}

static avdk_err_t video_player_ctlr_open(bk_video_player_ctlr_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == VIDEO_PLAYER_STATUS_NONE, AVDK_ERR_INVAL, TAG, "player is not closed");

    uint32_t video_parser_to_decode_count = (controller->config.video.parser_to_decode_buffer_count > 0) ?
                                           controller->config.video.parser_to_decode_buffer_count : 2;
    uint32_t video_decode_to_output_count = (controller->config.video.decode_to_output_buffer_count > 0) ?
                                           controller->config.video.decode_to_output_buffer_count : 2;
    uint32_t audio_parser_to_decode_count = (controller->config.audio.parser_to_decode_buffer_count > 0) ?
                                           controller->config.audio.parser_to_decode_buffer_count : 2;
    uint32_t audio_decode_to_output_count = (controller->config.audio.decode_to_output_buffer_count > 0) ?
                                           controller->config.audio.decode_to_output_buffer_count : 2;

    ret = video_pipeline_init(&controller->video_pipeline, video_parser_to_decode_count, video_decode_to_output_count);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to init video pipeline, ret=%d\n", __func__, ret);
        goto cleanup;
    }

    ret = video_player_audio_pipeline_init(&controller->audio_pipeline, audio_parser_to_decode_count, audio_decode_to_output_count);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to init audio pipeline, ret=%d\n", __func__, ret);
        video_pipeline_deinit(&controller->video_pipeline);
        goto cleanup;
    }

    if (rtos_init_mutex(&controller->active_mutex) != BK_OK)
    {
        LOGE("%s: Failed to init active mutex\n", __func__);
        ret = AVDK_ERR_IO;
        goto cleanup;
    }

    if (rtos_init_mutex(&controller->time_mutex) != BK_OK)
    {
        LOGE("%s: Failed to init time mutex\n", __func__);
        ret = AVDK_ERR_IO;
        goto cleanup;
    }
    controller->current_time_ms = 0;
    controller->av_sync_offset_ms = 0;
    controller->last_time_update_tick_ms = rtos_get_time();
    controller->clock_source = 0; // prefer audio-driven clock by default

    if (rtos_init_semaphore(&controller->stop_sem, 1) != BK_OK)
    {
        LOGE("%s: Failed to init stop semaphore\n", __func__);
        ret = AVDK_ERR_IO;
        goto cleanup;
    }

    controller->is_paused = false;
    controller->pause_start_time_ms = 0;
    controller->video_eof_reached = false;
    controller->audio_eof_reached = false;
    controller->eof_notified_session_id = 0;
    controller->paused_seek_pending = false;
    controller->paused_seek_time_ms = 0;
    controller->current_file_path = NULL;
    controller->delivered_video_frame_index = 0;
    controller->video_seek_drop_enable = false;
    controller->video_seek_drop_until_pts_ms = 0;
    controller->vp_evt_last_video_pts = 0;
    controller->vp_evt_last_video_time_ms = 0;
    controller->vp_evt_last_video_session_id = 0;

    ret = bk_video_player_container_parse_init(controller);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to init container parse resources, ret=%d\n", __func__, ret);
        goto cleanup;
    }

    ret = bk_video_player_audio_decode_init(controller);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to init audio decode resources, ret=%d\n", __func__, ret);
        bk_video_player_container_parse_deinit(controller);
        goto cleanup;
    }

    ret = bk_video_player_video_decode_init(controller);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to init video decode resources, ret=%d\n", __func__, ret);
        bk_video_player_audio_decode_deinit(controller);
        bk_video_player_container_parse_deinit(controller);
        goto cleanup;
    }

    controller->module_status.status = VIDEO_PLAYER_STATUS_OPENED;
    LOGI("%s: Player opened successfully\n", __func__);

    return AVDK_ERR_OK;

cleanup:
    cleanup_open_resources(controller);
    return ret;
}

static avdk_err_t video_player_ctlr_close(bk_video_player_ctlr_handle_t handler)
{
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == VIDEO_PLAYER_STATUS_OPENED ||
                         controller->module_status.status == VIDEO_PLAYER_STATUS_PLAYING ||
                         controller->module_status.status == VIDEO_PLAYER_STATUS_PAUSED ||
                         controller->module_status.status == VIDEO_PLAYER_STATUS_STOPPED ||
                         controller->module_status.status == VIDEO_PLAYER_STATUS_FINISHED,
                         AVDK_ERR_INVAL, TAG, "player status is invalid");

    bk_video_player_video_decode_deinit(controller);
    bk_video_player_audio_decode_deinit(controller);
    bk_video_player_container_parse_deinit(controller);

    deinit_active_decoders(controller);

    // Release any leftover probe cache.
    rtos_lock_mutex(&controller->decoder_list_mutex);
    video_player_prefetch_parser_clear_locked(controller);
    rtos_unlock_mutex(&controller->decoder_list_mutex);

    // Stop event notify thread (if any) before freeing controller resources.
    video_player_event_notify_deinit(controller);

    rtos_deinit_semaphore(&controller->stop_sem);

    if (controller->active_mutex != NULL)
    {
        rtos_deinit_mutex(&controller->active_mutex);
        controller->active_mutex = NULL;
    }
    if (controller->time_mutex != NULL)
    {
        rtos_deinit_mutex(&controller->time_mutex);
        controller->time_mutex = NULL;
    }

    // Best-effort release any leftover encoded packet buffers before destroying pool node arrays.
    drain_filled_packet_pool(&controller->audio_pipeline.parser_to_decode_pool,
                             controller->config.audio.buffer_free_cb,
                             controller->config.user_data);
    drain_filled_packet_pool_video(&controller->video_pipeline.parser_to_decode_pool,
                                   controller->config.video.packet_buffer_free_cb,
                                   controller->config.user_data);
    drain_all_packet_pool_nodes(&controller->audio_pipeline.parser_to_decode_pool,
                                controller->config.audio.buffer_free_cb,
                                controller->config.user_data);
    drain_all_packet_pool_nodes_video(&controller->video_pipeline.parser_to_decode_pool,
                                      controller->config.video.packet_buffer_free_cb,
                                      controller->config.user_data);

    video_pipeline_deinit(&controller->video_pipeline);
    video_player_audio_pipeline_deinit(&controller->audio_pipeline);

    if (controller->current_file_path != NULL)
    {
        os_free(controller->current_file_path);
        controller->current_file_path = NULL;
    }

    controller->module_status.status = VIDEO_PLAYER_STATUS_CLOSED;
    LOGI("%s: Player closed successfully\n", __func__);

    return AVDK_ERR_OK;
}

static avdk_err_t video_player_ctlr_set_file_path(bk_video_player_ctlr_handle_t handler, const char *file_path)
{
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");

    // If there is a probe cache for a different file, release it now.
    rtos_lock_mutex(&controller->decoder_list_mutex);
    if (controller->prefetch_container_parser != NULL &&
        controller->prefetch_file_path != NULL &&
        os_strcmp(controller->prefetch_file_path, file_path) != 0)
    {
        video_player_prefetch_parser_clear_locked(controller);
    }
    rtos_unlock_mutex(&controller->decoder_list_mutex);

    if (controller->current_file_path != NULL)
    {
        os_free(controller->current_file_path);
        controller->current_file_path = NULL;
    }

    size_t path_len = os_strlen(file_path) + 1;
    controller->current_file_path = (char *)os_malloc(path_len);
    AVDK_RETURN_ON_FALSE(controller->current_file_path, AVDK_ERR_NOMEM, TAG, "Failed to allocate current_file_path");
    os_strncpy(controller->current_file_path, file_path, path_len - 1);
    controller->current_file_path[path_len - 1] = '\0';

    /*
     * IMPORTANT:
     * set_file_path() is used when switching to a new media file (e.g. playlist play/index).
     * Reset the unified playback time point here so that the next play starts from the beginning,
     * instead of inheriting the previous file's EOF timestamp and accidentally seeking near EOF.
     *
     * Seek/restart flow does NOT call set_file_path(), so this will not break bk_video_player_ctlr_seek().
     */
    controller->paused_seek_pending = false;
    controller->paused_seek_time_ms = 0;
    video_player_set_current_time_ms(controller, 0);

    return AVDK_ERR_OK;
}

static avdk_err_t video_player_ctlr_play(bk_video_player_ctlr_handle_t handler, const char *file_path)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == VIDEO_PLAYER_STATUS_OPENED ||
                         controller->module_status.status == VIDEO_PLAYER_STATUS_STOPPED ||
                         controller->module_status.status == VIDEO_PLAYER_STATUS_FINISHED,
                         AVDK_ERR_INVAL, TAG, "player is not in valid state for play");

    /*
     * Save current file path for seek/resume restart (single file only).
     *
     * IMPORTANT:
     * - When switching to a NEW file (playlist/index/start_file), set_file_path() must reset
     *   current_time_ms to 0 to avoid inheriting previous file's EOF timestamp.
     * - When restarting playback for the SAME file (seek/paused-seek-resume), do NOT call
     *   set_file_path(), because it would reset current_time_ms to 0 and break seek target.
     */
    if (controller->current_file_path == NULL || os_strcmp(controller->current_file_path, file_path) != 0)
    {
        avdk_err_t set_path_ret = video_player_ctlr_set_file_path(handler, file_path);
        if (set_path_ret != AVDK_ERR_OK)
        {
            return set_path_ret;
        }
    }

    deinit_active_decoders(controller);

    ret = select_decoders(controller, file_path);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to select decoders for: %s, ret=%d\n", __func__, file_path, ret);
        deinit_active_decoders(controller);
        if (controller->module_status.status != VIDEO_PLAYER_STATUS_OPENED)
        {
            controller->module_status.status = VIDEO_PLAYER_STATUS_STOPPED;
        }
        return ret;
    }

    // Configure upper-layer audio output for the new file (sample rate/bits/channels).
    // This must happen after select_decoders(), because current_media_info is updated there.
    if (controller->audio_track_enabled &&
        controller->config.audio.audio_output_config_cb != NULL &&
        controller->current_media_info.audio.channels > 0 &&
        controller->current_media_info.audio.sample_rate > 0)
    {
        void *cfg_user_data = controller->config.audio.audio_output_config_user_data;
        if (cfg_user_data == NULL)
        {
            cfg_user_data = controller->config.user_data;
        }

        avdk_err_t cfg_ret = controller->config.audio.audio_output_config_cb(cfg_user_data,
                                                                            &controller->current_media_info.audio);
        if (cfg_ret != AVDK_ERR_OK)
        {
            LOGE("%s: audio_output_config_cb failed, ret=%d\n", __func__, cfg_ret);
            deinit_active_decoders(controller);
            if (controller->module_status.status != VIDEO_PLAYER_STATUS_OPENED)
            {
                controller->module_status.status = VIDEO_PLAYER_STATUS_STOPPED;
            }
            return cfg_ret;
        }
    }

    // Select clock source: prefer audio-driven if audio track is enabled, otherwise video-driven.
    if (controller->audio_track_enabled)
    {
        controller->clock_source = VIDEO_PLAYER_CLOCK_AUDIO;
    }
    else
    {
        controller->clock_source = VIDEO_PLAYER_CLOCK_VIDEO;
    }

    // Reset pipeline PTS state based on the single current playback time point.
    uint64_t start_pts_ms = video_player_get_current_time_ms(controller);
    video_pipeline_set_next_frame_pts(&controller->video_pipeline, start_pts_ms);
    video_pipeline_update_pts(&controller->video_pipeline, start_pts_ms);
    audio_pipeline_update_pts(&controller->audio_pipeline, start_pts_ms);

    // Drain stale encoded packets (with old PTS) from previous play/pause session.
    drain_filled_packet_pool(&controller->audio_pipeline.parser_to_decode_pool,
                             controller->config.audio.buffer_free_cb,
                             controller->config.user_data);
    drain_filled_packet_pool_video(&controller->video_pipeline.parser_to_decode_pool,
                                   controller->config.video.packet_buffer_free_cb,
                                   controller->config.user_data);

    // For disabled tracks, treat as "already EOF" so that the other track can drive FINISHED.
    controller->video_eof_reached = !controller->video_track_enabled;
    controller->audio_eof_reached = !controller->audio_track_enabled;
    controller->paused_seek_pending = false;
    controller->paused_seek_time_ms = 0;

    // Start a new playback session (used to tag packets and drop stale ones).
    controller->play_session_id++;
    if (controller->play_session_id == 0)
    {
        controller->play_session_id = 1;
    }

    /*
     * If previous session was paused, make sure we clear pause state here.
     * Otherwise, parse/decode threads will keep sleeping and playback will look "stuck"
     * after restart (start->pause->start).
     */
    controller->is_paused = false;
    controller->pause_start_time_ms = 0;
    controller->delivered_video_frame_index = 0;
    controller->vp_evt_last_video_pts = 0;
    controller->vp_evt_last_video_time_ms = rtos_get_time();
    controller->vp_evt_last_video_session_id = controller->play_session_id;

    controller->video_parse_thread_running = controller->video_track_enabled;
    controller->audio_parse_thread_running = controller->audio_track_enabled;
    controller->audio_decode_thread_running = controller->audio_track_enabled;
    controller->video_decode_thread_running = controller->video_track_enabled;

    if (controller->video_parse_thread_running)
    {
        rtos_set_semaphore(&controller->video_parse_sem);
    }
    if (controller->audio_parse_thread_running)
    {
        rtos_set_semaphore(&controller->audio_parse_sem);
    }

    controller->module_status.status = VIDEO_PLAYER_STATUS_PLAYING;
    LOGI("%s: Player started playing: %s\n", __func__, file_path);

    return AVDK_ERR_OK;
}

static avdk_err_t video_player_ctlr_stop(bk_video_player_ctlr_handle_t handler)
{
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == VIDEO_PLAYER_STATUS_PLAYING ||
                         controller->module_status.status == VIDEO_PLAYER_STATUS_PAUSED ||
                         controller->module_status.status == VIDEO_PLAYER_STATUS_FINISHED,
                         AVDK_ERR_INVAL, TAG, "player is not in valid state for stop");

    controller->video_parse_thread_running = false;
    controller->audio_parse_thread_running = false;
    controller->audio_decode_thread_running = false;
    controller->video_decode_thread_running = false;

    // Invalidate any in-flight packets/decodes immediately.
    // Decode threads may still be processing a buffer already taken from pool; session mismatch will drop it safely.
    controller->play_session_id++;
    if (controller->play_session_id == 0)
    {
        controller->play_session_id = 1;
    }
    rtos_set_semaphore(&controller->video_parse_sem);
    rtos_set_semaphore(&controller->audio_parse_sem);

    rtos_delay_milliseconds(50);

    deinit_active_decoders(controller);

    controller->video_eof_reached = false;
    controller->audio_eof_reached = false;
    controller->eof_notified_session_id = 0;
    controller->is_paused = false;
    controller->pause_start_time_ms = 0;

    os_memset(&controller->current_media_info, 0, sizeof(controller->current_media_info));

    // Reset pipeline PTS state based on current playback time.
    // Do not change controller->current_time_ms here; caller decides whether to restart from 0 or seek time.
    uint64_t cur_pts_ms = video_player_get_current_time_ms(controller);
    video_pipeline_set_next_frame_pts(&controller->video_pipeline, cur_pts_ms);
    video_pipeline_update_pts(&controller->video_pipeline, cur_pts_ms);
    audio_pipeline_update_pts(&controller->audio_pipeline, cur_pts_ms);

    // Drain any remaining encoded packets to avoid stale PTS on next start.
    drain_filled_packet_pool(&controller->audio_pipeline.parser_to_decode_pool,
                             controller->config.audio.buffer_free_cb,
                             controller->config.user_data);
    drain_filled_packet_pool_video(&controller->video_pipeline.parser_to_decode_pool,
                                   controller->config.video.packet_buffer_free_cb,
                                   controller->config.user_data);

    controller->module_status.status = VIDEO_PLAYER_STATUS_STOPPED;
    LOGI("%s: Player stopped\n", __func__);

    return AVDK_ERR_OK;
}

static avdk_err_t video_player_ctlr_set_pause(bk_video_player_ctlr_handle_t handler, bool pause)
{
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");

    if (pause && !controller->is_paused)
    {
        // Pausing: record current time
        controller->is_paused = true;
        controller->pause_start_time_ms = rtos_get_time();
        controller->module_status.status = VIDEO_PLAYER_STATUS_PAUSED;
        LOGI("%s: Playback paused\n", __func__);
    }
    else if (!pause && controller->is_paused)
    {
        // Resume after paused seek:
        // Old packets may still exist in decode pipelines, so we must restart playback from the seek position.
        if (controller->paused_seek_pending)
        {
            AVDK_RETURN_ON_FALSE(controller->current_file_path != NULL, AVDK_ERR_INVAL, TAG, "current_file_path is NULL");

            uint64_t t = controller->paused_seek_time_ms;
            controller->paused_seek_pending = false;
            controller->paused_seek_time_ms = 0;

            avdk_err_t ret = video_player_ctlr_stop(handler);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: stop failed during resume-after-paused-seek, ret=%d\n", __func__, ret);
                return ret;
            }

            // Update time directly (do not call bk_video_player_ctlr_seek here to avoid PAUSED preview path).
            video_player_set_current_time_ms(controller, t);

            ret = video_player_ctlr_play(handler, controller->current_file_path);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: play failed during resume-after-paused-seek, ret=%d\n", __func__, ret);
                return ret;
            }
            return AVDK_ERR_OK;
        }

        // Normal resuming: current playback time is driven by decoded A/V PTS (current_time_ms),
        // so no additional time adjustment is required here.
        controller->is_paused = false;
        controller->pause_start_time_ms = 0;
        controller->module_status.status = VIDEO_PLAYER_STATUS_PLAYING;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t video_player_ctlr_seek_preview(bk_video_player_ctlr_handle_t handle, uint64_t time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");

    private_video_player_ctlr_t *controller = __containerof(handle, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == VIDEO_PLAYER_STATUS_PAUSED,
                         AVDK_ERR_INVAL, TAG, "seek preview only supported in PAUSED state");

    // Update current playback time first (protected by mutex).
    video_player_set_current_time_ms(controller, time_ms);

    // Decode and display exactly one video frame without resuming playback and without decoding audio.
    avdk_err_t ret = AVDK_ERR_OK;
    video_player_container_parser_ops_t *parser = NULL;
    video_player_video_decoder_ops_t *decoder = NULL;

    rtos_lock_mutex(&controller->active_mutex);
    parser = controller->active_container_parser;
    decoder = controller->active_video_decoder;
    rtos_unlock_mutex(&controller->active_mutex);

    AVDK_RETURN_ON_FALSE(parser && parser->get_video_packet_size && parser->read_video_packet,
                         AVDK_ERR_UNSUPPORTED, TAG, "active video parser is invalid");
    AVDK_RETURN_ON_FALSE(decoder && decoder->decode, AVDK_ERR_UNSUPPORTED, TAG, "active video decoder is invalid");
    AVDK_RETURN_ON_FALSE(controller->config.video.packet_buffer_alloc_cb &&
                         controller->config.video.packet_buffer_free_cb &&
                         controller->config.video.buffer_alloc_cb,
                         AVDK_ERR_UNSUPPORTED, TAG, "video buffer callbacks are invalid");

    uint64_t target_pts = video_player_get_current_time_ms(controller);
    const uint64_t drop_until_pts = target_pts;

    // Seek preview may require a preroll keyframe packet before the target P-frame packet.
    // We decode up to 2 frames:
    // - If decoded pts < drop_until_pts, drop it (do NOT callback).
    // - Deliver the first decoded frame whose pts >= drop_until_pts.
    for (uint32_t attempt = 0; attempt < 2; attempt++)
    {
        uint64_t req_pts = (attempt == 0) ? target_pts : VIDEO_PLAYER_PTS_INVALID;

        uint32_t pkt_size = 0;
        rtos_lock_mutex(&controller->active_mutex);
        ret = parser->get_video_packet_size(parser, &pkt_size, req_pts);
        rtos_unlock_mutex(&controller->active_mutex);
        if (ret != AVDK_ERR_OK || pkt_size == 0)
        {
            LOGW("%s: get_video_packet_size failed, ret=%d, size=%u, req_pts=%llu\n",
                 __func__, ret, pkt_size, (unsigned long long)req_pts);
            return (ret != AVDK_ERR_OK) ? ret : AVDK_ERR_IO;
        }

        video_player_buffer_t pkt = {0};
        pkt.length = pkt_size;
        ret = controller->config.video.packet_buffer_alloc_cb(controller->config.user_data, &pkt);
        if (ret != AVDK_ERR_OK || pkt.data == NULL)
        {
            LOGE("%s: packet_buffer_alloc_cb failed, ret=%d, size=%u\n", __func__, ret, pkt_size);
            return (ret != AVDK_ERR_OK) ? ret : AVDK_ERR_NOMEM;
        }

        pkt.pts = 0;
        rtos_lock_mutex(&controller->active_mutex);
        ret = parser->read_video_packet(parser, &pkt, req_pts);
        rtos_unlock_mutex(&controller->active_mutex);
        if (ret != AVDK_ERR_OK)
        {
            controller->config.video.packet_buffer_free_cb(controller->config.user_data, &pkt);
            LOGW("%s: read_video_packet failed, ret=%d\n", __func__, ret);
            return ret;
        }

        video_player_buffer_t out = {0};
        uint32_t bytes_per_pixel = 2;
        if (controller->config.video.output_format == PIXEL_FMT_RGB888)
        {
            bytes_per_pixel = 3;
        }
        out.length = controller->current_media_info.video.width * controller->current_media_info.video.height * bytes_per_pixel;
        ret = controller->config.video.buffer_alloc_cb(controller->config.user_data, &out);
        if (ret != AVDK_ERR_OK)
        {
            controller->config.video.packet_buffer_free_cb(controller->config.user_data, &pkt);
            LOGE("%s: buffer_alloc_cb failed, ret=%d\n", __func__, ret);
            return ret;
        }

        if (out.frame_buffer != NULL)
        {
            frame_buffer_t *fb = (frame_buffer_t *)out.frame_buffer;
            fb->fmt = controller->config.video.output_format;
        }

        rtos_lock_mutex(&controller->active_mutex);
        ret = decoder->decode(decoder, &pkt, &out, controller->config.video.output_format);
        rtos_unlock_mutex(&controller->active_mutex);

        // Encoded packet buffer must always be released after decode.
        controller->config.video.packet_buffer_free_cb(controller->config.user_data, &pkt);

        if (ret != AVDK_ERR_OK)
        {
            if (controller->config.video.buffer_free_cb != NULL)
            {
                controller->config.video.buffer_free_cb(controller->config.user_data, &out);
            }
            LOGW("%s: decode failed, ret=%d\n", __func__, ret);
            return ret;
        }

        // Drop preroll frames whose pts is before seek target.
        if (out.pts < drop_until_pts)
        {
            if (controller->config.video.buffer_free_cb != NULL)
            {
                controller->config.video.buffer_free_cb(controller->config.user_data, &out);
            }
            continue;
        }

        // Update current playback time based on decoded frame PTS if available.
        if (out.pts > 0)
        {
            video_player_set_current_time_ms(controller, out.pts);
        }

        // Display one frame. Ownership follows the same rule as video decode thread:
        // If decode_complete_cb is provided, the upper layer takes ownership and must free the buffer.
        if (controller->config.video.decode_complete_cb != NULL)
        {
            video_player_video_frame_meta_t meta;
            os_memset(&meta, 0, sizeof(meta));
            meta.video = controller->current_media_info.video;
            meta.frame_index = 0; // Seek preview frame is not part of normal playback sequence.
            meta.pts_ms = out.pts;

            controller->config.video.decode_complete_cb(controller->config.user_data, &meta, &out);
        }
        else if (controller->config.video.buffer_free_cb != NULL)
        {
            controller->config.video.buffer_free_cb(controller->config.user_data, &out);
        }

        LOGI("%s: Seek preview done at %llu ms (frame_pts=%llu)\n",
             __func__, (unsigned long long)time_ms, (unsigned long long)out.pts);

        return AVDK_ERR_OK;
    }

    LOGW("%s: Seek preview failed to reach target pts=%llu ms\n",
         __func__, (unsigned long long)drop_until_pts);
    return AVDK_ERR_IO;
}

static avdk_err_t video_player_ctlr_seek(bk_video_player_ctlr_handle_t handle, uint64_t time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");

    private_video_player_ctlr_t *controller = __containerof(handle, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(controller->current_file_path != NULL, AVDK_ERR_INVAL, TAG, "current_file_path is NULL");

    // Update the single current playback time point (protected by mutex).
    // If currently paused, controller will decode exactly one video frame for preview and keep paused.
    // It also marks pending so that resume will restart from the seek position.
    video_player_set_current_time_ms(controller, time_ms);
    LOGI("%s: Seek to %llu ms (current playback time updated, status=%d)\n",
         __func__, (unsigned long long)time_ms, controller->module_status.status);

    if (controller->module_status.status == VIDEO_PLAYER_STATUS_PAUSED)
    {
        controller->paused_seek_pending = true;
        controller->paused_seek_time_ms = time_ms;
        return video_player_ctlr_seek_preview(handle, time_ms);
    }

    controller->paused_seek_pending = false;
    controller->paused_seek_time_ms = 0;

    // For PLAYING/FINISHED: restart session without closing parser to reduce seek latency.
    if (controller->module_status.status == VIDEO_PLAYER_STATUS_PLAYING ||
        controller->module_status.status == VIDEO_PLAYER_STATUS_FINISHED)
    {
        return video_player_ctlr_seek_inplace(controller);
    }

    // For OPENED/STOPPED: start playing from seek position.
    return video_player_ctlr_play(handle, controller->current_file_path);
}

static avdk_err_t video_player_ctlr_fast_forward(bk_video_player_ctlr_handle_t handle, uint32_t time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");

    private_video_player_ctlr_t *controller = __containerof(handle, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");

    uint64_t current_ms = video_player_get_current_time_ms(controller);
    uint64_t target_ms = current_ms + (uint64_t)time_ms;

    // Best-effort clamp to duration if available.
    if (controller->current_media_info.duration_ms > 0 && target_ms > controller->current_media_info.duration_ms)
    {
        target_ms = controller->current_media_info.duration_ms;
    }

    return video_player_ctlr_seek(handle, target_ms);
}

static avdk_err_t video_player_ctlr_rewind(bk_video_player_ctlr_handle_t handle, uint32_t time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");

    private_video_player_ctlr_t *controller = __containerof(handle, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");

    uint64_t current_ms = video_player_get_current_time_ms(controller);
    uint64_t target_ms = 0;
    if (current_ms > (uint64_t)time_ms)
    {
        target_ms = current_ms - (uint64_t)time_ms;
    }

    return video_player_ctlr_seek(handle, target_ms);
}

static avdk_err_t video_player_ctlr_register_audio_decoder(bk_video_player_ctlr_handle_t handler,
                                                           const video_player_audio_decoder_ops_t *decoder_ops)
{
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(decoder_ops, AVDK_ERR_INVAL, TAG, "decoder_ops is NULL");

    return bk_video_player_audio_decoder_list_add(controller, decoder_ops);
}

static avdk_err_t video_player_ctlr_register_video_decoder(bk_video_player_ctlr_handle_t handler,
                                                           video_player_video_decoder_ops_t *decoder_ops)
{
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(decoder_ops, AVDK_ERR_INVAL, TAG, "decoder_ops is NULL");

    return bk_video_player_video_decoder_list_add(controller, decoder_ops);
}

static avdk_err_t video_player_ctlr_register_container_parser(bk_video_player_ctlr_handle_t handler,
                                                              video_player_container_parser_ops_t *parser_ops)
{
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(parser_ops, AVDK_ERR_INVAL, TAG, "parser_ops is NULL");

    return bk_video_player_container_parser_list_add(controller, parser_ops);
}

static avdk_err_t video_player_ctlr_get_media_info(bk_video_player_ctlr_handle_t handle,
                                              const char *file_path,
                                              video_player_media_info_t *media_info)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(media_info, AVDK_ERR_INVAL, TAG, "media_info is NULL");

    private_video_player_ctlr_t *controller = __containerof(handle, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");

    /*
     * Media info can be queried either from current playback (cached params) or by probing a file path.
     * If file_path is NULL/empty, fall back to controller->current_file_path.
     *
     * IMPORTANT: container parser ops in this project are typically singletons (shared parser_ctx),
     * so do not probe while playback threads may be using the active parser.
     */
    os_memset(media_info, 0, sizeof(video_player_media_info_t));

    const char *path_to_use = NULL;
    rtos_lock_mutex(&controller->active_mutex);
    if (file_path != NULL && file_path[0] != '\0')
    {
        path_to_use = file_path;
    }
    else if (controller->current_file_path != NULL && controller->current_file_path[0] != '\0')
    {
        path_to_use = controller->current_file_path;
    }

    // Do not probe parsers while playing/paused to avoid races on shared parser_ctx.
    video_player_status_t status = controller->module_status.status;
    video_player_container_parser_ops_t *active_parser = controller->active_container_parser;
    bool can_use_cached = (active_parser != NULL &&
                           path_to_use != NULL &&
                           controller->current_file_path != NULL &&
                           os_strcmp(path_to_use, controller->current_file_path) == 0);
    rtos_unlock_mutex(&controller->active_mutex);

    AVDK_RETURN_ON_FALSE(path_to_use != NULL, AVDK_ERR_INVAL, TAG, "file path is NULL/empty and no default path");

    bool is_playing_or_paused = (status == VIDEO_PLAYER_STATUS_PLAYING || status == VIDEO_PLAYER_STATUS_PAUSED);

    // Fast path: use cached params when querying current opened file.
    if (can_use_cached)
    {
        rtos_lock_mutex(&controller->active_mutex);
        *media_info = controller->current_media_info;
        rtos_unlock_mutex(&controller->active_mutex);
    }
    else
    {
        // Probe path: auto-select a suitable parser by trying registered parsers.
        avdk_err_t last_open_err = is_playing_or_paused ? AVDK_ERR_BUSY : AVDK_ERR_UNSUPPORTED;
        bool got_any = false;

        /*
         * Hold decoder_list_mutex during traversal to protect list nodes from being freed.
         *
         * NOTE:
         * This expands the lock scope to cover parser probing (open/get_media_info/close).
         * It avoids use-after-free without copying the list, but may increase lock hold time.
         */
        rtos_lock_mutex(&controller->decoder_list_mutex);
        video_player_container_parser_node_t *head = controller->container_parser_list;

        // Two-pass selection by file suffix to reduce meaningless open errors.
        for (uint8_t pass = 0; pass < 2 && !got_any; pass++)
        {
            video_player_container_parser_node_t *node = head;
            while (node != NULL)
            {
                video_player_container_parser_ops_t *base_ops = node->ops;
                if (base_ops == NULL || base_ops->open == NULL || base_ops->close == NULL)
                {
                    node = node->next;
                    continue;
                }

                /*
                 * For a different file during PLAYING/PAUSED, we must use an independent parser instance
                 * (separate parser_ctx) to avoid racing with playback on singleton parser_ctx.
                 */
                video_player_container_parser_ops_t *parser_ops = base_ops;
                bool need_destroy = false;
                if (is_playing_or_paused)
                {
                    if (base_ops->create == NULL || base_ops->destroy == NULL)
                    {
                        node = node->next;
                        continue;
                    }
                    parser_ops = base_ops->create(controller->config.user_data,
                                                  controller->config.video.packet_buffer_alloc_cb,
                                                  controller->config.video.packet_buffer_free_cb);
                    if (parser_ops == NULL)
                    {
                        node = node->next;
                        continue;
                    }
                    need_destroy = true;
                }
                else
                {
                    // Not playing: prefer independent instance if available.
                    if (base_ops->create != NULL && base_ops->destroy != NULL)
                    {
                        parser_ops = base_ops->create(controller->config.user_data,
                                                      controller->config.video.packet_buffer_alloc_cb,
                                                      controller->config.video.packet_buffer_free_cb);
                        if (parser_ops != NULL)
                        {
                            need_destroy = true;
                        }
                        else
                        {
                            parser_ops = base_ops;
                        }
                    }
                }

                bool matched = false;
                // Match by extensions (if supported). If not supported, matched stays false and
                // the parser will be tried in fallback pass.
                matched = container_parser_supports_file_path(parser_ops, path_to_use);

                if (pass == 0)
                {
                    if (!matched)
                    {
                        if (need_destroy && parser_ops->destroy != NULL)
                        {
                            parser_ops->destroy(parser_ops);
                        }
                        node = node->next;
                        continue;
                    }
                }
                else
                {
                    if (matched)
                    {
                        if (need_destroy && parser_ops->destroy != NULL)
                        {
                            parser_ops->destroy(parser_ops);
                        }
                        node = node->next;
                        continue;
                    }
                }

                avdk_err_t ret = parser_ops->open(parser_ops, path_to_use);
                if (ret != AVDK_ERR_OK)
                {
                    last_open_err = ret;
                    if (need_destroy && parser_ops->destroy != NULL)
                    {
                        parser_ops->destroy(parser_ops);
                    }
                    node = node->next;
                    continue;
                }

                if (parser_ops->get_media_info != NULL)
                {
                    if (parser_ops->get_media_info(parser_ops, media_info) == AVDK_ERR_OK)
                    {
                        got_any = true;
                    }
                }

                // If we created an independent parser instance (need_destroy==true) and the player is not
                // playing/paused, keep the opened parser for later playback reuse. This avoids parsing
                // the same container twice when upper layer probes media info before playback.
                if (got_any && !is_playing_or_paused && need_destroy)
                {
                    char *dup_path = os_strdup(path_to_use);
                    if (dup_path != NULL)
                    {
                        video_player_prefetch_parser_clear_locked(controller);
                        controller->prefetch_container_parser = parser_ops;
                        controller->prefetch_file_path = dup_path;
                        controller->prefetch_media_info = *media_info;
                        // Ownership transferred: do not close/destroy here.
                    }
                    else
                    {
                        // Cannot cache without owning a copy of file path; close/destroy to avoid leaks.
                        (void)parser_ops->close(parser_ops);
                        if (parser_ops->destroy != NULL)
                        {
                            parser_ops->destroy(parser_ops);
                        }
                    }
                }
                else
                {
                    (void)parser_ops->close(parser_ops);
                    if (need_destroy && parser_ops->destroy != NULL)
                    {
                        parser_ops->destroy(parser_ops);
                    }
                }
                break;
            }
        }

        if (!got_any)
        {
            rtos_unlock_mutex(&controller->decoder_list_mutex);
            // If no parser can open the file, return the last open error for easier diagnosis.
            return last_open_err;
        }

        rtos_unlock_mutex(&controller->decoder_list_mutex);
    }

    // File size is best-effort for probe mode.
    if (!can_use_cached)
    {
        struct stat st;
        if (stat(path_to_use, &st) == 0 && st.st_size > 0)
        {
            media_info->file_size_bytes = (uint64_t)st.st_size;
        }
    }

    return AVDK_ERR_OK;
}

static avdk_err_t video_player_ctlr_get_status(bk_video_player_ctlr_handle_t handle, video_player_status_t *status)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(status, AVDK_ERR_INVAL, TAG, "status is NULL");

    private_video_player_ctlr_t *controller = __containerof(handle, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");

    *status = controller->module_status.status;
    return AVDK_ERR_OK;
}

static avdk_err_t video_player_ctlr_get_current_time(bk_video_player_ctlr_handle_t handle, uint64_t *time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(time_ms, AVDK_ERR_INVAL, TAG, "time_ms is NULL");

    private_video_player_ctlr_t *controller = __containerof(handle, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");

    *time_ms = video_player_get_current_time_ms(controller);

    return AVDK_ERR_OK;
}

static avdk_err_t video_player_ctlr_delete(bk_video_player_ctlr_handle_t handler)
{
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");

    video_player_status_t st = controller->module_status.status;

    /*
     * If the player is opened/playing/paused/stopped/finished, close it first to release runtime resources.
     * If the player is never opened (status NONE), close() is not allowed; just free registration lists.
     */
    if (st == VIDEO_PLAYER_STATUS_OPENED ||
        st == VIDEO_PLAYER_STATUS_PLAYING ||
        st == VIDEO_PLAYER_STATUS_PAUSED ||
        st == VIDEO_PLAYER_STATUS_STOPPED ||
        st == VIDEO_PLAYER_STATUS_FINISHED)
    {
        LOGW("%s: Player not closed, closing now before deletion\n", __func__);
        (void)video_player_ctlr_close(handler);
    }

    bk_video_player_audio_decoder_list_clear(controller);
    bk_video_player_video_decoder_list_clear(controller);
    bk_video_player_container_parser_list_clear(controller);

    // Deinitialize decoder list mutex last.
    rtos_deinit_mutex(&controller->decoder_list_mutex);

    os_free(controller);
    return AVDK_ERR_OK;
}

static avdk_err_t video_player_ctlr_ioctl(bk_video_player_ctlr_handle_t handler, bk_video_player_ioctl_cmd_t cmd, void *param)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_video_player_ctlr_t *controller = __containerof(handler, private_video_player_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");

    switch (cmd)
    {
        case BK_VIDEO_PLAYER_IOCTL_CMD_BASE:
            break;
        case BK_VIDEO_PLAYER_IOCTL_CMD_SET_AV_SYNC_OFFSET_MS:
        {
            AVDK_RETURN_ON_FALSE(param != NULL, AVDK_ERR_INVAL, TAG, "param is NULL");
            bk_video_player_av_sync_offset_param_t *p = (bk_video_player_av_sync_offset_param_t *)param;

            if (p->offset_ms > BK_VP_AV_SYNC_OFFSET_MS_MAX || p->offset_ms < -BK_VP_AV_SYNC_OFFSET_MS_MAX)
            {
                LOGE("%s: av_sync_offset_ms out of range: %d (range=[%d,%d])\n",
                     __func__, (int)p->offset_ms, -BK_VP_AV_SYNC_OFFSET_MS_MAX, BK_VP_AV_SYNC_OFFSET_MS_MAX);
                ret = AVDK_ERR_INVAL;
                break;
            }

            // time_mutex is initialized in open(). Reject before open() to keep mutex discipline.
            if (controller->time_mutex == NULL)
            {
                ret = AVDK_ERR_INVAL;
                break;
            }

            rtos_lock_mutex(&controller->time_mutex);
            controller->av_sync_offset_ms = p->offset_ms;
            /*
             * IMPORTANT:
             * av_sync_offset_ms is only applied when video syncs to the audio-driven clock.
             * If we previously fell back to VIDEO clock (e.g. audio-driven wait timeout),
             * then updating offset "works" but appears to have no effect.
             *
             * For interactive tuning (CLI avsync), switch back to AUDIO clock when audio is present
             * and not EOF, so that subsequent adjustments take effect immediately.
             */
            bool can_use_audio_clock = (controller->current_media_info.audio.channels > 0 &&
                                       controller->current_media_info.audio.sample_rate > 0 &&
                                       !controller->audio_eof_reached);
            if (can_use_audio_clock && controller->clock_source == VIDEO_PLAYER_CLOCK_VIDEO)
            {
                controller->clock_source = VIDEO_PLAYER_CLOCK_AUDIO;
                // Prevent immediate "stalled" detection after switching back.
                controller->last_time_update_tick_ms = rtos_get_time();
                LOGI("%s: Switch clock source back to AUDIO for avsync tuning\n", __func__);
            }
            rtos_unlock_mutex(&controller->time_mutex);

            LOGI("%s: Set av_sync_offset_ms=%d\n", __func__, (int)p->offset_ms);
            ret = AVDK_ERR_OK;
            break;
        }
    default:
        LOGE("%s: Unknown ioctl command: %d\n", __func__, cmd);
        ret = AVDK_ERR_INVAL;
        break;
    }

    return ret;
}

avdk_err_t bk_video_player_ctlr_new(bk_video_player_ctlr_handle_t *handle, bk_video_player_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    private_video_player_ctlr_t *controller = os_malloc(sizeof(private_video_player_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_video_player_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_video_player_config_t));

    // Video output format must be explicitly provided by upper layer.
    // It affects output buffer allocation size and decoder output pixel conversion behavior.
    if (controller->config.video.output_format == 0 || controller->config.video.output_format == PIXEL_FMT_UNKNOW)
    {
        LOGE("%s: video output format is not set (fmt=%d)\n", __func__, controller->config.video.output_format);
        os_free(controller);
        return AVDK_ERR_INVAL;
    }

    /*
     * Initialize decoder/parser registration mutex here, so that registration can be performed
     * before open(). open() will initialize runtime resources (pipelines, semaphores, etc.).
     */
    if (rtos_init_mutex(&controller->decoder_list_mutex) != BK_OK)
    {
        LOGE("%s: Failed to init decoder list mutex\n", __func__);
        os_free(controller);
        return AVDK_ERR_IO;
    }

    // Core layer always stops on EOF (no loop/next functionality)
    controller->eof_notified_session_id = 0;

    // Init playback finished notify thread only when upper layer provides callback.
    video_player_event_notify_init(controller);

    controller->ops.open = video_player_ctlr_open;
    controller->ops.close = video_player_ctlr_close;

    controller->ops.set_file_path = video_player_ctlr_set_file_path;
    controller->ops.play = video_player_ctlr_play;
    controller->ops.stop = video_player_ctlr_stop;
    controller->ops.set_pause = video_player_ctlr_set_pause;
    controller->ops.seek = video_player_ctlr_seek;
    controller->ops.fast_forward = video_player_ctlr_fast_forward;
    controller->ops.rewind = video_player_ctlr_rewind;

    controller->ops.register_audio_decoder = video_player_ctlr_register_audio_decoder;
    controller->ops.register_video_decoder = video_player_ctlr_register_video_decoder;
    controller->ops.register_container_parser = video_player_ctlr_register_container_parser;
    controller->ops.get_media_info = video_player_ctlr_get_media_info;
    controller->ops.get_status = video_player_ctlr_get_status;
    controller->ops.get_current_time = video_player_ctlr_get_current_time;
    controller->ops.delete_video_player = video_player_ctlr_delete;
    controller->ops.ioctl = video_player_ctlr_ioctl;
    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}
