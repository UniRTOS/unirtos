#ifndef __QL_SIM_H__
#define __QL_SIM_H__
#include "qosa_def.h"
#include "qosa_sim.h"
#include "qcm_mifi_net.h"

/**
 * @brief SIM card slot definitions
 */
typedef enum
{
    QCM_MIFI_SIM_SLOT_0 = 0,    ///< Slot 0
    QCM_MIFI_SIM_SLOT_1 = 1,    ///< Slot 1
    QCM_MIFI_SIM_SLOT_2 = 2,    ///< Slot 2
    QCM_MIFI_SIM_SLOT_MAX = 3,  ///< Total number of slots
} qcm_mifi_sim_slot_e;

/**
 * @brief Mobile network operator name
 */
typedef enum
{
    QCM_MIFI_SIM_OPER_UNKNOWN,  ///< Unknown operator
    QCM_MIFI_SIM_OPER_CMCC,     ///< China Mobile
    QCM_MIFI_SIM_OPER_CU,       ///< China Unicom
    QCM_MIFI_SIM_OPER_CT,       ///< China Telecom
    QCM_MIFI_SIM_OPER_CBN       ///< China Broadcasting Network
} qcm_mifi_sim_oper_name_e;

/**
 * @brief Auto-polling (SIM slot scanning) state
 */
typedef enum
{
    QCM_MIFI_SIMPOLL_IDLE,     ///< Idle, not started
    QCM_MIFI_SIMPOLL_RUNNING,  ///< Polling in progress
    QCM_MIFI_SIMPOLL_SUCCESS,  ///< Polling succeeded, SIM ready
    QCM_MIFI_SIMPOLL_FAILED,   ///< Polling failed after all rounds
    QCM_MIFI_SIMPOLL_STOPPED   ///< Polling stopped by user
} qcm_mifi_simpoll_state_e;

/** 
 * @brief           SIM card information
 */
typedef struct
{
    qcm_mifi_sim_slot_e slot;                             ///< Slot index
    qosa_bool_t         present;                          ///< Whether the SIM card is ready
    qosa_uint8_t        iccid[QOSA_SIM_ICCID_LEN + 1];    ///< ICCID string
    qosa_uint8_t        imsi[QOSA_SIM_IMSI_LEN_MAX + 1];  ///< IMSI string
} qcm_mifi_sim_info_t;

/** 
 * @brief           Auto-polling configuration (public subset)
 */
typedef struct
{
    qosa_uint8_t  poll_en;      ///< Auto-polling enable (0: disable, 1: no forbid_map, 2: no present + forbid_map)
    qosa_uint8_t  scan_en;      ///< Boot scan enable (0: disable, 1: every boot, 2: first boot only)
    qosa_uint32_t rounds;       ///< Number of polling rounds
    qosa_uint32_t interv_time;  ///< Interval between rounds in seconds
    qosa_uint32_t period_len;   ///< Periodic check interval in seconds
} qcm_mifi_simpoll_cfg_t;

/**
 * @brief           Callback type for SIM slot switch events
 * @param[in]       sim_slot    The new SIM slot index after switching
 */
typedef void (*qcm_mifi_sim_switch_cb_t)(qosa_int32_t sim_slot);

/**
 * @brief           Callback type for SIM slot change notifications
 * @param[in]       sim_slot    Current SIM slot index, see qcm_mifi_sim_slot_e
 * @return          None
 * @note            Must not block inside the callback
 */
typedef void (*qcm_mifi_sim_change_cb_t)(qosa_int32_t sim_slot);

/**
 * @brief           Callback type for SIM card information updates
 * @param[out]      info    Pointer to the updated SIM info array, see qcm_mifi_sim_info_t
 * @param[out]      size    Size of the array in bytes
 */
typedef void (*qcm_mifi_sim_info_cb_t)(qcm_mifi_sim_info_t *info, qosa_uint32_t size);

/**
 * @brief           Switch to a specified SIM slot
 * @param[in]       sim_slot    Target SIM slot, see qcm_mifi_sim_slot_e
 * @param[in]       manual      QOSA_TRUE for user-initiated switch (stops auto-poll), QOSA_FALSE for internal switch
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_sim_set_slot(qosa_uint8_t sim_slot, qosa_bool_t manual);

/**
 * @brief           Get the current active SIM slot
 * @param[out]      sim_slot    Current SIM slot index, see qcm_mifi_sim_slot_e
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_sim_get_slot(qosa_uint8_t *sim_slot);

/**
 * @brief           实时获取当前SIM状态
 * @param[in]       simid       SIM卡ID，双卡单待场景固定使用0
 * @param[out]      status      SIM实时状态
 * @return          成功返回QCM_MIFI_ERR_OK，否则返回错误码
 * @note            优先查询底层实时状态，查询失败时回退到缓存状态
 */
qcm_mifi_err_e qcm_mifi_sim_get_status(qosa_uint8_t simid, qosa_sim_status_e *status);

/**
 * @brief           获取平台实际支持的SIM卡槽掩码
 * @param[out]      slot_mask   卡槽掩码，bit0/bit1/bit2分别对应槽位0/1/2
 * @return          成功返回QCM_MIFI_ERR_OK，否则返回错误码
 */
qcm_mifi_err_e qcm_mifi_sim_get_supported_slots(qosa_uint8_t *slot_mask);

/**
 * @brief           Check whether a SIM slot is forbidden
 * @param[in]       slot        SIM slot index to query
 * @param[out]      forbid      QOSA_TRUE if the slot is forbidden, QOSA_FALSE otherwise
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_sim_get_forbid(qosa_int32_t slot, qosa_bool_t *forbid);

/**
 * @brief           Set or clear the forbidden flag for a SIM slot
 * @param[in]       slot        SIM slot index to modify
 * @param[in]       is_forbid   QOSA_TRUE to forbid the slot, QOSA_FALSE to allow it
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 * @note            If the currently active slot is forbidden, it will auto-switch to the next valid slot
 */
qcm_mifi_err_e qcm_mifi_sim_set_forbid(qosa_int32_t slot, qosa_bool_t is_forbid);

/**
 * @brief           Get auto-polling configuration
 * @param[out]      cfg     Current auto-polling configuration, see qcm_mifi_simpoll_cfg_t
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_sim_autopoll_get_cfg(qcm_mifi_simpoll_cfg_t *cfg);

/**
 * @brief           Set auto-polling configuration
 * @param[in]       cfg     Desired auto-polling configuration, see qcm_mifi_simpoll_cfg_t
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_sim_autopoll_set_cfg(qcm_mifi_simpoll_cfg_t *cfg);

/**
 * @brief           Identify mobile network operator by IMSI
 * @param[in]       imsi    IMSI byte string
 * @param[out]      type    Identified operator, see qcm_mifi_sim_oper_name_e
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_sim_get_oper_by_imsi(qosa_uint8_t *imsi, qcm_mifi_sim_oper_name_e *type);

/**
 * @brief           Get current auto-polling state
 * @param[out]      state   Current auto-polling state, see qcm_mifi_simpoll_state_e
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_sim_autopoll_get_status(qcm_mifi_simpoll_state_e *state);

/**
 * @brief           Stop the auto-polling process
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_sim_autopoll_stop(void);

/**
 * @brief           Restart the auto-polling process
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_sim_autopoll_restart(void);

/**
 * @brief           Register a callback for SIM slot switch completion
 * @param[in]       cb      Callback invoked after the active slot changes
 * @return          None
 */
void qcm_mifi_sim_register_switch_cb(qcm_mifi_sim_switch_cb_t cb);

/**
 * @brief           Request all cached SIM information
 * @param[in]       cb      Callback that receives the SIM information array
 * @return          0 on success, negative value on error
 */
qosa_int32_t qcm_mifi_sim_update_all_sim_info(qcm_mifi_sim_info_cb_t cb);

/**
 * @brief           Save the default SIM slot used by auto-polling
 * @param[in]       slot    Default SIM slot, see qcm_mifi_sim_slot_e
 * @return          0 on success, negative value on error
 */
qosa_int32_t qcm_mifi_sim_save_default_slot(qosa_int32_t slot);

/**
 * @brief           Initialize the MiFi SIM management module
 * @note            Creates the SIM manager instance and starts the auto-poll task
 */
void qcm_mifi_sim_init(void);

#endif /* _QL_SIM_H_ */
