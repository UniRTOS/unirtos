/*****************************************************************/ /**
* @file unirtos_atcmd_camera.h
* @brief
* @author bronson.zhan@quectel.com
* @date 2025-07-29
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2025-07-29 <td>1.0 <td>bronson.zhan <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_CAMERA_H__
#define __UNIRTOS_ATCMD_CAMERA_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"

/**
 * @enum UNIR_CAMERA_ERROR_CODES
 * @struct Camera AT Command Error Codes
 */
typedef enum
{
    UNIR_CAMERA_OK = 0,
    UNIR_CAMERA_UNKNOWN_ERROR = 7100, /*!< Unknown Error */
    UNIR_CAMERA_PARAM_ERROR = 7200,   /*!< Not Support Parameter */
    UNIR_CAMERA_INIT_ERROR = 7300,    /*!< Initial Error */
    UNIR_CAMERA_DECODE_ERROR = 7400,  /*!< Decode Error */
    UNIR_CAMERA_WATER_METER_AI_INERENCE_ERROR = 7500, /*!< AI recognition failed */
    UNIR_CAMERA_WATER_METER_AI_INIT_ERROR = 7600,          /*!< Failed to initialize the water meter AI module */
    UNIR_CAMERA_WATER_METER_AI_NOT_INIT_ERROR = 7700,      /*!< Water meter AI module has not been initialized */
    UNIR_CAMERA_WATER_METER_AI_NOT_PICTURE_ERROR = 7800,   /*!< No valid input image provided for recognition */
    UNIR_CAMERA_WATER_METER_AI_PICTURE_ROTATE_ERROR = 7900,/*!< Input image rotation is invalid or unsupported */
    UNIR_CAMERA_WATER_METER_AI_LICENSE_ERROR = 8000,/*!< License error */
    UNIR_CAMERA_ERROR_MAX,
} UNIR_CAMERA_ERROR_CODES;

/**
 * @enum at_process_camera_req_cmd_e
 * @brief Message command words used by the AT task to send to the center task
 */
typedef enum
{
    CAMERA_EVENT_QCAMREAD_SEND, /*!< QCAMREAD data transmission processing */
                                /*!< QCAMREAD data sending processing */

    CAMERA_EVENT_MAX,
} at_process_camera_req_cmd_e;

/**
 * @brief AT task sends message body to itself for processing data sending tasks
 */
typedef struct
{
    at_process_camera_req_cmd_e cmd_id;
    void                       *argv;
} at_process_camera_event_cmd_info_t;

/**
 * @struct QFDWL_PARAM
 * @brief Used by QFDWL+QFREAD functions for file downloading
 */
typedef struct
{
    qosa_at_dev_type_e dev_port;             /*!< Corresponding AT channel */
                                             /*!< Corresponding AT channel */

    qosa_uint8_t *data_address;              /*!< Data source pointer */
                                             /*!< Data source pointer */
    qosa_uint32_t size_downloaded;           /*!< Downloaded length */
                                             /*!< Already downloaded length */
    qosa_uint32_t size_read;                 /*!< Expected length to read */
                                             /*!< Expected read length */
    qosa_bool_t data_need_free;              /*!< Whether the data source needs to be released */
                                             /*!< Whether data source needs to be freed */

    qosa_uint16_t checksum;                  /*!< Calculate the CRC value of the file */
                                             /*!< Calculate file CRC value */
    qosa_uint16_t odd_byte;                  /*!< Calculate the CRC value of the file */
                                             /*!< Calculate file CRC value */
    qosa_uint8_t odd_flag;                   /*!< Calculate the CRC value of the file */
                                             /*!< Calculate file CRC value */
    UNIR_CAMERA_ERROR_CODES last_error_code; /*!< The final error code */
                                             /*!< Last error code when finished */

    qosa_bool_t flow_flag;                   /*!< Flow control management */
                                             /*!< Flow control management */
    qosa_bool_t force_exit;                  /*!< Force exit, user actively executes +++ or dtr */
                                             /*!< Force exit, user actively executes +++ or DTR */
    qosa_bool_t non_empty_report;            /*!< Whether to report empty notification */
                                             /*!< Whether to report empty notification */

    qosa_buffer_watermark_t tx_wm_ptr;       /*!< watermark TX pointer */
                                             /*!< watermark TX pointer */
    qosa_buffer_watermark_t rx_wm_ptr;       /*!< watermark RX pointer */
                                             /*!< watermark RX pointer */
    qosa_q_type_t q_ptr_tx;                  /*!< watermark attached queue */
                                             /*!< watermark associated queue */
    qosa_q_type_t q_ptr_rx;                  /*!< watermark attached queue */
                                             /*!< watermark associated queue */

    qosa_timer_t wait_timer;                 /*!< Prevent deadlock due to inconsistent state caused by critical conditions */
                                             /*!< Prevent deadlock due to critical state causing unsynchronized state */
    qosa_bool_t read_finish;                 /*!< Whether the data has been read completely */
                                             /*!< Whether data reading has been completed */
} camera_hd_output_param_t;

void qstd_exec_camera_qcamopen_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_camera_qcamidfy_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_camera_qcamcap_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_camera_qcamread_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_camera_qcampre_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_camera_qcamclose_cmd(qosa_at_cmd_t *cmd);

#ifdef CONFIG_QAPP_UNIRTOS_CAMERA_WATER_METER_AI_AT_FUNC
void qstd_exec_camera_qcamwmaiopen_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_camera_qcamwmaiinfer_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_camera_qcamwmaiver_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_camera_qcamwmaiclose_cmd(qosa_at_cmd_t *cmd);
#endif /*CONFIG_QAPP_UNIRTOS_CAMERA_WATER_METER_AI_AT_FUNC*/
#endif /* __UNIRTOS_ATCMD_CAMERA_H__ */
