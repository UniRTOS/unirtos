#include <common/bk_include.h>
#include "cli.h"
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include <common/bk_kernel_err.h>
#include <string.h>

#include <common/sys_config.h>
#include <components/log.h>
#include <components/event.h>
#include <components/netif.h>

#include "network_type.h"
#include "ntwk_socket.h"

#include "wifi_api.h"

#define TAG "ntwk-socket"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

int ntwk_socket_set_qos(int fd, int qos)
{
    int ret = setsockopt(fd, IPPROTO_IP, IP_TOS, &qos, sizeof(qos));

    LOGV("%s\n", __func__);

    if (ret < 0)
    {
        LOGE("%s failed: %d\n", ret);
    }

    return ret;
}

int ntwk_socket_sendto(int *fd, const struct sockaddr *dst, uint8_t *data, uint32_t length)
{
    int ret = 0;

    uint8_t *ptr = data;
    uint16_t size = length;
    int max_retry = NTWK_SEND_MAX_RETRY;
    uint16_t index = 0;

    do
    {

        if (*fd < 0)
        {
            ret = -1;
            break;
        }

        ret = sendto(*fd, ptr + index, size - index, MSG_DONTWAIT | MSG_MORE,
                     dst, sizeof(struct sockaddr_in));

        //LOGD("send: %d, %d\n", ret, size);

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                ret = 0;
            }
            else
            {
                LOGV("%s, %d, %d\n", __func__, ret, errno);
                break;
            }
        }

        index += ret;

        if (index == size)
        {
            ret = size;
            break;
        }

        max_retry--;

        if (max_retry < 0)
        {
            ret = -1;
            max_retry = 0;
            LOGE("reach max retry\n");
            break;
        }

        rtos_delay_milliseconds(NTWK_SEND_MAX_DELAY);

    }
    while (index < size);

    return ret;
}

int ntwk_socket_write(int *fd, uint8_t *data, uint32_t length)
{
    int ret = 0;

    uint8_t *ptr = data;
    uint16_t size = length;
    int max_retry = NTWK_SEND_MAX_RETRY;
    uint16_t index = 0;

    do
    {

        if (*fd < 0)
        {
            ret = -1;
            break;
        }

        ret = write(*fd, ptr + index, size - index);

        //LOGD("send: %d, %d\n", ret, size);

        if (ret < 0)
        {
            ret = 0;
        }

        index += ret;

        if (index == size)
        {
            ret = size;
            break;
        }

        max_retry--;

        if (max_retry < 0)
        {
            ret = -1;
            LOGE("reach max retry\n");
            break;
        }

        rtos_delay_milliseconds(NTWK_SEND_MAX_DELAY);

    }
    while (index < size);

    return ret;
}

