/*****************************************************************/ /**
* @file qosa_clat.h
* @brief qosa clat interface definitions
* @author John.ou@quectel.com
* @date 2026-03-24
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2026-03-24 <td>1.0 <td>John.ou <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_CLAT__H__
#define __QOSA_CLAT__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"
#include "qosa_at_param.h"
#include "qosa_ip_addr.h"
#include "qosa_nvitem.h"

#define QOSA_CLAT_FQDN_LEN 64

/**
 * @enum qosa_clat_errcode_e
 * @brief CLAT error code
 */
typedef enum
{
    QOSA_CLAT_OK = 0,
    QOSA_CLAT_ERROR_INVALID_PARAM = 1,
    QOSA_CLAT_ERROR_MEMORY = 2,
    QOSA_CLAT_ERROR_UNKNOWN = 3,
} qosa_clat_errcode_e;

/**
 * @enum qosa_clat_option_e
 * @brief CLAT option
 */
typedef enum
{
    QOSA_CLAT_OPT_PROFILEID, /*!< config profile ID */
    QOSA_CLAT_OPT_SIMCID,    /*!< SIM ID */
    QOSA_CLAT_OPT_PDPTYPE,   /*!< PDP type */

    QOSA_CLAT_OPT_STATUS,           /*!< CLAT status */
    QOSA_CLAT_OPT_DNS64_MODE,       /*!< DNS64 mode */
    QOSA_CLAT_OPT_DEST_PREFIX,      /*!< Dest ip6 prefix */
    QOSA_CLAT_OPT_PREFIX_LEN,       /*!< Dest ip6 prefix length */
    QOSA_CLAT_OPT_FQDN_DNS64,       /*!< FQDN */
    QOSA_CLAT_OPT_UL_IGNORE_V4_TOS, /*!< Ignores V4 TOS in uplink */
    QOSA_CLAT_OPT_DL_V4_TOS,        /*!< V4 TOS in downlink */
    QOSA_CLAT_OPT_NO_DF,            /*!< Not set DF bit */
    QOSA_CLAT_OPT_UDP_ZERO_CKSUM,   /*!< UDP zero checksum */
    QOSA_CLAT_OPT_ICMPV4_ERR_MODE,  /*!< ICMPv4 error mode */
    QOSA_CLAT_OPT_ICMPV6_ERR_MODE,  /*!< ICMPv6 error mode */
} qosa_clat_option_e;

/**
 * @struct qosa_clat_config
 * @brief CLAT config
 */
typedef struct qosa_clat_config
{
    qosa_uint8_t   simcid;
    qosa_bool_t    clat_switch;              /*!< CLAT Enable */
    qosa_uint8_t   dns64_mode;               /*!< DNS64 mode */
    qosa_ip_addr_t dst_prefix;               /*!< Destination ip6 prefix */
    qosa_uint8_t   dest_prefix_len;          /*!< Destination ip6 prefix length */
    char           fqdn[QOSA_CLAT_FQDN_LEN]; /*!< Fully qualified domain name */
    qosa_bool_t    ul_ignore_v4_tos;         /*!< Ignores V4 TOS in uplink */
    qosa_uint8_t   dl_v4_tos;                /*!< V4 TOS in downlink */
    qosa_bool_t    no_df;                    /*!< Not set DF bit */
    qosa_bool_t    udp_zero_cksum;           /*!< UDP zero checksum */
    qosa_uint8_t   icmpv4_err_mode;          /*!< ICMPv4 error mode */
    qosa_uint8_t   icmpv6_err_mode;          /*!< ICMPv6 error mode */
} qosa_clat_config_t;

/**
 * @brief Get CLAT config
 *
 * @param [in] profile_id
 *           - profile ID
 * @param [out] cfg
 *           - CLAT config
 * @return qosa_clat_errcode_e
 *           - QOSA_CLAT_OK: success
 *           - Other: fail
 */
qosa_clat_errcode_e qosa_clat_get_cfg(int profile_id, qosa_clat_config_t *cfg);

/**
 * @brief Set CLAT config
 *
 * @param [in] profile_id
 *           - profile ID
 * @param [in] cfg
 *           - CLAT config
 * @return qosa_clat_errcode_e
 *           - QOSA_CLAT_OK: success
 *           - Other: fail
 */
qosa_clat_errcode_e qosa_clat_set_cfg(int profile_id, qosa_clat_config_t *cfg);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* _QOSA_CLAT_H */
