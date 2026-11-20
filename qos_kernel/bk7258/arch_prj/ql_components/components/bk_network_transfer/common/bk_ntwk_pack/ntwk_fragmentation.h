
#pragma once

#include <os/os.h>
#include <common/bk_include.h>
#include <common/bk_err.h>
#include "common/transfer_list/trans_list.h"

#include "network_transfer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DATA_POOL_LEN        CONFIG_DATA_POOL_LEN
#define DATA_NODE_SIZE      CONFIG_DATA_NODE_SIZE


typedef enum {
    FRAG_BUF_INIT = 0,    /**< Video buffer initialized */
    FRAG_BUF_COPY,        /**< Fragment buffer copying data */
    FRAG_BUF_GET,         /**< Fragment buffer frame received */
    FRAG_BUF_FULL,        /**< Fragment buffer full */
    FRAG_BUF_DEINIT,      /**< Fragment buffer deinitialized */
    FRAG_BUF_ERR,         /**< Fragment buffer error */
} fragmentation_state_t;


typedef struct
{
	uint8_t id;     /**< Frame ID (0-255) */
	uint8_t eof;    /**< End of frame flag (1 = last fragment, 0 = more fragments) */
	uint8_t cnt;    /**< Current fragment sequence number (1-based) */
	uint8_t size;   /**< Total number of fragments for this frame */
	uint8_t data[]; /**< Fragment payload data (variable length) */
} ntwk_fragm_head_t;

typedef struct {

	/// frame_buffer
	frame_buffer_t *frame;
	/// recoder the buff ptr of every time receive video packte
	uint8_t *buf_ptr;
	/// video buff receive state
	uint8_t start_buf;
	/// the packet count of one frame
	uint32_t frame_pkt_cnt;
} cache_buffer_t;

typedef struct {
	struct trans_list_hdr hdr;
	void *buf_start;
	uint32_t buf_len;
} data_elem_t;

typedef struct {
	beken_semaphore_t sem;
	uint8_t *pool;
	data_elem_t elem[DATA_POOL_LEN / DATA_NODE_SIZE];
	struct trans_list free;
	struct trans_list ready;
} data_pool_t;


typedef int (*fragment_recv_t) (chan_type_t chan, uint8_t *data, uint32_t length);

typedef struct
{
    ntwk_fragm_head_t *fragment_data; /**< Fragment buffer */
    uint32_t fragment_size;                   /**< Maximum fragment payload size */
    fragment_recv_t frag_recv;             /**< Send callback function */
	bool initialized;
} fragment_cfg_t;

typedef frame_buffer_t *(*unfragment_data_malloc_cb_t)(uint32_t size);
typedef bk_err_t (*unfragment_data_send_cb_t)(frame_buffer_t *data);
typedef bk_err_t (*unfragment_data_free_cb_t)(frame_buffer_t *data);

typedef struct
{
    uint8_t task_running;
    beken_thread_t thread;
    beken_semaphore_t sem;
    data_pool_t pool;
    cache_buffer_t cache_buf;
    unfragment_data_malloc_cb_t malloc_cb;
    unfragment_data_send_cb_t send_cb;
    unfragment_data_free_cb_t free_cb;
    uint32_t frame_size;
    uint32_t frame_cnt;
	bool initialized;
} unfragment_cfg_t;


bk_err_t ntwk_fragment_start(chan_type_t chan_type, uint32_t fragment_size, void *user_data);
bk_err_t ntwk_fragment_stop(chan_type_t chan_type);
bk_err_t ntwk_fragment_register_recv_cb(chan_type_t chan_type,fragment_recv_t cb);

int ntwk_fragment_ctrl_fragment(uint8_t *data, uint32_t length);
int ntwk_fragment_ctrl_unfragment(uint8_t *data, uint32_t length);

int ntwk_fragment_video_fragment(uint8_t *data, uint32_t length);
int ntwk_fragment_video_unfragment(uint8_t *data, uint32_t length);

int ntwk_fragment_audio_fragment(uint8_t *data, uint32_t length);
int ntwk_fragment_audio_unfragment(uint8_t *data, uint32_t length);

int ntwk_fragm_get_header_size(void);

bk_err_t ntwk_unfragment_start(chan_type_t chan_type, uint32_t frame_size, void *user_data);
bk_err_t ntwk_unfragment_stop(chan_type_t chan_type);
bk_err_t ntwk_unfragment_register_malloc_cb(chan_type_t chan_type, unfragment_data_malloc_cb_t cb);
bk_err_t ntwk_unfragment_register_send_cb(chan_type_t chan_type, unfragment_data_send_cb_t cb);
bk_err_t ntwk_unfragment_register_free_cb(chan_type_t chan_type, unfragment_data_free_cb_t cb);

bk_err_t ntwk_fragmentation_init(chan_type_t chan_type);
bk_err_t ntwk_fragmentation_deinit(chan_type_t chan_type);


#ifdef __cplusplus
}
#endif

