/*****************************************************************/ /**
* @file qosa_network.h
* @brief Network operation interface
* @author Joe.tu@quectel.com
* @date 2024-03-14
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-03-14 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_NETWORK_H__
#define __QOSA_NETWORK_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_modem.h"
#include "qosa_network_config.h"
#include "qosa_network_param.h"

/** Maximum length of network name */
#define QOSA_NW_NETWORK_NAME_LEN_MAX        (50 + 1)

/** Maximum length of PLMN ID is 6 digits */
#define QOSA_NW_PLMN_STRLEN_MAX             (6 + 1)

/** Whether MNC of plmn is 2 digits */
#define QOSA_NW_PLMN_IS_2_DIGIT_MNC(p_plmn) (((p_plmn)->mnc_digit_num == 2) ? QOSA_TRUE : QOSA_FALSE)

/** Whether plmn is valid */
#define QOSA_NW_PLMN_IS_VALID(p_plmn)                                                                                                                          \
    ((((p_plmn)->mnc_digit_num == 2) || ((p_plmn)->mnc_digit_num == 3)) && (((p_plmn)->mcc != 0xffff) && ((p_plmn)->mnc != 0xffff)) && ((p_plmn)->mcc != 0))

/** Whether plmn is the same */
#define QOSA_NW_PLMN_COMP(a, b)  (((a)->mcc == (b)->mcc) && ((a)->mnc == (b)->mnc) && ((a)->mnc_digit_num == (b)->mnc_digit_num))

/** Determine whether the current UE has successfully registered to the cell */
#define QOSA_NW_ATTACHED(status) ((status == QOSA_NW_REG_HOME) ? QOSA_TRUE : (status == QOSA_NW_REG_ROAMING ? QOSA_TRUE : QOSA_FALSE))

#ifdef CONFIG_QOSA_NW_GSM_SUPPORT
#define QOSA_AUTO_CONTAIN_GSM (QOSA_NW_RAT_2G)
#else
#define QOSA_AUTO_CONTAIN_GSM (0)
#endif
#ifdef CONFIG_QOSA_NW_WCDMA_SUPPORT
#define QOSA_AUTO_CONTAIN_WCDMA (QOSA_NW_RAT_3G)
#else
#define QOSA_AUTO_CONTAIN_WCDMA (0)
#endif
#ifdef CONFIG_QOSA_NW_LTE_SUPPORT
#define QOSA_AUTO_CONTAIN_LTE (QOSA_NW_RAT_4G)
#else
#define QOSA_AUTO_CONTAIN_LTE (0)
#endif
#ifdef CONFIG_QOSA_NW_NR_SUPPORT
#define QOSA_AUTO_CONTAIN_NR5G (QOSA_NW_RAT_5G)
#else
#define QOSA_AUTO_CONTAIN_NR5G (0)
#endif

/** When setting rat mode, QOSA_NW_RAT_ALL can represent all rat */
#define QOSA_NW_RAT_ALL                (QOSA_AUTO_CONTAIN_GSM | QOSA_AUTO_CONTAIN_WCDMA | QOSA_AUTO_CONTAIN_LTE | QOSA_AUTO_CONTAIN_NR5G)

/** Index definition for qosa_nw_band_t.ltebandval, representing LTE band1 to LTE band32 */
#define QOSA_NW_LTE_BAND_IDX_1_32      (0)

/** Index definition for qosa_nw_band_t.ltebandval, representing LTE band33 to LTE band64 */
#define QOSA_NW_LTE_BAND_IDX_33_64     (1)

/** Index definition for qosa_nw_band_t.ltebandval, representing LTE band65 to LTE band96 */
#define QOSA_NW_LTE_BAND_IDX_65_96     (2)

/** Use single BIT to represent rat */
#define QOSA_NW_RAT_BIT_NONE_0         (0) /*!< Reserved bit 0 */
#define QOSA_NW_RAT_BIT_NONE_1         (1) /*!< Reserved bit 1 */
#define QOSA_NW_RAT_BIT_GSM            (2) /*!< GSM RAT bit */
#define QOSA_NW_RAT_BIT_WCDMA          (3) /*!< WCDMA RAT bit */
#define QOSA_NW_RAT_BIT_LTE            (4) /*!< LTE RAT bit */
#define QOSA_NW_RAT_BIT_NR5G           (5) /*!< NR5G RAT bit */

/** qosa_nw_param_invalid_xxx Invalid parameter values */
#define QOSA_NW_PI_UINT64              (0xFFFFFFFFFFFFFFFF)
#define QOSA_NW_PI_UINT32              (0xFFFFFFFF)
#define QOSA_NW_PI_UINT16              (0xFFFF)
#define QOSA_NW_PI_UINT8               (0xFF)
#define QOSA_NW_PI_INT32               (0x7FFFFFFF)
#define QOSA_NW_PI_INT16               (0x7FFF)
#define QOSA_NW_PI_INT8                (0x7F)
#define QOSA_NW_PI_BOOL                (0x7F)

/** Maximum number of cells of the same type in neighbor cell information query */
#define QOSA_NW_CELL_INFO_NUM_MAX      (7)

/** Maximum number of cells of the same type in frequency point scanning */
#define QOSA_NW_FREQ_SCAN_CELL_NUM_MAX (10)

/** Number of supported operator lists */
#define QOSA_NW_SUPP_OPER_NUM          (10)

/** rat order list size */
#define QOSA_NW_RAT_SEQ_NUM            (8)

/** Invalid PLMN value */
#define QOSA_NW_PLMN_INVALID                                                                                                                                   \
    {                                                                                                                                                          \
        0xffff, 0xffff, 0xff                                                                                                                                   \
    }

/** Number of frequency points locked */
#define QOSA_NW_LOCK_FREQ_NUM_MAX            (10)

/** Neighbor cell scanning behavior control: whether to scan specific RAT and whether to scan specific band */
#define QOSA_NW_NCELL_EXT_OP_SCAN_GSM        (0x0001)
#define QOSA_NW_NCELL_EXT_OP_SCAN_WCDMA      (0x0002)
#define QOSA_NW_NCELL_EXT_OP_SCAN_LTE        (0x0004)
#define QOSA_NW_NCELL_EXT_OP_SCAN_NR5G       (0x0008)
#define QOSA_NW_NCELL_EXT_OP_SCAN_GSM_BAND   (0x0100)
#define QOSA_NW_NCELL_EXT_OP_SCAN_WCDMA_BAND (0x0200)
#define QOSA_NW_NCELL_EXT_OP_SCAN_LTE_BAND   (0x0400)
#define QOSA_NW_NCELL_EXT_OP_SCAN_NR5G_BAND  (0x0800)

/** Neighbor cell scanning behavior control: default scan all supported RAT, default scan all band, or default behavior defined by platform side */
#define QOSA_NW_NCELL_EXT_OP_DEFAULT         (0)

/** Frequency scanning behavior control: whether to scan specific RAT and whether to scan specific band */
#define QOSA_NW_FREQ_SCAN_OP_SCAN_GSM        (0x0001)
#define QOSA_NW_FREQ_SCAN_OP_SCAN_WCDMA      (0x0002)
#define QOSA_NW_FREQ_SCAN_OP_SCAN_LTE        (0x0004)
#define QOSA_NW_FREQ_SCAN_OP_SCAN_NR5G       (0x0008)

/** Frequency scanning behavior control: default scan all supported RAT, default scan all band, or default behavior defined by platform side */
#define QOSA_NW_FREQ_SCAN_OP_DEFAULT         (0)

/** S-NSSAI maximum length */
#define QOSA_S_NSSAI_MAX_LEN                 (20)

/** NSSAI string maximum length */
#define QOSA_NSSAI_MAX_LEN                   (80)

typedef void (*nw_callback_t)(void *ctx, void *argv);

#define QOSA_ERRCODE_NW_BASE            (QOSA_COMPONENT_NETWORK << 16)

/** Default value for shakeperiod for Jamming detection */
#define QOSA_JD_SHAKEPERIOD_VAL_VAL_DEF CONFIG_UNIRTOS_JAMM_DETECT_SHAKEPERIOD_DEFAULT
/** Minimum value for rssi for Jamming detection */
#define QOSA_JD_RSSI_VAL_MIN            CONFIG_UNIRTOS_JAMM_DETECT_RSSI_MIN
/** Maximum value for rssi for Jamming detection */
#define QOSA_JD_RSSI_VAL_MAX            CONFIG_UNIRTOS_JAMM_DETECT_RSSI_MAX
/** Default value for rssi for Jamming detection */
#define QOSA_JD_RSSI_VAL_VAL_DEF        CONFIG_UNIRTOS_JAMM_DETECT_RSSI_DEFAULT
/** Minimum value for rsrq for Jamming detection */
#define QOSA_JD_RSRQ_VAL_MIN            CONFIG_UNIRTOS_JAMM_DETECT_RSRQ_MIN
/** Maximum value for rsrq for Jamming detection */
#define QOSA_JD_RSRQ_VAL_MAX            CONFIG_UNIRTOS_JAMM_DETECT_RSRQ_MAX
/** Default value for rsrq for Jamming detection */
#define QOSA_JD_RSRQ_VAL_VAL_DEF        CONFIG_UNIRTOS_JAMM_DETECT_RSRQ_DEFAULT
/** Default value for rsrp for Jamming detection */
#define QOSA_JD_RSRP_VAL_VAL_DEF        CONFIG_UNIRTOS_JAMM_DETECT_RSRP_DEFAULT
/** Minimum value for sinr for Jamming detection */
#define QOSA_JD_SINR_VAL_MIN            CONFIG_UNIRTOS_JAMM_DETECT_SINR_MIN
/** Maximum value for sinr for Jamming detection */
#define QOSA_JD_SINR_VAL_MAX            CONFIG_UNIRTOS_JAMM_DETECT_SINR_MAX
/** Default value for sinr for Jamming detection */
#define QOSA_JD_SINR_VAL_VAL_DEF        CONFIG_UNIRTOS_JAMM_DETECT_SINR_DEFAULT

/** Max num of blacklist scene */
#define QOSA_NW_QBLACKLIST_SCENE_MAX 8

// clang-format off
/**
 * @enum qosa_nw_err_e
 * @brief network error codes
 *
 */
typedef enum nw_err
{
    QOSA_NW_ERR_OK = 0,

    QOSA_NW_ERR_OPERATION_NOT_ALLOWED   = 3 | QOSA_ERRCODE_NW_BASE, /*!< operation not allowed */
    QOSA_NW_ERR_OPERATION_NOT_SUPPORTED = 4 | QOSA_ERRCODE_NW_BASE, /*!< operation not supported */
    QOSA_NW_ERR_MEMORY_FULL             = 20 | QOSA_ERRCODE_NW_BASE, /*!< memory full */
    QOSA_NW_ERR_MEMORY_FAILURE          = 23 | QOSA_ERRCODE_NW_BASE, /*!< memory failure */
    QOSA_NW_ERR_INVALID_PARAM           = 53 | QOSA_ERRCODE_NW_BASE, /*!< invalid parameter */
    QOSA_NW_ERR_EXECUTE                 = 1 | (QOSA_ERRCODE_NW_BASE + QOSA_AT_ERR_OFS),
} qosa_nw_err_e;
// clang-format on

/**
 * @enum qosa_nw_reg_status_e
 * @brief UE registration status
 *
 */
typedef enum
{
    QOSA_NW_NOT_REG = 0,           /*!< Not registered, ME is currently not searching for operators to register with */
    QOSA_NW_REG_HOME = 1,          /*!< Registered, home network */
    QOSA_NW_NOT_REG_SEARCHING = 2, /*!< Not registered, ME is currently searching for operators to register with */
    QOSA_NW_REG_DENIED = 3,        /*!< Registration denied */
    QOSA_NW_REG_UNKNOWN = 4,       /*!< Unknown status */
    QOSA_NW_REG_ROAMING = 5,       /*!< Registered, roaming network */
#if 0
    QOSA_NW_REG_SMS_ONLY_HOME,
    QOSA_NW_REG_SMS_ONLY_ROAMING,
    QOSA_NW_REG_EMERGENCY,
    QOSA_NW_REG_CSFB_NOT_PREFERRED_HOME,
    QOSA_NW_REG_CSFB_NOT_PREFERRED_ROAMING,
#endif
} qosa_nw_reg_status_e;

/**
 * @enum qosa_nw_act_e
 * @brief Represents network layer access technology
 *
 */
typedef enum
{
    QOSA_NW_ACT_GSM = 0,                        /*!< GSM */
    QOSA_NW_ACT_GSM_COMPACT,                    /*!< GSM compact */
    QOSA_NW_ACT_UTRAN,                          /*!< UTRAN */
    QOSA_NW_ACT_GSM_EGPRS,                      /*!< gsm w/EGPRS */
    QOSA_NW_ACT_UTRAN_HSDPA,                    /*!< UTRAN w/HSDPA  */
    QOSA_NW_ACT_UTRAN_HSUPA,                    /*!< UTRAN w/HSUPA  */
    QOSA_NW_ACT_UTRAN_HSPA,                     /*!< UTRAN w/HSDPA and w/HSUPA */
    QOSA_NW_ACT_EUTRAN = 7,                     /*!< EUTRAN */
    QOSA_NW_ACT_EC_GSM_IOT,                     /*!< EC-GSM-IoT (A/Gb mode) */
    QOSA_NW_ACT_EUTRAN_NB_S1_MODE,              /*!< E-UTRAN (NB-S1 mode) */
    QOSA_NW_ACT_EUTRA_CONNECTED_TO_A_5GCN = 10, /*!< E-UTRA connected to a 5GCN */
    QOSA_NW_ACT_NR_CONNECTED_TO_A_5GCN,         /*!< NR connected to a 5GCN */
    QOSA_NW_ACT_NG_RAN,                         /*!< NG-RAN */
    QOSA_NW_ACT_EUTRA_NR_DUAL_CONNECTIVITY,     /*!< E-UTRA-NR dual connectivity */
    QOSA_NW_ACT_UNKNOWN = 0xff,                 /*!< Unknown act */
} qosa_nw_act_e;

/**
 * @enum qosa_nw_rat_e
 * @brief Represents rat, uses bit to represent rat information, can be used in combination when setting rat mode
 *
 */
typedef enum
{
    QOSA_NW_RAT_UNKNOWN,
    QOSA_NW_RAT_2G = 1 << QOSA_NW_RAT_BIT_GSM,   /*!< RAT: GSM */
    QOSA_NW_RAT_3G = 1 << QOSA_NW_RAT_BIT_WCDMA, /*!< RAT: WCDMA */
    QOSA_NW_RAT_4G = 1 << QOSA_NW_RAT_BIT_LTE,   /*!< RAT: LTE */
    QOSA_NW_RAT_5G = 1 << QOSA_NW_RAT_BIT_NR5G,  /*!< RAT: NR5G */
} qosa_nw_rat_e;

/**
 * @enum qosa_nw_band_gsm_e
 * @brief Band under GSM
 *
 */
typedef enum
{
    QOSA_NW_BAND_GSM_900 = 0x01,  /*!< Band 8 */
    QOSA_NW_BAND_GSM_1800 = 0x02, /*!< Band 3 */
    QOSA_NW_BAND_GSM_850 = 0x04,  /*!< Band 5 */
    QOSA_NW_BAND_GSM_1900 = 0x08, /*!< Band 2 */
} qosa_nw_band_gsm_e;

/**
 * @enum qosa_nw_band_wcdma_e
 * @brief Band under WCDMA
 *
 */
typedef enum
{
    QOSA_NW_BAND_WCDMA_2100 = 0x10,  /*!< Band 1 */
    QOSA_NW_BAND_WCDMA_1900 = 0x20,  /*!< Band 2 */
    QOSA_NW_BAND_WCDMA_850 = 0x40,   /*!< Band 5 */
    QOSA_NW_BAND_WCDMA_900 = 0x80,   /*!< Band 8 */
    QOSA_NW_BAND_WCDMA_800 = 0x100,  /*!< Band 6 */
    QOSA_NW_BAND_WCDMA_1700 = 0x200, /*!< Band 4 */
} qosa_nw_band_wcdma_e;

/**
 * @enum qosa_nw_band_lte_e
 * @brief Band under LTE
 *
 */
typedef enum
{
    QOSA_NW_BAND_LTE_BAND1 = 1,   /*!< Band 1 */
    QOSA_NW_BAND_LTE_BAND2 = 2,   /*!< Band2 */
    QOSA_NW_BAND_LTE_BAND3 = 3,   /*!< Band3 */
    QOSA_NW_BAND_LTE_BAND4 = 4,   /*!< Band4 */
    QOSA_NW_BAND_LTE_BAND5 = 5,   /*!< Band5 */
    QOSA_NW_BAND_LTE_BAND7 = 7,   /*!< Band7 */
    QOSA_NW_BAND_LTE_BAND8 = 8,   /*!< Band8 */
    QOSA_NW_BAND_LTE_BAND12 = 12, /*!< Band12 */
    QOSA_NW_BAND_LTE_BAND13 = 13, /*!< Band13 */
    QOSA_NW_BAND_LTE_BAND14 = 14, /*!< Band14 */
    QOSA_NW_BAND_LTE_BAND17 = 17, /*!< Band17 */
    QOSA_NW_BAND_LTE_BAND18 = 18, /*!< Band18 */
    QOSA_NW_BAND_LTE_BAND19 = 19, /*!< Band19 */
    QOSA_NW_BAND_LTE_BAND20 = 20, /*!< Band20 */
    QOSA_NW_BAND_LTE_BAND25 = 25, /*!< Band25 */
    QOSA_NW_BAND_LTE_BAND26 = 26, /*!< Band26 */
    QOSA_NW_BAND_LTE_BAND28 = 28, /*!< Band28 */
    QOSA_NW_BAND_LTE_BAND30 = 30, /*!< Band30 */
    QOSA_NW_BAND_LTE_BAND31 = 31, /*!< Band31 */
    QOSA_NW_BAND_LTE_BAND33 = 33, /*!< Band33 */
    QOSA_NW_BAND_LTE_BAND34 = 34, /*!< Band34 */
    QOSA_NW_BAND_LTE_BAND38 = 38, /*!< Band38 */
    QOSA_NW_BAND_LTE_BAND39 = 39, /*!< Band39 */
    QOSA_NW_BAND_LTE_BAND40 = 40, /*!< Band40 */
    QOSA_NW_BAND_LTE_BAND41 = 41, /*!< Band41 */
    QOSA_NW_BAND_LTE_BAND42 = 42, /*!< Band42 */
    QOSA_NW_BAND_LTE_BAND43 = 43, /*!< Band43 */
    QOSA_NW_BAND_LTE_BAND48 = 48, /*!< Band48 */
    QOSA_NW_BAND_LTE_BAND65 = 65, /*!< Band65 */
    QOSA_NW_BAND_LTE_BAND66 = 66, /*!< Band66 */
    QOSA_NW_BAND_LTE_BAND71 = 71, /*!< Band71 */
    QOSA_NW_BAND_LTE_BAND72 = 72, /*!< Band72 */
} qosa_nw_band_lte_e;

/**
 * @enum qosa_nw_band_nr_e
 * @brief Band under NR
 *
 */
typedef enum
{
    QOSA_NW_BAND_NR_BAND1 = 1,   /*!< Band 1 */
    QOSA_NW_BAND_NR_BAND2 = 2,   /*!< Band2 */
    QOSA_NW_BAND_NR_BAND3 = 3,   /*!< Band3 */
    QOSA_NW_BAND_NR_BAND4 = 4,   /*!< Band4 */
    QOSA_NW_BAND_NR_BAND5 = 5,   /*!< Band5 */
    QOSA_NW_BAND_NR_BAND7 = 7,   /*!< Band7 */
    QOSA_NW_BAND_NR_BAND8 = 8,   /*!< Band8 */
    QOSA_NW_BAND_NR_BAND12 = 12, /*!< Band12 */
    QOSA_NW_BAND_NR_BAND13 = 13, /*!< Band13 */
    QOSA_NW_BAND_NR_BAND14 = 14, /*!< Band14 */
    QOSA_NW_BAND_NR_BAND17 = 17, /*!< Band17 */
    QOSA_NW_BAND_NR_BAND18 = 18, /*!< Band18 */
    QOSA_NW_BAND_NR_BAND19 = 19, /*!< Band19 */
    QOSA_NW_BAND_NR_BAND20 = 20, /*!< Band20 */
    QOSA_NW_BAND_NR_BAND25 = 25, /*!< Band25 */
    QOSA_NW_BAND_NR_BAND26 = 26, /*!< Band26 */
    QOSA_NW_BAND_NR_BAND28 = 28, /*!< Band28 */
    QOSA_NW_BAND_NR_BAND30 = 30, /*!< Band30 */
    QOSA_NW_BAND_NR_BAND31 = 31, /*!< Band31 */
    QOSA_NW_BAND_NR_BAND33 = 33, /*!< Band33 */
    QOSA_NW_BAND_NR_BAND34 = 34, /*!< Band34 */
    QOSA_NW_BAND_NR_BAND38 = 38, /*!< Band38 */
    QOSA_NW_BAND_NR_BAND39 = 39, /*!< Band39 */
    QOSA_NW_BAND_NR_BAND40 = 40, /*!< Band40 */
    QOSA_NW_BAND_NR_BAND41 = 41, /*!< Band41 */
    QOSA_NW_BAND_NR_BAND48 = 48, /*!< Band48 */
    QOSA_NW_BAND_NR_BAND65 = 65, /*!< Band65 */
    QOSA_NW_BAND_NR_BAND66 = 66, /*!< Band66 */
    QOSA_NW_BAND_NR_BAND71 = 71, /*!< Band71 */
    QOSA_NW_BAND_NR_BAND72 = 72, /*!< Band72 */
    QOSA_NW_BAND_NR_BAND75 = 75, /*!< Band75 */
    QOSA_NW_BAND_NR_BAND76 = 76, /*!< Band76 */
    QOSA_NW_BAND_NR_BAND77 = 77, /*!< Band77 */
    QOSA_NW_BAND_NR_BAND78 = 78, /*!< Band78 */
    QOSA_NW_BAND_NR_BAND79 = 79, /*!< Band79 */
    QOSA_NW_BAND_NR_BAND90 = 90, /*!< Band90 */
    QOSA_NW_BAND_NR_BAND91 = 91, /*!< Band91 */
    QOSA_NW_BAND_NR_BAND92 = 92, /*!< Band92 */
    QOSA_NW_BAND_NR_BAND93 = 93, /*!< Band93 */
    QOSA_NW_BAND_NR_BAND94 = 94, /*!< Band94 */
} qosa_nw_band_nr_e;

/**
 * @enum qosa_nw_oper_stat_e
 * @brief Operator status
 *
 */
typedef enum
{
    QOSA_NW_OPER_STAT_UNKNOWN,   /*!< unknown operator */
    QOSA_NW_OPER_STAT_AVAILABLE, /*!< available operator */
    QOSA_NW_OPER_STAT_CURRENT,   /*!< current operator */
    QOSA_NW_OPER_STAT_FORBIDDEN, /*!< forbidden operator */
} qosa_nw_oper_stat_e;

typedef enum
{
    QOSA_OPER_NAME_FORMAT_LONG,    /*!< Long string format */
    QOSA_OPER_NAME_FORMAT_SHORT,   /*!< Short string format */
    QOSA_OPER_NAME_FORMAT_NUMERIC, /*!< Numeric format, plmn value */
    QOSA_OPER_NAME_FORMAT_UNKNOWN, /*!< Unknown format */
} qosa_nw_oper_name_format_e;

/**
 * @enum qosa_nw_rrc_conn_mode_e
 * @brief RRC connection mode
 *
 */
typedef enum
{
    QOSA_NW_RRC_CONN_MODE_IDLE,      /*!< RRC connection idle status */
    QOSA_NW_RRC_CONN_MODE_CONNECTED, /*!< RRC connection established */
} qosa_nw_rrc_conn_mode_e;

/**
 * @enum qosa_nw_rrc_state_e
 * @brief RRC state
 *
 */
typedef enum
{
    QOSA_NW_RRC_UTRAN_URA_PCH_STATE,
    QOSA_NW_RRC_UTRAN_CELL_PCH_STATE,
    QOSA_NW_RRC_UTRAN_CELL_FACH_STATE,
    QOSA_NW_RRC_UTRAN_CELL_DCH_STATE,
    QOSA_NW_RRC_GERAN_CS_CONNECTED_STATE,
    QOSA_NW_RRC_GERAN_PS_CONNECTED_STATE,
    QOSA_NW_RRC_GERAN_CS_AND_PS_CONNECTED_STATE,
    QOSA_NW_RRC_EUTRAN_CONNECTED_STATE,
} qosa_nw_rrc_state_e;

/**
 * @enum qosa_nw_rrc_access_type_e
 * @brief RRC access type
 *
 */
typedef enum
{
    QOSA_NW_RRC_ACCESS_TYPE_GERAN,      /*!< GERAN */
    QOSA_NW_RRC_ACCESS_TYPE_UTRAN_TDD,  /*!< UTRAN TDD */
    QOSA_NW_RRC_ACCESS_TYPE_UTRAN_FDD,  /*!< UTRAN FDD */
    QOSA_NW_RRC_ACCESS_TYPE_EUTRAN_TDD, /*!< EUTRAN TDD */
    QOSA_NW_RRC_ACCESS_TYPE_EUTRAN_FDD, /*!< EUTRAN FDD */
} qosa_nw_rrc_access_type_e;

/**
 * @enum qosa_nw_srv_domain_e
 * @brief Packet domain/Circuit domain registration
 *
 */
typedef enum
{
    QOSA_NW_SRV_DOMAIN_CS,    /*!< CS domain */
    QOSA_NW_SRV_DOMAIN_PS,    /*!< PS domain */
    QOSA_NW_SRV_DOMAIN_CS_PS, /*!< CS and PS domain */
} qosa_nw_srv_domain_e;

/**
 * @enum qosa_nw_voice_domain_e
 * @brief Voice domain registration
 *
 */
typedef enum
{
    QOSA_NW_VOICE_DOMAIN_CS,              /*!< CS domain only */
    QOSA_NW_VOICE_DOMAIN_IMS_PS,          /*!< IMS PS domain only */
    QOSA_NW_VOICE_DOMAIN_CS_PREFERED,     /*!< CS domain preferred */
    QOSA_NW_VOICE_DOMAIN_IMS_PS_PREFERED, /*!< IMS PS domain preferred */
} qosa_nw_voice_domain_e;

/**
 * @enum qosa_nw_ue_usage_setting_e
 * @brief UE's usage setting
 *
 */
typedef enum
{
    QOSA_NW_UE_USAGE_SETTING_VOICE_CENTRIC, /*!< Voice centric mode */
    QOSA_NW_UE_USAGE_SETTING_DATA_CENTRIC,  /*!< Data centric mode */
    QOSA_NW_UE_USAGE_SETTING_MAX,           /*!< Maximum value */
} qosa_nw_ue_usage_setting_e;

/**
 * @enum qosa_nw_roaming_pref_e
 * @brief Roaming preference setting
 *
 */
typedef enum
{
    QOSA_NW_ROAMING_DISABLE = 0,               /*!< Disable roaming function */
    QOSA_NW_ROAMING_ONLY_ON_HOME_NETWORKS = 1, /*!< Only roam on home networks */
    QOSA_NW_ROAMING_ON_AFFILIATE_NETWORKS = 3, /*!< Roam on affiliate networks */
    QOSA_NW_ROAMING_ON_ANY_NETWORK = 255,      /*!< Roam on any network */
} qosa_nw_roaming_pref_e;

/**
 * @enum qosa_nw_freq_lock_opcode_e
 * @brief Network frequency band lock/unlock operation code
 *
 */
typedef enum
{
    QOSA_NW_LOCK_FREQ_OPCODE_UNLOCK,    /*!< Unlock frequency band */
    QOSA_NW_LOCK_FREQ_OPCODE_LOCK_FREQ, /*!< Lock frequency band */
    QOSA_NW_LOCK_FREQ_OPCODE_LOCK_CELL, /*!< Lock physical cell cell */
} qosa_nw_freq_lock_opcode_e;

/**
 * @enum qosa_nw_nssai_type_e
 * @brief NSSAI configuration type
 */
typedef enum
{
    QOSA_NW_S_NSSAI_DEFAULT,                     /*!< Default NSSAI configuration */
    QOSA_NW_S_NSSAI_DEFAULT_REJECT,              /*!< Default NSSAI configuration rejected */
    QOSA_NW_S_NSSAI_DEFAULT_REJECT_CONFIG,       /*!< Default NSSAI configuration rejected configuration */
    QOSA_NW_S_NSSAI_DEFAULT_REJECT_CONFIG_ALLOW, /*!< Default NSSAI configuration rejected configuration allow */
} qosa_nw_nssai_type_e;

/**
 * @enum qosa_nw_nas_event_type_e
 * @brief NAS event type
 */
typedef enum
{
    QOSA_NW_NAS_RRC_REL_DURING_ATTACH = 0, /*!< RRC release, without ATTACH ACCEPT/REJECT */
    QOSA_NW_NAS_RRC_REL_DURING_TAU,        /*!< RRC release, without TAU ACCEPT/REJECT */
    QOSA_NW_NAS_NO_RSP_DURING_ATTACH,      /*!< No response for ATTACH REQUEST */
    QOSA_NW_NAS_NO_RSP_DURING_TAU,         /*!< No response for TAU REQUEST */
    QOSA_NW_NAS_NO_RSP_DURING_SR,          /*!< No response for SERVICE REQUEST */
    QOSA_NW_NAS_NO_RSP_DURING_DETACH,      /*!< No response for DETACH REQUEST */
    QOSA_NW_NAS_ATTACH_REJECT,             /*!< ATTACH REJECT */
    QOSA_NW_NAS_TAU_REJECT,                /*!< TAU REJECT */
    QOSA_NW_NAS_SERVICE_REJECT,            /*!< SERVICE REJECT */
    QOSA_NW_NAS_AUTHENTICATION_REJECT,     /*!< AUTHENTICATION REJECT */
    QOSA_NW_NAS_ATTACH_TIMEOUT,            /*!< ATTACH TIMEOUT */
    QOSA_NW_NAS_TAU_TIMEOUT,               /*!< TAU TIMEOUT */
    QOSA_NW_NAS_LTE_RESELECTION_SUC,       /*!< LTE reselection successfully */
    QOSA_NW_NAS_LTE_RRC_CONN_SETUP_NOT_RECEIVED,    /*!< No RRC connection setup received */
    QOSA_NW_NAS_EVENT_MAX,
} qosa_nw_nas_event_type_e;

/**
 * @enum qosa_nw_as_event_type_e
 * @brief AS event type
 */
typedef enum
{
    QOSA_NW_AS_RACH_FAIL = 0,               /*!< RACH fail */
    QOSA_NW_AS_RRC_ESTABLISH_FAIL,          /*!< Reject by network,when establish RRC */
    QOSA_NW_AS_DL_LOSS_SYNC,                /*!< DL loss sync */
} qosa_nw_as_event_type_e;

/**
 * @enum qosa_nw_reject_type_e
 * @brief Reject type
 */
typedef enum
{
    QOSA_REJECT_TYPE_EMM, /*!< EMM reject type */
    QOSA_REJECT_TYPE_ESM, /*!< ESM reject type */
} qosa_nw_reject_type_e;

/**
 * @enum qosa_emm_cause_e
 * @brief EMM reject cause
 */
typedef enum
{
    QOSA_EMM_CAUSE_IMSI_UNKNOWN_IN_HSS = 0x02,
    QOSA_EMM_CAUSE_ILLEGAL_UE = 0x03,
    QOSA_EMM_CAUSE_IMSI_UNKNOWN_IN_VLR = 0x04,
    QOSA_EMM_CAUSE_IMEI_NOT_ACCEPTED = 0x05,
    QOSA_EMM_CAUSE_ILLEGAL_ME = 0x06,
    QOSA_EMM_CAUSE_EPS_SERVICES_NOT_ALLOWED = 0x07,
    QOSA_EMM_CAUSE_EPS_AND_NON_EPS_SERVICES_NOT_ALLOWED = 0x08,
    QOSA_EMM_CAUSE_UE_ID_CAN_NOT_BE_DERIVED_IN_NETWORK = 0x09,
    QOSA_EMM_CAUSE_IMPLICITLY_DETACHED = 0x0A,
    QOSA_EMM_CAUSE_PLMN_NOT_ALLOWED = 0X0B,
    QOSA_EMM_CAUSE_TRACKING_AREA_NOT_ALLOWED = 0x0C,
    QOSA_EMM_CAUSE_ROAMING_NOT_ALLOWED_IN_THIS_TRACKING_AREA = 0X0D,
    QOSA_EMM_CAUSE_EPS_SERVICE_NOT_ALLOWED_IN_THIS_PLMN = 0x0E,
    QOSA_EMM_CAUSE_NO_SUITABLE_CELLS_IN_TRACKING_AREA = 0x0F,
    QOSA_EMM_CAUSE_MSC_TEMPORARILY_NOT_REACHABLE = 0x10,
    QOSA_EMM_CAUSE_NETWORK_FAILURE = 0x11,
    QOSA_EMM_CAUSE_CS_DOMAIN_NOT_AVAILABLE = 0x12,
    QOSA_EMM_CAUSE_ESM_FAILURE = 0x13,
    QOSA_EMM_CAUSE_MAC_FAILURE = 0X14,
    QOSA_EMM_CAUSE_SYNCH_FAILURE = 0X15,
    QOSA_EMM_CAUSE_CONGESTION = 0X16,
    QOSA_EMM_CAUSE_UE_SECURITY_CAPAILITIES_MISMATCH = 0x17,
    QOSA_EMM_CAUSE_SECURITY_MODE_REJECTED_UNSPECIFIED = 0x18,
    QOSA_EMM_CAUSE_NOT_AUTHORIZED_FOR_THIS_CSG = 0x19,
    QOSA_EMM_CAUSE_NON_EPS_AUTHENTICATION_UNACCEPTABLE = 0x1A,
    QOSA_EMM_CAUSE_REQUESTED_SERVICE_OPTION_NOT_AUTHORIZED_IN_THIS_PLMN = 0X23,
    QOSA_EMM_CAUSE_CS_FALLBACK_CALL_EST_NOT_ALLOWED = 0X26,
    QOSA_EMM_CAUSE_CS_SERVICE_TEMPORARILY_NOT_AVAILABLE = 0x27,
    QOSA_EMM_CAUSE_NO_EPS_BEARER_CONTEXT_ACTIVATED = 0x28,
    QOSA_EMM_CAUSE_SERVERE_NETWORK_FAILURE = 0x2A,
    QOSA_EMM_CAUSE_PLMN_NOT_ALLOWED_TO_OPERATE_AT_THE_PRESENT_UE_LOCATION = 0x4E,
    QOSA_EMM_CAUSE_SYMANTICALLY_INCORRECT_MESSAGE = 0X5F,
    QOSA_EMM_CAUSE_INVALID_MANDATORY_INFORMATION = 0X60,
    QOSA_EMM_CAUSE_MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED = 0X61,
    QOSA_EMM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE = 0X62,
    QOSA_EMM_CAUSE_INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED = 0X63,
    QOSA_EMM_CAUSE_CONDITIONAL_IE_ERROR = 0X64,
    QOSA_EMM_CAUSE_MESSAGE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE = 0X65,
    QOSA_EMM_CAUSE_PROTOCOL_ERROR_UNSPECIFIED = 0X6F,
    QOSA_EMM_CAUSE_UNKNOWN = 0xFF
} qosa_emm_cause;

/**
 * @enum qosa_esm_cause_e
 * @brief ESM reject cause
 */
typedef enum
{
    QOSA_ESM_CAUSE_SUCC = 0x00,

    /*
     * ref: 24.301, 9.9.4.4
    */
    QOSA_ESM_OPERATOR_DETERMINED_BARRING = 0x08,
    QOSA_ESM_INSUFFICIENT_RESOURCES = 0x1a,
    QOSA_ESM_UNKNOWN_OR_MISSING_APN = 0x1b,
    QOSA_ESM_UNKNOWN_PDN_TYPE = 0x1c,
    QOSA_ESM_USER_AUTHENTICATION_FAILED = 0x1d,
    QOSA_ESM_REQUEST_REJECTED_BY_SERVING_GW_OR_PDN_GW = 0x1e,
    QOSA_ESM_REQUEST_REJECTED_UNSPECIFIED = 0x1f,
    QOSA_ESM_SERVICE_OPTION_NOT_SUPPORTED = 0x20,
    QOSA_ESM_REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED = 0x21,
    QOSA_ESM_SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER = 0x22,
    QOSA_ESM_PTI_ALREADY_IN_USE = 0x23,
    QOSA_ESM_REGULAR_DEACTIVATION = 0x24,
    QOSA_ESM_EPS_QOS_NOT_ACCEPTED = 0x25,
    QOSA_ESM_NETWORK_FAILURE = 0x26,
    QOSA_ESM_REACTIVATION_REQUESTED = 0x27,
    QOSA_ESM_FEATURE_NOT_SUPPORTED = 0x28,
    QOSA_ESM_SEMANTIC_ERROR_IN_THE_TFT_OPERATION = 0x29,
    QOSA_ESM_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION = 0x2a,
    QOSA_ESM_INVALID_EPS_BEARER_IDENTITY = 0x2b,
    QOSA_ESM_SEMANTIC_ERRORS_IN_PACKET_FILTER = 0x2c,
    QOSA_ESM_SYNTACTICAL_ERRORS_IN_PACKET_FILTER = 0x2d,
    QOSA_ESM_EPS_BEARER_CONTEXT_WITHOUT_TFT_ALREADY_ACTIVATED = 0x2e,
    QOSA_ESM_PTI_MISMATCH = 0x2f,
    QOSA_ESM_LAST_PDN_DISCONNECTION_NOT_ALLOWED = 0x31,
    QOSA_ESM_PDN_TYPE_IPV4_ONLY_ALLOWED = 0x32,
    QOSA_ESM_PDN_TYPE_IPV6_ONLY_ALLOWED = 0x33,
    QOSA_ESM_SINGLE_ADDRESS_BEARERS_ONLY_ALLOWED = 0x34,
    QOSA_ESM_ESM_INFORMATION_NOT_RECEIVED = 0x35,
    QOSA_ESM_PDN_CONNECTION_DOES_NOT_EXIST = 0x36,
    QOSA_ESM_MULTIPLE_PDN_CONNECTIONS_FOR_A_GIVEN_APN_NOT_ALLOWED = 0x37,
    QOSA_ESM_COLLISION_WITH_NETWORK_INITIATED_REQUEST = 0x38,
    QOSA_ESM_PDN_TYPE_IPV4V6_ONLY_ALLOW = 0x39,
    QOSA_ESM_PDN_TYPE_NON_IP_ONLY_ALLOW = 0x3a,
    QOSA_ESM_UNSUPPORTED_QCI_VALUE = 0x3b,
    QOSA_ESM_BEARER_HANDLING_NOT_SUPPORTED = 0x3c,
    QOSA_ESM_PDN_TYPE_ETHERNET_ONLY_ALLOWED = 0x3d,
    QOSA_ESM_MAX_PDP_CONTEXT_REACHED = 0x41,
    QOSA_ESM_APN_NOT_SUPPORTED_IN_PLMN_RAT_COMBINATION = 0x42,
    QOSA_ESM_INVALID_PTI_VALUE = 0x51,
    QOSA_ESM_SEMANTICALLY_INCORRECT_MESSAGE = 0x5f,
    QOSA_ESM_INVALID_MANDATORY_INFORMATION = 0x60,
    QOSA_ESM_MESSAGE_TYPE_NONEXISTENT_OR_NOT_IMPLEMENTED = 0x61,
    QOSA_ESM_MESSAGE_TYPE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE = 0x62,
    QOSA_ESM_INFORMATION_ELEMENT_NONEXISTENT_OR_NOT_IMPLEMENTED = 0x63,
    QOSA_ESM_CONDITIONAL_IE_ERROR = 0x64,
    QOSA_ESM_MESSAGE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE = 0x65,
    QOSA_ESM_PROTOCOL_ERROR_OR_UNSPECIFIED = 0x6f,
    QOSA_ESM_APN_RESTRICTION_VALUE_INCOMPATIBLE_WITH_ACTIVE_EPS_BEARER_CONTEXT = 0x70,
    QOSA_ESM_MUTIPLE_ACCESSES_TO_A_PDN_CONNECTION_NOT_ALLOWED = 0x71,
    QOSA_ESM_CAUSE_UNKNOWN = 0xFF
} qosa_esm_cause;

/**
 * @enum qosa_nw_jdcfg_type_e
 * @brief
 */
typedef enum
{
    QOSA_NW_JDCFG_TYPE_ENUM_NONE = -1,
    QOSA_NW_JDCFG_TYPE_MODE = 0,
    QOSA_NW_JDCFG_TYPE_URC,
    QOSA_NW_JDCFG_TYPE_PERIOD,
    QOSA_NW_JDCFG_TYPE_MINCH,
    QOSA_NW_JDCFG_TYPE_SINR,
    QOSA_NW_JDCFG_TYPE_RXLEV,
    QOSA_NW_JDCFG_TYPE_RSRP,
    QOSA_NW_JDCFG_TYPE_RSRQ,
    QOSA_NW_JDCFG_TYPE_RSSI,
    QOSA_NW_JDCFG_TYPE_SHAKEPERIOD,
    QOSA_NW_JDCFG_TYPE_RSSI_GSM,
    QOSA_NW_JDCFG_TYPE_TDETECT,
    QOSA_NW_JDCFG_TYPE_REPORTINTERVAL,
    QOSA_NW_JDCFG_TYPE_JAMM_DETECT_RESULT,
    QOSA_NW_JDCFG_TYPE_ENUM_MAX
} qosa_nw_jdcfg_type_e;

/**
 * @enum qosa_nw_jdcfg_type_e
 * @brief
 */
typedef enum
{
    QOSA_NW_JAMM_DETECT_RESULT_DETECTEDINVALID = 0,
    QOSA_NW_JAMM_DETECT_RESULT_NOTDETECTED = 1,
    QOSA_NW_JAMM_DETECT_RESULT_DETECTED = 2,
    QOSA_NW_JAMM_DETECT_RESULT_ENUM_MAX
} qosa_nw_jamm_detect_result_e;

/**
 * @enum qosa_nw_ue_operation_mode_e
 * @brief UE's operation mode setting
 *
 */
typedef enum
{
    QOSA_NW_PS_MODE_2,      /*!< only EPS services with data centric mode */
    QOSA_NW_CS_PS_MODE_1,   /*!< EPS and non-EPS services with voice centric mode */
    QOSA_NW_CS_PS_MODE_2,   /*!< EPS and non-EPS services with data centric mode */
    QOSA_NW_PS_MODE_1,      /*!< only EPS services with voice centric mode */
    QOSA_NW_UE_OPERATION_MODE_MAX,           /*!< Maximum value */
} qosa_nw_ue_operation_mode_e;

/**
 * @enum qosa_nw_blacklist_policy_type_e
 * @brief Network blacklist trigger policy type
 *
 */
typedef enum
{
    QOSA_NW_BLACKLIST_POLICY_NONE = 0,                  /**< No processing policy, only add to blacklist */
    QOSA_NW_BLACKLIST_POLICY_TIMEOUT = 1 << 0,          /**< Policy bit: Auto remove from blacklist when timeout */
    QOSA_NW_BLACKLIST_POLICY_CFUN = 1 << 1,             /**< Policy bit: Trigger CFUN mode switch processing */
    QOSA_NW_BLACKLIST_POLICY_POWER_DOWN = 1 << 2,       /**< Policy bit: Trigger RF module power off processing */
} qosa_nw_blacklist_policy_type_e;

/**
 * @enum qosa_nw_blacklist_params_e
 * @brief Network blacklist config parameter definition
 *
 */
typedef enum
{
    QOSA_NW_BLACKLIST_CELL_MAX = 10,                /**< Max number of cells stored in blacklist */

    QOSA_NW_BLACKLIST_DISABLE = 0,                  /**< Network blacklist function: Disable */
    QOSA_NW_BLACKLIST_ENABLE = 1,                   /**< Network blacklist function: Enable */
    /* Unit: s */
    QOSA_NW_BLACKLIST_TIMEOUT_MIN = 300,            /**< Min timeout for auto remove blacklist, Unit: s */
    QOSA_NW_BLACKLIST_TIMEOUT_MAX = 86400,          /**< Max timeout for auto remove blacklist, Unit: s */
    QOSA_NW_BLACKLIST_TIMEOUT_DEFAULT = 1800,       /**< Default timeout for auto remove blacklist, Unit: s */

    QOSA_NW_BLACKLIST_STUCK_DISABLE = 0,            /**< Blacklist anti-stuck protection: Disable */
    QOSA_NW_BLACKLIST_STUCK_ENABLE = 1,             /**< Blacklist anti-stuck protection: Enable */
    /* Unit: s */
    QOSA_NW_BLACKLIST_STUCK_TIMEOUT_MIN = 60,       /**< Min weighted release timeout for unregistered cell, Unit: s */
    QOSA_NW_BLACKLIST_STUCK_TIMEOUT_MAX = 3600,     /**< Max weighted release timeout for unregistered cell, Unit: s */
    QOSA_NW_BLACKLIST_STUCK_TIMEOUT_DEFAULT = 300,  /**< Default weighted release timeout for unregistered cell, Unit: s */

    QOSA_NW_BLACKLIST_CNT_MODE_CONSECUTIVE = 0,     /**< Failure counting mode: Consecutive counting */
    QOSA_NW_BLACKLIST_CNT_MODE_CUMULATIVE = 1,      /**< Failure counting mode: Cumulative counting */
    QOSA_NW_BLACKLIST_CNT_THRESH_MIN = 1,           /**< Min threshold of failure scenario count */
    QOSA_NW_BLACKLIST_CNT_THRESH_MAX = 255,         /**< Max threshold of failure scenario count */
    QOSA_NW_BLACKLIST_CNT_THRESH_DEFAULT = 5,       /**< Default threshold of failure scenario count */

    QOSA_NW_BLACKLIST_OSC_ROUNDS_MIN = 1,           /**< Min rounds of ping-pong handover detection */
    QOSA_NW_BLACKLIST_OSC_ROUNDS_MAX = 255,         /**< Max rounds of ping-pong handover detection */
    QOSA_NW_BLACKLIST_OSC_ROUNDS_DEFAULT = 3,       /**< Default rounds of ping-pong handover detection */
    /* Unit: s*/
    QOSA_NW_BLACKLIST_OSC_PERIOD_MIN = 300,         /**< Min period of ping-pong handover detection, Unit: s */
    QOSA_NW_BLACKLIST_OSC_PERIOD_MAX = 32400,       /**< Max period of ping-pong handover detection, Unit: s */
    QOSA_NW_BLACKLIST_OSC_PERIOD_DEFAULT = 600,     /**< Default period of ping-pong handover detection, Unit: s */

    QOSA_NW_BLACKLIST_HOST_MAX_SIZE = 256,          /**< Max char length of blacklist HOST domain/address */
} qosa_nw_blacklist_params_e;

/**
 * @enum qosa_nw_blacklist_scene_e
 * @brief Network blacklist trigger scene type
 *
 */
typedef enum
{
    QOSA_NW_ATTACH_OR_SERVICE_REJECT=0,
    QOSA_NW_ATTACH_NO_RSP,
    QOSA_NW_SERVICE_NO_RSP,
    QOSA_NW_NO_DATA_TRASMIT,
    QOSA_NW_RESELECT_FREQUENTLY=4,
    QOSA_NW_QBLACKCELL_UNKNOWN_SCENE = 7,
} qosa_nw_blacklist_scene_e;

/**
 * @enum qosa_nw_ims_type_e
 * @brief
 */
typedef enum
{
    QOSA_NW_IMS_TYPE_DEFAULT = 0,
    QOSA_NW_IMS_TYPE_ENABLE = 1,
    QOSA_NW_IMS_TYPE_DISABLE = 2,

} qosa_nw_ims_type_e;
/*-------------------------------------- basic struct --------------------------------------*/

/**
 * @struct qosa_nw_oper_name_t
 * @brief Operator name
 *
 */
typedef struct nw_network_name
{
    qosa_uint8_t length;                             /*!< Length of operator name */
    qosa_uint8_t charset;                            /*!< Reserved for character set representation */
    char         name[QOSA_NW_NETWORK_NAME_LEN_MAX]; /*!< Operator name */
} qosa_nw_oper_name_t;                               //TODO: character set

/**
 * @struct qosa_nw_band_t
 * @brief Network band
 * @note When setting command, if you want to set only one RAT's band, other band values should be kept as 0, indicating not to change their band values.
 *
 */
typedef struct nw_band
{
    qosa_uint32_t opt;           /*!< Reserved */
    qosa_uint32_t gwbandval;     /*!< bit0: GSM 900; bit1: 1800; bit2:850; ... bit4: WCDMA2100; bit5:WCDMA1900; ... */
    qosa_uint32_t ltebandval[8]; /*!< First element stores LTE band 1 to 32; Second element stores LTE band 33 to 64; Third element stores LTE band 65 to 96 */
    qosa_uint32_t nrbandval[32]; /*!< First element stores NR band 1 to 32; Second element stores NR band 33 to 64; Third element stores NR band 65 to 96 */
    qosa_uint32_t reserve;       /*!< Reserved */
} qosa_nw_band_t;

/**
 * @struct qosa_nw_rrc_conn_info_t
 * @brief Network side RRC connection status
 *
 */
typedef struct
{
    qosa_nw_rrc_conn_mode_e   mode;   /*!< RRC connection mode */
    qosa_nw_rrc_state_e       state;  /*!< RRC state */
    qosa_nw_rrc_access_type_e access; /*!< RRC access type */
} qosa_nw_rrc_conn_info_t;

/**
 * @struct qosa_nw_reject_info_t
 * @brief Reject information
 *
 */
typedef struct
{
    qosa_nw_reject_type_e type;         /*!< Reject type */
    qosa_uint16_t         reject_cause; /*!< Reject cause */
} qosa_nw_reject_info_t;
/*-------------------------------------- event --------------------------------------*/

/**
 * @struct qosa_nw_reg_status_event_t
 * @brief UE registration status report event content
 * @brief Event QOSA_EVENT_MODEM_NW_CS_REG_STATUS/QOSA_EVENT_MODEM_NW_PS_REG_STATUS report carries data content
 *
 */
typedef struct nw_reg_status_event
{
    qosa_uint8_t         simid;       /*!< SIM card ID */
    qosa_nw_reg_status_e reg_status;  /*!< Network registration status */
    qosa_bool_t          loc_present; /*!< Whether lac cid act parameters exist, when loc_present is false, lac, cid, act parameters should not be used */
    qosa_uint32_t        lac;         /*!< Location area ID or tracking area ID */
#ifdef CONFIG_QOSA_NW_NR_SUPPORT
    qosa_uint64_t cid;                /*!< Cell ID */
#else
    qosa_uint32_t cid; /*!< Cell ID */
#endif
    qosa_nw_act_e act; /*!< Network RAT */

#ifdef CONFIG_QOSA_NW_NR_SUPPORT
    qosa_uint16_t allowed_nssai_len;  /*!< Allowed NSSAI length; 0 means invalid */
    char          allowed_nssai[200]; /*!< Allowed NSSAI string */
#endif
    qosa_uint32_t reserve;
} qosa_nw_reg_status_event_t;

/**
 * @struct qosa_nw_reg_signal_quality_event_t
 * @brief Signal status change report event
 * @brief Event QOSA_EVENT_MODEM_NW_SIGNAL_QUALITY report carries data content
 *
 */
typedef struct
{
    qosa_uint8_t  simid; /*!< SIM card ID */
    qosa_nw_rat_e rat;   /*!< Serving cell RAT */

    union
    {
        struct
        {
            qosa_signal_level_t
                signal_level; /*!< signal_level under GSM: refers to received signal strength; unit: dBm; measurement value range: -113 ~ -48; invalid value: QOSA_NW_PI_INT16 */
            qosa_ber_t ber; /*!< ber under GSM: refers to bit error rate; unit: none; report value range: 0~7, 99 means unknown */
        } gsm;
        struct
        {
            /* Not recommended to use this parameter, this parameter is directly uploaded by the platform layer, calculation rules are uncertain */
            qosa_uint16_t rssi_meas;   /*!< rssi measurement value under 3G, unit dBm */
            qosa_uint16_t rssi_report; /*!< rssi measurement value under 3G, unit dBm */
            qosa_rscp_t   rscp;        /*!< rscp under WCDMA: refers to received signal code power; unit: dBm; measurement value*100, range: -12000 ~ -2500 */
            qosa_ecno_t   ecno;        /*!< ecno under WCDMA: refers to Ec/Io; unit: dB; measurement value*100, range: -2400 ~ 0 */
        } wcdma;
        struct
        {
            qosa_rssi_t rssi; /*!< RSSI under LTE: refers to received signal strength indicator; unit: dBm; measurement value*100, range: -10000 ~ -2500 */
            qosa_rsrp_t rsrp; /*!< rsrp under LTE: refers to reference signal received power; unit: dBm; measurement value*100, range: -15600 ~ -4400 */
            qosa_sinr_t sinr; /*!< sinr under LTE: refers to signal to interference plus noise ratio; unit: dB; measurement value*100, range: -2300 ~ 4000 */
            qosa_rsrq_t rsrq; /*!< rsrq under LTE: refers to reference signal received quality; unit: dB; measurement value*100, range: -3400 ~ -250 */
        } lte;
        struct
        {
            qosa_rsrp_t rsrp; /*!< ss_rsrp under 5gnr: refers to reference signal received power; unit: dBm; measurement value*100, range: -15600 ~ -4400 */
            qosa_sinr_t
                sinr; /*!< ss_sinr under 5gnr: refers to signal to interference plus noise ratio; unit: dB; measurement value*100, range: -2300 ~ 4000 */
            qosa_rsrq_t rsrq; /*!< ss_rsrq under 5gnr: refers to reference signal received quality; unit: dB; measurement value*100, range: -3400 ~ -250 */
        } nr5g;
    };

    qosa_uint32_t reserve;
} qosa_nw_reg_signal_quality_event_t;

/**
 * @struct qosa_nw_rrc_conn_event_t
 * @brief RRC connection status report event parameters, corresponding to event ID QOSA_EVENT_MODEM_NW_RRC_STATUS
 *
 */
typedef struct
{
    qosa_uint8_t              simid  : 8; /*!< SIM card ID */
    qosa_nw_rrc_conn_mode_e   mode   : 8; /*!< RRC connection mode */
    qosa_nw_rrc_state_e       state  : 8; /*!< RRC state */
    qosa_nw_rrc_access_type_e access : 8; /*!< RRC access type */
} qosa_nw_rrc_conn_event_t;

/**
 * @struct qosa_nw_nitz_event_t
 * @brief NITZ event parameters, corresponding to event ID QOSA_EVENT_MODEM_NW_NITZ_INFO
 *
 */
typedef struct
{
    qosa_uint8_t  simid;    /*!< SIM card ID */
    qosa_uint32_t year;     /*!< Year */
    qosa_uint32_t month;    /*!< Month */
    qosa_uint32_t day;      /*!< Day */
    qosa_uint32_t hour;     /*!< Hour */
    qosa_uint32_t minute;   /*!< Minute */
    qosa_uint32_t second;   /*!< Second */
    qosa_int32_t  timezone; /*!< Time zone */
    qosa_int32_t  dst;      /*!< Daylight saving time */
} qosa_nw_nitz_event_t;

/**
 * @struct qosa_nw_nas_event_t
 * @brief NAS event parameters, corresponding to event ID QOSA_EVENT_MODEM_NW_NAS_EVENT
 *
 */
typedef struct
{
    qosa_uint8_t             simid;        /*!< SIM card ID, identifying which SIM card the event belongs to */
    qosa_nw_nas_event_type_e event_type;   /*!< NAS event type, identifying specific event category */
    qosa_uint16_t            reject_cause; /*!< Reject cause code, used when event is reject type */
    qosa_uint8_t             rat;
    qosa_uint32_t            arfcn; /*!< E-UTRA carrier frequency(maxEARFCN2); 0..262143; invalid value: QOSA_NW_PI_UINT32 */
    qosa_uint16_t            pcid;   /*!< physical cell identity(PhysCellId); 0..503; invalid value: QOSA_NW_PI_UINT16 */
} qosa_nw_nas_event_t;

/**
 * @struct qosa_nw_as_event_t
 * @brief
 * @brief AS event parameters, corresponding to event ID QOSA_EVENT_MODEM_NW_AS_EVENT
 */
typedef struct
{
    qosa_uint8_t            simid;           /*!< SIM card ID */
    qosa_uint8_t            is_occured;      /*!< Whether as event occurs */
    qosa_nw_as_event_type_e event_type;      /*!< AS event type, identifying specific event category */
} qosa_nw_as_event_t;

/**
 * @struct qosa_nw_reject_event_t
 * @brief Network reject event parameters, corresponding to event ID QOSA_EVENT_MODEM_NW_REJECT_EVENT
 *
 */
typedef struct
{
    qosa_uint8_t          simid;       /*!< SIM card ID */
    qosa_nw_reject_info_t reject_info; /*!< Reject information, including reject cause and other detailed information */
} qosa_nw_reject_event_t;

/**
 * @struct qosa_nw_jamm_detect_event_t
 * @brief Network reject event parameters, corresponding to event ID QOSA_EVENT_MODEM_JAMM_DETECT_STATUS
 *
 */
typedef struct
{
    qosa_uint8_t jammdetectrslt; /*!< Reporting of jamming detection results range value: 0-2 default: 0 JammNotDetected：1 JammDetected：2  */
} qosa_nw_jamm_detect_event_t;

/**
 * @struct qosa_nw_cell_change_event_t
 * @brief Network reject event parameters, corresponding to event ID QOSA_EVENT_MODEM_NW_CELL_CHANGE_EVENT
 *
 */
typedef struct
{
    qosa_uint8_t  simid;                   /*!< SIM card ID */
    qosa_uint16_t old_pcid;   /*!< old physical cell identity(PhysCellId); 0..503; invalid value: QOSA_NW_PI_UINT16 */
    qosa_uint16_t new_pcid;   /*!< new physical cell identity(PhysCellId); 0..503; invalid value: QOSA_NW_PI_UINT16 */

    qosa_uint32_t old_earfcn; /*!< old E-UTRA carrier frequency(maxEARFCN2); 0..262143; invalid value: QOSA_NW_PI_UINT32 */
    qosa_uint32_t new_earfcn; /*!< new E-UTRA carrier frequency(maxEARFCN2); 0..262143; invalid value: QOSA_NW_PI_UINT32 */
} qosa_nw_cell_change_event_t;

/**
 * @struct qosa_nw_detach_event_t
 * @brief Network detach event reporting parameters -- corresponding event: QOSA_EVENT_NW_DETACH
 *
 */
typedef struct
{
    qosa_uint8_t simid; /*!< SIM card ID */
    qosa_bool_t is_me; /*!< Is it triggered by ME */
} qosa_nw_detach_event_t;
/*-------------------------------------- pa --------------------------------------*/
/**
 * @struct qosa_nw_oper_list_t
 * @brief Operator list structure
 *
 * This structure is used to store operator information, including operator status, network RAT, PLMN and operator name information
 */
typedef struct oper_list
{
    qosa_nw_oper_stat_e stat;       /*!< Operator status */
    qosa_nw_act_e       act;        /*!< Operator network RAT */
    qosa_plmn_t         plmn;       /*!< Operator PLMN, the struct of plmn; example: "46001" (mcc=460, mnc=01, mnc_digit_num=2) */
    qosa_nw_oper_name_t short_name; /*!< Short operator name */
    qosa_nw_oper_name_t long_name;  /*!< Long operator name */
} qosa_nw_oper_list_t;

/**
 * @struct qosa_nw_nr5g_param_t
 * @brief 5G NR network parameter structure
 *
 */
typedef struct
{
    qosa_uint64_t cellid; /*!< CellIdentity, NR Cell identity(NCI); BIT STRING (SIZE (36)); invalid value: QOSA_NW_PI_UINT64 */
    qosa_plmn_t
        plmn; /*!< plmn id (MCC, MNC): refers to the struct of plmn; example: "46001" (mcc=460, mnc=01, mnc_digit_num=2); invalid value: 0xFFFF,0xFFFF,0xFF */
    qosa_uint32_t tac;        /*!< tracking area code; BIT STRING (SIZE (24)) ;invalid value: QOSA_NW_PI_UINT32 */

    qosa_uint16_t pcid;       /*!< physical cell identity(PhysCellId); 0..1007; invalid value: QOSA_NW_PI_UINT16 */

    qosa_uint32_t arfcn;      /*!< NR carrier frequency; 0..3279165; invalid value: QOSA_NW_PI_UINT32 */
    qosa_uint16_t band;       /*!< NR frequency band number; 1..1024; invalid value: QOSA_NW_PI_UINT16 */

    qosa_uint8_t duplex_mode; /*!< Duplex mode; 0 - FDD, 1 - TDD; invalid value: QOSA_NW_PI_UINT8 */

    qosa_uint8_t
        ul_bandwidth; /*!< Uplink bandwidth configuration value, parameter value: 0-mhz5, 1-mhz10, 2-mhz15, etc.. ref SupportedBandwidth; invalid value: QOSA_NW_PI_UINT8 */
    qosa_uint8_t
        dl_bandwidth; /*!< Downlink bandwidth configuration value, parameter value: 0-mhz5, 1-mhz10, 2-mhz15, etc.. ref SupportedBandwidth; invalid value: QOSA_NW_PI_UINT8 */

    qosa_uint16_t ul_NRB; /*!< Uplink RB number; invalid value: QOSA_NW_PI_UINT16 */
    qosa_uint16_t dl_NRB; /*!< Downlink RB number; invalid value: QOSA_NW_PI_UINT16 */

    qosa_rsrp_t
        rsrp; /*!< ss_rsrp under 5G: refers to reference signal received power; unit: dBm; measurement value*100, range: -15600 ~ -4400; invalid value: QOSA_NW_PI_INT16 */
    qosa_rsrq_t
        rsrq; /*!< ss_rsrq under 5G: refers to reference signal received quality; unit: dB; measurement value*100, range: -3400 ~ -250; invalid value: QOSA_NW_PI_INT16 */
    qosa_sinr_t
        sinr; /*!< ss_sinr under 5G: refers to signal to interference plus noise ratio; unit: dB; measurement value*100, range: -2300 ~ 4000; invalid value: QOSA_NW_PI_INT16 */

    qosa_uint8_t scs;     /*!< subcarrier spacing; 0..6; ref maxSCSs; invalid value: QOSA_NW_PI_UINT8 */

    qosa_srxlev_t srxlev; /*!< Cell selection level: refers to cell selection S value; unit: dB; actual value; range: -; invalid value: QOSA_NW_PI_INT16 */

    qosa_uint16_t offset_to_point_a;     /*!< offsetToPointA; 0..2199; ref offsetToPointA; invalid value is QOSA_NW_PI_UINT16 */

    qosa_uint8_t ssb_sub_carrier_offset; /*!< ssb-SubcarrierOffset; 0..15; ref ssb-SubcarrierOffset; invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t ssb_scs;                /*!< ssbSubcarrierSpacing; 0..6; ref ssb-maxSCSs; invalid value is QOSA_NW_PI_UINT8 */
#ifdef CONFIG_QOSA_NW_NR_SUPPORT
    qosa_uint16_t allowed_nssai_len;     /*!< Allowed NSSAI length; 0 means invalid */
    char          allowed_nssai[200];    /*!< Allowed NSSAI string */
#endif
} qosa_nw_nr5g_param_t;

/**
 * @struct qosa_nw_lte_param_t
 * @brief Define network module LTE board network parameters, parameters may be invalid in different usage scenarios
 *
 */
typedef struct
{
    qosa_uint32_t cellid; /*!< CellIdentity; BIT STRING (SIZE (28)); invalid value: QOSA_NW_PI_UINT32 */
    qosa_plmn_t
        plmn; /*!< plmn id (MCC, MNC): refers to the struct of plmn; example: "46001" (mcc=460, mnc=01, mnc_digit_num=2); invalid value: 0xFFFF,0xFFFF,0xFF */
    qosa_uint16_t tac;         /*!< tracking area code; BIT STRING (SIZE (16)); invalid value: QOSA_NW_PI_UINT16 */

    qosa_uint16_t pcid;        /*!< physical cell identity(PhysCellId); 0..503; invalid value: QOSA_NW_PI_UINT16 */

    qosa_uint32_t earfcn;      /*!< E-UTRA carrier frequency(maxEARFCN2); 0..262143; invalid value: QOSA_NW_PI_UINT32 */
    qosa_uint16_t band;        /*!< band; invalid value: QOSA_NW_PI_UINT16 */

    qosa_bool_t is_tdd;        /*!< Whether it is TDD or FDD band, OSA_TRUE means it is TDD */

    qosa_uint8_t ul_bandwidth; /*!< Uplink bandwidth configuration value, parameter value: 0 - 1.4M, 1 - 3M, 2 - 5M, 3 - 10M, 4 - 15M, 5 - 20M */

    qosa_uint8_t dl_bandwidth; /*!< Downlink bandwidth configuration value, parameter value: 0 - 1.4M, 1 - 3M, 2 - 5M, 3 - 10M, 4 - 15M, 5 - 20M */

    qosa_rssi_t rssi;     /*!< Received signal strength indicator; unit: dBm; measurement value*100, range: -10000 ~ -2500; invalid value: QOSA_NW_PI_INT16 */
    qosa_rsrp_t rsrp;     /*!< Reference signal received power; unit: dBm; measurement value*100, range: -15600 ~ -4400; invalid value: QOSA_NW_PI_INT16 */
    qosa_rsrq_t rsrq;     /*!< Reference signal received quality; unit: dB; measurement value*100, range: -3400 ~ -250; invalid value: QOSA_NW_PI_INT16 */
    qosa_sinr_t sinr;     /*!< Signal to interference plus noise ratio; unit: dB; measurement value*100, range: -2300 ~ 4000; invalid value: QOSA_NW_PI_INT16 */

    qosa_srxlev_t srxlev; /*!< Cell selection S value; unit: dB; actual value; range: -; invalid value: QOSA_NW_PI_INT16 */
    qosa_squal_t  squal;  /*!< Cell selection signal quality; unit: dB; actual value; range: -; invalid value: QOSA_NW_PI_INT16 */

    qosa_uint8_t cell_resel_priority; /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t s_non_intra_search;  /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t thresh_serving_low;  /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t s_intra_search;      /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */

    qosa_uint8_t threshX_low;         /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t threshX_high;        /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */

    qosa_int16_t rx_dbm;              /*!< Received power, unit dbm, actual value*100; Allow invalid, invalid value is QOSA_NW_PI_INT16 */

    qosa_bool_t cell_barred;          /*!< Cell barred access identifier; Allow invalid, invalid value is QOSA_NW_PI_BOOL */

    qosa_int8_t  txpower;             /*!< Current TX power level in dbm, -128 is the invalid value */
    qosa_int32_t txtime;              /*!< Total TX time since last reboot in millisecond */
    qosa_int32_t rxtime;              /*!< Total RX time since last reboot in millisecond */

    qosa_bler_t ul_bler;          /*!< Upload Block Error Rate; invalid value: QOSA_NW_PI_UINT16 */
    qosa_bler_t dl_bler;          /*!< Download Block Error Rate; invalid value: QOSA_NW_PI_UINT16 */
} qosa_nw_lte_param_t;

/**
 * @struct qosa_nw_wcdma_param_t
 * @brief Define network module WCDMA board network parameters, parameters may be invalid in different usage scenarios
 *
 */
typedef struct
{
    qosa_uint32_t cellid; /*!< CellIdentity; BIT STRING (SIZE (28)); invalid value: QOSA_NW_PI_UINT32 */
    qosa_plmn_t
        plmn; /*!< plmn id (MCC, MNC): refers to the struct of plmn; example: "46001" (mcc=460, mnc=01, mnc_digit_num=2); invalid value: 0xFFFF,0xFFFF,0xFF */
    qosa_uint16_t lac;    /*!< Allow invalid, invalid value: QOSA_NW_PI_UINT16 */
    qosa_uint16_t rac;    /*!< routing area code */

    qosa_uint16_t psc;    /*!< serving cell primary scrambling code */

    qosa_uint16_t uarfcn; /*!< carrier frequency; 0..16383; invalid value: QOSA_NW_PI_UINT16 */
    qosa_uint16_t band;   /*!< band; invalid value: QOSA_NW_PI_UINT16 */

    qosa_ecno_t ecno;     /*!< Ec/Io: Allow invalid; unit: dB; measurement value*100, range: -2400 ~ 0; invalid value: QOSA_NW_PI_INT16 */
    qosa_rscp_t
        rscp; /*!< Received signal code power: Allow invalid; unit: dBm; measurement value*100, range: -12000 ~ -2500; invalid value: QOSA_NW_PI_INT16 */

    /* Not recommended to use these parameters, these parameters are directly uploaded by the platform layer, calculation rules are uncertain */
    qosa_uint16_t rssi_meas;           /*!< rssi measurement value under 3G, unit dBm */
    qosa_uint16_t rssi_report;         /*!< rssi report value under 3G, range 0~31 or 99 means unknown or not detected */
    qosa_uint16_t rssi_rxlev;          /*!< rssi report value under 3G, range 0~63 or 99 means unknown or not detected */

    qosa_uint8_t  is_fdpch;            /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint32_t sf;                  /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT32 */
    qosa_uint8_t  slot_fmt;            /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_int16_t  speech_code;         /*!< Allow invalid, invalid value is QOSA_NW_PI_INT16 */
    qosa_uint8_t  compressed_mode;     /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */

    qosa_uint8_t  cell_resel_priority; /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint16_t thresh_Xhigh;        /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT16 */
    qosa_uint16_t thresh_Xlow;         /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT16 */
    qosa_srxlev_t srxlev;              /*!< Cell selection S value: Allow invalid; unit: dB; actual value; range: -; invalid value: QOSA_NW_PI_INT16 */
    qosa_int16_t  srxqual;             /*!< Allow invalid; invalid value: QOSA_NW_PI_INT16 */
    qosa_uint16_t set;                 /*!< Allow invalid; invalid value: QOSA_NW_PI_UINT16 */
    qosa_int16_t  rank;                /*!< Allow invalid, invalid value is QOSA_NW_PI_INT16 */

    qosa_bool_t cell_barred;           /*!< Cell barred access identifier; Allow invalid, invalid value is QOSA_NW_PI_BOOL */
} qosa_nw_wcdma_param_t;

/**
 * @struct qosa_nw_jamm_detect_set_param_t
 * @brief Define the Jamming detection parameters, as these parameters may be ineffective in different application platforms.
 *
 */
typedef struct
{
    qosa_nw_jdcfg_type_e qjdcfg_enum;
    qosa_int32_t         value; /*!< Allow invalid, invalid value is QOSA_NW_PI_INT32 */
} qosa_nw_jamm_detect_set_param_t;

/**
 * @struct qosa_nw_jamm_detect_setting_param_t
 * @brief Define the Jamming detection parameters, as these parameters may be ineffective in different application platforms.
 *
 */
typedef struct
{
    qosa_nw_jamm_detect_set_param_t param;
    qosa_uint8_t                    cmd_str[20]; /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_int32_t                    def;         /*!< Allow invalid, invalid value is QOSA_NW_PI_INT32 */
    qosa_int32_t                    minval;      /*!< Allow invalid, invalid value is QOSA_NW_PI_INT32 */
    qosa_int32_t                    maxval;      /*!< Allow invalid, invalid value is QOSA_NW_PI_INT32 */
} qosa_nw_jamm_detect_setting_param_t;

/**
 * @struct qosa_nw_gsm_param_t
 * @brief Define network module GSM board network parameters, parameters may be invalid in different usage scenarios
 *
 */
typedef struct
{
    qosa_uint32_t cellid; /*!< CellIdentity; BIT STRING (SIZE (28)); invalid value: QOSA_NW_PI_UINT32 */
    qosa_plmn_t
        plmn; /*!< plmn id (MCC, MNC): refers to the struct of plmn; example: "46001" (mcc=460, mnc=01, mnc_digit_num=2); invalid value: 0xFFFF,0xFFFF,0xFF */
    qosa_uint16_t lac;          /*!< Allow invalid, invalid value: QOSA_NW_PI_UINT16 */

    qosa_uint8_t bsic;          /*!< base station identity code; range: 0~63  */

    qosa_uint16_t arfcn;        /*!< carrier frequency; 0..16383; invalid value: QOSA_NW_PI_UINT16 */
    qosa_uint16_t band;         /*!< band; invalid value: QOSA_NW_PI_UINT16 */

    qosa_int32_t ber;           /*!< Allow invalid, invalid value: QOSA_NW_PI_INT32 */
    qosa_signal_level_t
        signal_level;           /*!< Allow invalid, invalid value: QOSA_NW_PI_INT16; received signal strength; unit: dBm; measurement value range: -113 ~ -48 */

    qosa_uint8_t txp;           /*!< Maximum transmission power; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t rla;           /*!< Minimum access level; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t drx;           /*!< Non-continuous reception cycle length; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_int32_t c1;            /*!< Allow invalid, invalid value is QOSA_NW_PI_INT32 */
    qosa_int32_t c2;            /*!< Allow invalid, invalid value is QOSA_NW_PI_INT32 */
    qosa_int32_t c31;           /*!< Allow invalid, invalid value is QOSA_NW_PI_INT32 */
    qosa_int32_t c32;           /*!< Allow invalid, invalid value is QOSA_NW_PI_INT32 */

    qosa_uint8_t  gprs_support; /* Whether current cell supports GPRS; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint16_t tch;          /*!< Traffic channel; Allow invalid, invalid value is QOSA_NW_PI_UINT16 */
    qosa_uint8_t  ts;           /*!< Time slot number; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint32_t ta;           /*!< Base station time advance; Allow invalid, invalid value is QOSA_NW_PI_UINT32 */
    qosa_uint8_t  maio;         /*!< Mobile allocation pointer offset; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t  hsn;          /*!< Hopping sequence number; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */

    qosa_uint8_t rxqual_full;   /*!< Reception quality at full power transmission; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t rxqual_sub;    /*!< Reception quality at non-continuous transmission; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t rxlev_full;    /*!< Reception level at full power transmission; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t rxlev_sub;     /*!< Reception level at non-continuous transmission; Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t dl_chan_mode;  /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */

    qosa_uint8_t cell_resel_priority; /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t thresh_gsm_high;     /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t thresh_gsm_low;      /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */
    qosa_uint8_t ncc_permitted;       /*!< Allow invalid, invalid value is QOSA_NW_PI_UINT8 */

    qosa_srxlev_t srxlev;             /*!< Cell selection S value: Allow invalid; unit: dB; actual value; range: -; invalid value: QOSA_NW_PI_INT16 */
    qosa_int16_t  rank;               /*!< Allow invalid, invalid value is QOSA_NW_PI_INT16 */

    qosa_bool_t cell_barred;          /*!< Cell barred access identifier; Allow invalid, invalid value is QOSA_NW_PI_BOOL */
} qosa_nw_gsm_param_t;

/**
 * @struct qosa_nw_scell_info_t
 * @brief Serving cell information set
 *
 */
typedef struct
{
    qosa_nw_act_e act; /*!< Network RAT */
    union
    {
        qosa_nw_nr5g_param_t  nr5g;  /*!< 5G NR parameters */
        qosa_nw_lte_param_t   lte;   /*!< LTE parameters */
        qosa_nw_wcdma_param_t wcdma; /*!< WCDMA parameters */
        qosa_nw_gsm_param_t   gsm;   /*!< GSM parameters */
    };
} qosa_nw_scell_info_t;

/**
 * @struct qosa_nw_ncell_info_t
 * @brief Neighbor cell data structure
 *
 */
typedef struct
{
    qosa_nw_rat_e rat; /*!< Network RAT */

#ifdef CONFIG_QOSA_NW_NR_SUPPORT
    qosa_uint8_t         nr5g_num;                        /*!< 5G NR neighbor cell count */
    qosa_nw_nr5g_param_t nr5g[QOSA_NW_CELL_INFO_NUM_MAX]; /*!< 5G NR neighbor cell information set */
#endif

    qosa_uint8_t        lte_num;                            /*!< LTE neighbor cell count */
    qosa_nw_lte_param_t lte[QOSA_NW_CELL_INFO_NUM_MAX * 2]; /*!< LTE neighbor cell information set */

#ifdef CONFIG_QOSA_NW_WCDMA_SUPPORT
    qosa_uint8_t          wcdma_num;                        /*!< WCDMA neighbor cell count */
    qosa_nw_wcdma_param_t wcdma[QOSA_NW_CELL_INFO_NUM_MAX]; /*!< WCDMA neighbor cell information set */
#endif

#ifdef CONFIG_QOSA_NW_GSM_SUPPORT
    qosa_uint8_t        gsm_num;                        /*!< GSM neighbor cell count */
    qosa_nw_gsm_param_t gsm[QOSA_NW_CELL_INFO_NUM_MAX]; /*!< GSM neighbor cell information set */
#endif
} qosa_nw_ncell_info_t;

/**
 * @struct qosa_nw_freq_scan_cell_list_t
 * @brief Frequency scan cell list structure
 *
 * This structure is used to store frequency scan results, containing cell information under different network RATs
 */
typedef struct
{
#ifdef CONFIG_QOSA_NW_NR_SUPPORT
    qosa_uint8_t         nr5g_num;                             /*!< 5G NR cell count */
    qosa_nw_nr5g_param_t nr5g[QOSA_NW_FREQ_SCAN_CELL_NUM_MAX]; /*!< 5G NR cell parameter array */
#endif
#ifdef CONFIG_QOSA_NW_LTE_SUPPORT
    qosa_uint8_t        lte_num;                             /*!< LTE cell count */
    qosa_nw_lte_param_t lte[QOSA_NW_FREQ_SCAN_CELL_NUM_MAX]; /*!< LTE cell parameter array */
#endif
#ifdef CONFIG_QOSA_NW_WCDMA_SUPPORT
    qosa_uint8_t          wcdma_num;                             /*!< WCDMA cell count */
    qosa_nw_wcdma_param_t wcdma[QOSA_NW_FREQ_SCAN_CELL_NUM_MAX]; /*!< WCDMA cell parameter array */
#endif
#ifdef CONFIG_QOSA_NW_GSM_SUPPORT
    qosa_uint8_t        gsm_num;                             /*!< GSM cell count */
    qosa_nw_gsm_param_t gsm[QOSA_NW_FREQ_SCAN_CELL_NUM_MAX]; /*!< GSM cell parameter array */
#endif
} qosa_nw_freq_scan_cell_list_t;

/**
 * @struct qosa_nw_lock_freq_t
 * @brief Frequency lock structure
 *
 * This structure is used to store frequency lock information, including frequency, physical cell ID and other parameters
 */
typedef struct
{
    qosa_uint32_t freq; /*!< Frequency */
    qosa_uint16_t pci;  /*!< Physical Cell ID, invalid value: QOSA_NW_PI_UINT16 */

    qosa_uint8_t  scs;  /*!< subcarrier spacing; 0..6; for nr5g only */
    qosa_uint16_t band; /*!< band; 1..1024; for nr5g only */
} qosa_nw_lock_freq_t;

/**
 * @struct qosa_nw_freq_lock_list_t
 * @brief Frequency lock list structure
 *
 * This structure is used to store and manage frequency lock configuration information, including the number of locked frequencies and specific lock information.
 */
typedef struct
{
    qosa_uint32_t num;                   /*!< Number of frequency locks, range: 0~QOSA_NW_LOCK_FREQ_NUM_MAX */
    qosa_nw_lock_freq_t
        lock[QOSA_NW_LOCK_FREQ_NUM_MAX]; /*!< Frequency lock information array, each element contains frequency, PCI and other lock parameters */
} qosa_nw_freq_lock_list_t;

/**
 * @struct qosa_nw_ncell_ext_config_t
 * @brief Extended neighbor cell scan configuration parameters
 *
 */
typedef struct
{
    qosa_uint32_t opcode; /*!< Operation code, available options refer to OSA_NW_NCELL_EXT_OP series macros */
    qosa_nw_band_t
        scan_band; /*!< Neighbor cell scan specified band, opcode specifies whether there is specified band scan behavior, if not specified, this parameter is invalid */
    qosa_uint32_t reserved; /*!< Reserved parameter */
} qosa_nw_ncell_ext_config_t;

/**
 * @struct qosa_nw_freq_scan_t
 * @brief Frequency scan configuration parameters
 * 1. When opcode is configured as QOSA_NW_FREQ_SCAN_OP_DEFAULT, if scan_band is not configured, all supported bands will be scanned
 * 2. When opcode is configured as single QOSA_NW_FREQ_SCAN_OP_SCAN_NR5G, only 5G NR bands will be scanned; if scan_band is not configured, all 5G NR bands will be scanned
 * 3. When opcode is configured as single QOSA_NW_FREQ_SCAN_OP_SCAN_LTE, only LTE bands will be scanned; if scan_band is not configured, all LTE bands will be scanned
 *
 */
typedef struct
{
    qosa_uint32_t opcode; /*!< Operation code, available options refer to OSA_NW_FREQ_SCAN_OP series macros */
    qosa_nw_band_t
        scan_band; /*!< Frequency scan specified band, opcode specifies whether there is specified band scan behavior, if not specified, this parameter is invalid */
    qosa_uint32_t reserved; /*!< Reserved parameter */
} qosa_nw_freq_scan_t;

/**
 * @struct qosa_nw_rrc_fast_release_t
 * @brief RRC fast release parameters
 *
 */
typedef struct
{
    qosa_uint32_t mode;       /*!< Whether rrc fast release function is enabled, 0: disabled, 1: enabled */
    qosa_uint32_t idle_time;  /*!< Time for the module to enter sleep state after sending data, range 1~50, unit: second */
    qosa_uint32_t retry_time; /*!< Time to restore enhanced sleep mode if an exception occurs, range 1~600, unit: second */
} qosa_nw_rrc_fast_release_t;

/**
 * @struct qosa_nw_s_nssai_t
 * @brief Single network slice selection assistance information (S-NSSAI) structure
 *
 * This structure is used to represent single network slice selection assistance information, including slice/service type (SST) and slice differentiator (SD)
 */
typedef struct
{
    qosa_uint16_t nssai_len;                  /*!< S-NSSAI length, in bytes, e.g., "01.000000" length is 4 */
    char          data[QOSA_S_NSSAI_MAX_LEN]; /*!< S-NSSAI slice string, string format, e.g., "01.000000" */
} qosa_nw_s_nssai_t;

/**
 * @struct qosa_nw_nssai_t
 * @brief Network slice selection assistance information (NSSAI) structure
 *
 * This structure is used to represent a group of network slice selection assistance information, can contain multiple S-NSSAI, and optionally include PLMN information
 */
typedef struct
{
    qosa_uint16_t nssai_len;                /*!< NSSAI length, in bytes, e.g., "01.000000" length is 4 */
    char          data[QOSA_NSSAI_MAX_LEN]; /*!< NSSAI slice string, string format, e.g., "01.000000:01.000000" */

    qosa_bool_t plmn_present;               /*!< Whether PLMN information exists flag */
    qosa_plmn_t plmn;                       /*!< Public land mobile network identifier; example: "46001" (mcc=460, mnc=01, mnc_digit_num=2) */
} qosa_nw_nssai_t;

/**
 * @struct qosa_nw_nssai_list_t
 * @brief NSSAI configuration list structure
 *
 * This structure is used to store and manage various types of NSSAI configuration information, including default configuration, reject configuration, allow configuration, etc.
 */
typedef struct
{
    qosa_bool_t     default_present;             /*!< Whether default configuration NSSAI exists flag */
    qosa_nw_nssai_t default_s_nssai;             /*!< Default configuration NSSAI */

    qosa_bool_t     rejected_3gpp_present;       /*!< Whether 3GPP access rejected NSSAI exists flag */
    qosa_nw_nssai_t rejected_3gpp_s_nssai;       /*!< 3GPP access rejected NSSAI */

    qosa_bool_t     rejected_non3gpp_present;    /*!< Whether non-3GPP access rejected NSSAI exists flag */
    qosa_nw_nssai_t rejected_non3gpp_s_nssai;    /*!< Non-3GPP access rejected NSSAI */

    qosa_uint8_t    configured_num;              /*!< Number of configured NSSAI */
    qosa_nw_nssai_t configured_s_nssai[10];      /*!< Configured NSSAI array */

    qosa_uint8_t    allowed_3gpp_num;            /*!< Number of 3GPP access allowed NSSAI */
    qosa_nw_nssai_t allowed_3gpp_s_nssai[10];    /*!< 3GPP access allowed NSSAI array */

    qosa_uint8_t    allowed_non3gpp_num;         /*!< Number of non-3GPP access allowed NSSAI */
    qosa_nw_nssai_t allowed_non3gpp_s_nssai[10]; /*!< Non-3GPP access allowed NSSAI array */
} qosa_nw_nssai_list_t;


/**
 * @struct qosa_nw_cell_select_param_t
 * @brief Network cell selection parameter structure
 *
 */
typedef struct
{
    struct
    {
        qosa_uint8_t srxlev;        /**< Serving cell rx level value */
        qosa_uint8_t squal;         /**< Serving cell signal quality value */
    } scell;                       /**< Serving cell measurement parameter */

    struct
    {
        struct
        {
            qosa_uint8_t rsrp_offs; /**< LTE neighbor cell RSRP offset value */
            qosa_uint8_t rsrq_offs; /**< LTE neighbor cell RSRQ offset value */
        } lte;                     /**< LTE network neighbor cell offset parameter */

        struct
        {
            qosa_uint8_t rscp_offs; /**< WCDMA neighbor cell RSCP offset value */
            qosa_uint8_t ecno_offs; /**< WCDMA neighbor cell EcNo offset value */
        } wcdma;                   /**< WCDMA network neighbor cell offset parameter */

        struct
        {
            qosa_uint8_t rssi_offs; /**< GSM neighbor cell RSSI offset value */
        } gsm;                     /**< GSM network neighbor cell offset parameter */
    } ncell;                      /**< Neighbor cell offset configuration parameter */
} qosa_nw_cell_select_param_t;


/**
 * @struct qosa_nw_blacklist_common_settings_t
 * @brief Network blacklist common configuration structure
 *
 * This structure is used to configure the core common parameters of network blacklist in common scenarios
 */
typedef struct
{
    qosa_uint8_t                    enable;                /**< Blacklist master switch. 0-Disable (QOSA_NW_BLACKLIST_DISABLE), 1-Enable (QOSA_NW_BLACKLIST_ENABLE) */
    qosa_uint32_t                   timeout;               /*!< Auto-removal timeout of blacklisted cell, Unit: seconds */
    qosa_uint8_t                    stuck_enable;          /*!< Anti-sticking protection switch for blacklist, 0-Disable, 1-Enable */
    qosa_uint32_t                   stuck_timeout;         /*!< Weighted release timeout of blacklisted cell after no registration behavior, Unit: seconds */
    qosa_uint8_t                    cnt_mode;              /*!< Evaluation count mode for failure scenarios 1-3, 0-Consecutive count, 1-Cumulative count */
    qosa_uint8_t                    cnt_thresh;            /*!< Trigger count threshold for failure scenarios 1-3, Value range:1~255 */
    qosa_uint8_t                    osc_rounds;            /**< Required oscillation cycles for ping-pong handover detection, Threshold range:1~255 */
    qosa_uint32_t                   osc_cycle;             /**< Observation time window for ping-pong handover oscillation detection, Unit: seconds */
    qosa_uint8_t event_policy[QOSA_NW_QBLACKLIST_SCENE_MAX];      /**< See qosa_nw_blacklist_policy_type_e and qosa_nw_blacklist_scene_e. Blacklist trigger policy corresponding to each blacklist sence, combined configuration enumeration value */
} qosa_nw_blacklist_common_settings_t;

/**
 * @struct qosa_nw_blacklist_data_service_settings_t
 * @brief Network blacklist data service special configuration structure
 *
 * This structure is used to configure the personalized parameters of network blacklist in data service scenarios
 */
typedef struct
{
    qosa_uint8_t  enable;                /**< Data service blacklist function switch, 0-Disable, 1-Enable */
    qosa_uint32_t packet_cnt;            /**< Packet statistics threshold for data service failure */
    qosa_uint32_t packet_timeout;        /**< Packet timeout judgment time for data service, Unit: seconds */
    qosa_uint32_t cfun_thresh;           /**< Threshold for triggering CFUN switch in data service */
    qosa_uint8_t  blacklist_action;      /*!< Blacklist trigger action configuration (This field is not supported temporarily) */
    char          host[QOSA_NW_BLACKLIST_HOST_MAX_SIZE];  /**< Storage for HOST address/domain name corresponding to data service blacklist */
} qosa_nw_blacklist_data_service_settings_t;

/**
 * @struct qosa_nw_blacklist_param_t
 * @brief Network blacklist total configuration parameter structure
 *
 * This structure is the top-level configuration structure of network blacklist, which integrates the common general configuration and data service special configuration of network blacklist.
 */
typedef struct
{
    qosa_nw_blacklist_common_settings_t       common;        /**< Network blacklist common general configuration items */
    qosa_nw_blacklist_data_service_settings_t data_service;  /**< Network blacklist data service special configuration items */
} qosa_nw_blacklist_param_t;

/**
 * @struct qosa_nw_blacklist_cell_t
 * @brief blacklist cell basic info
 *
 */
typedef struct
{
    qosa_uint32_t            arfcn; /*!< E-UTRA carrier frequency(maxEARFCN2); 0..262143; invalid value: QOSA_NW_PI_UINT32 */
    qosa_uint16_t            pcid;   /*!< physical cell identity(PhysCellId); 0..503; invalid value: QOSA_NW_PI_UINT16 */
    qosa_nw_nas_event_type_e event;  /*!< Event that caused the blacklisting */
} qosa_nw_blacklist_cell_t;

/**
 * @struct qosa_nw_blacklist_lists_param_t
 * @brief blacklist cell parameter list
 *
 */
typedef struct
{
    qosa_bool_t                     valid;  /*!< Valid flag */
    qosa_nw_blacklist_cell_t        cell;   /*!< Blacklist cell info */
    qosa_nw_blacklist_policy_type_e policy; /*!< Policy that got from setting */
    qosa_time_t                     stime;  /*!< Time record */
} qosa_nw_blacklist_lists_param_t;

/**
 * @struct qosa_nw_blacklist_lists_t
 * @brief blacklist cell list config
 *
 */
typedef struct
{
    qosa_uint8_t                    num;                              /*!< Blacklist cell count */
    qosa_nw_blacklist_lists_param_t list[QOSA_NW_BLACKLIST_CELL_MAX]; /*!< Blacklist cell list */
} qosa_nw_blacklist_lists_t;

/**
 * @struct qosa_nw_blacklist_t
 * @brief network blacklist config
 *
 */
typedef struct
{
    qosa_nw_blacklist_lists_t lte;   /*!< LTE blacklist cell list */
    qosa_nw_blacklist_lists_t wcdma; /*!< WCDMA blacklist cell list */
    qosa_nw_blacklist_lists_t gsm;   /*!< GSM blacklist cell list */
    qosa_nw_blacklist_lists_t nr5g;  /*!< NR5G blacklist cell list */
} qosa_nw_blacklist_t;

/*-------------------------------------- cnf --------------------------------------*/

/* All cnf results need to ensure the first two parameters of the structure are simid and err_code, which is OSA_MODEM_COMMON_CNF_HDR, user layer does not need to release parameter content, after executing the callback, internally it will be released directly */

/**
 * @struct qosa_nw_general_cnf_t
 * @brief Network general result return, used to return simple success or failure, no additional result return type
 *
 */
typedef qosa_modem_general_cnf_t qosa_nw_general_cnf_t;

/**
 * @struct qosa_nw_get_ncell_ext_cnf_t
 * @brief Return result of getting extended neighbor cell information
 *
 */
typedef struct
{
    QOSA_MODEM_COMMON_CNF_HDR;  /*!< Common header */
    qosa_nw_ncell_info_t ncell; /*!< Neighbor cell information set */
} qosa_nw_get_ncell_ext_cnf_t;

/**
 * @struct qosa_nw_set_plmn_sertch_cnf_t
 * @brief Return result of setting plmn network selection
 *
 */
typedef struct
{
    QOSA_MODEM_COMMON_CNF_HDR; /*!< Common header */
    qosa_uint8_t mode;         /*!< plmn search mode */
} qosa_nw_set_plmn_sertch_cnf_t;

/**
 * @struct qosa_nw_get_supp_oper_cnf_t
 * @brief Return result of getting available operator list
 *
 */
typedef struct
{
    QOSA_MODEM_COMMON_CNF_HDR;                       /*!< Common header */
    qosa_uint8_t        oper_num;                    /*!< Operator list count */
    qosa_nw_oper_list_t list[QOSA_NW_SUPP_OPER_NUM]; /*!< Operator list */
} qosa_nw_get_supp_oper_cnf_t;

/**
 * @struct qosa_nw_trigger_freq_scan_cnf_t
 * @brief Return result of getting frequency band scan results
 *
 */
typedef struct
{
    QOSA_MODEM_COMMON_CNF_HDR;               /*!< Common header */
    qosa_nw_freq_scan_cell_list_t cell_list; /*!< Frequency scan results */
} qosa_nw_trigger_freq_scan_cnf_t;

/*-------------------------------------- api --------------------------------------*/

/**
 * @brief Get CS domain and PS domain reg registration status
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[out] cs_status
 *          - CS domain registration status, parameter reference qosa_nw_reg_status_e
 * @param[out] ps_status
 *          - PS domain registration status, parameter reference qosa_nw_reg_status_e
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_get_reg_status(qosa_uint8_t simid, qosa_uint8_t *cs_status, qosa_uint8_t *ps_status);

/**
 * @brief Get current cell's RAT, if not registered, return OSA_NW_RAT_UNKNOWN
 *
 * @param[in] simid
 *          - SIM card ID
 * @return qosa_nw_rat_e
 */
qosa_nw_rat_e qosa_nw_get_current_rat(qosa_uint8_t simid);

/**
 * @brief Get corresponding RAT according to ACT
 *
 * @param[in] act
 *          - Network RAT
 * @return qosa_nw_rat_e - RAT
 */
qosa_nw_rat_e qosa_nw_act2rat(qosa_nw_act_e act);

/**
 * @brief Get currently configured rat mode, representing currently configured rat mode, used for network selection, e.g., configured as GSM+LTE combination mode, note the difference with qosa_nw_get_current_rat.
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] rat_mode
 *          - Combination value of qosa_nw_rat_e, e.g., for 2G+4G combination mode, it is QOSA_NW_RAT_2G|QOSA_NW_RAT_4G;
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_rat_mode_from_nv(qosa_uint8_t simid, qosa_uint8_t *rat_mode);

/**
 * @brief Get configurable rat mode combination, e.g., current model is 3G+4G, then return QOSA_NW_RAT_3G|QOSA_NW_RAT_4G
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] rat_mode_ability
 *          - Combination value of qosa_nw_rat_e, e.g., for 2G+4G combination mode, it is QOSA_NW_RAT_2G|QOSA_NW_RAT_4G;
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_supported_rat_mode(qosa_uint8_t simid, qosa_uint8_t *rat_mode_ability);

/**
 * @brief Configure rat mode, result returned through `qosa_nw_general_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] rat_mode
 *          - Combination value of qosa_nw_rat_e, e.g., for 2G+4G combination mode, it is QOSA_NW_RAT_2G|QOSA_NW_RAT_4G; additionally, QOSA_NW_RAT_ALL can be used to represent all rat capabilities of the current model
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - Callback function context
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_set_rat_mode(qosa_uint8_t simid, qosa_uint8_t rat_mode, qosa_bool_t effect, nw_callback_t cb, void *ctx);

/**
 * @brief Get rat order during network selection
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] rat_order
 *          - RAT order, array length is QOSA_NW_RAT_SEQ_NUM, array elements are osa_nw_rat_e type.
 *            The valid sequence ends with QOSA_NW_RAT_UNKNOWN, priority: rat_order[0] > rat_order[1] > ... > the element before QOSA_NW_RAT_UNKNOWN
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_rat_order(qosa_uint8_t simid, qosa_nw_rat_e *rat_order);

/**
 * @brief Set rat order during network selection, result returned through `qosa_nw_general_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] rat_order
 *          - RAT order, array length is QOSA_NW_RAT_SEQ_NUM, array elements are osa_nw_rat_e type.
 *            The valid sequence ends with QOSA_NW_RAT_UNKNOWN, priority: rat_order[0] > rat_order[1] > ... > the element before QOSA_NW_RAT_UNKNOWN
 * @param[in] effect
 *          - Whether to take effect immediately, if false, need to restart to make configuration take effect
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - Callback function context
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_set_rat_order(qosa_uint8_t simid, qosa_nw_rat_e *rat_order, qosa_bool_t effect, nw_callback_t cb, void *ctx);

/**
 * @brief Get serving cell information
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] scell
 *          - Serving cell information
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_scell_info(qosa_uint8_t simid, qosa_nw_scell_info_t *scell);

/**
 * @brief Get neighbor cell information (not including plmn, cid information, not decoding neighbor cell SIB messages)
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] ncell
 *          - Neighbor cell content
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_ncell_basic(qosa_uint8_t simid, qosa_nw_ncell_info_t *ncell);

/**
 * @brief Used to get neighbor cell information when there is no card or in idle state (decoding SIB messages), result returned through `qosa_nw_get_ncell_ext_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] config
 *          - Extended configuration parameters
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - Callback function context
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_ncell_ext(qosa_uint8_t simid, qosa_nw_ncell_ext_config_t *config, nw_callback_t cb, void *ctx);

#if 0
/**
 * @brief Get currently registered network PLMN information
 *
 * @param[out] plmn
 *          - PLMN information
 * @return qosa_nw_err_e
 */
qosa_nw_err_e osa_nw_get_current_oper_plmn(qosa_uint8_t simid, qosa_plmn_t *plmn);
#endif

/**
 * @brief Convert PLMN to string format
 *
 * @param[in] plmn
 *          - PLMN to be converted
 * @param[out] plmn_str_buf
 *          - String buffer provided by caller, length must be `QOSA_NW_PLMN_STRLEN_MAX`, used to store converted string.
 * @return char* Output string, return empty string if conversion fails
 */
char *qosa_nw_plmn_stoa(qosa_plmn_t *plmn, char *plmn_str_buf);

/**
 * @brief Convert string format PLMN to osa_plmn_t format
 *
 * @param[in] plmn_str
 *          - Input PLMN string, format such as "46001" or "460001"
 * @param[out] plmn
 *          - Output PLMN structure
 * @return int
 *          - 0: Conversion successful
 *          - -1: Conversion failed
 */
int qosa_nw_plmn_atos(const char *plmn_str, qosa_plmn_t *plmn);

/**
 * @brief Convert operator name to string format, note, only supports ASCII type operator names, does not support other character sets
 *
 * @param[in] oper_name
 *          - Operator name to be converted
 * @param[out] oper_name_str_buf
 *          - String buffer provided by caller, length must be `QOSA_NW_NETWORK_NAME_LEN_MAX`, used to store converted string.
 * @return char* Output string, return QOSA_NULL if conversion fails
 */
char *qosa_nw_oper_name_stoa(qosa_nw_oper_name_t *oper_name, char *oper_name_str_buf);

/**
 * @brief Get currently registered network PLMN information
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] long_name
 *          - Long network operator name
 * @param[out] short_name
 *          - Short network operator name
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_oper_name(qosa_uint8_t simid, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);

/**
 * @brief Get ME configured operator name information, need to specify PLMN
 *
 * @param[in] plmn
 *          - Operator PLMN
 * @param[out] long_name
 *          - Long network operator name
 * @param[out] short_name
 *          - Short network operator name
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_cfg_oper_name(qosa_plmn_t *plmn, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);

/**
 * @brief Get ME configured operator name information, can be used with for loop traversal
 *
 * @param[in] index
 *          - Index number, starting from 0
 * @param[out] plmn
 *          - Operator PLMN
 * @param[out] long_name
 *          - Long network operator name
 * @param[out] short_name
 *          - Short network operator name
 * @return qosa_nw_err_e
 *          - Success: QOSA_NW_ERR_OK
 *          - Index out of range: QOSA_NW_ERR_OPERATION_NOT_ALLOWED
 *          - Others: Return corresponding error code
 */
qosa_nw_err_e qosa_nw_each_cfg_oper_name(qosa_uint32_t index, qosa_plmn_t *plmn, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);

/**
 * @brief Do PLMN scan, get supported operator information, result returned through `qosa_nw_get_supp_oper_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - Callback function context
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_query_supp_operator(qosa_uint8_t simid, nw_callback_t cb, void *ctx);

/**
 * @brief Do automatic network search, result returned through `qosa_nw_general_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - Callback function context
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_auto_register(qosa_uint8_t simid, nw_callback_t cb, void *ctx);

/**
 * @brief Do manual network search, result returned through `qosa_nw_set_plmn_sertch_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] format
 *          - Operator string format
 * @param[in] plmn_str
 *          - Operator string, can be PLMN string mode or operator name
 * @param[in] act
 *          - Network RAT
 * @param[in] fail_to_auto
 *          - Whether to switch to automatic mode and retry again after manual network selection fails
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - Callback function context
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_manual_register(
    qosa_uint8_t               simid,
    qosa_nw_oper_name_format_e format,
    char                      *plmn_str,
    qosa_nw_act_e              act,
    qosa_bool_t                fail_to_auto,
    nw_callback_t              cb,
    void                      *ctx
);

/**
 * @brief Do network deregistration, both CS and PS domains are deregistered, result returned through `qosa_nw_general_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - Callback function context
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_deregister(qosa_uint8_t simid, nw_callback_t cb, void *ctx);

/**
 * @brief Get currently configured band information
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] cfg_band
 *          - Configured band information
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_cfg_band(qosa_uint8_t simid, qosa_nw_band_t *cfg_band);

/**
 * @brief Get supported band information
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] support_band
 *          - Supported band list
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_support_band(qosa_uint8_t simid, qosa_nw_band_t *support_band);

/**
 * @brief Configure band, result returned through `qosa_nw_general_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] cfg_band
 *          - User configured band information
 * @param[in] effect
 *          - Whether to take effect immediately
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - Callback function context
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_set_cfg_band(qosa_uint8_t simid, const qosa_nw_band_t *cfg_band, qosa_bool_t effect, nw_callback_t cb, void *ctx);

/**
 * @brief Get RRC connection information
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] conn_info
 *          - RRC connection information
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_get_rrc_conn_info(qosa_uint8_t simid, qosa_nw_rrc_conn_info_t *conn_info);

/**
 * @brief Trigger frequency scan, result returned through `qosa_nw_trigger_freq_scan_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] config
 *          - Scan configuration parameters
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - Context
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_trigger_freq_scan(qosa_uint8_t simid, qosa_nw_freq_scan_t *config, nw_callback_t cb, void *ctx);

/**
 * @brief Query packet domain/circuit domain registration mode information
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] domain
 *          - Packet domain/circuit domain registration mode parameter reference qosa_nw_srv_domain_e
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_get_srv_domain(qosa_uint8_t simid, qosa_uint8_t *domain);

/**
 * @brief Set packet domain/circuit domain registration mode
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] domain
 *          - Packet domain/circuit domain registration mode parameter reference qosa_nw_srv_domain_e
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - User context
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_set_srv_domain(qosa_uint8_t simid, qosa_uint8_t domain, nw_callback_t cb, void *ctx);

/**
 * @brief Get voice domain registration mode information
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] domain
 *          - Voice domain registration mode parameter reference qosa_nw_voice_domain_e
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_get_voice_domain(qosa_uint8_t simid, qosa_uint8_t *domain);

/**
 * @brief Set voice domain registration mode, result returned through `qosa_nw_general_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] domain
 *          - Voice domain registration mode parameter reference qosa_nw_voice_domain_e
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - User context
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_set_voice_domain(qosa_uint8_t simid, qosa_uint8_t domain, nw_callback_t cb, void *ctx);

/**
 * @brief Get UE's usage setting
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] setting
 *          - UE usage setting, parameter reference qosa_nw_ue_usage_setting_e
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_get_ue_usage_setting(qosa_uint8_t simid, qosa_uint8_t *setting);

/**
 * @brief Set UE's usage setting, result returned through `qosa_nw_general_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] setting
 *          - UE usage setting, parameter reference qosa_nw_ue_usage_setting_e
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - User context
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_set_ue_usage_setting(qosa_uint8_t simid, qosa_uint8_t setting, nw_callback_t cb, void *ctx);

/**
 * @brief Get roaming preference setting
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] setting
 *          - Roaming preference setting, parameter reference qosa_nw_roaming_pref_e
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_get_roaming_pref(qosa_uint8_t simid, qosa_uint8_t *setting);

/**
 * @brief Set roaming preference setting, result returned through `qosa_nw_general_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] setting
 *          - Roaming preference setting, parameter reference qosa_nw_roaming_pref_e
 * @param[in] effect
 *          - Effect mode, 0-take effect after restart, 1-take effect immediately
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - User context
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_set_roaming_pref(qosa_uint8_t simid, qosa_uint8_t setting, qosa_uint8_t effect, nw_callback_t cb, void *ctx);

/**
 * @brief Set frequency lock and unlock, result returned through `qosa_nw_general_cnf_t`
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] rat
 *          - Frequency lock operation RAT type
 * @param[in] opcode
 *          - qosa_nw_freq_lock_opcode_e Frequency lock operation code, lock frequency band or unlock
 * @param[in] freq_lock
 *          - Frequency lock list
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - User context
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_freq_lock(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t opcode, qosa_nw_freq_lock_list_t *freq_lock, nw_callback_t cb, void *ctx);

/**
 * @brief Get frequency lock configuration
 * @note If the returned num is 0, it means no lock, if the pci in the returned list is invalid value, it means no physical cell is locked
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] rat
 *          - Frequency lock operation RAT type
 * @param[out] freq_lock
 *          - Frequency lock list
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_get_freq_lock_config(qosa_uint8_t simid, qosa_uint8_t rat, qosa_nw_freq_lock_list_t *freq_lock);

/**
 * @brief Set default configured NSSAI
 *
 * @param[in] simid
 *          - SIM card ID
 *          - 0 represents SIM card 1, 1 represents SIM card 2
 * @param[in] length
 *          - Length of NSSAI, number of bytes in NSSAI data, not nssai_data string length
 * @param[in] nssai_data
 *          - NSSAI configuration data
 *          - String form, multiple S-NSSAI separated by colon `:`
 * @return qosa_nw_err_e
 *          - Success returns 0
 *          - Failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_nssai_set_default_config(qosa_uint8_t simid, qosa_uint8_t length, const char *nssai_data);

/**
 * @brief Get default configured NSSAI
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[out] length
 *          - Length of NSSAI, number of bytes in NSSAI data
 * @param[out] nssai_data
 *          - Used to store currently configured NSSAI data
 *          - Caller needs to allocate buffer, buffer size should not be less than QOSA_NSSAI_MAX_LEN
 * @return qosa_nw_err_e
 *          - Success returns 0
 *          - Failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_nssai_get_default_config(qosa_uint8_t simid, qosa_uint8_t *length, char *nssai_data);

/**
 * @brief Clear default configured NSSAI
 *
 * @param[in] simid
 *          - SIM card ID
 * @return qosa_nw_err_e
 *          - Success returns 0
 *          - Failure returns corresponding error code
 */
qosa_nw_err_e qosa_nw_nssai_clear_default_config(qosa_uint8_t simid);

/**
 * @brief Get NSSAI configuration
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] type
 *          - NSSAI configuration type, reference qosa_nw_nssai_type_e
 * @param[in] plmn
 *          - Specified plmn, if OSA_NULL, means no PLMN information
 * @param[out] list
 *          - Returned NSSAI configuration list
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_nssai_get_nssai(qosa_uint8_t simid, qosa_nw_nssai_type_e type, qosa_plmn_t *plmn, qosa_nw_nssai_list_t *list);

/**
 * @brief Set rrc fast release configuration
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] fast_release
 *          - Rrc fast release configuration
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - User context
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_set_rrc_fast_release(qosa_uint8_t simid, qosa_nw_rrc_fast_release_t *fast_release, nw_callback_t cb, void *ctx);

/**
 * @brief Get rrc fast release configuration
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] fast_release
 *          - Rrc fast release configuration
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_rrc_fast_release(qosa_uint8_t simid, qosa_nw_rrc_fast_release_t *fast_release);

/**
 * @brief One-time rrc fast release
 *
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_rrc_fast_release(qosa_uint8_t simid);

/**
 * @brief Get corresponding cause string according to reject type and reject cause id
 *
 * @param[in] type
 *          - Reject type
 * @param[in] reject_cause
 *          - Reject cause id
 * @param[out] cause_string
 *          - Corresponding cause string format information
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_reject_cause_string(qosa_nw_reject_type_e type, qosa_uint16_t reject_cause, char *cause_string);

/**
 * @brief Get reject event information
 *
 * @param[in] simid
 *          - sim id
 * @param[out] reject_info
 *          - reject infomation
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_reject_cause_event_info(qosa_uint8_t simid,qosa_nw_reject_info_t *reject_info);

/**
 * @brief Set Jamming Detection function
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] jdc_func
 *          - Jamming Detection parameters
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - Callback function context
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_set_jamm_detect_func(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *jdc_func, nw_callback_t cb, void *ctx);

/**
 * @brief Get Jamming Detection function
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] jdc
 *          - Jamming Detection parameters
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_jamm_detect_func(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *jdc_func);

/**
 * @brief Set Jamming Detection parameters
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] jdc_config
 *          - Jamming Detection parameters
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_set_jamm_detect_param(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *jdc_config);

/**
 * @brief Get Jamming Detection parameters
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] all_param
 *          - Jamming Detection parameters
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_jamm_detect_param(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *all_param);

/**
 * @brief Get as event if occured
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] as_event_info
 *          - as event infomation
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_as_event(qosa_uint8_t simid,qosa_nw_as_event_t *as_event_info);

/**
 * @brief Whether serving cell is changed in the past 60s
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] result
 *          - execute result
 *
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_scell_is_changed(qosa_uint8_t simid,qosa_bool_t *result);

/**
 * @brief Get the base station timing results corresponding to the sim card
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @return qosa_bool_t
 */
qosa_bool_t qosa_nw_nitz_is_ready(void);

/**
 * @brief Set UE operation mode
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[in] mode
 *          - UE operation mode, parameter reference qosa_nw_ue_operation_mode_e
 * @param[in] cb
 *          - Callback function
 * @param[in] ctx
 *          - User context
 * @return qosa_nw_err_e Success returns OSA_NW_ERR_OK, failure returns corresponding error code

 */
qosa_nw_err_e qosa_nw_set_ue_operation_mode(qosa_uint8_t simid,qosa_uint8_t mode,nw_callback_t cb, void* ctx);

/**
 * @brief Get UE operation mode
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] mode
 *          - execute result
 *
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_ue_operation_mode(qosa_uint8_t simid,qosa_uint8_t *mode);

/**
 * @brief Set UE blacklist parameters
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[in] all_param
 *          - parameter reference qosa_nw_blacklist_param_t
 *
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_set_blacklist_param(qosa_uint8_t simid, qosa_nw_blacklist_param_t *all_param);

/**
 * @brief Get UE blacklist parameters
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] all_param
 *          - parameter reference qosa_nw_blacklist_param_t
 *
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_blacklist_param(qosa_uint8_t simid, qosa_nw_blacklist_param_t *all_param);

/**
 * @brief Set UE cell selection parameters
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] all_param
 *          - parameter reference qosa_nw_cell_select_param_t
 *
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_set_cell_select_param(qosa_uint8_t simid, qosa_nw_cell_select_param_t *all_param);

/**
 * @brief Get UE cell selection parameters
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] all_param
 *          - parameter reference qosa_nw_cell_select_param_t
 *
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_nw_get_cell_select_param(qosa_uint8_t simid, qosa_nw_cell_select_param_t *all_param);

/**
 * @brief Set IMS mode configuration
 *
 * This function configures the IMS mode for the specified SIM card, including IMS enable setting and VoLTE capability flag
 *
 * @param[in] simid
 *          - Identifier of the SIM card to configure
 *
 * @param[in] ims_conf
 *          - IMS configuration value (e.g., enable or disable)
 *
 * @param[in] volte_cap
 *          - Flag indicating whether VoLTE capability is supported
 *
 * @return qosa_nw_err_e
 *       - Status code indicating the result of the operation (e.g., success or specific error type)
 */
qosa_nw_err_e qosa_set_ims_mode(qosa_uint8_t simid, qosa_uint8_t ims_conf);

/**
 * @brief Get IMS mode configuration
 *
 * This function retrieves the current IMS mode settings for the specified SIM card, including IMS enable status and VoLTE capability
 *
 * @param[in] simid
 *          - Identifier of the SIM card to query
 *
 * @param[out] ims_conf
 *
 *          - Pointer to a variable that will receive the IMS configuration value
 * @param[out] volte_cap
 *          - Pointer to a variable that will receive the VoLTE capability flag
 *
 * @return qosa_nw_err_e
 *       - Status code indicating the result of the operation (e.g., success or specific error type)
 */
qosa_nw_err_e qosa_get_ims_mode(qosa_uint8_t simid, qosa_uint8_t *ims_conf, qosa_bool_t *volte_cap);
/**
 * @brief Get UE autoapn status
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] all_param
 *          - parameter reference qosa_nw_cell_select_param_t
 *
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_qautoapn_enable_get(qosa_uint8_t simid, qosa_bool_t *enable);
/**
 * @brief Set UE autoapn status
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] all_param
 *          - parameter reference qosa_nw_cell_select_param_t
 *
 * @return qosa_nw_err_e
 */
qosa_nw_err_e qosa_qautoapn_enable_set(qosa_uint8_t simid, qosa_bool_t enable);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_NETWORK_H__ */
