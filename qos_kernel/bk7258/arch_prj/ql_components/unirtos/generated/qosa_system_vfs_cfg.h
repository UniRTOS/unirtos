/*****************************************************************//**
* @file qosa_system_vfs_cfg.h
* @brief Virtual File System configuration
* @author larson.li@quectel.com
* @date 2023-06-01
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-06-01 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_SYSTEM_VFS_CFG_H__
#define __QOSA_SYSTEM_VFS_CFG_H__

// Configure file operation related header files called by different platforms, such as linux <sys/stat.h>, if not available, please configure as <stdio.h>
#define CONFIG_QOSA_VFS_SYS_STAT_H <sys/stat.h>

// Configure file control related header files for different platforms, such as linux <sys/fcntl.h>
#define CONFIG_QOSA_VFS_SYS_FCNTL_H <vfs_fcntl.h>

#define CONFIG_QOSA_VFS_O_RDONLY O_RDONLY

#define CONFIG_QOSA_VFS_O_WRONLY O_WRONLY

#define CONFIG_QOSA_VFS_O_RDWR O_RDWR

#define CONFIG_QOSA_VFS_O_CREAT O_CREAT

#define CONFIG_QOSA_VFS_O_EXCL O_EXCL

#define CONFIG_QOSA_VFS_O_NOCTTY O_NOCTTY

#define CONFIG_QOSA_VFS_O_TRUNC O_TRUNC

#define CONFIG_QOSA_VFS_O_APPEND O_APPEND

#define CONFIG_QOSA_VFS_O_NONBLOCK O_NONBLOCK

#define CONFIG_QOSA_VFS_O_NDELAY O_NDELAY

#define CONFIG_QOSA_VFS_O_SYNC O_SYNC

#define CONFIG_QOSA_VFS_O_NOFOLLOW O_NOFOLLOW

#define CONFIG_QOSA_VFS_O_DIRECTORY O_DIRECTORY

#define CONFIG_QOSA_VFS_S_IRWXU S_IRWXU

#define CONFIG_QOSA_VFS_S_IRUSR S_IRUSR

#define CONFIG_QOSA_VFS_S_IWUSR S_IWUSR

#define CONFIG_QOSA_VFS_S_IXUSR S_IXUSR

#define CONFIG_QOSA_VFS_S_IRWXG S_IRWXG

#define CONFIG_QOSA_VFS_S_IRGRP S_IRGRP

#define CONFIG_QOSA_VFS_S_IWGRP S_IWGRP

#define CONFIG_QOSA_VFS_S_IXGRP S_IXGRP

#define CONFIG_QOSA_VFS_S_IRWXO S_IRWXO

#define CONFIG_QOSA_VFS_S_IROTH S_IROTH

#define CONFIG_QOSA_VFS_S_IWOTH S_IWOTH

#define CONFIG_QOSA_VFS_S_IXOTH S_IXOTH

#define CONFIG_QOSA_VFS_S_IFDIR S_IFDIR

#define CONFIG_QOSA_VFS_S_IFREG S_IFREG

#define CONFIG_QOSA_VFS_SEEK_SET SEEK_SET

#define CONFIG_QOSA_VFS_SEEK_CUR SEEK_CUR

#define CONFIG_QOSA_VFS_SEEK_END SEEK_END

#endif /* __QOSA_SYSTEM_VFS_CFG_H__ */
