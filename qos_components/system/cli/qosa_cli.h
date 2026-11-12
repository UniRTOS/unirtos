/*****************************************************************/ /**
 * @file qosa_cli.h
 * @brief UniRTOS CLI session wrapper interface
 * @author joe.tu@quectel.com
 * @date 2026-06-28
 *
 * @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description"
 * <tr><td>2026-06-28 <td>1.0 <td>Joe.Tu <td> Init
 * </table>
 **********************************************************************/
#ifndef __QOSA_CLI_H__
#define __QOSA_CLI_H__

#include "qosa_def.h"
#include "qosa_at_sio.h"


/**
 * @brief Callback used to emit CLI text to the active frontend.
 *
 * @param[in] const char *text
 *          - Text buffer to output.
 *
 * @param[in] qosa_size_t length
 *          - Valid byte length of text.
 *
 * @param[in] void *ctx
 *          - User context passed during session enter.
 */
typedef void (*qosa_cli_write_cb_t)(const char *text, qosa_size_t length, void *ctx);

/**
 * @brief Callback used by a registered CLI command.
 *
 * @param[in] char *write_buffer
 *          - Shared CLI output buffer owned by the command runner.
 *
 * @param[in] qosa_size_t write_buffer_len
 *          - Valid byte capacity of write_buffer.
 *
 * @param[in] const char *command_string
 *          - Full command line string received from the frontend.
 *
 * @return qosa_bool_t
 *          - QOSA_TRUE when more output chunks will follow.
 *          - QOSA_FALSE when the command has finished emitting output.
 */
typedef qosa_bool_t (*qosa_cli_cmd_handler_t)(char *write_buffer, qosa_size_t write_buffer_len, const char *command_string);

/**
 * @struct qosa_cli_command_t
 * @brief Public CLI command descriptor used by platform modules.
 */
typedef struct
{
    const char            *command;                    /*!< Command keyword without trailing spaces. */
    const char            *help;                       /*!< Help text shown by the built-in help command. */
    qosa_cli_cmd_handler_t handler;                    /*!< Command callback invoked by the CLI core. */
    qosa_int8_t            expected_parameter_count;   /*!< Expected parameter count, or -1 for variable count. */
} qosa_cli_command_t;

/**
 * @brief Initialize the shared CLI core and register platform commands once.
 *
 * @return qosa_bool_t
 *          - QOSA_TRUE on success.
 *          - QOSA_FALSE on invalid platform state.
 */
qosa_bool_t qosa_cli_init(void);

/**
 * @brief Register a CLI command by using the UniRTOS public command descriptor.
 *
 * @param[in] const qosa_cli_command_t *command
 *          - Command descriptor with static-lifetime strings and handler.
 *
 * @note Example for platform or feature modules:
 * @code
 * #if defined(CONFIG_QOSA_CLI_FUNC)
 * static qosa_bool_t qdemo_cli_cmd(char *write_buffer, qosa_size_t write_buffer_len, const char *command_string)
 * {
 *     QOSA_UNUSED(command_string);
 *     qosa_snprintf(write_buffer, write_buffer_len, "demo ok\r\n");
 *     return QOSA_FALSE;
 * }
 *
 * static const qosa_cli_command_t g_qdemo_cli_cmd = {
 *     .command = "demo",
 *     .help = "demo:\r\n Show demo output\r\n\r\n",
 *     .handler = qdemo_cli_cmd,
 *     .expected_parameter_count = 0
 * };
 *
 * static void qdemo_cli_register(void)
 * {
 *     (void)qosa_cli_register_command(&g_qdemo_cli_cmd);
 * }
 * #endif // CONFIG_QOSA_CLI_FUNC
 * @endcode
 *
 * @return qosa_bool_t
 *          - QOSA_TRUE when the command is registered.
 *          - QOSA_FALSE when parameters are invalid or registration storage cannot be allocated.
 */
qosa_bool_t qosa_cli_register_command(const qosa_cli_command_t *command);

/**
 * @brief Enter an interactive CLI session on the specified AT port.
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *          - Owner AT device port.
 *
 * @param[in] qosa_bool_t echo_input
 *          - Whether local input should be echoed back to the frontend.
 *
 * @param[in] qosa_cli_write_cb_t write_cb
 *          - Output callback used by the session.
 *
 * @param[in] void *write_ctx
 *          - User context passed to write_cb.
 *
 * @return qosa_bool_t
 *          - QOSA_TRUE when the session is entered.
 *          - QOSA_FALSE when another session is already active or parameters are invalid.
 */
qosa_bool_t qosa_cli_session_enter(qosa_at_dev_type_e dev_port, qosa_bool_t echo_input, qosa_cli_write_cb_t write_cb, void *write_ctx);

/**
 * @brief Exit the active CLI session if the caller owns it.
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *          - Owner AT device port.
 */
void qosa_cli_session_exit(qosa_at_dev_type_e dev_port);

/**
 * @brief Write output to the active CLI session from any context.
 *
 * Safe to call from asynchronous callbacks (e.g. lwiperf report).
 * Does nothing when no CLI session is active.
 *
 * @param[in] text   Output string.
 * @param[in] length Valid byte length of text.
 */
void qosa_cli_session_write(const char *text, qosa_size_t length);

/**
 * @brief Query whether a CLI session is active.
 *
 * @return qosa_bool_t
 *          - QOSA_TRUE when any CLI session is active.
 *          - QOSA_FALSE otherwise.
 */
qosa_bool_t qosa_cli_session_is_active(void);

/**
 * @brief Query whether the specified AT port owns the active CLI session.
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *          - AT device port to check.
 *
 * @return qosa_bool_t
 *          - QOSA_TRUE when dev_port owns the active session.
 *          - QOSA_FALSE otherwise.
 */
qosa_bool_t qosa_cli_session_is_owner(qosa_at_dev_type_e dev_port);

/**
 * @brief Get the owner AT port of the active CLI session.
 *
 * @return qosa_at_dev_type_e
 *          - Active owner port.
 *          - QOSA_DEV_NONE when no session is active.
 */
qosa_at_dev_type_e qosa_cli_session_get_dev_port(void);

/**
 * @brief Consume raw input bytes for the active CLI session.
 *
 * @param[in] qosa_uint8_t *buff
 *          - Raw input buffer from edit mode.
 *
 * @param[in] qosa_uint32_t size
 *          - Buffer length.
 *
 * @param[out] qosa_bool_t *should_exit
 *          - Set to QOSA_TRUE when the session should be closed by caller.
 *
 * @return qosa_uint32_t
 *          - Number of bytes consumed from buff.
 */
qosa_uint32_t qosa_cli_session_consume(qosa_uint8_t *buff, qosa_uint32_t size, qosa_bool_t *should_exit);

#endif /* __QOSA_CLI_H__ */
