/**********************************************************************
 * @file esim_config.h
 * @brief
 * @author larson.li@quectel.com
 * @date 2024-10-21
 *
 * @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description"
 * <tr><td>2024-10-21 <td>1.0 <td>Larson.Li <td> Init
 * </table>
 **********************************************************************/
#ifndef __ESIM_CONFIG_H__
#define __ESIM_CONFIG_H__

/** Compatible with LPA interface */
#define ESIM_COMPATIBLE_LPA_INTERFACE

/** Library version */
#define ESIM_LIB_VER_STR "eSIM_IPA_v1.8.4"

/** Compile time */
#define LIB_COMPILE_TIME "20260303"

/** IPA support */
#define ESIM_IPA_SUPPORT

/** IPA indirect profile download support */
//#define ESIM_IPA_INDIRECT_PROFILE_DOWMLOAD_SUPPORT

/** SM_DP+ data format */
#ifndef ESIM_IPA_INDIRECT_PROFILE_DOWMLOAD_SUPPORT
/** Communicate with SM_DP+, data uses JSON format */
#define ESIM_SMDPP_DATA_FORMAT_JSON
#else
/** Communicate with SM_DP+, data uses ASN.1 format */
#define ESIM_SMDPP_DATA_FORMAT_ASN1
#endif /* ESIM_IPA_INDIRECT_PROFILE_DOWMLOAD_SUPPORT */

/** ESIM network communication protocol uses HTTPS */
#define ESIM_NET_PROTO_SUPPORTS_HTTPS

#ifndef ESIM_NET_PROTO_SUPPORTS_HTTPS
/** ESIM network communication protocol uses LwM2M */
#define ESIM_NET_PROTO_SUPPORTS_LWM2M
#endif /* ESIM_NET_PROTO_SUPPORTS_HTTPS */

#endif /* __UNIRTOS_ESIM_VERSION_H__ */
