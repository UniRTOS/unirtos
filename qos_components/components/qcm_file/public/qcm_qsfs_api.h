/*****************************************************************/ /**
* @file qcm_qsfs_api.h
* @brief
* @author larson.li@quectel.com
* @date 2024-04-21
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-04-21 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QCM_QSFS_API_H__
#define __QCM_QSFS_API_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_virtual_file.h"

#define QCM_SFS_DEBUG_LEVEL0 0  // Close
#define QCM_SFS_DEBUG_LEVEL1 1  // Print Log
#define QCM_SFS_DEBUG_LEVEL2 2  // STD Can Read SFS and Print Log
#define QCM_SFS_DEBUG_LEVEL  QCM_SFS_DEBUG_LEVEL1

int qcm_sfs_file_proc_info_init(int fd, int mode);

void qcm_sfs_file_proc_info_delete(int fd);

void qcm_sfs_file_set_done_flag(int fd, qosa_bool_t flag);

int qcm_sfs_file_set_aes_key(void *key, qosa_uint8_t len);

int qcm_sfs_fstat(int fd, char *fname, char *path, struct qosa_vfs_stat_t *st);

qosa_ssize_t qcm_sfs_write(int fd, const void *data, qosa_size_t size);

qosa_ssize_t qcm_sfs_read(int fd, void *dst, qosa_size_t size);

void qcm_sfs_file_init(void);

#endif /* __QCM_QSFS_API_H__ */
