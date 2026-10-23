/*****************************************************************/ /**
* @file mpc.h
* @brief Modem platform layer access layer component, provides support and some definitions for platform layer access
* @author Joe.tu@quectel.com
* @date 2024-03-14
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-03-14 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __MPC__H__
#define __MPC__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#define MPC_RET_SUCCESS                             0x00
#define MPC_RET_FAIL                                0x0f
#define MPC_FREE_LATER(ev, p)                       (ev.d |= (0x01 << (((qosa_ptr)&ev.p - ((qosa_ptr)&ev + 4)) / sizeof(qosa_ptr))))

/**
 * Convenience macro used when MPC layer returns correct result, can carry up to 3 parameters, I indicates that the parameter bit is a non-pointer type integer; P indicates that the parameter bit is a pointer type parameter; pointer type parameters are applied for a heap space by MPC layer, MS layer is automatically responsible for release.
 * MPC layer marks this parameter bit with 'P', later MS layer checks this mark and automatically releases.
 *
 */
#define MPC_POST_SUCCESS(simid, cb)                 (mpc_post_success_full(simid, cb, QOSA_NULL, QOSA_FALSE, QOSA_NULL, QOSA_FALSE, QOSA_NULL, QOSA_FALSE))
#define MPC_POST_SUCCESS_I(simid, cb, p1)           (mpc_post_success_full(simid, cb, (p1), QOSA_FALSE, QOSA_NULL, QOSA_FALSE, QOSA_NULL, QOSA_FALSE))
#define MPC_POST_SUCCESS_II(simid, cb, p1, p2)      (mpc_post_success_full(simid, cb, (p1), QOSA_FALSE, (p2), QOSA_FALSE, QOSA_NULL, QOSA_FALSE))
#define MPC_POST_SUCCESS_III(simid, cb, p1, p2, p3) (mpc_post_success_full(simid, cb, (p1), QOSA_FALSE, (p2), QOSA_FALSE, (p3), QOSA_FALSE))

#define MPC_POST_SUCCESS_P(simid, cb, p1)           (mpc_post_success_full(simid, cb, (qosa_ptr)(p1), QOSA_TRUE, QOSA_NULL, QOSA_FALSE, QOSA_NULL, QOSA_FALSE))

#define MPC_POST_SUCCESS_PI(simid, cb, p1, p2)      (mpc_post_success_full(simid, cb, (qosa_ptr)(p1), QOSA_TRUE, (p2), QOSA_FALSE, QOSA_NULL, QOSA_FALSE))
#define MPC_POST_SUCCESS_IP(simid, cb, p1, p2)      (mpc_post_success_full(simid, cb, (p1), QOSA_FALSE, (qosa_ptr)(p2), QOSA_TRUE, QOSA_NULL, QOSA_FALSE))
#define MPC_POST_SUCCESS_PP(simid, cb, p1, p2)      (mpc_post_success_full(simid, cb, (qosa_ptr)(p1), QOSA_TRUE, (qosa_ptr)(p2), QOSA_TRUE, QOSA_NULL, QOSA_FALSE))

#define MPC_POST_SUCCESS_PII(simid, cb, p1, p2, p3) (mpc_post_success_full(simid, cb, (qosa_ptr)(p1), QOSA_TRUE, (p2), QOSA_FALSE, (p3), QOSA_FALSE))
#define MPC_POST_SUCCESS_IPI(simid, cb, p1, p2, p3) (mpc_post_success_full(simid, cb, (p1), QOSA_FALSE, (qosa_ptr)(p2), QOSA_TRUE, (p3), QOSA_FALSE))
#define MPC_POST_SUCCESS_IIP(simid, cb, p1, p2, p3) (mpc_post_success_full(simid, cb, (p1), QOSA_FALSE, (p2), QOSA_FALSE, (qosa_ptr)(p3), QOSA_TRUE))
#define MPC_POST_SUCCESS_PPI(simid, cb, p1, p2, p3) (mpc_post_success_full(simid, cb, (qosa_ptr)(p1), QOSA_TRUE, (qosa_ptr)(p2), QOSA_TRUE, (p3), QOSA_FALSE))
#define MPC_POST_SUCCESS_IPP(simid, cb, p1, p2, p3) (mpc_post_success_full(simid, cb, (p1), QOSA_FALSE, (qosa_ptr)(p2), QOSA_TRUE, (qosa_ptr)(p3), QOSA_TRUE))
#define MPC_POST_SUCCESS_PPP(simid, cb, p1, p2, p3)                                                                                                            \
    (mpc_post_success_full(simid, cb, (qosa_ptr)(p1), QOSA_TRUE, (qosa_ptr)(p2), QOSA_TRUE, (qosa_ptr)(p3), QOSA_TRUE))

typedef qosa_uint32_t mpc_async_t;

typedef enum
{
    MPC_EVENT_SIM_INIT_IND,
    MPC_EVENT_SIM_INSERT_STAT_IND,
    /**
     * 1. When UniRTOS SMS complete functionality is needed, do not report this event through mpc ind mechanism, but report `MPC_SMS_INIT_IND` through callback function in mpc_sms.h file
     * 2. If UniRTOS SMS complete functionality is not needed, but UniRTOS needs to report complete boot process, can report SMS_INIT_IND message through this EVENT
     * 3. If neither is needed, do not report
     *
     */
    MPC_EVENT_SMS_INIT_IND, /*!< SMS initialization ok IND */
    MPC_EVENT_PBK_INIT_IND,
    MPC_EVENT_NW_REG_STATUS_IND,
    MPC_EVENT_NW_SIGNAL_QUALITY_IND,
    MPC_EVENT_NW_RRC_STATUS_IND,
    MPC_EVENT_DATACALL_PDP_STATUS_IND, /*!< Network side deactivation IND */
    MPC_EVENT_DATACALL_PDP_ACTIVE_STATUS_IND, /*!< Platform layer application executes activation or deactivation operation IND, used for UniRTOS layer status synchronization */
    MPC_EVENT_NW_NITZ_IND,
    MPC_EVENT_USBNET_STATUS_IND,
    MPC_EVENT_NW_NAS_EVENT_IND,
    MPC_EVENT_DEV_AIRPLANE_CTRL_IND,
    MPC_EVENT_NW_JAMM_DETECT_IND,
    MPC_EVENT_NW_REJECT_EVENT,
    MPC_EVENT_NW_CELL_CHANGE_EVENT,
    MPC_EVENT_NW_AS_EVENT_IND,
    MPC_EVENT_IMS_RING_IND,
    MPC_EVENT_IMS_DISCONNECT_IND,
    MPC_EVENT_IMS_CONN_ID_IND,

    MPC_EVENT_MAX,
} mpc_ind_e;

typedef struct
{
    qosa_uint32_t param0;
    qosa_ptr      param1;
    qosa_ptr      param2;
    qosa_ptr      param3;
    qosa_uint32_t d       : 3;
    qosa_uint32_t simid   : 1;
    qosa_uint32_t reserve : 28;
} mpc_event_prototype_t;

typedef struct
{
    qosa_uint32_t ind_id;
    qosa_ptr      param1;
    qosa_ptr      param2;
    qosa_ptr      param3;
    qosa_uint32_t d       : 3;
    qosa_uint32_t simid   : 1;
    qosa_uint32_t reserve : 28;
} mpc_ind_t;

/**
 * @brief When ret is success, p1 p2 p3 are custom
 *      When ret is fail, p1 represents detailed error code
 *
 */
typedef struct
{
    qosa_uint32_t event_id;
    qosa_ptr      param1;
    qosa_ptr      param2;
    qosa_ptr      param3;
    qosa_uint32_t d       : 3;
    qosa_uint32_t simid   : 1;
    qosa_uint32_t ret     : 4;
    qosa_uint32_t reserve : 24;
} mpc_cnf_t;

/** MPC layer context, passed to MS layer when MPC layer asynchronous interface returns */
typedef qosa_ptr mpc_cb_t;

/**
 * @brief MPC sends IND event to MS layer
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] ind
 *          - IND message body
 * @return int
 */
int mpc_post_ind(mpc_ind_t* ind);

/**
 * @brief
 *
 * @param[in] as Context carried by the request
 *          -
 * @param[in] cnf Dynamically applied cnf
 *          -
 * @return int
 */
int mpc_post_cnf(mpc_cb_t cb, mpc_cnf_t* cnf);

/**
 * @brief Release dynamically allocated resources in mpc event parameters
 *
 * @param[in] ev
 *          - Signal uploaded by mpc layer
 */
void mpc_free_param(mpc_event_prototype_t* ev);

/**
 * @brief Used for platform layer code to reply a successful mpc cnf message when implementing mpc interface
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] cb
 *          -
 * @param[in] param1
 *          - Parameter 1
 * @param[in] p1_is_p
 *          - Whether parameter 1 is a pointer
 * @param[in] param2
 *          - Parameter 2
 * @param[in] p2_is_p
 *          - Whether parameter 2 is a pointer
 * @param[in] param3
 * @param[in] p3_is_p
 *          - Parameter 3
 *          - Whether parameter 3 is a pointer
 */
void mpc_post_success_full(
    qosa_uint8_t simid,
    mpc_cb_t     cb,
    qosa_ptr     param1,
    qosa_bool_t  p1_is_p,
    qosa_ptr     param2,
    qosa_bool_t  p2_is_p,
    qosa_ptr     param3,
    qosa_bool_t  p3_is_p
);

/**
 * @brief MPC layer returns error result
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cb
 *          - MS layer callback context
 * @param[in] p1
 *          - Parameter 1
 * @param[in] p2
 *          - Parameter 2
 * @param[in] p3
 *          - Parameter 3
 */
void mpc_post_error(qosa_uint8_t simid, mpc_cb_t cb, qosa_ptr p1, qosa_ptr p2, qosa_ptr p3);

#endif/ !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif
