#include <common/bk_include.h>

#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>
#include <stdlib.h>
#ifdef CONFIG_RTT
#include <sys/socket.h>
#endif
#include "lwip/sockets.h"

#include "bk_uart.h"
#include <os/mem.h>


#include "network_type.h"
#include "bk_network_service/bk_ntwk_socket/ntwk_socket.h"
#include "network_transfer.h"
#include "ntwk_udp_service.h"
#include "network_transfer_internal.h"

#define TAG "ntwk-UDP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// TCP keepalive function (shared by both server and client)
static int ntwk_udp_cntrl_set_keepalive(int sockfd,  int keepalive, int keepidle, int keepintvl, int keepcnt)
{
	int ret;

	// Enable SO_KEEPALIVE
	ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
	if (ret < 0)
	{
		LOGE("setsockopt SO_KEEPALIVE failed: %d\n", errno);
		return ret;
	}

	// Set keepalive idle time
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
	if (ret < 0)
	{
		LOGW("setsockopt TCP_KEEPIDLE failed: %d\n", errno);
	}

	// Set keepalive probe interval
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
	if (ret < 0)
	{
		LOGW("setsockopt TCP_KEEPINTVL failed: %d\n", errno);
	}

	// Set keepalive probe count
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));
	if (ret < 0)
	{
		LOGW("setsockopt TCP_KEEPCNT failed: %d\n", errno);
	}

	LOGV("TCP keepalive enabled: idle=%ds, interval=%ds, count=%d\n", 
		keepidle, keepintvl, keepcnt);

	return BK_OK;
}

#if !CONFIG_NTWK_CLIENT_SERVICE_ENABLE
// UDP Server global variables
static video_udp_service_t *video_udp_service = NULL;
static aud_udp_service_t *aud_udp_service = NULL;
static ntwk_udp_ctrl_info_t *ntwl_udp_ctrl_info = NULL;

// UDP Server implementation

// Video receive data handler
static void ntwk_udp_video_receive_data(uint8_t *data, uint16_t length)
{
    // Directly call user registered receive callback if available
    if (video_udp_service && video_udp_service->receive_cb) {
        video_udp_service->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
}

// Audio receive data handler
static void ntwk_udp_audio_recive_data(uint8_t *data, uint16_t length)
{
    // Directly call user registered receive callback if available
    if (aud_udp_service && aud_udp_service->receive_cb) {
        aud_udp_service->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
}

static void ntwk_udp_update_remote_address(in_addr_t address)
{
    if (video_udp_service != NULL)
    {
        video_udp_service->video_remote.sin_addr.s_addr = address;
    }
    if (aud_udp_service != NULL)
    {
        aud_udp_service->aud_remote.sin_addr.s_addr = address;
    }
}

int ntwk_udp_video_send_packet(uint8_t *data, uint32_t length, image_format_t video_type)
{
    if (video_udp_service == NULL || !video_udp_service->video_status)
    {
        return -1;
    }

    return ntwk_socket_sendto(&video_udp_service->video_fd, (struct sockaddr *)&video_udp_service->video_remote, data, length);
}

int ntwk_udp_audio_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type)
{
    if (aud_udp_service == NULL || !aud_udp_service->aud_status)
    {
        return -1;
    }

    return ntwk_socket_sendto(&aud_udp_service->aud_fd, (struct sockaddr *)&aud_udp_service->aud_remote, data, length);
}

int ntwk_udp_ctrl_chan_send(uint8_t *data, uint32_t length)
{
    int ret = 0;

    if (ntwl_udp_ctrl_info->server_state == BK_FALSE)
    {
        LOGE("%s server not ready\n", __func__);
        return -1;
    }

    if (ntwl_udp_ctrl_info->client_fd < 0)
    {
        LOGE("%s client not ready\n", __func__);
        return -1;
    }

    ret = ntwk_socket_write(&ntwl_udp_ctrl_info->client_fd, data, length);

    return ret;
}

void ntwk_udp_ctrl_receive_data(uint8_t *data, uint16_t length)
{
    LOGD("%s start\r\n", __func__);

    if (ntwl_udp_ctrl_info && ntwl_udp_ctrl_info->receive_cb) {
        ntwl_udp_ctrl_info->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
    LOGD("%s end\r\n", __func__);
}

in_addr_t ntwk_udp_ctrl_get_socket_address(void)
{
    return ntwl_udp_ctrl_info->remote_address;
}

static void ntwk_udp_ctrl_server_thread(beken_thread_arg_t data)
{
    int rcv_len = 0;
    bk_err_t ret = BK_OK;
    u8 *rcv_buf = NULL;
    fd_set watchfd;

    LOGD("%s entry\n", __func__);
    (void)(data);

    rcv_buf = (u8 *) ntwk_malloc((NTWK_TRANS_CMD_BUFFER + 1) * sizeof(u8));

    if (!rcv_buf)
    {
        LOGE("udp malloc failed\n");
        goto out;
    }

    ntwl_udp_ctrl_info->chan_state = NTWK_TRANS_CHAN_START;
    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0 , NTWK_TRANS_CHAN_CTRL);

    // for data transfer
    ntwl_udp_ctrl_info->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ntwl_udp_ctrl_info->server_fd == -1)
    {
        LOGE("socket failed\n");
        goto out;
    }

    ntwl_udp_ctrl_info->socket.sin_family = AF_INET;
    ntwl_udp_ctrl_info->socket.sin_port = htons(NTWK_TRANS_CMD_PORT);
    ntwl_udp_ctrl_info->socket.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(ntwl_udp_ctrl_info->server_fd, (struct sockaddr *)&ntwl_udp_ctrl_info->socket, sizeof(struct sockaddr_in)) == -1)
    {
        LOGE("bind failed\n");
        goto out;
    }

    if (listen(ntwl_udp_ctrl_info->server_fd, 0) == -1)
    {
        LOGE("listen failed\n");
        goto out;
    }

    LOGD("%s: start listen \n", __func__);

    while (1)
    {
        FD_ZERO(&watchfd);
        FD_SET(ntwl_udp_ctrl_info->server_fd, &watchfd);

        ntwl_udp_ctrl_info->chan_state = NTWK_TRANS_CHAN_WAITING_CONNECTED;
        LOGD("%s, waiting for a new connection\n", __func__);
        ret = select(ntwl_udp_ctrl_info->server_fd + 1, &watchfd, NULL, NULL, NULL);
        if (ret <= 0)
        {
            LOGE("select ret:%d\n", ret);
            continue;
        }
        else
        {
            // is new connection
            if (FD_ISSET(ntwl_udp_ctrl_info->server_fd, &watchfd))
            {
                struct sockaddr_in client_addr;
                socklen_t cliaddr_len = 0;

                cliaddr_len = sizeof(client_addr);

                ntwl_udp_ctrl_info->client_fd = accept(ntwl_udp_ctrl_info->server_fd, (struct sockaddr *)&client_addr, &cliaddr_len);

                if (ntwl_udp_ctrl_info->client_fd < 0)
                {
                    LOGE("accept return fd:%d\n", ntwl_udp_ctrl_info->client_fd);
                    break;
                }

                uint8_t *src_ipaddr = (UINT8 *)&client_addr.sin_addr.s_addr;
                ntwl_udp_ctrl_info->chan_state = NTWK_TRANS_CHAN_CONNECTED;
                LOGD("accept a new connection fd:%d, %d.%d.%d.%d\n", ntwl_udp_ctrl_info->client_fd, src_ipaddr[0], src_ipaddr[1],
                     src_ipaddr[2], src_ipaddr[3]);

                ntwk_udp_cntrl_set_keepalive(ntwl_udp_ctrl_info->client_fd,
                                        NTWK_TRANS_CTRL_CHAN_KEEPALIVE_ENABLE,
                                        NTWK_TRANS_CTRL_CHAN_KEEPALIVE_IDLE_TIME,
                                        NTWK_TRANS_CTRL_CHAN_KEEPALIVE_INTERVAL,
                                        NTWK_TRANS_CTRL_CHAN_KEEPALIVE_COUNT);
                ntwl_udp_ctrl_info->remote_address = client_addr.sin_addr.s_addr;
                ntwk_socket_set_qos(ntwl_udp_ctrl_info->client_fd, IP_QOS_PRIORITY_HIGHEST);

                if (ntwl_udp_ctrl_info->server_state == BK_FALSE)
                {
                    ntwl_udp_ctrl_info->server_state = BK_TRUE;
                    ntwk_udp_update_remote_address(ntwl_udp_ctrl_info->remote_address);
                    ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, ntwl_udp_ctrl_info->remote_address, NTWK_TRANS_CHAN_CTRL);
                }

                while (ntwl_udp_ctrl_info->server_state == BK_TRUE)
                {
                    rcv_len = recv(ntwl_udp_ctrl_info->client_fd, rcv_buf, NTWK_TRANS_CMD_BUFFER, 0);
                    if (rcv_len > 0)
                    {
                        //bk_net_send_data(rcv_buf, rcv_len, TVIDEO_SND_TCP);
                        LOGD("%s, got length: %d\n", __func__, rcv_len);
                        ntwk_udp_ctrl_receive_data(rcv_buf, rcv_len);
                    }
                    else
                    {
                        // close this socket
                        LOGD("%s, recv close fd:%d, rcv_len:%d, error:%d\n", __func__, ntwl_udp_ctrl_info->client_fd, rcv_len, errno);
                        close(ntwl_udp_ctrl_info->client_fd);
                        ntwl_udp_ctrl_info->client_fd = -1;
                        ntwl_udp_ctrl_info->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;

                        if (ntwl_udp_ctrl_info->server_state == BK_TRUE)
                        {
                            ntwl_udp_ctrl_info->server_state = BK_FALSE;
                            ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0 , NTWK_TRANS_CHAN_CTRL);
                        }

                        break;
                    }
                }
            }
        }
    }
out:

    LOGE("%s exit %d\n", __func__, ntwl_udp_ctrl_info->server_state);
    if (rcv_buf)
    {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    ntwl_udp_ctrl_info->server_state = BK_FALSE;

    if (ntwl_udp_ctrl_info->client_fd != -1)
    {
        close(ntwl_udp_ctrl_info->client_fd);
        ntwl_udp_ctrl_info->client_fd = -1;
    }

    if (ntwl_udp_ctrl_info->server_fd != -1)
    {
        close(ntwl_udp_ctrl_info->server_fd);
        ntwl_udp_ctrl_info->server_fd = -1;
    }

    rtos_delete_thread(NULL);
}

bk_err_t ntwk_udp_ctrl_chan_start(void *param)
{
    bk_err_t ret = BK_OK;

    if (ntwl_udp_ctrl_info == NULL)
    {
        LOGE("malloc ntwl_udp_ctrl_info\n");
        return BK_ERR_NO_MEM;
    }

    if (!ntwl_udp_ctrl_info->thread)
    {
        ret = rtos_create_thread(&ntwl_udp_ctrl_info->thread,
                                 4,
                                 "ntwl_udp_ctrl_srv",
                                 (beken_thread_function_t)ntwk_udp_ctrl_server_thread,
                                 1024 * 4,
                                 (beken_thread_arg_t)NULL);
        if (ret != kNoErr)
        {
            LOGE("Error: failed to create udp ctrl server: %d\n", ret);
        }
    }

    return ret;
}

bk_err_t ntwk_udp_ctrl_chan_stop(void)
{
    if (ntwl_udp_ctrl_info == NULL)
    {
        LOGE("ntwl_udp_ctrl_info is NULL, nothing to deinit\n");
        return BK_FAIL;
    }

    ntwl_udp_ctrl_info->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0 , NTWK_TRANS_CHAN_CTRL);



    if (ntwl_udp_ctrl_info->client_fd != -1)
    {
        close(ntwl_udp_ctrl_info->client_fd);
        ntwl_udp_ctrl_info->client_fd = -1;
    }

    if (ntwl_udp_ctrl_info->server_fd != -1)
    {
        close(ntwl_udp_ctrl_info->server_fd);
        ntwl_udp_ctrl_info->server_fd = -1;
    }

    if (ntwl_udp_ctrl_info->thread != NULL)
    {
        rtos_thread_join(ntwl_udp_ctrl_info->thread);
        ntwl_udp_ctrl_info->thread = NULL;
    }

    ntwl_udp_ctrl_info->receive_cb = NULL;

    LOGD("%s end\n", __func__);
    return BK_OK;
}

bk_err_t ntwk_udp_ctrl_register_receive_cb(ntwk_udp_ctrl_receive_cb_t cb)
{
    if (ntwl_udp_ctrl_info == NULL)
    {
        LOGE("%s: UDP control channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwl_udp_ctrl_info->receive_cb = cb;
    LOGD("%s end\n", __func__);

    return BK_OK;
}


static void ntwk_udp_video_service_main(beken_thread_arg_t data)
{
    int ret = 0;
    int rcv_len = 0;
    socklen_t srvaddr_len = 0;
    fd_set watchfd;
    struct timeval timeout;
    u8 *rcv_buf = NULL;
    in_addr_t remote = ntwk_udp_ctrl_get_socket_address();

    LOGD("%s, port: %d\n", __func__, NTWK_TRANS_UDP_VIDEO_PORT);
    
    (void)(data);

    video_udp_service->chan_state = NTWK_TRANS_CHAN_START;
    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_VIDEO);

    rcv_buf = (u8 *)ntwk_malloc((NTWK_TRANS_DATA_MAX_SIZE + 1) * sizeof(u8));
    if (!rcv_buf)
    {
        LOGE("video udp malloc failed\n");
        goto out;
    }

    // for data transfer
    video_udp_service->video_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (video_udp_service->video_fd == -1)
    {
        LOGE("video socket failed\n");
        goto out;
    }

    video_udp_service->video_remote.sin_family = AF_INET;
    video_udp_service->video_remote.sin_port = htons(NTWK_TRANS_UDP_VIDEO_PORT);

    if (remote == 0)
    {
        video_udp_service->video_remote.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        video_udp_service->video_remote.sin_addr.s_addr = remote;
    }

    srvaddr_len = (socklen_t)sizeof(struct sockaddr_in);
    if (bind(video_udp_service->video_fd, (struct sockaddr *)&video_udp_service->video_remote, srvaddr_len) == -1)
    {
        LOGE("video bind failed\n");
        goto out;
    }

    timeout.tv_sec = APP_DEMO_UDP_SOCKET_TIMEOUT / 1000;
    timeout.tv_usec = (APP_DEMO_UDP_SOCKET_TIMEOUT % 1000) * 1000;

    video_udp_service->video_status = 1;
    video_udp_service->chan_state = NTWK_TRANS_CHAN_CONNECTED;

    ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, 0, NTWK_TRANS_CHAN_VIDEO);

    while (video_udp_service->video_status)
    {
        FD_ZERO(&watchfd);
        FD_SET(video_udp_service->video_fd, &watchfd);

        ret = select(video_udp_service->video_fd + 1, &watchfd, NULL, NULL, &timeout);
        if (ret < 0)
        {
            LOGE("video select ret:%d\n", ret);
            break;
        }
        else if (ret > 0)
        {
            if (FD_ISSET(video_udp_service->video_fd, &watchfd))
            {
                rcv_len = recvfrom(video_udp_service->video_fd, rcv_buf, NTWK_TRANS_DATA_MAX_SIZE, 0,
                                   (struct sockaddr *)&video_udp_service->video_remote, &srvaddr_len);

                if (rcv_len <= 0)
                {
                    LOGE("video recv close fd:%d, error_code:%d\n", video_udp_service->video_fd, errno);

                    video_udp_service->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;
                    ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, errno, NTWK_TRANS_CHAN_VIDEO);

                    break;
                }
                else
                {
                    rcv_len = (rcv_len > NTWK_TRANS_DATA_MAX_SIZE) ? NTWK_TRANS_DATA_MAX_SIZE : rcv_len;
                    rcv_buf[rcv_len] = 0;
                    ntwk_udp_video_receive_data(rcv_buf, rcv_len);
                    LOGD("video udp receiver\n");
                }
            }
        }
    }

out:

    LOGE("%s, exit %d\n", __func__, video_udp_service->video_status);

    if (rcv_buf)
    {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    if (video_udp_service->video_fd != -1)
    {
        close(video_udp_service->video_fd);
        video_udp_service->video_fd = -1;
    }

    rtos_delete_thread(NULL);
}

static void ntwk_udp_aud_service_main(beken_thread_arg_t data)
{
    int ret = 0;
    int rcv_len = 0;
    socklen_t srvaddr_len = 0;
    fd_set watchfd;
    struct timeval timeout;
    u8 *rcv_buf = NULL;
    in_addr_t remote = ntwk_udp_ctrl_get_socket_address();

    LOGD("ntwk_udp_aud_service, port: %d\n", NTWK_TRANS_UDP_AUDIO_PORT);
    (void)(data);

    aud_udp_service->chan_state = NTWK_TRANS_CHAN_START;
    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_AUDIO);

    rcv_buf = (u8 *)ntwk_malloc((NTWK_TRANS_DATA_MAX_SIZE + 1) * sizeof(u8));
    if (!rcv_buf)
    {
        LOGE("aud udp malloc failed\n");
        goto out;
    }

    // for data transfer
    aud_udp_service->aud_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (aud_udp_service->aud_fd == -1)
    {
        LOGE("aud socket failed\n");
        goto out;
    }

    aud_udp_service->aud_remote.sin_family = AF_INET;
    aud_udp_service->aud_remote.sin_port = htons(NTWK_TRANS_UDP_AUDIO_PORT);

    if (remote == 0)
    {
        aud_udp_service->aud_remote.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        aud_udp_service->aud_remote.sin_addr.s_addr = remote;
    }

    srvaddr_len = (socklen_t)sizeof(struct sockaddr_in);
    if (bind(aud_udp_service->aud_fd, (struct sockaddr *)&aud_udp_service->aud_remote, srvaddr_len) == -1)
    {
        LOGE("aud bind failed\n");
        goto out;
    }

    timeout.tv_sec = APP_DEMO_UDP_SOCKET_TIMEOUT / 1000;
    timeout.tv_usec = (APP_DEMO_UDP_SOCKET_TIMEOUT % 1000) * 1000;

    aud_udp_service->aud_status = 1;
    aud_udp_service->chan_state = NTWK_TRANS_CHAN_CONNECTED;

    ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, 0, NTWK_TRANS_CHAN_AUDIO);

    while (aud_udp_service->aud_status)
    {
        FD_ZERO(&watchfd);
        FD_SET(aud_udp_service->aud_fd, &watchfd);

        ret = select(aud_udp_service->aud_fd + 1, &watchfd, NULL, NULL, &timeout);
        if (ret < 0)
        {
            LOGE("aud select ret:%d\n", ret);
            break;
        }
        else if (ret > 0)
        {
            if (FD_ISSET(aud_udp_service->aud_fd, &watchfd))
            {
                rcv_len = recvfrom(aud_udp_service->aud_fd, rcv_buf, NTWK_TRANS_DATA_MAX_SIZE, 0,
                                   (struct sockaddr *)&aud_udp_service->aud_remote, &srvaddr_len);

                if (rcv_len <= 0)
                {
                    LOGE("aud recv close fd:%d, error_code:%d\n", aud_udp_service->aud_fd, errno);

                    aud_udp_service->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;
                    ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, errno, NTWK_TRANS_CHAN_AUDIO);

                    break;
                }
                else
                {
                    rcv_len = (rcv_len > NTWK_TRANS_DATA_MAX_SIZE) ? NTWK_TRANS_DATA_MAX_SIZE : rcv_len;
                    rcv_buf[rcv_len] = 0;
                    ntwk_udp_audio_recive_data(rcv_buf, rcv_len);
                    //LOGD("aud udp receiver\n");
                }
            }
        }
    }

out:

    LOGE("%s, exit %d\n", __func__, aud_udp_service->aud_status);

    if (rcv_buf)
    {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    if (aud_udp_service->aud_fd != -1)
    {
        close(aud_udp_service->aud_fd);
        aud_udp_service->aud_fd = -1;
    }

    rtos_delete_thread(NULL);
}

bk_err_t ntwk_udp_video_chan_start(void *param)
{
    bk_err_t ret = BK_OK;

    LOGD("%s\n", __func__);

    // Initialize image service
    if (video_udp_service == NULL)
    {
        LOGE("video_udp_service already init\n");
        return BK_FAIL;
    }

    ret = rtos_create_thread(&video_udp_service->thd,
                             4,
                             "ntwk_video",
                             (beken_thread_function_t)ntwk_udp_video_service_main,
                             1024 * 2,
                             (beken_thread_arg_t)NULL);
    if (ret != BK_OK)
    {
        LOGE("%s, ntwk_udp_video_service_main failed %d\n", __func__, ret);
    }

    return ret;
}

bk_err_t ntwk_udp_video_chan_stop(void)
{
    LOGD("%s\n", __func__);

    if (video_udp_service == NULL)
    {
        LOGE("%s, service null\n", __func__);
        return BK_FAIL;
    }

    video_udp_service->video_status = 0;
    video_udp_service->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_VIDEO);

    if (video_udp_service->video_fd != -1)
    {
        close(video_udp_service->video_fd);
        video_udp_service->video_fd = -1;
    }

    // Wait for thread to exit
    if (video_udp_service->thd != NULL)
    {
        rtos_thread_join(&video_udp_service->thd);
        video_udp_service->thd = NULL;
    }

    LOGD("UDP video channel deinitialized\n");
    return BK_OK;
}

bk_err_t ntwk_udp_audio_chan_start(void *param)
{
    bk_err_t ret = BK_OK;

    LOGD("%s\n", __func__);

    // Initialize audio service
    if (aud_udp_service == NULL)
    {
        LOGE("aud_udp_service already init\n");
        return BK_FAIL;
    }

    ret = rtos_create_thread(&aud_udp_service->thd,
                             4,
                             "ntwk_aud",
                             (beken_thread_function_t)ntwk_udp_aud_service_main,
                             1024 * 2,
                             (beken_thread_arg_t)NULL);
    if (ret != BK_OK)
    {
        LOGE("Error: Failed to create ntwk aud udp service: %d\n", ret);
    }

    return ret;
}

bk_err_t ntwk_udp_audio_chan_stop(void)
{
    LOGD("%s\n", __func__);

    if (aud_udp_service == NULL)
    {
        LOGE("%s, service null\n", __func__);
        return BK_FAIL;
    }

    aud_udp_service->aud_status = 0;
    aud_udp_service->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_AUDIO);

    if (aud_udp_service->aud_fd != -1)
    {
        close(aud_udp_service->aud_fd);
        aud_udp_service->aud_fd = -1;
    }

    // Wait for thread to exit
    if (aud_udp_service->thd != NULL)
    {
        rtos_thread_join(&aud_udp_service->thd);
        aud_udp_service->thd = NULL;
    }

    LOGD("UDP audio channel deinitialized\n");
    return BK_OK;
}

bk_err_t ntwk_udp_video_register_receive_cb(ntwk_udp_video_receive_cb_t cb)
{
    if (video_udp_service == NULL)
    {
        LOGE("%s: UDP Video channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    video_udp_service->receive_cb = cb;
    //LOGD("%s: UDP Video receive callback registered successfully\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_udp_audio_register_receive_cb(ntwk_udp_audio_receive_cb_t cb)
{
    if (aud_udp_service == NULL)
    {
        LOGE("%s: UDP Audio channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    aud_udp_service->receive_cb = cb;
    //LOGD("%s: UDP Audio receive callback registered successfully\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_udp_init(chan_type_t chan_type)
{
    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (ntwl_udp_ctrl_info == NULL)
            {
                ntwl_udp_ctrl_info = ntwk_malloc(sizeof(ntwk_udp_ctrl_info_t));
                if (ntwl_udp_ctrl_info == NULL)
                {
                    LOGE("malloc ntwl_udp_ctrl_info\n");
                    return BK_ERR_NO_MEM;
                }
        
                os_memset(ntwl_udp_ctrl_info, 0, sizeof(ntwk_udp_ctrl_info_t));
            }
            else
            {
                LOGE("ntwl_udp_ctrl_info already initialized\n");
                return BK_FAIL;
            }
        } break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (video_udp_service == NULL)
            {
                video_udp_service = ntwk_malloc(sizeof(video_udp_service_t));
                if (video_udp_service == NULL)
                {
                    LOGE("video_udp_service malloc failed\n");
                    return BK_FAIL;
                }
                os_memset(video_udp_service, 0, sizeof(video_udp_service_t));
            }
            else
            {
                LOGE("video_udp_service already initialized\n");
                return BK_FAIL;
            }
        } break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (aud_udp_service == NULL)
            {
                aud_udp_service = ntwk_malloc(sizeof(aud_udp_service_t));
                if (aud_udp_service == NULL)
                {
                    LOGE("aud_udp_service malloc failed\n");
                    return BK_FAIL;
                }
                os_memset(aud_udp_service, 0, sizeof(aud_udp_service_t));
            }
            else
            {
                LOGE("aud_udp_service malloc failed\n");
                return BK_FAIL;
            }
        } break;
        default:
        {
            LOGE("invalid channel type\n");
            return BK_FAIL;
        } break;
    }

    return BK_OK;
}

bk_err_t ntwk_udp_deinit(chan_type_t chan_type)
{
    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (ntwl_udp_ctrl_info != NULL)
            {
                os_free(ntwl_udp_ctrl_info);
                ntwl_udp_ctrl_info = NULL;
            }
        } break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (video_udp_service != NULL)
            {
                os_free(video_udp_service);
                video_udp_service = NULL;
            }
        } break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (aud_udp_service != NULL)
            {
                os_free(aud_udp_service);
                aud_udp_service = NULL;
            }
        } break;
        default:
        {
            LOGE("invalid channel type\n");
            return BK_FAIL;
        } break;
    }

    return BK_OK;
}
#else // CONFIG_NTWK_CLIENT_SERVICE_ENABLE
// UDP Client implementation
static video_udp_client_service_t *video_udp_client_service = NULL;
static aud_udp_client_service_t *aud_udp_client_service = NULL;
static ntwk_udp_ctrl_client_info_t *ntwk_udp_ctrl_client_info = NULL;

// UDP Client control channel receive data handler
static void ntwk_udp_ctrl_client_receive_data(uint8_t *data, uint16_t length)
{
    LOGV("%s start\r\n", __func__);

    if (ntwk_udp_ctrl_client_info && ntwk_udp_ctrl_client_info->receive_cb) {
        ntwk_udp_ctrl_client_info->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
    LOGV("%s end\r\n", __func__);
}

// UDP Client control channel thread (actually uses TCP client)
static void ntwk_udp_ctrl_client_thread(beken_thread_arg_t data)
{
    int rcv_len = 0;
    bk_err_t ret = BK_OK;
    u8 *rcv_buf = NULL;
    int connect_retry = 0;
    const int max_retry = 5;

    LOGV("%s entry\n", __func__);
    (void)(data);

    rcv_buf = (u8 *) ntwk_malloc((NTWK_TRANS_CMD_BUFFER + 1) * sizeof(u8));
    if (!rcv_buf)
    {
        LOGE("tcp client malloc failed\n");
        goto out;
    }

    if (ntwk_udp_ctrl_client_info == NULL) {
        LOGV("%s: service freed, exit\n", __func__);
        goto out;
    }

    ntwk_udp_ctrl_client_info->chan_state = NTWK_TRANS_CHAN_START;
    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_CTRL);

    while (1)
    {
        if (ntwk_udp_ctrl_client_info == NULL) {
            LOGV("%s: service freed during run, exit\n", __func__);
            break;
        }
        // Check if stop was called
        if (ntwk_udp_ctrl_client_info->chan_state == NTWK_TRANS_CHAN_STOP)
        {
            LOGD("%s, stop called, exiting\n", __func__);
            break;
        }

        // Create socket (TCP)
        ntwk_udp_ctrl_client_info->client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (ntwk_udp_ctrl_client_info->client_fd == -1)
        {
            LOGE("socket failed\n");
            goto out;
        }

        // Set server address
        ntwk_udp_ctrl_client_info->server_addr.sin_family = AF_INET;
        ntwk_udp_ctrl_client_info->server_addr.sin_port = htons(ntwk_udp_ctrl_client_info->server_port);
        ntwk_udp_ctrl_client_info->server_addr.sin_addr.s_addr = ntwk_udp_ctrl_client_info->server_address;

        ntwk_udp_ctrl_client_info->chan_state = NTWK_TRANS_CHAN_WAITING_CONNECTED;
        LOGV("%s, connecting to server %s:%d\n", __func__,
             inet_ntoa(*(struct in_addr *)&ntwk_udp_ctrl_client_info->server_address),
             ntwk_udp_ctrl_client_info->server_port);

        // Connect to server
        ret = connect(ntwk_udp_ctrl_client_info->client_fd,
                     (struct sockaddr *)&ntwk_udp_ctrl_client_info->server_addr,
                     sizeof(struct sockaddr_in));
        if (ret < 0)
        {
            LOGE("connect failed: %d\n", errno);
            close(ntwk_udp_ctrl_client_info->client_fd);
            ntwk_udp_ctrl_client_info->client_fd = -1;

            // Check if stop was called during retry
            if (ntwk_udp_ctrl_client_info->chan_state == NTWK_TRANS_CHAN_STOP)
            {
                LOGV("%s, stop called during connect retry\n", __func__);
                break;
            }

            connect_retry++;
            if (connect_retry < max_retry)
            {
                LOGW("Retry connecting... (%d/%d)\n", connect_retry, max_retry);
                rtos_delay_milliseconds(1000);
                continue;
            }
            else
            {
                LOGE("Max retry reached, exit\n");
                goto out;
            }
        }

        connect_retry = 0;
        ntwk_udp_ctrl_client_info->chan_state = NTWK_TRANS_CHAN_CONNECTED;
        ntwk_udp_ctrl_client_info->client_state = BK_TRUE;

        //LOGV("ctrl, Connected to server fd:%d\n", ntwk_udp_ctrl_client_info->client_fd);
        #if 0
        {
            struct sockaddr_in local_addr, peer_addr;
            socklen_t len = sizeof(local_addr);
            if (getsockname(ntwk_udp_ctrl_client_info->client_fd, (struct sockaddr *)&local_addr, &len) == 0) {
                len = sizeof(peer_addr);
                if (getpeername(ntwk_udp_ctrl_client_info->client_fd, (struct sockaddr *)&peer_addr, &len) == 0) {
                    LOGI("ctrl, connected: local_port=%u, peer_port=%u\n",
                         (unsigned)ntohs(local_addr.sin_port), (unsigned)ntohs(peer_addr.sin_port));
                }
            }
        }
        #endif

        ntwk_udp_cntrl_set_keepalive(ntwk_udp_ctrl_client_info->client_fd,
                               NTWK_TRANS_CTRL_CHAN_KEEPALIVE_ENABLE,
                               NTWK_TRANS_CTRL_CHAN_KEEPALIVE_IDLE_TIME,
                               NTWK_TRANS_CTRL_CHAN_KEEPALIVE_INTERVAL,
                               NTWK_TRANS_CTRL_CHAN_KEEPALIVE_COUNT);
        ntwk_socket_set_qos(ntwk_udp_ctrl_client_info->client_fd, IP_QOS_PRIORITY_HIGHEST);

        ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, ntwk_udp_ctrl_client_info->server_address, NTWK_TRANS_CHAN_CTRL);

        // Receive data loop
        while (ntwk_udp_ctrl_client_info != NULL && ntwk_udp_ctrl_client_info->client_state == BK_TRUE)
        {
            rcv_len = recv(ntwk_udp_ctrl_client_info->client_fd, rcv_buf, NTWK_TRANS_CMD_BUFFER, 0);
            if (rcv_len > 0)
            {
                LOGD("ctrl, got length: %d\n", rcv_len);
                ntwk_udp_ctrl_client_receive_data(rcv_buf, rcv_len);
            }
            else
            {
                LOGD("%s, recv close fd:%d, rcv_len:%d, error:%d\n", __func__,
                     ntwk_udp_ctrl_client_info->client_fd, rcv_len, errno);
                close(ntwk_udp_ctrl_client_info->client_fd);
                ntwk_udp_ctrl_client_info->client_fd = -1;
                ntwk_udp_ctrl_client_info->client_state = BK_FALSE;
                
                // Check if stop was called before setting disconnected state
                if (ntwk_udp_ctrl_client_info->chan_state == NTWK_TRANS_CHAN_STOP)
                {
                    // Stop was called, exit the thread
                    break;
                }
                
                ntwk_udp_ctrl_client_info->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;
                ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0, NTWK_TRANS_CHAN_CTRL);

                // Try to reconnect
                rtos_delay_milliseconds(1000);
                break;
            }
        }
    }

out:
    if (ntwk_udp_ctrl_client_info != NULL) {
        LOGE("%s exit %d\n", __func__, ntwk_udp_ctrl_client_info->client_state);
        ntwk_udp_ctrl_client_info->client_state = BK_FALSE;
        if (ntwk_udp_ctrl_client_info->client_fd != -1) {
            close(ntwk_udp_ctrl_client_info->client_fd);
            ntwk_udp_ctrl_client_info->client_fd = -1;
        }
    }
    if (rcv_buf) {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    rtos_delete_thread(NULL);
}

bk_err_t ntwk_udp_ctrl_client_chan_start(void *param)
{
    bk_err_t ret = BK_OK;
    ntwk_server_net_info_t *server_net_info = ntwk_trans_get_server_net_info();

    if (!server_net_info)
    {
        LOGE("server_net_info is NULL\n");
        return BK_FAIL;
    }

    if (ntwk_udp_ctrl_client_info == NULL)
    {
        LOGE("ntwk_udp_ctrl_client_info is NULL\n");
        return BK_FAIL;
    }

    ntwk_udp_ctrl_client_info->server_address = inet_addr((char *)server_net_info->ip_addr);
    if (ntwk_udp_ctrl_client_info->server_address == INADDR_NONE)
    {
        LOGE("Invalid IP address: %s\n", server_net_info->ip_addr);
        return BK_FAIL;
    }

    ntwk_udp_ctrl_client_info->server_port = (uint16_t)atoi((char *)server_net_info->cmd_port);
    if (ntwk_udp_ctrl_client_info->server_port == 0)
    {
        LOGE("Invalid port: %s\n", server_net_info->cmd_port);
        return BK_FAIL;
    }

    if (!ntwk_udp_ctrl_client_info->thread)
    {
        ret = rtos_create_thread(&ntwk_udp_ctrl_client_info->thread,
                                 4,
                                 "ntwk_tcp_ctrl_cli",
                                 (beken_thread_function_t)ntwk_udp_ctrl_client_thread,
                                 1024 * 4,
                                 (beken_thread_arg_t)NULL);
        if (ret != kNoErr)
        {
            LOGE("Error: failed to create tcp ctrl client: %d\n", ret);
        }
    }

    return ret;
}

bk_err_t ntwk_udp_ctrl_client_chan_stop(void)
{
    if (ntwk_udp_ctrl_client_info == NULL)
    {
        LOGE("ntwk_udp_ctrl_client_info is NULL, nothing to deinit\n");
        return BK_FAIL;
    }

    ntwk_udp_ctrl_client_info->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_CTRL);

    ntwk_udp_ctrl_client_info->client_state = BK_FALSE;

    if (ntwk_udp_ctrl_client_info->client_fd != -1)
    {
        close(ntwk_udp_ctrl_client_info->client_fd);
        ntwk_udp_ctrl_client_info->client_fd = -1;
    }

    if (ntwk_udp_ctrl_client_info->thread != NULL)
    {
        rtos_thread_join(ntwk_udp_ctrl_client_info->thread);
        ntwk_udp_ctrl_client_info->thread = NULL;
    }

    ntwk_udp_ctrl_client_info->receive_cb = NULL;
   // ntwk_udp_client_deinit(NTWK_TRANS_CHAN_CTRL);

    LOGV("%s end\n", __func__);
    return BK_OK;
}

int ntwk_udp_ctrl_client_chan_send(uint8_t *data, uint32_t length)
{
    int ret = 0;

    if (ntwk_udp_ctrl_client_info == NULL)
    {
        LOGE("%s client info is NULL\n", __func__);
        return -1;
    }

    if (ntwk_udp_ctrl_client_info->client_state == BK_FALSE)
    {
        LOGE("%s client not ready\n", __func__);
        return -1;
    }

    if (ntwk_udp_ctrl_client_info->client_fd < 0)
    {
        LOGE("%s client fd invalid\n", __func__);
        return -1;
    }

    ret = ntwk_socket_write(&ntwk_udp_ctrl_client_info->client_fd, data, length);

    return ret;
}

bk_err_t ntwk_udp_ctrl_client_register_receive_cb(ntwk_udp_ctrl_receive_cb_t cb)
{
    if (ntwk_udp_ctrl_client_info == NULL)
    {
        LOGE("%s: UDP control client channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwk_udp_ctrl_client_info->receive_cb = cb;
    //LOGV("%s: Receive callback registered successfully\n", __func__);

    return BK_OK;
}

// UDP Client video channel receive data handler
static void ntwk_udp_video_client_receive_data(uint8_t *data, uint16_t length)
{
    if (video_udp_client_service && video_udp_client_service->receive_cb) {
        video_udp_client_service->receive_cb(data, length);
    } else {
        LOGW("%s\n", __func__);
    }
}

// UDP Client video channel thread
static void ntwk_udp_video_client_thread(beken_thread_arg_t data)
{
    int ret = 0;
    int rcv_len = 0;
    socklen_t addr_len = 0;
    fd_set watchfd;
    struct timeval timeout;
    u8 *rcv_buf = NULL;

    LOGV("%s entry\n", __func__);
    (void)(data);

    if (video_udp_client_service == NULL) {
        LOGV("%s: service freed, exit\n", __func__);
        goto out;
    }

    video_udp_client_service->chan_state = NTWK_TRANS_CHAN_START;
    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_VIDEO);

    rcv_buf = (u8 *)ntwk_malloc((NTWK_TRANS_DATA_MAX_SIZE + 1) * sizeof(u8));
    if (!rcv_buf)
    {
        LOGE("video udp client malloc failed\n");
        goto out;
    }

    if (video_udp_client_service == NULL) {
        LOGV("%s: service freed, exit\n", __func__);
        goto out;
    }

    // Create socket
    video_udp_client_service->video_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (video_udp_client_service->video_fd == -1)
    {
        LOGE("video socket failed\n");
        goto out;
    }

    /* Bind to port 0 so kernel assigns local port immediately; else getsockname() returns 0 until first send/recv */
    {
        struct sockaddr_in bind_addr;
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_port = 0;
        bind_addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(video_udp_client_service->video_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
            LOGW("video bind(0) failed: %d\n", errno);
        }
    }

    // Set server address
    video_udp_client_service->server_addr.sin_family = AF_INET;
    video_udp_client_service->server_addr.sin_port = htons(video_udp_client_service->server_port);
    video_udp_client_service->server_addr.sin_addr.s_addr = video_udp_client_service->server_address;

    timeout.tv_sec = APP_DEMO_UDP_SOCKET_TIMEOUT / 1000;
    timeout.tv_usec = (APP_DEMO_UDP_SOCKET_TIMEOUT % 1000) * 1000;

    if (setsockopt(video_udp_client_service->video_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        LOGW("setsockopt SO_RCVTIMEO failed\n");
    }

    video_udp_client_service->video_status = 1;
    video_udp_client_service->chan_state = NTWK_TRANS_CHAN_CONNECTED;
    ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, video_udp_client_service->server_address, NTWK_TRANS_CHAN_VIDEO);
#if 0
    LOGV("%s: connected to server %s:%d\n", __func__,
         inet_ntoa(*(struct in_addr *)&video_udp_client_service->server_address),
         video_udp_client_service->server_port);


    {
        struct sockaddr_in local_addr;
        socklen_t len = sizeof(local_addr);
        if (getsockname(video_udp_client_service->video_fd, (struct sockaddr *)&local_addr, &len) == 0) {
            LOGI("video, connected: local_port=%u, peer_port=%u\n",
                 (unsigned)ntohs(local_addr.sin_port), (unsigned)video_udp_client_service->server_port);
        }
    }
#endif
    while (video_udp_client_service != NULL && video_udp_client_service->video_status)
    {
        // Check if stop was called
        if (video_udp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
        {
            LOGD("%s, stop called, exiting\n", __func__);
            break;
        }

        FD_ZERO(&watchfd);
        FD_SET(video_udp_client_service->video_fd, &watchfd);

        ret = select(video_udp_client_service->video_fd + 1, &watchfd, NULL, NULL, &timeout);
        if (ret < 0)
        {
            // Check if stop was called
            if (video_udp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
            {
                LOGV("%s, stop called during select\n", __func__);
                break;
            }
            LOGE("video select ret:%d\n", ret);
            break;
        }
        else if (ret > 0)
        {
            if (FD_ISSET(video_udp_client_service->video_fd, &watchfd))
            {
                struct sockaddr_in from_addr;
                addr_len = sizeof(struct sockaddr_in);
                rcv_len = recvfrom(video_udp_client_service->video_fd, rcv_buf, NTWK_TRANS_DATA_MAX_SIZE, 0,
                                   (struct sockaddr *)&from_addr, &addr_len);

                if (rcv_len > 0)
                {
                    rcv_len = (rcv_len > NTWK_TRANS_DATA_MAX_SIZE) ? NTWK_TRANS_DATA_MAX_SIZE : rcv_len;
                    rcv_buf[rcv_len] = 0;
#if 0
                    {
                        struct sockaddr_in local_addr;
                        socklen_t local_len = sizeof(local_addr);
                        if (getsockname(video_udp_client_service->video_fd, (struct sockaddr *)&local_addr, &local_len) == 0) {
                            LOGI("video recv: len=%d, local_port=%u, peer_port=%u\n",
                                 rcv_len, (unsigned)ntohs(local_addr.sin_port), (unsigned)ntohs(from_addr.sin_port));
                        }
                    }
#endif
                    ntwk_udp_video_client_receive_data(rcv_buf, rcv_len);
                }
                else if (rcv_len < 0)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        // Check if stop was called before setting disconnected state
                        if (video_udp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
                        {
                            LOGD("%s, stop called during recvfrom\n", __func__);
                            break;
                        }
                        LOGE("video client recvfrom error: %d\n", errno);
                        video_udp_client_service->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;
                        ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, errno, NTWK_TRANS_CHAN_VIDEO);
                        break;
                    }
                }
            }
        }
    }

out:
    if (video_udp_client_service != NULL) {
        LOGE("%s exit %d\n", __func__, video_udp_client_service->video_status);
        video_udp_client_service->video_status = 0;
        if (video_udp_client_service->video_fd != -1) {
            close(video_udp_client_service->video_fd);
            video_udp_client_service->video_fd = -1;
        }
    }
    if (rcv_buf) {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    rtos_delete_thread(NULL);
}

bk_err_t ntwk_udp_video_client_chan_start(void *param)
{
    bk_err_t ret = BK_OK;
    ntwk_server_net_info_t *server_net_info = ntwk_trans_get_server_net_info();

    if (!server_net_info)
    {
        LOGE("server_net_info is NULL\n");
        return BK_FAIL;
    }

    LOGV("%s\n", __func__);

    if (video_udp_client_service == NULL)
    {
        LOGE("video_udp_client_service is NULL\n");
        return BK_FAIL;
    }

    video_udp_client_service->server_address = inet_addr((char *)server_net_info->ip_addr);
    if (video_udp_client_service->server_address == INADDR_NONE)
    {
        LOGE("Invalid IP address: %s\n", server_net_info->ip_addr);
        return BK_FAIL;
    }

    video_udp_client_service->server_port = (uint16_t)atoi((char *)server_net_info->video_port);
    if (video_udp_client_service->server_port == 0)
    {
        LOGE("Invalid port: %s\n", server_net_info->video_port);
        return BK_FAIL;
    }

    ret = rtos_create_thread(&video_udp_client_service->thd,
                             4,
                             "ntwk_video_cli",
                             (beken_thread_function_t)ntwk_udp_video_client_thread,
                             1024 * 2,
                             (beken_thread_arg_t)NULL);
    if (ret != BK_OK)
    {
        LOGE("%s, ntwk_udp_video_client_thread failed %d\n", __func__, ret);
    }

    return ret;
}

bk_err_t ntwk_udp_video_client_chan_stop(void)
{
    LOGV("%s\n", __func__);

    if (video_udp_client_service == NULL)
    {
        LOGE("%s, service null\n", __func__);
        return BK_FAIL;
    }

    video_udp_client_service->video_status = 0;
    video_udp_client_service->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_VIDEO);

    if (video_udp_client_service->video_fd != -1)
    {
        close(video_udp_client_service->video_fd);
        video_udp_client_service->video_fd = -1;
    }

    if (video_udp_client_service->thd != NULL)
    {
        rtos_thread_join(video_udp_client_service->thd);
        video_udp_client_service->thd = NULL;
    }

    LOGV("UDP video client channel deinitialized\n");
    return BK_OK;
}

int ntwk_udp_video_client_send_packet(uint8_t *data, uint32_t length, image_format_t video_type)
{
    if (video_udp_client_service == NULL || !video_udp_client_service->video_status)
    {
        return -1;
    }

    return sendto(video_udp_client_service->video_fd, data, length, 0,
                 (struct sockaddr *)&video_udp_client_service->server_addr,
                 sizeof(struct sockaddr_in));
}

bk_err_t ntwk_udp_video_client_register_receive_cb(ntwk_udp_video_receive_cb_t cb)
{
    if (video_udp_client_service == NULL)
    {
        LOGE("%s: UDP Video client channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    video_udp_client_service->receive_cb = cb;
    LOGI("%s: UDP Video receive callback registered successfully\n", __func__);

    return BK_OK;
}

// UDP Client audio channel receive data handler
static void ntwk_udp_audio_client_receive_data(uint8_t *data, uint16_t length)
{
    if (aud_udp_client_service && aud_udp_client_service->receive_cb) {
        aud_udp_client_service->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
}

// UDP Client audio channel thread
static void ntwk_udp_audio_client_thread(beken_thread_arg_t data)
{
    int ret = 0;
    int rcv_len = 0;
    socklen_t addr_len = 0;
    fd_set watchfd;
    struct timeval timeout;
    u8 *rcv_buf = NULL;

    LOGV("%s entry\n", __func__);
    (void)(data);

    if (aud_udp_client_service == NULL) {
        LOGV("%s: service freed, exit\n", __func__);
        goto out;
    }

    aud_udp_client_service->chan_state = NTWK_TRANS_CHAN_START;
    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_AUDIO);

    rcv_buf = (u8 *)ntwk_malloc((NTWK_TRANS_DATA_MAX_SIZE + 1) * sizeof(u8));
    if (!rcv_buf)
    {
        LOGE("aud udp client malloc failed\n");
        goto out;
    }

    if (aud_udp_client_service == NULL) {
        LOGV("%s: service freed, exit\n", __func__);
        goto out;
    }

    // Create socket
    aud_udp_client_service->aud_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (aud_udp_client_service->aud_fd == -1)
    {
        LOGE("aud socket failed\n");
        goto out;
    }

    /* Bind to port 0 so kernel assigns local port immediately; else getsockname() returns 0 until first send/recv */
    {
        struct sockaddr_in bind_addr;
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_port = 0;
        bind_addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(aud_udp_client_service->aud_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
            LOGW("audio bind(0) failed: %d\n", errno);
        }
    }

    // Set server address
    aud_udp_client_service->server_addr.sin_family = AF_INET;
    aud_udp_client_service->server_addr.sin_port = htons(aud_udp_client_service->server_port);
    aud_udp_client_service->server_addr.sin_addr.s_addr = aud_udp_client_service->server_address;

    timeout.tv_sec = APP_DEMO_UDP_SOCKET_TIMEOUT / 1000;
    timeout.tv_usec = (APP_DEMO_UDP_SOCKET_TIMEOUT % 1000) * 1000;

    if (setsockopt(aud_udp_client_service->aud_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        LOGW("setsockopt SO_RCVTIMEO failed\n");
    }

    aud_udp_client_service->aud_status = 1;
    aud_udp_client_service->chan_state = NTWK_TRANS_CHAN_CONNECTED;
    ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, aud_udp_client_service->server_address, NTWK_TRANS_CHAN_AUDIO);
#if 0
    LOGV("%s: connected to server %s:%d\n", __func__,
         inet_ntoa(*(struct in_addr *)&aud_udp_client_service->server_address),
         aud_udp_client_service->server_port);
    {
        struct sockaddr_in local_addr;
        socklen_t len = sizeof(local_addr);
        if (getsockname(aud_udp_client_service->aud_fd, (struct sockaddr *)&local_addr, &len) == 0) {
            LOGI("audio, connected: local_port=%u, peer_port=%u\n",
                 (unsigned)ntohs(local_addr.sin_port), (unsigned)aud_udp_client_service->server_port);
        }
    }
#endif
    while (aud_udp_client_service != NULL && aud_udp_client_service->aud_status)
    {
        // Check if stop was called
        if (aud_udp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
        {
            LOGV("%s, stop called, exiting\n", __func__);
            break;
        }

        FD_ZERO(&watchfd);
        FD_SET(aud_udp_client_service->aud_fd, &watchfd);

        ret = select(aud_udp_client_service->aud_fd + 1, &watchfd, NULL, NULL, &timeout);
        if (ret < 0)
        {
            // Check if stop was called
            if (aud_udp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
            {
                LOGD("%s, stop called during select\n", __func__);
                break;
            }
            LOGE("aud select ret:%d\n", ret);
            break;
        }
        else if (ret > 0)
        {
            if (FD_ISSET(aud_udp_client_service->aud_fd, &watchfd))
            {
                struct sockaddr_in from_addr;
                addr_len = sizeof(struct sockaddr_in);
                rcv_len = recvfrom(aud_udp_client_service->aud_fd, rcv_buf, NTWK_TRANS_DATA_MAX_SIZE, 0,
                                   (struct sockaddr *)&from_addr, &addr_len);

                if (rcv_len > 0)
                {
                    rcv_len = (rcv_len > NTWK_TRANS_DATA_MAX_SIZE) ? NTWK_TRANS_DATA_MAX_SIZE : rcv_len;
                    rcv_buf[rcv_len] = 0;
#if 1
                    {
                        struct sockaddr_in local_addr;
                        socklen_t local_len = sizeof(local_addr);
                        if (getsockname(aud_udp_client_service->aud_fd, (struct sockaddr *)&local_addr, &local_len) == 0) {
                            LOGI("audio recv: len=%d, local_port=%u, peer_port=%u\n",
                                 rcv_len, (unsigned)ntohs(local_addr.sin_port), (unsigned)ntohs(from_addr.sin_port));
                        }
                    }
#endif
                    ntwk_udp_audio_client_receive_data(rcv_buf, rcv_len);
                }
                else if (rcv_len < 0)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        // Check if stop was called before setting disconnected state
                        if (aud_udp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
                        {
                            LOGV("%s, stop called during recvfrom\n", __func__);
                            break;
                        }
                        LOGE("aud client recvfrom error: %d\n", errno);
                        aud_udp_client_service->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;
                        ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, errno, NTWK_TRANS_CHAN_AUDIO);
                        break;
                    }
                }
            }
        }
    }

out:
    if (aud_udp_client_service != NULL) {
        LOGE("%s exit %d\n", __func__, aud_udp_client_service->aud_status);

        if (rcv_buf) {
            os_free(rcv_buf);
            rcv_buf = NULL;
        }

        aud_udp_client_service->aud_status = 0;

        if (aud_udp_client_service->aud_fd != -1) {
            close(aud_udp_client_service->aud_fd);
            aud_udp_client_service->aud_fd = -1;
        }
    } else {
        if (rcv_buf) {
            os_free(rcv_buf);
            rcv_buf = NULL;
        }
    }

    rtos_delete_thread(NULL);
}

bk_err_t ntwk_udp_audio_client_chan_start(void *param)
{
    bk_err_t ret = BK_OK;
    ntwk_server_net_info_t *server_net_info = ntwk_trans_get_server_net_info();

    if (!server_net_info)
    {
        LOGE("server_net_info is NULL\n");
        return BK_FAIL;
    }

    LOGV("%s\n", __func__);

    if (aud_udp_client_service == NULL)
    {
        LOGE("aud_udp_client_service is NULL\n");
        return BK_FAIL;
    }

    aud_udp_client_service->server_address = inet_addr((char *)server_net_info->ip_addr);
    if (aud_udp_client_service->server_address == INADDR_NONE)
    {
        LOGE("Invalid IP address: %s\n", server_net_info->ip_addr);
        return BK_FAIL;
    }

    aud_udp_client_service->server_port = (uint16_t)atoi((char *)server_net_info->audio_port);
    if (aud_udp_client_service->server_port == 0)
    {
        LOGE("Invalid port: %s\n", server_net_info->audio_port);
        return BK_FAIL;
    }

    ret = rtos_create_thread(&aud_udp_client_service->thd,
                             4,
                             "ntwk_aud_cli",
                             (beken_thread_function_t)ntwk_udp_audio_client_thread,
                             1024 * 2,
                             (beken_thread_arg_t)NULL);
    if (ret != BK_OK)
    {
        LOGE("Error: Failed to create ntwk aud udp client: %d\n", ret);
    }

    return ret;
}

bk_err_t ntwk_udp_audio_client_chan_stop(void)
{
    LOGV("%s\n", __func__);

    if (aud_udp_client_service == NULL)
    {
        LOGE("%s, service null\n", __func__);
        return BK_FAIL;
    }

    aud_udp_client_service->aud_status = 0;
    aud_udp_client_service->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_AUDIO);

    if (aud_udp_client_service->aud_fd != -1)
    {
        close(aud_udp_client_service->aud_fd);
        aud_udp_client_service->aud_fd = -1;
    }

    if (aud_udp_client_service->thd != NULL)
    {
        rtos_thread_join(aud_udp_client_service->thd);
        aud_udp_client_service->thd = NULL;
    }

   // ntwk_udp_client_deinit(NTWK_TRANS_CHAN_AUDIO);
    LOGV("UDP audio client channel deinitialized\n");
    return BK_OK;
}

int ntwk_udp_audio_client_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type)
{
    if (aud_udp_client_service == NULL || !aud_udp_client_service->aud_status)
    {
        return -1;
    }

    return sendto(aud_udp_client_service->aud_fd, data, length, 0,
                 (struct sockaddr *)&aud_udp_client_service->server_addr,
                 sizeof(struct sockaddr_in));
}

bk_err_t ntwk_udp_audio_client_register_receive_cb(ntwk_udp_audio_receive_cb_t cb)
{
    if (aud_udp_client_service == NULL)
    {
        LOGE("%s: UDP Audio client channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    aud_udp_client_service->receive_cb = cb;
    LOGI("%s: UDP Audio receive callback registered successfully\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_udp_client_init(chan_type_t chan_type)
{
    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (ntwk_udp_ctrl_client_info != NULL)
            {
                LOGE("ntwk_udp_ctrl_client_info already initialized\n");
                return BK_FAIL;
            }
            ntwk_udp_ctrl_client_info = ntwk_malloc(sizeof(ntwk_udp_ctrl_client_info_t));
            if (ntwk_udp_ctrl_client_info == NULL)
            {
                LOGE("malloc ntwk_udp_ctrl_client_info failed\n");
                return BK_ERR_NO_MEM;
            }
            os_memset(ntwk_udp_ctrl_client_info, 0, sizeof(ntwk_udp_ctrl_client_info_t));
            ntwk_udp_ctrl_client_info->client_fd = -1;
        } break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (video_udp_client_service != NULL)
            {
                LOGE("video_udp_client_service already initialized\n");
                return BK_FAIL;
            }
            video_udp_client_service = ntwk_malloc(sizeof(video_udp_client_service_t));
            if (video_udp_client_service == NULL)
            {
                LOGE("video_udp_client_service malloc failed\n");
                return BK_FAIL;
            }
            os_memset(video_udp_client_service, 0, sizeof(video_udp_client_service_t));
            video_udp_client_service->video_fd = -1;
        } break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (aud_udp_client_service != NULL)
            {
                LOGE("aud_udp_client_service already initialized\n");
                return BK_FAIL;
            }
            aud_udp_client_service = ntwk_malloc(sizeof(aud_udp_client_service_t));
            if (aud_udp_client_service == NULL)
            {
                LOGE("aud_udp_client_service malloc failed\n");
                return BK_FAIL;
            }
            os_memset(aud_udp_client_service, 0, sizeof(aud_udp_client_service_t));
            aud_udp_client_service->aud_fd = -1;
        } break;
        default:
        {
            LOGE("invalid channel type\n");
            return BK_FAIL;
        } break;
    }

    return BK_OK;
}

bk_err_t ntwk_udp_client_deinit(chan_type_t chan_type)
{
    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (ntwk_udp_ctrl_client_info != NULL)
            {
                ntwk_udp_ctrl_client_chan_stop();
                os_free(ntwk_udp_ctrl_client_info);
                ntwk_udp_ctrl_client_info = NULL;
            }
        } break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (video_udp_client_service != NULL)
            {
                ntwk_udp_video_client_chan_stop();
                os_free(video_udp_client_service);
                video_udp_client_service = NULL;
            }
        } break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (aud_udp_client_service != NULL)
            {
                ntwk_udp_audio_client_chan_stop();
                os_free(aud_udp_client_service);
                aud_udp_client_service = NULL;
            }
        } break;
        default:
        {
            LOGE("invalid channel type\n");
            return BK_FAIL;
        } break;
    }

    return BK_OK;
}
#endif // CONFIG_NTWK_CLIENT_SERVICE_ENABLE