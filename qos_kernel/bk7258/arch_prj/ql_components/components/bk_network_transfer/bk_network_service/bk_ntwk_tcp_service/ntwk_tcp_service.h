#pragma once

#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "network_transfer.h"

#ifdef __cplusplus
extern "C" {
#endif


#define NTWK_TCP_BUFFER (1460)

// Video receive callback function type
typedef int (*ntwk_video_receive_cb_t)(uint8_t *data, uint32_t length);

// Audio receive callback function type
typedef int (*ntwk_audio_receive_cb_t)(uint8_t *data, uint32_t length);

typedef int (*ntwk_tcp_ctrl_receive_cb_t)(uint8_t *data, uint32_t length);

#if !CONFIG_NTWK_CLIENT_SERVICE_ENABLE
// TCP Server structures and functions
typedef struct
{
    uint32_t server_state : 1;
    struct sockaddr_in socket;
    beken_thread_t thread;
    int server_fd;
    int client_fd;
    in_addr_t remote_address;
    ntwk_tcp_ctrl_receive_cb_t receive_cb;  /**< User registered receive callback */
    ntwk_trans_chan_state chan_state;
} ntwk_tcp_ctrl_info_t;


typedef struct
{
    beken_thread_t video_thd;
    struct sockaddr_in video_remote;
    struct sockaddr_in video_socket;
    int video_fd;
    int video_server_fd;
    uint16_t video_status : 1;
    ntwk_trans_chan_state chan_state;
    ntwk_video_receive_cb_t receive_cb;
} video_tcp_service_t;

typedef struct
{
    beken_thread_t aud_thd;
    struct sockaddr_in aud_remote;
    struct sockaddr_in aud_socket;
    int aud_fd;
    int aud_server_fd;
    uint16_t aud_status : 1;
    ntwk_trans_chan_state chan_state;
    ntwk_audio_receive_cb_t receive_cb;
} aud_tcp_service_t;

in_addr_t ntwk_tcp_ctrl_get_socket_address(void);

bk_err_t ntwk_tcp_ctrl_chan_start(void *param);
bk_err_t ntwk_tcp_ctrl_chan_stop(void);

int ntwk_tcp_ctrl_chan_send(uint8_t *data, uint32_t length);

bk_err_t ntwk_tcp_ctrl_register_receive_cb(ntwk_tcp_ctrl_receive_cb_t cb);

bk_err_t ntwk_tcp_video_chan_start(void *param);
bk_err_t ntwk_tcp_video_chan_stop(void);
int ntwk_tcp_video_send_packet(uint8_t *data, uint32_t length, image_format_t video_type);

bk_err_t ntwk_tcp_audio_chan_start(void *param);
bk_err_t ntwk_tcp_audio_chan_stop(void);
int ntwk_tcp_audio_send_packet(uint8_t *data, uint32_t len, audio_enc_type_t audio_type);

// Register receive callback functions
bk_err_t ntwk_tcp_video_register_receive_cb(ntwk_video_receive_cb_t cb);
bk_err_t ntwk_tcp_audio_register_receive_cb(ntwk_audio_receive_cb_t cb);

bk_err_t ntwk_tcp_init(chan_type_t chan_type);
bk_err_t ntwk_tcp_deinit(chan_type_t chan_type);
#else // CONFIG_NTWK_CLIENT_SERVICE_ENABLE
// TCP Client functions
typedef struct
{
    uint32_t client_state : 1;
    struct sockaddr_in server_addr;
    beken_thread_t thread;
    int client_fd;
    in_addr_t server_address;
    uint16_t server_port;
    ntwk_tcp_ctrl_receive_cb_t receive_cb;
    ntwk_trans_chan_state chan_state;
} ntwk_tcp_ctrl_client_info_t;

typedef struct
{
    beken_thread_t video_thd;
    struct sockaddr_in server_addr;
    int video_fd;
    in_addr_t server_address;
    uint16_t server_port;
    uint16_t video_status : 1;
    ntwk_trans_chan_state chan_state;
    ntwk_video_receive_cb_t receive_cb;
} video_tcp_client_service_t;

typedef struct
{
    beken_thread_t aud_thd;
    struct sockaddr_in server_addr;
    int aud_fd;
    in_addr_t server_address;
    uint16_t server_port;
    uint16_t aud_status : 1;
    ntwk_trans_chan_state chan_state;
    ntwk_audio_receive_cb_t receive_cb;
} aud_tcp_client_service_t;

// TCP Client control channel functions
bk_err_t ntwk_tcp_ctrl_client_chan_start(void *param);
bk_err_t ntwk_tcp_ctrl_client_chan_stop(void);
int ntwk_tcp_ctrl_client_chan_send(uint8_t *data, uint32_t length);
bk_err_t ntwk_tcp_ctrl_client_register_receive_cb(ntwk_tcp_ctrl_receive_cb_t cb);

// TCP Client video channel functions
bk_err_t ntwk_tcp_video_client_chan_start(void *param);
bk_err_t ntwk_tcp_video_client_chan_stop(void);
int ntwk_tcp_video_client_send_packet(uint8_t *data, uint32_t length, image_format_t video_type);
bk_err_t ntwk_tcp_video_client_register_receive_cb(ntwk_video_receive_cb_t cb);

// TCP Client audio channel functions
bk_err_t ntwk_tcp_audio_client_chan_start(void *param);
bk_err_t ntwk_tcp_audio_client_chan_stop(void);
int ntwk_tcp_audio_client_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type);
bk_err_t ntwk_tcp_audio_client_register_receive_cb(ntwk_audio_receive_cb_t cb);

// TCP Client init/deinit
bk_err_t ntwk_tcp_client_init(chan_type_t chan_type);
bk_err_t ntwk_tcp_client_deinit(chan_type_t chan_type);
#endif // CONFIG_NTWK_CLIENT_SERVICE_ENABLE

#ifdef __cplusplus
}
#endif
