#pragma once

#include <os/os.h>
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/video_types.h>
#if (CONFIG_CS2_P2P_SERVER || CONFIG_CS2_P2P_CLIENT)
#include "PPCS_cs2_comm.h"
#endif

#include "network_type.h"
#include "common/network_transfer_common.h"
#include "common/bk_ntwk_sdp/ntwk_sdp.h"

#ifdef __cplusplus
extern "C" {
#endif


#define NTWK_TRANS_SERVICE_NAME_LEN 100
#define NTWK_TRANS_DEFAULT_PAYLOAD_SIZE  0

typedef enum {
    NTWK_TRANS_CHAN_CTRL = 0,      /**< Control channel*/
    NTWK_TRANS_CHAN_VIDEO,         /**< Video channel */
    NTWK_TRANS_CHAN_AUDIO,         /**< Audio channel*/
    NTWK_TRANS_CHAN_MAX
} chan_type_t;

typedef enum
{
    NTWK_TRANS_CHAN_START = 0,
    NTWK_TRANS_CHAN_WAITING_CONNECTED,
    NTWK_TRANS_CHAN_CONNECTED,
    NTWK_TRANS_CHAN_DISCONNECTED,
    NTWK_TRANS_CHAN_STOP,
} ntwk_trans_chan_state;

typedef enum {
    NTWK_TRANS_EVT_START,        /**< Event: Start the channel state machine. Triggers initialization or preparation for entering waiting/connection flow. */
    NTWK_TRANS_EVT_CONNECTED,    /**< Event: Remote connection is established. Data link is ready for transmission. */
    NTWK_TRANS_EVT_DISCONNECTED, /**< Event: Connection closed or error occurred; channel returns to waiting state for next remote connection. */
    NTWK_TRANS_EVT_STOP,         /**< Event: State machine stops; clean up and deinitialization should be performed. */
} evt_code_t;

typedef struct
{
    chan_type_t chan_type;     /**< Type of the channel where the event occurred */
    evt_code_t code;           /**< Event code indicating the specific network transfer event/state */
    int param;            /**< Additional event-specific parameters or data, context dependent */
} ntwk_trans_event_t;

#if CONFIG_NTWK_CLIENT_SERVICE_ENABLE
typedef struct ntwk_server_net_info
{
    uint8_t ip_addr[16];
    uint8_t cmd_port[6];
    uint8_t video_port[6];
    uint8_t audio_port[6];
}ntwk_server_net_info_t;
#endif

/**
 * @brief Event callback function type for network transfer events
 * @param event Pointer to event structure
 */
typedef void (*ntwk_trans_msg_event_cb_t)(ntwk_trans_event_t *event);

/**
 * @brief User receive callback type for packet channel
 * Compatible with ctrl/video/audio channel's recive callback signature
 */
typedef int (*ntwk_trans_recv_cb_t)(uint8_t *data, uint32_t length);

/**
 * @brief Network transfer control channel structure
 *
 * This structure defines the callback functions for the control channel in the network transfer module,
 * used to manage operations such as sending, receiving, packing, and unpacking control commands and signal data.
 * The control channel mainly transmits system control information, such as connection establishment and parameter configuration.
 */
typedef struct {
    /**
     * @brief Channel type
     * @return chan_type_t Channel type
     */
    chan_type_t type;

    /**
     * @brief Send control data
     *
     * Send control commands and signal data to the remote device
     *
     * @param data Control data buffer
     * @param length Data length
     * @return int Result of sending, returns 0 on success, negative value on failure
     */
    int (*send)(uint8_t *data, uint32_t length);

    /**
     * @brief Receive control data
     *
     * Receive control commands and signal data from the remote device
     *
     * @param data Receive buffer
     * @param length Length of the buffer
     * @return int Result of receiving, returns the number of bytes received on success, negative value on failure
     */
    int (*recive)(uint8_t *data, uint32_t length);

    /**
     * @brief Pack control data
     *
     * Pack the control data according to the network transmission protocol, adding necessary header information
     *
     * @param data Control data
     * @param length Data length
     * @param pack_ptr Packed data pointer
     * @param pack_ptr_length Packed data length
     * @return int Packing result, returns the length of packed data on success, negative value on failure
     */
    int (*pack)(uint8_t *data, uint32_t length, uint8_t **pack_ptr, uint32_t *pack_ptr_length);

    /**
     * @brief Unpack control data
     *
     * Extract control data from the network packet and remove header information
     *
     * @param data Network packet
     * @param length Length of the packet
     * @return int Unpacking result, returns the length of unpacked data on success, negative value on failure
     */
    int (*unpack)(uint8_t *data, uint32_t length);
        /**
     * @brief Fragment data
     * @param data Fragment data
     * @param length Length of the fragment
     * @return bk_err_t Error code, BK_OK on success, others on failure
     */
    bk_err_t (*fragment)(uint8_t *data, uint32_t length);
    /**
     * @brief Unfragment data
     * @param data Fragment data
     * @param length Length of the fragment
     * @return bk_err_t Error code, BK_OK on success, others on failure
     */
    bk_err_t (*unfragment)(uint8_t *data, uint32_t length);
} ntwk_trans_ctrl_chan_t;

/**
 * @brief Network transfer video channel structure
 *
 * This structure defines the callback functions for video channel operations in the network transfer module,
 * used to manage operations such as sending, receiving, packing, unpacking, dropping frames, and the frame queue for video data.
 */
typedef struct {
    /**
     * @brief Channel type
     * @return chan_type_t Channel type
     */
    chan_type_t type;
    /**
     * @brief Video format type
     * @return image_format_t Video format type
     */
    image_format_t vid_type;

    /**
     * @brief Send video data
     *
     * @param data Video data buffer
     * @param length Data length
     * @param video_type Video format type
     * @return int Result of sending, returns 0 on success, negative value on failure
     */
    int (*send)(uint8_t *data, uint32_t length, image_format_t video_type);

    /**
     * @brief Receive video data
     *
     * @param data Receive buffer
     * @param length Length of the buffer
     * @return int Result of receiving, returns the number of bytes received on success, negative value on failure
     */
    int (*recive)(uint8_t *data, uint32_t length);

    /**
     * @brief Pack video data
     *
     * Pack the video data according to the network transmission protocol, adding necessary header information
     *
     * @param data Video data
     * @param length Data length
     * @return int Packing result, returns the length of packed data on success, negative value on failure
     */
    int (*pack)(uint8_t *data, uint32_t length, uint8_t **pack_ptr, uint32_t *pack_ptr_length);

    /**
     * @brief Unpack video data
     *
     * Extract video data from the network packet and remove header information
     *
     * @param data Network packet
     * @param length Length of the packet
     * @return int Unpacking result, returns the length of unpacked data on success, negative value on failure
     */
    int (*unpack)(uint8_t *data, uint32_t length);
    /**
     * @brief Fragment data
     * @param data Fragment data
     * @param length Length of the fragment
     * @return bk_err_t Error code, BK_OK on success, others on failure
     */
    bk_err_t (*fragment)(uint8_t *data, uint32_t length);
    /**
     * @brief Unfragment data
     * @param data Fragment data
     * @param length Length of the fragment
     * @return bk_err_t Error code, BK_OK on success, others on failure
     */
    bk_err_t (*unfragment)(uint8_t *data, uint32_t length);
    /**
     * @brief Drop frame check
     *
     * Decide whether the current frame needs to be dropped based on network condition and buffer state,
     * used for adaptive video transmission strategy
     *
     * @param frame Frame buffer
     * @return bool true indicates frame drop required, false to keep the frame
     */
    bool (*drop_check)(frame_buffer_t *frame);

} ntwk_trans_video_chan_t;

/**
 * @brief Network transfer audio channel structure
 *
 * This structure defines the callback functions for audio channel operations in the network transfer module,
 * used to manage operations such as sending, receiving, packing, and unpacking of audio data.
 */
typedef struct {
    /**
     * @brief Channel type
     * @return chan_type_t Channel type
     */
    chan_type_t type;
    /**
     * @brief Audio encoding type
     * @return audio_enc_type_t Audio encoding type
     */
    audio_enc_type_t aud_type;

    /**
     * @brief Send audio data
     *
     * @param data Audio data buffer
     * @param length Data length
     * @param audio_type Audio encoding type
     * @return int Result of sending, returns 0 on success, negative value on failure
     */
    int (*send)(uint8_t *data, uint32_t length, audio_enc_type_t audio_type);

    /**
     * @brief Receive audio data
     *
     * @param data Receive buffer
     * @param length Length of the buffer
     * @return int Result of receiving, returns the number of bytes received on success, negative value on failure
     */
    int (*recive)(uint8_t *data, uint32_t length);

    /**
     * @brief Pack audio data
     *
     * Pack the audio data according to the network transmission protocol, adding necessary header information
     *
     * @param data Audio data
     * @param length Data length
     * @param pack_ptr Packed data pointer
     * @param pack_ptr_length Packed data length
     * @return int Packing result, returns the length of packed data on success, negative value on failure
     */
    int (*pack)(uint8_t *data, uint32_t length, uint8_t **pack_ptr, uint32_t *pack_ptr_length);

    /**
     * @brief Unpack audio data
     *
     * Extract the audio data from the network packet, removing header information
     *
     * @param data Network packet
     * @param length Length of the packet
     * @return int Unpacking result, returns the length of unpacked data on success, negative value on failure
     */
    int (*unpack)(uint8_t *data, uint32_t length);
        /**
     * @brief Fragment data
     * @param data Fragment data
     * @param length Length of the fragment
     * @return bk_err_t Error code, BK_OK on success, others on failure
     */
    bk_err_t (*fragment)(uint8_t *data, uint32_t length);
    /**
     * @brief Unfragment data
     * @param data Fragment data
     * @param length Length of the fragment
     * @return bk_err_t Error code, BK_OK on success, others on failure
     */
    bk_err_t (*unfragment)(uint8_t *data, uint32_t length);
} ntwk_trans_audio_chan_t;

/**
 * @brief Network Transfer Unfragmentation malloc callback type
 * @param size Size of the data
 * @param data_ptr Data pointer
 * @return bk_err_t Error code, BK_OK on success, others on failure
 */
typedef frame_buffer_t *(*ntwk_trans_unfragment_malloc_cb_t)(uint32_t size);
/**
 * @brief Network Transfer Unfragmentation send callback type
 * @param data Data buffer
 * @param data_len Length of the data
 * @return bk_err_t Error code, BK_OK on success, others on failure
 */
typedef bk_err_t (*ntwk_trans_unfragment_send_cb_t)(frame_buffer_t *data);
/**
 * @brief Network Transfer Unfragmentation free callback type
 * @param data Data pointer
 * @return bk_err_t Error code, BK_OK on success, others on failure
 */
typedef bk_err_t (*ntwk_trans_unfragment_free_cb_t)(frame_buffer_t *data);

/**
 * @brief Network transfer context structure
 *
 * This structure defines the complete context information for the network transfer module, including service type,
 * the configurations of each channel, and event handling as the core components. It is the main configuration structure for the network transfer module,
 * used to centrally manage and coordinate the control, video, and audio channels.
 */
typedef struct {
    /**
     * @brief Service name (replaces service_type)
     *
     * Specifies the network transfer service name, such as BK_TCP_SERVICE_NAME,
     * BK_UDP_SERVICE_NAME, BK_CS2_SERVICE_NAME or user-defined service name.
     * Maximum length: 99 characters. Customer can fill in custom service name.
     */
    char service_name[NTWK_TRANS_SERVICE_NAME_LEN];

    /**
     * @brief Control channel pointer
     * Points to the control channel configuration structure, used to manage the transmission of control commands and signal data
     */
    ntwk_trans_ctrl_chan_t *cntrl_chan;
    /**
     * @brief Video channel pointer
     * Points to the video channel configuration structure, used to manage video data transmission, frame queue, and frame drop strategy
     */
    ntwk_trans_video_chan_t *video_chan;
    /**
     * @brief Audio channel pointer
     * Points to the audio channel configuration structure, used to manage audio data transmission and codec
     */
    ntwk_trans_audio_chan_t *audio_chan;

    /**
     * @brief Initialization flag
     * Indicates whether the network transfer module has been initialized
     * true means initialized, false means not initialized
     */
    bool initialized;
} ntwk_trans_ctxt_t;

/**
 * @brief init network transfer context
 *
 * init network transfer context. This interface will automatically adapt to all supported service types,
 * so there is no need to modify the wifi_transfer component code for use.
 *
 * @param ctxt Pointer to the network transfer context, cannot be NULL
 * @return bk_err_t Registration result, returns BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_ctxt_init(ntwk_trans_ctxt_t *ctxt);

/**
 * @brief deinit network transfer context
 *
 * Unregister network transfer context, clean up relevant resources and reset global context
 *
 * @return bk_err_t Unregister result, returns BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_ctxt_deinit(void);

/**
 * @brief Get the currently registered network transfer context
 *
 * Get the pointer to the current registered network transfer context
 *
 * @return ntwk_trans_ctxt_t* Pointer to the current context, returns NULL if not registered
 */
ntwk_trans_ctxt_t *ntwk_trans_get_ctxt(void);

/**
 * @brief Get the service name from the currently registered network transfer context
 *
 * Get the service name string from the current registered network transfer context
 *
 * @return const char* Pointer to the service name string, returns NULL if context is not initialized
 */
const char *ntwk_trans_get_service_name(void);

/**
 * @brief Start the service of the specified channel
 *
 * Start the service of the specified channel for the currently registered context and automatically adapt the service type
 *
 * @param chan_type Channel type
 * @param param Channel parameter, can be NULL
 * @return bk_err_t Result of starting, returns BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_chan_start(chan_type_t chan_type, void *param);

/**
 * @brief Stop the service of the specified channel
 *
 * Stop the service of the specified channel for the currently registered context and automatically adapt the service type
 *
 * @param chan_type Channel type
 * @return bk_err_t Result of stopping, returns BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_chan_stop(chan_type_t chan_type);

/**
 * @brief Send control data
 *
 * Send control data through the currently registered context, automatically adapts the service type
 *
 * @param data Control data
 * @param length Data length
 * @return int Result of sending, returns the number of bytes sent on success, negative value on failure
 */
int ntwk_trans_ctrl_send(uint8_t *data, uint32_t length);

/**
 * @brief Send video data
 *
 * Send video data through the currently registered context, automatically adapts the service type
 *
 * @param data Video data
 * @param length Data length
 * @param video_type Video format type
 * @return int Result of sending, returns the number of bytes sent on success, negative value on failure
 */
int ntwk_trans_video_send(uint8_t *data, uint32_t length, image_format_t video_type);

/**
 * @brief Send audio data
 *
 * Send audio data through the currently registered context, automatically adapts the service type
 *
 * @param data Audio data
 * @param length Data length
 * @param audio_type Audio encoding type
 * @return int Result of sending, returns the number of bytes sent on success, negative value on failure
 */
int ntwk_trans_audio_send(uint8_t *data, uint32_t length, audio_enc_type_t audio_type);

/**
 * @brief Handle control data received
 * @param data Control data
 * @param length Data length
 * @return int Result of receiving, returns the number of bytes received on success, negative value on failure
 */
int ntwk_trans_ctrl_recv_handler(uint8_t *data, uint32_t length);

/**
 * @brief Handle video data received
 * @param data Video data
 * @param length Data length
 * @return int Result of receiving, returns the number of bytes received on success, negative value on failure
 */
int ntwk_trans_video_recv_handler(uint8_t *data, uint32_t length);

/**
 * @brief Handle audio data received
 * @param data Audio data
 * @param length Data length
 * @return int Result of receiving, returns the number of bytes received on success, negative value on failure
 */
int ntwk_trans_audio_recv_handler(uint8_t *data, uint32_t length);

/**
 * @brief Handle packet data received
 * @param chan_type Channel type
 * @param data Packet data
 * @param length Data length
 * @return int Result of receiving, returns the number of bytes received on success, negative value on failure
 */
int ntwk_trans_pack_rx_handler(chan_type_t chan_type, uint8_t *data, uint32_t length);

/**
 * @brief Handle fragment data received
 * @param chan Channel type (CTRL/VIDEO/AUDIO)
 * @param data Data buffer
 * @param length Data length
 * @return bk_err_t Error code, BK_OK on success, others on failure
 */
int ntwk_trans_fragment_rx_handler(chan_type_t chan, uint8_t *data, uint32_t length);

/**
 * @brief Register network transfer message event callback
 *
 * Register a callback function to handle network transfer events.
 * The callback will be invoked when events such as channel start, connection
 * established, disconnected, or stop occur on any channel (CTRL/VIDEO/AUDIO).
 *
 * @param cb Event callback function pointer (signature: void (*)(ntwk_trans_event_t *event))
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_register_msg_event_cb(ntwk_trans_msg_event_cb_t cb);

/**
 * @brief Register control channel receive callback
 *
 * Register a callback function to handle received data on the control channel.
 * The callback will be invoked when data is received on the control channel.
 *
 * @param cb Receive callback function pointer (signature: int (*)(uint8_t *data, uint32_t length))
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_register_ctrl_recv_cb(ntwk_trans_recv_cb_t cb);

/**
 * @brief Register video channel receive callback
 *
 * Register a callback function to handle received data on the video channel.
 * The callback will be invoked when video data is received.
 *
 * @param cb Receive callback function pointer (signature: int (*)(uint8_t *data, uint32_t length))
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_register_video_recv_cb(ntwk_trans_recv_cb_t cb);

/**
 * @brief Register audio channel receive callback
 *
 * Register a callback function to handle received data on the audio channel.
 * The callback will be invoked when audio data is received.
 *
 * @param cb Receive callback function pointer (signature: int (*)(uint8_t *data, uint32_t length))
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_register_audio_recv_cb(ntwk_trans_recv_cb_t cb);

/**
 * @brief Register unfragmented data memory allocation callback
 *
 * Register a callback function to allocate memory for unfragmented data on the specified channel.
 * This callback is used when the system needs to allocate a buffer for assembling fragmented packets.
 *
 * @param chan Channel type (CHAN_CTRL/CHAN_VIDEO/CHAN_AUDIO)
 * @param cb Malloc callback function pointer (signature: frame_buffer_t *(*)(uint32_t size))
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_register_unfragment_malloc_cb(chan_type_t chan, ntwk_trans_unfragment_malloc_cb_t cb);

/**
 * @brief Register unfragmented data send callback
 *
 * Register a callback function to send unfragmented (assembled) data on the specified channel.
 * This callback is invoked after fragmented packets have been reassembled into a complete frame.
 *
 * @param chan Channel type (CHAN_CTRL/CHAN_VIDEO/CHAN_AUDIO)
 * @param cb Send callback function pointer (signature: bk_err_t (*)(frame_buffer_t *data))
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_register_unfragment_send_cb(chan_type_t chan, ntwk_trans_unfragment_send_cb_t cb);

/**
 * @brief Register unfragmented data memory free callback
 *
 * Register a callback function to free memory allocated for unfragmented data on the specified channel.
 * This callback is used to release buffers after the unfragmented data has been processed.
 *
 * @param chan Channel type (CHAN_CTRL/CHAN_VIDEO/CHAN_AUDIO)
 * @param cb Free callback function pointer (signature: bk_err_t (*)(frame_buffer_t *data))
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_register_unfragment_free_cb(chan_type_t chan, ntwk_trans_unfragment_free_cb_t cb);

#if CONFIG_NTWK_CLIENT_SERVICE_ENABLE
/**
 * @brief Set server network information
 *
 * Configure the server network information including IP address and ports for control, video, and audio channels.
 *
 * @param net_info Pointer to the server network information structure
 * @return bk_err_t BK_OK on success, error code on failure
 */
bk_err_t ntwk_trans_set_server_net_info(ntwk_server_net_info_t *net_info);

/**
 * @brief Get server network information
 *
 * Retrieve the currently configured server network information including IP address and ports for control, video, and audio channels.
 *
 * @return ntwk_server_net_info_t* Pointer to the server network information structure, NULL if not configured
 */
ntwk_server_net_info_t *ntwk_trans_get_server_net_info(void);
#endif

#ifdef __cplusplus
}
#endif