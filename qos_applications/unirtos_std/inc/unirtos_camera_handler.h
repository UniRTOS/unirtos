/*****************************************************************/ /**
* @camera unirtos_camera_handler.h
* @brief
* @author bronson.zhan@quectel.com
* @date 2025-07-30
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-07-30 <td>1.0 <td>Bronson.Zhan <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_CAMERA_HANDLER_H__
#define __UNIRTOS_CAMERA_HANDLER_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_camera.h"
#include "qosa_at_cmd.h"
#include "qosa_watermark.h"
#include "qosa_buffer_block.h"
#include "unirtos_atcmd_cfg.h"

#define UNIR_CAMERA_TASK_PRIO        QOSA_PRIORITY_NORMAL
#define UNIR_CAMERA_TASK_EVENT_MAX   30

#define UNIR_CAMERA_CACHE_BUFFER_MAX 2

/**
 * @enum qcamera_cmd_e_type
 * @brief  Mainly used for camera handler task command types
 */
typedef enum
{
    QCAMERA_MIN_CMD = -1,

    QCAMERA_OPEN_REQ,                 /*!< Open camera request camera_task_open_req_t */
    QCAMERA_GET_OPEN_STATUS_REQ,      /*!< Get camera open status request camera_task_param_common_req_t */

    QCAMERA_CLOSE_REQ,                /*!< Close camera request camera_task_close_req_t */

    QCAMERA_DECODE_REQ,               /*!< Scan code request camera_task_decode_req_t */
    QCAMERA_GET_DECODE_PARAM_REQ,     /*!< Get scan code parameters request camera_task_param_common_req_t */

    QCAMERA_CAPTURE_REQ,              /*!< Capture photo request camera_task_capture_req_t */
    QCAMERA_GET_CAPTURE_PARAM_REQ,    /*!< Get capture photo parameters request camera_task_param_common_req_t */

    QCAMERA_READ_CACHE_REQ,           /*!< Cache photo readout request camera_task_read_cache_req_t */
    QCAMERA_GET_READ_CACHE_PARAM_REQ, /*!< Get cached photo parameters request camera_task_param_common_req_t */

    QCAMERA_WATER_METER_AI_INERENCE_REQ,       /*!< AI recognition request ai_recognition_request_t */

    QCAMERA_MAX_CMD
} qcamera_cmd_e_type;

/************************************************************************/
/*  Message body for camera task to send to AT task                                     */
/************************************************************************/
/**
 * @struct camera_task_common_cnf_t
 * @brief  Standard request result notification for sending related requests to the AT task
 */
typedef struct
{
    qosa_at_dev_type_e dev_port;       /*!< The device channel number corresponding to the AT command */

    qosa_camera_error_e camera_result; /*!< Request execution result */
} camera_task_common_cnf_t;

/**
 * @struct camera_task_read_cache_cnf_t
 * @brief  Used to send the result of a read cache parameters request to the AT task
 */
typedef struct
{
    qosa_at_dev_type_e dev_port;       /*!< The device channel number corresponding to the AT command */

    qosa_camera_error_e camera_result; /*!< Request execution result */

    qosa_bool_t is_open;               /*!< Camera open status */
} camera_task_open_status_cnf_t;

/**
 * @struct camera_task_decode_ind_t
 * @brief  Used to send the decoding result report to the camera task
 */
typedef struct
{
    /*
        <result>	Integer. Decoding result.
                    0		Success
                    1		No decoding library
                    Others	Failure
        <type>      Integer. Type of the parsed barcode or QR code. Different decoding libraries output different type codes.
                    0		CODE39 (Barcode)
                    1		CODE128 (Barcode)
                    2		CODE_QR (QR Code)
                    0xff		Represents unknown type
        <length>    Integer. Length of the decoded data. Unit: bytes.
        <out_data>  Decoded data. The output result depends on the decoding library.
    */
    qosa_at_dev_type_e dev_port; /*!< The device channel number corresponding to the AT command */

    qosa_uint8_t  decode_result; /*!< Decoded result */
    qosa_uint8_t  decode_type;   /*!< Type of parsed barcode or QR code */
    qosa_uint32_t decode_length; /*!< Length of the parsing result */
    qosa_uint8_t *decode_data;   /*!< Decoded data */
} camera_task_decode_ind_t;

/**
 * @struct camera_task_decode_param_cnf_t
 * @brief  Used to send the result notification of a request for obtaining decoding parameters to the camera task
 */
typedef struct
{
    /*
        <op>			Integer. Start or stop decoding.
					    0	Stop decoding
			            1	Start decoding
        <decodecnt>		Integer. Set the number of decoding cycles; decoding will stop once the set count is reached. Range: 0~255; Default: 0 (indicates continuous decoding).
    */
    qosa_at_dev_type_e dev_port;       /*!< The device channel number corresponding to the AT command */

    qosa_camera_error_e camera_result; /*!< Request execution result */

    qosa_uint8_t decode_option;        /*!< Decoding mode */
    qosa_uint8_t decode_cnt;           /*!< Decoding count setting */
} camera_task_decode_param_cnf_t;

/**
 * @struct camera_task_capture_param_cnf_t
 * @brief  Used to send the request result notification for camera capture to the AT task
 */
typedef struct
{
    qosa_at_dev_type_e dev_port;       /*!< The device channel number corresponding to the AT command */

    qosa_camera_error_e camera_result; /*!< Request execution result */

    qosa_bool_t  capture_switch;       /*!< TRUE: Enable capture FALSE: Disable capture */
    qosa_uint8_t capture_mode;         /*!< Capture mode */

    qosa_uint8_t fresh_rate;           /*!< Direct output mode output rate, valid when the capture mode is set to direct output mode */

    qosa_uint8_t cache_num;            /*!< Cache mode buffer count, this parameter is valid when the capture mode is set to cache mode */
    qosa_uint8_t zip_mode;             /*!< Data compression mode, this parameter is valid when the capture mode is set to buffer mode */
    qosa_uint8_t zip_ratio;            /*!< Data compression ratio, this parameter is valid when the capture mode is set to buffer mode */

    qosa_camera_crop_info_t crop_info; /*!< Image cropping range */
} camera_task_capture_param_cnf_t;

/**
 * @struct camera_task_read_cache_cnf_t
 * @brief  Used to send the result of a read cache parameters request to the AT task
 */
typedef struct
{
    qosa_at_dev_type_e dev_port;       /*!< The device channel number corresponding to the AT command */

    qosa_camera_error_e camera_result; /*!< Request execution result */

    qosa_uint8_t zip_mode;             /*!< Data compression mode, this parameter is valid when the capture mode is set to buffer mode */

    qosa_uint8_t *cache_data;          /*!< Read cache data */
    qosa_uint8_t *cache_ziped_data;    /*!< The read compressed cache data, needs to be actively released */
    qosa_uint32_t cache_data_length;   /*!< Length of the read cached data */
} camera_task_read_cache_cnf_t;

/**
 * @struct camera_task_read_cache_cnf_t
 * @brief  Used to send the result of a read cache parameters request to the AT task
 */
typedef struct
{
    qosa_at_dev_type_e dev_port;       /*!< The device channel number corresponding to the AT command */

    qosa_camera_error_e camera_result; /*!< Request execution result */

    qosa_uint8_t  avail_cache_index;   /*!< Obtainable index */
    qosa_uint32_t cache_data_length;   /*!< Length of cached data for the current index number */
} camera_task_read_cache_param_cnf_t;

/**
 * @enum camera_resp_cmd_e
 * @brief Message command words used by the camera task to send to the center task
 */
typedef enum
{
    CAMERA_RESP_OPEN_CNF,                 /*!< Camera open request result notification camera_task_common_cnf_t */
    CAMERA_RESP_GET_OPEN_STATUS_CNF,      /*!< Get camera open status notification camera_task_open_status_cnf_t */

    CAMERA_RESP_CLOSE_CNF,                /*!< Camera close request result notification camera_task_common_cnf_t */

    CAMERA_RESP_DECODE_CNF,               /*!< Scan code request result notification camera_task_common_cnf_t */
    CAMERA_RESP_DECODE_IND,               /*!< Scan code result reporting camera_task_decode_ind_t */
    CAMERA_RESP_GET_DECODE_PARAM_CNF,     /*!< Get scan code parameter result notification camera_task_decode_param_cnf_t */

    CAMERA_RESP_CAPTURE_CNF,              /*!< Capture photo request result notification camera_task_common_cnf_t */
    CAMERA_RESP_CAPTURE_IND,              /*!< Capture photo result report TODO */
    CAMERA_RESP_GET_CAPTURE_PARAM_CNF,    /*!< Get capture photo parameter result notification camera_task_capture_param_cnf_t */

    CAMERA_RESP_READ_CACHE_CNF,           /*!< Camera read cache completion notification camera_task_read_cache_cnf_t */
    CAMERA_RESP_GET_READ_CACHE_PARAM_CNF, /*!< Get cached photo parameter result notification camera_task_read_cache_param_cnf_t */

    CAMERA_RESP_MAX
} camera_resp_cmd_e;

/**
 * @brief Message body sent from the CAMERA task to the AT task, used for synchronizing file processing result information
 */
typedef struct
{
    camera_resp_cmd_e cmd_id;
    void             *argv;
} at_camera_event_cmd_info;

/************************************************************************/
/*  Used for AT thread to send message content to camera task               */
/************************************************************************/

/**
 * @struct camera_task_open_req_t
 * @brief Request to send to the camera task for opening the camera.
 */
typedef struct
{
    /*
        <height>		Integer. Image height. Range: 50~480; Unit: pixels. When set to 0, defaults to the Camera model's default image height.
        <width>			Integer. Image width. Range: 50~640; Unit: pixels. When set to 0, defaults to the Camera model's default image width.
        <format>		Integer. Image output format.
					    0   Only Y
			            1	YUV422
    */

    qosa_at_dev_type_e dev_port;          /*!< The device channel number corresponding to the AT command */

    qosa_camera_channel_e camera_channel; /*!< camera channel number */

    qosa_uint32_t camera_height;          /*!< Camera height that needs to be initialized */
    qosa_uint32_t camera_width;           /*!< Camera width to be initialized */

    qosa_uint8_t camera_output_format;    /*!< Camera output format */
} camera_task_open_req_t;

/**
 * @struct camera_task_close_req_t
 * @brief  Request to send a close camera command to the camera task.
 */
typedef struct
{
    qosa_at_dev_type_e dev_port;          /*!< The device channel number corresponding to the AT command */

    qosa_camera_channel_e camera_channel; /*!< camera channel number */
} camera_task_close_req_t;

/**
 * @struct camera_task_decode_req_t
 * @brief  Request for capturing an image sent to the camera task
 */
typedef struct
{
    /*
        <op>			Integer. Start or stop decoding.
					    0	Stop decoding
			            1	Start decoding
        <decodecnt>		Integer. Set the number of decoding cycles; decoding will stop once the set count is reached. Range: 0~255; Default: 0 (indicates continuous decoding).
    */
    qosa_at_dev_type_e dev_port; /*!< The device channel number corresponding to the AT command */

    qosa_uint8_t decode_option;  /*!< Decoding mode */
    qosa_uint8_t decode_cnt;     /*!< Decoding count setting */
} camera_task_decode_req_t;

/**
 * @enum camera_capture_mode_e
 * @brief Camera Capture Mode
 */
typedef enum
{
    CAMERA_CAPTURE_MODE_DIRECT, /*!< Direct spit mode */
    CAMERA_CAPTURE_MODE_CACHE,  /*!< Cache mode */
} camera_capture_mode_e;

/**
 * @struct camera_task_capture_req_t
 * @brief Request for capturing an image sent to the camera task
 */
typedef struct
{
    /*
        <type>        Integer. Start or stop capture.
                    0    Stop capture
                    1    Start capture
        <mode>        Integer. Capture mode.
                    0    Direct output mode (real-time capture and immediate output)
                    1    Cache mode (attempts capture after each command input, caches according to the module's memory capacity after capture, and the cached data is read via the AT+QCAMREAD command. Returns OK only after capture is complete.)
        <fresh_rate> Integer. Direct output mode output rate. Range: 5~255; Unit: seconds.
        <cache_num>    Integer. Cache mode buffer count.
                    0    Single buffer cache, can only cache one packet of data.
                    1    Double buffer cache, caches two packets of data per command.
        <zip_mode>    Integer. Data compression mode.
                    0    Raw data, no compression.
                    1    Compressed using LZMA algorithm.
        <zip_ratio>    Integer. Data compression quality, valid when compression is enabled. Range: 1~100; Default: 100.
        <start_x>    Integer. JPG crop X-axis start position, valid when compression is enabled. Range: 0~640; Default: 0.
        <end_x>        Integer. JPG crop X-axis end position, valid when compression is enabled. Range: 0~640; Default: 0.
        <start_y>    Integer. JPG crop Y-axis start position, valid when compression is enabled. Range: 0~480; Default: 0.
        <end_y>        Integer. JPG crop Y-axis end position, valid when compression is enabled. Range: 0~480; Default: 0.
    */
    qosa_at_dev_type_e dev_port;          /*!< The device channel number corresponding to the AT command */

    qosa_camera_channel_e camera_channel; /*!< camera channel number */

    qosa_bool_t  capture_switch;          /*!< TRUE: Enable capture FALSE: Disable capture */
    qosa_uint8_t capture_mode;            /*!< Capture mode */

    qosa_uint8_t fresh_rate;              /*!< Direct output mode output rate, valid when the capture mode is set to direct output mode */

    qosa_uint8_t cache_num;               /*!< Cache mode buffer count, this parameter is valid when the capture mode is set to cache mode */
    qosa_bool_t  zip_mode;                /*!< Data compression mode, this parameter is valid when the capture mode is set to buffer mode */
    qosa_uint8_t zip_ratio;               /*!< Data compression ratio, this parameter is valid when the capture mode is set to buffer mode */

    qosa_camera_crop_info_t crop_info;    /*!< Image cropping range */
} camera_task_capture_req_t;

/**
 * @struct camera_task_read_cache_req_t
 * @brief Request for reading cached images sent to the camera task
 */
typedef struct
{
    /*
        <cache_index>		Integer. Cache index, maximum of two caches can be stored, namely index0 and index1
    */
    qosa_at_dev_type_e dev_port;          /*!< The device channel number corresponding to the AT command */

    qosa_camera_channel_e camera_channel; /*!< camera channel number */

    qosa_uint8_t cache_index;             /*!< Buffer index to output */
} camera_task_read_cache_req_t;

/**
 * @struct camera_task_param_common_req_t
 * @brief Request for reading cached images sent to the camera task
 */
typedef struct
{
    qosa_at_dev_type_e dev_port;          /*!< The device channel number corresponding to the AT command */

    qosa_camera_channel_e camera_channel; /*!< camera channel number */
} camera_task_param_common_req_t;

// typedef qosa_uint32_t camera_task_empty_sig; /* Reserved */

/**
 * @struct at_to_camera_param_info_t
 * @brief Used to send command execution requests to the camera task
 */

typedef struct
{
    qcamera_cmd_e_type cmd;
    void              *argv;
} camera_task_msg_t;

/**
 * @brief Notifies the camera task of message events via AT
 *
 * @param[in] qcamera_cmd_e_type cmd
 *          - Corresponds to the specific command type
 *
 * @param[in] void * argv
 *          - User parameter to be passed
 *
 * @return int
 *       - Returns 0 on successful execution
 *       - Returns a negative number on execution failure
 */
qosa_uint32_t qosa_notify_camera_task(qcamera_cmd_e_type cmd, void *argv);


#ifdef CONFIG_QAPP_UNIRTOS_CAMERA_WATER_METER_AI_AT_FUNC

typedef struct
{
    qosa_at_dev_type_e dev_port;          /*!< The device channel number corresponding to the AT command */

    qosa_camera_channel_e camera_channel; /*!< camera channel number */

    qosa_uint8_t cache_index;             /*!< Buffer index to output */

    qosa_uint32_t bbox_num;               /*!< The number of digital boxes that need to be reasoned out */

    qosa_uint32_t img_rotation_direction;  /*!< Image rotation direction for AI inference*/

} camera_task_ai_recognition_req_t;

/**
 * @brief Handles AI recognition parameter processing in the camera task context
 *
 * This function is invoked to process AI-based recognition parameters passed via a message
 * or event (e.g., from an AT command or internal request). It typically parses the input
 * arguments, configures the AI engine, and initiates digit/object recognition on captured frames.
 *
 * @param[in] void * argv
 *          - Pointer to the recognition request structure (e.g., camera_task_ai_recognition_req_t)
 *            containing configuration such as image buffer, model type, or region of interest.
 *
 * @return void
 */
void qosa_camera_water_meter_ai_recognition_param_handler(void *argv);
/**
 * @brief Rotates water meter image data
 *
 * This function performs rotation operation on source image data and stores the result
 * in the destination buffer. Used to adjust the orientation of captured images before
 * further processing or transmission.
 *
 * @param[in] qosa_uint8_t *image_src_data
 *          - Pointer to the source image buffer containing raw pixel data.
 *
 * @param[out] qosa_uint8_t *image_rot_data
 *          - Pointer to the destination buffer where rotated image data will be stored.
 *
 * @param[in] int height
 *          - Height of the source image in pixels.
 *
 * @param[in] int width
 *          - Width of the source image in pixels.
 *
 * @param[in] int value
 *          - Rotation parameter, typically representing angle (e.g., 90, 180, 270) or direction flag.
 *
 * @return int
 *          - Status code indicating success (0) or failure (negative value).
 */
int qosa_camera_water_meter_picture_rotate( qosa_uint8_t *image_src_data, qosa_uint8_t *image_rot_data, int height, int width, int value);
#endif /*CONFIG_QAPP_UNIRTOS_CAMERA_WATER_METER_AI_AT_FUNC*/
#endif /* __UNIRTOS_CAMERA_HANDLER_H__ */
