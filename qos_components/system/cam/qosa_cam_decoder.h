/*****************************************************************/ /**
* @file qosa_cam_decoder.h
* @brief code decoder for camera
* @author bronson.zhan@quectel.com
* @date 2025-12-26
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-12-26 <td>1.0 <td>Bronson.Zhan <td> Init
* </table>
**********************************************************************/

#ifndef QOSA_CAM_DECODER_H
#define QOSA_CAM_DECODER_H

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"
#include "qosa_camera.h"

/*===========================================================================
 * Macro Definition
 ===========================================================================*/
#define QOSA_DECODER_ERRCODE_BASE (QOSA_COMPONENT_CODE_DECODER << 16)

/*===========================================================================
 * Enum
===========================================================================*/
typedef enum
{
    QOSA_DECODER_SUCCESS = 0,

    QOSA_DECODER_INIT_ERR = 1 | QOSA_DECODER_ERRCODE_BASE,
    QOSA_DECODER_REPEAT_INIT_ERR,
    QOSA_DECODER_COPYRIGHT_ERR,
    QOSA_DECODER_SETMIRROR_ERR,

    QOSA_DECODER_ERR,
    QOSA_DECODER_MEM_ERR,
    QOSA_DECODER_PREVIEW_ERR,
    QOSA_DECODER_GET_RESULT_ERR,
    QOSA_DECODER_GET_RESULT_LENGTH_ERR,

    QOSA_DECODER_DESTROY_ERR,
} qosa_decoder_errcode_e;

typedef enum
{
    QOSA_DECODER_TYPE_NONE = 0,      /*!< no symbol decoded */
    QOSA_DECODER_TYPE_PARTIAL = 1,   /*!< intermediate status */
    QOSA_DECODER_TYPE_EAN8 = 8,      /*!< EAN-8 */
    QOSA_DECODER_TYPE_UPCE = 9,      /*!< UPC-E */
    QOSA_DECODER_TYPE_ISBN10 = 10,   /*!< ISBN-10 (from EAN-13). @since 0.4 */
    QOSA_DECODER_TYPE_UPCA = 12,     /*!< UPC-A */
    QOSA_DECODER_TYPE_EAN13 = 13,    /*!< EAN-13 */
    QOSA_DECODER_TYPE_ISBN13 = 14,   /*!< ISBN-13 (from EAN-13). @since 0.4 */
    QOSA_DECODER_TYPE_I25 = 25,      /*!< Interleaved 2 of 5. @since 0.4 */
    QOSA_DECODER_TYPE_CODE39 = 39,   /*!< Code 39. @since 0.4 */
    QOSA_DECODER_TYPE_PDF417 = 57,   /*!< PDF417. @since 0.6 */
    QOSA_DECODER_TYPE_QRCODE = 64,   /*!< QR Code. @since 0.10 */
    QOSA_DECODER_TYPE_CODE128 = 128, /*!< Code 128 */

    QOSA_DECODER_TYPE_MAX = 0xff,
} qosa_decoder_type_e;

typedef enum
{
    QOSA_DECODE_NORMAL_MODE = 0, /*!< Normal code parsing */
    QOSA_DECODE_MIRROR_MODE,     /*!< Mirror code parsing */
    QOSA_DECODE_AUTO_MODE,       /*!< Works for both normal parsing and mirror parsing */

    QOSA_DECODE_MODE_MAX,
} qosa_decode_mode_e;

/*===========================================================================
 * Variate
 ===========================================================================*/

/*===========================================================================
 * Functions
 ===========================================================================*/

/**
 * @brief Initialize the code decoder
 *
 * @return qosa_decoder_errcode_e
 *          - QOSA_DECODER_SUCCESS: Success
 *          - other: Error code
 */
qosa_decoder_errcode_e qosa_qr_decoder_init(void);

/**
 * @brief Get the decode result
 *
 * @param[out] qosa_decoder_type_e* type
 *          - the type of the code
 * 
 * @param[out] qosa_uint8_t *result
 *          - the result of the code
 *
 * @return qosa_decoder_errcode_e
 *          - QOSA_DECODER_SUCCESS: Success
 *          - other: Error code
 */
qosa_decoder_errcode_e qosa_qr_get_decoder_result(qosa_decoder_type_e *type, qosa_uint8_t *result);

/**
 * @brief Decode the image
 *
 * @param[in] qosa_camera_channel_e camera_no
 *          - if you don't input img_buffer, need gives camera number to preview image in the function
 * 
 * @param[in] qosa_uint8_t *img_buffer
 *          - the image need to be decoded
 * 
 * @param[in] qosa_uint32_t width
 *          - the width of the image
 * 
 * @param[in] qosa_uint32_t height
 *          - the height of the image
 * 
 * @return qosa_decoder_errcode_e
 *          - QOSA_DECODER_SUCCESS: Success
 *          - other: Error code
 */
qosa_decoder_errcode_e qosa_qr_image_decoder(qosa_camera_channel_e camera_no, qosa_uint8_t *img_buffer, qosa_uint32_t width, qosa_uint32_t height);

/**
 * @brief Destroy the deocder
 *
 * @return qosa_decoder_errcode_e
 *          - QOSA_DECODER_SUCCESS: Success
 *          - other: Error code
 */
qosa_decoder_errcode_e qosa_destroy_decoder(void);

/**
 * @brief Get version of the deocder
 *
 * @param[out] qosa_uint8_t *version
 *          - the data of the version
 *
 * @return qosa_decoder_errcode_e
 *          - QOSA_DECODER_SUCCESS: Success
 *          - other: Error code
 */
qosa_decoder_errcode_e qosa_get_decoder_version(qosa_uint8_t *version);

/**
 * @brief Set decode mode
 *
 * @param[in] qosa_decode_mode_e decode_mode
 *          - decode mode
 *
 * @return qosa_decoder_errcode_e
 *          - QOSA_DECODER_SUCCESS: Success
 *          - other: Error code
 */
qosa_decoder_errcode_e qosa_set_decode_mirror(qosa_decode_mode_e decode_mode);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* QOSA_CAM_DECODER_H */
