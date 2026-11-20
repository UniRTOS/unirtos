#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include "components/system.h"
#include "components/log.h"
#include "driver/flash.h"
#include "modules/ota.h"
#include "utils_httpc.h"
#include "modules/wifi.h"
#include "bk_https.h"
#include "bk_private/bk_ota_private.h"

#if CONFIG_PSA_MBEDTLS
#include "psa/crypto.h"
#endif


#define TAG "HTTPS_OTA"

extern UINT8  ota_flag ;
#ifdef CONFIG_HTTP_AB_PARTITION
extern part_flag update_part_flag;
#endif

#define HTTPS_INPUT_SIZE   (5120)

/* this crt for url https://docs.bekencorp.com , support test*/
const char ca_crt_rsa[] = {
"-----BEGIN CERTIFICATE-----\r\n"
"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\r\n"
"clib3BcxKZX9qK4A6FAATghuSQ==\r\n"
"-----END CERTIFICATE-----\r\n"
};

bk_http_client_handle_t bk_https_client_flash_init(bk_http_input_t config)
{
	bk_http_client_handle_t client = NULL;

	ota_flag = 1;
#if CONFIG_SYSTEM_CTRL
	//bk_wifi_ota_dtim(1);
#endif
#if HTTP_WR_TO_FLASH
	http_flash_init();
#endif
	client = bk_http_client_init(&config);

	return client;

}

bk_err_t bk_https_client_flash_deinit(bk_http_client_handle_t client)
{
	int err;

	ota_flag = 0;
#if CONFIG_SYSTEM_CTRL
	//bk_wifi_ota_dtim(0);
#endif
#if HTTP_WR_TO_FLASH
	http_flash_deinit();
#endif
	if(!client)
		err = bk_http_client_cleanup(client);
	else
		return BK_FAIL;

	return err;

}

bk_err_t https_ota_event_cb(bk_http_client_event_t *evt)
{
    if(!evt)
    {
        return BK_FAIL;
    }

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
	BK_LOGE(TAG, "HTTPS_EVENT_ERROR\r\n");
	break;
    case HTTP_EVENT_ON_CONNECTED:
	BK_LOGE(TAG, "HTTPS_EVENT_ON_CONNECTED\r\n");
#ifdef CONFIG_HTTP_OTA_WITH_BLE
#if CONFIG_BLUETOOTH
        bk_ble_register_sleep_state_callback(ble_sleep_cb);
#endif
#endif
	break;
    case HTTP_EVENT_HEADER_SENT:
	BK_LOGE(TAG, "HTTPS_EVENT_HEADER_SENT\r\n");
	break;
    case HTTP_EVENT_ON_HEADER:
	BK_LOGE(TAG, "HTTPS_EVENT_ON_HEADER\r\n");
	break;
    case HTTP_EVENT_ON_DATA:
	//do something: evt->data, evt->data_len
	bk_ota_process_data((char *)evt->data, evt->data_len, evt->data_len, evt->client->response->content_length);
	BK_LOGD(TAG, "HTTP_EVENT_ON_DATA, length:%d , content_length:0x%x \r\n", evt->data_len , evt->client->response->content_length);
	break;
    case HTTP_EVENT_ON_FINISH:
	//bk_ota_process_data((char *)evt->data, evt->data_len, evt->data_len, evt->client->response->content_length);
	bk_https_client_flash_deinit(evt->client);
	BK_LOGI(TAG, "HTTPS_EVENT_ON_FINISH\r\n");
	break;
    case HTTP_EVENT_DISCONNECTED:
	BK_LOGE(TAG, "HTTPS_EVENT_DISCONNECTED\r\n");
	break;

    }
    return BK_OK;
}


int bk_https_ota_download(const char *url)
{
	int err;

	if(!url)
	{
		err = BK_FAIL;
		BK_LOGI(TAG, "url is NULL\r\n");

		return err;
	}

	err = ota_get_init_status();
	if(err == 1) //has already init
	{
		BK_LOGI(TAG, "has already do ota init \r\n");
	}
	else //do ota init
	{
		if(ota_do_init_operation() == BK_FAIL)
		{
			BK_LOGE(TAG, "do ota init fail\r\n");
			ota_do_deinit_operation();
			return BK_FAIL;
		}
	}

	bk_http_input_t config = {
		.url = url,
		.cert_pem = ca_crt_rsa,
		.event_handler = https_ota_event_cb,
		.buffer_size = HTTPS_INPUT_SIZE,
		.timeout_ms = 15000
	};

	bk_http_client_handle_t client = bk_https_client_flash_init(config);
	if (client == NULL) {
		BK_LOGI(TAG, "client is NULL\r\n");
		err = BK_FAIL;
		ota_do_deinit_operation();
		return err;
	}
	err = bk_http_client_perform(client);
	if(err == BK_OK){
		BK_LOGI(TAG, "bk_http_client_perform ok\r\n");

#ifdef CONFIG_HTTP_AB_PARTITION
	if(ota_get_dest_id() == OTA_WR_TO_FLASH)
	{
		int ret_val = 0;
		#ifdef CONFIG_OTA_HASH_FUNCTION
		ret_val= ota_do_hash_check();
		if(ret_val != BK_OK)
		{
			BK_LOGE(TAG,"hash fail.\r\n");
			ota_do_deinit_operation();
			return  ret_val;
		}
		#endif
		ret_val = bk_ota_update_partition_flag(0);
		if(ret_val != BK_OK)
		{
			ota_do_deinit_operation();
			return ret_val;
		}
	}
	BK_LOGI(TAG,"ota_success.\r\n");
	ota_do_deinit_operation();
	bk_reboot();
#else
	BK_LOGI(TAG,"ota_success.\r\n");
	ota_do_deinit_operation();
    bk_reboot();
#endif
	}
	else{
		bk_https_client_flash_deinit(client);
		ota_do_deinit_operation();
		BK_LOGI(TAG, "bk_http_client_perform fail, err:%x\r\n", err);
	}

	return err;
}
