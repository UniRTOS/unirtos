
#pragma once

#include <os/os.h>
#include <common/bk_include.h>
#include <components/video_types.h>
#include <common/bk_err.h>

#if CONFIG_ARCH_CM33
#include <driver/aon_rtc.h>
#endif

#include "network_transfer.h"
#ifdef __cplusplus
extern "C" {
#endif

#define HEAD_MAGIC_CODE             (0xF0D5)
#define HEAD_FLAGS_CRC              (1 << 0)
#define CRC8_INIT_VALUE 0xFF

typedef struct
{
	uint16_t magic;         /**<  Magic code  2 bytes */
	uint16_t flags;         /**<  Flags  2 bytes */
	uint32_t timestamp;     /**<  Timestamp  4 bytes */
	uint16_t sequence;      /**<  Sequence  2 bytes */
	uint16_t length;        /**<  Length  2 bytes */
	uint8_t crc;            /**<  CRC  1 byte */
	uint8_t reserved[3];    /**<  RESERVED  3 bytes */
	uint8_t  payload[];     /**<  Payload  variable length */
} __attribute__((__packed__)) ntwk_pack_head_t;

typedef struct
{
	uint8_t *cbuf;
	uint16_t csize;
	uint16_t ccount;
	uint16_t sequence;
	ntwk_pack_head_t *tbuf;
	uint16_t tsize;
	uint8_t chan_type;
} ntwk_pack_chan_t;

typedef struct
{
    struct timeval tv;
} ntwk_pack_info_t;

#define HEAD_SIZE_TOTAL             (sizeof(ntwk_pack_head_t))

typedef void (*pack_recive_cb_t)(void *channel, uint16_t sequence, uint16_t flags, uint32_t timestamp, uint8_t sequences, uint8_t *data, uint16_t length);

/**
 * @brief Packet receive callback type (internal use)
 * Compatible with ctrl/video/audio channel's recive callback signature
 */
typedef int (*ntwk_pack_recv_cb_t)(chan_type_t chan_type, uint8_t *data, uint32_t length);

/**
 * @brief Packet channel management structure
 * Manages channel and receive callback for each channel type
 */
typedef struct {
    ntwk_pack_chan_t *channel;           /**< Channel pointer for packet operations */
    ntwk_pack_recv_cb_t recv_cb;  /**< User receive callback function */
    bool initialized;                   /**< Channel initialization state */
} ntwk_pkt_chan_mgr_t;


bk_err_t ntwk_pack_init(chan_type_t chan_type);

bk_err_t ntwk_pack_deinit(chan_type_t chan_type);
/**
 * @brief Start/initialize packet channel for specified channel type
 * @param chan_type Channel type (CTRL/VIDEO/AUDIO)
 * @param max_rx_size Maximum receive buffer size
 * @param max_tx_size Maximum transmit buffer size
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_pack_chan_start(chan_type_t chan_type, uint16_t max_rx_size, uint16_t max_tx_size);

/**
 * @brief Stop/deinitialize packet channel for specified channel type
 * @param chan_type Channel type (CTRL/VIDEO/AUDIO)
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_pack_chan_stop(chan_type_t chan_type);

/**
 * @brief Register receive callback for specified channel type
 * @param chan_type Channel type (CTRL/VIDEO/AUDIO)
 * @param recv_cb User receive callback function
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_pack_register_recv_cb(chan_type_t chan_type, ntwk_pack_recv_cb_t recv_cb);

/**
 * @brief Pack data for specified channel type (network_transfer.h compatible)
 * @param chan_type Channel type (CTRL/VIDEO/AUDIO)
 * @param data Data to pack
 * @param length Data length
 * @param pack_ptr Output pointer to packed data (with header)
 * @param pack_ptr_length Output length of packed data
 * @return int Packed data length on success, negative on failure
 */
int ntwk_pack_pack_by_type(chan_type_t chan_type, uint8_t *data, uint32_t length, 
                          uint8_t **pack_ptr, uint32_t *pack_ptr_length);

/**
 * @brief Unpack data for specified channel type (network_transfer.h compatible)
 * @param chan_type Channel type (CTRL/VIDEO/AUDIO)
 * @param data Packed data to unpack
 * @param length Packed data length
 * @return int Unpacked data length on success, negative on failure
 */
int ntwk_pack_unpack_by_type(chan_type_t chan_type, uint8_t *data, uint32_t length);

/* Channel-specific pack/unpack functions (network_transfer.h compatible) */

/**
 * @brief Control channel pack function
 * Compatible with ntwk_trans_ctrl_chan_t.pack
 */
int ntwk_pack_ctrl_pack(uint8_t *data, uint32_t length, uint8_t **pack_ptr, uint32_t *pack_ptr_length);

/**
 * @brief Control channel unpack function
 * Compatible with ntwk_trans_ctrl_chan_t.unpack
 */
int ntwk_pack_ctrl_unpack(uint8_t *data, uint32_t length);

/**
 * @brief Video channel pack function
 * Compatible with ntwk_trans_video_chan_t.pack
 */
int ntwk_pack_video_pack(uint8_t *data, uint32_t length, uint8_t **pack_ptr, uint32_t *pack_ptr_length);

/**
 * @brief Video channel unpack function
 * Compatible with ntwk_trans_video_chan_t.unpack
 */
int ntwk_pack_video_unpack(uint8_t *data, uint32_t length);

/**
 * @brief Audio channel pack function
 * Compatible with ntwk_trans_audio_chan_t.pack
 */
int ntwk_pack_audio_pack(uint8_t *data, uint32_t length, uint8_t **pack_ptr, uint32_t *pack_ptr_length);

/**
 * @brief Audio channel unpack function
 * Compatible with ntwk_trans_audio_chan_t.unpack
 */
int ntwk_pack_audio_unpack(uint8_t *data, uint32_t length);

int ntwk_pack_get_header_size(void);

bk_err_t ntwk_pack_clear_ccount(chan_type_t chan_type);

#ifdef __cplusplus
}
#endif

