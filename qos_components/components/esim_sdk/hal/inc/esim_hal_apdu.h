/*****************************************************************/ /**
* @file esim_hal_apdu.h
* @brief
* @author neo.jiang@quectel.com
* @date 2024-10-21
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-10-21 <td>1.0 <td>neo.jiang <td> Init
* </table>
**********************************************************************/
#ifndef __ESIM_HAL_APDU_H__
#define __ESIM_HAL_APDU_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "esim_error.h"
#include "qosa_sim.h"

typedef struct
{
    qosa_uint8_t *apdu_data_ptr; /*!< Internal storage APDU data address pointer*/
    qosa_uint32_t apdu_data_len; /*!< APDU data length*/
    qosa_uint32_t max_len;       /*!< Maximum length of APDU data space*/
} esim_apdu_data_t;

#if defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC) || \
    defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
/*!< Using the size of qosa_sim_generic_access_cnf_t directly on 160x to create a msg queue will result in failure.*/
typedef struct
{
    int flag; /* reserved */
    qosa_sim_generic_access_cnf_t *apdu_info;
} esim_apdu_msg_info_t;
#endif /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC || CONFIG_QOSA_ASR1803_PLATFORM_FUNC */

/**
 * @brief Open a logical APDU channel
 * Typically this function also needs to include configuring terminal capabilities and selecting the AID.
 *
 * @param[out] qosa_uint8_t * channel_id
 *          - Returns the channel ID that has been successfully opened
 *
 * @return esim_result_errno_e
 *          - Returns ESIM_OK on success
 *          - Returns other values on failure
 */
esim_result_errno_e esim_open_isdr_channel(qosa_uint8_t *channel_id);

/**
 * @brief Close the corresponding APDU logical channel, corresponding one-to-one with open.
 *
 * @param[in] qosa_uint8_t channel_id
 *          - The specified APDU logical channel.
 *
 * @return esim_result_errno_e
 *          - Returns ESIM_OK on success
 *          - Returns other values on failure
 */
esim_result_errno_e esim_close_channel(qosa_uint8_t channel_id);

/**
 * @brief Send APDU data to the specified channel ID, and simultaneously receive the corresponding reply content.
 *
 * The memory management of rsp_apdu_ptr/send_apdu_ptr is handled within the SDK library; no external release action is required.
 *
 * @param[in] qosa_uint8_t channel_id
 *          - The specified channel ID to operate on
 *
 * @param[in] esim_apdu_data_t * send_apdu_ptr
 *          - The content of the APDU to be sent
 *
 * @param[in] esim_apdu_data_t * rsp_apdu_ptr
 *          - The received APDU content
 *
 * @return esim_result_errno_e
 *          - Operation successful returns ESIM_OK
 *          - Returns other values on failure
 */
esim_result_errno_e esim_send_apdu_get_responce(qosa_uint8_t channel_id, esim_apdu_data_t *send_apdu_ptr, esim_apdu_data_t *rsp_apdu_ptr);

/**
 * @brief APDU function initialization
 *
 * Should be executed before the esim service is enabled.
 *
 * @return int
 *          - Returns QOSA_OK on successful operation
 *          - Returns other values on failure
 */
int esim_apdu_init(void);

/**
 * @brief APDU function deinitialization
 */
void esim_apdu_deinit(void);



#endif
