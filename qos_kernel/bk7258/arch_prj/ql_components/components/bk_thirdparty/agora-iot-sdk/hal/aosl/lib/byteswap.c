/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Jul 31st, 2020
 * Module:	bswap implementation file for those OS not having
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_byteswap.h>
#include <kernel/swab.h>

__export_in_so__ uint32_t aosl_bswap_32 (uint32_t v)
{
	return aosl_swab32 (v);
}

__export_in_so__ uint64_t aosl_bswap_64 (uint64_t v)
{
	return aosl_swab64 (v);
}