/*****************************************************************/ /**
* @file qcm_lwm2m_fota.h
* @brief LWM2M FOTA implementation header file
* @author Jamie.li@quectel.com
* @date 2024-09-15
*
* @copyright Copyright (c) 2024 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
*
* <table><tbody><tr><th>Date </th><th>Version </th><th>Author </th><th>Description"
* </th></tr><tr><td>2024-09-15 </td><td>1.0 </td><td>Jamie.Li </td><td> Initial version
* </td></tr></tbody></table>
**********************************************************************/

#ifndef __QCM_LWM2M_FOTA_H__
#define __QCM_LWM2M_FOTA_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qcm_lwm2m_api.h"
#include "qcm_proj_config.h"
#include "qosa_fota.h"
/**
 * @enum  qcm_lwm2m_fota_type_e
 * @brief LWM2M FOTA types
 * @note The following states are not the same as 5/0/3 state
 */
typedef enum
{
    QCM_LWM2M_FOTA_STATE_IDLE = 0x00,   /*!< Idle state */
    QCM_LWM2M_FOTA_STATE_INIT,          /*!< Initial state */
    QCM_LWM2M_FOTA_STATE_DOWNLOADEDING, /*!< Downloading firmware */
    QCM_LWM2M_FOTA_STATE_DOWNLOADED,    /*!< Download completed */
} qcm_lwm2m_fota_state_e;

/**
 * @enum  qcm_lwm2m_fota_type_e
 * @brief LWM2M FOTA types
 */
typedef enum
{
    QCM_LWM2M_FOTA_TYPE_NONE = 0x00, /*!< Not supported */
    QCM_LWM2M_FOTA_TYPE_INBAND,      /*!< Inband: 5/0/0 */
    QCM_LWM2M_FOTA_TYPE_OUTBAND,     /*!< Outband: 5/0/1 */
} qcm_lwm2m_fota_type_e;

/**
 * @enum  qcm_lwm2m_fota_type_e
 * @brief LWM2M FOTA types
 */
typedef enum
{
    QCM_LWM2M_FOTA_SCHEME_NONE = 0x00, /*!< Not supported */
    QCM_LWM2M_FOTA_SCHEME_COAP,        /*!< COAP protocol */
    QCM_LWM2M_FOTA_SCHEME_COAPS,       /*!< COAPS protocol */
    QCM_LWM2M_FOTA_SCHEME_HTTP,        /*!< HTTP protocol */
    QCM_LWM2M_FOTA_SCHEME_HTTPS,       /*!< HTTPS protocol */
} qcm_lwm2m_fota_scheme_e;

typedef enum
{
    QCM_LWM2M_FOTA_CODE_BEGIN_DOWNLOADING, /*!< Start downloading */
    QCM_LWM2M_FOTA_CODE_DOWNLOAD_COMPLETED,/*!< Download completed, verification not finished */
    QCM_LWM2M_FOTA_CODE_DOWNLOAD_SUCCESS,  /*!< Download successful */
    QCM_LWM2M_FOTA_CODE_DOWNLOAD_FAIL,     /*!< Download failed */
    QCM_LWM2M_FOTA_CODE_UPDATING,          /*!< Start updating */
    QCM_LWM2M_FOTA_CODE_UPDATE_FAIL,       /*!< Update failed */
    QCM_LWM2M_FOTA_CODE_UPDATE_SUCCESS,    /*!< Update successful --- Report update result after restart and registration */
    QCM_LWM2M_FOTA_CODE_OVER,              /*!< Update finished --- Report update result after restart and registration */
} qcm_lwm2m_fota_code_e;

/**
 * @enum  qcm_lwm2m_fota_opt_e
 * @brief LWM2M FOTA options
 */
typedef enum
{
    QCM_LWM2M_FOTA_OPT_SIMID = 0x00,
    QCM_LWM2M_FOTA_OPT_PDPID,
    QCM_LWM2M_FOTA_OPT_AUTO_UPGRADE,
    QCM_LWM2M_FOTA_OPT_EVENT_CB,
    QCM_LWM2M_FOTA_OPT_EVENT_CB_ARG,
} qcm_lwm2m_fota_opt_e;

typedef void (*qcm_lwm2m_fota_event_cb)(qcm_lwm2m_fota_code_e code, void *arg);

/**
 * @struct qcm_lwm2m_fota_ctrl_t
 * @brief Configuration parameters for FOTA functionality
 */
struct qcm_lwm2m_fota_ctrl_s
{
    qosa_mutex_t                   lock;        /*!< FOTA control lock */
    qosa_int8_t                    client_id;   /*!< LWM2M client id */
    qcm_lwm2m_fota_state_e         state;       /*!< FOTA state */
    qcm_lwm2m_fota_type_e          type;        /*!< FOTA download type */
    qcm_lwm2m_fota_scheme_e        scheme;      /*!< FOTA download protocol */
    lwm2m_firmware_state_e         fota_state;  /*!< Corresponding firmware 5/0/3 state */
    lwm2m_firmware_uptate_result_e fota_result; /*!< Corresponding firmware 5/0/5 result */
    qosa_fota_t                   *fota_ptr;    /*!< FOTA operation handle */

    void *protocol_ptr;                         /*!< Protocol related handle */

    void (*deinit_cb)(qosa_int8_t client_id);   /*!< Protocol related handle release callback function */

    /* cfg */
    qosa_uint8_t            sim_id;
    qosa_uint8_t            pdp_id;
    qosa_uint8_t            auto_upgrade;
    char                   *url_ptr;    /*!< FOTA download URL */
    void                   *event_arg;
    qcm_lwm2m_fota_event_cb event_func; /*!< FOTA event callback */

    /* Download record */
    qosa_uint32_t write_pos; /*!< Length of downloaded content written */

    /* Deferred-update gate (guarded by lock) */
    qosa_bool_t pkg_verified;   /*!< Package signature/hash verification passed */
    qosa_bool_t pending_update; /*!< Server UPDATE(5/0/2) received while verifying */
};
typedef struct qcm_lwm2m_fota_ctrl_s qcm_lwm2m_fota_ctrl_t;

/**
 * @brief Initialize the LWM2M FOTA module.
 *
 * This function initializes the LWM2M FOTA (Firmware Over-The-Air) module,
 * setting up necessary resources and states for FOTA operations.
 *
 * @return int - Returns 0 on success, negative value on failure.
 */
int qcm_lwm2m_fota_init(void);

/**
 * @brief Initialize an LWM2M FOTA client instance.
 *
 * This function creates and initializes a new LWM2M FOTA client with the specified ID and type.
 *
 * @param [in] client_id     The unique identifier for the FOTA client instance.
 * @param [in] type          The type of FOTA client to initialize.
 * @return qcm_lwm2m_error_code_e - Error code indicating success or failure.
 */
qcm_lwm2m_error_code_e qcm_lwm2m_fota_client_init(qosa_int8_t client_id, qcm_lwm2m_fota_type_e type);

/**
 * @brief Deinitialize an LWM2M FOTA client instance.
 *
 * This function cleans up and releases resources associated with the specified FOTA client.
 *
 * @param [in] client_id     The unique identifier for the FOTA client instance to deinitialize.
 */
void qcm_lwm2m_fota_client_deinit(qosa_int8_t client_id);

/**
 * @brief Save firmware package data.
 *
 * This function saves received firmware package data at the specified offset for the given client.
 *
 * @param [in] client_id         The client identifier.
 * @param [in] inband_offset     The offset within the firmware image where data should be saved.
 * @param [in] fwbuf             Pointer to the firmware data buffer.
 * @return qcm_lwm2m_error_code_e - Error code indicating success or failure.
 */
qcm_lwm2m_error_code_e qcm_lwm2m_fota_save_pkg_data(qosa_int8_t client_id, qosa_uint16_t inband_offset, qcm_lwm2m_firmware_data_t *fwbuf);

/**
 * @brief Update the FOTA state for a client.
 *
 * This function updates the current FOTA state (e.g., downloading, updating) for the specified client.
 *
 * @param [in] client_id     The client identifier.
 * @param [in] fota_state    The new FOTA state to set.
 * @return qcm_lwm2m_error_code_e - Error code indicating success or failure.
 */
qcm_lwm2m_error_code_e qcm_lwm2m_fota_update_state(qosa_int8_t client_id, lwm2m_firmware_state_e fota_state);

/**
 * @brief Start firmware download process.
 *
 * This function initiates the firmware download process from the provided URL for the specified client.
 *
 * @param [in] client_id     The client identifier.
 * @param [in] url_ptr       Pointer to the URL string from which to download the firmware.
 * @return qcm_lwm2m_error_code_e - Error code indicating success or failure.
 */
qcm_lwm2m_error_code_e qcm_lwm2m_fota_dl_start(qosa_int8_t client_id, const char *url_ptr);

/**
 * @brief Stop firmware download process.
 *
 * This function stops the ongoing firmware download process for the specified client.
 *
 * @param [in] client_id     The client identifier.
 * @return qcm_lwm2m_error_code_e - Error code indicating success or failure.
 */
qcm_lwm2m_error_code_e qcm_lwm2m_fota_dl_stop(qosa_int8_t client_id);

/**
 * @brief Set FOTA options.
 *
 * This function sets various options for the FOTA client, such as timeouts or retry counts.
 *
 * @param [in] client_id     The client identifier.
 * @param [in] opt           The option to configure.
 * @param ...           Variable arguments depending on the option being set.
 * @return qcm_lwm2m_error_code_e - Error code indicating success or failure.
 */
qcm_lwm2m_error_code_e qcm_lwm2m_fota_setopt(qosa_int8_t client_id, qcm_lwm2m_fota_opt_e opt, ...);

/**
 * @brief Reboot the device after FOTA update.
 *
 * This function triggers a device reboot after a successful firmware update.
 *
 * @param [in] client_id     The client identifier.
 * @return qcm_lwm2m_error_code_e - Error code indicating success or failure.
 */
qcm_lwm2m_error_code_e qcm_lwm2m_fota_reboot(qosa_int8_t client_id);

/**
 * @brief Refresh firmware resource values.
 *
 * This function updates specific firmware resource values identified by their ID.
 *
 * @param [in] resource_id       The ID of the resource to update.
 * @param [in] resource_value    The new value for the resource.
 */
void qcm_lwm2m_fota_refresh_firmware_resource(qosa_uint16_t resource_id, qosa_int32_t resource_value);

/**
 * @brief Get current FOTA state.
 *
 * This function retrieves the current FOTA state for the specified client.
 *
 * @param [in] client_id     The client identifier.
 * @param [in/out] fota_state    Pointer to store the retrieved FOTA state.
 * @return qcm_lwm2m_error_code_e - Error code indicating success or failure.
 */
qcm_lwm2m_error_code_e qcm_lwm2m_get_fota_state(qosa_int8_t client_id, lwm2m_firmware_state_e *fota_state);

#endif /* __QCM_LWM2M_FOTA_H__ */
