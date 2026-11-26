/*****************************************************************/ /**
 * @file qcm_mifi_developer.h
 * @brief MiFi developer mode operation wrapper interface
 *
 * @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 **********************************************************************/

#ifndef __QCM_MIFI_DEVELOPER_H__
#define __QCM_MIFI_DEVELOPER_H__

#include "qcm_mifi_net.h"

#ifdef __cplusplus
extern "C" {
#endif

qcm_mifi_err_e qcm_mifi_developer_get_serial_port_status(qosa_uint8_t *status);
qcm_mifi_err_e qcm_mifi_developer_set_serial_port_status(qosa_uint8_t status);
qosa_bool_t qcm_mifi_developer_verify_password(const char *password);

#ifdef __cplusplus
}
#endif

#endif /* __QCM_MIFI_DEVELOPER_H__ */
