/*************************************************************
 * Author:	zhangguanxian@agora.io
 * Date	 :	2025/12/16
 * Module:	utils hal definitions.
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2025 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#ifndef __AOSL_HAL_UTILS_H__
#define __AOSL_HAL_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get device UUID
 * @param [out] buf buffer to store UUID string
 * @param [in] buf_sz size of buffer
 * @return 0 on success, < 0 on error
 */
int aosl_hal_get_uuid (char buf [], int buf_sz);

/**
 * @brief get OS version string
 * @param [out] buf buffer to store OS version string
 * @param [in] buf_sz size of buffer
 * @return 0 on success, < 0 on error
 */
int aosl_hal_os_version (char buf [], int buf_sz);

#ifdef __cplusplus
}
#endif

#endif