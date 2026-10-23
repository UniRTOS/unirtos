/*****************************************************************/ /**
* @file qosa_camera_ai.h
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

#ifndef QOSA_CAMERA_AI_H
#define QOSA_CAMERA_AI_H

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"
#include "qosa_camera_water_meter_ai.h"

/*===========================================================================
 * Macro Definition
 ===========================================================================*/

/*===========================================================================
 * Enum
===========================================================================*/
/**
 * @brief Error codes for the QOSA camera AI module.
 *
 * This enumeration defines the possible return status codes from the QOSA camera AI functions.
 * A value of QOSA_CAMERA_AI_SUCCESS indicates successful operation, while other values indicate
 * specific error conditions that may occur during initialization, configuration, or recognition.
 */
typedef enum
{
    QOSA_CAMERA_AI_SUCCESS = 0,            ///< Operation completed successfully.
    QOSA_CAMERA_WATER_METER_AI_INIT_ERR,               ///< Failed to initialize the camera AI module.
    QOSA_CAMERA_AI_INVALID_ERR,            ///< Invalid argument or parameter provided.
    QOSA_CAMERA_AI_REPEAT_INIT_ERR,        ///< The module has already been initialized (redundant initialization attempt).
    QOSA_CAMERA_AI_RECOGNITION_ERR,        ///< An error occurred during the AI recognition process (e.g., detection or classification failed).
} qosa_camera_ai_errcode_e;

/**
 * @brief Represents a bounding box with associated detection result.
 *
 * This structure describes an axis-aligned bounding box (e.g., around a detected digit),
 * along with the predicted class (value) and its confidence score.
 *
 * The bounding box is defined by two corner coordinates:
 *   - (x1, y1): top-left corner
 *   - (x2, y2): bottom-right corner
 *
 * It is typically used in AI-based visual recognition tasks such as digit detection,
 * where the "digit" appears inside the box.
 */
typedef struct qosa_bbox {
    float x1;          ///< X-coordinate of the top-left corner of the bounding box.
    float y1;          ///< Y-coordinate of the top-left corner of the bounding box.
    float x2;          ///< X-coordinate of the bottom-right corner of the bounding box.
    float y2;          ///< Y-coordinate of the bottom-right corner of the bounding box.
    float score;     ///< Confidence score (0.0 to 1.0) indicating the model's certainty that the content inside the box is valid.
    int cls;         ///< Predicted class label (e.g., the recognized digit: 0–9, or other defined categories).
} qosa_bbox;
/*===========================================================================
 * Variate
 ===========================================================================*/

/*===========================================================================
 * Functions
 ===========================================================================*/

 /**
 * @brief Initialize detection
 *
 * @param num_size Valid length of input water meter result, used to exclude abnormal cases
 * @return int 0 indicates success, other values indicate failure
 */
qosa_camera_ai_errcode_e qosa_camera_water_meter_ai_init(qosa_uint32_t num_size);


/**
 * @brief Perform detection inference
 *
 * @param input_buf Pointer to input image data
 * @param image_height Input image height
 * @param image_width Input image width
 * @param out_buf Pointer to output detection results
 * @param size Pointer to output detection result count
 * @return int 0 indicates success, other values indicate failure
 */
qosa_camera_ai_errcode_e qosa_camera_water_meter_ai_inference(qosa_uint8_t* input_buf, qosa_uint32_t image_height, qosa_uint32_t image_width, qosa_bbox *out_buf, qosa_uint32_t* size);


/**
 * @brief Get SDK version number
 *
 * @return const char* Pointer to version number string
 */
qosa_camera_ai_errcode_e qosa_camera_water_meter_ai_get_version(qosa_uint8_t** version_info);

/**
 * @brief Close the water meter AI recognition module
 *
 * @param[in] num_size Number of digits in the water meter reading (e.g., 6 for a 6-digit meter)
 * @return qosa_camera_ai_errcode_e Error code indicating the result of the close operation
 */
qosa_camera_ai_errcode_e qosa_camera_water_meter_ai_close(void);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* QOSA_CAMERA_AI_H */
