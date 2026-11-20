// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <components/media_types.h>
#include <components/usb_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of UVC ports
 * 
 * This macro defines the maximum number of UVC ports supported,
 * based on the USB host hub maximum external ports configuration.
 */
#ifdef CONFIG_USBHOST_HUB_MAX_EHPORTS
#define UVC_PORT_MAX                      (CONFIG_USBHOST_HUB_MAX_EHPORTS)
#else
#define UVC_PORT_MAX                      (1)
#endif

/**
 * @brief UVC Error Codes
 * 
 * Defines the different types of UVC error codes that can occur during device operation.
 */
typedef enum {
    BK_UVC_CONNECT = 0,        /**< UVC device connected successfully */
    BK_UVC_CONFIG_ERROR,       /**< UVC configuration error occurred */
    BK_UVC_NO_MEMORY,          /**< Insufficient memory for UVC operations */
    BK_UVC_NO_RESOURCE,        /**< No available resources for UVC */
    BK_UVC_PPI_ERROR,          /**< PPI (Parallel Peripheral Interface) error */
    BK_UVC_DISCONNECT,         /**< UVC device disconnected */
    BK_UVC_NO_RESPON,          /**< UVC device not responding */
    BK_UVC_DMA_ERROR,          /**< DMA (Direct Memory Access) error */
    BK_UVC_NOT_PERMIT,         /**< Operation not permitted */
    BK_UVC_POWER_ERROR,        /**< Power supply error */
} uvc_error_code_t;

/**
 * @brief UVC Stream States
 * 
 * Represents the different operational states of a UVC data stream.
 */
typedef enum {
    UVC_STREAM_STATE_RUNNING = 0,    /**< Stream is running normally */
    UVC_STREAM_STATE_RESUME,         /**< Stream is resumed from a suspended state */
    UVC_STREAM_STATE_SUSPEND,        /**< Stream is temporarily suspended */
} uvc_stream_state_t;

/**
 * @brief UVC Stream Types
 * 
 * Defines the streaming modes for UVC data transmission.
 */
typedef enum
{
    UVC_SINGLE_STREAM = 0,    /**< Single stream mode - one data stream */
    UVC_DOUBLE_STREAM,        /**< Double stream mode - two simultaneous data streams */
} uvc_stream_type_t;

/**
 * @brief UVC Data Separation Information Structure
 * 
 * Used to track the valid data after separating H.264/JPEG from raw data stream.
 * This structure holds information about the location and length of valid data
 * within a UVC data packet.
 */
typedef struct
{
    uint32_t data_len;     /**< Length of the valid data in bytes */
    uint8_t *data_off;     /**< Pointer to the start of valid data within the packet */
} uvc_separate_info_t;

/**
 * @brief UVC Callback Functions Structure
 * 
 * Contains function pointers for various UVC events and operations.
 * These callbacks allow the application to respond to UVC events and
 * manage frame buffer allocation and processing.
 *
 * @attention In the callback functions, no blocking operations or long operations should be performed, otherwise the hardware interrupt will be delayed, leading to abnormal image data.
 */
typedef struct
{
    /**
     * @brief Frame buffer allocation callback
     * 
     * This function is called to allocate memory for a frame buffer.
     * 
     * @param format Image format of the frame (e.g., MJPEG, YUV)
     * @param size Size of memory to allocate in bytes
     * @return Pointer to allocated frame buffer, or NULL on failure
     */
    frame_buffer_t *(*malloc)(image_format_t format, uint32_t size);

    /**
     * @brief Frame processing completion callback
     * 
     * This function is called when frame processing is complete.
     * 
     * @param port Camera port number
     * @param format Image format of the processed frame
     * @param frame Pointer to the frame buffer containing the processed data
     * @param result Result of the frame processing operation
     */
    void (*complete)(uint8_t port, image_format_t format, frame_buffer_t *frame, int result);

    /**
     * @brief UVC event callback
     * 
     * This function is called when UVC state changes occur.
     * 
     * @param port_info Pointer to USB hub port information
     * @param arg User-defined argument passed to the callback
     * @param code UVC error code indicating the type of event
     */
    void (*uvc_event_callback)(bk_usb_hub_port_info *port_info, void *arg, uvc_error_code_t code);
} bk_uvc_callback_t;

/**
 * @brief UVC Camera Configuration Structure
 * 
 * Contains all configuration parameters needed for UVC camera initialization
 * and operation. This structure is used when setting up a new UVC camera.
 */
typedef struct
{
    uvc_stream_type_t type;    /**< Stream type (single or double stream) */
    uint8_t port;              /**< Camera port number (1-based indexing) */
    uint8_t drop_num;          /**< Number of frames to drop when UVC stream starts */
    image_format_t img_format; /**< Image format (e.g., MJPEG, YUV, H264) */
    uint16_t width;            /**< Frame width in pixels */
    uint16_t height;           /**< Frame height in pixels */
    uint32_t fps;              /**< Frames per second (frame rate) */
    void *user_data;           /**< User-defined argument to pass to uvc_event_callback */
} bk_cam_uvc_config_t;

/**
 * @brief UVC Packet Separation Configuration Structure
 * 
 * Contains configuration for handling UVC packet separation and processing.
 * This structure defines callbacks for packet processing operations.
 */
typedef struct
{
    uint8_t id;              /**< Camera port ID */

    /**
     * @brief UVC packet separation callback
     * 
     * This function is called to separate UVC packets from the raw data stream.
     * 
     * @param data Pointer to the raw data
     * @param length Length of the raw data in bytes
     * @param sepatate_info Pointer to structure that will receive separation results
     */
    void (*uvc_separate_packet_cb)(uint8_t *data, uint32_t length, uvc_separate_info_t *sepatate_info);

    /**
     * @brief UVC packet initialization callback
     * 
     * This function is called to initialize UVC packet processing.
     * 
     * @param device Pointer to UVC camera configuration
     * @param init Initialization flag (1 for init, 0 for deinit)
     * @param cb Pointer to UVC callback functions
     * @return BK_ERR_NONE on success, error code on failure
     */
    bk_err_t (*uvc_init_packet_cb)(bk_cam_uvc_config_t *device, uint8_t init, const bk_uvc_callback_t *cb);

    /**
     * @brief UVC end-of-frame packet callback
     * 
     * This function is called when an end-of-frame packet is received.
     * 
     * @param device Pointer to UVC camera configuration
     */
    void (*uvc_eof_packet_cb)(bk_cam_uvc_config_t *device);
} uvc_separate_config_t;

/**
 * @brief Default UVC Configuration for 864x480 Resolution at 30 FPS with MJPEG Format
 * 
 * Provides a standard configuration for UVC camera initialization with:
 * - Resolution: 864x480 pixels
 * - Frame rate: 30 FPS
 * - Image format: MJPEG
 * - Single stream mode
 * - Port 1
 * - No frame dropping
 * - No user argument
 */
#define BK_UVC_864X480_30FPS_MJPEG_CONFIG()	\
{\
    .type = UVC_SINGLE_STREAM, \
    .port = 1, \
    .drop_num = 0, \
    .img_format = IMAGE_MJPEG, \
    .width = 864, \
    .height = 480, \
    .fps = 30, \
    .user_data = NULL, \
}

#define BK_UVC_1920X1080_30FPS_H26X_CONFIG()	\
    {\
        .type = UVC_SINGLE_STREAM, \
        .port = 1, \
        .drop_num = 0, \
        .img_format = IMAGE_H264, \
        .width = 1920, \
        .height = 1080, \
        .fps = 15, \
        .user_data = NULL, \
    }

#ifdef __cplusplus
}
#endif
