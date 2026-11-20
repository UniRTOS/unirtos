/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Jul 26th, 2018
 * Module:	Time relative utilities header file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AOSL_API_TIME_H__
#define __AOSL_API_TIME_H__

#include <api/aosl_types.h>
#include <api/aosl_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The AOSL timestamp type */
typedef uint64_t aosl_ts_t;


extern __aosl_api__ aosl_ts_t aosl_tick_now ();
extern __aosl_api__ aosl_ts_t aosl_tick_ms ();
extern __aosl_api__ aosl_ts_t aosl_tick_us ();

extern __aosl_api__ aosl_ts_t aosl_time_sec ();
extern __aosl_api__ aosl_ts_t aosl_time_ms ();

extern __aosl_api__ void aosl_msleep (uint64_t ms);

extern __aosl_api__ int aosl_time_str(char *buf, int len);

#ifdef __cplusplus
}
#endif


#endif /* __AOSL_API_TIME_H__ */
