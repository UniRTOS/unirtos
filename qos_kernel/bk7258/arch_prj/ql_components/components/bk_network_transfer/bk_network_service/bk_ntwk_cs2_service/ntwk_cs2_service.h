#pragma once

#include "lwip/sockets.h"
#include "PPCS_cs2_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THROUGHPUT_DEBUG 1

#define NTWK_CS2_RECV_TMP_BUFF_SIZE   (1472)
#define RECV_TMP_CMD_BUFF_SIZE   (200)

#if THROUGHPUT_DEBUG
#define THROUGHPUT_ANLAYSE_MS 2000
#endif

#define CS2_P2P_TRANSFER_DELAY 10

typedef enum
{
	NTWK_TURN_OFF,
	NTWK_TURN_ON,
} ntwk_cs2_state_t;

// Audio receive callback function type
typedef int (*ntwk_audio_receive_cb_t)(uint8_t *data, uint32_t length);

/**
 * @brief User registered callback for receiving control data
 */
typedef int (*ntwk_cs2_ctrl_receive_cb_t)(uint8_t *data, uint32_t length);

// Video receive callback function type
typedef int (*ntwk_video_receive_cb_t)(uint8_t *data, uint32_t length);

typedef struct
{
    uint8_t is_running;
    uint8_t aud_running;
    beken_mutex_t tx_lock;

    uint16_t video_status : 1;
    uint16_t aud_status : 1;
    uint16_t device_connected : 1;

    beken_mutex_t mutex;

    beken_thread_t thd;
    p2p_cs2_key_t *cs2_key;

    beken_semaphore_t aud_sem;
    uint32_t video_server_state : 1;
    uint32_t audio_server_state : 1;

    ntwk_cs2_ctrl_receive_cb_t ctrl_receive_cb;  /**< User registered receive callback */
    ntwk_video_receive_cb_t video_receive_cb;
    ntwk_audio_receive_cb_t audio_receive_cb;
} ntwk_cs2_info_t;

int ntwk_cs2_p2p_write(int SessionID, uint8_t Channel, uint8_t *buff, uint32_t size);
void ntwk_cs2_video_timer_deinit(void);

bk_err_t ntwk_cs2_get_current_write_size(uint32_t *write_size);

bk_err_t ntwk_cs2_ctrl_chan_start(void *param);
bk_err_t ntwk_cs2_ctrl_chan_stop(void);
int ntwk_cs2_p2p_ctrl_send(uint8_t *data, uint32_t length);
bk_err_t ntwk_cs2_ctrl_register_receive_cb(ntwk_cs2_ctrl_receive_cb_t cb);


bk_err_t ntwk_cs2_video_chan_start(void *param);
bk_err_t ntwk_cs2_video_chan_stop(void);
int ntwk_cs2_video_send_packet(uint8_t *data, uint32_t length, image_format_t video_type);
bk_err_t ntwk_cs2_video_register_receive_cb(ntwk_video_receive_cb_t cb);

bk_err_t ntwk_cs2_audio_chan_start(void *param);
bk_err_t ntwk_cs2_audio_chan_stop(void);
int ntwk_cs2_audio_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type);
bk_err_t ntwk_cs2_audio_register_receive_cb(ntwk_audio_receive_cb_t cb);

bk_err_t ntwk_cs2_init(void);
bk_err_t ntwk_cs2_deinit(void);

#ifdef __cplusplus
}
#endif
