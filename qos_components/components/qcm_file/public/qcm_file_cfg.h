/*****************************************************************/ /**
* @file qcm_file_cfg.h
* @brief
* @author larson.li@quectel.com
* @date 2023-06-09
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-06-09 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QCM_FILE_CFG_H__
#define __QCM_FILE_CFG_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qcm_proj_config.h"
// clang-format off

//********************* unirtos config nv file definition *******************/

#define QCM_FACTORYNV_DIR_ROOT      "/factory"
#define QCM_FACTORYNV_DIR_ROOT_LEN  8


//********************** unirtos user file definition ***********************/
// QCM_USER_DIR_ROOT is the directory for user file
// under QCM_USER_DIR_ROOT, you should only use unir_vfs_xxxxx interface to do file access.
/** QCM_USER_DIR_ROOT defined according to the platform file system */
#if defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC)\
 || defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_UIS8852_PLATFORM_FUNC)\
 || defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)\
 || defined(CONFIG_QOSA_UIS8360S_PLATFORM_FUNC) || defined(CONFIG_QOSA_XY4100_PLATFORM_FUNC)\
 || defined(CONFIG_QOSA_UIS8910_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
#define QCM_USER_DIR_ROOT      "/user/"
#define QCM_USER_DIR_ROOT_LEN  6  //only check first 5 characters
#else /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */
#define QCM_USER_DIR_ROOT      "/"
#define QCM_USER_DIR_ROOT_LEN  1  //only check first 5 characters
#endif /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */

#define QCM_SFS_FILE_ROOT_DIR  "/qsfs/"
#define QCM_SFS_FILE_ROOT_DIR_LEN 6

/** data root directory folder */
#define QCM_DATA_FILE_ROOT_DIR "/data/"
#define QCM_DATA_FILE_ROOT_DIR_LEN 6

/** ext nor flash root directory folder */
#define QCM_EXT_FILE_ROOT_DIR  "/extnor/"
#define QCM_EXT_FILE_ROOT_DIR_LEN 8

#define QCM_RUNNING_NV_DIR     "/modemnvm"
#define QCM_RUNNING_NV_DIR_LEN 9

/** FOTA root directory folder */
#define QCM_OPEN_FOTA_ROOT_DIR "/fota/"
#define QCM_OPEN_FOTA_ROOT_DIR_LEN 6

//this file is stored in the directory of OSA_OPEN_NVM_ROOT_DIR, because it is called by ql_nvm_fread/ql_nvm_fwrite, there is no need to add the directory
#define QCM_OPEN_CUST_NVM      "cust_nvm.nv"
#define QCM_OPEN_FAC_CUST_NVM  QCM_FACTORYNV_DIR_ROOT"/fac_cust_nvm.nv"

/** SD root directory folder */
#define QCM_SD1_ROOT_DIR  "/sdcard0/"
#define QCM_SD1_ROOT_DIR_LEN 9

// clang-format on

#endif /* __QCM_FILE_CFG_H__ */
