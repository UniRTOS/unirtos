/* Copyright (C) 2020 UNIRTOS Technologies Limited and/or its affiliates("UNIRTOS").
 * All rights reserved.
 */

#ifndef __QOSA_SIM_CONFIG__H__
#define __QOSA_SIM_CONFIG__H__

/**
 * whether facility AB is supported
 */
/* #undef CONFIG_QOSA_SIM_FACILITY_AB */

/**
 * whether facility AB is supported
 */
/* #undef CONFIG_QOSA_SIM_FACILITY_AC */

/**
 * whether facility AB is supported
 */
/* #undef CONFIG_QOSA_SIM_FACILITY_AG */

/**
 * whether facility AB is supported
 */
/* #undef CONFIG_QOSA_SIM_FACILITY_AI */

/**
 * whether facility AB is supported
 */
/* #undef CONFIG_QOSA_SIM_FACILITY_AO */

/**
 * whether facility AB is supported
 */
/* #undef CONFIG_QOSA_SIM_FACILITY_IR */

/**
 * whether facility AB is supported
 */
/* #undef CONFIG_QOSA_SIM_FACILITY_OI */

/**
 * whether facility AB is supported
 */
/* #undef CONFIG_QOSA_SIM_FACILITY_OX */

/**
 * whether facility AB is supported
 */
#define CONFIG_QOSA_SIM_FACILITY_SC

/**
 * whether facility AB is supported
 */
/* #undef CONFIG_QOSA_SIM_FACILITY_P2 */

/**
 * whether replaced 0 by 0xff, for CRSM P3
 */
/* #undef CONFIG_QOSA_SIM_CRSM_P3_ZERO_REPLACED_BY_FF */

/**
 * the number of logical channel
 */
#define CONFIG_QOSA_SIM_LOGICAL_CHANNEL_NUM 8

/**
 * whether get sim/sms/pbk init status data from MPC
 * 
 */
/* #undef CONFIG_QOSA_SIM_DIFF_USE_MPC_SIM_INIT */

/**
 * whether SIM card hot-swap (insertion) detection is supported (AT+QSIMDET)
 */
/* #undef CONFIG_QOSA_SIM_DET_SUPPORT */

#endif /* __SIM_CONFIG__H__ */
