
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>

#include "ntwk_fragmentation.h"
#include "network_transfer.h"
#include "network_transfer_internal.h"

#define TAG "ntwk-fragm"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define NTWK_FRAG_HEADER_SIZE  (sizeof(ntwk_fragm_head_t))

static fragment_cfg_t *s_fragment_cfg_mgr[NTWK_TRANS_CHAN_MAX] = {NULL};
static unfragment_cfg_t *s_unfragment_cfg_mgr[NTWK_TRANS_CHAN_MAX] = {NULL};

int ntwk_fragm_get_header_size(void)
{
    return sizeof(ntwk_fragm_head_t);
}

bk_err_t ntwk_fragmentation_init(chan_type_t chan_type)
{
    if (s_fragment_cfg_mgr[chan_type] == NULL)
    {
        s_fragment_cfg_mgr[chan_type] = (fragment_cfg_t *)ntwk_malloc(sizeof(fragment_cfg_t));

        if (s_fragment_cfg_mgr[chan_type] == NULL)
        {
            LOGE("%s, malloc fragment_cfg_mgr[chan_type] fail\n", __func__);
            return BK_ERR_NO_MEM;
        }

        os_memset(s_fragment_cfg_mgr[chan_type], 0, sizeof(fragment_cfg_t));
    }

    if (s_unfragment_cfg_mgr[chan_type] == NULL)
    {
        s_unfragment_cfg_mgr[chan_type] = (unfragment_cfg_t *)ntwk_malloc(sizeof(unfragment_cfg_t));
        if (s_unfragment_cfg_mgr[chan_type] == NULL)
        {
            LOGE("%s, malloc s_unfragment_cfg_mgr[chan_type] fail\n", __func__);
            return BK_FAIL;
        }
    
        os_memset(s_unfragment_cfg_mgr[chan_type], 0, sizeof(unfragment_cfg_t));
    }

    return BK_OK;
}

bk_err_t ntwk_fragmentation_deinit(chan_type_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    if ((s_fragment_cfg_mgr[chan_type] != NULL) && (s_fragment_cfg_mgr[chan_type]->initialized == false))
    {
        os_free(s_fragment_cfg_mgr[chan_type]);
        s_fragment_cfg_mgr[chan_type] = NULL;
    }

    if ((s_unfragment_cfg_mgr[chan_type] != NULL) && (s_unfragment_cfg_mgr[chan_type]->initialized == false))
    {
        os_free(s_unfragment_cfg_mgr[chan_type]);
        s_unfragment_cfg_mgr[chan_type] = NULL;
    }

    return BK_OK;
}

bk_err_t ntwk_fragment_start(chan_type_t chan_type, uint32_t fragment_size, void *user_data)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) 
    {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

	if (s_fragment_cfg_mgr[chan_type] == NULL)
	{
		LOGW("%s, fragment not started\n", __func__);
		return BK_FAIL;
	}
    s_fragment_cfg_mgr[chan_type]->fragment_data = (ntwk_fragm_head_t *)ntwk_malloc(NTWK_FRAG_HEADER_SIZE + fragment_size);
    if (s_fragment_cfg_mgr[chan_type]->fragment_data == NULL)
    {
        LOGE("%s, malloc failed\n", __func__);
        return BK_ERR_NO_MEM;
    }

    s_fragment_cfg_mgr[chan_type]->fragment_size = fragment_size;
	s_fragment_cfg_mgr[chan_type]->initialized = true;

	LOGV("%s, end\n", __func__);

	return BK_OK;
}

bk_err_t ntwk_fragment_stop(chan_type_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

	if (s_fragment_cfg_mgr[chan_type] == NULL)
	{
		LOGW("%s, fragmentation not started\n", __func__);
		return BK_FAIL;
	}

	if (s_fragment_cfg_mgr[chan_type]->fragment_data)
	{
		os_free(s_fragment_cfg_mgr[chan_type]->fragment_data);
		s_fragment_cfg_mgr[chan_type]->fragment_data = NULL;
	}

	s_fragment_cfg_mgr[chan_type]->initialized = false;

	LOGV("%s, stopped\n", __func__);

	return BK_OK;
}

bk_err_t ntwk_fragment_register_recv_cb(chan_type_t chan_type, fragment_recv_t cb)
{
	if (s_fragment_cfg_mgr[chan_type] == NULL)
	{
		LOGE("%s, fragment not started\n", __func__);
		return BK_FAIL;
	}

	s_fragment_cfg_mgr[chan_type]->frag_recv = cb;

	LOGV("%s, callback registered\n", __func__);

	return BK_OK;
}

int ntwk_fragment(chan_type_t chan_type, uint8_t *data, uint32_t length)
{
    int ret = BK_OK;
    uint32_t i;
    frame_buffer_t *buffer = NULL;

    if (data == NULL || length == 0)
	{
		LOGE("%s, invalid parameters\n", __func__);
		return -3;
	}

	if ((s_fragment_cfg_mgr[chan_type] == NULL) || (s_fragment_cfg_mgr[chan_type]->initialized == false))
	{
		return -1;
	}

	if (s_fragment_cfg_mgr[chan_type]->frag_recv == NULL)
	{
		LOGE("%s, send callback not registered\n", __func__);
		return -4;
	}

    buffer = (frame_buffer_t *)data;

	uint32_t fragment_size = s_fragment_cfg_mgr[chan_type]->fragment_size;
	uint32_t count = length / fragment_size;
	uint32_t tail = length % fragment_size;
	ntwk_fragm_head_t *frag_hdr = s_fragment_cfg_mgr[chan_type]->fragment_data;
    uint8_t *src_address = buffer->frame;

	LOGV("%s, length: %u, fragment_size: %u, count: %u, tail: %u\n", 
		__func__, length, fragment_size, count, tail);

	// Set common header fields
	frag_hdr->id = (buffer->sequence & 0xFF);
    frag_hdr->cnt = 0;
    frag_hdr->eof = 0;
	frag_hdr->size = count + (tail ? 1 : 0);

	// Send full-size fragments
	for (i = 0; i < count; i++)
	{
		frag_hdr->cnt = i + 1;

		if ((tail == 0) && (i == count - 1))
		{
			frag_hdr->eof = 1;
		}
		// Copy data to fragment buffer
		os_memcpy_word((uint32_t *)frag_hdr->data, (uint32_t *)(src_address + (fragment_size * i)),fragment_size);

        LOGV("seq: %d [%d %d %d]\n", buffer->sequence,frag_hdr->id,frag_hdr->eof,frag_hdr->cnt);

		// Send fragment
		ret = s_fragment_cfg_mgr[chan_type]->frag_recv(chan_type, (uint8_t *)frag_hdr, NTWK_FRAG_HEADER_SIZE + fragment_size);
		if (ret < 0)
		{
			LOGE("%s, send fragment %u failed, ret: %d\n", __func__, i, ret);
			return -5;
		}
	}

	// Send tail fragment if exists
	if (tail)
	{
		frag_hdr->cnt = count + 1;
		frag_hdr->eof = 1;

        os_memcpy_word((uint32_t *)frag_hdr->data, (uint32_t *)(src_address + (fragment_size * i)), (tail % 4) ? ((tail / 4 + 1) * 4) : tail);

        LOGV("seq: %d [%d %d %d]\n", buffer->sequence,frag_hdr->id,frag_hdr->eof,frag_hdr->cnt);

		ret = s_fragment_cfg_mgr[chan_type]->frag_recv(chan_type, (uint8_t *)frag_hdr, NTWK_FRAG_HEADER_SIZE + tail);
		if (ret < 0)
		{
			LOGE("%s, send tail fragment failed, ret: %d\n", __func__, ret);
			return -6;
		}
	}

	LOGV("%s, fragment completed, total fragments: %u\n", __func__, length);

	return length;
}

static bk_err_t unfragment_config_mem_free(unfragment_cfg_t *config)
{
    if (config)
    {
        if (config->pool.pool)
        {
            os_free(config->pool.pool);
            config->pool.pool = NULL;
        }

        //释放信号量
        if (config->pool.sem)
        {
            rtos_deinit_semaphore(&config->pool.sem);
        }

        if (config->cache_buf.frame)
        {
            if (config->free_cb)
            {
                config->free_cb(config->cache_buf.frame);
            }
            config->cache_buf.frame = NULL;
        }

        //释放信号量
        if (config->sem)
        {
            rtos_deinit_semaphore(&config->sem);
        }
    }

    return BK_OK;
}

static bk_err_t data_pool_init(data_pool_t *pool)
{
    //初始化信号量
    bk_err_t ret = rtos_init_semaphore(&pool->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, init pool->sem fail, ret = %d\n", __func__, ret);
        return ret;
    }

    if (pool->pool == NULL)
    {
        pool->pool = (uint8_t *)ntwk_packet_malloc(DATA_POOL_LEN);
        if (pool->pool == NULL)
        {
            LOGE("data_pool alloc failed\r\n");
            rtos_deinit_semaphore(&pool->sem);
            return BK_ERR_NO_MEM;
        }

        //os_memset(pool->pool, 0, DATA_POOL_LEN);
    }

    trans_list_init(&pool->free);
    trans_list_init(&pool->ready);

    for (uint8_t i = 0; i < (DATA_POOL_LEN / DATA_NODE_SIZE); i++) {
        pool->elem[i].buf_start =
            (void *)&pool->pool[i * DATA_NODE_SIZE];
        pool->elem[i].buf_len = 0;

        trans_list_push_back(&pool->free,
            (struct trans_list_hdr *)&pool->elem[i].hdr);
    }

    return ret;
}

static void unfragment_process_packet(unfragment_cfg_t *config, uint8_t *data, uint32_t length)
{
    if (config->task_running == 0)
    {
        return;
    }

    cache_buffer_t *frame_buffer = &config->cache_buf;

    if ((frame_buffer->start_buf == FRAG_BUF_INIT || frame_buffer->start_buf == FRAG_BUF_COPY) && frame_buffer->frame)
    {
        ntwk_fragm_head_t *hdr = (ntwk_fragm_head_t *)data;
        uint32_t org_len;
        GLOBAL_INT_DECLARATION();

        org_len = length - sizeof(ntwk_fragm_head_t);
        data = data + sizeof(ntwk_fragm_head_t);

        LOGV("id:%d eof %d cnt %d size %d len %d org_len %d\r\n", hdr->id,hdr->eof,hdr->cnt,hdr->size,length,org_len);

        if (hdr->cnt == 1) {
            frame_buffer->frame->length = 0;
            frame_buffer->frame_pkt_cnt = 0;
            frame_buffer->buf_ptr = frame_buffer->frame->frame;
            frame_buffer->start_buf = FRAG_BUF_COPY;
            LOGV("sof:%d\r\n", frame_buffer->frame->sequence);
        }
        else
        {
            if (frame_buffer->start_buf == FRAG_BUF_INIT)
                frame_buffer->start_buf = FRAG_BUF_COPY;
        }

       /* LOGD("hdr-id:%d-%d, frame_packet_cnt:%d-%d, state:%d\r\n", hdr->id, config->frame->sequence,
            (cache_buffer->frame_pkt_cnt + 1), hdr->cnt, cache_buffer->start_buf); */

        if (((frame_buffer->frame_pkt_cnt + 1) == hdr->cnt)
            && (frame_buffer->start_buf == FRAG_BUF_COPY))
        {
            if (frame_buffer->frame->length + org_len > frame_buffer->frame->size)
            {
                frame_buffer->frame->length += org_len;
                frame_buffer->frame_pkt_cnt += 1;
                if (hdr->eof == 1)
                {
                    LOGE("%s transfer_length %d is over cache buf size %d \r\n", __func__, frame_buffer->frame->length, frame_buffer->frame->size);
                    frame_buffer->buf_ptr = frame_buffer->frame->frame;
                    frame_buffer->frame->length = 0;
                    frame_buffer->frame->sequence = config->frame_cnt++;
                }
                return;
            }

            os_memcpy(frame_buffer->buf_ptr, data, org_len);

            GLOBAL_INT_DISABLE();
            frame_buffer->frame->length += org_len;
            frame_buffer->buf_ptr += org_len;
            frame_buffer->frame_pkt_cnt += 1;
            GLOBAL_INT_RESTORE();

            if (hdr->eof == 1)
            {
                frame_buffer_t *new_frame = NULL;
                
                new_frame = config->malloc_cb(config->frame_size);

                if (new_frame)
                {
                    config->send_cb(frame_buffer->frame);
                    frame_buffer->frame = new_frame;
                }
                else
                {
                    LOGV("frame buffer malloc failed\r\n");
                }

                frame_buffer->buf_ptr = frame_buffer->frame->frame;
                frame_buffer->frame->length = 0;
                frame_buffer->frame->sequence = config->frame_cnt++;
            }
        }
    }
    else
    {
        if (config && config->malloc_cb)
        {
            config->cache_buf.frame = config->malloc_cb(config->frame_size);
            if (config->cache_buf.frame == NULL)
            {
                LOGE("%s, malloc frame failed\n", __func__);
                return;
            }

            config->cache_buf.buf_ptr = config->cache_buf.frame->frame;
            config->cache_buf.frame->length = 0;
            config->cache_buf.frame->sequence = config->frame_cnt++;
            config->cache_buf.start_buf = FRAG_BUF_INIT;
        }
    }
}

static void unfragment_process_task_entry(beken_thread_arg_t data)
{
    unfragment_cfg_t *unfrag_config = (unfragment_cfg_t *)data;
    data_elem_t *elem = NULL;
    bk_err_t err = 0;

    data_pool_t *pool = &unfrag_config->pool;
    unfrag_config->task_running = true;
    rtos_set_semaphore(&unfrag_config->sem);

    while (unfrag_config->task_running)
    {
        err = rtos_get_semaphore(&pool->sem, 1000);
        if(!unfrag_config->task_running)
        {
            break;
        }

        if(err != 0)
        {
            LOGV("%s get sem timeout\n", __func__);
            continue;
        }

        while((elem = (data_elem_t *)trans_list_pick(&pool->ready)) != NULL)
        {
            unfragment_process_packet(unfrag_config, elem->buf_start, elem->buf_len);

            trans_list_pop_front(&pool->ready);
            trans_list_push_back(&pool->free, (struct trans_list_hdr *)&elem->hdr);
        }
    };

    unfrag_config->thread = NULL;
    rtos_set_semaphore(&unfrag_config->sem);
    rtos_delete_thread(NULL);
}


bk_err_t ntwk_unfragment_start(chan_type_t chan_type, uint32_t frame_size, void *user_data)
{
    bk_err_t ret = BK_OK;

    if (s_unfragment_cfg_mgr[chan_type] == NULL)
    {
        LOGW("%s, unfragment already open\n", __func__);
        return BK_FAIL;
    }

    s_unfragment_cfg_mgr[chan_type]->initialized = true;
	s_unfragment_cfg_mgr[chan_type]->frame_size = frame_size;

    ret = data_pool_init(&s_unfragment_cfg_mgr[chan_type]->pool);
    if (ret != BK_OK)
    {
        LOGE("%s, data_pool_init fail, ret = %d\n", __func__, ret);
        goto error;
    }

    //创建信号量
    ret = rtos_init_semaphore(&s_unfragment_cfg_mgr[chan_type]->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, init s_unfragment_cfg->sem fail, ret = %d\n", __func__, ret);
        goto error;
    }

    //创建接收线程
    ret = rtos_create_thread(&s_unfragment_cfg_mgr[chan_type]->thread,
                                6,
                                "unfragment_task",
                                (beken_thread_function_t)unfragment_process_task_entry,
                                4 * 1024,
                                s_unfragment_cfg_mgr[chan_type]);
    if (ret != BK_OK)
    {
        LOGE("%s, create unfragment_task fail, ret = %d\n", __func__, ret);
        goto error;
    }

    rtos_get_semaphore(&s_unfragment_cfg_mgr[chan_type]->sem, BEKEN_WAIT_FOREVER);

    return ret;

error:

    LOGE("%s, failed!, ret = %d\n", __func__, ret);
    unfragment_config_mem_free(s_unfragment_cfg_mgr[chan_type]);
    s_unfragment_cfg_mgr[chan_type] = NULL;
    return ret;
}

bk_err_t ntwk_unfragment_stop(chan_type_t chan_type)
{
    bk_err_t ret = BK_OK;
    LOGV("%s chan_type %d\r\n", __func__,chan_type);

    if (s_unfragment_cfg_mgr[chan_type] == NULL)
    {
        LOGE("%s, video data process not open\n", __func__);
        return BK_FAIL;
    }

    if (s_unfragment_cfg_mgr[chan_type]->initialized == false)
    {
        LOGE("%s, task not running\n", __func__);
        return BK_FAIL;
    }

    if (s_unfragment_cfg_mgr[chan_type]->cache_buf.start_buf == FRAG_BUF_COPY)
    {
        s_unfragment_cfg_mgr[chan_type]->cache_buf.start_buf = FRAG_BUF_DEINIT;
    }

    s_unfragment_cfg_mgr[chan_type]->task_running = false;
    rtos_get_semaphore(&s_unfragment_cfg_mgr[chan_type]->sem, BEKEN_WAIT_FOREVER);
    
    unfragment_config_mem_free(s_unfragment_cfg_mgr[chan_type]);
    
    s_unfragment_cfg_mgr[chan_type]->malloc_cb = NULL;
    s_unfragment_cfg_mgr[chan_type]->send_cb = NULL;
    s_unfragment_cfg_mgr[chan_type]->free_cb = NULL;
    s_unfragment_cfg_mgr[chan_type]->frame_size = 0;

    s_unfragment_cfg_mgr[chan_type]->initialized = false;

    return ret;
}

bk_err_t ntwk_unfragment(uint32_t chan_type, uint8_t *data, uint32_t length)
{
    data_elem_t *elem = NULL;

    unfragment_cfg_t *config = s_unfragment_cfg_mgr[chan_type];

    if ((config == NULL) || (config->initialized == false) || (config->task_running == false))
    {
        LOGE("unfr uninit type %d\n", chan_type);
        return -1;
    }

    data_pool_t *pool = &config->pool;

    if (length <= 4)
    {
        //LOGE("%s, length is too short\n", __func__);
        return -1;
    }

    elem = (data_elem_t *)trans_list_pick(&pool->free);
    if (elem)
    {
        os_memcpy(elem->buf_start, data, length);
        elem->buf_len = length;
        trans_list_pop_front(&pool->free);
        trans_list_push_back(&pool->ready, (struct trans_list_hdr *)&elem->hdr);
        rtos_set_semaphore(&pool->sem);
    }
    else
    {
        LOGD("list all busy\r\n");
    }

    return BK_OK;
}

bk_err_t ntwk_unfragment_register_malloc_cb(chan_type_t chan_type, unfragment_data_malloc_cb_t cb)
{
    if (cb == NULL)
    {
        LOGE("%s: callback is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    if (s_unfragment_cfg_mgr[chan_type] == NULL)
    {
        LOGE("%s: not initialized\n", __func__);
        return BK_FAIL;
    }

    s_unfragment_cfg_mgr[chan_type]->malloc_cb = cb;

    return BK_OK;
}

bk_err_t ntwk_unfragment_register_send_cb(chan_type_t chan_type, unfragment_data_send_cb_t cb)
{
    if (cb == NULL)
    {
        LOGE("%s: callback is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    if (s_unfragment_cfg_mgr[chan_type] == NULL)
    {
        LOGE("%s: not initialized\n", __func__);
        return BK_FAIL;
    }

    s_unfragment_cfg_mgr[chan_type]->send_cb = cb;

    return BK_OK;
}

bk_err_t ntwk_unfragment_register_free_cb(chan_type_t chan_type, unfragment_data_free_cb_t cb)
{
    if (cb == NULL)
    {
        LOGE("%s: callback is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    if (s_unfragment_cfg_mgr[chan_type] == NULL)
    {
        LOGE("%s: not initialized\n", __func__);
        return BK_FAIL;
    }

    s_unfragment_cfg_mgr[chan_type]->free_cb = cb;

    return BK_OK;
}

int ntwk_fragment_ctrl_fragment(uint8_t *data, uint32_t length)
{
    return ntwk_fragment(NTWK_TRANS_CHAN_CTRL, data, length);
}
int ntwk_fragment_ctrl_unfragment(uint8_t *data, uint32_t length)
{
    return ntwk_unfragment(NTWK_TRANS_CHAN_CTRL, data, length);
}
int ntwk_fragment_video_fragment(uint8_t *data, uint32_t length)
{
    return ntwk_fragment(NTWK_TRANS_CHAN_VIDEO, data, length);
}
int ntwk_fragment_video_unfragment(uint8_t *data, uint32_t length)
{
    return ntwk_unfragment(NTWK_TRANS_CHAN_VIDEO, data, length);
}
int ntwk_fragment_audio_fragment(uint8_t *data, uint32_t length)
{
    return ntwk_fragment(NTWK_TRANS_CHAN_AUDIO, data, length);
}
int ntwk_fragment_audio_unfragment(uint8_t *data, uint32_t length)
{
    return ntwk_unfragment(NTWK_TRANS_CHAN_AUDIO,data, length);
}