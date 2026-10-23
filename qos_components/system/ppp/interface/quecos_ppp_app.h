/*****************************************************************/ /**
* @brief QuecOS PPP Application Interface Header File
* @details This file provides interface definitions for PPP (Point-to-Point Protocol) application initialization, connection establishment,
*          connection termination, and related configuration functions. Supports multiple PPP disconnection methods,
*          providing a flexible connection management mechanism.
* @author Joe.tu@quectel.com
* @date 2025-04-07
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-04-07 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_at_sio.h"

/**
 * @enum qppp_drop_method_e
 * @brief PPP Disconnection Method Enumeration
 * @details Defines different handling methods for PPP connection disconnection, controlling whether to send a PPP terminate frame
 *          and the conditions and manner of sending it.
 */
typedef enum
{
    QPPP_DROP_METHOD_NONE = 0,            /*!< No disconnection handling method, keep default behavior, determined by terminate frame flag */
    QPPP_DROP_METHOD_NOT_SEND_TERM = 1,   /*!< Disconnect without sending a PPP termination frame */
    QPPP_DROP_METHOD_AUTO_SEND_TERM = 2,  /*!< Automatically decide whether to send a PPP termination frame based on the connection status */
    QPPP_DROP_METHOD_FORCE_SEND_TERM = 3, /*!< Force sending a PPP termination frame before disconnecting */
} qppp_drop_method_e;

/**
 * @typedef establish_cb
 * @brief PPP connection establishment callback function type
 * @brief Used to notify the caller of the connection result when the PPP connection establishment is complete
 *
 * @param[in] ctx Callback function context pointer, specified by the caller when establishing the connection
 * @param[in] err Connection establishment result error code, 0 indicates success, non-zero indicates failure
 * @return int Callback processing result, 0 indicates success, non-zero indicates failure
 */
typedef int (*establish_cb)(void* ctx, qosa_int32_t err);

/**
 * @struct qppp_establish_param_t
 * @brief PPP Connection Establishment Request Parameter Structure
 * @details Contains all parameter information required to establish a PPP connection
 */
typedef struct
{
    qosa_uint8_t       simid;    /*!< SIM card ID used by PPP, indicating which SIM card is used */
    qosa_uint8_t       pdpid;    /*!< PDP context ID used by PPP, indicating which APN configuration is used */
    qosa_at_dev_type_e dev_hd;   /*!< Corresponding AT command channel handle, used for the AT channel in PPP communication */
    establish_cb       callback; /*!< Connection establishment callback function, called when the connection is completed */
    void*              ctx;      /*!< Context pointer for the connection establishment callback function, passed to the callback function */
} qppp_establish_param_t;

/**
 * @struct qppp_terminate_param_t
 * @brief PPP Connection Termination Request Parameter Structure
 * @details Contains the parameter information required to terminate a PPP connection, specifying the method of disconnection
 */
typedef struct
{
    qosa_at_dev_type_e dev_hd;      /*!< Corresponds to the AT command channel handle, specifies the channel where the PPP connection to be terminated is located */
    qppp_drop_method_e drop_method; /*!< PPP disconnection method, specifies the handling method when disconnecting */
} qppp_terminate_param_t;

/**
 * @brief PPP Application Initialization Function
 * @details Initializes the PPP application module, allocates necessary resources, registers relevant callback functions,
 *          and prepares for the subsequent establishment and management of PPP connections. This function must be called
 *          before invoking any other PPP interface functions.
 */
void qosa_ppp_app_init(void);

/**
 * @brief Request to establish a PPP connection
 * @details Asynchronously establishes a PPP connection based on the specified parameters. The connection establishment process is asynchronous,
 *          and the result will be notified to the caller via a callback function.
 *
 * @param[in] req Pointer to the PPP connection establishment request parameters
 *             - simid: Specifies the SIM card to use
 *             - pdpid: Specifies the PDP context to use
 *             - dev_hd: Specifies the AT command channel
 *             - callback: Callback function upon connection establishment completion
 *             - ctx: Context passed to the callback function
 *
 * @return int Function execution result
 */
int qppp_establish_req(qppp_establish_param_t* req);

/**
 * @brief Request to hang up the PPP connection
 * @details Terminates the established PPP connection according to the specified parameters. Supports multiple disconnection methods,
 *           allowing the choice of whether to send a PPP termination frame and the sending method.
 *
 * @param[in] ter Pointer to the PPP connection termination request parameters
 *             - dev_hd: Specifies the AT channel where the PPP connection to be terminated resides
 *             - drop_method: Specifies the handling method for disconnecting
 *
 * @return int Function execution result
 */
int qppp_terminate_req(qppp_terminate_param_t* ter);

/**
 * @brief Set PPP termination frame transmission flag
 * @details Configures whether to send a termination frame when disconnecting the PPP link.
 *          This flag affects the normal PPP disconnection procedure, ensuring proper
 *          negotiation and disconnection with the peer device.
 *
 * @param[in] flag Termination frame transmission flag
 *             - 1: Send a terminate REQ command when hanging up PPP to perform the normal disconnection procedure
 *             - 0: Do not send a terminate REQ command when hanging up PPP; disconnect directly
 *
 * @return int Function execution result
 */
int qppp_set_termframe_flag(qosa_uint8_t flag);

/**
 * @brief Get PPP termination frame transmission flag
 * @details Retrieves the currently configured PPP termination frame transmission flag status, used to understand
 *           the behavior configuration when the current PPP connection is disconnected.
 *
 * @return qosa_uint8_t Current termination frame transmission flag status
 *         - 1: Configured to send terminate REQ command
 *         - 0: Configured not to send terminate REQ command
 */
qosa_uint8_t qppp_get_termframe_flag(void);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 