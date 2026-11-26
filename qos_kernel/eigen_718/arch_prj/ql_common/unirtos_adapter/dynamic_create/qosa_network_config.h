/* Copyright (C) 2020 UNIRTOS Technologies Limited and/or its affiliates("UNIRTOS").
 * All rights reserved.
 */

#ifndef __QOSA_NETWORK_CONFIG__H__
#define __QOSA_NETWORK_CONFIG__H__

/**
 * whether support GSM or not
 */
/* #undef CONFIG_QOSA_NW_GSM_SUPPORT */
/**
 * whether whether support WCDMA or not
 */
/* #undef CONFIG_QOSA_NW_WCDMA_SUPPORT */
/**
 * whether whether support LTE or not
 */
#define CONFIG_QOSA_NW_LTE_SUPPORT
/**
 * whether whether support NR or not
 */
/* #undef CONFIG_QOSA_NW_NR_SUPPORT */
/**
 * is LTE only module?
 */
#define CONFIG_QOSA_NW_LTE_ONLY
/**
 * whether nw reject cause is supported
 */
#define CONFIG_QOSA_NW_REJECT_CAUSE_SUPPORT

/**
 * UniRTOS nw nb support
 */
/* #undef CONFIG_QOSA_NW_NB_SUPPORT */

/**
 * UniRTOS nw bju support
 */
/* #undef CONFIG_QOSA_NW_BJU_SUPPORT */

/** 配置干扰检测shakeperiod的默认值，各个平台不同 */
/** Configure the default number of Jamming detection for shakeperiod */
#define CONFIG_UNIRTOS_JAMM_DETECT_SHAKEPERIOD_DEFAULT 3

/** 配置干扰检测rssi的默认值，各个平台不同 */
/** Configure the default number of Jamming detection for rssi */
#define CONFIG_UNIRTOS_JAMM_DETECT_RSSI_DEFAULT -40

/** 配置干扰检测rssi的最大值，各个平台不同 */
/** Configure the maximum number of Jamming detection for rssi */
#define CONFIG_UNIRTOS_JAMM_DETECT_RSSI_MAX -30

/** 配置干扰检测rssi的最小值，各个平台不同 */
/** Configure the minimum number of Jamming detection for rssi */
#define CONFIG_UNIRTOS_JAMM_DETECT_RSSI_MIN -90

/** 配置干扰检测rsrq的默认值，各个平台不同 */
/** Configure the default number of Jamming detection for rsrq */
#define CONFIG_UNIRTOS_JAMM_DETECT_RSRQ_DEFAULT -15

/** 配置干扰检测rsrq的最大值，各个平台不同 */
/** Configure the maximum number of Jamming detection for rsrq */
#define CONFIG_UNIRTOS_JAMM_DETECT_RSRQ_MAX 0

/** 配置干扰检测rsrq的最小值，各个平台不同 */
/** Configure the minimum number of Jamming detection for rsrq */
#define CONFIG_UNIRTOS_JAMM_DETECT_RSRQ_MIN -20

/** 配置干扰检测rsrp的默认值，各个平台不同 */
/** Configure the default number of Jamming detection for rsrp */
#define CONFIG_UNIRTOS_JAMM_DETECT_RSRP_DEFAULT -105

/** 配置干扰检测sinr的默认值，各个平台不同 */
/** Configure the default number of Jamming detection for sinr */
#define CONFIG_UNIRTOS_JAMM_DETECT_SINR_DEFAULT -7

/** 配置干扰检测sinr的最大值，各个平台不同 */
/** Configure the maximum number of Jamming detection for sinr */
#define CONFIG_UNIRTOS_JAMM_DETECT_SINR_MAX 30

/** 配置干扰检测sinr的最小值，各个平台不同 */
/** Configure the minimum number of Jamming detection for sinr */
#define CONFIG_UNIRTOS_JAMM_DETECT_SINR_MIN -50

/** Configure the modem task stack size */
#define CONFIG_QOSA_MODEM_TASK_STACK_SIZE 16


/**UNIRTOS Region Division Operator */
#define CONFIG_QOSA_OPER_ASIA_AREA
/* #undef CONFIG_QOSA_OPER_EUROPE_AREA */
/* #undef CONFIG_QOSA_OPER_NORTH_AMERICA_AREA */
/* #undef CONFIG_QOSA_OPER_LATIN_AMERICA_AREA */
/* #undef CONFIG_QOSA_OPER_OCEANIA_AREA */
/* #undef CONFIG_QOSA_OPER_GLOBAL_AREA */

/* Add Starlink EHPLMNs for SD2C */
/* #undef CONFIG_QOSA_NW_STARLINK_EHPLMN */

#endif
