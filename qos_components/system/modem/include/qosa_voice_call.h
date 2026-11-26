/*****************************************************************/ /**
* @file qosa_voice_call.h
* @brief VOICE CALL Function
* @author Liaz.liao@quectel.com
* @date 2026-03-05
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2026-03-05 <td>1.0 <td>Liaz.liao <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_VOICE_CALL__H__
#define __QOSA_VOICE_CALL__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"
#include "qosa_ims.h"

/**
 * @brief Starts a voice call asynchronously.
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] phone_number
 *          - The phone number to dial (null-terminated string). Must not be NULL.
 * @param[in] cb
 *          - Callback function invoked when call result is available.
 * @param[in] ctx
 *          - User context passed to the callback.
 * @return
 *         - 0:  Request submitted successfully (callback will be called)
 *         - -1: phone_number is NULL (callback will NOT be called)
 */
int qosa_start_voice_call(qosa_uint8_t simid, const char *phone_number, ims_callback_t cb, void *ctx);

/**
 * @brief Stops the current voice call on the specified SIM asynchronously.
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] cb
 *          - Callback function invoked when call result is available.
 * @param[in] ctx
 *          - User context passed to the callback.
 * @return
 *         - 0:  Request submitted successfully (callback will be called)
 *         - -1: simid is invalid (callback will NOT be called)
 */
int qosa_stop_voice_call(qosa_uint8_t simid, ims_callback_t cb, void *ctx);

/**
 * @brief Answers an incoming voice call asynchronously.
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] cb
 *          - Callback function invoked when call result is available.
 * @param[in] ctx
 *          - User context passed to the callback.
 * @return
 *         - 0:  Request submitted successfully (callback will be called)
 *         - -1: simid is invalid (callback will NOT be called)
 */
int qosa_answer_call(qosa_uint8_t simid, ims_callback_t cb, void *ctx);

#endif /* Wi-Fi platform exclusion */
#endif /* __QOSA_VOICE_CALL__H__ */
