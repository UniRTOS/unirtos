/*****************************************************************/ /**
* @file mpc_sim.h
* @brief SMS operation interface for modem port layer
* @author Joe.tu@quectel.com
* @date 2024-03-18
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-03-18 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __MPC_SMS__H__
#define __MPC_SMS__H__

#include "mpc.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sms.h"
#include "ms_sms.h"

/** SMS module initialization OK completion event */
#define MPC_SMS_INIT_IND            (0x0001)
/** New SMS arrival event, report complete PDU data, upper layer determines how to notify user layer, whether to store, etc., event parameters `qosa_sms_record_t.pdu`, `qosa_sms_record_t.msg_type` */
#define MPC_SMS_NEW_MESSAGE_IND     (0x0002)
/** New SMS arrival event, bottom layer has completed storage and other operations, notify upper layer to complete cache update, user notification and other operations, event parameters `mpc_sms_new_msg_idx_t.index`, `mpc_sms_new_msg_idx_t.stor` are valid */
#define MPC_SMS_NEW_MESSAGE_IDX_IND (0x0003)

/** MPC layer SMS related event callback function prototype */
typedef void (*mpc_sms_cb)(qosa_uint8_t simid, qosa_uint32_t event, void* data);

/**
 * @enum mpc_sms_me_flush_e
 * @brief When updating SMS ME storage, give operation type
 *
 */
typedef enum
{
    MPC_SMS_ME_FLUSH_OPT_WRITE = 0,  /*!< Write SMS operation */
    MPC_SMS_ME_FLUSH_OPT_DELETE = 1, /*!< Delete SMS operation */
    MPC_SMS_ME_FLUSH_OPT_UPDATE = 2, /*!< Update SMS operation */
} mpc_sms_me_flush_e;

#if 1
typedef struct
{
    qosa_uint16_t       index;     // Index in storage
    qosa_sms_status_e   status;    // Status
    qosa_sms_msg_type_e msg_type;  // Short message type
} qosa_sms_record_without_pdu_t;
#endif

/**
 * @struct mpc_sms_new_msg_idx_t
 * @brief New SMS reception indication, indicating bottom layer has completed storage and other operations, notify upper layer to complete cache update, user notification and other operations
 *
 */
typedef struct
{
    qosa_uint16_t       index;    /*!< Index in storage */
    qosa_sms_stor_e     stor;     /*!< Storage type */
    qosa_sms_msg_type_e msg_type; /*!< Short message type */
} mpc_sms_new_msg_idx_t;

/**
 * @struct mpc_sms_config_t
 * @brief Platform layer SMS configuration
 *
 */
typedef struct
{
    // CPMS parameters, some platforms implement storage of new SMS after arrival on CP side, so storage selection is implemented on platform layer
    qosa_uint8_t mem1; /*!< Memory 1, memory from which messages are read or deleted */
    qosa_uint8_t mem2; /*!< Memory 2, memory to which writing and sending operations are performed */
    qosa_uint8_t mem3; /*!< Memory 3, memory to which received messages are stored */

    // CNMI parameters, some platforms implement storage of new SMS after arrival on CP side, so new SMS arrival behavior configuration also needs to be implemented on platform layer
    qosa_uint8_t nmi_mode; /*!< NMI mode */
    qosa_uint8_t nmi_mt;   /*!< NMI mt */
    qosa_uint8_t nmi_bm;   /*!< NMI bm */
    qosa_uint8_t nmi_ds;   /*!< NMI ds */
    qosa_uint8_t nmi_bfr;  /*!< NMI bfr */
} mpc_sms_config_t;

/** Error code conversion, add corresponding function when adapting to platform, 0 means success */
int err_sms_mpc2ms(qosa_ptr err);

/**
 * @brief Get SMS service center address (SCA) for specified SIM card
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[out] sca
 *          - Output parameter, used to store the obtained SMS service center address information
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Get successful
 *          - QOSA_SMS_ERROR: Get failed
 */
int mpc_sms_get_sca(qosa_uint8_t simid, qosa_sms_address_info_t* sca);

/**
 * @brief Set SMS service center address (SCA) for specified SIM card
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] sca
 *          - Input parameter, containing SMS service center address information to be set
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Set successful
 *          - QOSA_SMS_ERROR: Set failed
 */
int mpc_sms_set_sca(qosa_uint8_t simid, qosa_sms_address_info_t* sca);

/**
 * @brief Modify platform layer configuration information
 *
 * @param[in] simid
 *          - sim id
 * @param[in] config
 *          - Input parameter, containing configuration information to be modified
 *          Including:
 *          - `Default storage type`: Default memory type used for reading/writing SMS and storing newly received SMS
 *          - `New SMS reception behavior`: Control system behavior parameters when new SMS arrives
 *
 * @return int
 */
int mpc_sms_set_config(qosa_uint8_t simid, mpc_sms_config_t* config);

/**
 * @brief Get platform layer configuration information
 *
 * This function is used to provide necessary SMS configuration details to upper layer modules after device power-on,
 * this information is usually pulled during system initialization.
 *
 * @note Information that needs to be returned:
 * 1. Default storage type information for reading/writing SMS and storing newly received SMS
 * 2. Behavior control parameters when receiving new SMS
 *
 * @param[in] simid
 *          - sim id
 * @param[out] config
 *          - Used to store retrieved SMS configuration information
 *          Including:
 *          - `Default storage type`: Default memory type used for reading/writing SMS and storing newly received SMS
 *          - `New SMS reception behavior`: Control system behavior parameters when new SMS arrives
 * @return int
 *          - QOSA_SMS_SUCCESS: Success
 *          - QOSA_SMS_ERROR: Failure
 *
 */
int mpc_sms_get_config(qosa_uint8_t simid, mpc_sms_config_t* config);

/**
 * @brief Read SMS storage space information, such as maximum available space for SMS, used space, etc.
 *
 * @param[in] simid
 *          - sim id
 * @param[out] info
 *          - Only need to adapt used_sm and total_sm
 *          - used_me/total_me do not need to be adapted, related information is returned through mpc_sms_me_get_count/mpc_sms_me_get_capacity
 * @return int
 *          - QOSA_SMS_SUCCESS: success
 *          - QOSA_SMS_ERROR: fail
 */
int mpc_sms_read_storage_info(qosa_uint8_t simid, qosa_sms_stor_info_t* info);

/**
 * @brief Get status information of specified index SMS record (without affecting original status)
 *
 * @note This interface reads its index and status information without changing the record status in SMS storage.
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] index
 *          - SMS storage index
 * @param[out] record
 *          - Output SMS status information structure, must contain at least valid index and status fields
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: success
 *          - QOSA_SMS_ERROR: fail
 */
int mpc_sms_raw_record_status(qosa_uint8_t simid, qosa_uint16_t index, qosa_sms_record_without_pdu_t* record);

/**
 * @brief Read SMS record of specified index in SIM card (index starts from 0)
 *
 * @note Requirement: If SMS record corresponding to specified index does not exist, need to return `QOSA_SMS_NO_MSG_ERR`.
 *            record needs to return `index`, `status`, `msg_type`, `pdu`, PDU data is complete PDU data including SCA, if no SCA, need to add a 0 to TPDU.
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] index
 *          - SMS record index number, counting from 0
 * @param[out] record
 *          - Pointer to SMS record structure for output
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Read successful
 *          - QOSA_SMS_NO_MSG_ERR: No corresponding SMS record
 *          - QOSA_SMS_ERROR: Read failed (other errors)
 */
int mpc_sms_read_record(qosa_uint8_t simid, qosa_uint16_t index, qosa_sms_record_t* record);

/**
 * @brief Write a SMS record to SIM storage
 *
 * @note If storage is full, should return error code QOSA_SMS_CMS_MEM_FULL
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] record
 *          - SMS record content to be written
 * @param[out] index
 *          - Index number returned after successful writing (starts from 0)
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Write successful
 *          - QOSA_SMS_CMS_MEM_FULL: Storage full
 *          - QOSA_SMS_ERROR: Write failed (other errors)
 */
int mpc_sms_write_record(qosa_uint8_t simid, qosa_sms_record_t* record, qosa_uint16_t* index);

/**
 * @brief Control connection maintenance strategy between multiple SMS messages to improve efficiency of continuously sending multiple SMS messages.
 *
 * @param[in] simid
 *          - sim id
 * @param[in] mode
 *          - QOSA_SMS_MORE_MSG_MODE_DISABLE: Disable this feature, close link immediately after sending one message.
 *          - QOSA_SMS_MORE_MSG_MODE_ONESHOT: Enable this feature, link automatically closes when no SMS is sent for a short time (1~5 seconds), and reset state to 0.
 *          - QOSA_SMS_MORE_MSG_MODE_CONTINUOUS: Enable this feature, link automatically closes when no SMS is sent, but state is not automatically reset to 0.
 * @return int
 */
int mpc_sms_set_more_msg_mode(qosa_uint8_t simid, qosa_sms_more_msg_mode_e mode);

/**
 * @brief Get current settings of multiple SMS message connection maintenance strategy.
 *
 * @param[in] simid
 *          - sim id
 * @param[out] mode
 *          - Pointer to output variable, used to return current mode value
 * @return int
 */
int mpc_sms_get_more_msg_mode(qosa_uint8_t simid, qosa_sms_more_msg_mode_e* mode);

/**
 * @brief Platform layer interface, used to send SMS
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] record
 *          - SMS record to be sent, only `pdu` field is valid, other fields are ignored
 * @param[out] mr
 *          - Message reference number (Message Reference, MR) returned after successful SMS sending
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Send successful
 *          - QOSA_SMS_ERROR: Send failed
 */
int mpc_sms_send_record(qosa_uint8_t simid, qosa_sms_record_t* record, qosa_uint8_t* mr);

/**
 * @brief Delete SMS record of specified index
 *
 * @param[in] simid
 *          - sim id
 * @param[in] index
 *          - SMS record index number, counting from 0
 * @return int
 */
int mpc_sms_delete_record(qosa_uint8_t simid, qosa_uint16_t index);

/**
 * @brief Update status of specified SMS record (index starts from 0)
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] index
 *          - SMS record index number to be updated (starts from 0)
 * @param[in] status
 *          - New SMS status to be set, type is qosa_sms_status_e
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Status update successful
 *          - QOSA_SMS_ERROR: Update failed (other errors)
 */
int mpc_sms_update_record_status(qosa_uint8_t simid, qosa_uint16_t index, qosa_sms_status_e status);

/**
 * @brief Send RP-ACK or RP-ERROR response to protocol stack
 *
 * Used to confirm received RP-DATA messages, only valid when there is a pending confirmation request.
 *
 * @param[in] simid
 *          - SIM card identifier (currently not used, but reserved to support multiple SIM)
 * @param[in] ack_type
 *          - Response type: 0 means RP-ACK, 1 means RP-ERROR
 * @param[in] cause
 *          - Error reason, only valid when ack_type is RP-ERROR
 * @param[in] pdu
 *          - SMS PDU data pointer (currently not used, can be NULL)
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Response successful
 *          - QOSA_SMS_CMS_NO_CNMA_ACK_EXPECTED: Currently no pending message to respond
 *          - QOSA_SMS_ERROR: Memory allocation failure or other error
 */

int mpc_sms_rp_ack(qosa_uint8_t simid, qosa_uint8_t ack_type, qosa_uint8_t cause, qosa_sms_pdu_t* pdu);

/**
 * @brief Notify SMS storage full status
 *
 * @param[in] simid
 *          - sim id
 * @param[in] is_full
 *          - Whether SMS storage is full
 * @return int
 */
int mpc_sms_notify_mem_full(qosa_uint8_t simid, qosa_bool_t is_full);

/**
 * @brief ME storage get number of used SMS
 *
 * @param[in] simid
 *          - sim id
 * @return qosa_uint16_t Return number of used SMS
 */
qosa_uint16_t mpc_sms_me_get_count(qosa_uint8_t simid);

/**
 * @brief ME storage get total SMS capacity
 *
 * @param[in] simid
 *          - sim id
 * @return qosa_uint16_t Return total SMS capacity
 */
qosa_uint16_t mpc_sms_me_get_capacity(qosa_uint8_t simid);

/**
 * @brief Get index list of all valid SMS in ME storage under specified SIM card
 * @note Index numbers start counting from 0
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[out] indices
 *          - Pre-allocated array, used to store valid SMS record indexes
 * @param[in] max_size
 *          - Maximum capacity of indices array, used to prevent array out-of-bounds writing
 *
 * @return qosa_uint16_t
 *          - Actual number of valid SMS indexes written to indices array;
 *          - Return 0 if no valid records or parameters are illegal.
 */
qosa_uint16_t mpc_sms_me_get_all_indices(qosa_uint8_t simid, qosa_uint16_t* indices, qosa_uint16_t max_size);

/**
 * @brief Get index list of SMS with specific status in ME storage under specified SIM card
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] status
 *          - Status filter bitmap, type is `qosa_sms_status_bit_e`, can combine multiple status bits bitwise.
 *          - For example: unread, read, draft, sent successfully, etc.
 *          - If set to `QOSA_SMS_STATUS_BIT_ALL`, match all valid SMS.
 * @param[out] indices
 *          - Pointer, array provided by caller, used to receive matched SMS indexes (starting from 0).
 *          - Length must not be less than `max_size`.
 * @param[in] max_size
 *          - Maximum capacity of `indices` array, function writes at most this number of matching indexes.
 * @return qosa_uint16_t
 *          - Actual number of SMS indexes written;
 *          - Return 0 if no matches or input is invalid.
 */
qosa_uint16_t mpc_sms_me_get_indices_by_status_list(qosa_uint8_t simid, qosa_sms_status_bit_e status, qosa_uint16_t* indices, qosa_uint16_t max_size);

/**
 * @brief Read SMS record of specified index (index starts from 0)
 *
 * @note Requirement: If SMS record corresponding to specified index does not exist, need to return `QOSA_SMS_NO_MSG_ERR`.
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] index
 *          - SMS record index number, counting from 0
 * @param[out] record
 *          - Pointer to SMS record structure for output
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Read successful
 *          - QOSA_SMS_NO_MSG_ERR: No corresponding SMS record
 *          - QOSA_SMS_ERROR: Read failed (other errors)
 */
int mpc_sms_me_read_record(qosa_uint8_t simid, qosa_uint16_t index, qosa_sms_record_t* record);

/**
 * @brief Write a SMS record to ME storage
 *
 * @note Requirement: If storage is full, should return error code `QOSA_SMS_CMS_MEM_FULL`
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] record
 *          - SMS record content to be written
 * @param[out] index
 *          - SMS index number returned after successful writing (starts from 0)
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Write successful
 *          - QOSA_SMS_CMS_MEM_FULL: Storage full
 *          - QOSA_SMS_ERROR: Write failed (other errors)
 */
int mpc_sms_me_write_record(qosa_uint8_t simid, qosa_sms_record_t* record, qosa_uint16_t* index);

/**
 * @brief Delete SMS record of specified index (index starts from 0)
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] index
 *          - SMS record index number to be deleted (starts from 0)
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Delete successful
 *          - QOSA_SMS_NO_MSG_ERR: No SMS record at specified index
 *          - QOSA_SMS_ERROR: Delete failed (other errors)
 */
int mpc_sms_me_delete_record(qosa_uint8_t simid, qosa_uint16_t index);

/**
 * @brief Update status of specified SMS record (index starts from 0)
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] index
 *          - SMS record index number to be updated (starts from 0)
 * @param[in] new_status
 *          - New SMS status to be set
 *
 * @return int
 *          - QOSA_SMS_SUCCESS: Status update successful
 *          - QOSA_SMS_NO_MSG_ERR: No valid SMS record at specified index
 *          - QOSA_SMS_ERROR: Status update failed (other errors)
 */
int mpc_sms_me_update_record_status(qosa_uint8_t simid, qosa_uint16_t index, qosa_sms_status_e new_status);

/**
 * @brief After operating on ME SMS storage, refresh SMS storage information (update data to non-volatile storage)
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] opt
 *          - Operation type
 * @return int
 */
int mpc_sms_me_flush(qosa_uint8_t simid, mpc_sms_me_flush_e opt);

/**
 * @brief Register callback function, call this callback function to notify ms_sms layer when SMS initialization status changes, new SMS arrives
 *       - MPC_SMS_INIT_IND: SMS initialization completed
 *       - MPC_SMS_NEW_SMS_IND: New SMS arrives
 *
 * @param[in] cb
 *          - ms_sms layer callback function
 */
void mpc_sms_register_cb(mpc_sms_cb cb);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif
