/** @file         qurl_network.h
 *  @brief        Brief description: Network-related
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2024-02-21 13:45:21
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
*/

#ifndef QURL_NETWOEK_H
#define QURL_NETWOEK_H

#include "qurl_config.h"
#include "qurl_def_basic.h"
#include "qurl_def_in.h"

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
int qurl_nw_activate_async(qurl_core_ctrl_t *core_ptr);

int qurl_nw_activate_async_ex(int nw_id, qurl_nw_activate_cb nw_activate_func, void *cb_arg);

int qurl_nw_is_activated(qurl_core_ctrl_t *core_ptr);

int qurl_nw_get_ip(qurl_core_ctrl_t *core_ptr);
#endif

#endif /* Head define end*/
