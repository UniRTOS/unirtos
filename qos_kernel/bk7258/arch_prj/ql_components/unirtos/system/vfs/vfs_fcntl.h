/*****************************************************************//**
* @file qosa_virtual_file_ops.h
* @brief
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
#ifndef __VFS_FCNTL_H__
#define __VFS_FCNTL_H__

#if !(defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
/** 以只读方式打开文件 */
#define O_RDONLY    0
/** 以只写方式打开文件 */
#define O_WRONLY    1
/** 以可读写方式打开文件。上述三种旗标是互斥的，也就是不可同时使用，但可与下列的旗标利用OR(|)运算符组合。 */
#define O_RDWR      2
/** 若欲打开的文件不存在则自动建立该文件。 */
#define O_CREAT     0x0200
/**
 * 如果O_CREAT 也被设置，此指令会去检查文件是否存在。文件若不存在则建立该文件，
 * 否则将导致打开文件错误。此外，若O_CREAT与O_EXCL同时设置，并且欲打开的文件为
 * 符号连接，则会打开文件失败。
 */
#define O_EXCL      0x0800
/** 如果欲打开的文件为终端机设备时，则不会将该终端机当成进程控制终端机。 */
#define O_NOCTTY    0x8000
/** 若文件存在并且以可写的方式打开时，此旗标会令文件长度清为0，而原来存于该文件的资料也会消失。 */
#define O_TRUNC     0x0400
/** 当读写文件时会从文件尾开始移动，也就是所写入的数据会以附加的方式加入到文件后面。 */
#define O_APPEND    0x0008
/** 以同步的方式打开文件。 */
#define O_SYNC      0x2000
/** 如果参数pathname 所指的文件为一符号连接，则会令打开文件失败。 */
#define O_NOFOLLOW  0x100000
/** 如果参数pathname 所指的文件并非为一目录，则会令打开文件失败。 */
#define O_DIRECTORY 0x200000
#else

#include <fcntl.h>
#endif


#endif /* __VFS_FCNTL_H__ */

