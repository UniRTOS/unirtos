/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Jul 26th, 2018
 * Module:	AOSL utilities definition file.
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AOSL_UTILS_H__
#define __AOSL_UTILS_H__

#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_mm.h>


#ifdef __cplusplus
extern "C" {
#endif

extern __aosl_api__ int aosl_get_uuid (char buf [], size_t buf_sz);
extern __aosl_api__ int aosl_os_version (char buf [], size_t buf_sz);

#ifdef __cplusplus
}
#endif


#endif /* __AOSL_UTILS_H__ */