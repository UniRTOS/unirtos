/*************************************************************
 * Author:	zhangguanxian@agora.io
 * Date	 :	2025/12/16
 * Module:	errno hal definitions.
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2025 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#ifndef __AOSL_HAL_ERRNO_H__
#define __AOSL_HAL_ERRNO_H__

#ifdef __cplusplus
extern "C" {
#endif

#define AOSL_HAL_RET_SUCCESS          0
#define AOSL_HAL_RET_FAILURE         -1
#define AOSL_HAL_RET_EAGAIN          -2001
#define AOSL_HAL_RET_EINTR           -2002

/**
 * @brief Convert standard errno to AOSL HAL error codes
 * @param [in] errnum The standard errno value
 * @return Corresponding AOSL HAL error code
 */
int aosl_hal_errno_convert(int errnum);

#ifdef __cplusplus
}
#endif

#endif