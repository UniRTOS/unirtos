/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Aug 16th, 2020
 * Module:	AOSL regular file async read/write operations
 *          implementation file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#include <api/aosl_file.h>

__export_in_so__ int aosl_mkdir(const char *path)
{
	if (aosl_hal_fexist(path)) {
		return 0;
	}
	return aosl_hal_mkdir(path);
}

__export_in_so__ int aosl_rmdir(const char *path)
{
	return aosl_hal_rmdir(path);
}

__export_in_so__ size_t aosl_fsize(const char *path)
{
	return aosl_hal_fsize(path);
}

__export_in_so__ int aosl_fexist(const char *path)
{
	return aosl_hal_fexist(path);
}

__export_in_so__ int aosl_file_create(const char *filepath)
{
	return aosl_hal_file_create(filepath);
}

__export_in_so__ int aosl_file_delete(const char *filepath)
{
	return aosl_hal_file_delete(filepath);
}

__export_in_so__ int aosl_file_rename(const char *old_name, const char *new_name)
{
	return aosl_hal_file_rename(old_name, new_name);
}

__export_in_so__ aosl_fs_t aosl_fopen(const char *file, const char *mode)
{
	return (aosl_fs_t)aosl_hal_fopen(file, mode);
}

__export_in_so__ int aosl_fclose(aosl_fs_t fs)
{
	return aosl_hal_fclose(fs);
}

__export_in_so__ size_t aosl_fread(aosl_fs_t fs, void *buf, size_t size)
{
	return aosl_hal_fread(fs, buf, size);
}

__export_in_so__ size_t aosl_fwrite(aosl_fs_t fs, const void *buf, size_t size)
{
	return aosl_hal_fwrite(fs, buf, size);
}
