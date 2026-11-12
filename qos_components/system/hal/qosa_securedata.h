/*****************************************************************/ /**
* @file qosa_securedata.h
* @brief Secure data storage interface
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
**********************************************************************/
#ifndef __QOSA_SECUREDATA_H__
#define __QOSA_SECUREDATA_H__

#include "qosa_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QOSA_SECDATA_MAX_COUNT        35
#define QOSA_SECDATA_KEY_OP_AUTH      "OP_AUTH"
#define QOSA_SECDATA_KEY_WIFI_SSID    "WIFI_SSID"
#define QOSA_SECDATA_KEY_WIFI_SSID_5G "WIFI_SSID_5G"
#define QOSA_SECDATA_KEY_MIFI_BATTERY_SOC "MIFI_BATTERY_SOC"
#define QOSA_SECDATA_KEY_MIFI_LIMIT_SPEED "MIFI_LIMIT_SPEED"
#define QOSA_SECDATA_KEY_MIFI_NET_CFG     "MIFI_NET_CFG"
#define QOSA_SECDATA_KEY_MIFI_SIM_CFG     "MIFI_SIM_CFG"
#define QOSA_SECDATA_KEY_MIFI_SIM_INFO    "MIFI_SIM_INFO"
#define QOSA_SECDATA_KEY_MIFI_POWER_SAVER "MIFI_POWER_SAVER"
#define QOSA_SECDATA_KEY_MIFI_USB_ENUM_TYPE "USB_ENUM_TYPE"
#define QOSA_SECDATA_KEY_WIFI_MAC     "WIFI_MAC"

#define QOSA_SECDATA_READ             0
#define QOSA_SECDATA_WRITE            1

typedef enum
{
    QOSA_SECDATA_OK = 0,
    QOSA_SECDATA_ERR_PARAM = -1,
    QOSA_SECDATA_ERR_NOT_FOUND = -2,
    QOSA_SECDATA_ERR_CRC = -3,
    QOSA_SECDATA_ERR_FULL = -4,
    QOSA_SECDATA_ERR_FLASH = -5,
    QOSA_SECDATA_ERR_NOT_SUPPORTED = -6,
} qosa_securedata_ret_e;

qosa_int32_t qosa_securedata_init(void);
qosa_int32_t qosa_securedata_read(qosa_uint8_t index, qosa_uint8_t *buffer, qosa_uint32_t len);
qosa_int32_t qosa_securedata_write(qosa_uint8_t index, const qosa_uint8_t *buffer, qosa_uint32_t len);
qosa_int32_t qosa_securedata_read_by_name(const char *name, qosa_uint8_t *buffer, qosa_uint32_t len);
qosa_int32_t qosa_securedata_write_by_name(const char *name, const qosa_uint8_t *buffer, qosa_uint32_t len);
qosa_int32_t qosa_securedata_get_max_len_by_name(const char *name);
qosa_int32_t qosa_mifi_securedata_operation(qosa_int32_t operation_type, const char *name, char *buffer, qosa_int32_t len);

/* Source-side compatibility entry points for code still using MiFi securedata names. */
void ql_securedata_init(void);
qosa_int32_t ql_securedata_read(qosa_uint8_t index, qosa_uint8_t *buffer, qosa_uint32_t len);
qosa_int32_t ql_securedata_store(qosa_uint8_t index, qosa_uint8_t *buffer, qosa_uint32_t len);
qosa_int32_t quec_mifi_securedata_opration(qosa_int32_t operation_type, char *name, char *buffer, qosa_int32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __QOSA_SECUREDATA_H__ */
