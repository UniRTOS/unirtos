#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define NTWK_TRANS_CMD_PORT                 (7100)

#define NTWK_TRANS_UDP_VIDEO_PORT           (7180)
#define NTWK_TRANS_UDP_AUDIO_PORT           (7170)

#define NTWK_TRANS_TCP_VIDEO_PORT           (7150)
#define NTWK_TRANS_TCP_AUDIO_PORT           (7140)

#define NTWK_TRANS_UDP_DATA_MAX_SIZE        (1472)
#define NTWK_TRANS_TCP_DATA_MAX_SIZE        (1460)
#define NTWK_TRANS_DATA_MAX_SIZE            (1024)

#define NTWK_TRANS_CMD_BUFFER               (1460)

#define NTWK_TRANS_CTRL_CHAN_KEEPALIVE_ENABLE      (1)
#define NTWK_TRANS_CTRL_CHAN_KEEPALIVE_IDLE_TIME   (60)
#define NTWK_TRANS_CTRL_CHAN_KEEPALIVE_INTERVAL    (5)
#define NTWK_TRANS_CTRL_CHAN_KEEPALIVE_COUNT       (3)

#define NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_ENABLE      (1)
#define NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_IDLE_TIME   (30)
#define NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_INTERVAL    (5)
#define NTWK_TRANS_VIDEO_CHAN_KEEPALIVE_COUNT       (3)

#define NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_ENABLE       (1)
#define NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_IDLE_TIME   (30)
#define NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_INTERVAL    (5)
#define NTWK_TRANS_AUDIO_CHAN_KEEPALIVE_COUNT       (3)


#define TRANSMISSION_BIG_ENDIAN (BK_FALSE)


#if TRANSMISSION_BIG_ENDIAN == BK_TRUE
#define CHECK_ENDIAN_UINT32(var)    htonl(var)
#define CHECK_ENDIAN_UINT16(var)    htons(var)

#define STREAM_TO_UINT16(u16, p) {u16 = (((uint16_t)(*((p) + 1))) + (((uint16_t)(*((p)))) << 8)); (p) += 2;}
#define STREAM_TO_UINT32(u32, p) {u32 = ((((uint32_t)(*((p) + 3)))) + ((((uint32_t)(*((p) + 2)))) << 8) + ((((uint32_t)(*((p) + 1)))) << 16) + ((((uint32_t)(*((p))))) << 24)); (p) += 4;}


#else
#define CHECK_ENDIAN_UINT32
#define CHECK_ENDIAN_UINT16

#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define STREAM_TO_UINT32(u32, p) {u32 = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + ((((uint32_t)(*((p) + 2)))) << 16) + ((((uint32_t)(*((p) + 3)))) << 24)); (p) += 4;}


#endif

#define STREAM_TO_UINT8(u8, p) {u8 = (uint8_t)(*(p)); (p) += 1;}

#ifdef __cplusplus
}
#endif

