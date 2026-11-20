/*************************************************************
 * Author:	zhangguanxian@agora.io
 * Date	 :	2025/12/16
 * Module:	log hal definitions.
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2025 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#ifndef __AOSL_HAL_LOG_H__
#define __AOSL_HAL_LOG_H__

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief print formatted output
 * @param format format string
 * @param args variable arguments list
 * @return number of characters printed, or -1 on error
 */
int aosl_hal_printf(const char *format, va_list args);

#ifdef __cplusplus
}
#endif

#endif