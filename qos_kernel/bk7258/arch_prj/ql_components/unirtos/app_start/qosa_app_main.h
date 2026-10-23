/*================================================================
  Copyright (c) 2021, Quectel Wireless Solutions Co., Ltd. All rights reserved.
  Quectel Wireless Solutions Proprietary and Confidential.
=================================================================*/
#ifndef __QL_APP_MAIN_H__
#define __QL_APP_MAIN_H__

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include "qosa_def.h"
#include "qosa_log.h"

typedef void (*ql_app_t)(void *argv);

// #define _appRegTable_attr_ volatile const __attribute__((__section__(".LOAD_APP_TABLE")))
#define _app_init_used  __attribute__((used))
#define _appRegTable_attr_  __attribute__((__section__(".LOAD_APP_TABLE")))
typedef struct
{
	ql_app_t   app_entry;
	char      *app_name;
	uint32_t   stack_size_byte;
	uint8_t    startup_prio;
	uint32_t   *arvg;
} appRegItem_t;

// #define application_init(app_entry, app_name, stack_size_kib, startup_prio, arvg) _appRegTable_attr_  appRegItem_t _regAppItem_##app_entry  = {app_entry, app_name, stack_size_kib*1024, startup_prio, arvg}

#define application_init(app_entry, app_name, stack_size_kib, startup_prio, arvg) \
	const appRegItem_t  _regAppItem_##app_entry \
    _app_init_used _appRegTable_attr_ = { \
        app_entry,\
		app_name, \
		stack_size_kib*1024, \
		startup_prio, \
		arvg \
    }


static inline void ql_trace(const char *format, ...)
{
	char log_buffer[256];
	va_list args;

	va_start(args, format);
	vsnprintf(log_buffer, sizeof(log_buffer), format, args);
	va_end(args);

	QOSA_LOG_I(QOSA_MAKE_LOG_TAG('U', 'N', 'R', 'T'), "%s", log_buffer);
}

#endif