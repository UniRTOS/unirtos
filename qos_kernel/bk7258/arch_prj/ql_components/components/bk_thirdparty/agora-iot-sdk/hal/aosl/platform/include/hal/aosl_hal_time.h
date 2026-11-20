/*************************************************************
 * Author:	zhangguanxian@agora.io
 * Date	 :	2025/12/16
 * Module:	time hal definitions.
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2025 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#ifndef __AOSL_HAL_TIME_H__
#define __AOSL_HAL_TIME_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get current tick in milliseconds since epoch
 * @return current tick in milliseconds since epoch
 */
uint64_t aosl_hal_get_tick_ms (void);

/**
 * @brief get current time in milliseconds since epoch
 * @return current time in milliseconds since epoch
 */
uint64_t aosl_hal_get_time_ms (void);

/**
 * @brief get current time as formatted string
 * @param [out] buf buffer to store formatted time string
 * @param [in] len length of buffer
 * @return 0 on success, < 0 on error
 */
int aosl_hal_get_time_str(char *buf, int len);

/**
 * @brief sleep for specified milliseconds
 * @param [in] ms milliseconds to sleep
 */
void aosl_hal_msleep(uint64_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __AOSL_HAL_TIME_H__ */