/*****************************************************************/ /**
 * @file   qosa_log.h
 * @brief
 * @author larson.li@quectel.com
 * @date   2023-03-23
 *
 * @copyright  Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date       <th>Version    <th>Author          <th>Description"
 * <tr><td>2023-03-23 <td>1.0        <td>Larson.Li       <td> Init
 * <tr><td>2025-10-25 <td>1.1        <td>Larson.Li       <td> Adapt the log interface for each platform side
 * </table>
 **********************************************************************/
#include "qosa_def.h"

#ifdef CONFIG_QOSA_EIGEN718_PLATFORM_FUNC
#include "qosa_eigen_log.h"
#endif /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */

#ifdef CONFIG_QOSA_QCX216_PLATFORM_FUNC
#include "qosa_eigen_log.h"
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC */

#if defined(CONFIG_QOSA_UIS8852_PLATFORM_FUNC)
#include "qosa_rda_log.h"
#endif  // CONFIG_QOSA_UIS8852_PLATFORM_FUNC

#if defined(CONFIG_QOSA_QT2131_PLATFORM_FUNC)
#include "qosa_qt2131_log.h"
#endif //CONFIG_QOSA_QT2131_PLATFORM_FUNC
#if defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC)
#include "qosa_bk7258_log.h"
#endif //CONFIG_QOSA_BK7258_PLATFORM_FUNC
#if defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC)
#include "qosa_bk7238_log.h"
#endif //CONFIG_QOSA_BK7238_PLATFORM_FUNC
#if defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)
#include "qosa_ecr6600_log.h"
#endif //CONFIG_QOSA_ECR6600_PLATFORM_FUNC
#if defined(CONFIG_QOSA_LINUX_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
#include "qosa_public_log.h"
#endif  // CONFIG_QOSA_LINUX_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC
