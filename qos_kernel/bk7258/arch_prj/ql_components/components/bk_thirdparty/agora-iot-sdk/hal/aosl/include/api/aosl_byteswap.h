/*************************************************************
 * Author		:		Lionfore Hao (haolianfu@agora.io)
 * Date			:		Jul 31st, 2020
 * Module		:		AOSL byteswap header file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AOSL_BYTESWAP_H__
#define __AOSL_BYTESWAP_H__

#include <api/aosl_types.h>
#include <api/aosl_defs.h>

extern __aosl_api__ uint32_t aosl_bswap_32 (uint32_t v);
extern __aosl_api__ uint64_t aosl_bswap_64 (uint64_t v);

#endif /* __AOSL_BYTESWAP_H__ */