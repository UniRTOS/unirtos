#include "os/os.h"
#include "os/mem.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "bk_video_player_pipeline.h"

#define TAG "pipeline"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static avdk_err_t pipeline_init_common(video_player_buffer_pool_t *parser_to_decode_pool,
                                       video_player_buffer_pool_t *decode_to_output_pool,
                                       beken_mutex_t *pts_mutex,
                                       uint32_t parser_to_decode_count,
                                       uint32_t decode_to_output_count,
                                       const char *log_name)
{
    if (parser_to_decode_pool == NULL || decode_to_output_pool == NULL || pts_mutex == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    avdk_err_t ret = buffer_pool_init(parser_to_decode_pool, parser_to_decode_count);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to init parser_to_decode_pool, ret=%d\n", log_name, ret);
        return ret;
    }

    ret = buffer_pool_init(decode_to_output_pool, decode_to_output_count);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to init decode_to_output_pool, ret=%d\n", log_name, ret);
        buffer_pool_deinit(parser_to_decode_pool);
        return ret;
    }

    ret = rtos_init_mutex(pts_mutex);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to init pts_mutex, ret=%d\n", log_name, ret);
        buffer_pool_deinit(parser_to_decode_pool);
        buffer_pool_deinit(decode_to_output_pool);
        return ret;
    }

    return AVDK_ERR_OK;
}

static void pipeline_deinit_common(video_player_buffer_pool_t *parser_to_decode_pool,
                                   video_player_buffer_pool_t *decode_to_output_pool,
                                   beken_mutex_t *pts_mutex)
{
    if (parser_to_decode_pool == NULL || decode_to_output_pool == NULL || pts_mutex == NULL)
    {
        return;
    }

    rtos_deinit_mutex(pts_mutex);
    buffer_pool_deinit(decode_to_output_pool);
    buffer_pool_deinit(parser_to_decode_pool);
}

avdk_err_t video_pipeline_init(video_pipeline_t *pipeline, uint32_t file_to_decode_buffer_count, uint32_t decode_to_output_buffer_count)
{
    AVDK_RETURN_ON_FALSE(pipeline, AVDK_ERR_INVAL, TAG, "pipeline is NULL");

    avdk_err_t ret = pipeline_init_common(&pipeline->parser_to_decode_pool,
                                          &pipeline->decode_to_output_pool,
                                          &pipeline->pts_mutex,
                                          file_to_decode_buffer_count,
                                          decode_to_output_buffer_count,
                                          __func__);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    pipeline->current_video_pts = 0;
    pipeline->next_video_frame_pts = 0; // Start from 0

    LOGI("%s: Video pipeline initialized, parser_to_decode=%u, decode_to_output=%u\n", 
         __func__, file_to_decode_buffer_count, decode_to_output_buffer_count);

    return AVDK_ERR_OK;
}

avdk_err_t video_pipeline_deinit(video_pipeline_t *pipeline)
{
    AVDK_RETURN_ON_FALSE(pipeline, AVDK_ERR_INVAL, TAG, "pipeline is NULL");

    pipeline_deinit_common(&pipeline->parser_to_decode_pool,
                           &pipeline->decode_to_output_pool,
                           &pipeline->pts_mutex);

    LOGI("%s: Video pipeline deinitialized\n", __func__);

    return AVDK_ERR_OK;
}

avdk_err_t video_player_audio_pipeline_init(audio_pipeline_t *pipeline, uint32_t file_to_decode_buffer_count, uint32_t decode_to_output_buffer_count)
{
    AVDK_RETURN_ON_FALSE(pipeline, AVDK_ERR_INVAL, TAG, "pipeline is NULL");

    avdk_err_t ret = pipeline_init_common(&pipeline->parser_to_decode_pool,
                                          &pipeline->decode_to_output_pool,
                                          &pipeline->pts_mutex,
                                          file_to_decode_buffer_count,
                                          decode_to_output_buffer_count,
                                          __func__);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    pipeline->current_audio_pts = 0;

    LOGI("%s: Audio pipeline initialized, parser_to_decode=%u, decode_to_output=%u\n", 
         __func__, file_to_decode_buffer_count, decode_to_output_buffer_count);

    return AVDK_ERR_OK;
}

avdk_err_t video_player_audio_pipeline_deinit(audio_pipeline_t *pipeline)
{
    AVDK_RETURN_ON_FALSE(pipeline, AVDK_ERR_INVAL, TAG, "pipeline is NULL");

    pipeline_deinit_common(&pipeline->parser_to_decode_pool,
                           &pipeline->decode_to_output_pool,
                           &pipeline->pts_mutex);

    LOGI("%s: Audio pipeline deinitialized\n", __func__);

    return AVDK_ERR_OK;
}

void video_pipeline_update_pts(video_pipeline_t *pipeline, uint64_t pts)
{
    if (pipeline == NULL)
    {
        return;
    }

    rtos_lock_mutex(&pipeline->pts_mutex);
    pipeline->current_video_pts = pts;
    rtos_unlock_mutex(&pipeline->pts_mutex);
}

uint64_t video_pipeline_get_pts(video_pipeline_t *pipeline)
{
    if (pipeline == NULL)
    {
        return 0;
    }

    uint64_t pts = 0;
    rtos_lock_mutex(&pipeline->pts_mutex);
    pts = pipeline->current_video_pts;
    rtos_unlock_mutex(&pipeline->pts_mutex);

    return pts;
}

void audio_pipeline_update_pts(audio_pipeline_t *pipeline, uint64_t pts)
{
    if (pipeline == NULL)
    {
        return;
    }

    rtos_lock_mutex(&pipeline->pts_mutex);
    pipeline->current_audio_pts = pts;
    rtos_unlock_mutex(&pipeline->pts_mutex);
}

uint64_t audio_pipeline_get_pts(audio_pipeline_t *pipeline)
{
    if (pipeline == NULL)
    {
        return 0;
    }

    uint64_t pts = 0;
    rtos_lock_mutex(&pipeline->pts_mutex);
    pts = pipeline->current_audio_pts;
    rtos_unlock_mutex(&pipeline->pts_mutex);

    return pts;
}

void video_pipeline_set_next_frame_pts(video_pipeline_t *pipeline, uint64_t pts)
{
    if (pipeline == NULL)
    {
        return;
    }

    rtos_lock_mutex(&pipeline->pts_mutex);
    pipeline->next_video_frame_pts = pts;
    rtos_unlock_mutex(&pipeline->pts_mutex);
}

uint64_t video_pipeline_get_next_frame_pts(video_pipeline_t *pipeline)
{
    if (pipeline == NULL)
    {
        return 0;
    }

    uint64_t pts = 0;
    rtos_lock_mutex(&pipeline->pts_mutex);
    pts = pipeline->next_video_frame_pts;
    rtos_unlock_mutex(&pipeline->pts_mutex);

    return pts;
}

