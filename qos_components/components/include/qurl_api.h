/** @file         qurl_api.h
 *  @brief        Brief description
 *  @details      Detailed description
 *  @author       lzm
 *  @date         2023-10-19 15:52:48
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Modification log:
 **********************************************************
 */

#ifndef QURL_API_H
#define QURL_API_H

#include "qurl_code.h"
#include "qurl_def.h"

/**
 * @brief Global initialization of qurl library
 *
 * @return qurl_ecode_t
 *       - Returns QURL_OK if successful
 *       - Returns other values if failed
 */
qurl_ecode_t qurl_global_init(void);

/**
 * @brief Global deinitialization of qurl library
 *
 * @return qcm_ntp_result_code
 *       - Returns QURL_OK if successful
 *       - Returns other values if failed
 */
qurl_ecode_t qurl_global_deinit(void);

/**
 * @brief Create a new qurl instance
 *
 * @param[in] qurl_core_t *core_ptr
 *          - qurl core operation handle
 *
 * @return qurl_ecode_t
 *       - Returns QURL_OK if successful
 *       - Returns other values if failed
 */
qurl_ecode_t qurl_core_create(qurl_core_t *core_ptr);

/**
 * @brief Delete the previously created qurl instance
 *
 * @param[in] qurl_core_t core
 *          - qurl core operation handle
 *
 * @return qurl_ecode_t
 *       - Returns QURL_OK if successful
 *       - Returns other values if failed
 */
qurl_ecode_t qurl_core_delete(qurl_core_t core);

/**
 * @brief Reset the previously created qurl instance
 *
 * @param[in] qurl_core_t core
 *          - qurl core operation handle
 *
 * @return qurl_ecode_t
 *       - Returns QURL_OK if successful
 *       - Returns other values if failed
 */
qurl_ecode_t qurl_core_reset(qurl_core_t core);

/**
 * @brief Set options for the given qurl instance
 *
 * @param[in] qurl_core_t core
 *          - qurl core operation handle
 *
 * @param[in] qurl_opt_e opt
 *          - Corresponding setting option
 *
 * @return qurl_ecode_t
 *       - Returns QURL_OK if successful
 *       - Returns other values if failed
 */
qurl_ecode_t qurl_core_setopt(qurl_core_t core, int opt, ...);

/**
 * @brief Get information for the given qurl instance
 *
 * @param[in] qurl_core_t core
 *          - qurl core operation handle
 *
 * @param[in] qurl_opt_e opt
 *          - Corresponding information option to get
 *
 * @return qurl_ecode_t
 *       - Returns QURL_OK if successful
 *       - Returns other values if failed
 */
qurl_ecode_t qurl_core_getinfo(qurl_core_t core, int opt, ...);

/**
 * @brief Start executing blocking network transmission
 *
 * @param[in] qurl_core_t core
 *          - qurl core operation handle
 *
 * @return qurl_ecode_t
 *       - Returns QURL_OK if successful
 *       - Returns other values if failed
 */
qurl_ecode_t qurl_core_perform(qurl_core_t core);

/**
 * @brief Terminate the network transmission process in advance
 *
 * @param[in] qurl_core_t *core
 *          - qurl core operation handle
 *
 * @return qurl_ecode_t
 *       - Returns QURL_OK if successful
 *       - Returns other values if failed
 */
qurl_ecode_t qurl_core_abort(qurl_core_t *core);

/**
 * @brief Insert into linked list by copying string
 *
 * @param[in] qurl_slist_t slist
 *          - Single linked list to insert into
 *
 * @param[in] char *str
 *          - String to add
 *
 * @return qurl_slist_t
 *       - Returns the inserted single linked list if successful
 *       - Returns NULL if failed
 */
qurl_slist_t qurl_slist_add_strdup(qurl_slist_t slist, char *str);

/**
 * @brief Free the entire single linked list
 *
 * @param[in] qurl_slist_t list
 *          - Single linked list to free
 */
void qurl_slist_del_all(qurl_slist_t list);

/**
 * @brief Initialize qurl-related tls parameters
 *
 * @param[in] qurl_tls_cfg_t *tls_cfg_ptr
 *          - tls configuration parameters
 */
void qurl_tls_cfg_init(qurl_tls_cfg_t *tls_cfg_ptr);

/**
 * @brief Get the reason for the last closure of the qurl instance
 *
 * @param[in] qurl_core_t core
 *          - qurl core operation handle
 */
int qurl_core_get_last_close_event(qurl_core_t core);

#endif /* Head define end*/
