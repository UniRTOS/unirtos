/*****************************************************************/ /**
* @file mbedtls.c
* @brief
* @author larson.li@quectel.com
* @date 2023-08-09
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2023-08-09 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#include "mbedtls.h"
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "qcm_vtls.h"
#include "qosa_rtc.h"
#include "qosa_virtual_file.h"
#include "qcm_proj_config.h"
#ifdef CONFIG_QCM_FILE_API_FUNC
#include "qcm_file_api.h"
#endif /* CONFIG_QCM_FILE_API_FUNC */
#include "qcm_vtls_cfg.h"
#include "qcm_utils.h"
#include <mbedtls/version.h>
#if MBEDTLS_VERSION_NUMBER >= 0x02040000
#include <mbedtls/net_sockets.h>
#else
#include <mbedtls/net.h>
#endif
#include <mbedtls/ssl.h>
#include <mbedtls/x509.h>

#if MBEDTLS_VERSION_NUMBER >= 0x03000000
#include "mbedtls/build_info.h"
#endif /* MBEDTLS_VERSION_NUMBER */

#include <mbedtls/error.h>

#ifdef CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC
#include <mbedtls/private/entropy.h>
#include <mbedtls/private/ctr_drbg.h>
#include <mbedtls/private/sha256.h>
#include <mbedtls/x509_crt.h>
#else /* CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC */
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/sha256.h>
#endif /* CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC */

#include "mbedtls/debug.h"
#if defined(MBEDTLS_TIMING_C)
#include "mbedtls/timing.h"
#endif

#define QOS_LOG_TAG LOG_TAG_MBED_API

static qcm_vtls_result_status_e mbed_connect_step2(qcm_ssl_connect_data_t *connssl);

/**
 * @struct mbed_ssl_backend_data
 * @brief ssl mbedtls extension data structures
 */
struct mbed_ssl_backend_data
{
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context  entropy;
    mbedtls_ssl_context      ssl;
    mbedtls_x509_crt         cacert;
    mbedtls_x509_crt         clicert;
    mbedtls_pk_context       pk;
    mbedtls_ssl_config       config;
#if defined(MBEDTLS_SSL_ALPN)
    const char               *alpn_list[2];
#endif
#if defined(MBEDTLS_TIMING_C)
    mbedtls_timing_delay_context timer;
#endif
};

/**
 * @enum mbedtls_dtls_event_e
 * @brief DTLS retransmission situation
 */
typedef enum
{
    MBEDTLS_DTLS_CREATE = 1,         /*!< DTLS connection creation notification*/
    MBEDTLS_DTLS_RETRANSMISSION = 2, /*!< DTLS timeout requires retransmission event notification*/
    MBEDTLS_DTLS_CONNECTED = 3,      /*!< DTLS connection successful event notification*/
    MBEDTLS_DTLS_CLOSE = 4           /*!< DTLS connection closed event notification*/
} mbedtls_dtls_event_e;

/**
 * @struct mbedtls_dtls_task_event_t
 * @brief Structure for DTLS task event notification
 */
typedef struct
{
    mbedtls_dtls_event_e    event_id;
    qcm_ssl_connect_data_t *connssl;
} mbedtls_dtls_task_event_t;
#if defined(MBEDTLS_SSL_PROTO_DTLS)
static qosa_mutex_t g_dtls_retransmission_lock = QOSA_NULL; /*!< DTLS retransmission task lock*/
static qosa_task_t  g_dtls_retransmission_task = QOSA_NULL; /*!< DTLS retransmission task*/
static qosa_msgq_t  g_dtls_retransmission_msgq = QOSA_NULL; /*!< Message queue in the DTLS retransmission task*/
#endif /* MBEDTLS_SSL_PROTO_DTLS */
/**
 * @struct mbed_file_crt_info_t
 * @brief The read certificate file is being cleaned.
 */
typedef struct
{
    qosa_uint32_t file_len;  /*!< Length of the read file*/
    char         *file_data; /*!< Read file data*/
} mbed_file_crt_info_t;

/**
 * @brief Check if the global CA certificate path is set
 */
extern qosa_bool_t qcm_ssl_check_is_set_cacertex(void);

/**
 * @brief Get the global CA certificate path address
 */
extern char **qcm_ssl_global_cacertex_path(void);

// clang-format off
/*
 *  The primary purpose of the profile is to support a minimum key length of RSA 1024.
 */
static const mbedtls_x509_crt_profile mbedtls_x509_crt_profile_fr =
{
  /* Hashes from SHA-1 and above */
  MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA1) |
  MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_RIPEMD160) |
  MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA224) |
  MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA256) |
  MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA384) |
  MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA512),
  0xFFFFFFF, /* Any PK alg    */
  0xFFFFFFF, /* Any curve     */
  1024,      /* RSA min key len */
};
// clang-format on
#ifdef MBEDTLS_DEBUG_C
#define MBEDTLS_DEBUG 1
#endif
#ifdef MBEDTLS_DEBUG
static void mbed_debug(void *context, int level, const char *f_name, int line_nb, const char *line)
{
    QLOGV_EX("%s[%s, %d][level:%d] %s", context, f_name, line_nb, level, line);
}
#else
#endif

/**
 * @brief Set TLS version selection min max
 */
static void mbedtls_set_version_switch(qcm_ssl_config_t *conf, struct mbed_ssl_backend_data *backend)
{
#if MBEDTLS_VERSION_NUMBER < 0x03000000
    int mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_3;
    int mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_3;

    if (conf->transport == QCM_SSL_TLS_PROTOCOL)
    {
        switch (conf->ssl_version)
        {
            case QCM_SSL_VERSION_0:
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_0;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_0;
                break;
            case QCM_SSL_VERSION_1:
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_1;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_1;
                break;
            case QCM_SSL_VERSION_2:
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_2;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_2;
                break;
            case QCM_SSL_VERSION_3:
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_3;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_3;
                break;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
             case QCM_SSL_VERSION_ALL:
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_0;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_3;
                break;
#endif
            default:
                break;
        }
    }
    if (conf->transport == QCM_SSL_DTLS_PROTOCOL)
    {
        switch (conf->dtls_version)
        {
            case QCM_SSL_VER_DTLS10:
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_2;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_2;
                break;
            case QCM_SSL_VER_DTLS12:
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_3;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_3;
                break;
            case QCM_SSL_VER_DTLS_ALL:
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_2;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_3;
                break;
            default:
                break;
        }
    }
#elif defined(CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC)
    int mbedtls_ver_min = MBEDTLS_SSL_VERSION_TLS1_2;
    int mbedtls_ver_max = MBEDTLS_SSL_VERSION_TLS1_2;

    if (conf->transport == QCM_SSL_TLS_PROTOCOL)
    {
        switch (conf->ssl_version)
        {
            case QCM_SSL_VERSION_3:  //SSL1.2
                mbedtls_ver_min = MBEDTLS_SSL_VERSION_TLS1_2;
                mbedtls_ver_max = MBEDTLS_SSL_VERSION_TLS1_2;
                break;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
             case QCM_SSL_VERSION_ALL:
                mbedtls_ver_min = MBEDTLS_SSL_VERSION_TLS1_2;
                mbedtls_ver_max = MBEDTLS_SSL_VERSION_TLS1_3;
                break;
#endif
            case QCM_SSL_VERSION_5:  //TLS1.3
                mbedtls_ver_min = MBEDTLS_SSL_VERSION_TLS1_3;
                mbedtls_ver_max = MBEDTLS_SSL_VERSION_TLS1_3;
                break;
            default:
                break;
        }
    }
    else if (conf->transport == QCM_SSL_DTLS_PROTOCOL)
    {
        switch (conf->dtls_version)
        {
            case QCM_SSL_VER_DTLS12:
            case QCM_SSL_VER_DTLS_ALL:
                mbedtls_ver_min = MBEDTLS_SSL_VERSION_TLS1_2;
                mbedtls_ver_max = MBEDTLS_SSL_VERSION_TLS1_2;
                break;
            default:
                break;
        }
    }
#else
    int mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_3;
    int mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_3;

    if (conf->transport == QCM_SSL_TLS_PROTOCOL)
    {
        switch (conf->ssl_version)
        {
            case QCM_SSL_VERSION_0:  //SSL3.0
            case QCM_SSL_VERSION_1:  //SSL1.0
            case QCM_SSL_VERSION_2:  //SSL1.1
            case QCM_SSL_VERSION_3:  //SSL1.2
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_3;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_3;
                break;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
             case QCM_SSL_VERSION_ALL:
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_3;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_4;
                break;
#endif
            case QCM_SSL_VERSION_5:  //TLS1.3
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_4;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_4;
                break;
            default:
                break;
        }
    }
    if (conf->transport == QCM_SSL_DTLS_PROTOCOL)
    {
        switch (conf->dtls_version)
        {
            case QCM_SSL_VER_DTLS10:
            case QCM_SSL_VER_DTLS12:
            case QCM_SSL_VER_DTLS_ALL:
                mbedtls_ver_min = MBEDTLS_SSL_MINOR_VERSION_3;
                mbedtls_ver_max = MBEDTLS_SSL_MINOR_VERSION_3;
                break;
            default:
                break;
        }
    }
#endif

#ifdef CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC
    mbedtls_ssl_conf_min_tls_version(&backend->config, mbedtls_ver_min);
    mbedtls_ssl_conf_max_tls_version(&backend->config, mbedtls_ver_max);
#else /* CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC */
    mbedtls_ssl_conf_min_version(&backend->config, MBEDTLS_SSL_MAJOR_VERSION_3, mbedtls_ver_min);
    mbedtls_ssl_conf_max_version(&backend->config, MBEDTLS_SSL_MAJOR_VERSION_3, mbedtls_ver_max);
#endif /* CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC */
}
#if defined(MBEDTLS_SSL_PROTO_TLS1_3)
static void mbedtls_keylog_cb(
    void                       *p,
    mbedtls_ssl_key_export_type type,
    const unsigned char        *secret,
    size_t                      secret_len,
    const unsigned char         client_random[32],
    const unsigned char         server_random[32],
    mbedtls_tls_prf_types       prf
)
{
    QOSA_UNUSED(p);
    QOSA_UNUSED(prf);
    const char   *label = QOSA_NULL;
    char          client_random_str[72] = {0};
    char          traffic_secret[160] = {0};
    qosa_uint16_t i = 0;

    switch (type)
    {
        case MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET:
            label = "CLIENT_RANDOM";
            break;
#if defined(MBEDTLS_SSL_PROTO_TLS1_3)
        case MBEDTLS_SSL_KEY_EXPORT_TLS1_3_CLIENT_EARLY_SECRET:
            label = "CLIENT_EARLY_TRAFFIC_SECRET";
            break;
        case MBEDTLS_SSL_KEY_EXPORT_TLS1_3_EARLY_EXPORTER_SECRET:
            label = "EARLY_EXPORTER_SECRET";
            break;
        case MBEDTLS_SSL_KEY_EXPORT_TLS1_3_CLIENT_HANDSHAKE_TRAFFIC_SECRET:
            label = "CLIENT_HANDSHAKE_TRAFFIC_SECRET";
            break;
        case MBEDTLS_SSL_KEY_EXPORT_TLS1_3_SERVER_HANDSHAKE_TRAFFIC_SECRET:
            label = "SERVER_HANDSHAKE_TRAFFIC_SECRET";
            break;
        case MBEDTLS_SSL_KEY_EXPORT_TLS1_3_CLIENT_APPLICATION_TRAFFIC_SECRET:
            label = "CLIENT_TRAFFIC_SECRET_0";
            break;
        case MBEDTLS_SSL_KEY_EXPORT_TLS1_3_SERVER_APPLICATION_TRAFFIC_SECRET:
            label = "SERVER_TRAFFIC_SECRET_0";
            break;
#endif               /* MBEDTLS_SSL_PROTO_TLS1_3 */
        default:
            return;  //Other types do not need to be written.
    }

    QLOGD("%s ", label);
    for (; i < 32; i++)
    {
        qosa_sprintf(client_random_str + i * 2, "%02x", client_random[i]);
    }
    QLOGD("client_random: %s ", client_random_str);

    for (i = 0; i < secret_len; i++)
    {
        qosa_sprintf(traffic_secret + i * 2, "%02x", secret[i]);
    }
    QLOGD("traffic_secret: %s ", traffic_secret);
}
#endif /* MBEDTLS_SSL_PROTO_TLS1_3 */
/* Ignore CA check items */
static int mbed_ssl_verify(void *data, mbedtls_x509_crt *crt, int depth, qosa_uint32_t *flags)
{
    char          buf[1024] = {0};
    qosa_uint32_t ssl_ctx_ignore_certitem_cfg = *((qosa_uint32_t *)data);

    QLOGD("Verify requested for (Depth %d) flags=%d", depth, *flags);
    mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
    //QLOGD("%s", buf);

    /* The user is not configured to ignore CA check items
       or each CA item has been validated */
    if ((0 == ssl_ctx_ignore_certitem_cfg) || (0 == *flags))
    {
        return (0);
    }

    /* mbed has 20 CA checks
       If the user ignores a CA item
       and the check fails, change the item's flag */

    qosa_uint32_t cert_item[QCM_VTLS_SSL_CERT_ITEM_NUM]
        = {MBEDTLS_X509_BADCERT_EXPIRED,       MBEDTLS_X509_BADCERT_REVOKED,      MBEDTLS_X509_BADCERT_CN_MISMATCH, MBEDTLS_X509_BADCERT_NOT_TRUSTED,
           MBEDTLS_X509_BADCRL_NOT_TRUSTED,    MBEDTLS_X509_BADCRL_EXPIRED,       MBEDTLS_X509_BADCERT_MISSING,     MBEDTLS_X509_BADCERT_SKIP_VERIFY,
           MBEDTLS_X509_BADCERT_OTHER,         MBEDTLS_X509_BADCERT_FUTURE,       MBEDTLS_X509_BADCRL_FUTURE,       MBEDTLS_X509_BADCERT_KEY_USAGE,
           MBEDTLS_X509_BADCERT_EXT_KEY_USAGE, MBEDTLS_X509_BADCERT_NS_CERT_TYPE, MBEDTLS_X509_BADCERT_BAD_MD,      MBEDTLS_X509_BADCERT_BAD_PK,
           MBEDTLS_X509_BADCERT_BAD_KEY,       MBEDTLS_X509_BADCRL_BAD_MD,        MBEDTLS_X509_BADCRL_BAD_PK,       MBEDTLS_X509_BADCRL_BAD_KEY};

    int i = 0;
    for (i = 0; i < QCM_VTLS_SSL_CERT_ITEM_NUM; i++)
    {
        if (ssl_ctx_ignore_certitem_cfg & cert_item[i])
        {
            if (*flags & cert_item[i])
            {
                *flags &= ~(cert_item[i]);
            }
        }
    }

    mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
    QLOGD("%s\n", buf);
    return (0);
}
/**
 * @brief Adapt the mbedtls write function and return the internal mbedtls error status code
 */
static int mbed_bio_write(void *ctx, const unsigned char *buf, size_t len)
{
    int                     ret = 0;
    qcm_ssl_connect_data_t *ssl_ctx = (qcm_ssl_connect_data_t *)ctx;
    QLOGD("... %p", ssl_ctx->ssl_config.io_write);
    ret = ssl_ctx->ssl_config.io_write(ssl_ctx, buf, len);
    QLOGD("ret=%d", ret);

    if (ret < 0)
    {
        if (ret == QCM_VTLS_IO_ERR_SSL_WANT_WRITE || ret == QCM_VTLS_IO_ERR_SSL_WANT_READ)
        {
            ret = MBEDTLS_ERR_SSL_WANT_WRITE;
        }
        else if (ret == QCM_VTLS_IO_ERR_SSL_CONN_RESET)
        {
            ret = MBEDTLS_ERR_NET_CONN_RESET;
        }
    }

    return ret;
}

/**
 * @brief Adapt the mbedtls read function and return the internal mbedtls error status code
 */
static int mbed_bio_read(void *ctx, unsigned char *buf, size_t len)
{
    int                     ret = 0;
    qcm_ssl_connect_data_t *ssl_ctx = (qcm_ssl_connect_data_t *)ctx;
    QLOGD("... %p", ssl_ctx->ssl_config.io_read);
    ret = ssl_ctx->ssl_config.io_read(ssl_ctx, buf, len);
    QLOGD("ret=%d", ret);

    if (ret < 0)
    {
        if (ret == QCM_VTLS_IO_ERR_SSL_WANT_WRITE || ret == QCM_VTLS_IO_ERR_SSL_WANT_READ)
        {
            ret = MBEDTLS_ERR_SSL_WANT_READ;
        }
        else if (ret == QCM_VTLS_IO_ERR_SSL_CONN_RESET)
        {
            ret = MBEDTLS_ERR_NET_CONN_RESET;
        }
    }

    return ret;
}

/**
 * @brief Release mbed_file_crt_info_t application information
 *
 * @param[in] mbed_file_crt_info_t * data
 *          - Pointer address to be freed
 *
 */
static void mbed_file_data_free(mbed_file_crt_info_t *data)
{
    if (data != QOSA_NULL)
    {
        if (data->file_data != QOSA_NULL)
        {
            qosa_free(data->file_data);
        }
        qosa_free(data);
    }
}

/**
 * @brief Interface for obtaining certificate files
 *
 * @param[in] char * file_name
 *         - Corresponding certificate file name
 *
 * @return mbed_file_crt_info_t *
 *       - Returns a pointer to the mbed_file_crt_info_t structure type on success
 *       - Returns OSA_NULL on failure
 */
static mbed_file_crt_info_t *mbed_read_file_crt(char *file_name)
{
    QOSA_UNUSED(file_name);
#ifdef UNIRTOS_OPEN_MODE_SUPPORT
    struct qosa_vfs_stat_t fs_stat = {0};
    qosa_int32_t           cert_fd = -1;
    mbed_file_crt_info_t  *data = QOSA_NULL;

    data = (mbed_file_crt_info_t *)qosa_malloc(sizeof(mbed_file_crt_info_t));
    if (data == QOSA_NULL)
    {
        return QOSA_NULL;
    }

    if (qosa_vfs_stat(file_name, &fs_stat) == -1 || fs_stat.st_size == 0)
    {
        goto exit;
    }

    data->file_data = qosa_malloc(fs_stat.st_size + 1);
    if (data->file_data == QOSA_NULL)
    {
        goto exit;
    }

    qosa_memset(data->file_data, 0x00, fs_stat.st_size + 1);
    cert_fd = qosa_vfs_open(file_name, QOSA_VFS_O_RDONLY);
    // QLOGD("file_name :%s", file_name);
    data->file_len = qosa_vfs_read(cert_fd, data->file_data, fs_stat.st_size);

    qosa_vfs_close(cert_fd);

    return data;

exit:
    mbed_file_data_free(data);
#else  /*  UNIRTOS_OPEN_MODE_SUPPORT */
#ifdef CONFIG_QCM_FILE_API_FUNC
    struct qosa_vfs_stat_t fs_stat = {0};
    Q_FILE                 cert_fd = -1;
    mbed_file_crt_info_t  *data = QOSA_NULL;

    data = (mbed_file_crt_info_t *)qosa_malloc(sizeof(mbed_file_crt_info_t));
    if (data == QOSA_NULL)
    {
        return QOSA_NULL;
    }

    if (qcm_file_stat_func(file_name, &fs_stat) == -1 || fs_stat.st_size == 0)
    {
        goto exit;
    }

    data->file_data = qosa_malloc(fs_stat.st_size + 1);
    if (data->file_data == QOSA_NULL)
    {
        goto exit;
    }
    qosa_memset(data->file_data, 0x00, fs_stat.st_size + 1);
    cert_fd = qcm_file_fopen(file_name, "r");

    data->file_len = qcm_file_fread(data->file_data, fs_stat.st_size, 1, cert_fd);
    qcm_file_fclose(cert_fd);

    return data;

exit:
    mbed_file_data_free(data);
#endif /* CONFIG_QCM_FILE_API_FUNC */
#endif /* UNIRTOS_OPEN_MODE_SUPPORT */

    return QOSA_NULL;
}
#if defined(MBEDTLS_SSL_PROTO_DTLS)
/**
 * @brief Task for DTLS retransmission.
 *
 * @param[in] void * argv
 *          - Unused parameters
 */
static void mbedtls_dtls_retransmission_handler(void *argv)
{
    QOSA_UNUSED(argv);
    qosa_uint8_t dtls_count = 1;
    int          ret = 0;

    while (1)
    {
        mbedtls_dtls_task_event_t msg_info = {0};

        ret = qosa_msgq_wait(g_dtls_retransmission_msgq, (qosa_uint8_t *)&msg_info, sizeof(mbedtls_dtls_task_event_t), QOSA_WAIT_FOREVER);
        if (ret != QOSA_OK)
        {
            QLOGE("ret=%x", ret);
            continue;
        }

        QLOGD("event_id :%d", msg_info.event_id);
        switch (msg_info.event_id)
        {
            case MBEDTLS_DTLS_CREATE: {
                dtls_count++;
            }
            break;

            case MBEDTLS_DTLS_RETRANSMISSION: {
                if (msg_info.connssl->connecting_state <= QCM_SSL_CONNECT_DONE)
                {
                    ret = mbed_connect_step2(msg_info.connssl);
                    QLOGD("connecting_state : %d ret :%d", msg_info.connssl->connecting_state, ret);
                }
                else
                {
                    dtls_count--;
                }
            }
            break;

            case MBEDTLS_DTLS_CONNECTED:
            case MBEDTLS_DTLS_CLOSE: {
                dtls_count--;
            }
            break;

            default:
                break;
        }

        if (dtls_count <= 0)
        {
            QLOGV("dtls task delete!!!!");

            qosa_mutex_lock(g_dtls_retransmission_lock, QOSA_WAIT_FOREVER);

            if (g_dtls_retransmission_msgq != QOSA_NULL)
            {
                qosa_msgq_delete(g_dtls_retransmission_msgq);
                g_dtls_retransmission_msgq = QOSA_NULL;
            }

            g_dtls_retransmission_task = QOSA_NULL;
            qosa_mutex_unlock(g_dtls_retransmission_lock);
            break;
        }
    }
}
#endif /* MBEDTLS_SSL_PROTO_DTLS */

#if defined(MBEDTLS_TIMING_C) && defined(MBEDTLS_SSL_PROTO_DTLS)

/**
 * @brief Timer timeout function during DTLS handshake.
 *
 * @param[in] void * argv
 *          - Pointer to the corresponding SSL initialization structure
 */
static void mbedtls_retransmit_timer_handle(void *argv)
{
    mbedtls_dtls_task_event_t msg_info = {0};

    msg_info.event_id = MBEDTLS_DTLS_RETRANSMISSION;
    msg_info.connssl = (qcm_ssl_connect_data_t *)argv;

    QLOGV("mbedtls_retransmit_timer_handle");
    qosa_msgq_release(g_dtls_retransmission_msgq, sizeof(mbedtls_dtls_task_event_t), (qosa_uint8_t *)&msg_info, QOSA_NO_WAIT);
}

/**
 * @brief Initialize DTLS retransmission, start the dtls_task here for retransmission; if it already exists, notify the task of the new DTLS connection establishment.
 *
 * @param[in] qcm_ssl_connect_data_t * argv
 *          - Pointer to the corresponding SSL initialization structure
 */
static void mbedtls_dtls_retransmit_init(qcm_ssl_connect_data_t *connssl)
{
    mbedtls_dtls_task_event_t msg_info = {0};

    if (g_dtls_retransmission_lock == QOSA_NULL)
    {
        qosa_mutex_create(&g_dtls_retransmission_lock);
    }

    qosa_mutex_lock(g_dtls_retransmission_lock, QOSA_WAIT_FOREVER);
    if (g_dtls_retransmission_msgq != QOSA_NULL && g_dtls_retransmission_task != QOSA_NULL)
    {
        msg_info.event_id = MBEDTLS_DTLS_CREATE;
        msg_info.connssl = connssl;

        qosa_msgq_release(g_dtls_retransmission_msgq, sizeof(mbedtls_dtls_task_event_t), (qosa_uint8_t *)&msg_info, QOSA_NO_WAIT);
    }
    else
    {
        if (g_dtls_retransmission_msgq == QOSA_NULL)
        {
            qosa_msgq_create(&g_dtls_retransmission_msgq, sizeof(mbedtls_dtls_task_event_t), 10);
        }

        if (g_dtls_retransmission_task == QOSA_NULL)
        {
            qosa_task_create(&g_dtls_retransmission_task, 2048, QOSA_PRIORITY_NORMAL, "dtls_task", mbedtls_dtls_retransmission_handler, QOSA_NULL);
        }
    }
    qosa_timer_create(&connssl->dtls_retransmission_timer, mbedtls_retransmit_timer_handle, connssl);
    qosa_mutex_unlock(g_dtls_retransmission_lock);

    return;
}

/**
 * @brief Release the DTLS retransmission timer and notify the task that this connection has been established.
 *
 * @param[in] qcm_ssl_connect_data_t * argv
 *          - Pointer to the corresponding SSL initialization structure
 */
static void mbedtls_dtls_retransmit_free(qcm_ssl_connect_data_t *connssl)
{
    mbedtls_dtls_task_event_t msg_info = {0};

    if (qosa_timer_is_running(connssl->dtls_retransmission_timer) == QOSA_TRUE)
    {
        qosa_timer_stop(connssl->dtls_retransmission_timer);
    }
    qosa_timer_delete(connssl->dtls_retransmission_timer);
    connssl->dtls_retransmission_timer = QOSA_NULL;

    msg_info.event_id = MBEDTLS_DTLS_CLOSE;
    qosa_msgq_release(g_dtls_retransmission_msgq, sizeof(mbedtls_dtls_task_event_t), (qosa_uint8_t *)&msg_info, QOSA_NO_WAIT);

    return;
}
#endif

/**
 * @brief step1 is primarily used to configure the SSL connection configuration
 *
 * @param[in] qcm_ssl_connect_data * connssl
 *          - Pointer to the corresponding SSL initialization structure
 *
 * @return qcm_vtls_result_status_e
 *       - If the execution is successful, it returns QCM_VTLS_RESULT_OK
 *       - If execution fails, return other errors
 */
static qcm_vtls_result_status_e mbed_connect_step1(qcm_ssl_connect_data_t *connssl)
{
    int         ret = -1;
    const char *hostname = connssl->hostname;
    char        errorbuf[128];
    static int  cust_id = 0;
    char        pers[20] = {0};
    cust_id = cust_id % 1000;
    qosa_snprintf(pers, 20, "unir_ssl_ctx_%d", cust_id);
    cust_id++;

    qcm_ssl_config_t             *conf = &connssl->ssl_config;
    struct mbed_ssl_backend_data *backend = (struct mbed_ssl_backend_data *)connssl->backend;

    mbedtls_entropy_init(&backend->entropy);
    mbedtls_ctr_drbg_init(&backend->ctr_drbg);

#if defined(MBEDTLS_USE_PSA_CRYPTO) || defined(MBEDTLS_SSL_PROTO_TLS1_3)
    psa_status_t status;
    status = psa_crypto_init();
    if (status != PSA_SUCCESS)
    {
        QLOGE("Failed to initialize PSA Crypto implementation: %d\n", (int)status);
        ret = MBEDTLS_ERR_SSL_HW_ACCEL_FAILED;
        return QCM_VTLS_FAILED_INIT_ERR;
    }
#endif /* MBEDTLS_USE_PSA_CRYPTO || MBEDTLS_SSL_PROTO_TLS1_3 */

    ret = mbedtls_ctr_drbg_seed(&backend->ctr_drbg, mbedtls_entropy_func, &backend->entropy, (const unsigned char *)pers, qosa_strlen(pers));
    if (ret)
    {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, errorbuf, sizeof(errorbuf));
#endif
        QLOGE("mbedtls_ctr_drbg_seed returned (-0x%X) %s", -ret, errorbuf);
        return QCM_VTLS_FAILED_INIT_ERR;
    }

    mbedtls_ssl_config_init(&backend->config);

    ret = mbedtls_ssl_config_defaults(
        &backend->config,
        MBEDTLS_SSL_IS_CLIENT,
        conf->transport == QCM_SSL_TLS_PROTOCOL ? MBEDTLS_SSL_TRANSPORT_STREAM : MBEDTLS_SSL_TRANSPORT_DATAGRAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );
    if (ret)
    {
        QLOGE("mbedTLS: ssl_config failed");
        return QCM_VTLS_SSL_CONNECT_ERR;
    }
#if MBEDTLS_VERSION_NUMBER < 0x03000000
#ifdef MBEDTLS_ARC4_C
    mbedtls_ssl_conf_arc4_support(&backend->config, MBEDTLS_SSL_ARC4_ENABLED);
#endif  // MBEDTLS_ARC4_C
#endif

    /* Load the trusted CA */
    mbedtls_x509_crt_init(&backend->cacert);
    /* Load the client certificate */
    mbedtls_x509_crt_init(&backend->clicert);
    /* Load the client private key */
    mbedtls_pk_init(&backend->pk);
#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    if (conf->mfl_support == QOSA_TRUE)
    {
        mbedtls_ssl_conf_max_frag_len(&(backend->config), conf->mfl_size);
    }
#endif /* MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */
    QLOGV("  . Loading the CA root certificate and Set ssl config ...");


#ifdef MBEDTLS_KEY_EXCHANGE_SOME_PSK_ENABLED
    if (conf->psk_enable == QCM_SSL_PSK_ENABLED)
    {
        if (conf->psk_key != QOSA_NULL && conf->psk_identity != QOSA_NULL)
        {
            int psk_ret = 0;
            qosa_uint8_t psk[MBEDTLS_PSK_MAX_LEN] = {0};
            qosa_size_t  psk_len = 0;
            char        *psk_key = QOSA_NULL;
            QLOGD(" psk %s psk_identiy %s, psk_format:%d", conf->psk_key, conf->psk_identity, conf->psk_format);
            if (conf->psk_key_len > 0)
            {
                if (conf->psk_format == 1 && conf->psk_key_len % 2 == 0 && conf->psk_key_len < 2 * MBEDTLS_PSK_MAX_LEN
                    && (qcm_utils_data_string_to_hex((const char *)conf->psk_key, conf->psk_key_len, psk)) == 0)
                {
                    QLOGD("pre-shared key HEX\n");
                    psk_len = conf->psk_key_len / 2;
                    psk_key = (char *)psk;
                }
                else
                {
                    psk_key = conf->psk_key;
                    psk_len = conf->psk_key_len;
                }
            }

            if (psk_len > 0)
            {
#if defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) 
#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED)
              psk_ret = mbedtls_ssl_conf_psk(
                    &backend->config,
                    (const qosa_uint8_t *)psk_key,
                    psk_len,
                    (qosa_uint8_t *)conf->psk_identity,
                    qosa_strlen(conf->psk_identity)
                );  
#else
             psk_ret = MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE;
#endif
#else
      
                psk_ret = mbedtls_ssl_conf_psk(
                    &backend->config,
                    (const qosa_uint8_t *)psk_key,
                    psk_len,
                    (qosa_uint8_t *)conf->psk_identity,
                    qosa_strlen(conf->psk_identity)
                );
#endif
                if (psk_ret != 0)
                {
                    QLOGD("failed mbedtls set psk %d", psk_ret);
                }
            }
        }
        else
        {
            QLOGE("psk set error");
            ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
            goto exit;
        }
    }
#endif /* MBEDTLS_KEY_EXCHANGE_SOME_PSK_ENABLED */
    if (conf->auth_mode == QCM_SSL_VERIFY_NULL)
    {
        mbedtls_ssl_conf_authmode(&(backend->config), MBEDTLS_SSL_VERIFY_NONE);
    }
    else
    {
        int    kk = 0;
        int    cacert_cnt = 0;
        char **cacert_path = QOSA_NULL;
        int    max_cacert_cnt;
        mbedtls_ssl_conf_authmode(&(backend->config), MBEDTLS_SSL_VERIFY_REQUIRED);

        if (qcm_ssl_check_is_set_cacertex() == QOSA_FALSE)
        {
            //cacertex is not empty
            cacert_path = qcm_ssl_global_cacertex_path();
            max_cacert_cnt = SSL_MAX_CA_CERT_EX_CNT;
        }
        else
        {
            //Load the user-saved file path CA root certificate file
            cacert_path = conf->ca_cert_path;
            max_cacert_cnt = SSL_MAX_CA_CERT_CNT;
        }

        while (kk < max_cacert_cnt)
        {
            if (cacert_path[kk] != QOSA_NULL)
            {
                mbed_file_crt_info_t *data = QOSA_NULL;
                QLOGD("=%s", cacert_path[kk]);
                data = mbed_read_file_crt(cacert_path[kk]);

                if (data == QOSA_NULL)
                {
                    ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
                    goto exit;
                }

                if ((ret = mbedtls_x509_crt_parse(&(backend->cacert), (const unsigned char *)data->file_data, data->file_len + 1)) != 0)
                {
                    QLOGE("x509 pasre failed,ret:(-0x%x)", -ret);
                    mbed_file_data_free(data);
                    data = QOSA_NULL;
                    ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
                    goto exit;
                }
                mbed_file_data_free(data);
                cacert_cnt++;
            }
            kk++;
        }
        //Load the buff-type certificate file configured by the user
        kk = 0;
        cacert_path = conf->ca_cert_buffer;
        while ((kk < max_cacert_cnt) && (cacert_cnt < max_cacert_cnt))
        {
            if (cacert_path[kk] != QOSA_NULL)
            {
                int ret = 0;
                QLOGD("cacert_path=%d,%s", kk, cacert_path[kk]);
                if ((ret = mbedtls_x509_crt_parse(&(backend->cacert), (const unsigned char *)cacert_path[kk], qosa_strlen(cacert_path[kk]) + 1)) != 0)
                {
                    QLOGE("x509 pasre failed,ret:(-0x%x)", -ret);
                    ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
                    goto exit;
                }
                cacert_cnt++;
            }
            kk++;
        }
        //Configure the CA certificate into the sslconfig
        QLOGD("cacert_cnt=%d", cacert_cnt);
        if (cacert_cnt > 0)
        {
            mbedtls_ssl_conf_ca_chain(&(backend->config), &(backend->cacert), QOSA_NULL);
        }
        else
        {
            ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
            goto exit;
        }

        //If mutual authentication is required, then the client certificate file and public key need to be loaded.
        if (conf->auth_mode == QCM_SSL_VERIFY_CLIENT_SERVER)
        {
            //An error is reported if both the path and buffer method are empty.
            if ((conf->own_cert_path == QOSA_NULL || conf->own_key_path == QOSA_NULL)
                && (conf->own_cert_buffer == QOSA_NULL || conf->own_key_buff == QOSA_NULL))
            {
                ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
                goto exit;
            }

            //Buffer type configuration for client certificate and file
            QLOGD("client_cert_type=%d", conf->client_cert_type);
            if (conf->client_cert_type == QCM_SSL_CLIENT_CERT_BUFFER)
            {
                int ret = 0;
                if ((ret = mbedtls_x509_crt_parse(&(backend->clicert), (const unsigned char *)conf->own_cert_buffer, qosa_strlen(conf->own_cert_buffer) + 1))
                    != 0)
                {
                    QLOGE("x509 pasre failed,ret:(-0x%x)", -ret);
                    ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
                    goto exit;
                }
#if (MBEDTLS_VERSION_NUMBER < 0x04000000 && MBEDTLS_VERSION_NUMBER >= 0x03000000)
                ret = mbedtls_pk_parse_key(
                    &backend->pk,
                    (const unsigned char *)conf->own_key_buff,
                    qosa_strlen(conf->own_key_buff) + 1,
                    (const unsigned char *)conf->own_key_pwd,
                    conf->own_key_pwd ? qosa_strlen(conf->own_key_pwd) : 0,
                    mbedtls_ctr_drbg_random,
                    &backend->ctr_drbg
                );
#else
                ret = mbedtls_pk_parse_key(
                    &backend->pk,
                    (const unsigned char *)conf->own_key_buff,
                    qosa_strlen(conf->own_key_buff) + 1,
                    (const unsigned char *)conf->own_key_pwd,
                    conf->own_key_pwd ? qosa_strlen(conf->own_key_pwd) : 0
                );
#endif
                if (ret != 0)
                {
                    QLOGE(" failed\n  !  mbedtls_pk_parse_key returned -0x%x\n\n", (unsigned int)-ret);
                    goto exit;
                }
            }
            else if (conf->client_cert_type == QCM_SSL_CLIENT_CERT_FILE)
            {
                mbed_file_crt_info_t *data = QOSA_NULL;
                QLOGD("own_cert_path=%s", conf->own_cert_path);
                data = mbed_read_file_crt(conf->own_cert_path);

                if (data == QOSA_NULL)
                {
                    ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
                    goto exit;
                }

                if ((ret = mbedtls_x509_crt_parse(&(backend->clicert), (const unsigned char *)data->file_data, data->file_len + 1)) != 0)
                {
                    QLOGE("x509 pasre failed,ret:(-0x%x)", -ret);
                    mbed_file_data_free(data);
                    data = QOSA_NULL;
                    ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
                    goto exit;
                }
                mbed_file_data_free(data);
                data = QOSA_NULL;

                QLOGD("own_key_path=%s", conf->own_key_path);
                data = mbed_read_file_crt(conf->own_key_path);
                if (data == QOSA_NULL)
                {
                    ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
                    goto exit;
                }

#if (MBEDTLS_VERSION_NUMBER < 0x04000000 && MBEDTLS_VERSION_NUMBER >= 0x03000000)
                ret = mbedtls_pk_parse_key(
                    &backend->pk,
                    (const unsigned char *)data->file_data,
                    data->file_len + 1,
                    (const unsigned char *)conf->own_key_pwd,
                    conf->own_key_pwd ? qosa_strlen(conf->own_key_pwd) : 0,
                    mbedtls_ctr_drbg_random,
                    &backend->ctr_drbg
                );
#else
                ret = mbedtls_pk_parse_key(
                    &backend->pk,
                    (const unsigned char *)data->file_data,
                    data->file_len + 1,
                    (const unsigned char *)conf->own_key_pwd,
                    conf->own_key_pwd ? qosa_strlen(conf->own_key_pwd) : 0
                );
#endif
                mbed_file_data_free(data);
                data = QOSA_NULL;
                if (ret != 0)
                {
                    ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
                    goto exit;
                }
            }
            mbedtls_ssl_conf_own_cert(&(backend->config), &(backend->clicert), &(backend->pk));
        }
    }

    QLOGD("mbedTLS: Connecting to %s:%d", hostname, connssl->port);

#ifdef MBEDTLS_DEBUG
    /* In order to make that work in mbedtls MBEDTLS_DEBUG_C must be defined. */
    QLOGD("harry set debug=%p", mbed_debug);
    mbedtls_ssl_conf_dbg(&backend->config, mbed_debug, "[MBEDTLS]");
    /* - 0 No debug
     * - 1 Error
     * - 2 State change
     * - 3 Informational
     * - 4 Verbose
     */
    QLOGD("ssl_log_debug=%d", conf->ssl_log_debug);
    mbedtls_debug_set_threshold(conf->ssl_log_debug);
#endif
    mbedtls_ssl_init(&backend->ssl);
#if defined(MBEDTLS_SSL_PROTO_TLS1_3)
    mbedtls_ssl_set_export_keys_cb(&backend->ssl, mbedtls_keylog_cb, QOSA_NULL);
#endif /* MBEDTLS_SSL_PROTO_TLS1_3 */
    /* new profile with RSA min key len = 1024 ... */
    mbedtls_ssl_conf_cert_profile(&backend->config, &mbedtls_x509_crt_profile_fr);

    //Set SSL request protocol version
    mbedtls_set_version_switch(conf, backend);

#if MBEDTLS_VERSION_NUMBER < 0x03000000
    QLOGD("ignore_invalid_certsign=%d", conf->ignore_invalid_certsign);
    // unir_mbedtls_ssl_set_ignore_cert_flag(&backend->config, conf->ignore_invalid_certsign);
#endif
#ifndef CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC
    mbedtls_ssl_conf_rng(&backend->config, mbedtls_ctr_drbg_random, &backend->ctr_drbg);
#endif /* CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC */

    //Ignore certificate errors for fixed items
    mbedtls_ssl_conf_verify(&backend->config, mbed_ssl_verify, &conf->ignore_certitem);

    mbedtls_ssl_set_bio(&backend->ssl, (void *)connssl, mbed_bio_write, mbed_bio_read, QOSA_NULL /*  rev_timeout() */);

    //Set user-configured custom algorithm
    if (conf->ciphersuites != QOSA_NULL)
    {
        int i = 0;
        for (i = 0; conf->ciphersuites[i] != 0; i++)
        {
            QLOGD("ciphersuites[%d]=%d,%x", i, conf->ciphersuites[i], conf->ciphersuites[i]);
        }
        mbedtls_ssl_conf_ciphersuites(&backend->config, conf->ciphersuites);
    }
    else
    {
        mbedtls_ssl_conf_ciphersuites(&backend->config, mbedtls_ssl_list_ciphersuites());
    }

    if (conf->alpn_name != QOSA_NULL)
    {
#if defined(MBEDTLS_SSL_ALPN)
        int alpn_ret = 0;

        backend->alpn_list[0] = conf->alpn_name;
        backend->alpn_list[1] = QOSA_NULL;
        alpn_ret = mbedtls_ssl_conf_alpn_protocols(&backend->config, backend->alpn_list);
        if (alpn_ret != 0)
        {
#ifdef MBEDTLS_ERROR_C
            mbedtls_strerror(alpn_ret, errorbuf, sizeof(errorbuf));
#endif
            QLOGE("mbedTLS: set ALPN failed ret=(-0x%X) %s, name=%s, len=%d", -alpn_ret, errorbuf, conf->alpn_name, qosa_strlen(conf->alpn_name));
            ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
            goto exit;
        }
        QLOGI("mbedTLS: ALPN configured name=%s, len=%d", conf->alpn_name, qosa_strlen(conf->alpn_name));
#else
        QLOGE("mbedTLS: ALPN configured but MBEDTLS_SSL_ALPN is disabled");
        ret = QCM_VTLS_SSL_INVALID_PARAM_ERR;
        goto exit;
#endif
    }

    if (conf->transport == QCM_SSL_DTLS_PROTOCOL)
    {
        //Set dtls mtu
#if defined(MBEDTLS_SSL_PROTO_DTLS)
        mbedtls_ssl_set_mtu(&backend->ssl, conf->dtls_mtu_size);
        mbedtls_ssl_set_datagram_packing(&backend->ssl, 0);
#endif /* MBEDTLS_SSL_PROTO_DTLS */

        //Set dtls timeout
        //The minimum value is 10s because the minimum configuration value for ssl_negotiate_timeout in qsslcfg
#if defined(MBEDTLS_TIMING_C)
        mbedtls_ssl_set_timer_cb(&backend->ssl, &backend->timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);
#ifdef MBEDTLS_SSL_PROTO_DTLS
        mbedtls_dtls_retransmit_init(connssl);
        mbedtls_ssl_conf_handshake_timeout(&backend->config, conf->ssl_negotiate_timeout * 1000, conf->ssl_negotiate_timeout * 1000);
#endif
#endif
    }

    //Configure whether to enable session renegotiation
#if defined(MBEDTLS_SSL_RENEGOTIATION)
    if (conf->renegotiation == QOSA_TRUE)
    {
        mbedtls_ssl_conf_renegotiation(&backend->config, MBEDTLS_SSL_RENEGOTIATION_ENABLED);
    }
#endif

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    mbedtls_ssl_conf_session_tickets(&backend->config, MBEDTLS_SSL_SESSION_TICKETS_DISABLED);
#endif

#if defined(MBEDTLS_SSL_PROTO_TLS1_3)
    mbedtls_ssl_conf_tls13_key_exchange_modes(&backend->config, MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL);
#endif /* MBEDTLS_SSL_PROTO_TLS1_3 */

    if (QOSA_TRUE == conf->sni_enable)
    {
        if (!hostname || mbedtls_ssl_set_hostname(&backend->ssl, hostname))
        {
            /* mbedtls_ssl_set_hostname() sets the name to use in CN/SAN checks and
            the name to set in the SNI extension. So even if curl connects to a
            host specified as an IP address, this function must be used. */
            QLOGE("Failed to set SNI");
            return QCM_VTLS_SSL_CONNECT_ERR;
        }
    }
    else
    {
        if (mbedtls_ssl_set_hostname(&backend->ssl, QOSA_NULL))
        {
            QLOGE("Failed to set default ssl->hostname");
            return QCM_VTLS_SSL_CONNECT_ERR;
        }
    }

    ret = mbedtls_ssl_setup(&backend->ssl, &backend->config);
    if (ret != 0)
    {
        QLOGE("mbedTLS: ssl_init failed ret=%d", ret);
        return QCM_VTLS_SSL_CONNECT_ERR;
    }
    if (conf->session_cache_enable == QOSA_TRUE)
    {
        qcm_vtls_result_status_e result_code = QCM_VTLS_RESULT_OK;
        void                    *old_session = QOSA_NULL;
        result_code = qcm_ssl_get_sessionid(connssl, &old_session);
        if (result_code == QCM_VTLS_RESULT_OK)
        {
            ret = mbedtls_ssl_set_session(&backend->ssl, old_session);
            if (ret)
            {
                QLOGE("mbedtls_ssl_set_session returned -0x%x", -ret);
                return QCM_VTLS_SSL_CONNECT_ERR;
            }
        }
        QLOGD("mbedTLS re-using session");
    }

    connssl->connecting_state = QCM_SSL_CONNECT_2;
    return QCM_VTLS_RESULT_OK;
exit:
    return ret;
}

/**
 * @brief Start executing SSL session handshake action
 *
 * @param[in] qcm_ssl_connect_data * connssl
 *          - Pointer to the corresponding SSL initialization structure
 *
 * @return qcm_vtls_result_status_e
 *       - If the execution is successful, it returns QCM_VTLS_RESULT_OK
 *       - If execution fails, return other errors
 */
static qcm_vtls_result_status_e mbed_connect_step2(qcm_ssl_connect_data_t *connssl)
{
    int                           ret = 0;
#if defined(MBEDTLS_TIMING_C) && defined(MBEDTLS_SSL_PROTO_DTLS)
    qosa_uint32_t                 fin_ms = 0;
#endif
#if defined(MBEDTLS_SSL_ALPN)
    const char                   *negotiated_alpn = QOSA_NULL;
#endif
    struct mbed_ssl_backend_data *backend = (struct mbed_ssl_backend_data *)connssl->backend;

    //If the IO is a blocking function, this function returns with the result immediately.
    ret = mbedtls_ssl_handshake(&backend->ssl);

    if (ret == MBEDTLS_ERR_SSL_WANT_READ)
    {
        if (connssl->ssl_config.transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM)
        {
#if defined(MBEDTLS_TIMING_C) && defined(MBEDTLS_SSL_PROTO_DTLS)
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
            fin_ms = backend->timer.MBEDTLS_PRIVATE(fin_ms);
#else
            #if defined(CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC) || defined(CONFIG_QOSA_MBEDTLS3_X_X_LIBRARY_FUNC)
            fin_ms = backend->timer.MBEDTLS_PRIVATE(fin_ms);
            #else /* CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC */
            fin_ms = backend->timer.fin_ms;
            #endif /* CONFIG_QCM_MBEDTLS_4_LIBRARY_FUNC */
#endif
            QLOGD("connecting_state %d fin_ms %d", connssl->connecting_state, fin_ms);

            if (connssl->connecting_state != QCM_SSL_CONNECT_DONE && qosa_timer_is_running(connssl->dtls_retransmission_timer) == QOSA_FALSE)
            {
                qosa_timer_start(connssl->dtls_retransmission_timer, fin_ms, 0);
            }
#endif
        }

        connssl->connecting_state = QCM_SSL_CONNECT_2_READING;
        return QCM_VTLS_RESULT_OK;
    }
    else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        connssl->connecting_state = QCM_SSL_CONNECT_2_WRITING;
        return QCM_VTLS_RESULT_OK;
    }
    else if (ret)
    {
        char errorbuf[128];
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, errorbuf, sizeof(errorbuf));
#endif
        QLOGE("ssl_handshake returned - mbedTLS: (-0x%X) %s", -ret, errorbuf);
        return QCM_VTLS_SSL_CONNECT_ERR;
    }

    QLOGD("mbedTLS: Handshake complete, cipher is %s", mbedtls_ssl_get_ciphersuite(&backend->ssl));

#if defined(MBEDTLS_SSL_ALPN)
    if (connssl->ssl_config.alpn_name != QOSA_NULL)
    {
        negotiated_alpn = mbedtls_ssl_get_alpn_protocol(&backend->ssl);
        QLOGI("mbedTLS: ALPN negotiated=%s", negotiated_alpn != QOSA_NULL ? negotiated_alpn : "none");
    }
#endif

    connssl->connecting_state = QCM_SSL_CONNECT_3;
    QLOGV("SSL connected");

#if defined(MBEDTLS_TIMING_C) && defined(MBEDTLS_SSL_PROTO_DTLS)
    if (connssl->ssl_config.transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM)
    {
        mbedtls_dtls_retransmit_free(connssl);
    }
#endif

    return QCM_VTLS_RESULT_OK;
}

/**
 * @brief Tasks to handle after SSL connection is established, primarily used to save session information for the current conversation
 *
 * @param[in] qcm_ssl_connect_data * connssl
 *          - Pointer to the corresponding SSL initialization structure
 *
 * @return qcm_vtls_result_status_e
 *       - Returns QCM_VTLS_RESULT_OK if the execution is successful
 *       - If execution fails, return other errors.
 */
static qcm_vtls_result_status_e mbed_connect_step3(qcm_ssl_connect_data_t *connssl)
{
    int                           ret = 0;
    qcm_vtls_result_status_e      result_code = QCM_VTLS_RESULT_OK;
    mbedtls_ssl_session          *our_ssl_sessionid;
    struct mbed_ssl_backend_data *backend = (struct mbed_ssl_backend_data *)connssl->backend;

    if (connssl->ssl_config.session_cache_enable == QOSA_TRUE)
    {
        our_ssl_sessionid = qosa_malloc(sizeof(mbedtls_ssl_session));
        if (!our_ssl_sessionid)
            return QCM_VTLS_SSL_MEMORY_ERR;

        mbedtls_ssl_session_init(our_ssl_sessionid);

        ret = mbedtls_ssl_get_session(&backend->ssl, our_ssl_sessionid);
        if (ret)
        {
            if (ret != MBEDTLS_ERR_SSL_ALLOC_FAILED)
                mbedtls_ssl_session_free(our_ssl_sessionid);
            qosa_free(our_ssl_sessionid);
            QLOGE("mbedtls_ssl_get_session returned -0x%x", -ret);
            return QCM_VTLS_SSL_CONNECT_ERR;
        }

        result_code = qcm_ssl_add_sessionid(connssl, our_ssl_sessionid);
        if (result_code != QCM_VTLS_RESULT_OK)
        {
            mbedtls_ssl_session_free(our_ssl_sessionid);
            qosa_free(our_ssl_sessionid);
        }
    }

    connssl->connecting_state = QCM_SSL_CONNECT_DONE;
    return QCM_VTLS_RESULT_OK;
}

/**
 * @brief Start executing SSL connection handshake action
 *  1. If configured as blocking, it will exit only when the SSL handshake succeeds or the SSL connection fails.
 *  2. If in non-blocking mode, SSL will return QCM_VTLS_SSL_READ_WRITE_EAGAIN.
 *
 * @param[in] qcm_ssl_connect_data * connssl
 *          - Pointer to the corresponding SSL initialization structure
 *
 * @param[in] qosa_bool_t nonblocking
 *          - Configure whether to enable non-blocking IO mode for SSL handshake connection
 *
 * @param[in] qosa_bool_t * done
 *          - Whether the SSL handshake session is completed
 *
 * @return qcm_vtls_result_status_e
 *       - Returns QCM_VTLS_RESULT_OK if the execution is successful
 *       - If execution fails, return other errors
 */
static qcm_vtls_result_status_e mbed_connect_common(qcm_ssl_connect_data_t *connssl, qosa_bool_t nonblocking, qosa_bool_t *done)
{
    int               retcode = 0;
    qosa_time_t       timeout_ms = 0;
    qcm_ssl_config_t *conf = &connssl->ssl_config;

    /* check if the connection has already been established */
    if (QCM_SSL_CONNECTION_COMPLETE == connssl->state)
    {
        *done = QOSA_TRUE;
        return QCM_VTLS_RESULT_OK;
    }

    if (connssl->cur_connect_time == 0)
    {
        connssl->cur_connect_time = qosa_get_system_time_seconds();
    }

    //Start SSL connection initialization action
    if (QCM_SSL_CONNECT_1 == connssl->connecting_state)
    {
        retcode = mbed_connect_step1(connssl);
        if (retcode)
            return retcode;
    }

    //Start SSL handshake process. If using blocking I/O, step2 exits when SSL returns the result.
    //If it is non-blocking IO, then nonblocking == 0, so a while loop should be used to repeatedly call the function for data reading, while using select to monitor the socket event status.
    //Meanwhile, the execution interval of the select function can calculate the time difference of the current SSL connection, and if it times out, it should exit.
    while (QCM_SSL_CONNECT_2 == connssl->connecting_state || QCM_SSL_CONNECT_2_READING == connssl->connecting_state
           || QCM_SSL_CONNECT_2_WRITING == connssl->connecting_state)
    {
        //Calculate the current time difference
        timeout_ms = (qosa_get_system_time_seconds() - connssl->cur_connect_time);

        //Check if the time has expired
        timeout_ms = conf->ssl_negotiate_timeout - timeout_ms;
        QLOGD("...timeout_ms=%d,%d", timeout_ms, conf->ssl_negotiate_timeout);
        if (timeout_ms <= 0)
        {
            QLOGE("SSL connection timeout");
            return QCM_VTLS_SSL_OPERATION_TIMEDOUT;
        }

        QLOGD("connssl->connecting_state=%d nonblocking=%d", connssl->connecting_state, nonblocking);
        /* if ssl is expecting something, check if it's available. */
        if ((connssl->connecting_state == QCM_SSL_CONNECT_2_READING || connssl->connecting_state == QCM_SSL_CONNECT_2_WRITING) && nonblocking == QOSA_FALSE)
        {
            int         what = 0;
            qosa_bool_t writefd = QCM_SSL_CONNECT_2_WRITING == connssl->connecting_state ? QOSA_TRUE : QOSA_FALSE;
            qosa_bool_t readfd = QCM_SSL_CONNECT_2_READING == connssl->connecting_state ? QOSA_TRUE : QOSA_FALSE;
            QLOGE("writefd=%d readfd=%d", writefd, readfd);
            //select waits for IO events to return
            what = conf->io_select(connssl, readfd, writefd, nonblocking ? 0 : timeout_ms);
            if (what < 0)
            {
                /* fatal error */
                QLOGE("select/poll on SSL socket, error");
                return QCM_VTLS_SSL_CONNECT_ERR;
            }
            else if (0 == what)
            {
                if (nonblocking)
                {
                    *done = QOSA_FALSE;
                    return QCM_VTLS_RESULT_OK;
                }
                else
                {
                    /* timeout */
                    QLOGE("SSL connection timeout");
                    return QCM_VTLS_SSL_OPERATION_TIMEDOUT;
                }
            }
            /* socket is readable or writable */
        }

#if defined(MBEDTLS_TIMING_C) && defined(MBEDTLS_SSL_PROTO_DTLS)
        if (qosa_timer_is_running(connssl->dtls_retransmission_timer) == QOSA_TRUE)
        {
            QLOGD("dtls_retransmission_timer  stop");
            qosa_timer_stop(connssl->dtls_retransmission_timer);
        }
#endif

        retcode = mbed_connect_step2(connssl);
        if (retcode
            || (nonblocking
                && (QCM_SSL_CONNECT_2 == connssl->connecting_state || QCM_SSL_CONNECT_2_READING == connssl->connecting_state
                    || QCM_SSL_CONNECT_2_WRITING == connssl->connecting_state)))
            return retcode;

    } /* repeat step2 until all transactions are done. */

    //If step2 is completed, it indicates that the SSL handshake has been completed, and the task processing after a successful connection begins.
    if (QCM_SSL_CONNECT_3 == connssl->connecting_state)
    {
        retcode = mbed_connect_step3(connssl);
        if (retcode)
            return retcode;
    }

    //Set the SSL connection status to connected state
    if (QCM_SSL_CONNECT_DONE == connssl->connecting_state)
    {
        connssl->cur_connect_time = 0;  //Reset handshake time
        connssl->state = QCM_SSL_CONNECTION_COMPLETE;
        *done = QOSA_TRUE;
    }
    else
        *done = QOSA_FALSE;

    /* Reset our connect state machine */
    connssl->connecting_state = QCM_SSL_CONNECT_1;

    return QCM_VTLS_RESULT_OK;
}

/**
 * @brief mbedtls starts global initialization
 */
static int qcm_mbedtls_init(void)
{
    return QOSA_TRUE;
}

/**
 * @brief mbedtls global resource allocation and deallocation
 */
static void qcm_mbedtls_cleanup(void)
{
}

/**
 * @brief mbedtls version information, returns the actual length of the string
 */
static qosa_size_t qcm_mbedtls_version(char *buffer, qosa_size_t size)
{
    return qosa_snprintf(buffer, size, "mbedTLS/%s", MBEDTLS_VERSION_STRING);
}

/**
 * @brief mbedtls provides a non-blocking SSL connect interface to Vtls
 */
static int qcm_mbedtls_connect_nonblocking(qcm_ssl_connect_data_t *connssl, qosa_bool_t *done)
{
    return mbed_connect_common(connssl, QOSA_TRUE, done);
}

/**
 * @brief mbedtls provides a blocking SSL connect interface to Vtls
 */
static int qcm_mbedtls_connect(qcm_ssl_connect_data_t *connssl)
{
    int         retcode;
    qosa_bool_t done = QOSA_FALSE;

    retcode = mbed_connect_common(connssl, QOSA_FALSE, &done);
    if (retcode)
        return retcode;

    QOSA_UNUSED(done);

    return QCM_VTLS_RESULT_OK;
}

/**
 * @brief Get the length of unread data in the mbedtls internal buffer
 *
 * @param[in] const qcm_ssl_connect_data * connssl
 *          - SSL handle information to operate on
 *
 * @return qosa_bool_t
 *       - If it is OSA_TRUE, it indicates that there is data in the cache.
 *       - If it is OSA_FALSE, it indicates that there is no data in the buffer.
 */
static qosa_bool_t qcm_mbedtls_data_pending(qcm_ssl_connect_data_t *connssl)
{
    struct mbed_ssl_backend_data *backend = (struct mbed_ssl_backend_data *)connssl->backend;

    return mbedtls_ssl_get_bytes_avail(&backend->ssl) != 0;
}

/**
 * @brief mbedtls close resource shutdown
 *
 * @param[in] const qcm_ssl_connect_data * connssl
 *          - SSL handle information to operate on
 *
 * @return void
 */
static void qcm_mbedtls_close(qcm_ssl_connect_data_t *connssl)
{
    struct mbed_ssl_backend_data *backend = (struct mbed_ssl_backend_data *)connssl->backend;

#if defined(MBEDTLS_TIMING_C) && defined(MBEDTLS_SSL_PROTO_DTLS)
    mbedtls_dtls_retransmit_free(connssl);
#endif
    mbedtls_pk_free(&backend->pk);
    mbedtls_x509_crt_free(&backend->clicert);
    mbedtls_x509_crt_free(&backend->cacert);
    mbedtls_ssl_config_free(&backend->config);
    mbedtls_ssl_free(&backend->ssl);
    mbedtls_ctr_drbg_free(&backend->ctr_drbg);
    mbedtls_entropy_free(&backend->entropy);
}

/**
 * @brief Data reading interface after SSL connection is established
 *
 * @param[in] qcm_ssl_connect_data * connssl
 *          - Corresponding SSL internal link pointer
 *
 * @param[in] char * buf
 *          - read reads the data content pointer
 *
 * @param[in] qosa_size_t buffersize
 *          - Corresponding buffer space size
 *
 * @param[out] qcm_vtls_result_status_e * curlcode
 *          - Corresponding API interface execution status
 *
 * @return qosa_size_t
 *       - Returns the actual number of bytes read
 */
static qosa_size_t qcm_mbed_recv(qcm_ssl_connect_data_t *connssl, char *buf, qosa_size_t buffersize, qcm_vtls_result_status_e *curlcode)
{
    int                           ret = 0;
    struct mbed_ssl_backend_data *backend = (struct mbed_ssl_backend_data *)connssl->backend;
    ret = mbedtls_ssl_read(&backend->ssl, (unsigned char *)buf, buffersize);

    if (ret <= 0)
    {
        QLOGD("ret=%d %d", ret, MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY);
        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
        {
            *curlcode = QCM_VTLS_SSL_PEER_CLOSE_NOTIFY_ERR;
            return 0;
        }
        // ret == MBEDTLS_ERR_SSL_CONN_EOF -0x7280
        // EOF not error
        else if (ret == 0)
        {
            return 0;
        }

        *curlcode = (ret == MBEDTLS_ERR_SSL_WANT_READ) ? QCM_VTLS_SSL_READ_WRITE_EAGAIN : QCM_VTLS_SSL_CONNECT_ERR;
        return -1;
    }

    return ret;
}

/**
 * @brief Data transmission interface after SSL connection is established
 *
 * @param[in] qcm_ssl_connect_data * connssl
 *          - Corresponding SSL internal link pointer
 *
 * @param[in] char * buf
 *          - write data content pointer
 *
 * @param[in] qosa_size_t buffersize
 *          - The actual data length inside the corresponding buffer
 *
 * @param[in] qcm_vtls_result_status_e * curlcode
 *          - Corresponding API interface execution status
 *
 * @return qosa_size_t
 *       - Returns the actual number of bytes read
 */
static qosa_size_t qcm_mbed_send(qcm_ssl_connect_data_t *connssl, const void *mem, qosa_size_t len, qcm_vtls_result_status_e *curlcode)
{
    int                           ret = 0;
    struct mbed_ssl_backend_data *backend = (struct mbed_ssl_backend_data *)connssl->backend;
    ret = mbedtls_ssl_write(&backend->ssl, (unsigned char *)mem, len);
    QLOGD("ret = %d len=%d mem=%s", ret, len, mem);
    if (ret < 0)
    {
        *curlcode = (ret == MBEDTLS_ERR_SSL_WANT_WRITE) ? QCM_VTLS_SSL_READ_WRITE_EAGAIN : QCM_VTLS_SSL_CONNECT_ERR;
        ret = -1;
    }

    return ret;
}
/**
 * @brief Check if the corresponding ciphersuite algorithm is valid
 *
 * @param[in] int cs_id
 *          - ID of the corresponding cipher suite
 *
 * @return qosa_bool_t
 *       - Returns OSA_TRUE if successful
 *       - Returns OSA_FALSE on failure
 */
static qosa_bool_t qcm_mbed_check_ciphersuit(int cs_id)
{
    QLOGD("cs_id: 0x%X", cs_id);
    if (mbedtls_ssl_ciphersuite_from_id(cs_id) == QOSA_NULL)
    {
        QLOGE("invalid ciphersuite");
        return QOSA_FALSE;
    }
    else
    {
        QLOGD("valid");
        return QOSA_TRUE;
    }
}
/**
 * @brief Get the system-supported ciphersuite algorithms
 *
 * @return const int *
 *       - Returns a pointer to the array of ciphersuit algorithms supported by the system
 */
static const int *qcm_mbed_get_ciphersuit(void)
{
    const int *support_list;

    support_list = mbedtls_ssl_list_ciphersuites();
    QLOGD("support ciphersuite list = %p", support_list);

    return support_list;
}

/**
 * @brief Delete the session ID.
 *
 * @param[in] void *sessionid
 *          - Stored session ID information.
 */
static void qcm_mbed_sessionid_free(void *sessionid)
{
    if (sessionid == QOSA_NULL)
    {
        return;
    }

    mbedtls_ssl_session_free((mbedtls_ssl_session *)sessionid);
}

/**
 * @brief Register the mbedtls API into VTLS for use
 */
const struct qcm_ssl_adapter qcm_ssl_mbedtls = {
    {QCM_SSL_BACKEND_MBEDTLS, "mbedtls"}, /* info */
    sizeof(struct mbed_ssl_backend_data),
    qcm_mbedtls_init,                     /* init */
    qcm_mbedtls_cleanup,                  /* cleanup */
    qcm_mbedtls_version,                  /* version */
    qcm_mbedtls_data_pending,             /* data_pending */
    qcm_mbedtls_connect,                  /* connect */
    qcm_mbedtls_connect_nonblocking,      /* connect_nonblocking */
    qcm_mbedtls_close,                    /* close_one */
    qcm_mbed_recv,
    qcm_mbed_send,
    qcm_mbed_check_ciphersuit,
    qcm_mbed_get_ciphersuit,
    qcm_mbed_sessionid_free,
};
