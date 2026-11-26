/*****************************************************************/ /**
* @file unirtos_atcmd_sim.h
* @brief
* @author Joe.tu@quectel.com
* @date 2024-05-16
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-04-16 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_SIM_H__
#define __UNIRTOS_ATCMD_SIM_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"

/**
 * @brief AT+CIMI command processing function
 *
 * This function is used to handle the AT+CIMI command, which reads the IMSI (International Mobile Subscriber Identity) information of the SIM card.
 * It executes the specific command processing logic by calling the AT command push interface.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Includes device port, command type, parameters, and other information
 */
void qstd_exec_cimi_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+CLCK command processing function
 *
 * This function is used to process the AT+CLCK command, which is used to set or query the facility lock status of the SIM card,
 * such as PIN lock, fixed dialing lock, and other functions.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_clck_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+CPIN command processing function
 *
 * This function is used to process the AT+CPIN command, which queries or sets the SIM card's PIN code status,
 * including operations such as verifying the PIN code and unlocking the PUK code.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains information such as the device port, command type, parameters, etc.
 */
void qstd_exec_cpin_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+CPWD command processing function
 *
 * This function is used to process the AT+CPWD command, which is used to modify the SIM card password,
 * including operations to change passwords such as PIN1, PIN2, etc.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains information such as device port, command type, parameters, etc.
 */
void qstd_exec_cpwd_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+CSIM command processing function
 *
 * This function is used to process the AT+CSIM command, which sends APDU commands to the SIM card
 * to perform various SIM card operations, such as reading files, verifying PIN, etc.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains information such as device port, command type, parameters, etc.
 */
void qstd_exec_csim_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+CRSM command processing function
 *
 * This function is used to process the AT+CRSM command, which performs restricted access operations on the SIM card,
 * including reading, updating binary files and records, and other operations.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_crsm_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QCCID command processing function
 *
 * This function is used to process the AT+QCCID command, which reads the ICCID (Integrated Circuit Card Identifier) information of the SIM card.
 * This is the unique identifier of the SIM card.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qccid_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+NCCID command processing function
 *
 * This function is used to process the AT+NCCID command, which reads the ICCID (Integrated Circuit Card Identifier) information of the SIM card.
 * This is the unique identifier of the SIM card.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_nccid_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QINISTAT command processing function
 *
 * This function is used to process the AT+QINISTAT command, which queries the SIM card initialization status,
 * including PIN code readiness, SMS initialization completion, phonebook initialization completion, and other statuses.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the information of the command to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qinistat_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QSIMDET command processing function
 *
 * This function is used to process the AT+QSIMDET command, which configures the SIM card hot-swap detection feature,
 * including enabling/disabling hot-swap detection and setting the detection pin level.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qsimdet_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QSIMSTAT command processing function
 *
 * This function is used to process the AT+QSIMSTAT command, which queries the status information of the SIM card,
 * including whether the SIM card is inserted, whether it is ready, the PIN code status, etc.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qsimstat_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+CCHO command execution handler
 *
 * This function handles the AT+CCHO command, which is used to open a logical channel for the SIM card.
 * Upon successful opening, the SIM will allocate a logical channel number. Subsequent APDU commands can be sent on this channel via the AT+CGLA command.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Includes device port, command type, parameters, and other information
 */
void qstd_exec_ccho_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+CGLA command processing function
 *
 * This function is used to process the AT+CGLA command, which is used to send APDU commands to the SIM card via a logical channel,
 * executing various SIM card operations, and supports a multi-application environment.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the information of the command to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_cgla_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+CCHC command processing function
 *
 * This function is used to process the AT+CCHC command, which is used to close a specified logical channel of the SIM card,
 * releasing resources by closing the designated logical channel.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the information of the command to be processed
 *           - Includes device port, command type, parameters, and other information
 */
void qstd_exec_cchc_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+CNUM command execution handler
 *
 * This function is used to process the AT+CNUM command, which reads the subscriber number information stored on the SIM card,
 * including the number type and the number content.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Includes device port, command type, parameters, and other information
 */
void qstd_exec_cnum_cmd(qosa_at_cmd_t *cmd);

#ifdef CONFIG_QAPP_UNIRTOS_AT_STK_CMD
/**
 * @brief AT^STA command processing function
 *
 * This function is used to process the AT^STA command, which queries and sets SAT terminal status parameters,
 * and triggers SAT terminal profile activation when needed.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Includes device port, command type, parameters, and other information
 */
void qstd_exec_sta_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT^STGI command processing function
 *
 * Returns detailed information of a pending SIM proactive command cached when
 * the @c ^STN URC was reported. Supports TEST/READ/SET forms.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Includes device port, command type, parameters, and other information
 */
void qstd_exec_stgi_cmd(qosa_at_cmd_t *cmd);
/**
 * @brief AT^STR (STK Terminal Response) command handler.
 *
 * Builds a raw BER-TLV terminal response from the supplied parameters and
 * sends it to the SIM card.  Must be called after AT^STGI; clears the cached
 * pending proactive command on success.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 */
void qstd_exec_str_cmd(qosa_at_cmd_t *cmd);
#endif

#ifdef CONFIG_QOSA_DSDS_FUNC
/**
 * @brief AT+QDSCCID command processing function
 *
 * This function is used to process the AT+QDSCCID command, which reads the ICCID information of the specified SIM card in dual SIM dual standby mode,
 * supporting the reading of the Integrated Circuit Card Identifier for both SIM cards separately.
 *
 * @note This command only takes effect in dual SIM dual standby mode
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qdsccid_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QDSIMI command processing function
 *
 * This function is used to process the AT+QDSIMI command, which reads the IMSI information of the specified SIM card in dual SIM dual standby mode,
 * supporting the reading of the International Mobile Subscriber Identity for both SIM cards separately.
 *
 * @note This command only takes effect in dual SIM dual standby mode.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qdsimi_cmd(qosa_at_cmd_t *cmd);
#endif
/**
 * @brief AT+QDSIM command processing function
 *
 * This function is used to process the AT+QDSIM command, which queries the status information of the SIM card,
 * including whether the SIM card is inserted, ready, and other status details.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Includes device port, command type, parameters, and other information
 */
void qstd_exec_qdsim_cmd(qosa_at_cmd_t *cmd);

#ifdef CONFIG_QOSA_DSDS_FUNC
/**
 * @brief AT+QDSPIN command processing function
 *
 * This function is used to process the AT+QDSPIN command, which queries the PIN status of both SIM cards in dual SIM dual standby mode,
 * including whether the SIM card is ready, whether a PIN is required, and other information.
 *
 * @note This command only takes effect in dual SIM dual standby mode
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qdspin_cmd(qosa_at_cmd_t *cmd);
#endif

/**
 * @brief AT+QDSTYPE command processing function
 *
 * This function is used to handle the AT+QDSTYPE command, which queries or sets the device's dual SIM mode type.
 * Common definitions for dual SIM mode types (single SIM, dual SIM dual standby, dual SIM single standby).
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qdstype_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QPINC command processing function
 *
 * This function is used to process the AT+QPINC command, which queries the remaining attempts for the SIM card PIN codes,
 * including the remaining attempt count information for passwords such as PIN1, PIN2, PUK1, and PUK2.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the information of the command to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qpinc_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief AT+QSLOTSTAT command processing function
 *
 * This function is used to process the AT+QSLOTSTAT command, which queries the status information of the SIM card slot,
 * including whether a SIM card is inserted into the slot, whether the SIM card is ready, and other status information.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qslotstat_cmd(qosa_at_cmd_t *cmd);
#ifdef CONFIG_QOSA_SIM_FUNC
/**
 * @brief AT+QSPN command processing function
 *
 * This function is used to process the AT+QSPN command, which queries or sets the service provider name of the SIM card,
 * including reading the operator name information stored in the SIM card.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qspn_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QOSA_SIM_FUNC */
/**
 * @brief AT+QWRITESIM command processing function
 *
 * This function is used to process the AT+QWRITESIM command, querying the update count information of a specific file on the SIM card
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qwritesim_cmd(qosa_at_cmd_t *cmd);


#ifdef CONFIG_QAPP_UNIRTOS_AT_ESIM_EID_CMD
/**
 * @brief AT+QESIM command processing function
 *
 * This function is used to process the AT+QESIM command, which performs SIM-related configuration and query operations
 * based on different sub-command names, and dispatches processing through the registered QESIM command table.
 *
 * @param [in] cmd
 *           - Pointer to the AT command structure, containing the command information to be processed
 *           - Contains device port, command type, parameters, and other information
 */
void qstd_exec_qesim_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_ESIM_EID_CMD */

#endif /* __UNIRTOS_ATCMD_SIM_H__ */
