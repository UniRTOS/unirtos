#ifndef _UNIRTOS_ATCMD_QINGYA_H_
#define _UNIRTOS_ATCMD_QINGYA_H_

#include "qosa_def.h"
#include "qosa_at_cmd.h"

/**
 * @brief Execute the AT+QAUTHCODE command
 *
 * @param[in] cmd Pointer to the AT command structure
 */
void qosa_exec_qauthcode_cmd(qosa_at_cmd_t *cmd);

#endif /* _UNIRTOS_ATCMD_QINGYA_H_ */