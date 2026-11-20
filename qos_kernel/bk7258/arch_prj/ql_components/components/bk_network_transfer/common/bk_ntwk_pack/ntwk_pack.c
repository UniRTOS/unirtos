// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "bk_wifi.h"
#include <modules/wifi.h>
#include "modules/wifi_types.h"
#include <driver/dma.h>
#include "bk_general_dma.h"
#include "media_utils.h"

#include "avdk_crc.h"
#if CONFIG_ARCH_CM33
#include <driver/aon_rtc.h>
#endif

#include "network_transfer.h"
#include "ntwk_pack.h"
#include "network_type.h"
#include "network_transfer_internal.h"


#define TAG "ntwk-pack"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

ntwk_pack_info_t g_ntwk_pkt_info = {0};

/**
 * @brief Channel manager array indexed by chan_type
 * [0] = NTWK_TRANS_CHAN_CTRL
 * [1] = NTWK_TRANS_CHAN_VIDEO
 * [2] = NTWK_TRANS_CHAN_AUDIO
 */
static ntwk_pkt_chan_mgr_t *g_pkt_chan_mgr[NTWK_TRANS_CHAN_MAX] = {NULL};

int ntwk_pack_get_header_size(void)
{
    return sizeof(ntwk_pack_head_t);
}
/**
 * @brief Internal adapter callback for ntwk_pkt_unpack
 * Adapts detailed callback parameters to simple user callback
 */
static void ntwk_pkt_recv_adapter(void *channel, uint16_t sequence, uint16_t flags, 
                                   uint32_t timestamp, uint8_t sequences, 
                                   uint8_t *data, uint16_t length)
{
    ntwk_pack_chan_t *chan = (ntwk_pack_chan_t *)channel;
    uint8_t chan_type = chan->chan_type;

    if (chan_type < NTWK_TRANS_CHAN_MAX && g_pkt_chan_mgr[chan_type] != NULL && g_pkt_chan_mgr[chan_type]->recv_cb != NULL) {
        g_pkt_chan_mgr[chan_type]->recv_cb(chan_type, data, length);
    }
}

void ntwk_hex_dump(uint8_t *data, uint32_t length)
{
#ifdef DUMP_DEBUG
    for (int i = 0; i < length; i++)
    {
        BK_RAW_LOGD(TAG, "%02X ", data[i]);

        if ((i + 1) % 20 == 0)
        {
            BK_RAW_LOGD(TAG, "\r\n");
        }
    }
    BK_RAW_LOGD(TAG, "\r\n");
#endif
}

ntwk_pack_chan_t *ntwk_pkt_malloc(uint16_t max_rx_size, uint16_t max_tx_size)
{
    ntwk_pack_chan_t *ntwk_db_chan = (ntwk_pack_chan_t *)ntwk_malloc(sizeof(ntwk_pack_chan_t));

    if (ntwk_db_chan == NULL)
    {
        LOGE("malloc ntwk_db_chan failed\n");
        goto error;
    }

    os_memset(ntwk_db_chan, 0, sizeof(ntwk_pack_chan_t));

    ntwk_db_chan->cbuf = ntwk_malloc(max_rx_size + sizeof(ntwk_pack_head_t));

    if (ntwk_db_chan->cbuf == NULL)
    {
        LOGE("malloc cache buffer failed\n");
        goto error;
    }

    ntwk_db_chan->csize = max_rx_size + sizeof(ntwk_pack_head_t);

    ntwk_db_chan->tbuf = ntwk_malloc(max_tx_size + sizeof(ntwk_pack_head_t));

    if (ntwk_db_chan->tbuf == NULL)
    {
        LOGE("malloc cache buffer failed\n");
        goto error;
    }

    ntwk_db_chan->tsize = max_tx_size;

    LOGV("%s, %p, %p %d, %p %d\n", __func__, ntwk_db_chan, ntwk_db_chan->cbuf, ntwk_db_chan->csize, ntwk_db_chan->tbuf, ntwk_db_chan->tsize);

    return ntwk_db_chan;


error:

    if (ntwk_db_chan->cbuf)
    {
        os_free(ntwk_db_chan->cbuf);
        ntwk_db_chan->cbuf = NULL;
    }

    if (ntwk_db_chan)
    {
        os_free(ntwk_db_chan);
        ntwk_db_chan = NULL;
    }

    return ntwk_db_chan;
}

void ntwk_pkt_free(ntwk_pack_chan_t *channel)
{
    if (channel == NULL)
    {
        LOGW("%s, channel is NULL\n", __func__);
        return;
    }

    LOGV("%s, %p, %p, %p\n", __func__, channel, channel->cbuf, channel->tbuf);

    if (channel->cbuf)
    {
        os_free(channel->cbuf);
        channel->cbuf = NULL;
    }

    if (channel->tbuf)
    {
        os_free(channel->tbuf);
        channel->tbuf = NULL;
    }

    os_free(channel);
}

bk_err_t ntwk_pack_clear_ccount(chan_type_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        //LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    if (g_pkt_chan_mgr[chan_type] == NULL) {
        //LOGE("%s: chan_type %d not initialized\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    if (!g_pkt_chan_mgr[chan_type]->initialized) {
        //LOGE("%s: chan_type %d not initialized\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    ntwk_pack_chan_t *chan = g_pkt_chan_mgr[chan_type]->channel;
    if (chan == NULL) {
        LOGE("%s: channel is NULL for chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    chan->ccount = 0;
    LOGV("%s: cleared ccount for chan_type %d\n", __func__, chan_type);

    return BK_OK;
}

void ntwk_pkt_unpack(void *channel, uint8_t *data, uint32_t length, pack_recive_cb_t cb)
{
    ntwk_pack_chan_t *chan = (ntwk_pack_chan_t *)channel;
    ntwk_pack_head_t head, *ptr;
    uint8_t *p = data;

    uint32_t left = length;
    int cp_len = 0;

#ifdef DUMP_DEBUG
    static uint32_t count = 0;

    LOGV("DUMP DATA %u, size: %u\n", count++, length);

    ntwk_hex_dump(data, length);
#else
    LOGV("recv unpack: %u\n", length);
#endif

    while (left != 0)
    {
        if (chan->ccount == 0)
        {
            if (left < HEAD_SIZE_TOTAL)
            {
                //LOGE("left head size not enough: %d, ccount: %d\n", left, chan->ccount);
                // Check buffer boundary to prevent overflow
                if (chan->ccount + left > chan->csize) {
                    LOGE("cbuf overflow L%d: ccount=%d, left=%d, csize=%d\n",__LINE__, chan->ccount, left, chan->csize);
                }
                os_memcpy(chan->cbuf + chan->ccount, p, left);
                chan->ccount += left;
                break;
            }

            ptr = (ntwk_pack_head_t *)p;

            head.magic = CHECK_ENDIAN_UINT16(ptr->magic);

            if (head.magic == HEAD_MAGIC_CODE)
            {
                /*
                *   Magic code  2 bytes
                *   Flags       2 bytes
                *   Timestamp   4 bytes
                *   Squence     2 bytes
                *   Length      2 bytes
                *   CRC         1 byte
                *   RESERVED    3 byte
                */

                head.flags = CHECK_ENDIAN_UINT16(ptr->flags);
                head.timestamp = CHECK_ENDIAN_UINT32(ptr->timestamp);
                head.sequence = CHECK_ENDIAN_UINT16(ptr->sequence);
                head.length = CHECK_ENDIAN_UINT16(ptr->length);
                head.crc = ptr->crc;
                head.reserved[0] = ptr->reserved[0];
                head.reserved[1] = ptr->reserved[1];
                head.reserved[2] = ptr->reserved[2];
#ifdef DEBUG_HEAD
                LOGD("head size: %d, %d, flags: %04X\n", HEAD_SIZE_TOTAL, sizeof(ntwk_pack_head_t), head.flags);
                LOGD("time: %u, len: %u, seq: %u, crc: %02X\n",
                     head.timestamp, head.length, head.sequence, head.crc);
#endif
            }
            else
            {
                LOGE("invaild src data\n");
                ntwk_hex_dump(p, left);
                LOGE("dump src data\n");
                //TODO FIXME
                ntwk_hex_dump(data, length);
                break;
            }

            if (left < head.length + HEAD_SIZE_TOTAL)
            {
                //LOGE("left payload size not enough: %d, ccount: %d, pay len: %d\n", left, chan->ccount, head.length);
                // Check buffer boundary to prevent overflow
                if (chan->ccount + left > chan->csize) {
                    LOGE("cbuf overflow L%d: ccount=%d, left=%d, csize=%d, head.length=%d\n",__LINE__, chan->ccount, left, chan->csize, head.length);
                }
                os_memcpy(chan->cbuf + chan->ccount, p, left);
                chan->ccount += left;
                break;
            }

#ifdef DEBUG_CRC
            if (HEAD_FLAGS_CRC & head.flags)
            {

                uint8_t ret_crc = hnd_crc8(p + HEAD_SIZE_TOTAL, head.length, CRC8_INIT_VALUE);

                if (ret_crc != head.crc)
                {
                    LOGD("check crc failed\n");
                }

                LOGD("CRC SRC: %02X,  CALC: %02X\n", head.crc, ret_crc);
            }
#endif

            if (cb)
            {
                cb(chan, head.sequence, head.flags, head.timestamp, head.sequence, ptr->payload, head.length);
            }

            p += HEAD_SIZE_TOTAL + head.length;
            left -= HEAD_SIZE_TOTAL + head.length;
        }
        else
        {
            if (chan->ccount < HEAD_SIZE_TOTAL)
            {
                cp_len = HEAD_SIZE_TOTAL - chan->ccount;

                if (cp_len < 0)
                {
                    //LOGE("cp_len error: %d at %d\n", cp_len, __LINE__);
                    break;
                }


                if (left < cp_len)
                {
                    // Check buffer boundary to prevent overflow
                    if (chan->ccount + left > chan->csize) {
                        LOGE("cbuf overflow: ccount=%d, left=%d, csize=%d\n", chan->ccount, left, chan->csize);
                    }
                    os_memcpy(chan->cbuf + chan->ccount, p, left);
                    chan->ccount += left;
                    left = 0;
                    //LOGE("cp_len head size not enough: %d, ccount: %d\n", cp_len, chan->ccount);
                    break;
                }
                else
                {
                    // Check buffer boundary to prevent overflow
                    if (chan->ccount + cp_len > chan->csize) {
                        LOGE("cbuf overflow L%d: ccount=%d, cp_len=%d, csize=%d\n",__LINE__, chan->ccount, cp_len, chan->csize);
                    }
                    os_memcpy(chan->cbuf + chan->ccount, p, cp_len);
                    chan->ccount += cp_len;
                    p += cp_len;
                    left -= cp_len;
                }
            }

            ptr = (ntwk_pack_head_t *)chan->cbuf;

            head.magic = CHECK_ENDIAN_UINT32(ptr->magic);

            if (head.magic == HEAD_MAGIC_CODE)
            {
                /*
                *   Magic code  2 bytes
                *   Flags       2 bytes
                *   Timestamp   4 bytes
                *   Squence     2 bytes
                *   Length      2 bytes
                *   CRC         1 byte
                *   RESERVED    3 byte
                */

                head.flags = CHECK_ENDIAN_UINT16(ptr->flags);
                head.timestamp = CHECK_ENDIAN_UINT32(ptr->timestamp);
                head.sequence = CHECK_ENDIAN_UINT16(ptr->sequence);
                head.length = CHECK_ENDIAN_UINT16(ptr->length);
                head.crc = ptr->crc;
                head.reserved[0] = ptr->reserved[0];
                head.reserved[1] = ptr->reserved[1];
                head.reserved[2] = ptr->reserved[2];

#ifdef DEBUG_HEAD
                LOGD("head size: %d, %d, flags: %04X\n", HEAD_SIZE_TOTAL, sizeof(ntwk_pack_head_t), head.flags);
                LOGD("time: %u, len: %u, seq: %u, crc: %02X\n",
                     head.timestamp, head.length, head.sequence, head.crc);
#endif
            }
            else
            {
                LOGE("invaild cached data, %04X, %d\n", head.magic, __LINE__);
                ntwk_hex_dump(chan->cbuf, chan->ccount);
                //TODO FIXME
                break;
            }

            if (chan->ccount < HEAD_SIZE_TOTAL + head.length)
            {
                cp_len = head.length + HEAD_SIZE_TOTAL - chan->ccount;

                if (cp_len < 0)
                {
                    LOGE("cp_len error: %d at %d\n", cp_len, __LINE__);
                    break;
                }

                if (left < cp_len)
                {
                    // Check buffer boundary to prevent overflow
                    if (chan->ccount + left > chan->csize) {
                        LOGE("cbuf overflow L%d: ccount=%d, left=%d, csize=%d\n",__LINE__, chan->ccount, left, chan->csize);
                    }
                    os_memcpy(chan->cbuf + chan->ccount, p, left);
                    chan->ccount += left;
                    left = 0;
                    ///LOGE("cp_len payload size not enough: %d, ccount: %d\n", cp_len, chan->ccount);
                    break;
                }
                else
                {
                    // Check buffer boundary to prevent overflow
                    if (chan->ccount + cp_len > chan->csize) {
                        LOGE("cbuf overflow L%d: ccount=%d, cp_len=%d, csize=%d\n",__LINE__, chan->ccount, cp_len, chan->csize);
                    }
                    os_memcpy(chan->cbuf + chan->ccount, p, cp_len);
                    left -= cp_len;
                    p += cp_len;
                    chan->ccount += cp_len;
                }

#ifdef DEBUG_CRC
                if (HEAD_FLAGS_CRC & head.flags)
                {

                    uint8_t ret_crc = hnd_crc8(chan->cbuf + HEAD_SIZE_TOTAL, head.length, CRC8_INIT_VALUE);

                    if (ret_crc != head.crc)
                    {
                        LOGD("check crc failed\n");
                    }

                    LOGD("CRC SRC: %02X,  CALC: %02X\n", head.crc, ret_crc);
                }
#endif

                if (cb)
                {
                    cb(chan, head.sequence, head.flags, head.timestamp, head.sequence, ptr->payload, head.length);
                }

                //LOGD("cached: %d, left: %d\n", chan->ccount, left);

                chan->ccount = 0;
            }
            else
            {
                LOGE("invaild flow data\n");
                ntwk_hex_dump(chan->cbuf, chan->ccount);
                //SHOULD NOT BE HERE
                //TODO FIMXME
                break;
            }
        }
    }

    //LOGD("next cached: %d\n", chan->ccount);
}

uint32_t ntwk_pkt_get_milliseconds(void)
{
    uint32_t time = 0;

#if CONFIG_ARCH_RISCV
    extern u64 riscv_get_mtimer(void);

    time = (riscv_get_mtimer() / 26000) & 0xFFFFFFFF;
#elif CONFIG_ARCH_CM33

    //time = (bk_aon_rtc_get_us() / 1000) & 0xFFFFFFFF;
    bk_rtc_gettimeofday(&(g_ntwk_pkt_info.tv), 0);

    long time_offset = 8 * 3600; //8 hour offset
    long ms_time = (g_ntwk_pkt_info.tv.tv_usec / 1000);
    long time_ms = 0;

    if (g_ntwk_pkt_info.tv.tv_sec < time_offset)
    {
        //LOGD(" local time sec: %ld \r\n",g_ntwk_pkt_info.tv.tv_sec);
        time_ms = (g_ntwk_pkt_info.tv.tv_sec * 1000) + ms_time;
    }
    else
    {
        time_ms = ((g_ntwk_pkt_info.tv.tv_sec - time_offset) * 1000) + ms_time;
    }

    time = (time_ms & 0xFFFFFFFF);

    //LOGD("sec: %ld time %d\r\n", g_ntwk_pkt_info.tv.tv_sec ,time);
#endif

    return time;
}

void ntwk_pkt_pack(void *channel, uint8_t *data, uint32_t length)
{
    ntwk_pack_chan_t *chan = (ntwk_pack_chan_t *)channel;
    if (chan == NULL) {
        LOGE("%s: chan is NULL\n", __func__);
        return;
    }

    ntwk_pack_head_t *head = chan->tbuf;

    /*
    *   Magic code  2 bytes
    *   Flags       2 bytes
    *   Timestamp   4 bytes
    *   Squence     2 bytes
    *   Length      2 bytes
    *   CRC         1 byte
    *   RESERVED    3 byte
    */
    head->magic = CHECK_ENDIAN_UINT16(HEAD_MAGIC_CODE);
    head->flags = CHECK_ENDIAN_UINT16(HEAD_FLAGS_CRC);
    head->timestamp = CHECK_ENDIAN_UINT32(ntwk_pkt_get_milliseconds());
    head->sequence = CHECK_ENDIAN_UINT16(++chan->sequence);
    head->length = CHECK_ENDIAN_UINT16(length);
    head->crc = hnd_crc8(data, length, CRC8_INIT_VALUE);;
    head->reserved[0] = 0;
    head->reserved[1] = 0;
    head->reserved[2] = 0;

    os_memcpy(head->payload, data, length);

}

bk_err_t ntwk_pack_init(chan_type_t chan_type)
{
    if (g_pkt_chan_mgr[chan_type] != NULL) {
        LOGE("%s: g_pkt_chan_mgr already initialized\n", __func__);
        return BK_OK;
    }

    g_pkt_chan_mgr[chan_type] = ntwk_malloc(sizeof(ntwk_pkt_chan_mgr_t));
    if (g_pkt_chan_mgr[chan_type] == NULL) {
        LOGE("%s: failed to allocate g_pkt_chan_mgr\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(g_pkt_chan_mgr[chan_type], 0, sizeof(ntwk_pkt_chan_mgr_t));

    return BK_OK;
}

bk_err_t ntwk_pack_deinit(chan_type_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    if (g_pkt_chan_mgr[chan_type] == NULL) {
        LOGE("%s: g_pkt_chan_mgr not initialized\n", __func__);
        return BK_OK;
    }

    os_free(g_pkt_chan_mgr[chan_type]);
    g_pkt_chan_mgr[chan_type] = NULL;

    return BK_OK;
}

bk_err_t ntwk_pack_chan_start(chan_type_t chan_type, uint16_t max_rx_size, uint16_t max_tx_size)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    if (g_pkt_chan_mgr[chan_type] == NULL) {
        LOGW("%s: chan_type %d already initialized\n", __func__, chan_type);
        return BK_OK;
    }

    g_pkt_chan_mgr[chan_type]->channel = ntwk_pkt_malloc(max_rx_size, max_tx_size);
    if (g_pkt_chan_mgr[chan_type]->channel == NULL) {
        LOGE("%s: failed to allocate channel for chan_type %d\n", __func__, chan_type);
        return BK_ERR_NO_MEM;
    }

    g_pkt_chan_mgr[chan_type]->channel->chan_type = chan_type;
    g_pkt_chan_mgr[chan_type]->initialized = true;

    return BK_OK;
}

bk_err_t ntwk_pack_chan_stop(chan_type_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    if (g_pkt_chan_mgr[chan_type] == NULL) {
        LOGW("%s: chan_type %d not initialized\n", __func__, chan_type);
        return BK_OK;
    }

    if (!g_pkt_chan_mgr[chan_type]->initialized) {
        LOGW("%s: chan_type %d not initialized\n", __func__, chan_type);
        return BK_OK;
    }

    // Free channel
    if (g_pkt_chan_mgr[chan_type]->channel) {
        ntwk_pkt_free(g_pkt_chan_mgr[chan_type]->channel);
        g_pkt_chan_mgr[chan_type]->channel = NULL;
    }

    g_pkt_chan_mgr[chan_type]->recv_cb = NULL;
    g_pkt_chan_mgr[chan_type]->initialized = false;

    LOGV("%s: chan_type %d stopped\n", __func__, chan_type);

    return BK_OK;
}

bk_err_t ntwk_pack_register_recv_cb(chan_type_t chan_type, ntwk_pack_recv_cb_t recv_cb)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    if (g_pkt_chan_mgr[chan_type] == NULL) {
        LOGE("%s: recv_cb is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    g_pkt_chan_mgr[chan_type]->recv_cb = recv_cb;

    return BK_OK;
}

int ntwk_pack_pack_by_type(chan_type_t chan_type, uint8_t *data, uint32_t length, 
                          uint8_t **pack_ptr, uint32_t *pack_ptr_length)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return -1;
    }

    if (!g_pkt_chan_mgr[chan_type]->initialized) {
        return -1;
    }

    if (data == NULL || pack_ptr == NULL || pack_ptr_length == NULL) {
        LOGE("%s: invalid parameters\n", __func__);
        return -1;
    }

    ntwk_pack_chan_t *chan = g_pkt_chan_mgr[chan_type]->channel;
    if (chan == NULL) {
        LOGE("%s: channel is NULL for chan_type %d\n", __func__, chan_type);
        return -1;
    }

    ntwk_pkt_pack(chan, data, length);

    *pack_ptr = (uint8_t *)chan->tbuf;
    *pack_ptr_length = HEAD_SIZE_TOTAL + length;

    return (int)*pack_ptr_length;
}

int ntwk_pack_unpack_by_type(chan_type_t chan_type, uint8_t *data, uint32_t length)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX) {
        LOGE("%s: invalid chan_type %d\n", __func__, chan_type);
        return -1;
    }

    if (!g_pkt_chan_mgr[chan_type]->initialized) {
        LOGE("%s: chan_type %d not initialized\n", __func__, chan_type);
        return -1;
    }

    if (data == NULL || length == 0) {
        LOGE("%s: invalid parameters\n", __func__);
        return -1;
    }

    ntwk_pack_chan_t *chan = g_pkt_chan_mgr[chan_type]->channel;
    if (chan == NULL) {
        LOGE("%s: channel is NULL for chan_type %d\n", __func__, chan_type);
        return -1;
    }

    ntwk_pkt_unpack(chan, data, length, ntwk_pkt_recv_adapter);

    return length;
}

/* ============================================================================
 * Channel-specific pack/unpack functions (network_transfer.h compatible)
 * ============================================================================
 */

/**
 * @brief Control channel pack function
 * 
 * This function is compatible with ntwk_trans_ctrl_chan_t.pack interface
 * Internally uses chan_type = 0 (NTWK_TRANS_CHAN_CTRL)
 */
int ntwk_pack_ctrl_pack(uint8_t *data, uint32_t length, uint8_t **pack_ptr, uint32_t *pack_ptr_length)
{
    return ntwk_pack_pack_by_type(NTWK_TRANS_CHAN_CTRL, data, length, pack_ptr, pack_ptr_length);
}

/**
 * @brief Control channel unpack function
 * 
 * This function is compatible with ntwk_trans_ctrl_chan_t.unpack interface
 * Internally uses chan_type = 0 (NTWK_TRANS_CHAN_CTRL)
 */
int ntwk_pack_ctrl_unpack(uint8_t *data, uint32_t length)
{
    return ntwk_pack_unpack_by_type(NTWK_TRANS_CHAN_CTRL, data, length);
}

/**
 * @brief Video channel pack function
 * 
 * This function is compatible with ntwk_trans_video_chan_t.pack interface
 * Internally uses chan_type = 1 (NTWK_TRANS_CHAN_VIDEO)
 */
int ntwk_pack_video_pack(uint8_t *data, uint32_t length, uint8_t **pack_ptr, uint32_t *pack_ptr_length)
{
    return ntwk_pack_pack_by_type(NTWK_TRANS_CHAN_VIDEO, data, length, pack_ptr, pack_ptr_length);
}

/**
 * @brief Video channel unpack function
 * 
 * This function is compatible with ntwk_trans_video_chan_t.unpack interface
 * Internally uses chan_type = 1 (NTWK_TRANS_CHAN_VIDEO)
 */
int ntwk_pack_video_unpack(uint8_t *data, uint32_t length)
{
    return ntwk_pack_unpack_by_type(NTWK_TRANS_CHAN_VIDEO, data, length);
}

/**
 * @brief Audio channel pack function
 * 
 * This function is compatible with ntwk_trans_audio_chan_t.pack interface
 * Internally uses chan_type = 2 (NTWK_TRANS_CHAN_AUDIO)
 */
int ntwk_pack_audio_pack(uint8_t *data, uint32_t length, uint8_t **pack_ptr, uint32_t *pack_ptr_length)
{
    return ntwk_pack_pack_by_type(NTWK_TRANS_CHAN_AUDIO, data, length, pack_ptr, pack_ptr_length);
}

/**
 * @brief Audio channel unpack function
 * 
 * This function is compatible with ntwk_trans_audio_chan_t.unpack interface
 * Internally uses chan_type = 2 (NTWK_TRANS_CHAN_AUDIO)
 */
int ntwk_pack_audio_unpack(uint8_t *data, uint32_t length)
{
    return ntwk_pack_unpack_by_type(NTWK_TRANS_CHAN_AUDIO, data, length);
}
