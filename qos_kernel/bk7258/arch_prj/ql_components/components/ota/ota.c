#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"

#ifdef CONFIG_OTA_HASH_FUNCTION
#include "driver/flash_partition.h"
#endif
#include "common/bk_err.h"
#include "bk_private/bk_ota_private.h"

ota_device_into_t *ota_info = NULL;

int bk_ota_process_data_handler (char*buff_data, uint32_t len,uint32_t received, uint32_t total)
{
	int ret = BK_FAIL;

	OTA_CHECK_POINTER(buff_data);
	os_memcpy(ota_info->fota_dl_info.wr_tmp_buf, buff_data, len);
	if(ota_info->fota_dl_info.wr_flash_flag == 0)
	{
		ota_info->fota_dl_info.image_size = total;
		ota_info->fota_dl_info.wr_flash_flag = 1;
	}

	switch (ota_info->ota_dest)
	{
		case OTA_WR_TO_FLASH:
		{
			ret = f_ota_fun_ptr->data_process(&ota_info->fota_dl_info, len, ota_info->fota_dl_info.ota_type, f_ota_fun_ptr->wr_flash);
		}
		break;

		case OTA_WR_TO_SD_CARD:
		{
#if (CONFIG_REMOTE_VFS_CLIENT || CONFIG_VFS)
			ret = f_ota_fun_ptr->data_process(&ota_info->fota_dl_info, len, ota_info->fota_dl_info.ota_type, f_ota_fun_ptr->write);
#endif
		}
		break;

		default:
		break;
	}

	if ((ret == BK_OK) && (ota_info->fota_dl_info.image_size > 0)) {
		ota_progress_event_handler(ota_info->fota_dl_info.received_total_size,
			ota_info->fota_dl_info.image_size);
	}

	return ret;
}

int ota_do_init_operation(void)
{
	int ret = BK_FAIL;

	OTA_MALLOC(ota_info, sizeof(ota_device_into_t));
	ret = f_ota_fun_ptr->init(&ota_info->fota_dl_info); 	//do init
	ota_info->fota_dl_info.ota_type = OTA_TYPE_WIFI;
	if(ret == BK_OK)
	{
		register_ota_callback(bk_ota_process_data_handler);
	}

	return BK_OK;
}

void ota_do_deinit_operation(void)
{
	f_ota_fun_ptr->deinit(&ota_info->fota_dl_info);
	OTA_FREE(ota_info);
}

int ota_get_init_status(void)
{
	int ret = 0;

	if(ota_info == NULL)
		return ret;
	else
		return ota_info->fota_dl_info.init_flag;
}

ota_wr_destination_t ota_get_dest_id(void)
{
	return ota_info->ota_dest;
}

int ota_do_open_sysfile(void)
{
	int ret = BK_OK;
#if (CONFIG_REMOTE_VFS_CLIENT || CONFIG_VFS)
	if(ota_info->ota_dest == OTA_WR_TO_SD_CARD)
	{
		ret = f_ota_fun_ptr->open(&ota_info->fota_dl_info);	//do open
		if(ret == BK_FAIL)
			ota_do_deinit_operation();
	}
#endif
	return ret;
}

void ota_do_umount_sysfile(void)
{
#if (CONFIG_REMOTE_VFS_CLIENT || CONFIG_VFS)
	if(ota_info->ota_dest == OTA_WR_TO_SD_CARD)
	{
		f_ota_fun_ptr->umount(&ota_info->fota_dl_info); //do_umount
	}
#endif
}

int bk_ota_start_download(const char *url,  ota_wr_destination_t ota_dest_id)
{
	int ret = BK_OK;

	OTA_CHECK_POINTER(url);

	ret = ota_do_init_operation(); //do init
	if(ret == BK_FAIL)
	{
		ota_do_deinit_operation();
		return BK_FAIL;
	}

	OTA_MALLOC(ota_info->src_path_name, strlen(url)+2);
	strncpy((char*)ota_info->src_path_name, url, strlen(url)+1);
	ret = ota_extract_path_segment((char*)ota_info->src_path_name, ota_info->fota_dl_info.dest_path_name);
	if(ret == BK_FAIL)
	{
		OTA_FREE(ota_info->src_path_name);
		ota_do_deinit_operation();
		return BK_FAIL;
	}
	OTA_FREE(ota_info->src_path_name);
	OTA_LOGI("ota_info->url :%s, dest_path_name :%s \r\n", url, ota_info->fota_dl_info.dest_path_name);
	switch(ota_dest_id)
	{
		case OTA_WR_TO_FLASH:
		{
			ota_info->ota_dest = ota_dest_id;
		}
		break;

		case OTA_WR_TO_SD_CARD:
		{
			ota_info->ota_dest = ota_dest_id;
			ret = ota_do_open_sysfile(); //do open
			if(ret == BK_FAIL)
				return BK_FAIL;
		}
		break;

		default:
			ota_info->ota_dest = OTA_WR_TO_FLASH;
		break;
	}

#if CONFIG_OTA_HTTPS
	ret = bk_https_ota_download(url);
#else
	ret = bk_http_ota_download(url);
#endif

	if(ret != BK_OK)
	{
		OTA_LOGE("ota_fail.\r\n");
	}

	return ret;
}
