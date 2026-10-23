/** @file         ../apps/ftp/qurl_ftp_app.c
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-11-28 20:44:10
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qurl_ftp_app.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
#include "unistd.h"
#endif

#if QURL_CFG_APPS_FTP

#define QURL_APPS_FTP_LOG                (1)
#define QURL_APPS_FTP_DOWNLOAD           (1)
#define QURL_APPS_FTP_DOWNLOAD_RANGE     (0)
#define QURL_APPS_FTP_DOWNLOAD_PORT      (0)
#define QURL_APPS_FTPS_DOWNLOAD          (0)
#define QURL_APPS_FTPS_EXPILCIT_DOWNLOAD (0)
#define QURL_APPS_FTP_UPLOAD             (0)
#define QURL_APPS_FTP_CUSTOMREQUEST      (0)
#define QURL_APPS_FTP_NOBODY             (0)
//Verify whether the behavior is normal when downloading variable-length data and the server actively closes the data connection.
#define QURL_APPS_FTP_TEST1              (0)
#define QURL_APPS_FTP_DOWNLOAD_HUGE      (0)

#define QURL_LOG_APP_YELLOW              "\033[0;33m"
#define qurl_app_log(debug, msg, ...)                                                                                                                          \
    do {                                                                                                                                                       \
        if ((debug) != 0)                                                                                                                                      \
        {                                                                                                                                                      \
            printf(QURL_LOG_APP_YELLOW "%s(),%d:" msg, __FUNCTION__, __LINE__, ##__VA_ARGS__);                                                                 \
        }                                                                                                                                                      \
    } while (0)

#define QURL_FTP_APP_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define QURL_FTP_APP_MIN(a, b) (((a) < (b)) ? (a) : (b))
typedef struct qurl_data_buf_s
{
    long  data_size;
    long  read_index;
    char *data_buf;
} qurl_data_buf_t;

#if QURL_APPS_FTP_DOWNLOAD
static char g_buff[2049] = {0};

static long qurl_ftp_app_w_h_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("%s", g_buff);

    return len;
}

static long qurl_ftp_app_w_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("app>>>>>>\r\n%s\r\napp<<<<<<\r\n", g_buff);

    return len;
}

qurl_ecode_t qurl_ftp_app_init(void)
{
    qurl_ecode_t ret = QURL_OK;
    qurl_core_t  core = QOSA_NULL;

    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
        return ret;
    }

    qurl_app_log(QURL_APPS_FTP_LOG, "\r\ndownload\r\n");
    // qurl_core_setopt(core, QURL_OPT_URL, "ftp://jamie:jamie123@192.168.100.128:21/work/test/test.txt");
    qurl_core_setopt(core, QURL_OPT_URL, "ftp://lzm:lzmjamie@192.168.100.100:21/work/test/test.txt");
    // qurl_core_setopt(core, QURL_OPT_URL, "ftp://lzm:lzmjamie@[2000::11]:21/work/test/test.txt");
    // qurl_core_setopt(core, QURL_OPT_FTP_USE_EPSV, 0L);
    // qurl_core_setopt(core, QURL_OPT_FTP_PORT, ":8888-8889");
    // qurl_core_setopt(core, QURL_OPT_URL, "ftp://39.104.17.118:21/jamie_test/test.txt");
    // qurl_core_setopt(core, QURL_OPT_URL, "ftp://39.104.17.118:21/jamie_test/");
    // qurl_core_setopt(core, QURL_OPT_USERNAME, "st_public");
    // qurl_core_setopt(core, QURL_OPT_PASSWORD, "qwer!@#123");

    qurl_core_setopt(core, QURL_OPT_FTP_PORT, "-");
    // qurl_core_setopt(core, QURL_OPT_FTP_USE_EPRT, 1L);

    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);

    qurl_core_setopt(core, QURL_OPT_TIMEOUT_MS, 0L);
    qurl_core_setopt(core, QURL_OPT_IDLE_TIMEOUT_MS, 10 * 1000L);

    ret = qurl_core_perform(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    // qurl_app_log(QURL_APPS_FTP_LOG, "\r\n\r\nsleeping\r\n\r\n");
    // sleep(130);
    // qurl_app_log(QURL_APPS_FTP_LOG, "\r\n\r\nwake up\r\n\r\n");

    // qurl_core_setopt(core, QURL_OPT_CUSTOMREQUEST, QOSA_NULL);
    // qurl_core_setopt(core, QURL_OPT_URL, "ftp://39.104.17.118:21/jamie_test/test.txt");

    // ret = qurl_core_perform(core);
    // if (ret != QURL_OK)
    // {
    // 	qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    // }

    qurl_core_delete(core);

    return ret;
}
#endif

#if QURL_APPS_FTPS_DOWNLOAD
static char g_buff[2049] = {0};

static long qurl_ftp_app_w_h_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("%s", g_buff);

    return len;
}

static long qurl_ftp_app_w_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("app>>>>>>\r\n%s\r\napp<<<<<<\r\n", g_buff);

    // sleep(10);

    return len;
}

qurl_ecode_t qurl_ftp_app_init(void)
{
    qurl_ecode_t   ret = QURL_OK;
    qurl_core_t    core = QOSA_NULL;
    qurl_tls_cfg_t tls_cfg = {0};

    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
        return ret;
    }

    qurl_app_log(QURL_APPS_FTP_LOG, "\r\ndownload\r\n");
    // qurl_core_setopt(core, QURL_OPT_URL, "ftp://jamie:jamie123@192.168.100.128:21/work/test/test.txt");
    // qurl_core_setopt(core, QURL_OPT_URL, "ftp://lzm:lzmjamie@192.168.100.100:21/work/test/test.txt");
    // qurl_core_setopt(core, QURL_OPT_FTP_USE_EPSV, 0L);
    // qurl_core_setopt(core, QURL_OPT_FTP_PORT, ":8888-8889");
    // qurl_core_setopt(core, QURL_OPT_URL, "ftps://192.168.100.100");
    // qurl_core_setopt(core, QURL_OPT_USERNAME, "lzm");
    // qurl_core_setopt(core, QURL_OPT_PASSWORD, "lzmjamie");
    qurl_core_setopt(core, QURL_OPT_URL, "ftps://112.31.84.164/jamie_test/");
    qurl_core_setopt(core, QURL_OPT_PORT, 8311L);
    qurl_core_setopt(core, QURL_OPT_USERNAME, "test");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "test");

    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);

    qurl_tls_cfg_init(&tls_cfg);
    qurl_core_setopt(core, QURL_OPT_TLS_CFG, &tls_cfg);

    qurl_core_setopt(core, QURL_OPT_CUSTOMREQUEST, "LIST .");

    ret = qurl_core_perform(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    qurl_core_setopt(core, QURL_OPT_URL, "ftps://112.31.84.164/jamie_test/test-FTPS.txt");
    ret = qurl_core_perform(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    qurl_core_setopt(core, QURL_OPT_URL, "ftps://112.31.84.164/jamie_test/test-FTPS1.txt");
    ret = qurl_core_perform(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    qurl_core_delete(core);

    return ret;
}
#endif

#if QURL_APPS_FTPS_EXPILCIT_DOWNLOAD
static char g_buff[2049] = {0};

static long qurl_ftp_app_w_h_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("%s", g_buff);

    return len;
}

static long qurl_ftp_app_w_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("app>>>>>>\r\n%s\r\napp<<<<<<\r\n", g_buff);

    // sleep(10);

    return len;
}

qurl_ecode_t qurl_ftp_app_init(void)
{
    qurl_ecode_t   ret = QURL_OK;
    qurl_core_t    core = QOSA_NULL;
    qurl_tls_cfg_t tls_cfg = {0};

    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
        return ret;
    }

    qurl_app_log(QURL_APPS_FTP_LOG, "\r\ndownload\r\n");

    qurl_core_setopt(core, QURL_OPT_URL, "ftp://112.31.84.164/jamie_test/jamie.txt");
    qurl_core_setopt(core, QURL_OPT_PORT, 8310L);
    qurl_core_setopt(core, QURL_OPT_USERNAME, "test");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "test");

    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);

    qurl_tls_cfg_init(&tls_cfg);
    qurl_core_setopt(core, QURL_OPT_TLS_CFG, &tls_cfg);
    qurl_core_setopt(core, QURL_OPT_TLS_USETLS, QURL_USETLS_ALL);

    // qurl_core_setopt(core, QURL_OPT_CUSTOMREQUEST, "LIST .");

    ret = qurl_core_perform(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    qurl_core_delete(core);

    return ret;
}
#endif

#if QURL_APPS_FTP_DOWNLOAD_RANGE
static char g_buff[2049] = {0};

static long qurl_ftp_app_w_h_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf(QURL_LOG_APP_YELLOW "%s", g_buff);

    return len;
}

static long qurl_ftp_app_w_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf(QURL_LOG_APP_YELLOW "app>>>>>>\r\n%s\r\napp<<<<<<\r\n", g_buff);

    sleep(3);
    return len;
}

qurl_ecode_t qurl_ftp_app_init(void)
{
    qurl_ecode_t ret = QURL_OK;
    qurl_core_t  core = QOSA_NULL;

    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
        return ret;
    }

    qurl_app_log(QURL_APPS_FTP_LOG, "\r\ndownload\r\n");
    qurl_core_setopt(core, QURL_OPT_URL, "ftp://39.104.17.118:21/jamie_test/test30.txt");
    qurl_core_setopt(core, QURL_OPT_USERNAME, "st_public");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "qwer!@#123");

    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);

    qurl_core_setopt(core, QURL_OPT_TIMEOUT_MS, 0L);
    qurl_core_setopt(core, QURL_OPT_IDLE_TIMEOUT_MS, 10 * 1000L);

    qurl_core_setopt(core, QURL_OPT_RANGE, "0-5");

    qurl_core_perform(core);

    sleep(3);

    qurl_core_reset(core);
    qurl_core_setopt(core, QURL_OPT_URL, "ftp://39.104.17.118:21/jamie_test/test30.txt");
    qurl_core_setopt(core, QURL_OPT_USERNAME, "st_public");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "qwer!@#123");

    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);

    qurl_core_setopt(core, QURL_OPT_TIMEOUT_MS, 0L);
    qurl_core_setopt(core, QURL_OPT_IDLE_TIMEOUT_MS, 10 * 1000L);

    qurl_core_setopt(core, QURL_OPT_RESUME_FROM, 15L);

    ret = qurl_core_perform(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    qurl_core_delete(core);

    return ret;
}
#endif

#if QURL_APPS_FTP_DOWNLOAD_PORT
static char g_buff[2049] = {0};

static long qurl_ftp_app_w_h_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf(QURL_LOG_APP_YELLOW "%s", g_buff);

    return len;
}

static long qurl_ftp_app_w_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf(QURL_LOG_APP_YELLOW "app>>>>>>\r\n%s\r\napp<<<<<<\r\n", g_buff);

    return len;
}

qurl_ecode_t qurl_ftp_app_init(void)
{
    qurl_ecode_t ret = QURL_OK;
    qurl_core_t  core = QOSA_NULL;

    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
        return ret;
    }

    qurl_app_log(QURL_APPS_FTP_LOG, "\r\ndownload\r\n");
    qurl_core_setopt(core, QURL_OPT_URL, "ftp://hwy.lizhuming.top:20021/test/test.txt");
    qurl_core_setopt(core, QURL_OPT_USERNAME, "jamie");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "jamieunir");

    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);

    qurl_core_setopt(core, QURL_OPT_TIMEOUT_MS, 0L);
    qurl_core_setopt(core, QURL_OPT_IDLE_TIMEOUT_MS, 10 * 1000L);

    qurl_core_setopt(core, QURL_OPT_FTP_PORT, ":8888-8889");

    qurl_core_perform(core);

    qurl_core_delete(core);

    return ret;
}
#endif

#if (QURL_APPS_FTP_UPLOAD)
static char g_buff[2049] = {0};

char g_upload_data[] = {"this is qurl ftp upload!\r\nhhhhh\r\nthis is data!!!\r\nzzyyds!!\r\n"};

static long qurl_ftp_app_w_h_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("%s", g_buff);

    return len;
}

static long qurl_ftp_app_w_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("app>>>>>>\r\n%s\r\napp<<<<<<\r\n", g_buff);

    return len;
}

static long qurl_ftp_app_r_cb(char *buf, long size, void *arg)
{
    long             len = 0;
    qurl_data_buf_t *data_buf_ptr = (qurl_data_buf_t *)arg;

    len = QURL_FTP_APP_MIN(size, data_buf_ptr->data_size - data_buf_ptr->read_index);

    qosa_memcpy(buf, data_buf_ptr->data_buf + data_buf_ptr->read_index, len);
    data_buf_ptr->read_index += len;

    qurl_app_log(QURL_APPS_FTP_LOG, "upload len:%d\r\n", len);

    return len;
}

qurl_ecode_t qurl_ftp_app_init(void)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_core_t      core = QOSA_NULL;
    qurl_slist_t     quote_slist = QOSA_NULL;
    long             content_length = 0;
    qurl_data_buf_t  data_buf = {0};
    qurl_so_linger_t so_linger = {0};

    (void)quote_slist;
    (void)content_length;

    data_buf.data_size = sizeof(g_upload_data);
    data_buf.data_buf = (char *)malloc(data_buf.data_size);
    qosa_memcpy(data_buf.data_buf, g_upload_data, data_buf.data_size);
    data_buf.read_index = 0;

    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    qurl_core_setopt(core, QURL_OPT_URL, "ftp://39.104.17.118:21/jamie_test/upload_test.txt");
    qurl_core_setopt(core, QURL_OPT_USERNAME, "st_public");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "qwer!@#123");

    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);
    qurl_core_setopt(core, QURL_OPT_READ_CB, qurl_ftp_app_r_cb);
    qurl_core_setopt(core, QURL_OPT_READ_CB_ARG, (void *)&data_buf);

    qurl_core_setopt(core, QURL_OPT_APPEND, 1L);

    so_linger.l_onoff = 1;
    qurl_core_setopt(core, QURL_OPT_SOCKET_SO_LINGER, &so_linger);

    ret = qurl_core_perform(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }
    qurl_core_delete(core);
    qosa_free(data_buf.data_buf);

    return QURL_OK;
}
#endif

#if QURL_APPS_FTP_TEST1
static char g_buff[2049] = {0};

static long qurl_ftp_app_w_h_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("%s", g_buff);

    return len;
}

static long qurl_ftp_app_w_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("app>>>>>>\r\n%s\r\napp<<<<<<\r\n", g_buff);

    return len;
}

qurl_ecode_t qurl_ftp_app_init(void)
{
    qurl_ecode_t    ret = QURL_OK;
    qurl_core_t     core = QOSA_NULL;
    qurl_data_buf_t data_buf = {0};

    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    ret = qurl_core_setopt(core, QURL_OPT_URL, "ftp://39.104.17.118:21");
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }
    qurl_core_setopt(core, QURL_OPT_USERNAME, "st_public");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "qwer!@#123");

    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);

    // qurl_core_setopt(core, QURL_OPT_CUSTOMREQUEST, "LIST .");

    ret = qurl_core_perform(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    ret = qurl_core_delete(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    qosa_free(data_buf.data_buf);

    return QURL_OK;
}
#endif

#if QURL_APPS_FTP_DOWNLOAD_HUGE
static char g_buff[2049] = {0};

static long qurl_ftp_app_w_h_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("%s", g_buff);

    return len;
}

static long qurl_ftp_app_w_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("app>>>>>>\r\n%s\r\napp<<<<<<\r\n", g_buff);

    return len;
}

qurl_ecode_t qurl_ftp_app_init(void)
{
    qurl_ecode_t    ret = QURL_OK;
    qurl_core_t     core = QOSA_NULL;
    qurl_data_buf_t data_buf = {0};

    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    ret = qurl_core_setopt(core, QURL_OPT_URL, "ftp://39.104.17.118:21/Aneira/80M");
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }
    qurl_core_setopt(core, QURL_OPT_USERNAME, "st_public");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "qwer!@#123");

    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);

    qurl_core_setopt(core, QURL_OPT_TIMEOUT_MS, 20 * 60 * 1000);

    // qurl_core_setopt(core, QURL_OPT_CUSTOMREQUEST, "CWD /Aneira");

    ret = qurl_core_perform(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    ret = qurl_core_delete(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    qosa_free(data_buf.data_buf);

    return QURL_OK;
}
#endif

#if (QURL_APPS_FTP_CUSTOMREQUEST)
static char g_buff[2049] = {0};

static long qurl_ftp_app_w_h_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("%s", g_buff);

    return len;
}

static long qurl_ftp_app_w_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("app>>>>>>\r\n%s\r\napp<<<<<<\r\n", g_buff);

    return len;
}

qurl_ecode_t qurl_ftp_app_init(void)
{
    qurl_ecode_t ret = QURL_OK;
    qurl_core_t  core = QOSA_NULL;
    qurl_slist_t quote_slist = QOSA_NULL;
    long         content_length = 0;

    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    // ret = qurl_core_setopt(core, QURL_OPT_URL, "ftp://jamie:jamie123@10.99.64.82:21/work/test/jamie.txt");
    // ret = qurl_core_setopt(core, QURL_OPT_URL, "ftp://jamie:jamie123@192.168.100.128:21/work/test/test.txt");
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);

    qurl_app_log(QURL_APPS_FTP_LOG, "\r\ndownload\r\n");
    // qurl_core_setopt(core, QURL_OPT_URL, "ftp://jamie:jamie123@192.168.100.128/work/test/test.txt");
    qurl_core_setopt(core, QURL_OPT_URL, "ftp://jamie:jamie123@192.168.100.128");
    // qurl_core_setopt(core, QURL_OPT_URL, "ftp://jamie:123456@10.99.64.82:2121/");
    // qurl_core_setopt(core, QURL_OPT_URL, "ftps://jamie:123456@10.99.64.82:2990/");
    qurl_core_setopt(core, QURL_OPT_USERNAME, "jamie");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "jamie123");
    qurl_core_setopt(core, QURL_OPT_TIMEOUT_MS, 100 * 1000);
    qurl_core_setopt(core, QURL_OPT_UPLOAD_DATA, QOSA_NULL);
    qurl_core_setopt(core, QURL_OPT_UPLOAD_SIZE, 0L);
    qurl_core_setopt(core, QURL_OPT_FTP_USE_EPSV, 0L);
    // qurl_core_setopt(core, QURL_OPT_FTP_PORT, ":8888-9999");
    // qurl_core_setopt(core, QURL_OPT_CUSTOMREQUEST, "LIST");
    qurl_core_setopt(core, QURL_OPT_NOBODY, 1L);
    qurl_core_setopt(core, QURL_OPT_REUSE_HOLD, 1L);
    qurl_core_perform(core);
    qurl_core_getinfo(core, QURL_INFO_RESP_CONTENT_LENGTH, &content_length);
    qurl_app_log(QURL_APPS_FTP_LOG, "\r\nlen:%ld\r\n", content_length);
    qurl_app_log(QURL_APPS_FTP_LOG, "\r\nperform2\r\n\r\n");
    qurl_core_setopt(core, QURL_OPT_FILETIME, 1L);
    quote_slist = qurl_slist_add_strdup(quote_slist, "PWD");
    qurl_core_setopt(core, QURL_OPT_URL, "ftp://39.104.17.118:21/jamie_test/test30.txt");
    qurl_core_setopt(core, QURL_OPT_USERNAME, "st_public");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "qwer!@#123");
    qurl_core_perform(core);
    qurl_core_getinfo(core, QURL_INFO_FILETIME, &content_length);
    qurl_app_log(QURL_APPS_FTP_LOG, "\r\ntime:%ld\r\n", content_length);
    if (content_length >= 0)
    {
        //Convert the time value to a date-time string
        struct tm *info;
        char       buffer[80];

        info = localtime(&content_length);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);

        printf("Remote file time: %s\n", buffer);
    }
    else
    {
        printf("Remote file time not available\n");
    }
    qurl_core_getinfo(core, QURL_INFO_RESP_CONTENT_LENGTH, &content_length);
    qurl_app_log(QURL_APPS_FTP_LOG, "\r\nlen:%ld\r\n", content_length);
    qurl_slist_del_all(quote_slist);
    quote_slist = QOSA_NULL;

    ret = qurl_core_delete(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    return QURL_OK;
}
#endif

#if (QURL_APPS_FTP_NOBODY)
static char g_buff[2049] = {0};

static long qurl_ftp_app_w_h_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("%s", g_buff);

    return len;
}

static long qurl_ftp_app_w_cb(char *buf, long size, void *arg)
{
    long len = size;
    long log_len = len > 2048 ? 2048 : len;

    (void)arg;

    qosa_memcpy(g_buff, buf, log_len);
    g_buff[log_len] = 0;

    printf("app>>>>>>\r\n%s\r\napp<<<<<<\r\n", g_buff);

    return len;
}

qurl_ecode_t qurl_ftp_app_init(void)
{
    qurl_ecode_t ret = QURL_OK;
    qurl_core_t  core = QOSA_NULL;
    qurl_slist_t quote_slist = QOSA_NULL;
    long         content_length = 0;

    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    qurl_core_setopt(core, QURL_OPT_URL, "ftp://39.104.17.118:21/jamie_test/test30.txt");
    qurl_core_setopt(core, QURL_OPT_USERNAME, "st_public");
    qurl_core_setopt(core, QURL_OPT_PASSWORD, "qwer!@#123");

    qurl_core_setopt(core, QURL_OPT_WRITE_CB, qurl_ftp_app_w_cb);
    qurl_core_setopt(core, QURL_OPT_WRITE_HEAD_CB, qurl_ftp_app_w_h_cb);
    qurl_core_setopt(core, QURL_OPT_NOBODY, 1L);

    qurl_core_setopt(core, QURL_OPT_FILETIME, 1L);

    qurl_core_perform(core);

    qurl_core_getinfo(core, QURL_INFO_FILETIME, &content_length);
    qurl_app_log(QURL_APPS_FTP_LOG, "\r\ntime:%ld\r\n", content_length);
    if (content_length >= 0)
    {
        //Convert time value to date-time string
        struct tm *info;
        char       buffer[80];

        info = localtime(&content_length);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);

        printf("Remote file time: %s\n", buffer);
    }
    else
    {
        printf("Remote file time not available\n");
    }
    qurl_app_log(QURL_APPS_FTP_LOG, "\r\nlen:%ld\r\n", content_length);
    qurl_slist_del_all(quote_slist);
    quote_slist = QOSA_NULL;

    ret = qurl_core_delete(core);
    if (ret != QURL_OK)
    {
        qurl_app_log(QURL_APPS_FTP_LOG, "%x\r\n", ret);
    }

    return QURL_OK;
}
#endif

#endif /* QURL_CFG_APPS_FTP */