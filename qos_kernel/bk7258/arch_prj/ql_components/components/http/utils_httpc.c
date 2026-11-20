/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */
#include <string.h>
#include <stddef.h>
#include <common/bk_include.h>
#include "utils_timer.h"
#include "lite-log.h"
#include "utils_httpc.h"
#include "bk_uart.h"
#include <os/mem.h>
#include <os/str.h>
#include <driver/wdt.h>
#include "security_ota.h"

#if HTTP_WR_TO_FLASH
#if CONFIG_FLASH_ORIGIN_API
#include "bk_flash.h"
#else
#include "driver/flash.h"
#endif
#endif
#include "bk_private/bk_ota_private.h"

#if CONFIG_HTTP
#define HTTPCLIENT_MIN(x,y) (((x)<(y))?(x):(y))
#define HTTPCLIENT_MAX(x,y) (((x)>(y))?(x):(y))

#define HTTPCLIENT_AUTHB_SIZE     128

#define HTTPCLIENT_CHUNK_SIZE     256
#define HTTPCLIENT_SEND_BUF_SIZE  2048

#define HTTPCLIENT_MAX_HOST_LEN   64
#define HTTPCLIENT_MAX_URL_LEN    256
#define HTTPCLIENT_MAX_PORT_LEN 6

#define HTTP_RETRIEVE_MORE_DATA   (1)            /**< More data needs to be retrieved. */

#if CONFIG_FLASH_ORIGIN_API
extern void flash_protection_op(UINT8 mode, PROTECT_TYPE type);
#endif
#if CONFIG_OTA_TFTP
#define HTTP_FLASH_WR_BUF_MAX WR_BUF_MAX
#else
#define HTTP_FLASH_WR_BUF_MAX   1024
#endif


HTTP_DATA_ST bk_http = {
	.http_total = 0,
	.do_data = 0,
#if HTTP_WR_TO_FLASH || CONFIG_DIRECT_XIP || CONFIG_OTA_OVERWRITE
	.wr_buf = NULL,
	.wr_last_len = 0,
	.wr_tmp_buf = NULL,
#endif
};
HTTP_DATA_ST *bk_http_ptr = &bk_http;
#if HTTP_WR_TO_FLASH
static UINT32 ota_wr_block = 0;
#endif

// static int httpclient_parse_host(const char *url, char *host, uint32_t maxhost_len);
static int httpclient_parse_url(const char *url, char *scheme, uint32_t max_scheme_len, char *host,
								uint32_t maxhost_len, int *port, char *path, uint32_t max_path_len);
static int httpclient_conn(httpclient_t *client);
static int httpclient_recv(httpclient_t *client, char *buf, int min_len, int max_len, int *p_read_len,
						   uint32_t timeout);
static int httpclient_retrieve_content(httpclient_t *client, char *data, int len, uint32_t timeout,
									   httpclient_data_t *client_data);
static int httpclient_response_parse(httpclient_t *client, char *data, int len, uint32_t timeout,
									 httpclient_data_t *client_data);

static void httpclient_base64enc(char *out, const char *in)
{
	const char code[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	int i = 0, x = 0, l = 0;

	for (; *in; in++) {
		x = x << 8 | *in;
		for (l += 8; l >= 6; l -= 6)
			out[i++] = code[(x >> (l - 6)) & 0x3f];
	}
	if (l > 0) {
		x <<= 6 - l;
		out[i++] = code[x & 0x3f];
	}
	for (; i % 4;)
		out[i++] = '=';
	out[i] = '\0';
}

uint32_t get_http_flash_wr_buf_max(void)
{
	return HTTP_FLASH_WR_BUF_MAX;
}

int httpclient_conn(httpclient_t *client)
{
	if (0 != client->net.connect(&client->net)) {
		log_err("establish connection failed");
		return ERROR_HTTP_CONN;
	}

	return SUCCESS_RETURN;
}



int httpclient_parse_url(const char *url, char *scheme, uint32_t max_scheme_len, char *host, uint32_t maxhost_len,
						 int *port, char *path, uint32_t max_path_len)
{
	char *scheme_ptr = (char *) url;
	char *host_ptr = (char *) os_strstr(url, "://");
	char port_array[HTTPCLIENT_MAX_PORT_LEN] = {0};
	uint32_t host_len = 0;
	uint32_t port_len = 0;
	uint32_t path_len;
	char *port_ptr;
	char *path_ptr;
	char *fragment_ptr;


	if (host_ptr == NULL) {
		log_err("Could not find host");
		return ERROR_HTTP_PARSE; /* URL is invalid */
	}

	if (max_scheme_len < host_ptr - scheme_ptr + 1) {
		/* including NULL-terminating char */
		log_err("Scheme str is too small (%u >= %u)", max_scheme_len, (uint32_t)(host_ptr - scheme_ptr + 1));
		return ERROR_HTTP_PARSE;
	}
	os_memcpy(scheme, scheme_ptr, host_ptr - scheme_ptr);
	scheme[host_ptr - scheme_ptr] = '\0';

	host_ptr += 3;

	path_ptr = os_strchr(host_ptr, '/');
	if (NULL == path_ptr) {
		log_err("invalid path");
		return -1;
	}

	port_ptr = os_strchr(host_ptr, ':');
	if (NULL == port_ptr) {
		*port = 0;
		port_len = 0;
	} else {
		port_len = path_ptr - port_ptr;
		if(port_len < HTTPCLIENT_MAX_PORT_LEN) {
			//port_ptr -> :8192
			os_memcpy(port_array, port_ptr + 1, port_len - 1);
			port_array[port_len - 1] = '\0';
			*port= os_strtoul(port_array, NULL, 10);
		} else {
			log_err("parse port error, port_len(%d).", port_len);
			*port = 0;
			port_len = 0;
		}

	}

	log_info("port (%d) port_len(%d).", *port, port_len);


	host_len = path_ptr - host_ptr - port_len;
	if (host_len == 0) {
		log_err("Parse Host error.");
		return ERROR_HTTP_PARSE;
	}

	if (maxhost_len < host_len + 1) {
		/* including NULL-terminating char */
		log_err("Host str is too long (host_len(%d) >= max_len(%d))", host_len + 1, maxhost_len);
		return ERROR_HTTP_PARSE;
	}
	os_memcpy(host, host_ptr, host_len);
	host[host_len] = '\0';

	fragment_ptr = os_strchr(host_ptr, '#');
	if (fragment_ptr != NULL)
		path_len = fragment_ptr - path_ptr;
	else
		path_len = os_strlen(path_ptr);

	if (max_path_len < path_len + 1) {
		/* including NULL-terminating char */
		log_err("Path str is too small (%d >= %d)", max_path_len, path_len + 1);
		return ERROR_HTTP_PARSE;
	}
	os_memcpy(path, path_ptr, path_len);
	path[path_len] = '\0';

	return SUCCESS_RETURN;
}

// int httpclient_parse_host(const char *url, char *host, uint32_t maxhost_len)
// {
// 	const char *host_ptr = (const char *) os_strstr(url, "://");
// 	uint32_t host_len = 0;
// 	char *path_ptr;

// 	if (host_ptr == NULL) {
// 		log_err("Could not find host");
// 		return ERROR_HTTP_PARSE; /* URL is invalid */
// 	}
// 	host_ptr += 3;

// 	path_ptr = os_strchr(host_ptr, '/');
// 	if (host_len == 0)
// 		host_len = path_ptr - host_ptr;

// 	if (maxhost_len < host_len + 1) {
// 		/* including NULL-terminating char */
// 		log_err("Host str is too small (%d >= %d)", maxhost_len, host_len + 1);
// 		return ERROR_HTTP_PARSE;
// 	}
// 	os_memcpy(host, host_ptr, host_len);
// 	host[host_len] = '\0';

// 	return SUCCESS_RETURN;
// }

int httpclient_get_info(httpclient_t *client, char *send_buf, int *send_idx, char *buf,
						uint32_t len) /* 0 on success, err code on failure */
{
	int ret;
	int cp_len;
	int idx = *send_idx;

	if (len == 0)
		len = os_strlen(buf);

	do {
		if ((HTTPCLIENT_SEND_BUF_SIZE - idx) >= len)
			cp_len = len;
		else
			cp_len = HTTPCLIENT_SEND_BUF_SIZE - idx;

		os_memcpy(send_buf + idx, buf, cp_len);
		idx += cp_len;
		len -= cp_len;

		if (idx == HTTPCLIENT_SEND_BUF_SIZE) {
			//            if (client->remote_port == HTTPS_PORT)
			//            {
			//                WRITE_IOT_ERROR_LOG("send buffer overflow");
			//                return ERROR_HTTP;
			//            }
			//ret = httpclient_tcp_send_all(client->handle, send_buf, HTTPCLIENT_SEND_BUF_SIZE);
			ret = client->net.write(&client->net, send_buf, HTTPCLIENT_SEND_BUF_SIZE, 5000);
			if (ret)
				return (ret);
		}
	} while (len);

	*send_idx = idx;
	return SUCCESS_RETURN;
}

void httpclient_set_custom_header(httpclient_t *client, char *header)
{
	client->header = header;
}

int httpclient_basic_auth(httpclient_t *client, char *user, char *password)
{
	if ((os_strlen(user) + os_strlen(password)) >= HTTPCLIENT_AUTHB_SIZE)
		return ERROR_HTTP;
	client->auth_user = user;
	client->auth_password = password;
	return SUCCESS_RETURN;
}

int httpclient_send_auth(httpclient_t *client, char *send_buf, int *send_idx)
{
	char b_auth[(int)((HTTPCLIENT_AUTHB_SIZE + 3) * 4 / 3 + 1)];
	char base64buff[HTTPCLIENT_AUTHB_SIZE + 3];

	httpclient_get_info(client, send_buf, send_idx, "Authorization: Basic ", 0);
	snprintf(base64buff, sizeof(base64buff), "%s:%s", client->auth_user, client->auth_password);
	log_debug("bAuth: %s", base64buff) ;
	httpclient_base64enc(b_auth, base64buff);
	b_auth[os_strlen(b_auth) + 1] = '\0';
	b_auth[os_strlen(b_auth)] = '\n';
	log_debug("b_auth:%s", b_auth) ;
	httpclient_get_info(client, send_buf, send_idx, b_auth, 0);
	return SUCCESS_RETURN;
}

int httpclient_send_header(httpclient_t *client, const char *url, int method, httpclient_data_t *client_data)
{
	char scheme[8] = { 0 };
	char *host;
	char *path;
	int len;
	char *send_buf;
	char *buf;
	char *meth = (method == HTTPCLIENT_GET) ? "GET" : (method == HTTPCLIENT_POST) ? "POST" :
				 (method == HTTPCLIENT_PUT) ? "PUT" : (method == HTTPCLIENT_DELETE) ? "DELETE" :
				 (method == HTTPCLIENT_HEAD) ? "HEAD" : "";
	int ret;
	int port;
	int rc = SUCCESS_RETURN;

	if (NULL == (host = (char *)os_malloc(HTTPCLIENT_MAX_HOST_LEN))) {
		log_err("not enough memory");
		return FAIL_RETURN;
	}
	if (NULL == (path = (char *)os_malloc(HTTPCLIENT_MAX_URL_LEN))) {
		log_err("not enough memory");
		rc = FAIL_RETURN;
		goto GO_ERR_3;
	}
	if (NULL == (send_buf = (char *)os_malloc(HTTPCLIENT_SEND_BUF_SIZE))) {
		log_err("not enough memory");
		rc = FAIL_RETURN;
		goto GO_ERR_2;
	}
	if (NULL == (buf = (char *)os_malloc(HTTPCLIENT_SEND_BUF_SIZE))) {
		log_err("not enough memory");
		rc = FAIL_RETURN;
		goto GO_ERR_1;
	}
	/* First we need to parse the url (http[s]://host[:port][/[path]]) */
	int res = httpclient_parse_url(url, scheme, sizeof(scheme), host, HTTPCLIENT_MAX_HOST_LEN, &port, path, HTTPCLIENT_MAX_URL_LEN);
	if (res != SUCCESS_RETURN) {
		log_err("httpclient_parse_url returned %d", res);
		//return res;
		rc = res;
		goto GO_ERR;
	}

	//if (client->remote_port == 0)
	//{
	if (os_strcmp(scheme, "http") == 0) {
		//client->remote_port = HTTP_PORT;
	} else if (os_strcmp(scheme, "https") == 0) {
		//client->remote_port = HTTPS_PORT;
	}
	//}

	/* Send request */
	os_memset(send_buf, 0, HTTPCLIENT_SEND_BUF_SIZE);
	os_memset(buf, 0, HTTPCLIENT_SEND_BUF_SIZE);
	len = 0; /* Reset send buffer */

	snprintf(buf, HTTPCLIENT_SEND_BUF_SIZE, "%s %s HTTP/1.1\r\nHost: %s\r\n", meth, path, host); /* Write request */
	ret = httpclient_get_info(client, send_buf, &len, buf, os_strlen(buf));
	if (ret) {
		log_err("Could not write request");
		//return ERROR_HTTP_CONN;
		rc = ERROR_HTTP_CONN;
		goto GO_ERR;
	}

	/* Send all headers */
	if (client->auth_user) {
		httpclient_send_auth(client, send_buf, &len); /* send out Basic Auth header */
	}

	/* Add user header information */
	if (client->header)
		httpclient_get_info(client, send_buf, &len, (char *) client->header, os_strlen(client->header));

	if (client_data->post_buf != NULL) {
		snprintf(buf, HTTPCLIENT_SEND_BUF_SIZE, "Content-Length: %d\r\n", client_data->post_buf_len);
		httpclient_get_info(client, send_buf, &len, buf, os_strlen(buf));

		if (client_data->post_content_type != NULL) {
			snprintf(buf, HTTPCLIENT_SEND_BUF_SIZE, "Content-Type: %s\r\n", client_data->post_content_type);
			httpclient_get_info(client, send_buf, &len, buf, os_strlen(buf));
		}
	}

	/* Close headers */
	httpclient_get_info(client, send_buf, &len, "\r\n", 0);

	log_multi_line(LOG_DEBUG_LEVEL, "REQUEST", "%s", send_buf, ">");

	//ret = httpclient_tcp_send_all(client->net.handle, send_buf, len);
	ret = client->net.write(&client->net, send_buf, len, 5000);
	if (ret > 0)
		log_debug("Written %d bytes", ret);
	else if (ret == 0) {
		log_err("ret == 0,Connection was closed by server");
		//return ERROR_HTTP_CLOSED; /* Connection was closed by server */
		rc = ERROR_HTTP_CLOSED;
	} else {
		log_err("Connection error (send returned %d)", ret);
		//return ERROR_HTTP_CONN;
		rc = ERROR_HTTP_CONN;
	}
GO_ERR:
	os_free(buf);
GO_ERR_1:
	os_free(send_buf);
GO_ERR_2:
	os_free(path);
GO_ERR_3:
	os_free(host);
	return rc;//SUCCESS_RETURN;
}

int httpclient_send_userdata(httpclient_t *client, httpclient_data_t *client_data)
{
	int ret = 0;

	if (client_data->post_buf && client_data->post_buf_len) {
		log_debug("client_data->post_buf: %s", client_data->post_buf);
		{
			//ret = httpclient_tcp_send_all(client->handle, (char *)client_data->post_buf, client_data->post_buf_len);
			ret = client->net.write(&client->net, (char *)client_data->post_buf, client_data->post_buf_len, 5000);
			if (ret > 0)
				log_debug("Written %d bytes", ret);
			else if (ret == 0) {
				log_err("ret == 0,Connection was closed by server");
				return ERROR_HTTP_CLOSED; /* Connection was closed by server */
			} else {
				log_err("Connection error (send returned %d)", ret);
				return ERROR_HTTP_CONN;
			}
		}
	}

	return SUCCESS_RETURN;
}

/* 0 on success, err code on failure */
int httpclient_recv(httpclient_t *client, char *buf, int min_len, int max_len, int *p_read_len, uint32_t timeout_ms)
{
	int ret = 0;
	iotx_time_t timer;

	iotx_time_init(&timer);
	utils_time_countdown_ms(&timer, timeout_ms);

	*p_read_len = 0;

	ret = client->net.read(&client->net, buf, max_len, iotx_time_left(&timer));
	//log_debug("Recv: | %s", buf);

	if (ret > 0)
		*p_read_len = ret;
	else if (ret == 0) {
		//timeout
		return ERROR_HTTP_CONN;
	} else if (-1 == ret) {
		log_info("Connection closed.");
		return ERROR_HTTP_CONN;
	} else if (-3 == ret) {
		log_info("checksum fail.");
		return FAIL_RETURN;
	} else {
		log_err("Connection error (recv returned %d)", ret);
		return ERROR_HTTP_CONN;
	}
	log_info("%u bytes has been read", *p_read_len);
	return 0;


}



#if HTTP_WR_TO_FLASH

void http_flash_init(void)
{
#if CONFIG_FLASH_ORIGIN_API
	UINT32 status;
#endif
	bk_http_ptr->wr_buf = NULL;
	bk_http_ptr->wr_tmp_buf = NULL;

	if (!bk_http_ptr->wr_buf) {
		bk_http_ptr->wr_buf = os_malloc(HTTP_FLASH_WR_BUF_MAX * sizeof(char));
		if (! bk_http_ptr->wr_buf)
			BK_LOGD(NULL, "wr_buf malloc err\r\n");
	}

	if (!bk_http_ptr->wr_tmp_buf) {
		bk_http_ptr->wr_tmp_buf = os_malloc(HTTP_FLASH_WR_BUF_MAX * sizeof(char));
		if (! bk_http_ptr->wr_tmp_buf)
			BK_LOGD(NULL, "wr_tmp_buf malloc err\r\n");
	}

	bk_http_ptr->wr_last_len = 0;
	ota_wr_block = 0;

}

void http_flash_deinit(void)
{
	os_free(bk_http_ptr->wr_buf);
	os_free(bk_http_ptr->wr_tmp_buf);
	os_memset(bk_http_ptr, 0, sizeof(HTTP_DATA_ST));

	ota_wr_block = 0;
#if CONFIG_FLASH_ORIGIN_API
	ddev_close(bk_http_ptr->flash_hdl);

	bk_flash_enable_security(FLASH_UNPROTECT_LAST_BLOCK);
#else
	//bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
#endif
	BK_LOGD(NULL, "write over\r\n");
}
#endif

#if CONFIG_UVC_OTA_DEMO
static http_data_process_callback_func http_data_process_cb = NULL;
void http_data_process_register_callback(http_data_process_callback_func cb)
{
    http_data_process_cb = cb;
}
#endif

int http_data_process(char *buf, UINT32 len, UINT32 recived, UINT32 total)
{
	int ret = BK_OK;
#if CONFIG_UVC_OTA_DEMO
	if(http_data_process_cb)
		http_data_process_cb(buf, len, recived, total);
	else
#endif

#if CONFIG_OTA_FUNCTION
	ret = bk_ota_process_data(buf, len, recived, total);
#endif

	return ret;
}

int httpclient_retrieve_content(httpclient_t *client, char *data, int len, uint32_t timeout_ms,
								httpclient_data_t *client_data)
{
	int count = 0;
	int templen = 0;
	int crlf_pos;
	iotx_time_t timer;
	char *b_data = NULL;
    int ret = 0;
	iotx_time_init(&timer);
	utils_time_countdown_ms(&timer, timeout_ms);

	/* Receive data */
	log_debug("Current data: %s", data);

	client_data->is_more = true;

	if (client_data->response_content_len == -1 && client_data->is_chunked == false) {
		while (true) {
			int max_len;
			//if (count + len < client_data->response_buf_len - 1) { /* modify by acl, only used for ota */
			//	os_memcpy(client_data->response_buf + count, data, len);
				count += len;
			//	client_data->response_buf[count] = '\0';
			//} else {
			//	os_memcpy(client_data->response_buf + count, data, client_data->response_buf_len - 1 - count);
			//	client_data->response_buf[client_data->response_buf_len - 1] = '\0';
			//	return HTTP_RETRIEVE_MORE_DATA;
			//}

			max_len = HTTPCLIENT_MIN(HTTPCLIENT_CHUNK_SIZE - 1, client_data->response_buf_len - 1 - count);
			ret = httpclient_recv(client, data, 1, max_len, &len, iotx_time_left(&timer));

			/* Receive data */
			log_debug("data len: %d %d", len, count);

			if (ret == ERROR_HTTP_CONN) {
				log_debug("ret == ERROR_HTTP_CONN");
				return ret;
			}

			if (len == 0) {
				/* read no more data */
				log_debug("no more len == 0");
				client_data->is_more = false;
				return SUCCESS_RETURN;
			}
		}
	}

	while (true) {
		uint32_t readLen = 0;

		if (client_data->is_chunked && client_data->retrieve_len <= 0) {
			/* Read chunk header */
			bool foundCrlf;
			int n;
			do {
				foundCrlf = false;
				crlf_pos = 0;
				data[len] = 0;
				if (len >= 2) {
					for (; crlf_pos < len - 2; crlf_pos++) {
						if (data[crlf_pos] == '\r' && data[crlf_pos + 1] == '\n') {
							foundCrlf = true;
							break;
						}
					}
				}
				if (!foundCrlf) {
					/* Try to read more */
					if (len < HTTPCLIENT_CHUNK_SIZE) {
						int new_trf_len;
						ret = httpclient_recv(client,
											  data + len,
											  0,
											  HTTPCLIENT_CHUNK_SIZE - len - 1,
											  &new_trf_len,
											  iotx_time_left(&timer));
						len += new_trf_len;
						if (ret == ERROR_HTTP_CONN)
							return ret;
						else
							continue;
					} else
						return ERROR_HTTP;
				}
			} while (!foundCrlf);
			data[crlf_pos] = '\0';
			n = sscanf(data, "%x", &readLen);/* chunk length */
			client_data->retrieve_len = readLen;
			client_data->response_content_len += client_data->retrieve_len;
			if (n != 1) {
				log_err("Could not read chunk length");
				return ERROR_HTTP_UNRESOLVED_DNS;
			}

			os_memmove(data, &data[crlf_pos + 2], len - (crlf_pos + 2)); /* Not need to move NULL-terminating char any more */
			len -= (crlf_pos + 2);

			if (readLen == 0) {
				/* Last chunk */
				client_data->is_more = false;
				log_debug("no more (last chunk)");
				break;
			}
		} else
			readLen = client_data->retrieve_len;

		os_printf("Total-Payload: %d Bytes; Read: %d Bytes", readLen, len);
#if HTTP_WR_TO_FLASH
		http_flash_init();
#endif
#if CONFIG_OTA_FUNCTION
	bk_ota_process_data(data, len, len,readLen);
#endif
#if (CONFIG_SECURITY_OTA)
		security_ota_init();
		if(security_ota_parse_data(data, len) != 0){
			return FAIL_RETURN;
		}

#endif

		b_data =  os_malloc((TCP_LEN_MAX + 1) * sizeof(char));
		if (b_data == NULL) {
			os_printf("b_data alloc fail\n");
			return ERROR_NO_ENOUGH_MEM;
		}
		bk_http_ptr->do_data = 1;
		bk_http_ptr->http_total = readLen - len;
		do {
			templen = HTTPCLIENT_MIN(len, readLen);
			//if (count + templen < client_data->response_buf_len - 1) { /* modify by acl, only used for ota */
				count += templen;
			//	client_data->response_buf[count] = '\0';
				client_data->retrieve_len -= templen;
			//} else {
			//	client_data->response_buf[client_data->response_buf_len - 1] = '\0';
			//	client_data->retrieve_len -= (client_data->response_buf_len - 1 - count);
			//	ret = HTTP_RETRIEVE_MORE_DATA;
            //    break;
			//}

			if (len > readLen) {
				log_debug("memmove %d %d %d\n", readLen, len, client_data->retrieve_len);
				os_memmove(b_data, &b_data[readLen], len - readLen); /* chunk case, read between two chunks */
				len -= readLen;
				readLen = 0;
				client_data->retrieve_len = 0;
			} else
				readLen -= len;

			if (readLen) {
				int max_len = HTTPCLIENT_MIN(TCP_LEN_MAX, client_data->response_buf_len - 1 - count);
				max_len = HTTPCLIENT_MIN(max_len, readLen);

				ret = httpclient_recv(client, b_data, 1, max_len, &len, iotx_time_left(&timer));
				if (ret == ERROR_HTTP_CONN) {
					BK_LOGD(NULL, "%s (line:%d) ERROR_HTTP_CONN len:%d readLen:%d\r\n", __func__, __LINE__, len, readLen);
                    break;
				}
			}

			if (readLen == len) {
				readLen = 0;
			}
		} while (readLen);

		bk_http_ptr->do_data = 0;
		os_free(b_data);
		b_data = NULL;
		if(ret != 0) {
			return ret;
		}

		if (client_data->is_chunked) {
			if (len < 2) {
				int new_trf_len;
				/* Read missing chars to find end of chunk */
				ret = httpclient_recv(client, data + len, 2 - len, HTTPCLIENT_CHUNK_SIZE - len - 1, &new_trf_len,
									  iotx_time_left(&timer));
				if (ret == ERROR_HTTP_CONN)
					return ret;
				len += new_trf_len;
			}
			if ((data[0] != '\r') || (data[1] != '\n')) {
				log_err("Format error, %s", data); /* after memmove, the beginning of next chunk */
				return ERROR_HTTP_UNRESOLVED_DNS;
			}
			os_memmove(data, &data[2], len - 2); /* remove the \r\n */
			len -= 2;
		} else {
			log_debug("no more (content-length)");
#if CONFIG_OTA_FUNCTION
			bk_ota_process_data((char*)bk_http_ptr->wr_buf, bk_http_ptr->wr_last_len, bk_http_ptr->wr_last_len,bk_http_ptr->http_total);
#endif
#if HTTP_WR_TO_FLASH
			http_flash_deinit();
#endif
#if (CONFIG_SECURITY_OTA)
			if (security_ota_deinit() != 0) {
				return FAIL_RETURN;
			}
#endif
			client_data->is_more = false;
			break;
		}

	}

	return SUCCESS_RETURN;
}

int httpclient_response_parse(httpclient_t *client, char *data, int len, uint32_t timeout_ms,
							  httpclient_data_t *client_data)
{
	int crlf_pos;
	iotx_time_t timer;

	iotx_time_init(&timer);
	utils_time_countdown_ms(&timer, timeout_ms);

	client_data->response_content_len = -1;

	char *crlf_ptr = os_strstr(data, "\r\n");
	if (crlf_ptr == NULL) {
		log_err("\r\n not found");
		return ERROR_HTTP_UNRESOLVED_DNS;
	}

	crlf_pos = crlf_ptr - data;
	data[crlf_pos] = '\0';

	/* Parse HTTP response */
	if (sscanf(data, "HTTP/%*d.%*d %d %*[^\r\n]", &(client->response_code)) != 1) {
		/* Cannot match string, error */
		log_err("Not a correct HTTP answer : %s\n", data);
		return ERROR_HTTP_UNRESOLVED_DNS;
	}

	if ((client->response_code < 200) || (client->response_code >= 400)) {
		/* Did not return a 2xx code; TODO fetch headers/(&data?) anyway and implement a mean of writing/reading headers */
		log_warning("Response code %d", client->response_code);
	}

	log_debug("Reading headers%s", data);

	os_memmove(data, &data[crlf_pos + 2], len - (crlf_pos + 2) + 1); /* Be sure to move NULL-terminating char as well */
	len -= (crlf_pos + 2);

	client_data->is_chunked = false;

	/* Now get headers */
	while (true) {
		char key[32];
		char value[32];
		int n;

		key[31] = '\0';
		value[31] = '\0';

		crlf_ptr = os_strstr(data, "\r\n");
		if (crlf_ptr == NULL) {
			if (len < HTTPCLIENT_CHUNK_SIZE - 1) {
				int new_trf_len, ret;
				ret = httpclient_recv(client, data + len, 1, HTTPCLIENT_CHUNK_SIZE - len - 1, &new_trf_len, iotx_time_left(&timer));
				len += new_trf_len;
				data[len] = '\0';
				log_debug("Read %d chars; In buf: [%s]", new_trf_len, data);
				if (ret == ERROR_HTTP_CONN)
					return ret;
				else
					continue;
			} else {
				log_debug("header len > chunksize");
				return ERROR_HTTP;
			}
		}

		crlf_pos = crlf_ptr - data;
		if (crlf_pos == 0) {
			/* End of headers */
			os_memmove(data, &data[2], len - 2 + 1); /* Be sure to move NULL-terminating char as well */
			len -= 2;
			break;
		}

		data[crlf_pos] = '\0';

		n = sscanf(data, "%31[^:]: %31[^\r\n]", key, value);
		if (n == 2) {
			log_debug("Read header : %s: %s", key, value);
			if (!os_strcasecmp(key, "Content-Length")) {
				sscanf(value, "%d", &(client_data->response_content_len));
				client_data->retrieve_len = client_data->response_content_len;
			} else if (!os_strcasecmp(key, "Transfer-Encoding")) {
				if (!os_strcasecmp(value, "Chunked")) {
					client_data->is_chunked = true;
					client_data->response_content_len = 0;
					client_data->retrieve_len = 0;
				}
			}
			os_memmove(data, &data[crlf_pos + 2], len - (crlf_pos + 2) + 1); /* Be sure to move NULL-terminating char as well */
			len -= (crlf_pos + 2);

		} else {
			log_err("Could not parse header");
			return ERROR_HTTP;
		}
	}

	if (client->response_code != 200) {
		BK_LOGD(NULL, "Could not found\r\n");
		return MQTT_SUB_INFO_NOT_FOUND_ERROR;
	}

	return httpclient_retrieve_content(client, data, len, iotx_time_left(&timer), client_data);
}


iotx_err_t httpclient_connect(httpclient_t *client)
{
	client->net.handle = 0;

	return httpclient_conn(client);
}

iotx_err_t httpclient_send_request(httpclient_t *client, const char *url, int method, httpclient_data_t *client_data)
{
	int ret = ERROR_HTTP_CONN;

	if (0 == client->net.handle) {
		log_debug("not connection have been established");
		return ret;
	}

	ret = httpclient_send_header(client, url, method, client_data);
	if (ret != 0) {
		log_err("httpclient_send_header is error,ret = %d", ret);
		return ret;
	}

	if (method == HTTPCLIENT_POST || method == HTTPCLIENT_PUT)
		ret = httpclient_send_userdata(client, client_data);

	return ret;
}

iotx_err_t httpclient_recv_response(httpclient_t *client, uint32_t timeout_ms, httpclient_data_t *client_data)
{
	int reclen = 0, ret = ERROR_HTTP_CONN;
	char buf[HTTPCLIENT_CHUNK_SIZE] = { 0 };
	iotx_time_t timer;

	iotx_time_init(&timer);
	utils_time_countdown_ms(&timer, timeout_ms);

	if (0 == client->net.handle) {
		log_debug("not connection have been established");
		return ret;
	}


	if (client_data->is_more) {
		client_data->response_buf[0] = '\0';
		ret = httpclient_retrieve_content(client, buf, reclen, iotx_time_left(&timer), client_data);
	} else {
		client_data->is_more = 1;
		ret = httpclient_recv(client, buf, 1, HTTPCLIENT_CHUNK_SIZE - 1, &reclen, iotx_time_left(&timer));
		if (ret != 0)
			return ret;

		buf[reclen] = '\0';

		if (reclen) {
			log_multi_line(LOG_DEBUG_LEVEL, "RESPONSE", "%s", buf, "<");
			ret = httpclient_response_parse(client, buf, reclen, iotx_time_left(&timer), client_data);
		}
	}

	return ret;
}

void httpclient_close(httpclient_t *client)
{
	if (client->net.handle > 0)
		client->net.disconnect(&client->net);
	client->net.handle = 0;
}

int httpclient_common(httpclient_t *client, const char *url, int port, const char *ca_crt, int method,
					  uint32_t timeout_ms,
					  httpclient_data_t *client_data)
{
	iotx_time_t timer;
	int ret = 0;
	int url_port = 0;
	char *host = NULL;
	char *path = NULL;
	char scheme[8] = { 0 };

	do {
		if (NULL == (host = (char *)os_zalloc(HTTPCLIENT_MAX_HOST_LEN))) {
			log_err("not enough memory");
			ret = FAIL_RETURN;
			break;
		}

		if (NULL == (path = (char *)os_zalloc(HTTPCLIENT_MAX_URL_LEN))) {
			log_err("not enough memory");
			ret = FAIL_RETURN;
			break;
		}

		if (0 == client->net.handle) {
			//Establish connection if no.
			// httpclient_parse_host(url, host, sizeof(host));

			/* First we need to parse the url (http[s]://host[:port][/[path]]) */
			ret = httpclient_parse_url(url, scheme, sizeof(scheme), host, HTTPCLIENT_MAX_HOST_LEN, &url_port, path, HTTPCLIENT_MAX_URL_LEN);
			if (ret != SUCCESS_RETURN) {
				log_err("httpclient_parse_url returned %d", ret);
				break;
			}

			log_info("host: '%s', port: %d, url_port: %d.", host, port, url_port);

			if(url_port == 0) {
				url_port = port;
			}

			iotx_net_init(&client->net, host, url_port, ca_crt);

			ret = httpclient_connect(client);
			if (0 != ret) {
				log_err("httpclient_connect is error,ret = %d", ret);
				httpclient_close(client);
				break;
			}

			ret = httpclient_send_request(client, url, method, client_data);
			if (0 != ret) {
				log_err("httpclient_send_request is error,ret = %d", ret);
				httpclient_close(client);
				break;
			}
		}

		iotx_time_init(&timer);
		utils_time_countdown_ms(&timer, timeout_ms);
#ifdef CONFIG_HTTP_OTA_WITH_BLE
#if CONFIG_BLUETOOTH
        //bk_ble_register_sleep_state_callback(ble_sleep_cb);
#endif
#endif
		if ((NULL != client_data->response_buf)
			|| (0 != client_data->response_buf_len)) {
			ret = httpclient_recv_response(client, iotx_time_left(&timer), client_data);
			if (ret < 0) {
				log_err("httpclient_recv_response is error,ret = %d", ret);
				#if HTTP_WR_TO_FLASH
				http_flash_deinit();
				httpclient_close(client);
				#endif
				break;
			}
		}

		if (! client_data->is_more) {
			//Close the HTTP if no more data.
			log_info("close http channel");
			httpclient_close(client);
		}

		ret = (ret >= 0)? 0 : -1;
	} while(0);

	if (NULL != host) {
		os_free(host);
		host = NULL;
	}

	if (NULL != path) {
		os_free(path);
		path = NULL;
	}

	return ret;
}

int utils_get_response_code(httpclient_t *client)
{
	return client->response_code;
}

iotx_err_t iotx_post(
	httpclient_t *client,
	const char *url,
	int port,
	const char *ca_crt,
	uint32_t timeout_ms,
	httpclient_data_t *client_data)
{
	return httpclient_common(client, url, port, ca_crt, HTTPCLIENT_POST, timeout_ms, client_data);
}
#endif
