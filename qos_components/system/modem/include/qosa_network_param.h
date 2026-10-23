/*****************************************************************/ /**
* @file qosa_network_param.h
* @brief Network parameters interface
* @author Joe.tu@quectel.com
* @date 2024-04-16
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-04-16 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_NETWORK_PARAM__H__
#define __QOSA_NETWORK_PARAM__H__

#include "qosa_def.h"

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"

#define QOSA_SS_RSRP_REPORT(x)    qosa_nr_rsrp_report((x))
#define QOSA_SS_RSRP_MEAS(x)      qosa_nr_rsrp_meas((x))

#define QOSA_SS_RSRQ_REPORT(x)    qosa_nr_rsrq_report((x))
#define QOSA_SS_RSRQ_MEAS(x)      qosa_nr_rsrq_meas((x))

#define QOSA_SS_SINR_REPORT(x)    qosa_nr_sinr_report((x))
#define QOSA_SS_SINR_MEAS(x)      qosa_nr_sinr_meas((x))

#define QOSA_RSRP_REPORT(x)       qosa_lte_rsrp_report((x))
#define QOSA_RSRP_MEAS(x)         qosa_lte_rsrp_meas((x))

#define QOSA_RSSI_REPORT(x)       qosa_lte_rssi_report((x))
#define QOSA_RSSI_MEAS(x)         qosa_lte_rssi_meas((x))

#define QOSA_RSRQ_REPORT(x)       qosa_lte_rsrq_report((x))
#define QOSA_RSRQ_MEAS(x)         qosa_lte_rsrq_meas((x))

#define QOSA_SINR_REPORT(x)       qosa_lte_sinr_report((x))
#define QOSA_SINR_MEAS(x)         qosa_lte_sinr_meas((x))

#define QOSA_RSCP_REPORT(x)       qosa_wcdma_rscp_report((x))
#define QOSA_RSCP_MEAS(x)         qosa_wcdma_rscp_meas((x))

#define QOSA_ECNO_REPORT(x)       qosa_wcdma_ecno_report((x))
#define QOSA_ECNO_MEAS(x)         qosa_wcdma_ecno_meas((x))

#define QOSA_SIGNAL_LEVEL_MEAS(x) (x)

/** Calculate absolute value */
#define VAL_ABS(x)                (((x) >= 0) ? (x) : (0 - (x)))

//LTE or NR 5G measurement data, note that in 5G, rsrp rsrq sinr use SS prefix conversion macros
typedef qosa_int16_t qosa_rsrp_t;   /**< reference signal received power; unit dBm; measurement value*100; range: -15600 ~ -4400 */
typedef qosa_int16_t qosa_rssi_t;   /**< received signal strength indicator; unit dBm; measurement value*100; range: -10000 ~ -2500 */
typedef qosa_int16_t qosa_rsrq_t;   /**< reference signal received quality; unit dB; measurement value*100; range: -3400 ~ -250 */
typedef qosa_int16_t qosa_sinr_t;   /**< signal to interference plus noise ratio; unit dB; measurement value*100; range: -2300 ~ 4000 */
typedef qosa_int16_t qosa_srxlev_t; /**< cell selection S value; unit dB; actual value; range -; invalid value is QOSA_NW_PI_INT16 */
typedef qosa_int16_t qosa_squal_t;  /**< cell selection signal quality; unit dB; actual value; range -; invalid value is QOSA_NW_PI_INT16 */
typedef qosa_uint16_t qosa_bler_t;  /**< Block Error Rate; measurement value*100; range 0 ~ 10000; invalid value is QOSA_NW_PI_UINT16 */

//WCDMA measurement data
//TODO: add rssi value constraints
typedef qosa_int16_t qosa_rscp_t;   /**< received signal code power; unit dBm; measurement value*100; range: -12000 ~ -2500 */
typedef qosa_int16_t qosa_ecno_t;   /**< Ec/Io: unit dB; measurement value*100; range: -2400 ~ 0 */
typedef qosa_int16_t qosa_w_rssi_t; /**< w rssi value: unit dB; measurement value*100; range: -10000 ~ -2500 */

//GSM measurement data
typedef qosa_int16_t qosa_signal_level_t; /**< received signal strength; unit dBm; measurement value; range: -113 ~ -48; invalid value is QOSA_NW_PI_INT16 */
//TODO: add signal quality data constraints
typedef qosa_int16_t qosa_ber_t; /**< bit error rate; unit: none; report value; range: 0~7, 99 means unknown */

/**
 * @struct qosa_plmn_t
 * @brief the struct of plmn, example: "46001" mcc=460, mnc=01, mnc_digit_num=2
 *
 */
typedef struct plmn
{
    qosa_uint16_t mcc;           /*!< mobile country code */
    qosa_uint16_t mnc;           /*!< mobile network code */
    qosa_uint8_t  mnc_digit_num; /*!< digit num of mnc */
} qosa_plmn_t;

/**
 * @struct qosa_oper_info_t
 * @brief Mobile country code and operator information
 *
 */
typedef struct
{
    const char *plmn;       /*!< Mobile country code */
    const char *long_name;  /*!< Operator long name */
    const char *short_name; /*!< Operator short name */
} qosa_oper_info_t;

/**
 * @struct qosa_mcc_oper_map_t
 * @brief Mobile country code and operator information mapping table
 *
 */
typedef struct
{
    qosa_uint16_t     mcc;            /*!< Mobile country code */
    qosa_oper_info_t *oper_list;      /*!< Operator information list */
    qosa_uint32_t     oper_list_size; /*!< Operator information list size */
} qosa_mcc_oper_map_t;

/**
 * @struct qosa_reject_cause_mapping_t
 * @brief Reject cause mapping table
 *
 */
typedef struct
{
    qosa_uint16_t cause; /*!< Reject cause */
    const char   *text;  /*!< Reject cause text */
} qosa_reject_cause_mapping_t;

int qosa_nr_rsrp_meas(qosa_rsrp_t x);
int qosa_nr_rsrp_report(qosa_rsrp_t x);
int qosa_nr_rsrq_meas(qosa_rsrq_t x);
int qosa_nr_rsrq_report(qosa_rsrq_t x);
int qosa_nr_sinr_meas(qosa_sinr_t x);
int qosa_nr_sinr_report(qosa_sinr_t x);
int qosa_lte_rsrp_meas(qosa_rsrp_t x);
int qosa_lte_rsrp_report(qosa_rsrp_t x);
int qosa_lte_rsrq_meas(qosa_rsrq_t x);
int qosa_lte_rsrq_report(qosa_rsrq_t x);
int qosa_lte_rssi_meas(qosa_rssi_t x);
int qosa_lte_rssi_report(qosa_rssi_t x);
int qosa_lte_sinr_meas(qosa_sinr_t x);
int qosa_lte_sinr_report(qosa_sinr_t x);
int qosa_wcdma_rscp_meas(qosa_rscp_t x);
int qosa_wcdma_rscp_report(qosa_rscp_t x);
int qosa_wcdma_ecno_meas(qosa_ecno_t x);
int qosa_wcdma_ecno_report(qosa_ecno_t x);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_NETWORK_PARAM__H__ */
