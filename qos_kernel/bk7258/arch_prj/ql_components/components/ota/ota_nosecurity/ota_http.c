#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"

#if CONFIG_OTA_HTTP
#include "utils_httpc.h"
#include "modules/wifi.h"
#endif
#ifdef CONFIG_OTA_HASH_FUNCTION
#include "driver/flash_partition.h"
#endif
#include "common/bk_err.h"
#include "bk_private/bk_ota_private.h"

#ifndef OTA_TAG
#define OTA_TAG	"OTA"
#endif

#ifdef CONFIG_OTA_DEBUG_LOG_OPEN
#define OTA_LOGI(...)	BK_LOGI(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGW(...)	BK_LOGW(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGE(...)	BK_LOGE(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGD(...)	BK_LOGD(OTA_TAG, ##__VA_ARGS__)
#else
#define OTA_LOGI(...)	BK_LOGI(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGW(...) 
#define OTA_LOGE(...)	BK_LOGE(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGD(...) 
#endif

#ifdef CONFIG_HTTP_AB_PARTITION
extern part_flag update_part_flag;
#endif
u8  ota_flag =0;

/*Please use the interface of the int bk_ota_start_download(const char *url, ota_wr_destination_t ota_dest_id) function in ota.c.*/
#if CONFIG_OTA_HTTP
int bk_http_ota_download(const char *uri)
{
	int ret;
	httpclient_t httpclient;
	httpclient_data_t httpclient_data;
	char http_content[HTTP_RESP_CONTENT_LEN];

	if(!uri){
		ret = BK_FAIL;
		OTA_LOGE( "uri is NULL\r\n");
		return ret;
	}

	ret = ota_get_init_status();
	if(ret == 1) //has already init
	{
		OTA_LOGI("has already do ota init \r\n");
	}
	else //do ota init
	{
		if(ota_do_init_operation() == BK_FAIL)
		{
			OTA_LOGE("do ota init fail\r\n");
			ota_do_deinit_operation();
			return BK_FAIL;
		}
	}

	OTA_LOGI("http_ota_download :0x%x",bk_http_ota_download);
#if CONFIG_OTA_DISPLAY_PICTURE_DEMO
	ota_do_umount_sysfile();
	if(ota_update_with_display_open() != BK_OK)
	{
		ota_do_deinit_operation();
		return BK_FAIL;
	}

	ret = ota_do_open_sysfile();
	if(ret == BK_FAIL)
		return BK_FAIL;
#endif
	ota_input_event_handler(EVT_OTA_START);

#if CONFIG_SYSTEM_CTRL
	//bk_wifi_ota_dtim(1);
#endif
	ota_flag = 1;
	os_memset(&httpclient, 0, sizeof(httpclient_t));
	os_memset(&httpclient_data, 0, sizeof(httpclient_data));
	os_memset(&http_content, 0, sizeof(HTTP_RESP_CONTENT_LEN));
	httpclient.header = "Accept: text/xml,text/html,\r\n";
	httpclient_data.response_buf = http_content;
	httpclient_data.response_content_len = HTTP_RESP_CONTENT_LEN;
	ret = httpclient_common(&httpclient,
							uri,
							80,/*port*/
							NULL,
							HTTPCLIENT_GET,
							300000,
							&httpclient_data);


	ota_flag = 0;
	if (0 != ret){
		OTA_LOGE("ota_fail: request epoch time from remote server failed.ret:%d\r\n",ret);
		ota_do_deinit_operation();
		ota_input_event_handler(EVT_OTA_FAIL);
	#if CONFIG_OTA_DISPLAY_PICTURE_DEMO
		ret = bk_sconf_trans_start();
		if(ret != BK_OK)
		{
			OTA_LOGE("start transfer fail! ret:%d\r\n",ret);
			return BK_FAIL;
		}
		if(ota_update_with_display_close() != BK_OK)
		{
			OTA_LOGE("disp close failed.ret:%d\r\n",ret);
			return BK_FAIL;
		}
	#endif
#if CONFIG_SYSTEM_CTRL
		//bk_wifi_ota_dtim(0);
#endif
	}else{
#ifdef CONFIG_HTTP_AB_PARTITION
		if(ota_get_dest_id() == OTA_WR_TO_FLASH)
		{
			int ret_val = 0;
			#ifdef CONFIG_OTA_HASH_FUNCTION
			ret_val= ota_do_hash_check();
			if(ret_val != BK_OK)
			{
				OTA_LOGE("hash fail.\r\n");
				ota_do_deinit_operation();
				return  ret_val;
			}
			#endif
			ret_val = bk_ota_update_partition_flag(ret);
			if(ret_val != BK_OK)
			{
				ota_do_deinit_operation();
				return ret_val;
			}
		}
		ota_do_deinit_operation();
		ota_input_event_handler(EVT_OTA_SUCCESS);
#if CONFIG_OTA_DISPLAY_PICTURE_DEMO
		if(ota_update_with_display_close() != BK_OK)
		{
			OTA_LOGE("disp close failed.ret:%d\r\n",ret);
		}
#endif
		OTA_LOGI("ota_success.\r\n");
		bk_reboot();
#else
		OTA_LOGI("ota_success.\r\n");
		ota_do_deinit_operation();
		ota_input_event_handler(EVT_OTA_SUCCESS);
		bk_reboot();
#endif /*CONFIG_HTTP_AB_PARTITION*/
	}
	return ret;
}

#endif
