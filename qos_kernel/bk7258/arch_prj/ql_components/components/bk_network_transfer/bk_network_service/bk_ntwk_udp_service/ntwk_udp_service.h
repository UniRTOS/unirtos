#pragma once
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "network_transfer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_DEMO_UDP_SOCKET_TIMEOUT     100  // ms
#define UDP_MAX_RETRY (1000)
#define UDP_MAX_DELAY (20)

// Video receive callback function type
typedef int (*ntwk_udp_video_receive_cb_t)(uint8_t *data, uint32_t length);

// Audio receive callback function type
typedef int (*ntwk_udp_audio_receive_cb_t)(uint8_t *data, uint32_t length);

typedef int (*ntwk_udp_ctrl_receive_cb_t)(uint8_t *data, uint32_t length);

#if !CONFIG_NTWK_CLIENT_SERVICE_ENABLE
// UDP Server structures and functions
typedef struct
{
    uint32_t server_state : 1;
    struct sockaddr_in socket;
    beken_thread_t thread;
    int server_fd;
    int client_fd;
    in_addr_t remote_address;
    ntwk_udp_ctrl_receive_cb_t receive_cb;  /**< User registered receive callback */
    ntwk_trans_chan_state chan_state;
} ntwk_udp_ctrl_info_t;

typedef struct
{
    beken_thread_t thd;
    struct sockaddr_in video_remote;
    int video_fd;
    uint16_t video_status : 1;
    ntwk_trans_chan_state chan_state;
    ntwk_udp_video_receive_cb_t receive_cb;
} video_udp_service_t;

typedef struct
{
    beken_thread_t thd;
    struct sockaddr_in aud_remote;
    int aud_fd;
    uint16_t aud_status : 1;
    ntwk_trans_chan_state chan_state;
    ntwk_udp_audio_receive_cb_t receive_cb;
} aud_udp_service_t;


bk_err_t ntwk_udp_ctrl_chan_start(void *param);
bk_err_t ntwk_udp_ctrl_chan_stop(void);

int ntwk_udp_ctrl_chan_send(uint8_t *data, uint32_t length);

bk_err_t ntwk_udp_ctrl_register_receive_cb(ntwk_udp_ctrl_receive_cb_t cb);

bk_err_t ntwk_udp_video_chan_start(void *param);
bk_err_t ntwk_udp_video_chan_stop(void);
int ntwk_udp_video_send_packet(uint8_t *data, uint32_t length, image_format_t video_type);

bk_err_t ntwk_udp_audio_chan_start(void *param);
bk_err_t ntwk_udp_audio_chan_stop(void);

int ntwk_udp_audio_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type);

// Register receive callback functions
bk_err_t ntwk_udp_video_register_receive_cb(ntwk_udp_video_receive_cb_t cb);
bk_err_t ntwk_udp_audio_register_receive_cb(ntwk_udp_audio_receive_cb_t cb);

bk_err_t ntwk_udp_init(chan_type_t chan_type);
bk_err_t ntwk_udp_deinit(chan_type_t chan_type);
#else // CONFIG_NTWK_CLIENT_SERVICE_ENABLE
// UDP Client functions
typedef struct
{
    uint32_t client_state : 1;
    struct sockaddr_in server_addr;
    beken_thread_t thread;
    int client_fd;
    in_addr_t server_address;
    uint16_t server_port;
    ntwk_udp_ctrl_receive_cb_t receive_cb;
    ntwk_trans_chan_state chan_state;
} ntwk_udp_ctrl_client_info_t;

typedef struct
{
    beken_thread_t thd;
    struct sockaddr_in server_addr;
    int video_fd;
    in_addr_t server_address;
    uint16_t server_port;
    uint16_t video_status : 1;
    ntwk_trans_chan_state chan_state;
    ntwk_udp_video_receive_cb_t receive_cb;
} video_udp_client_service_t;

typedef struct
{
    beken_thread_t thd;
    struct sockaddr_in server_addr;
    int aud_fd;
    in_addr_t server_address;
    uint16_t server_port;
    uint16_t aud_status : 1;
    ntwk_trans_chan_state chan_state;
    ntwk_udp_audio_receive_cb_t receive_cb;
} aud_udp_client_service_t;

// UDP Client control channel functions
bk_err_t ntwk_udp_ctrl_client_chan_start(void *param);
bk_err_t ntwk_udp_ctrl_client_chan_stop(void);
int ntwk_udp_ctrl_client_chan_send(uint8_t *data, uint32_t length);
bk_err_t ntwk_udp_ctrl_client_register_receive_cb(ntwk_udp_ctrl_receive_cb_t cb);

// UDP Client video channel functions
bk_err_t ntwk_udp_video_client_chan_start(void *param);
bk_err_t ntwk_udp_video_client_chan_stop(void);
int ntwk_udp_video_client_send_packet(uint8_t *data, uint32_t length, image_format_t video_type);
bk_err_t ntwk_udp_video_client_register_receive_cb(ntwk_udp_video_receive_cb_t cb);

// UDP Client audio channel functions
bk_err_t ntwk_udp_audio_client_chan_start(void *param);
bk_err_t ntwk_udp_audio_client_chan_stop(void);
int ntwk_udp_audio_client_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type);
bk_err_t ntwk_udp_audio_client_register_receive_cb(ntwk_udp_audio_receive_cb_t cb);

// UDP Client init/deinit
bk_err_t ntwk_udp_client_init(chan_type_t chan_type);
bk_err_t ntwk_udp_client_deinit(chan_type_t chan_type);
#endif // CONFIG_NTWK_CLIENT_SERVICE_ENABLE

#ifdef __cplusplus
}
#endif
