/** @file         qurl_platform_tls_port.c
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-11-23 15:40:44
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "qosa_errno.h"

#include "qurl_config.h"
#include "qurl_code.h"
#include "qurl_def_in.h"
#include "qurl_log.h"
#include "qurl_tls_def.h"
#include "qurl_select.h"

#include "qurl_platform_port.h"
#if QURL_CFG_ENABLE_TLS
#include "qcm_vtls.h"
#endif /* QURL_CFG_ENABLE_TLS */

#if QURL_CFG_ENABLE_TLS

/*Internal Auxiliary*/
/** Network IO Assistant*/
static int unirtos_bio_read(void *arg, unsigned char *buf, qosa_size_t len)
{
    int                     ret = 0;
    int                     err = 0;
    qcm_ssl_connect_data_t *ssl_ctx = (qcm_ssl_connect_data_t *)arg;
    qurl_tls_ctrl_t        *tls_ctrl_ptr = (qurl_tls_ctrl_t *)ssl_ctx->ssl_config.socket_fd;

    ret = qurl_socket_read(tls_ctrl_ptr->sock_fd, buf, len);

    if (ret < 0)
    {
        err = qurl_socket_get_errno(tls_ctrl_ptr->sock_fd);
        if ((err == EINPROGRESS) || (err == EWOULDBLOCK))
        {
            ret = QCM_VTLS_IO_ERR_SSL_WANT_READ;
        }
        else
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
    }

    return ret;
}

static int unirtos_bio_write(void *arg, const unsigned char *buf, qosa_size_t len)
{
    int                     ret = 0;
    int                     err = 0;
    qcm_ssl_connect_data_t *ssl_ctx = (qcm_ssl_connect_data_t *)arg;
    qurl_tls_ctrl_t        *tls_ctrl_ptr = (qurl_tls_ctrl_t *)ssl_ctx->ssl_config.socket_fd;

    ret = qurl_socket_write(tls_ctrl_ptr->sock_fd, buf, len);

    if (ret < 0)
    {
        err = qurl_socket_get_errno(tls_ctrl_ptr->sock_fd);
        if ((err == EINPROGRESS) || (err == EWOULDBLOCK))
        {
            ret = QCM_VTLS_IO_ERR_SSL_WANT_WRITE;
        }
        else
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
    }

    return ret;
}

static int unirtos_bio_select(void *arg, qosa_bool_t read_flag, qosa_bool_t write_flag, qosa_uint32_t timeout)
{
    int                     ret = 0;
    fd_set                  fds_read = {0};
    fd_set                  fds_write = {0};
    fd_set                  fds_err = {0};
    qurl_time_t             timeout_wait = 0;
    qurl_socket_t           max_fd = (qurl_socket_t)-1;
    qcm_ssl_connect_data_t *ssl_ctx = (qcm_ssl_connect_data_t *)arg;
    qurl_tls_ctrl_t        *tls_ctrl_ptr = (qurl_tls_ctrl_t *)ssl_ctx->ssl_config.socket_fd;

    FD_ZERO(&fds_read);
    FD_ZERO(&fds_write);
    FD_ZERO(&fds_err);

    if (read_flag && tls_ctrl_ptr->sock_fd >= 0)
    {
        FD_SET(tls_ctrl_ptr->sock_fd, &fds_read);
        FD_SET(tls_ctrl_ptr->sock_fd, &fds_err);
    }
    if (write_flag && tls_ctrl_ptr->sock_fd >= 0)
    {
        FD_SET(tls_ctrl_ptr->sock_fd, &fds_write);
        FD_SET(tls_ctrl_ptr->sock_fd, &fds_err);
    }
    max_fd = tls_ctrl_ptr->sock_fd;

    timeout_wait = (qurl_time_t)(timeout * 1000);
    ret = qurl_select(max_fd, &fds_read, &fds_write, &fds_err, timeout_wait);
    if (ret < 0)
    {
        qurl_logd("negotiate err:[-0x%d]\r\n", -ret);
    }
    else if (ret == 0)
    {
        qurl_logd("negotiate timeout\r\n");
    }

    return ret;
}

/** Configuration Conversion Between QURL and UNIRTOS*/
/**
 * Clear the tls_cfg of unirtos
 */
static void unirtos_tls_cfg_free(qcm_ssl_config_t *qcm_ssl_cfg_ptr)
{
    unsigned int i = 0;

    if (qcm_ssl_cfg_ptr == QOSA_NULL)
    {
        return;
    }

    if (qcm_ssl_cfg_ptr->ciphersuites != QOSA_NULL)
    {
        qurl_free(qcm_ssl_cfg_ptr->ciphersuites);
        qcm_ssl_cfg_ptr->ciphersuites = QOSA_NULL;
    }

    for (i = 0; i < SSL_MAX_CA_CERT_CNT; i++)
    {
        if (qcm_ssl_cfg_ptr->ca_cert_path[i] != QOSA_NULL)
        {
            qurl_free(qcm_ssl_cfg_ptr->ca_cert_path[i]);
            qcm_ssl_cfg_ptr->ca_cert_path[i] = QOSA_NULL;
        }
    }

    if (qcm_ssl_cfg_ptr->own_cert_path != QOSA_NULL)
    {
        qurl_free(qcm_ssl_cfg_ptr->own_cert_path);
        qcm_ssl_cfg_ptr->own_cert_path = QOSA_NULL;
    }

    if (qcm_ssl_cfg_ptr->own_key_path != QOSA_NULL)
    {
        qurl_free(qcm_ssl_cfg_ptr->own_key_path);
        qcm_ssl_cfg_ptr->own_key_path = QOSA_NULL;
    }

    if (qcm_ssl_cfg_ptr->own_key_pwd != QOSA_NULL)
    {
        qurl_free(qcm_ssl_cfg_ptr->own_key_pwd);
        qcm_ssl_cfg_ptr->own_key_pwd = QOSA_NULL;
    }

    for (i = 0; i < SSL_MAX_CA_CERT_CNT; i++)
    {
        if (qcm_ssl_cfg_ptr->ca_cert_buffer[i] != QOSA_NULL)
        {
            qurl_free(qcm_ssl_cfg_ptr->ca_cert_buffer[i]);
            qcm_ssl_cfg_ptr->ca_cert_buffer[i] = QOSA_NULL;
        }
    }

    if (qcm_ssl_cfg_ptr->own_cert_buffer != QOSA_NULL)
    {
        qurl_free(qcm_ssl_cfg_ptr->own_cert_buffer);
        qcm_ssl_cfg_ptr->own_cert_buffer = QOSA_NULL;
    }

    if (qcm_ssl_cfg_ptr->own_key_buff != QOSA_NULL)
    {
        qurl_free(qcm_ssl_cfg_ptr->own_key_buff);
        qcm_ssl_cfg_ptr->own_key_buff = QOSA_NULL;
    }

    if (qcm_ssl_cfg_ptr->psk_identity != QOSA_NULL)
    {
        qurl_free(qcm_ssl_cfg_ptr->psk_identity);
        qcm_ssl_cfg_ptr->psk_identity = QOSA_NULL;
    }

    if (qcm_ssl_cfg_ptr->psk_key != QOSA_NULL)
    {
        qurl_free(qcm_ssl_cfg_ptr->psk_key);
        qcm_ssl_cfg_ptr->psk_key = QOSA_NULL;
    }

    if (qcm_ssl_cfg_ptr->alpn_name != QOSA_NULL)
    {
        qurl_free(qcm_ssl_cfg_ptr->alpn_name);
        qcm_ssl_cfg_ptr->alpn_name = QOSA_NULL;
    }
}

/**
 *  1. Convert to independent copy, which means copying the content pointed to by the pointer.
 *  2. The target itself is inherently pure, meaning it currently does not actively detect and release the memory it occupies (this can be supplemented in subsequent development if needed).
 */
static qurl_ecode_t tls_cfg_qurl_to_unirtos(qcm_ssl_config_t *qcm_ssl_cfg_ptr, qurl_tls_cfg_t *tls_cfg_ptr)
{
    qurl_ecode_t       ret = QURL_OK;
    unsigned int       i = 0;
    unsigned int       j = 0;
    unsigned int       len = 0;
    qurl_slist_ctrl_t *node = QOSA_NULL;

    /*Protocol Version*/
    /** Refer to qurl_tls_version_e and ssl_version_type_e*/
    switch (tls_cfg_ptr->version)
    {
        case QURL_TLS_VERSION_SSL3_0: {
            qcm_ssl_cfg_ptr->ssl_version = QCM_SSL_VERSION_0;
            break;
        }
        case QURL_TLS_VERSION_TLS1_0: {
            qcm_ssl_cfg_ptr->ssl_version = QCM_SSL_VERSION_1;
            break;
        }
        case QURL_TLS_VERSION_TLS1_1: {
            qcm_ssl_cfg_ptr->ssl_version = QCM_SSL_VERSION_2;
            break;
        }
        case QURL_TLS_VERSION_DEFAULT:
        case QURL_TLS_VERSION_TLS1_2: {
            qcm_ssl_cfg_ptr->ssl_version = QCM_SSL_VERSION_3;
            break;
        }
        case QURL_TLS_VERSION_TLS1_3: {
            qcm_ssl_cfg_ptr->ssl_version = QCM_SSL_VERSION_5;
            break;
        }
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
        case QURL_TLS_VERSION_ALL: {
            qcm_ssl_cfg_ptr->ssl_version = QCM_SSL_VERSION_ALL;
            break;
        }
#endif
        default:
            return QURL_ECODE_PARAM_INVALID;
    }

    /*Transport Layer Type*/
    /** [20231125] Currently, the protocols supported by qurl are all based on TCP; future development of new protocols will require DTLS support.*/
    qcm_ssl_cfg_ptr->transport = QCM_SSL_TLS_PROTOCOL;

    /*Supported encryption suites*/
    if (tls_cfg_ptr->ciphersuites == QOSA_NULL)
    {
        i = 0;
    }
    else
    {
        for (i = 0; i < 255; i++) /*[lzm][note]Currently supports the maximum number of cipher suites*/
        {
            if (tls_cfg_ptr->ciphersuites[i] == 0)
            {
                break;
            }
        }
    }
    if (i > 0)
    {
        qcm_ssl_cfg_ptr->ciphersuites = qurl_malloc((i + 1) * sizeof(int));
        if (qcm_ssl_cfg_ptr->ciphersuites == QOSA_NULL)
        {
            ret = QURL_ECODE_NO_MEMORY;
            goto exit;
        }
        qcm_ssl_cfg_ptr->ciphersuites[i] = 0;
        for (; j < i; j++)
        {
            qcm_ssl_cfg_ptr->ciphersuites[j] = tls_cfg_ptr->ciphersuites[j];
        }
    }

    /*Authentication Method*/
    /** qurl_tls_verify_e and ssl_authmode_e are currently consistent and can be directly assigned.*/
    qcm_ssl_cfg_ptr->auth_mode = (qcm_ssl_authmode_e)tls_cfg_ptr->verify;

    /*SNI Extension*/
    qcm_ssl_cfg_ptr->sni_enable = (qosa_bool_t)tls_cfg_ptr->bits.sni_enable;

    /*CA certificate path*/
    for (i = 0, node = (qurl_slist_ctrl_t *)tls_cfg_ptr->ca_cert_path_slist; node && (i < SSL_MAX_CA_CERT_CNT); node = node->next, i++)
    {
        len = qurl_strlen((char *)node->data);
        if (len > 0)
        {
            qcm_ssl_cfg_ptr->ca_cert_path[i] = qurl_malloc(len + 1);
            if (qcm_ssl_cfg_ptr->ca_cert_path[i] == QOSA_NULL)
            {
                ret = QURL_ECODE_NO_MEMORY;
                goto exit;
            }
            qcm_ssl_cfg_ptr->ca_cert_path[i][len] = 0x00;
            qurl_memcpy(qcm_ssl_cfg_ptr->ca_cert_path[i], (char *)node->data, len);
        }
    }

    /*Client digital certificate path*/
    if (tls_cfg_ptr->own_cert_path_ptr != QOSA_NULL)
    {
        len = qurl_strlen(tls_cfg_ptr->own_cert_path_ptr);
        if (len > 0)
        {
            qcm_ssl_cfg_ptr->own_cert_path = qurl_malloc(len + 1);
            if (qcm_ssl_cfg_ptr->own_cert_path == QOSA_NULL)
            {
                ret = QURL_ECODE_NO_MEMORY;
                goto exit;
            }
            qcm_ssl_cfg_ptr->own_cert_path[len] = 0x00;
            qurl_memcpy(qcm_ssl_cfg_ptr->own_cert_path, tls_cfg_ptr->own_cert_path_ptr, len);
        }
    }

    /*Client private key path*/
    if (tls_cfg_ptr->own_key_path_ptr != QOSA_NULL)
    {
        len = qurl_strlen(tls_cfg_ptr->own_key_path_ptr);
        if (len > 0)
        {
            qcm_ssl_cfg_ptr->own_key_path = qurl_malloc(len + 1);
            if (qcm_ssl_cfg_ptr->own_key_path == QOSA_NULL)
            {
                ret = QURL_ECODE_NO_MEMORY;
                goto exit;
            }
            qcm_ssl_cfg_ptr->own_key_path[len] = 0x00;
            qurl_memcpy(qcm_ssl_cfg_ptr->own_key_path, tls_cfg_ptr->own_key_path_ptr, len);
        }
    }

    /*Client private key password string*/
    if (tls_cfg_ptr->own_key_pwd_ptr != QOSA_NULL)
    {
        len = qurl_strlen(tls_cfg_ptr->own_key_pwd_ptr);
        if (len > 0)
        {
            qcm_ssl_cfg_ptr->own_key_pwd = qurl_malloc(len + 1);
            if (qcm_ssl_cfg_ptr->own_key_pwd == QOSA_NULL)
            {
                ret = QURL_ECODE_NO_MEMORY;
                goto exit;
            }
            qcm_ssl_cfg_ptr->own_key_pwd[len] = 0x00;
            qurl_memcpy(qcm_ssl_cfg_ptr->own_key_pwd, tls_cfg_ptr->own_key_pwd_ptr, len);
        }
    }

    /*[lzm][todo]Skip buffer-form digital certificates and public/private keys for now, to be supplemented in subsequent development*/

    /*Handshake timeout duration*/
    qcm_ssl_cfg_ptr->ssl_negotiate_timeout = tls_cfg_ptr->negotiate_timeout;

    /*Ignore certain errors*/
    qcm_ssl_cfg_ptr->ignore_invalid_certsign = tls_cfg_ptr->ignore_invalid_certsign;
    qcm_ssl_cfg_ptr->ignore_multi_certchain_verify = tls_cfg_ptr->ignore_multi_certchain_verify;
    qcm_ssl_cfg_ptr->ignore_certitem = tls_cfg_ptr->ignore_certitem;

    /* [lzm][todo]DTLS */

    /* session */
    if (tls_cfg_ptr->bits.session_match || tls_cfg_ptr->bits.session_share)
    {
        qcm_ssl_cfg_ptr->session_cache_enable = QOSA_TRUE;
    }

    /* IO */
    qcm_ssl_cfg_ptr->io_read = unirtos_bio_read;
    qcm_ssl_cfg_ptr->io_write = unirtos_bio_write;
    qcm_ssl_cfg_ptr->io_select = unirtos_bio_select;

    /*Enable mbedtls debug logs*/
    qcm_ssl_cfg_ptr->ssl_log_debug = tls_cfg_ptr->ssl_log_debug;

    qcm_ssl_cfg_ptr->mfl_support = tls_cfg_ptr->mfl_support;
    qcm_ssl_cfg_ptr->mfl_size = tls_cfg_ptr->mfl_size;

    qcm_ssl_cfg_ptr->renegotiation = tls_cfg_ptr->bits.renegotiation;
    qcm_ssl_cfg_ptr->close_time_mode = tls_cfg_ptr->bits.close_time_mode;

    qcm_ssl_cfg_ptr->psk_enable = tls_cfg_ptr->psk_enable;
    qcm_ssl_cfg_ptr->psk_key_len = tls_cfg_ptr->psk_key_len;
    qcm_ssl_cfg_ptr->psk_format = tls_cfg_ptr->psk_format;
    if (tls_cfg_ptr->alpn_name != QOSA_NULL)
    {
        len = qurl_strlen(tls_cfg_ptr->alpn_name);
        if (len > 0)
        {
            qcm_ssl_cfg_ptr->alpn_name = qurl_malloc(len + 1);
            if (qcm_ssl_cfg_ptr->alpn_name == QOSA_NULL)
            {
                ret = QURL_ECODE_NO_MEMORY;
                goto exit;
            }
            qcm_ssl_cfg_ptr->alpn_name[len] = 0x00;
            qurl_memcpy(qcm_ssl_cfg_ptr->alpn_name, tls_cfg_ptr->alpn_name, len);
        }
    }
    if(tls_cfg_ptr->psk_identity != QOSA_NULL)
    {
        len = qurl_strlen(tls_cfg_ptr->psk_identity);
        if(len > 0)
        {
            qcm_ssl_cfg_ptr->psk_identity = qurl_malloc(len + 1);
            if(qcm_ssl_cfg_ptr->psk_identity == QOSA_NULL)
            {
                ret = QURL_ECODE_NO_MEMORY;
                goto exit;
            }
            qcm_ssl_cfg_ptr->psk_identity[len] = 0x00;
            qurl_memcpy(qcm_ssl_cfg_ptr->psk_identity, tls_cfg_ptr->psk_identity, len);
        }
    }
    if (tls_cfg_ptr->psk_key != QOSA_NULL)
    {
        len = qurl_strlen(tls_cfg_ptr->psk_key);
        if (len > 0)
        {
            qcm_ssl_cfg_ptr->psk_key = qurl_malloc(len + 1);
            if (qcm_ssl_cfg_ptr->psk_key == QOSA_NULL)
            {
                ret = QURL_ECODE_NO_MEMORY;
                goto exit;
            }
            qcm_ssl_cfg_ptr->psk_key[len] = 0x00;
            qurl_memcpy(qcm_ssl_cfg_ptr->psk_key, tls_cfg_ptr->psk_key, len);
        }
    }
    return QURL_OK;

exit:
    unirtos_tls_cfg_free(qcm_ssl_cfg_ptr);
    return ret;
}

/* API */
qurl_ecode_t qurl_tls_init(void)
{
    int ret = 0;

    ret = qcm_ssl_init();

    return ret == QOSA_TRUE ? QURL_OK : QURL_ECODE_TLS_INIT_FAILED;
}

qurl_ecode_t qurl_tls_deinit(void)
{
    return QURL_OK;
}

long qurl_tls_version(char *buf, long size)
{
    return (long)qcm_ssl_version(buf, (qosa_size_t)size);
}

/**
 * @brief Create a new TLS service
 * @note It will create an independent copy of the tls_cfg, so do not simply copy the pointer; instead, copy the contents inside the pointer.
 *  Since the cfg in qcm_ssl_new creates a copy, cfg needs to establish a new independent instance, maintained by tls, and this tls is maintained by conn, thereby enabling support for decoupling conn from core, which means supporting conn cache.
 *
 * @param tls_cfg_ptr
 * @return qurl_tls_ctrl_t*
 */
qurl_tls_ctrl_t *qurl_tls_new(qurl_tls_cfg_t *tls_cfg_ptr)
{
    qurl_tls_ctrl_t  *tls_ctrl_ptr = QOSA_NULL;
    qcm_ssl_config_t *qcm_ssl_cfg_ptr = QOSA_NULL;

    tls_ctrl_ptr = (qurl_tls_ctrl_t *)qurl_malloc(sizeof(qurl_tls_ctrl_t));
    if (tls_ctrl_ptr == QOSA_NULL)
    {
        return tls_ctrl_ptr;
    }
    qosa_memset(tls_ctrl_ptr, 0x00, sizeof(qurl_tls_ctrl_t));
    qcm_ssl_cfg_ptr = (qcm_ssl_config_t *)qurl_malloc(sizeof(qcm_ssl_config_t));
    if (qcm_ssl_cfg_ptr == QOSA_NULL)
    {
        qurl_free(tls_ctrl_ptr);
        tls_ctrl_ptr = QOSA_NULL;
        return tls_ctrl_ptr;
    }
    qosa_memset(qcm_ssl_cfg_ptr, 0x00, sizeof(qcm_ssl_config_t));

    /*QURL and OSA Configuration Conversion*/
    tls_cfg_qurl_to_unirtos(qcm_ssl_cfg_ptr, tls_cfg_ptr);
    qcm_ssl_cfg_ptr->socket_fd = (qosa_ptr)tls_ctrl_ptr; /*Here we do not pass sock_fd, we pass the control block address of tls_ctrl*/

    tls_ctrl_ptr->tls_cfg_ptr = qcm_ssl_cfg_ptr;

    tls_ctrl_ptr->tls_ptr = (void *)qcm_ssl_new(qcm_ssl_cfg_ptr);
    if (tls_ctrl_ptr->tls_ptr == QOSA_NULL)
    {
        qurl_free(qcm_ssl_cfg_ptr);
        qurl_free(tls_ctrl_ptr);
        tls_ctrl_ptr = QOSA_NULL;
        return QOSA_NULL;
    }

    return tls_ctrl_ptr;
}

qurl_ecode_t qurl_tls_free(qurl_tls_ctrl_t *tls_ctrl_ptr)
{
    if (tls_ctrl_ptr)
    {
        if (tls_ctrl_ptr->tls_ptr)
        {
            qcm_ssl_free(tls_ctrl_ptr->tls_ptr);
            tls_ctrl_ptr->tls_ptr = QOSA_NULL;
        }
        unirtos_tls_cfg_free(tls_ctrl_ptr->tls_cfg_ptr);
        qurl_safe_free(tls_ctrl_ptr->tls_cfg_ptr);
        qurl_safe_free(tls_ctrl_ptr->remote_host_name);
        qurl_safe_free(tls_ctrl_ptr);
    }

    return QURL_OK;
}

qurl_ecode_t qurl_tls_set_option(qurl_tls_ctrl_t *tls_ctrl_ptr, int opt_tag, void *opt_val)
{
    qurl_ecode_t ret = QURL_OK;

    if (tls_ctrl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    switch (opt_tag)
    {
        case QURL_TLS_OPT_SOCK_FD: {
            int arg = 0;

            if (opt_val != QOSA_NULL)
            {
                arg = *(int *)opt_val;
            }

            tls_ctrl_ptr->sock_fd = arg;
        }
        break;
        case QURL_TLS_OPT_REMOTE_HOST_NAME: {
            char *name_ptr = QOSA_NULL;

            if (tls_ctrl_ptr->remote_host_name != QOSA_NULL)
            {
                qurl_free(tls_ctrl_ptr->remote_host_name);
                tls_ctrl_ptr->remote_host_name = QOSA_NULL;
            }

            if (opt_val != QOSA_NULL)
            {
                name_ptr = qurl_strdup((char *)opt_val);
                if (name_ptr != QOSA_NULL)
                {
                    tls_ctrl_ptr->remote_host_name = name_ptr;
                }
                else
                {
                    ret = QURL_ECODE_NO_MEMORY;
                }
            }
        }
        break;
        case QURL_TLS_OPT_REMOTE_PORT: {
            int arg = 0;

            if (opt_val != QOSA_NULL)
            {
                arg = *(int *)opt_val;
            }

            tls_ctrl_ptr->remote_port = arg;
        }
        break;
        case QURL_TLS_OPT_MATCH_REMOTE_PORT: {
            int arg = 0;

            if (opt_val != QOSA_NULL)
            {
                arg = *(int *)opt_val;
            }

            tls_ctrl_ptr->match_remote_port = arg;
        }
        break;
        default: {
            ret = QURL_ECODE_PARAM_INVALID;
        }
    }

    return ret;
}

qurl_bit_t qurl_tls_data_pending(qurl_tls_ctrl_t *tls_ctrl_ptr)
{
    if (tls_ctrl_ptr == QOSA_NULL || tls_ctrl_ptr->tls_ptr == QOSA_NULL)
    {
        return QOSA_FALSE;
    }

    return qcm_ssl_data_pending(tls_ctrl_ptr->tls_ptr) ? QOSA_TRUE : QOSA_FALSE;
}

qurl_ecode_t qurl_tls_connect(qurl_tls_ctrl_t *tls_ctrl_ptr)
{
    qurl_ecode_t ret = 0;

    if (tls_ctrl_ptr == QOSA_NULL || tls_ctrl_ptr->tls_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    qcm_ssl_set_hostinfo(tls_ctrl_ptr->tls_ptr, tls_ctrl_ptr->remote_host_name, tls_ctrl_ptr->match_remote_port);
    ret = qcm_ssl_connect(tls_ctrl_ptr->tls_ptr);
    if (ret != 0)
    {
        qurl_loge("ret:[%x]\r\n", ret);
        return QURL_ECODE_TLS_CONNECT_ERR;
    }

    return QURL_OK;
}

qurl_ecode_t qurl_tls_connect_nonblocking(qurl_tls_ctrl_t *tls_ctrl_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t ret = 0;

    if (tls_ctrl_ptr == QOSA_NULL || tls_ctrl_ptr->tls_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    qcm_ssl_set_hostinfo(tls_ctrl_ptr->tls_ptr, tls_ctrl_ptr->remote_host_name, tls_ctrl_ptr->match_remote_port);
    ret = qcm_ssl_connect_nonblocking(tls_ctrl_ptr->tls_ptr, (qosa_bool_t *)done_ptr);
    if (ret != 0)
    {
        qurl_loge("ret:[%x]\r\n", ret);
        return QURL_ECODE_TLS_CONNECT_ERR;
    }

    return QURL_OK;
}

qurl_ecode_t qurl_tls_close(qurl_tls_ctrl_t *tls_ctrl_ptr)
{
    if (tls_ctrl_ptr == QOSA_NULL || tls_ctrl_ptr->tls_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    qcm_ssl_close(tls_ctrl_ptr->tls_ptr);

    return QURL_OK;
}

long qurl_tls_read(qurl_tls_ctrl_t *tls_ctrl_ptr, unsigned char *buf, long buf_size, qurl_ecode_t *ecode_ptr)
{
    long                     len = 0;
    qcm_vtls_result_status_e osa_ecode = QCM_VTLS_RESULT_OK;

    *ecode_ptr = QURL_OK;

    if (tls_ctrl_ptr == QOSA_NULL || tls_ctrl_ptr->tls_ptr == QOSA_NULL)
    {
        *ecode_ptr = QURL_ECODE_PARAM_INVALID;
        return -1;
    }

    len = (long)qcm_ssl_read(tls_ctrl_ptr->tls_ptr, (char *)buf, (qosa_size_t)buf_size, &osa_ecode);
    if (osa_ecode != QCM_VTLS_RESULT_OK)
    {
        if (osa_ecode == QCM_VTLS_SSL_READ_WRITE_EAGAIN)
        {
            *ecode_ptr = QURL_ECODE_TLS_READ_AGAIN;
        }
        else if (osa_ecode == QCM_VTLS_SSL_PEER_CLOSE_NOTIFY_ERR)
        {
            *ecode_ptr = QURL_ECODE_TLS_PEER_CLOSE_NOTIFY;
        }
        else
        {
            qurl_loge("osa_ecode:[%x]\r\n", osa_ecode);
            *ecode_ptr = QURL_ECODE_TLS_READ_ERR;
            return -1;
        }
    }

    return len;
}

long qurl_tls_write(qurl_tls_ctrl_t *tls_ctrl_ptr, unsigned char *buf, long buf_size, qurl_ecode_t *ecode_ptr)
{
    long                     len = 0;
    qcm_vtls_result_status_e osa_ecode = QCM_VTLS_RESULT_OK;

    *ecode_ptr = QURL_OK;

    if (tls_ctrl_ptr == QOSA_NULL || tls_ctrl_ptr->tls_ptr == QOSA_NULL)
    {
        *ecode_ptr = QURL_ECODE_PARAM_INVALID;
        return -1;
    }

    /*[202402291600]Fixed the requirement for the tls_write interface to send data in rounds. The next round can only be sent after the current round is completed.*/
    if (tls_ctrl_ptr->cur_w_limit <= 0)
    {
        tls_ctrl_ptr->cur_w_limit = buf_size;
    }
    else
    {
        if (tls_ctrl_ptr->cur_w_limit > buf_size)
        {
            *ecode_ptr = QURL_ECODE_TLS_WRITE_LEN_ERR;
            return -1;
        }
        else if (tls_ctrl_ptr->cur_w_limit < buf_size)
        {
            buf_size = tls_ctrl_ptr->cur_w_limit;
        }
    }

    len = (long)qcm_ssl_write(tls_ctrl_ptr->tls_ptr, (char *)buf, (qosa_size_t)buf_size, &osa_ecode);
    if (osa_ecode != QCM_VTLS_RESULT_OK)
    {
        if (osa_ecode == QCM_VTLS_SSL_READ_WRITE_EAGAIN)
        {
            *ecode_ptr = QURL_ECODE_TLS_WRITE_AGAIN;
        }
        else
        {
            qurl_loge("osa_ecode:[%x]\r\n", osa_ecode);
            *ecode_ptr = QURL_ECODE_TLS_READ_ERR;
            return -1;
        }
    }

    /* [202402291600] */
    if (len > 0)
    {
        tls_ctrl_ptr->cur_w_limit -= len;
    }

    return len;
}
#endif /* QURL_CFG_ENABLE_TLS */
