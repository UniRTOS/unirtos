/*****************************************************************/ /**
* @file qosa_camera_water_meter_ai_license.h
* @brief License interface for camera water meter AI
* @author bronson.zhan@quectel.com
* @date 2026-06-02
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-06-02 <td>1.0 <td>GitHub Copilot <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_CAMERA_WATER_METER_AI_LICENSE_H__
#define __QOSA_CAMERA_WATER_METER_AI_LICENSE_H__

#include "qosa_camera_water_meter_ai.h"

/**
 * @brief Check whether camera water meter AI license has been activated.
 *
 * The function verifies the saved license content through the common license framework.
 *
 * @return qosa_camera_ai_errcode_e
 *         - QOSA_CAMERA_AI_SUCCESS: License is valid.
 *         - QOSA_CAMERA_AI_INVALID_ERR: License is unavailable or invalid.
 */
qosa_camera_ai_errcode_e qosa_camera_water_meter_ai_license_check(void);

/**
 * @brief Initialize camera water meter AI license registration.
 */
void qosa_camera_water_meter_ai_license_init(void);

/**
 * @brief Get camera water meter AI license APP ID.
 *
 * @return qosa_int32_t APP ID used by the license framework.
 */
qosa_int32_t qosa_camera_water_meter_ai_license_get_app_id(void);

#endif /* __QOSA_CAMERA_WATER_METER_AI_LICENSE_H__ */