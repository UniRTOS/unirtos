#include <common/bk_include.h>

#include "lwip/tcp.h"
#include "bk_uart.h"
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <common/bk_kernel_err.h>
#include <components/video_types.h>

#include "lwip/sockets.h"
#include <stdlib.h>

#include "network_type.h"
#include "network_transfer.h"
#include "ntwk_cs2_service.h"
#include "network_transfer_internal.h"

#include "PPCS_API.h"
#include "PPCS_Error.h"
#include "PPCS_Type.h"

#include "PPCS_cs2_comm.h"

#define TAG "ntwk-cs2"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

ntwk_cs2_info_t *ntwk_cs2_info = NULL;

static volatile int s_current_sessionid = -1;
static st_PPCS_NetInfo s_cs2_p2p_networkinfo;

#if THROUGHPUT_DEBUG
static beken_timer_t s_throughput_timer;
static volatile uint32_t s_send_video_count = 0;
static volatile uint32_t s_send_video_bytes = 0;
static volatile uint32_t s_send_audio_bytes = 0;
static volatile uint32_t s_recv_audio_bytes = 0;
static volatile uint32_t s_recv_total_bytes = 0;
static volatile uint32_t s_send_time = 0;
static volatile uint32_t s_delay_count = 0;

static void throughput_anlayse_timer_hdl(void *param)
{
    LOGD("cs2_tp, send bytes %d, count %d, %.3f count/s, %.3f KB/s, %.3f ms/perc, %.3fB/perc, delay count %d\n",
         s_send_video_bytes,
         s_send_video_count,
         1.0 * s_send_video_count / (THROUGHPUT_ANLAYSE_MS / 1000),
         (float)(s_send_video_bytes / (THROUGHPUT_ANLAYSE_MS / 1000) / 1024.0),
         1.0 * s_send_time / s_send_video_count,
         1.0 * s_send_video_bytes / s_send_video_count,
         s_delay_count);

    s_delay_count = s_send_time = s_send_video_count = s_recv_total_bytes = s_send_video_bytes = s_send_audio_bytes = s_recv_audio_bytes = 0;

    return;

}

void ntwk_cs2_video_timer_deinit(void)
{
    bk_err_t t_err;

    if (!ntwk_cs2_info)
    {
        return;
    }

    rtos_lock_mutex(&ntwk_cs2_info->mutex);

    if (rtos_is_timer_init(&s_throughput_timer))
    {
        if (rtos_is_timer_running(&s_throughput_timer))
        {
            t_err = rtos_stop_timer(&s_throughput_timer);

            if (t_err != BK_OK)
            {
                LOGE("stop throughput timer fail\n");
                rtos_unlock_mutex(&ntwk_cs2_info->mutex);
                return ;
            }
        }

        t_err = rtos_deinit_timer(&s_throughput_timer);
        if (t_err != BK_OK)
        {
            LOGE("deinit throughput timer fail\n");
            rtos_unlock_mutex(&ntwk_cs2_info->mutex);
            return ;
        }
    }

    rtos_unlock_mutex(&ntwk_cs2_info->mutex);
}

bk_err_t ntwk_cs2_video_timer_init(void)
{
    bk_err_t t_err;

    rtos_lock_mutex(&ntwk_cs2_info->mutex);
    if (rtos_is_timer_init(&s_throughput_timer))
    {
        rtos_unlock_mutex(&ntwk_cs2_info->mutex);
        return BK_OK;
    }

    t_err = rtos_init_timer(&s_throughput_timer, THROUGHPUT_ANLAYSE_MS, throughput_anlayse_timer_hdl, NULL);

    if (t_err != BK_OK)
    {
        LOGE("init throughput timer fail\n");
        goto timer_err;
    }

    t_err = rtos_change_period(&s_throughput_timer, THROUGHPUT_ANLAYSE_MS);
    if (t_err != BK_OK)
    {
        LOGE("change throughput timer period fail\n");
        goto timer_err;
    }

    t_err = rtos_start_timer(&s_throughput_timer);
    if (t_err != BK_OK)
    {
        LOGE("start throughput timer fail\n");
        goto timer_err;
    }

    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    return BK_OK;

timer_err:

    rtos_unlock_mutex(&ntwk_cs2_info->mutex);
    ntwk_cs2_video_timer_deinit();

    return BK_FAIL;
}
#endif

bk_err_t ntwk_cs2_get_current_write_size(uint32_t *write_size)
{
   int32_t Check_ret = 0;
   UINT32 WriteSize = 0;

    if (s_current_sessionid < 0)
    {
        LOGE("%s Invalid sessionid:%d\n", __func__, s_current_sessionid);
        return -1;
    }

    Check_ret = PPCS_Check_Buffer(s_current_sessionid, VIDEO_P2P_CHANNEL, &WriteSize, NULL);

    if (0 > Check_ret)
    {
        LOGE("%s PPCS_Check_Buffer: Session=%d,CH=%d,WriteSize=%d,ret=%d %s\n", __func__,
            s_current_sessionid, VIDEO_P2P_CHANNEL, WriteSize, Check_ret, get_p2p_error_code_info(Check_ret));
        return -1;
    }

    *write_size = WriteSize;

    return BK_OK;
}

int ntwk_cs2_p2p_write(int SessionID, uint8_t Channel, uint8_t *buff, uint32_t size)
{
    int32_t ret = 0;
    int32_t Check_ret = 0;
    UINT32 WriteSize = size;
    uint32_t write_index = 0;
    uint32_t write_not_send_thr = PPCS_TX_BUFFER_THD;
    const uint32_t write_per_count_thr = size;///write_not_send_thr / 10;//1024 * 2;

    if (size == 0)
    {
        LOGE("%s, size: 0\n", __func__);
        return 0;
    }

    do
    {
        uint32_t will_write_size = ((size - write_index < write_per_count_thr) ? (size - write_index) : write_per_count_thr);
        // 在调用 PPCS_Write 之前一定要调用 PPCS_Check_Buffer 检测写缓存还有多少数据尚未发出去，需控制在一个合理范围，一般控制在 128KB/256KB 左右。
        Check_ret = PPCS_Check_Buffer(SessionID, Channel, &WriteSize, NULL);

        // st_debug("ThreadWrite PPCS_Check_Buffer: Session=%d,CH=%d,WriteSize=%d,ret=%d %s\n", SessionID, Channel, WriteSize, Check_ret, get_p2p_error_code_info(Check_ret));
        if (0 > Check_ret)
        {
            LOGE("%s PPCS_Check_Buffer: Session=%d,CH=%d,WriteSize=%d,ret=%d %s\n", __func__, SessionID, Channel, WriteSize, Check_ret, get_p2p_error_code_info(Check_ret));
            ret = Check_ret;
            goto WRITE_FAIL;
            break;
        }

        if (Channel == VIDEO_P2P_CHANNEL)
        {
            write_not_send_thr = CS2_IMG_MAX_TX_BUFFER_THD;
        }
        else if (Channel == AUD_P2P_CHANNEL)
        {
            write_not_send_thr = CS2_AUD_MAX_TX_BUFFER_THD;
        }

        // 写缓存的数据大小超过128KB/256KB，则需考虑延时缓一缓。
        // 如果发现 wsize 越来越大，可能网络状态很差，需要考虑一下丢帧或降码率，这是一个动态调整策略，非常重要!!
        // On device, Recommended CHECK_WRITE_THRESHOLD_SIZE == (128 or 256) * 1024 Byte. this sample set 1MB.

        if (WriteSize <= write_not_send_thr)
        {
            //LOGD("%s start write %d WriteSize %d SessionID %d channel %d\n", __func__, will_write_size, WriteSize, SessionID, Channel);

            //LOGD("channel: %d, write size: %d\n", Channel, will_write_size);
            ret = PPCS_Write(SessionID, Channel, (CHAR *)(buff + write_index), will_write_size);
            //LOGD("channel: %d, return size: %d\n", Channel, ret);

            if (0 > ret)
            {
                if (ERROR_PPCS_SESSION_CLOSED_TIMEOUT == ret)
                {
                    LOGE("%s Session=%d,CH=%d,ret=%d, Session Closed TimeOUT!!\n", __func__, SessionID, Channel, ret);
                }
                else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret)
                {
                    LOGE("%s Session=%d,CH=%d,ret=%d, Session Remote Closed!!\n", __func__, SessionID, Channel, ret);
                }
                else if (ERROR_PPCS_INVALID_PARAMETER == ret)
                {
                    LOGE("%s Session=%d,CH=%d,ret=%d, ERROR_PPCS_INVALID_PARAMETER %d!!\n", __func__, SessionID, Channel, ret, will_write_size);
                }
                else
                {
                    LOGE("%s Session=%d,CH=%d,ret=%d %s\n", __func__, SessionID, Channel, ret, get_p2p_error_code_info(ret));
                }

                goto WRITE_FAIL;
            }

            write_index += ret;
        }
        else
        {
            //LOGE("%s, full\n", __func__);
            break;
        }
    }
    while (write_index < size);

    return write_index;

WRITE_FAIL:

    return ret;
}

int ntwk_cs2_video_send_packet(uint8_t *data, uint32_t length, image_format_t video_type)
{
    int send_byte = 0;
    uint32_t index = 0;
    uint8_t *ptr = data;
    uint16_t size = length;
#if THROUGHPUT_DEBUG
    uint32_t start_time = 0;
#endif

    if (!ntwk_cs2_info->video_status)
    {
        LOGE("video not ready\n");
        return -1;
    }

    if (s_current_sessionid < 0)
    {
        LOGE("video send error, session lose\n");
        return -1;
    }

#if THROUGHPUT_DEBUG
    if (rtos_is_timer_running(&s_throughput_timer)) {
        start_time = rtos_get_time();
    }
#endif

    do
    {
        if (s_current_sessionid < 0)
        {
            return -1;
        }

        send_byte = ntwk_cs2_p2p_write(s_current_sessionid, VIDEO_P2P_CHANNEL, ptr + index, size - index);

        if (send_byte < 0)
        {
            LOGE("%s send return fd:%d sessionid:%d\n", __func__, send_byte,s_current_sessionid);
            rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
            return -1;
        }

        index += send_byte;

        if (index < size)
        {
#if THROUGHPUT_DEBUG
           if (rtos_is_timer_running(&s_throughput_timer)) {
                s_delay_count++;
           }
#endif
            //LOGD("%s delay %d, %d\n", __func__, index, size);
            rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
        }
    }
    while (index < size);

#if THROUGHPUT_DEBUG
    uint32_t end_time = rtos_get_time();

    //LOGE("send: %d, %d\n", index, length);
    if (rtos_is_timer_running(&s_throughput_timer)) {
        s_send_video_count++;
        s_send_video_bytes += size;
        s_send_time += end_time - start_time;
    }
#endif

    return index;
}

int ntwk_cs2_audio_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type)
{
    int send_byte = 0;
    uint32_t index = 0;
    uint8_t *ptr = data;
    uint16_t size = length;

    if (!ntwk_cs2_info->aud_status)
    {
        LOGE("audio not ready\n");
        return -1;
    }

    if (s_current_sessionid < 0)
    {
        LOGE("aud send error, session lose\n");
        return -1;
    }

    do
    {
        send_byte = ntwk_cs2_p2p_write(s_current_sessionid, AUD_P2P_CHANNEL, ptr + index, size - index);

        if (send_byte < 0)
        {
            LOGE("%s send return fd:%d sessionid:%d\n", __func__, send_byte,s_current_sessionid);
            rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
            return -1;
        }

        index += send_byte;

        if (index < size)
        {
            //LOGD("%s delay %d, %d\n", __func__, index, size);
            rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
        }
    }
    while (index < size && s_current_sessionid > 0);

    //LOGE("send: %d, %d\n", index, length);

    return index;
}

int ntwk_cs2_p2p_ctrl_send(uint8_t *data, uint32_t length)
{
    int send_byte = 0;
    uint32_t index = 0;

    do
    {
        send_byte = ntwk_cs2_p2p_write(s_current_sessionid, CMD_P2P_CHANNEL, data + index, length - index);

        if (send_byte < 0)
        {
            LOGE("%s send return fd:%d sessionid:%d\n", __func__, send_byte,s_current_sessionid);
            rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
            return -1;
        }

        index += send_byte;

        if (index < length)
        {
            LOGD("%s delay %d, %d\n", __func__, index, length);
            rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
        }

    }
    while (index < length);


    return index;
}


static void ntwk_cs2_session_close(void)
{
    time_info_t t1, t2;

    memset(&t2, 0, sizeof(t2));

    rtos_lock_mutex(&ntwk_cs2_info->mutex);

    if (s_current_sessionid == -1)
    {
        LOGW("%s already close\n", __func__);
        goto out;
    }

    cs2_p2p_get_time(&t1);

    PPCS_ForceClose(s_current_sessionid);// PPCS_Close(SessionID);// 不能多线程对同一个 SessionID 做 PPCS_Close(SessionID)/PPCS_ForceClose(SessionID) 的动作，否则可能导致崩溃。

    cs2_p2p_get_time(&t2);

    LOGD("%s: (%d) done!! t:%d ms\n", __func__, s_current_sessionid, TU_MS(t1, t2));

    s_current_sessionid = -1;

out:

    if (ntwk_cs2_info->device_connected == BK_TRUE)
    {
        ntwk_msg_event_report(NTWK_TRANS_EVT_STOP, 0, NTWK_TRANS_CHAN_CTRL);
        ntwk_cs2_info->device_connected = BK_FALSE;
    }

    rtos_unlock_mutex(&ntwk_cs2_info->mutex);

    #if THROUGHPUT_DEBUG
    ntwk_cs2_video_timer_deinit();
    #endif
}

int ntwk_cs2_p2p_interface_init(p2p_cs2_key_t *key)
{
    int ret = 0;
    UINT32 APIVersion = PPCS_GetAPIVersion();
    char VerBuf[64] = {0};

    snprintf(VerBuf, sizeof(VerBuf), "%d.%d.%d.%d",
             (APIVersion & 0xFF000000) >> 24,
             (APIVersion & 0x00FF0000) >> 16,
             (APIVersion & 0x0000FF00) >> 8,
             (APIVersion & 0x000000FF) >> 0);

    if (0 > strncmp(VerBuf, "3.5.0.0", 5))
    {
        LOGD("PPCS P2P API Version: %d.%d.%d.%d\n",
             (APIVersion & 0xFF000000) >> 24,
             (APIVersion & 0x00FF0000) >> 16,
             (APIVersion & 0x0000FF00) >> 8,
             (APIVersion & 0x000000FF) >> 0);
    }
    else
    {
        const char *pVer = PPCS_GetAPIInformation();// PPCS_GetAPIInformation: support by Version >= 3.5.0
        LOGD("PPCS_GetAPIInformation(%u Byte):\n%s\n", (unsigned)strlen(pVer), pVer);
    }

    time_info_t t1, t2;
    os_memset(&t1, 0, sizeof(t1));
    os_memset(&t2, 0, sizeof(t2));

    if (0 <= strncmp(VerBuf, "4.2.0.0", 5)) // PPCS_Initialize JsonString support by Version>=4.2.0
    {
        int MaxNumSess = 1; // Max Number Session: 1~256.
        int SessAliveSec = 15; // session timeout close alive: 6~30.


        char InitJsonString[256] = {0};
        snprintf(InitJsonString, sizeof(InitJsonString), "{\"InitString\":\"%s\",\"MaxNumSess\":%d,\"SessAliveSec\":%d}", key->initstring, MaxNumSess, SessAliveSec);
        // st_debug("InitJsonString=%s\n",InitJsonString);
        cs2_p2p_get_time(&t1);
        LOGD("[%s] PPCS_Initialize1(%s) ...\n", t1.date, InitJsonString);


        // 如果Parameter 不是正确的JSON字串则会被当成InitString[:P2PKey]来处理, 如此以兼容旧版.
        ret = PPCS_Initialize((char *)InitJsonString);

        cs2_p2p_get_time(&t2);
        LOGD("[%s] PPCS_Initialize2 len(%d): ret=%d, t:%d ms\n", t2.date, strlen(key->initstring), ret, TU_MS(t1, t2));


        if (ERROR_PPCS_SUCCESSFUL != ret && ERROR_PPCS_ALREADY_INITIALIZED != ret)
        {
            LOGD("[%s] PPCS_Initialize: ret=%d\n", t2.date, ret);
            return 0;
        }
    }
    else
    {
        cs2_p2p_get_time(&t1);
        LOGD("[%s] PPCS_Initialize3(%s) ...\n", t1.date, key->initstring);
        ret = PPCS_Initialize((char *)key->initstring);
        cs2_p2p_get_time(&t2);
        LOGD("[%s] PPCS_Initialize4(%s): ret=%d, t:%d ms\n", t2.date, key->initstring, ret, TU_MS(t1, t2));

        if (ERROR_PPCS_SUCCESSFUL != ret && ERROR_PPCS_ALREADY_INITIALIZED != ret)
        {
            LOGD("[%s] PPCS_Initialize: ret=%d\n", t2.date, ret);
            return 0;
        }
    }

    return 0;
}

void ntwk_cs2_p2p_interface_deinit(void)
{
    int ret = PPCS_DeInitialize();

    if (ERROR_PPCS_SUCCESSFUL != ret)
    {
        LOGE("%s PPCS_DeInitialize: ret=%d %s\n", __func__, ret, get_p2p_error_code_info(ret));
    }

    os_memset(&s_cs2_p2p_networkinfo, 0, sizeof(s_cs2_p2p_networkinfo));
}

static int ntwk_cs2_p2p_audio_receiver(beken_thread_arg_t arg)
{
    int32_t ret = 0;

    uint8_t *tmp_read_buf = NULL;

    tmp_read_buf = ntwk_malloc(NTWK_CS2_RECV_TMP_BUFF_SIZE);

    if (!tmp_read_buf)
    {
        LOGE("p2p", "%s alloc err\n", __func__);
        return -1;
    }

    rtos_set_semaphore(&ntwk_cs2_info->aud_sem);

    while (ntwk_cs2_info->aud_running == BK_TRUE)//Repeat < Total_Times)
    {

        if (0 <= s_current_sessionid)
        {
            LOGE("%s listen Sid %d\n", __func__, s_current_sessionid);

            rtos_lock_mutex(&ntwk_cs2_info->mutex);
            ntwk_cs2_info->aud_status = BK_TRUE;
            rtos_unlock_mutex(&ntwk_cs2_info->mutex);

            do
            {
                INT32 ReadSize = 0;

                ReadSize = NTWK_CS2_RECV_TMP_BUFF_SIZE;

                ret = PPCS_Read(s_current_sessionid, AUD_P2P_CHANNEL, (char *)tmp_read_buf, &ReadSize, 1000 * 2);

                if (ReadSize)
                {
                    LOGV("got audio count: %d\n", ReadSize);
                    ntwk_cs2_info->audio_receive_cb(tmp_read_buf, ReadSize);

                    continue;
                }

                if (ret == ERROR_PPCS_TIME_OUT)
                {
                    LOGV("got audio data timeout\n");
                    continue;
                }

                if (ret < 0 && ERROR_PPCS_TIME_OUT != ret)
                {
                    LOGE("%s PPCS_Read ret err %d %s\n", __func__, ret, get_p2p_error_code_info(ret));
                    ntwk_cs2_session_close();
                    break;
                }
            }
            while (ntwk_cs2_info->aud_running == BK_TRUE);

            ntwk_cs2_info->aud_status = BK_FALSE;
            ntwk_cs2_info->aud_running = false;

            rtos_delay_milliseconds(300);
            break;
        }
        else
        {
            LOGE("session error wait\n");
            rtos_delay_milliseconds(300);
        }
    }

    if (tmp_read_buf)
    {
        os_free(tmp_read_buf);
    }

    LOGE("audio thread exit\n");

    rtos_delete_thread(NULL);

    return 0;
}


static int ntwk_cs2_p2p_interface_core(p2p_cs2_key_t *key)
{
    int Repeat = 0;
    int32_t ret = 0;

    int session = -99;
    const unsigned long Total_Times = Repeat;
    uint8_t *tmp_read_buf = NULL;

    (void)Total_Times;
    Repeat = 0;

    ret = PPCS_NetworkDetect(&s_cs2_p2p_networkinfo, 0);
    show_network(s_cs2_p2p_networkinfo);

    tmp_read_buf = ntwk_malloc(RECV_TMP_CMD_BUFF_SIZE);

    if (!tmp_read_buf)
    {
        LOGE("p2p", "%s alloc err\n", __func__);
        return -1;
    }

    // Send CS2 service start event
    ntwk_msg_event_report(NTWK_TRANS_EVT_START, 0, NTWK_TRANS_CHAN_CTRL);

    while (ntwk_cs2_info->is_running)//Repeat < Total_Times)
    {
        Repeat++;

        ret = PPCS_NetworkDetect(&s_cs2_p2p_networkinfo, 0);

        if (ret < 0)
        {
            LOGE("p2p", "%s PPCS_NetworkDetect err %d %s\n", __func__, ret, get_p2p_error_code_info(ret));
            return -1;
        }

        s_current_sessionid = session = cs2_p2p_listen(key->did, key->apilicense, Repeat, &ntwk_cs2_info->is_running);

        if (0 <= session)
        {
            BK_LOGD("p2p", "%s listen Sid %d\n", __func__, s_current_sessionid);
            ntwk_cs2_video_timer_init();
            ntwk_cs2_info->device_connected = BK_TRUE;

            ntwk_cs2_info->video_status = BK_TRUE;
            ntwk_cs2_info->aud_status = BK_TRUE;

            do
            {
                INT32 ReadSize = 0;

                ReadSize = RECV_TMP_CMD_BUFF_SIZE;

                ret = PPCS_Read(session, CMD_P2P_CHANNEL, (char *)tmp_read_buf, &ReadSize, 1000 * 2);

                if (ReadSize)
                {
                    LOGD("got cmd count: %d\n", ReadSize);
                    ntwk_cs2_info->ctrl_receive_cb(tmp_read_buf, ReadSize);
                    continue;
                }

                if (ret == ERROR_PPCS_TIME_OUT)
                {
                    LOGV("got cmd data timeout, %d\n", ReadSize);
                    continue;
                }

                if (ret < 0 && ERROR_PPCS_TIME_OUT != ret)
                {
                    LOGD("%s PPCS_Read ret err %d %s\n", __func__, ret, get_p2p_error_code_info(ret));
                    goto READ_ERR;
                }
            }
            while (ntwk_cs2_info->is_running);
READ_ERR:

            LOGE("cmd session thread error, close\r\n");

            ntwk_cs2_session_close();

            session = -1;

            rtos_delay_milliseconds(300); // 两次 PPCS_Listen 之间需要保持间隔。

            continue;
        }
        else if (ERROR_PPCS_MAX_SESSION == session)
        {
            rtos_delay_milliseconds(1 * 1000);
        }
        else if (session == -1)
        {
            break;
        }

        ntwk_cs2_info->video_status = BK_FALSE;
        ntwk_cs2_info->aud_status = BK_FALSE;

    }


    if (tmp_read_buf)
    {
        os_free(tmp_read_buf);
    }

    if (s_current_sessionid >= 0)
    {
        PPCS_ForceClose(s_current_sessionid);
        s_current_sessionid = -1;
    }

    return 0;
}


static void ntwk_cs2_service_main(beken_thread_arg_t arg)
{
    ntwk_cs2_p2p_interface_init(ntwk_cs2_info->cs2_key);

    ntwk_cs2_info->is_running = 1;

    ntwk_cs2_p2p_interface_core(ntwk_cs2_info->cs2_key);

    ntwk_cs2_p2p_interface_deinit();
}

bk_err_t ntwk_cs2_ctrl_chan_start(void *param)
{
    bk_err_t ret;
    p2p_cs2_key_t *p2p_cs2_key = NULL;

    LOGD("%s\n", __func__);

    if (param == NULL)
    {
        LOGE("%s param is NULL\n", __func__);
        return BK_FAIL;
    }

    p2p_cs2_key = (p2p_cs2_key_t *)param;

    if (p2p_cs2_key == NULL
        || p2p_cs2_key->did == NULL
        || p2p_cs2_key->apilicense == NULL
        || p2p_cs2_key->initstring == NULL)
    {
        LOGE("%s p2p_cs2_key null\n", __func__);
        return BK_FAIL;
    }

    if (ntwk_cs2_info == NULL)
    {
        return BK_FAIL;
    }

    ntwk_cs2_info->cs2_key = p2p_cs2_key;

    rtos_init_mutex(&ntwk_cs2_info->tx_lock);

    rtos_init_mutex(&ntwk_cs2_info->mutex);

    ret = rtos_init_semaphore_ex(&ntwk_cs2_info->aud_sem, 1, 0);

    ret = rtos_create_thread(&ntwk_cs2_info->thd,
                             4,
                             "cs2",
                             (beken_thread_function_t)ntwk_cs2_service_main,
                             1024 * 8,
                             (beken_thread_arg_t)NULL);
    if (ret != BK_OK)
    {
        LOGE("Error: Failed to create cs2 service: %d\n", ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t ntwk_cs2_ctrl_chan_stop(void)
{
    LOGD("%s\n", __func__);

    if (ntwk_cs2_info == NULL)
    {
        LOGW("%s: ntwk_cs2_info is already NULL\n", __func__);
        return BK_FAIL;
    }

    // 停止服务运行标志
    ntwk_cs2_info->is_running = BK_FALSE;

    // 停止音频线程运行
    ntwk_cs2_info->aud_running = BK_FALSE;

    // 关闭当前会话
    if (s_current_sessionid >= 0)
    {
        ntwk_cs2_session_close();
    }

    // 释放音频信号量
    rtos_deinit_semaphore(&ntwk_cs2_info->aud_sem);

    // 释放互斥锁
    rtos_deinit_mutex(&ntwk_cs2_info->mutex);
    rtos_deinit_mutex(&ntwk_cs2_info->tx_lock);

    ntwk_cs2_deinit();
    LOGD("%s: CS2 service deinitialized successfully\n", __func__);
    return BK_OK;
}

bk_err_t ntwk_cs2_init(void)
{
    if (ntwk_cs2_info != NULL)
    {
        LOGW("%s ntwk_cs2_info already initialized\n", __func__);
        return BK_OK;
    }

    ntwk_cs2_info = ntwk_malloc(sizeof(ntwk_cs2_info_t));

    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s ntwk_cs2_info malloc failed\n", __func__);
        return BK_FAIL;
    }

    os_memset(ntwk_cs2_info, 0, sizeof(ntwk_cs2_info_t));

    return BK_OK;
}

bk_err_t ntwk_cs2_deinit(void)
{
    if (ntwk_cs2_info != NULL)
    {
        os_free(ntwk_cs2_info);
        ntwk_cs2_info = NULL;
    }

    return BK_OK;
}

bk_err_t ntwk_cs2_audio_chan_start(void *param)
{
    bk_err_t ret;

    LOGD("%s: start\n", __func__);

    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s, ntwk_cs2_info is NULL\n", __func__);
        return BK_FAIL;
    }

    if (ntwk_cs2_info->aud_running == BK_TRUE)
    {
        LOGW("%s already open\n", __func__);
        return BK_OK;
    }

    ntwk_cs2_info->aud_running = BK_TRUE;

    ret = rtos_create_thread(&ntwk_cs2_info->thd,
                                4,
                                "audo",
                                (beken_thread_function_t)ntwk_cs2_p2p_audio_receiver,
                                1024 * 4,
                                (beken_thread_arg_t)NULL);


    if (ret != BK_OK)
    {
        LOGE("%s init thread failed\n", __func__);
        goto error;
    }

    ret = rtos_get_semaphore(&ntwk_cs2_info->aud_sem, BEKEN_WAIT_FOREVER);

    if (ret != BK_OK)
    {
        LOGE("%s wait semaphore failed\n", __func__);
        goto error;
    }

    return BK_OK;

error:

    return BK_FAIL;
}

bk_err_t ntwk_cs2_video_chan_start(void *param)
{
    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s, service null\n", __func__);
        return BK_FAIL;
    }

    LOGD("%s, video channel started\n", __func__);
    return BK_OK;
}

bk_err_t ntwk_cs2_video_chan_stop(void)
{
    return BK_OK;
}

bk_err_t ntwk_cs2_audio_chan_stop(void)
{
    return BK_OK;
}

bk_err_t ntwk_cs2_ctrl_register_receive_cb(ntwk_cs2_ctrl_receive_cb_t cb)
{
    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s: Control channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwk_cs2_info->ctrl_receive_cb = cb;
    LOGD("%s: Receive callback registered successfully\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_cs2_video_register_receive_cb(ntwk_video_receive_cb_t cb)
{
    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s: Video channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwk_cs2_info->video_receive_cb = cb;
    LOGD("%s: Video receive callback registered successfully\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_cs2_audio_register_receive_cb(ntwk_audio_receive_cb_t cb)
{
    if (ntwk_cs2_info == NULL)
    {
        LOGE("%s: Audio channel not initialized\n", __func__);
        return BK_FAIL;
    }

    if (cb == NULL)
    {
        LOGE("%s: Invalid callback\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwk_cs2_info->audio_receive_cb = cb;
    LOGD("%s: Audio receive callback registered successfully\n", __func__);

    return BK_OK;
}





