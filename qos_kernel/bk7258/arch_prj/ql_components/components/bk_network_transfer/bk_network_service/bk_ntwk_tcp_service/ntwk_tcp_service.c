#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>
#include <getopt.h>
#include "lwip/sockets.h"
#include "lwip/udp.h"
#include "net.h"
#include "string.h"
#include <components/netif.h>
#include <common/bk_generic.h>
#include <stdlib.h>
#include "network_type.h"
#include "bk_network_service/bk_ntwk_socket/ntwk_socket.h"
#include "network_transfer.h"
#include "ntwk_tcp_service.h"
#include "network_transfer_internal.h"

#define TAG "ntwk-tcp"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Common function for both server and client
static int ntwk_tcp_set_keepalive(int sockfd,  int keepalive, int keepidle, int keepintvl, int keepcnt)
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
// TCP Server global variables
static video_tcp_service_t *video_tcp_service = NULL;
static aud_tcp_service_t *aud_tcp_service = NULL;
static ntwk_tcp_ctrl_info_t *ntwl_tcp_ctrl_info = NULL;

// TCP Server implementation

// Video receive data handler
static void ntwk_tcp_video_receive_data(uint8_t *data, uint16_t length)
{
    // Directly call user registered receive callback if available
    if (video_tcp_service && video_tcp_service->receive_cb) {
        video_tcp_service->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
}

// Audio receive data handler
static void ntwk_tcp_audio_recive_data(uint8_t *data, uint16_t length)
{
    // Directly call user registered receive callback if available
    if (aud_tcp_service && aud_tcp_service->receive_cb) {
        aud_tcp_service->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
}

int ntwk_tcp_video_send_packet(uint8_t *data, uint32_t length, image_format_t video_type)
{
    if (video_tcp_service == NULL || !video_tcp_service->video_status)
    {
        return -1;
    }

    return ntwk_socket_sendto(&video_tcp_service->video_fd, (struct sockaddr *)&video_tcp_service->video_remote, data, length);
}

int ntwk_tcp_audio_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type)
{
    if (aud_tcp_service == NULL || !aud_tcp_service->aud_status)
    {
        return -1;
    }

    return ntwk_socket_sendto(&aud_tcp_service->aud_fd, (struct sockaddr *)&aud_tcp_service->aud_remote, data, length);
}

int ntwk_tcp_ctrl_chan_send(uint8_t *data, uint32_t length)
{
    int ret = 0;

    if (ntwl_tcp_ctrl_info->server_state == BK_FALSE)
    {
        LOGE("%s server not ready\n", __func__);
        return -1;
    }

    if (ntwl_tcp_ctrl_info->client_fd < 0)
    {
        LOGE("%s client not ready\n", __func__);
        return -1;
    }

    ret = ntwk_socket_write(&ntwl_tcp_ctrl_info->client_fd, data, length);

    return ret;
}

void ntwk_tcp_ctrl_receive_data(uint8_t *data, uint16_t length)
{
    LOGD("%s start\r\n", __func__);

    if (ntwl_tcp_ctrl_info && ntwl_tcp_ctrl_info->receive_cb) {
        ntwl_tcp_ctrl_info->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
    LOGD("%s end\r\n", __func__);
}

static void ntwk_tcp_ctrl_server_thread(beken_thread_arg_t data)
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
        LOGE("tcp malloc failed\n");
        goto out;
    }

    ntwl_tcp_ctrl_info->chan_state = NTWK_TRANS_CHAN_START;
    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0 , NTWK_TRANS_CHAN_CTRL);

    // for data transfer
    ntwl_tcp_ctrl_info->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ntwl_tcp_ctrl_info->server_fd == -1)
    {
        LOGE("socket failed\n");
        goto out;
    }

    ntwl_tcp_ctrl_info->socket.sin_family = AF_INET;
    ntwl_tcp_ctrl_info->socket.sin_port = htons(NTWK_TRANS_CMD_PORT);
    ntwl_tcp_ctrl_info->socket.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(ntwl_tcp_ctrl_info->server_fd, (struct sockaddr *)&ntwl_tcp_ctrl_info->socket, sizeof(struct sockaddr_in)) == -1)
    {
        LOGE("bind failed\n");
        goto out;
    }

    if (listen(ntwl_tcp_ctrl_info->server_fd, 0) == -1)
    {
        LOGE("listen failed\n");
        goto out;
    }

    LOGD("%s: start listen \n", __func__);

    while (1)
    {
        if (ntwl_tcp_ctrl_info->chan_state == NTWK_TRANS_CHAN_STOP ||
            ntwl_tcp_ctrl_info->server_fd < 0)
            break;

        FD_ZERO(&watchfd);
        FD_SET(ntwl_tcp_ctrl_info->server_fd, &watchfd);

        ntwl_tcp_ctrl_info->chan_state = NTWK_TRANS_CHAN_WAITING_CONNECTED;
        LOGD("%s, waiting for a new connection\n", __func__);
        ret = select(ntwl_tcp_ctrl_info->server_fd + 1, &watchfd, NULL, NULL, NULL);
        if (ret <= 0)
        {
            if (ntwl_tcp_ctrl_info->chan_state == NTWK_TRANS_CHAN_STOP)
                break;
            LOGE("select ret:%d\n", ret);
            continue;
        }
        else
        {
            // is new connection
            if (FD_ISSET(ntwl_tcp_ctrl_info->server_fd, &watchfd))
            {
                struct sockaddr_in client_addr;
                socklen_t cliaddr_len = 0;

                cliaddr_len = sizeof(client_addr);

                ntwl_tcp_ctrl_info->client_fd = accept(ntwl_tcp_ctrl_info->server_fd, (struct sockaddr *)&client_addr, &cliaddr_len);

                if (ntwl_tcp_ctrl_info->client_fd < 0)
                {
                    LOGE("accept return fd:%d\n", ntwl_tcp_ctrl_info->client_fd);
                    break;
                }

                uint8_t *src_ipaddr = (UINT8 *)&client_addr.sin_addr.s_addr;
                ntwl_tcp_ctrl_info->chan_state = NTWK_TRANS_CHAN_CONNECTED;
                LOGD("accept a new connection fd:%d, %d.%d.%d.%d\n", ntwl_tcp_ctrl_info->client_fd, src_ipaddr[0], src_ipaddr[1],
                     src_ipaddr[2], src_ipaddr[3]);

                ntwk_tcp_set_keepalive(ntwl_tcp_ctrl_info->client_fd,
                                        NTWK_TRANS_CTRL_CHAN_KEEPALIVE_ENABLE,
                                        NTWK_TRANS_CTRL_CHAN_KEEPALIVE_IDLE_TIME,
                                        NTWK_TRANS_CTRL_CHAN_KEEPALIVE_INTERVAL,
                                        NTWK_TRANS_CTRL_CHAN_KEEPALIVE_COUNT);
                ntwl_tcp_ctrl_info->remote_address = client_addr.sin_addr.s_addr;
                ntwk_socket_set_qos(ntwl_tcp_ctrl_info->client_fd, IP_QOS_PRIORITY_HIGHEST);

                if (ntwl_tcp_ctrl_info->server_state == BK_FALSE)
                {
                    ntwl_tcp_ctrl_info->server_state = BK_TRUE;
                    ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, ntwl_tcp_ctrl_info->remote_address, NTWK_TRANS_CHAN_CTRL);
                }

                while (ntwl_tcp_ctrl_info->server_state == BK_TRUE)
                {
                    rcv_len = recv(ntwl_tcp_ctrl_info->client_fd, rcv_buf, NTWK_TRANS_CMD_BUFFER, 0);
                    if (rcv_len > 0)
                    {
                        //bk_net_send_data(rcv_buf, rcv_len, TVIDEO_SND_TCP);
                        LOGD("%s, got length: %d\n", __func__, rcv_len);
                        ntwk_tcp_ctrl_receive_data(rcv_buf, rcv_len);
                    }
                    else
                    {
                        // close this socket
                        LOGD("%s, recv close fd:%d, rcv_len:%d, error:%d\n", __func__, ntwl_tcp_ctrl_info->client_fd, rcv_len, errno);
                        close(ntwl_tcp_ctrl_info->client_fd);
                        ntwl_tcp_ctrl_info->client_fd = -1;
                        ntwl_tcp_ctrl_info->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;

                        if (ntwl_tcp_ctrl_info->server_state == BK_TRUE)
                        {
                            ntwl_tcp_ctrl_info->server_state = BK_FALSE;
                            ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0 , NTWK_TRANS_CHAN_CTRL);
                        }

                        break;
                    }
                }
            }
        }
    }
out:

    LOGE("%s exit %d\n", __func__, ntwl_tcp_ctrl_info->server_state);
    if (rcv_buf)
    {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    ntwl_tcp_ctrl_info->server_state = BK_FALSE;

    if (ntwl_tcp_ctrl_info->client_fd != -1)
    {
        close(ntwl_tcp_ctrl_info->client_fd);
        ntwl_tcp_ctrl_info->client_fd = -1;
    }

    if (ntwl_tcp_ctrl_info->server_fd != -1)
    {
        close(ntwl_tcp_ctrl_info->server_fd);
        ntwl_tcp_ctrl_info->server_fd = -1;
    }

    rtos_delete_thread(NULL);
}

in_addr_t ntwk_tcp_ctrl_get_socket_address(void)
{
    return ntwl_tcp_ctrl_info->remote_address;
}

bk_err_t ntwk_tcp_ctrl_chan_start(void *param)
{
    bk_err_t ret = BK_OK;

    if (ntwl_tcp_ctrl_info == NULL)
    {
        LOGE("ntwl_tcp_ctrl_info is NULL\n");
        return BK_FAIL;
    }

    if (!ntwl_tcp_ctrl_info->thread)
    {
        ret = rtos_create_thread(&ntwl_tcp_ctrl_info->thread,
                                 4,
                                 "ntwl_tcp_ctrl_srv",
                                 (beken_thread_function_t)ntwk_tcp_ctrl_server_thread,
                                 1024 * 4,
                                 (beken_thread_arg_t)NULL);
        if (ret != kNoErr)
        {
            LOGE("Error: failed to create tcp cmd server: %d\n", ret);
        }
    }

    return ret;
}

bk_err_t ntwk_tcp_ctrl_chan_stop(void)
{
    if (ntwl_tcp_ctrl_info == NULL)
    {
        LOGE("ntwl_tcp_ctrl_info is NULL, nothing to deinit\n");
        return BK_FAIL;
    }

    ntwl_tcp_ctrl_info->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0 , NTWK_TRANS_CHAN_CTRL);

    if (ntwl_tcp_ctrl_info->client_fd != -1)
    {
        close(ntwl_tcp_ctrl_info->client_fd);
        ntwl_tcp_ctrl_info->client_fd = -1;
    }

    if (ntwl_tcp_ctrl_info->server_fd != -1)
    {
        close(ntwl_tcp_ctrl_info->server_fd);
        ntwl_tcp_ctrl_info->server_fd = -1;
    }

    if (ntwl_tcp_ctrl_info->thread != NULL)
    {
        rtos_thread_join(ntwl_tcp_ctrl_info->thread);
        ntwl_tcp_ctrl_info->thread = NULL;
    }

    ntwl_tcp_ctrl_info->receive_cb = NULL;

    LOGD("%s end\n", __func__);
    return BK_OK;
}

bk_err_t ntwk_tcp_ctrl_register_receive_cb(ntwk_tcp_ctrl_receive_cb_t cb)
{
    if (ntwl_tcp_ctrl_info == NULL)
    {
        LOGE("%s: Control channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwl_tcp_ctrl_info->receive_cb = cb;
    LOGD("%s: Receive callback registered successfully\n", __func__);

    return BK_OK;
}

static void ntwk_tcp_video_server_thread(beken_thread_arg_t data)
{
    int rcv_len = 0;
    //  struct sockaddr_in server;
    bk_err_t ret = BK_OK;
    u8 *rcv_buf = NULL;
    fd_set watchfd;

    LOGD("%s entry\n", __func__);
    (void)(data);

    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_VIDEO);
    video_tcp_service->chan_state = NTWK_TRANS_CHAN_START;

    rcv_buf = (u8 *) ntwk_malloc((NTWK_TCP_BUFFER + 1) * sizeof(u8));

    if (!rcv_buf)
    {
        LOGE("tcp malloc failed\n");
        goto out;
    }

    // for data transfer
    video_tcp_service->video_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (video_tcp_service->video_server_fd == -1)
    {
        LOGE("socket failed\n");
        goto out;
    }

    video_tcp_service->video_socket.sin_family = AF_INET;
    video_tcp_service->video_socket.sin_port = htons(NTWK_TRANS_TCP_VIDEO_PORT);
    video_tcp_service->video_socket.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(video_tcp_service->video_server_fd, (struct sockaddr *)&video_tcp_service->video_socket, sizeof(struct sockaddr_in)) == -1)
    {
        LOGE("bind failed\n");
        goto out;
    }

    if (listen(video_tcp_service->video_server_fd, 0) == -1)
    {
        LOGE("listen failed\n");
        goto out;
    }

    LOGD("%s: start listen \n", __func__);

    while (1)
    {
        if (video_tcp_service->chan_state == NTWK_TRANS_CHAN_STOP ||
            video_tcp_service->video_server_fd < 0)
            break;

        FD_ZERO(&watchfd);
        FD_SET(video_tcp_service->video_server_fd, &watchfd);
        video_tcp_service->chan_state = NTWK_TRANS_CHAN_WAITING_CONNECTED;

        LOGD("%s, waiting for a new connection\n", __func__);
        ret = select(video_tcp_service->video_server_fd + 1, &watchfd, NULL, NULL, NULL);
        if (ret <= 0)
        {
            if (video_tcp_service->chan_state == NTWK_TRANS_CHAN_STOP)
                break;
            LOGE("select ret:%d\n", ret);
            continue;
        }
        else
        {
            // is new connection
            if (FD_ISSET(video_tcp_service->video_server_fd, &watchfd))
            {
                struct sockaddr_in client_addr;
                socklen_t cliaddr_len = 0;

                cliaddr_len = sizeof(client_addr);


                video_tcp_service->video_fd = accept(video_tcp_service->video_server_fd, (struct sockaddr *)&client_addr, &cliaddr_len);

                if (video_tcp_service->video_fd < 0)
                {
                    LOGE("accept return fd:%d\n", video_tcp_service->video_fd);
                    break;
                }
                video_tcp_service->chan_state = NTWK_TRANS_CHAN_CONNECTED;

                LOGD("accept a new connection fd:%d\n", video_tcp_service->video_fd);

                ntwk_tcp_set_keepalive(video_tcp_service->video_fd,
                                        NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_ENABLE,
                                        NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_IDLE_TIME,
                                        NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_INTERVAL,
                                        NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_COUNT);

                os_memcpy(&video_tcp_service->video_remote, &client_addr, sizeof(struct sockaddr_in));

                ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, client_addr.sin_addr.s_addr,NTWK_TRANS_CHAN_VIDEO);

                ntwk_socket_set_qos(video_tcp_service->video_fd, IP_QOS_PRIORITY_LOW);

                video_tcp_service->video_status = BK_TRUE;
                while (video_tcp_service->video_status == BK_TRUE)
                {
                    rcv_len = recv(video_tcp_service->video_fd, rcv_buf, NTWK_TCP_BUFFER, 0);
                    if (rcv_len > 0)
                    {
                        ntwk_tcp_video_receive_data(rcv_buf, rcv_len);
                        //LOGD("got length: %d\n", rcv_len);
                    }
                    else
                    {
                        // close this socket
                        int errno_temp = errno;

                        LOGD("vid recv close fd:%d, rcv_len:%d, error_code:%d\n", video_tcp_service->video_fd, rcv_len, errno);
                        close(video_tcp_service->video_fd);
                        video_tcp_service->video_fd = -1;
                        video_tcp_service->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;

                        if (errno_temp == ENOTCONN) {
                            ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, errno_temp, NTWK_TRANS_CHAN_VIDEO);
                        } else {
                            ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0, NTWK_TRANS_CHAN_VIDEO);
                        }

                        break;
                    }

                }
            }
        }
    }
out:

    LOGE("%s exit %d\n", __func__, video_tcp_service->chan_state);

    if (rcv_buf)
    {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    video_tcp_service->video_status = BK_FALSE;

    if (video_tcp_service->video_fd != -1)
    {
        close(video_tcp_service->video_fd);
        video_tcp_service->video_fd = -1;
    }

    rtos_delete_thread(NULL);
}

bk_err_t ntwk_tcp_video_chan_start(void *param)
{
    bk_err_t ret = BK_OK;

    LOGD("%s, %d\n", __func__, __LINE__);

    if (video_tcp_service == NULL)
    {
        LOGE("video_tcp_service is NULL\n");
        return BK_FAIL;
    }

    if (!video_tcp_service->video_thd)
    {
        ret = rtos_create_thread(&video_tcp_service->video_thd,
                                 4,
                                 "ntwk_tcp_video_srv",
                                 (beken_thread_function_t)ntwk_tcp_video_server_thread,
                                 1024 * 3,
                                 (beken_thread_arg_t)NULL);
        if (ret != kNoErr)
        {
            LOGE("%s, rtos_create_thread failed, ret:%d\n", __func__, ret);
        }
    }

    return ret;
}

bk_err_t ntwk_tcp_video_chan_stop(void)
{
    LOGD("%s, %d\n", __func__, __LINE__);

    if (video_tcp_service == NULL)
    {
        LOGE("%s: video_tcp_service is NULL\n", __func__);
        return BK_FAIL;
    }

    video_tcp_service->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_VIDEO);

    video_tcp_service->video_status = BK_FALSE;

    if (video_tcp_service->video_server_fd != -1)
    {
        close(video_tcp_service->video_server_fd);
        video_tcp_service->video_server_fd = -1;
    }

    if (video_tcp_service->video_thd)
    {
        rtos_thread_join(&video_tcp_service->video_thd);
        video_tcp_service->video_thd = NULL;
    }

    return BK_OK;
}


static void ntwk_tcp_audio_server_thread(beken_thread_arg_t data)
{
    int rcv_len = 0;
    //  struct sockaddr_in server;
    bk_err_t ret = BK_OK;
    u8 *rcv_buf = NULL;
    fd_set watchfd;

    LOGD("%s entry\n", __func__);
    (void)(data);

    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_AUDIO);
    aud_tcp_service->chan_state = NTWK_TRANS_CHAN_START;
    rcv_buf = (u8 *) ntwk_malloc((NTWK_TCP_BUFFER + 1) * sizeof(u8));

    if (!rcv_buf)
    {
        LOGE("tcp malloc failed\n");
        goto out;
    }

    // for data transfer
    aud_tcp_service->aud_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (aud_tcp_service->aud_server_fd == -1)
    {
        LOGE("socket failed\n");
        goto out;
    }

    aud_tcp_service->aud_socket.sin_family = AF_INET;
    aud_tcp_service->aud_socket.sin_port = htons(NTWK_TRANS_TCP_AUDIO_PORT);
    aud_tcp_service->aud_socket.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(aud_tcp_service->aud_server_fd, (struct sockaddr *)&aud_tcp_service->aud_socket, sizeof(struct sockaddr_in)) == -1)
    {
        LOGE("bind failed\n");
        goto out;
    }

    if (listen(aud_tcp_service->aud_server_fd, 0) == -1)
    {
        LOGE("listen failed\n");
        goto out;
    }

    LOGD("%s: start listen \n", __func__);

    while (1)
    {
        if (aud_tcp_service->chan_state == NTWK_TRANS_CHAN_STOP ||
            aud_tcp_service->aud_server_fd < 0)
            break;

        FD_ZERO(&watchfd);
        FD_SET(aud_tcp_service->aud_server_fd, &watchfd);
        aud_tcp_service->chan_state = NTWK_TRANS_CHAN_WAITING_CONNECTED;

        LOGD("%s, waiting for a new connection\n", __func__);
        ret = select(aud_tcp_service->aud_server_fd + 1, &watchfd, NULL, NULL, NULL);
        if (ret <= 0)
        {
            if (aud_tcp_service->chan_state == NTWK_TRANS_CHAN_STOP)
                break;
            LOGE("select ret:%d\n", ret);
            continue;
        }
        else
        {
            // is new connection
            if (FD_ISSET(aud_tcp_service->aud_server_fd, &watchfd))
            {
                struct sockaddr_in client_addr;
                socklen_t cliaddr_len = 0;

                cliaddr_len = sizeof(client_addr);


                aud_tcp_service->aud_fd = accept(aud_tcp_service->aud_server_fd, (struct sockaddr *)&client_addr, &cliaddr_len);

                if (aud_tcp_service->aud_fd < 0)
                {
                    LOGE("accept return fd:%d\n", aud_tcp_service->aud_fd);
                    break;
                }
                aud_tcp_service->chan_state = NTWK_TRANS_CHAN_CONNECTED;

                LOGD("accept a new connection fd:%d\n", aud_tcp_service->aud_fd);

                ntwk_tcp_set_keepalive(aud_tcp_service->aud_fd,
                                        NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_ENABLE,
                                        NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_IDLE_TIME,
                                        NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_INTERVAL,
                                        NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_COUNT);

                aud_tcp_service->aud_remote = client_addr;
                os_memcpy(&aud_tcp_service->aud_remote, &client_addr, sizeof(struct sockaddr_in));
                ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, client_addr.sin_addr.s_addr, NTWK_TRANS_CHAN_AUDIO);

                aud_tcp_service->aud_status = BK_TRUE;

                ntwk_socket_set_qos(aud_tcp_service->aud_fd, IP_QOS_PRIORITY_HIGH);

                while (aud_tcp_service->aud_status == BK_TRUE)
                {
                    rcv_len = recv(aud_tcp_service->aud_fd, rcv_buf, NTWK_TCP_BUFFER, 0);
                    if (rcv_len > 0)
                    {
                        //LOGD("got length: %d\n", rcv_len);
                        ntwk_tcp_audio_recive_data(rcv_buf, rcv_len);
                    }
                    else
                    {
                        int errno_temp = errno;
                        // close this socket
                        LOGD("aud recv close fd:%d, rcv_len:%d, error_code:%d\n", aud_tcp_service->aud_fd, rcv_len, errno);
                        close(aud_tcp_service->aud_fd);
                        aud_tcp_service->aud_fd = -1;
                        aud_tcp_service->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;

                        if (errno_temp == ENOTCONN) {
                            ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, errno_temp, NTWK_TRANS_CHAN_AUDIO);
                        } else {
                            ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0, NTWK_TRANS_CHAN_AUDIO);
                        }

                        break;
                    }

                }
            }
        }
    }
out:

    LOGE("%s exit %d\n", __func__, aud_tcp_service->aud_status);

    if (rcv_buf)
    {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    aud_tcp_service->aud_status = BK_FALSE;

    if (aud_tcp_service->aud_fd != -1)
    {
        close(aud_tcp_service->aud_fd);
        aud_tcp_service->aud_fd = -1;
    }

    rtos_delete_thread(NULL);
}

bk_err_t ntwk_tcp_audio_chan_start(void *param)
{
    bk_err_t ret = BK_OK;

    if (aud_tcp_service == NULL)
    {
        LOGE("aud_tcp_service is NULL\n");
        return BK_FAIL;
    }

    if (!aud_tcp_service->aud_thd)
    {
        ret = rtos_create_thread(&aud_tcp_service->aud_thd,
                                 4,
                                 "ntwk_tcp_aud_srv",
                                 (beken_thread_function_t)ntwk_tcp_audio_server_thread,
                                 1024 * 3,
                                 (beken_thread_arg_t)NULL);
        if (ret != kNoErr)
        {
            LOGE("Error: failed to create doorbell tcp aud server: %d\n", ret);
        }
    }

    return ret;
}

bk_err_t ntwk_tcp_audio_chan_stop(void)
{
    if (aud_tcp_service == NULL)
    {
        LOGE("aud_tcp_service is NULL\n");
        return BK_FAIL;
    }

    aud_tcp_service->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_AUDIO);

    if (aud_tcp_service->aud_server_fd != -1)
    {
        close(aud_tcp_service->aud_server_fd);
        aud_tcp_service->aud_server_fd = -1;
    }

    if (aud_tcp_service->aud_thd)
    {
        rtos_thread_join(&aud_tcp_service->aud_thd);
        aud_tcp_service->aud_thd = NULL;
    }

    return BK_OK;
}

bk_err_t ntwk_tcp_video_register_receive_cb(ntwk_video_receive_cb_t cb)
{
    if (video_tcp_service == NULL)
    {
        LOGE("%s: Video channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    video_tcp_service->receive_cb = cb;
    LOGD("%s: Video receive callback registered successfully\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_tcp_audio_register_receive_cb(ntwk_audio_receive_cb_t cb)
{
    if (aud_tcp_service == NULL)
    {
        LOGE("%s: Audio channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    aud_tcp_service->receive_cb = cb;
    LOGD("%s: Audio receive callback registered successfully\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_tcp_init(chan_type_t chan_type)
{
    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (ntwl_tcp_ctrl_info != NULL)
            {
                LOGE("%s: ntwl_tcp_ctrl_info already initialized\n", __func__);
                return BK_OK;
            }
            ntwl_tcp_ctrl_info = ntwk_malloc(sizeof(ntwk_tcp_ctrl_info_t));
            if (ntwl_tcp_ctrl_info == NULL)
            {
                LOGE("%s: malloc ntwl_tcp_ctrl_info failed\n", __func__);
                return BK_FAIL;
            }
            os_memset(ntwl_tcp_ctrl_info, 0, sizeof(ntwk_tcp_ctrl_info_t));
        } break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (video_tcp_service != NULL)
            {
                LOGE("%s: video_tcp_service already initialized\n", __func__);
                return BK_OK;
            }
            video_tcp_service = ntwk_malloc(sizeof(video_tcp_service_t));
            if (video_tcp_service == NULL)
            {
                LOGE("%s: malloc video_tcp_service failed\n", __func__);
                return BK_FAIL;
            }
            os_memset(video_tcp_service, 0, sizeof(video_tcp_service_t));
        } break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (aud_tcp_service != NULL)
            {
                LOGE("%s: aud_tcp_service already initialized\n", __func__);
                return BK_OK;
            }
            aud_tcp_service = ntwk_malloc(sizeof(aud_tcp_service_t));
            if (aud_tcp_service == NULL)
            {
                LOGE("%s: malloc aud_tcp_service failed\n", __func__);
                return BK_FAIL;
            }
            os_memset(aud_tcp_service, 0, sizeof(aud_tcp_service_t));
        } break;
        default:
            LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
            return BK_ERR_PARAM;
    }
    return BK_OK;
}

bk_err_t ntwk_tcp_deinit(chan_type_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (ntwl_tcp_ctrl_info != NULL)
            {
                os_free(ntwl_tcp_ctrl_info);
                ntwl_tcp_ctrl_info = NULL;
            }
        } break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (video_tcp_service != NULL)
            {
                os_free(video_tcp_service);
                video_tcp_service = NULL;
            }
        } break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (aud_tcp_service != NULL)
            {
                os_free(aud_tcp_service);
                aud_tcp_service = NULL;
            }
        } break;
        default:
            LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
            return BK_ERR_PARAM;
    }

    return BK_OK;
}
#else // CONFIG_NTWK_CLIENT_SERVICE_ENABLE
// TCP Client implementation
static video_tcp_client_service_t *video_tcp_client_service = NULL;
static aud_tcp_client_service_t *aud_tcp_client_service = NULL;
static ntwk_tcp_ctrl_client_info_t *ntwk_tcp_ctrl_client_info = NULL;

// TCP Client control channel receive data handler
static void ntwk_tcp_ctrl_client_receive_data(uint8_t *data, uint16_t length)
{
    LOGV("%s start\r\n", __func__);

    if (ntwk_tcp_ctrl_client_info && ntwk_tcp_ctrl_client_info->receive_cb) {
        ntwk_tcp_ctrl_client_info->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
    LOGV("%s end\r\n", __func__);
}

// TCP Client control channel thread
static void ntwk_tcp_ctrl_client_thread(beken_thread_arg_t data)
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

    ntwk_tcp_ctrl_client_info->chan_state = NTWK_TRANS_CHAN_START;
    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_CTRL);

    while (1)
    {
        // Check if stop was called
        if (ntwk_tcp_ctrl_client_info->chan_state == NTWK_TRANS_CHAN_STOP)
        {
            LOGD("%s, stop called, exiting\n", __func__);
            break;
        }

        // Create socket
        ntwk_tcp_ctrl_client_info->client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (ntwk_tcp_ctrl_client_info->client_fd == -1)
        {
            LOGE("socket failed\n");
            goto out;
        }

        // Set server address
        ntwk_tcp_ctrl_client_info->server_addr.sin_family = AF_INET;
        ntwk_tcp_ctrl_client_info->server_addr.sin_port = htons(ntwk_tcp_ctrl_client_info->server_port);
        ntwk_tcp_ctrl_client_info->server_addr.sin_addr.s_addr = ntwk_tcp_ctrl_client_info->server_address;

        ntwk_tcp_ctrl_client_info->chan_state = NTWK_TRANS_CHAN_WAITING_CONNECTED;
        LOGV("tcp-ctrl, connecting to server %s:%d\n",
             inet_ntoa(*(struct in_addr *)&ntwk_tcp_ctrl_client_info->server_address),
             ntwk_tcp_ctrl_client_info->server_port);

        // Connect to server
        ret = connect(ntwk_tcp_ctrl_client_info->client_fd,
                     (struct sockaddr *)&ntwk_tcp_ctrl_client_info->server_addr,
                     sizeof(struct sockaddr_in));
        if (ret < 0)
        {
            LOGE("connect failed: %d\n", errno);
            close(ntwk_tcp_ctrl_client_info->client_fd);
            ntwk_tcp_ctrl_client_info->client_fd = -1;

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
        ntwk_tcp_ctrl_client_info->chan_state = NTWK_TRANS_CHAN_CONNECTED;
        ntwk_tcp_ctrl_client_info->client_state = BK_TRUE;

        LOGD("ctrl, Connected to server fd:%d\n", ntwk_tcp_ctrl_client_info->client_fd);

        ntwk_tcp_set_keepalive(ntwk_tcp_ctrl_client_info->client_fd,
                               NTWK_TRANS_CTRL_CHAN_KEEPALIVE_ENABLE,
                               NTWK_TRANS_CTRL_CHAN_KEEPALIVE_IDLE_TIME,
                               NTWK_TRANS_CTRL_CHAN_KEEPALIVE_INTERVAL,
                               NTWK_TRANS_CTRL_CHAN_KEEPALIVE_COUNT);
        ntwk_socket_set_qos(ntwk_tcp_ctrl_client_info->client_fd, IP_QOS_PRIORITY_HIGHEST);

        ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, ntwk_tcp_ctrl_client_info->server_address, NTWK_TRANS_CHAN_CTRL);

        // Receive data loop
        while (ntwk_tcp_ctrl_client_info->client_state == BK_TRUE)
        {
            rcv_len = recv(ntwk_tcp_ctrl_client_info->client_fd, rcv_buf, NTWK_TRANS_CMD_BUFFER, 0);
            if (rcv_len > 0)
            {
                LOGD("got length: %d\n", rcv_len);
                ntwk_tcp_ctrl_client_receive_data(rcv_buf, rcv_len);
            }
            else
            {
                LOGD("recv close fd:%d, rcv_len:%d, error:%d\n",
                     ntwk_tcp_ctrl_client_info->client_fd, rcv_len, errno);
                close(ntwk_tcp_ctrl_client_info->client_fd);
                ntwk_tcp_ctrl_client_info->client_fd = -1;
                ntwk_tcp_ctrl_client_info->client_state = BK_FALSE;
                
                // Check if stop was called before setting disconnected state
                if (ntwk_tcp_ctrl_client_info->chan_state == NTWK_TRANS_CHAN_STOP)
                {
                    // Stop was called, exit the thread
                    break;
                }
                
                ntwk_tcp_ctrl_client_info->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;
                ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0, NTWK_TRANS_CHAN_CTRL);

                // Try to reconnect
                rtos_delay_milliseconds(1000);
                break;
            }
        }
    }

out:
    LOGE("%s exit %d\n", __func__, ntwk_tcp_ctrl_client_info->client_state);
    if (rcv_buf)
    {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    ntwk_tcp_ctrl_client_info->client_state = BK_FALSE;

    if (ntwk_tcp_ctrl_client_info->client_fd != -1)
    {
        close(ntwk_tcp_ctrl_client_info->client_fd);
        ntwk_tcp_ctrl_client_info->client_fd = -1;
    }

    rtos_delete_thread(NULL);
}

bk_err_t ntwk_tcp_ctrl_client_chan_start(void *param)
{
    bk_err_t ret = BK_OK;
    ntwk_server_net_info_t *server_net_info = ntwk_trans_get_server_net_info();

    if (!server_net_info)
    {
        LOGE("server_net_info is NULL\n");
        return BK_FAIL;
    }

    if (ntwk_tcp_ctrl_client_info == NULL)
    {
        LOGE("ntwk_tcp_ctrl_client_info is NULL\n");
        return BK_FAIL;
    }


    ntwk_tcp_ctrl_client_info->server_address = inet_addr((char *)server_net_info->ip_addr);
    if (ntwk_tcp_ctrl_client_info->server_address == INADDR_NONE)
    {
        LOGE("Invalid IP address: %s\n", server_net_info->ip_addr);
        return BK_FAIL;
    }

    ntwk_tcp_ctrl_client_info->server_port = (uint16_t)atoi((char *)server_net_info->cmd_port);
    if (ntwk_tcp_ctrl_client_info->server_port == 0)
    {
        LOGE("Invalid port: %s\n", server_net_info->cmd_port);
        return BK_FAIL;
    }

    if (!ntwk_tcp_ctrl_client_info->thread)
    {
        ret = rtos_create_thread(&ntwk_tcp_ctrl_client_info->thread,
                                 4,
                                 "ntwk_tcp_ctrl_cli",
                                 (beken_thread_function_t)ntwk_tcp_ctrl_client_thread,
                                 1024 * 4,
                                 (beken_thread_arg_t)NULL);
        if (ret != kNoErr)
        {
            LOGE("Error: failed to create tcp ctrl client: %d\n", ret);
        }
    }

    return ret;
}

bk_err_t ntwk_tcp_ctrl_client_chan_stop(void)
{
    if (ntwk_tcp_ctrl_client_info == NULL)
    {
        LOGE("ntwk_tcp_ctrl_client_info is NULL, nothing to deinit\n");
        return BK_FAIL;
    }

    ntwk_tcp_ctrl_client_info->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_CTRL);

    ntwk_tcp_ctrl_client_info->client_state = BK_FALSE;

    if (ntwk_tcp_ctrl_client_info->client_fd != -1)
    {
        close(ntwk_tcp_ctrl_client_info->client_fd);
        ntwk_tcp_ctrl_client_info->client_fd = -1;
    }

    if (ntwk_tcp_ctrl_client_info->thread != NULL)
    {
        rtos_thread_join(ntwk_tcp_ctrl_client_info->thread);
        ntwk_tcp_ctrl_client_info->thread = NULL;
    }

    ntwk_tcp_ctrl_client_info->receive_cb = NULL;

    LOGD("%s end\n", __func__);
    return BK_OK;
}

int ntwk_tcp_ctrl_client_chan_send(uint8_t *data, uint32_t length)
{
    int ret = 0;

    if (ntwk_tcp_ctrl_client_info == NULL)
    {
        LOGE("%s client info is NULL\n", __func__);
        return -1;
    }

    if (ntwk_tcp_ctrl_client_info->client_state == BK_FALSE)
    {
        LOGE("%s client not connected\n", __func__);
        return -1;
    }

    if (ntwk_tcp_ctrl_client_info->client_fd < 0)
    {
        LOGE("%s client fd invalid\n", __func__);
        return -1;
    }

    ret = ntwk_socket_write(&ntwk_tcp_ctrl_client_info->client_fd, data, length);

    return ret;
}

bk_err_t ntwk_tcp_ctrl_client_register_receive_cb(ntwk_tcp_ctrl_receive_cb_t cb)
{
    if (ntwk_tcp_ctrl_client_info == NULL)
    {
        LOGE("%s: Control client channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwk_tcp_ctrl_client_info->receive_cb = cb;
    LOGV("%s: Receive callback registered successfully\n", __func__);

    return BK_OK;
}

// TCP Client video channel receive data handler
static void ntwk_tcp_video_client_receive_data(uint8_t *data, uint16_t length)
{
    if (video_tcp_client_service && video_tcp_client_service->receive_cb) {
        video_tcp_client_service->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
}

// TCP Client video channel thread
static void ntwk_tcp_video_client_thread(beken_thread_arg_t data)
{
    int rcv_len = 0;
    bk_err_t ret = BK_OK;
    u8 *rcv_buf = NULL;
    int connect_retry = 0;
    const int max_retry = 5;

    LOGV("%s entry\n", __func__);
    (void)(data);

    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_VIDEO);
    video_tcp_client_service->chan_state = NTWK_TRANS_CHAN_START;

    rcv_buf = (u8 *) ntwk_malloc((NTWK_TCP_BUFFER + 1) * sizeof(u8));
    if (!rcv_buf)
    {
        LOGE("tcp video client malloc failed\n");
        goto out;
    }

    while (1)
    {
        // Check if stop was called
        if (video_tcp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
        {
            LOGV("%s, stop called, exiting\n", __func__);
            break;
        }

        // Create socket
        video_tcp_client_service->video_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (video_tcp_client_service->video_fd == -1)
        {
            LOGE("socket failed\n");
            goto out;
        }

        // Set server address
        video_tcp_client_service->server_addr.sin_family = AF_INET;
        video_tcp_client_service->server_addr.sin_port = htons(video_tcp_client_service->server_port);
        video_tcp_client_service->server_addr.sin_addr.s_addr = video_tcp_client_service->server_address;

        video_tcp_client_service->chan_state = NTWK_TRANS_CHAN_WAITING_CONNECTED;
        LOGV("connecting to server %s:%d\n",
             inet_ntoa(*(struct in_addr *)&video_tcp_client_service->server_address),
             video_tcp_client_service->server_port);

        // Connect to server
        ret = connect(video_tcp_client_service->video_fd,
                     (struct sockaddr *)&video_tcp_client_service->server_addr,
                     sizeof(struct sockaddr_in));
        if (ret < 0)
        {
            LOGE("connect failed: %d\n", errno);
            close(video_tcp_client_service->video_fd);
            video_tcp_client_service->video_fd = -1;

            // Check if stop was called during retry
            if (video_tcp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
            {
                LOGD("%s, stop called during connect retry\n", __func__);
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
        video_tcp_client_service->chan_state = NTWK_TRANS_CHAN_CONNECTED;
        LOGD("video, Connected to server fd:%d\n", video_tcp_client_service->video_fd);

        ntwk_tcp_set_keepalive(video_tcp_client_service->video_fd,
                               NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_ENABLE,
                               NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_IDLE_TIME,
                               NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_INTERVAL,
                               NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_COUNT);

        ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, video_tcp_client_service->server_address, NTWK_TRANS_CHAN_VIDEO);
        ntwk_socket_set_qos(video_tcp_client_service->video_fd, IP_QOS_PRIORITY_LOW);

        video_tcp_client_service->video_status = BK_TRUE;

        // Receive data loop
        while (video_tcp_client_service->video_status == BK_TRUE)
        {
            rcv_len = recv(video_tcp_client_service->video_fd, rcv_buf, NTWK_TCP_BUFFER, 0);
            if (rcv_len > 0)
            {
                ntwk_tcp_video_client_receive_data(rcv_buf, rcv_len);
            }
            else
            {
                int errno_temp = errno;
                LOGD("vid client recv close fd:%d, rcv_len:%d, error_code:%d\n",
                     video_tcp_client_service->video_fd, rcv_len, errno);
                close(video_tcp_client_service->video_fd);
                video_tcp_client_service->video_fd = -1;
                video_tcp_client_service->video_status = BK_FALSE;

                // Check if stop was called before setting disconnected state
                if (video_tcp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
                {
                    // Stop was called, exit the thread
                    break;
                }

                video_tcp_client_service->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;
                if (errno_temp == ENOTCONN) {
                    ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, errno_temp, NTWK_TRANS_CHAN_VIDEO);
                } else {
                    ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0, NTWK_TRANS_CHAN_VIDEO);
                }

                // Try to reconnect
                rtos_delay_milliseconds(1000);
                break;
            }
        }
    }

out:
    LOGE("%s exit %d\n", __func__, video_tcp_client_service->chan_state);

    if (rcv_buf)
    {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    video_tcp_client_service->video_status = BK_FALSE;

    if (video_tcp_client_service->video_fd != -1)
    {
        close(video_tcp_client_service->video_fd);
        video_tcp_client_service->video_fd = -1;
    }

    rtos_delete_thread(NULL);
}

bk_err_t ntwk_tcp_video_client_chan_start(void *param)
{
    bk_err_t ret = BK_OK;
    ntwk_server_net_info_t *server_net_info = ntwk_trans_get_server_net_info();

    if (!server_net_info)
    {
        LOGE("server_net_info is NULL\n");
        return BK_FAIL;
    }

    LOGV("%s, %d\n", __func__, __LINE__);

    if (video_tcp_client_service == NULL)
    {
        LOGE("video_tcp_client_service is NULL\n");
        return BK_FAIL;
    }

    video_tcp_client_service->server_address = inet_addr((char *)server_net_info->ip_addr);
    if (video_tcp_client_service->server_address == INADDR_NONE)
    {
        LOGE("Invalid IP address: %s\n", server_net_info->ip_addr);
        return BK_FAIL;
    }

    video_tcp_client_service->server_port = (uint16_t)atoi((char *)server_net_info->video_port);
    if (video_tcp_client_service->server_port == 0)
    {
        LOGE("Invalid port: %s\n", server_net_info->video_port);
        return BK_FAIL;
    }

    if (!video_tcp_client_service->video_thd)
    {
        ret = rtos_create_thread(&video_tcp_client_service->video_thd,
                                 4,
                                 "ntwk_tcp_vid_cli",
                                 (beken_thread_function_t)ntwk_tcp_video_client_thread,
                                 1024 * 3,
                                 (beken_thread_arg_t)NULL);
        if (ret != kNoErr)
        {
            LOGE("%s, rtos_create_thread failed, ret:%d\n", __func__, ret);
        }
    }

    return ret;
}

bk_err_t ntwk_tcp_video_client_chan_stop(void)
{
    LOGD("%s, %d\n", __func__, __LINE__);

    if (video_tcp_client_service == NULL)
    {
        LOGE("%s: video_tcp_client_service is NULL\n", __func__);
        return BK_FAIL;
    }

    video_tcp_client_service->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_VIDEO);

    video_tcp_client_service->video_status = BK_FALSE;

    if (video_tcp_client_service->video_fd != -1)
    {
        close(video_tcp_client_service->video_fd);
        video_tcp_client_service->video_fd = -1;
    }

    if (video_tcp_client_service->video_thd)
    {
        rtos_thread_join(video_tcp_client_service->video_thd);
        video_tcp_client_service->video_thd = NULL;
    }

    return BK_OK;
}

int ntwk_tcp_video_client_send_packet(uint8_t *data, uint32_t length, image_format_t video_type)
{
    if (video_tcp_client_service == NULL || !video_tcp_client_service->video_status)
    {
        return -1;
    }

    return ntwk_socket_sendto(&video_tcp_client_service->video_fd,
                             (struct sockaddr *)&video_tcp_client_service->server_addr,
                             data, length);
}

bk_err_t ntwk_tcp_video_client_register_receive_cb(ntwk_video_receive_cb_t cb)
{
    if (video_tcp_client_service == NULL)
    {
        LOGE("%s: Video client channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    video_tcp_client_service->receive_cb = cb;
    LOGV("%s: Video receive callback registered successfully\n", __func__);

    return BK_OK;
}

// TCP Client audio channel receive data handler
static void ntwk_tcp_audio_client_receive_data(uint8_t *data, uint16_t length)
{
    if (aud_tcp_client_service && aud_tcp_client_service->receive_cb) {
        aud_tcp_client_service->receive_cb(data, length);
    } else {
        LOGW("%s: No receive callback registered\n", __func__);
    }
}

// TCP Client audio channel thread
static void ntwk_tcp_audio_client_thread(beken_thread_arg_t data)
{
    int rcv_len = 0;
    bk_err_t ret = BK_OK;
    u8 *rcv_buf = NULL;
    int connect_retry = 0;
    const int max_retry = 5;

    LOGV("%s entry\n", __func__);
    (void)(data);

    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_AUDIO);
    aud_tcp_client_service->chan_state = NTWK_TRANS_CHAN_START;

    rcv_buf = (u8 *) ntwk_malloc((NTWK_TCP_BUFFER + 1) * sizeof(u8));
    if (!rcv_buf)
    {
        LOGE("tcp audio client malloc failed\n");
        goto out;
    }

    while (1)
    {
        // Check if stop was called
        if (aud_tcp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
        {
            LOGV("%s, stop called, exiting\n", __func__);
            break;
        }

        // Create socket
        aud_tcp_client_service->aud_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (aud_tcp_client_service->aud_fd == -1)
        {
            LOGE("socket failed\n");
            goto out;
        }

        // Set server address
        aud_tcp_client_service->server_addr.sin_family = AF_INET;
        aud_tcp_client_service->server_addr.sin_port = htons(aud_tcp_client_service->server_port);
        aud_tcp_client_service->server_addr.sin_addr.s_addr = aud_tcp_client_service->server_address;

        aud_tcp_client_service->chan_state = NTWK_TRANS_CHAN_WAITING_CONNECTED;
        LOGV("connecting to server %s:%d\n",
             inet_ntoa(*(struct in_addr *)&aud_tcp_client_service->server_address),
             aud_tcp_client_service->server_port);

        // Connect to server
        ret = connect(aud_tcp_client_service->aud_fd,
                     (struct sockaddr *)&aud_tcp_client_service->server_addr,
                     sizeof(struct sockaddr_in));
        if (ret < 0)
        {
            LOGE("connect failed: %d\n", errno);
            close(aud_tcp_client_service->aud_fd);
            aud_tcp_client_service->aud_fd = -1;

            // Check if stop was called during retry
            if (aud_tcp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
            {
                LOGD("%s, stop called during connect retry\n", __func__);
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
        aud_tcp_client_service->chan_state = NTWK_TRANS_CHAN_CONNECTED;
        LOGD("audio, Connected to server fd:%d\n", aud_tcp_client_service->aud_fd);

        ntwk_tcp_set_keepalive(aud_tcp_client_service->aud_fd,
                               NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_ENABLE,
                               NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_IDLE_TIME,
                               NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_INTERVAL,
                               NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_COUNT);

        ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, aud_tcp_client_service->server_address, NTWK_TRANS_CHAN_AUDIO);
        ntwk_socket_set_qos(aud_tcp_client_service->aud_fd, IP_QOS_PRIORITY_HIGH);

        aud_tcp_client_service->aud_status = BK_TRUE;

        // Receive data loop
        while (aud_tcp_client_service->aud_status == BK_TRUE)
        {
            rcv_len = recv(aud_tcp_client_service->aud_fd, rcv_buf, NTWK_TCP_BUFFER, 0);
            if (rcv_len > 0)
            {
                ntwk_tcp_audio_client_receive_data(rcv_buf, rcv_len);
            }
            else
            {
                int errno_temp = errno;
                LOGD("aud client recv close fd:%d, rcv_len:%d, error_code:%d\n",
                     aud_tcp_client_service->aud_fd, rcv_len, errno);
                close(aud_tcp_client_service->aud_fd);
                aud_tcp_client_service->aud_fd = -1;
                aud_tcp_client_service->aud_status = BK_FALSE;

                // Check if stop was called before setting disconnected state
                if (aud_tcp_client_service->chan_state == NTWK_TRANS_CHAN_STOP)
                {
                    // Stop was called, exit the thread
                    break;
                }

                aud_tcp_client_service->chan_state = NTWK_TRANS_CHAN_DISCONNECTED;
                if (errno_temp == ENOTCONN) {
                    ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, errno_temp, NTWK_TRANS_CHAN_AUDIO);
                } else {
                    ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0, NTWK_TRANS_CHAN_AUDIO);
                }

                // Try to reconnect
                rtos_delay_milliseconds(1000);
                break;
            }
        }
    }

out:
    LOGE("%s exit %d\n", __func__, aud_tcp_client_service->aud_status);

    if (rcv_buf)
    {
        os_free(rcv_buf);
        rcv_buf = NULL;
    }

    aud_tcp_client_service->aud_status = BK_FALSE;

    if (aud_tcp_client_service->aud_fd != -1)
    {
        close(aud_tcp_client_service->aud_fd);
        aud_tcp_client_service->aud_fd = -1;
    }

    rtos_delete_thread(NULL);
}

bk_err_t ntwk_tcp_audio_client_chan_start(void *param)
{
    bk_err_t ret = BK_OK;
    ntwk_server_net_info_t *server_net_info = ntwk_trans_get_server_net_info();

    if (!server_net_info)
    {
        LOGE("server_net_info is NULL\n");
        return BK_FAIL;
    }

    if (aud_tcp_client_service == NULL)
    {
        LOGE("aud_tcp_client_service is NULL\n");
        return BK_FAIL;
    }

    aud_tcp_client_service->server_address = inet_addr((char *)server_net_info->ip_addr);
    if (aud_tcp_client_service->server_address == INADDR_NONE)
    {
        LOGE("Invalid IP address: %s\n", server_net_info->ip_addr);
        return BK_FAIL;
    }

    aud_tcp_client_service->server_port = (uint16_t)atoi((char *)server_net_info->audio_port);
    if (aud_tcp_client_service->server_port == 0)
    {
        LOGE("Invalid port: %s\n", server_net_info->audio_port);
        return BK_FAIL;
    }

    if (!aud_tcp_client_service->aud_thd)
    {
        ret = rtos_create_thread(&aud_tcp_client_service->aud_thd,
                                 4,
                                 "ntwk_tcp_aud_cli",
                                 (beken_thread_function_t)ntwk_tcp_audio_client_thread,
                                 1024 * 3,
                                 (beken_thread_arg_t)NULL);
        if (ret != kNoErr)
        {
            LOGE("Error: failed to create tcp audio client: %d\n", ret);
        }
    }

    return ret;
}

bk_err_t ntwk_tcp_audio_client_chan_stop(void)
{
    if (aud_tcp_client_service == NULL)
    {
        LOGE("aud_tcp_client_service is NULL\n");
        return BK_FAIL;
    }

    aud_tcp_client_service->chan_state = NTWK_TRANS_CHAN_STOP;
    ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_AUDIO);

    aud_tcp_client_service->aud_status = BK_FALSE;

    if (aud_tcp_client_service->aud_fd != -1)
    {
        close(aud_tcp_client_service->aud_fd);
        aud_tcp_client_service->aud_fd = -1;
    }

    if (aud_tcp_client_service->aud_thd)
    {
        rtos_thread_join(aud_tcp_client_service->aud_thd);
        aud_tcp_client_service->aud_thd = NULL;
    }

    return BK_OK;
}

int ntwk_tcp_audio_client_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type)
{
    if (aud_tcp_client_service == NULL || !aud_tcp_client_service->aud_status)
    {
        return -1;
    }

    return ntwk_socket_sendto(&aud_tcp_client_service->aud_fd,
                             (struct sockaddr *)&aud_tcp_client_service->server_addr,
                             data, length);
}

bk_err_t ntwk_tcp_audio_client_register_receive_cb(ntwk_audio_receive_cb_t cb)
{
    if (aud_tcp_client_service == NULL)
    {
        LOGE("%s: Audio client channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    aud_tcp_client_service->receive_cb = cb;
    LOGV("%s: Audio receive callback registered successfully\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_tcp_client_init(chan_type_t chan_type)
{
    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (ntwk_tcp_ctrl_client_info != NULL)
            {
                LOGE("%s: ntwk_tcp_ctrl_client_info already initialized\n", __func__);
                return BK_OK;
            }
            ntwk_tcp_ctrl_client_info = ntwk_malloc(sizeof(ntwk_tcp_ctrl_client_info_t));
            if (ntwk_tcp_ctrl_client_info == NULL)
            {
                LOGE("%s: malloc ntwk_tcp_ctrl_client_info failed\n", __func__);
                return BK_FAIL;
            }
            os_memset(ntwk_tcp_ctrl_client_info, 0, sizeof(ntwk_tcp_ctrl_client_info_t));
            ntwk_tcp_ctrl_client_info->client_fd = -1;
        } break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (video_tcp_client_service != NULL)
            {
                LOGE("%s: video_tcp_client_service already initialized\n", __func__);
                return BK_OK;
            }
            video_tcp_client_service = ntwk_malloc(sizeof(video_tcp_client_service_t));
            if (video_tcp_client_service == NULL)
            {
                LOGE("%s: malloc video_tcp_client_service failed\n", __func__);
                return BK_FAIL;
            }
            os_memset(video_tcp_client_service, 0, sizeof(video_tcp_client_service_t));
            video_tcp_client_service->video_fd = -1;
        } break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (aud_tcp_client_service != NULL)
            {
                LOGE("%s: aud_tcp_client_service already initialized\n", __func__);
                return BK_OK;
            }
            aud_tcp_client_service = ntwk_malloc(sizeof(aud_tcp_client_service_t));
            if (aud_tcp_client_service == NULL)
            {
                LOGE("%s: malloc aud_tcp_client_service failed\n", __func__);
                return BK_FAIL;
            }
            os_memset(aud_tcp_client_service, 0, sizeof(aud_tcp_client_service_t));
            aud_tcp_client_service->aud_fd = -1;
        } break;
        default:
            LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
            return BK_ERR_PARAM;
    }
    return BK_OK;
}

bk_err_t ntwk_tcp_client_deinit(chan_type_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (ntwk_tcp_ctrl_client_info != NULL)
            {
                os_free(ntwk_tcp_ctrl_client_info);
                ntwk_tcp_ctrl_client_info = NULL;
            }
        } break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (video_tcp_client_service != NULL)
            {
                os_free(video_tcp_client_service);
                video_tcp_client_service = NULL;
            }
        } break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (aud_tcp_client_service != NULL)
            {
                os_free(aud_tcp_client_service);
                aud_tcp_client_service = NULL;
            }
        } break;
        default:
            LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
            return BK_ERR_PARAM;
    }

    LOGV("%s: chan_type %d deinitialized\n", __func__, chan_type);

    return BK_OK;
}
#endif // CONFIG_NTWK_CLIENT_SERVICE_ENABLE