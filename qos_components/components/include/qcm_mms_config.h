/********************************************************************
* @file qcm_mms_config.h
* @brief
* @author lawrence.liu@quectel.com
* @date 2024-02-28
*
* @copyright Copyright (c) 2024 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-02-28 <td>1.0 <td>lawrence.liu <td> Init
* </table>
**********************************************************************/
#ifndef __QCM_MMS_CONFIG_H__
#define __QCM_MMS_CONFIG_H__

#include "qosa_def.h"

#define QCM_MMS_TASK_PRIO                        QOSA_PRIORITY_NORMAL
#define QCM_MMS_MAX_SEND_LEN                     3 * 1024
#define QCM_MMS_MSGQ_MAX_CNT                     20

#define QCM_MMS_SEND_MAX_LEN                     (3 * 1024)

#define QCM_MMS_MIN_PROXY_PORT                   1
#define QCM_MMS_MAX_PROXY_PORT                   65535

#define QCM_MMS_MIN_VALIDITY                     0
#define QCM_MMS_MAX_VALIDITY                     6

#define QCM_MMS_MIN_PRIORITY                     0
#define QCM_MMS_MAX_PRIORITY                     3

#define QCM_MMS_MIN_SEND_RPT                     0
#define QCM_MMS_MAX_SEND_RPT                     1

#define QCM_MMS_MIN_READ_RPT                     0
#define QCM_MMS_MAX_READ_RPT                     1

#define QCM_MMS_MIN_VISIBLE                      0
#define QCM_MMS_MAX_VISIBLE                      2

#define QCM_MMS_MIN_MESSAGE_CLASS                0
#define QCM_MMS_MAX_MESSAGE_CLASS                4

#define QCM_MMS_MIN_PDP_CONNECT_TIMEOUT          1
#define QCM_MMS_MAX_PDP_CONNECT_TIMEOUT          125
#define QCM_MMS_DEFAULT_SEND_TIMEOUT             60

/**
 * @brief Default parameter configuration.
 */
#define QCM_MMS_DEFAULT_PDP_CID                  1
#define QCM_MMS_DEFAULT_URL                      "http://mmsc.monternet.com"
#define QCM_MMS_DEFAULT_PROXY                    "10.0.0.172"
#define QCM_MMS_DEFAULT_PROXY_PORT               80
#define QCM_MMS_DEFAULT_CHARSET                  0x03
#define QCM_MMS_DEFAULT_VALIDITY                 6
#define QCM_MMS_DEFAULT_PRIORITY                 3
#define QCM_MMS_DEFAULT_SEND_RPT                 0
#define QCM_MMS_DEFAULT_READ_RPT                 0
#define QCM_MMS_DEFAULT_VISIBLE                  2
#define QCM_MMS_DEFAULT_MESSAGE_CLASS            4
#define QCM_MMS_DEFAULT_CONNECT_TIMEOUT          125

#define QCM_MMS_HTTP_DOMAIN_NAME_LENGTH          150

/** Maximum number of addresses for MMS.*/
#define QCM_MMS_ADDRESS_MAX_NUMBER               6
#define QCM_MMS_ADDRESS_MAX_LENGTH               128
#define QCM_MMS_ADDRESS_TRANS_MAX_LENGTH         (QCM_MMS_ADDRESS_MAX_LENGTH + 13)

#define QCM_MMS_MULTIPARTRELATEDTYPE_MAX_LENGTH  30
#define QCM_MMS_MULTIPARTRELATEDSTART_MAX_LENGTH 30
#define QCM_MMS_CONTENTTYPETEXT_MAX_LENGTH       30
#define QCM_MMS_CONTENTLOCATION_MAX_LENGTH       100

#define QCM_MMS_SENDFILE_MAX_NAMELENGTH          90
#define QCM_MMS_SENDFILE_MAX_FULLPATHLENGTH      (QCM_MMS_SENDFILE_MAX_NAMELENGTH + 10)
#define QCM_MMS_SENDFILE_MAX_NUMBER              13

#define QCM_MMS_ATTACHMENT_MAX_LENGTH            128
#define QCM_MMS_SUBJECT_MAX_LENGTH               256
/** MMS defaults to using SIM card 0.*/
#define QCM_MMS_DEFAULT_SIM_ID                   0

#define QCM_MMS_CHARSET_ASCII_VALUE              0x03
#define QCM_MMS_CHARSET_UTF8_VALUE               0x6A
#define QCM_MMS_CHARSET_UCS2_VALUE               0x03E8
#define QCM_MMS_CHARSET_BIG5_VALUE               0x07EA
#define QCM_MMS_CHARSET_GBK_VALUE                0x71

#define QCM_MMS_FILE_TEXT_TYPE                   1
#define QCM_MMS_FILE_IMAGE_TYPE                  2
#define QCM_MMS_FILE_AUDIO_TYPE                  3
#define QCM_MMS_FILE_VIDEO_TYPE                  4
#define QCM_MMS_FILE_SMIL_TYPE                   5

#endif /* __QCM_MMS_CONFIG_H__ */
