/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Jul 15th, 2018
 * Module:	Time relative utilities implementation file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#include <api/aosl_types.h>
#include <api/aosl_time.h>
#include <hal/aosl_hal_time.h>


__export_in_so__ aosl_ts_t aosl_tick_now ()
{
	return (aosl_ts_t)(aosl_hal_get_tick_ms());
}

__export_in_so__ aosl_ts_t aosl_tick_ms ()
{
	return (aosl_ts_t)(aosl_hal_get_tick_ms ());
}

__export_in_so__ aosl_ts_t aosl_tick_us ()
{
	return 0;
}

__export_in_so__ aosl_ts_t aosl_time_sec ()
{
	return (aosl_ts_t)(aosl_hal_get_time_ms () / 1000);
}

__export_in_so__ aosl_ts_t aosl_time_ms ()
{
	return (aosl_ts_t)(aosl_hal_get_time_ms ());
}

__export_in_so__ void aosl_msleep (uint64_t ms)
{
	if (ms == 0) {
		ms = 1;
	}
	aosl_hal_msleep (ms);
}

__export_in_so__ int aosl_time_str(char *buf, int len)
{
	return aosl_hal_get_time_str(buf, len);
}
