/*************************************************************
 * Author:	zhangguanxian@agora.io
 * Date	 :	2025/12/16
 * Module:	OS relative utilities implementation file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2025 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#include <hal/aosl_hal_utils.h>
#include <api/aosl_types.h>
#include <api/aosl_defs.h>

__export_in_so__ int aosl_get_uuid (char buf [], size_t buf_sz)
{
	return aosl_hal_get_uuid(buf, (int)buf_sz);
}

__export_in_so__ int aosl_os_version (char buf [], size_t buf_sz)
{
	return aosl_hal_os_version(buf, (int)buf_sz);
}
