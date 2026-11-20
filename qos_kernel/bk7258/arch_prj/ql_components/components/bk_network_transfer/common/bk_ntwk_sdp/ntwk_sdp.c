#include <common/bk_include.h>
#include "cli.h"
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include "lwip/sockets.h"
#include "lwip/udp.h"
#include "net.h"
#include "string.h"
#include <components/netif.h>
#include "network_transfer_internal.h"

#include "ntwk_sdp.h"

#define TAG "ntwk-sdp"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


typedef struct
{
    int sock;
    beken_timer_t timer;
    UINT32 intval_ms;
    struct sockaddr_in sock_remote;
    char *sdp_data;
    uint16_t sdp_length;
} ntwk_sdp_t;


extern void ap_set_default_netif(void);
extern uint8_t uap_ip_start_flag;


ntwk_sdp_t *ntwk_sdp = NULL;


static int ntwk_sdp_init_socket(int *fd_ptr, UINT16 local_port)
{
    struct sockaddr_in l_socket;
    int so_broadcast = 1, sock = -1;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        LOGE("Socket error\n");
        goto sta_udp_exit;
    }

    l_socket.sin_family = AF_INET;
    l_socket.sin_port = htons(local_port);
    l_socket.sin_addr.s_addr = htonl(INADDR_ANY);
    os_memset(&(l_socket.sin_zero), 0, sizeof(l_socket.sin_zero));

    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast)) != 0)
    {
        LOGE("notify socket setsockopt error\n");
        goto sta_udp_exit;
    }

    if (bind(sock, (struct sockaddr *)&l_socket, sizeof(l_socket)) != 0)
    {
        LOGE(" notify socket bind error\n");
        goto sta_udp_exit;
    }

    if (fd_ptr)
    {
        *fd_ptr = sock;
    }

    return 0;

sta_udp_exit:
    if ((sock) != -1)
    {
        closesocket(sock);
        sock = -1;
    }

    return -1;
}


int ntwk_sdp_generate(const char *name, uint32_t cmd_port, uint32_t img_port, uint32_t aud_port)
{
    uint32 adv_len = ADV_ALLOC_LEN;
    const char *adv_temp =                                                                      \
            "{\"service\":\"%s\", \"cmd\":\"%u\",\"img\":\"%u\", \"aud\":\"%u\"}";

    if (ntwk_sdp->sdp_data == NULL)
    {
        ntwk_sdp->sdp_data = ntwk_malloc(adv_len);

        if (ntwk_sdp->sdp_data == NULL)
        {
            LOGE("no memory\r\n");
            return BK_FAIL;
        }
    }

    os_memset(ntwk_sdp->sdp_data, 0, adv_len);

    if (uap_ip_is_start())
    {
        LOGV("%s, ap mode\n", __func__);
    }
#if CONFIG_P2P
    else if (p2p_go_ip_is_start())
    {
        LOGV("%s, p2p_go mode\n", __func__);
    }
#endif
    else if (sta_ip_is_start())
    {
        LOGV("%s, sta mode\n", __func__);
    }

    sprintf(ntwk_sdp->sdp_data, adv_temp,
            name,
            cmd_port,
            img_port,
            aud_port);

    ntwk_sdp->sdp_length = strlen(ntwk_sdp->sdp_data);

    LOGV("adv_data:%s,%u\r\n", ntwk_sdp->sdp_data, ntwk_sdp->sdp_length);

    return 0;
}


static void ntwk_sdp_timer_handler(void *data)
{
    if (ntwk_sdp == NULL)
    {
        LOGE("ntwk_sdp NULL return");
        return;
    }

    if (ntwk_sdp->sdp_data == NULL
        || ntwk_sdp->sdp_data == NULL)
    {
        LOGE("ntwk_sdp sdp data NULL return");
        return;
    }

    if (uap_ip_start_flag == 1)
    {
        ap_set_default_netif();
    }
#if CONFIG_P2P
    else if (p2p_go_ip_is_start())
    {
        p2p_go_set_default_netif();
    }
#endif

    LOGV("sdp: %s\n", ntwk_sdp->sdp_data);
    sendto(ntwk_sdp->sock,
           ntwk_sdp->sdp_data,
           ntwk_sdp->sdp_length,
           0,
           (struct sockaddr *)&ntwk_sdp->sock_remote,
           sizeof(struct sockaddr));

}

int ntwk_sdp_start_timer(UINT32 time_ms)
{
    if (ntwk_sdp)
    {
        int err;
        UINT32 org_ms = ntwk_sdp->intval_ms;

        if (org_ms != 0)
        {
            if ((org_ms != time_ms))
            {
                if (ntwk_sdp->timer.handle != NULL)
                {
                    err = rtos_deinit_timer(&ntwk_sdp->timer);
                    if (kNoErr != err)
                    {
                        LOGE("deinit time fail\r\n");
                        return kGeneralErr;
                    }
                    ntwk_sdp->timer.handle = NULL;
                }
            }
            else
            {
                LOGE("timer aready start\r\n");
                return kNoErr;
            }
        }

        err = rtos_init_timer(&ntwk_sdp->timer,
                              time_ms,
                              ntwk_sdp_timer_handler,
                              NULL);
        if (kNoErr != err)
        {
            LOGE("init timer fail\r\n");
            return kGeneralErr;
        }
        ntwk_sdp->intval_ms = time_ms;

        err = rtos_start_timer(&ntwk_sdp->timer);
        if (kNoErr != err)
        {
            LOGE("start timer fail\r\n");
            return kGeneralErr;
        }
        LOGD("ntwk_sdp_start_timer\r\n");

        return kNoErr;
    }
    return kInProgressErr;
}

int ntwk_sdp_stop_timer(void)
{
    if (ntwk_sdp)
    {
        int err;

        err = rtos_stop_timer(&ntwk_sdp->timer);
        if (kNoErr != err)
        {
            LOGE("stop time fail\r\n");
            return kGeneralErr;
        }

        return kNoErr;
    }
    return kInProgressErr;
}

int ntwk_sdp_reload_timer(uint32_t time_ms)
{
    if (ntwk_sdp)
    {
        int err;

        //err = rtos_change_period(&ntwk_sdp->timer, 60000);
        err = rtos_change_period(&ntwk_sdp->timer, time_ms);
        if (kNoErr != err)
        {
            LOGE("change period fail\r\n");
            return kGeneralErr;
        }
        err = rtos_reload_timer(&ntwk_sdp->timer);
        if (kNoErr != err)
        {
            LOGE("change period fail\r\n");
            return kGeneralErr;
        }

        return kNoErr;
    }
    return kInProgressErr;
}



int ntwk_sdp_pub_deinit(void)
{
    LOGV("ntwk_sdp_deint\r\n");

    if (ntwk_sdp != NULL)
    {
        int err;
        if (ntwk_sdp->timer.handle != NULL)
        {
            err = rtos_deinit_timer(&ntwk_sdp->timer);
            if (kNoErr != err)
            {
                LOGE("deinit time fail\r\n");
                return kGeneralErr;
            }
        }

        closesocket(ntwk_sdp->sock);

        os_free(ntwk_sdp);
        ntwk_sdp = NULL;

        LOGV("ntwk_sdp_deint ok\r\n");
        return kNoErr;
    }

    return kInProgressErr;
}



int ntwk_sdp_start(const char *name, uint32_t cmd_port, uint32_t img_port, uint32_t aud_port)
{
    LOGD("ntwk_sdp_start\r\n");
    int ret = 0;

    if (ntwk_sdp == NULL)
    {
        ntwk_sdp = ntwk_malloc(sizeof(ntwk_sdp_t));

        if (ntwk_sdp == NULL)
        {
            LOGE("malloc fail\r\n");
            return BK_FAIL;
        }
    }
    else
    {
        LOGE("already init?\r\n");
        ntwk_sdp_reload(1000);
        return BK_FAIL;
    }

    os_memset(ntwk_sdp, 0, sizeof(ntwk_sdp_t));

    ntwk_sdp_generate(name, cmd_port, img_port, aud_port);

    if (ntwk_sdp_init_socket(&ntwk_sdp->sock, UDP_SDP_LOCAL_PORT) != 0)
    {
        LOGE("socket fail\r\n");
        ntwk_sdp = NULL;
        return kGeneralErr;
    }

    ntwk_sdp->sock_remote.sin_family = AF_INET;
    ntwk_sdp->sock_remote.sin_port = htons(UDP_SDP_REMOTE_PORT);
    ntwk_sdp->sock_remote.sin_addr.s_addr = INADDR_BROADCAST;

    if (ntwk_sdp_start_timer(1000) != kNoErr)
    {
        ret = -5;
        goto sdp_int_err;
    }

    LOGD("done\r\n");

    return 0;

sdp_int_err:
    ntwk_sdp_pub_deinit();

    return ret;
}

int ntwk_sdp_stop(void)
{
    LOGD("%s start\n", __func__);

    if (ntwk_sdp_stop_timer() != kNoErr)
    {
        return -1;
    }

    if (ntwk_sdp_pub_deinit() != kNoErr)
    {
        return -2;
    }

    LOGV("%s done\n", __func__);

    return 0;
}

int ntwk_sdp_reload(uint32_t time_ms)
{
    LOGD("%s start\n", __func__);

    if (ntwk_sdp_reload_timer(time_ms) != kNoErr)
    {
        return -1;
    }

    LOGV("%s done\n", __func__);

    return 0;
}

