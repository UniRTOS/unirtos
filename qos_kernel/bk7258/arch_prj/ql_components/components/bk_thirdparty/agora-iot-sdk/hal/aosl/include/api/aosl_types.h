/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Jul 27th, 2020
 * Module:	AOSL POSIX definitions header file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AOSL_TYPES_H__
#define __AOSL_TYPES_H__

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#if defined (__linux__) || defined (__MACH__) || defined (__LITEOS__) || defined (_WIN32) || defined (__FREERTOS__)
#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


#if !defined (__linux__) && !defined (__MACH__) && !defined(__ALIOSTHINGS__)
/**
 * Worry about some guy would like to define a macro
 * for this type, so confirm that it is not a macro.
 * -- Lionfore Hao Nov 5th, 2018
 **/
#ifndef __ssize_t_defined
typedef intptr_t ssize_t;
#define __ssize_t_defined
#endif
#endif


/* The proto for a general aosl object destructor function. */
typedef void (*aosl_obj_dtor_t) (uintptr_t argc, uintptr_t argv []);

typedef int aosl_fd_t;
#define AOSL_INVALID_FD ((aosl_fd_t)-1)

static __inline__ int aosl_fd_invalid (aosl_fd_t fd)
{
	return (int)(fd < 0);
}



#ifdef __cplusplus
}
#endif


#endif /* __AOSL_TYPES_H__ */