/*************************************************************
 * Author		:		Lionfore Hao (haolianfu@agora.io)
 * Date			:		Jul 21st, 2018
 * Module		:		Data Marshalling base object
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#include <api/aosl_marshalling.h>

#ifndef CONFIG_WINDOWS
#define __export_data_in_so__ __export_in_so__
#else
#define __export_data_in_so__
#endif

__export_data_in_so__ const aosl_type_info_t aosl_void_type = { .type_id = AOSL_TYPE_VOID, .obj_addr = NULL, };
__export_data_in_so__ const aosl_type_info_t aosl_int16_type = { .type_id = AOSL_TYPE_INT16, .obj_addr = NULL, };
__export_data_in_so__ const aosl_type_info_t aosl_int32_type = { .type_id = AOSL_TYPE_INT32, .obj_addr = NULL, };
__export_data_in_so__ const aosl_type_info_t aosl_int64_type = { .type_id = AOSL_TYPE_INT64, .obj_addr = NULL, };
__export_data_in_so__ const aosl_type_info_t aosl_float_type = { .type_id = AOSL_TYPE_FLOAT, .obj_addr = NULL, };
__export_data_in_so__ const aosl_type_info_t aosl_double_type = { .type_id = AOSL_TYPE_DOUBLE, .obj_addr = NULL, };
__export_data_in_so__ const aosl_type_info_t aosl_v4_ipaddr_type = { .type_id = AOSL_TYPE_V4_IPADDR, .obj_addr = NULL, };
__export_data_in_so__ const aosl_type_info_t aosl_v6_ipaddr_type = { .type_id = AOSL_TYPE_V6_IPADDR, .obj_addr = NULL, };

__export_data_in_so__ const aosl_type_info_t aosl_dynamic_int16_array_type = {
	.type_id = AOSL_TYPE_DYNAMIC_ARRAY,
	.obj_addr = NULL,
	.child = &aosl_int16_type,
};

__export_data_in_so__ const aosl_type_info_t aosl_dynamic_int32_array_type = {
	.type_id = AOSL_TYPE_DYNAMIC_ARRAY,
	.obj_addr = NULL,
	.child = &aosl_int32_type,
};

__export_data_in_so__ const aosl_type_info_t aosl_dynamic_int64_array_type = {
	.type_id = AOSL_TYPE_DYNAMIC_ARRAY,
	.obj_addr = NULL,
	.child = &aosl_int64_type,
};

__export_data_in_so__ const aosl_type_info_t aosl_dynamic_float_array_type = {
	.type_id = AOSL_TYPE_DYNAMIC_ARRAY,
	.obj_addr = NULL,
	.child = &aosl_float_type,
};

__export_data_in_so__ const aosl_type_info_t aosl_dynamic_double_array_type = {
	.type_id = AOSL_TYPE_DYNAMIC_ARRAY,
	.obj_addr = NULL,
	.child = &aosl_double_type,
};

__export_data_in_so__ const aosl_type_info_t aosl_dynamic_v4_ipaddr_array_type = {
	.type_id = AOSL_TYPE_DYNAMIC_ARRAY,
	.obj_addr = NULL,
	.child = &aosl_v4_ipaddr_type,
};

__export_data_in_so__ const aosl_type_info_t aosl_dynamic_v6_ipaddr_array_type = {
	.type_id = AOSL_TYPE_DYNAMIC_ARRAY,
	.obj_addr = NULL,
	.child = &aosl_v6_ipaddr_type,
};

__export_data_in_so__ const aosl_type_info_t aosl_dynamic_bytes_type = { .type_id = AOSL_TYPE_DYNAMIC_BYTES, .obj_addr = NULL, };
__export_data_in_so__ const aosl_type_info_t aosl_dynamic_string_type = { .type_id = AOSL_TYPE_DYNAMIC_STRING, .obj_addr = NULL, };