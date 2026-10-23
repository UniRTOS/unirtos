/*
 * Portions of this file are derived from curl/libcurl's lib/http.c.
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * should be included in the curl distribution. The terms are also
 * available at https://curl.se/docs/copyright.html.
 *
 * For qurl redistribution, see:
 * - qos_components/components/qurl/THIRD_PARTY_NOTICES.md
 * - qos_components/components/qurl/licenses/curl.txt
 */
/** @file         qurl_http.c
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-09-21 16:53:37
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qurl_config.h"
#include "qurl_code_in.h"
#include "qurl_log.h"
#include "qurl_def_in.h"
#include "qurl_slist.h"
#include "qurl_dbuf.h"
#include "qurl_default_basic.h"
#include "qurl_transfer.h"
#include "qurl_url.h"
#include "qurl_core.h"
#include "qurl_conn.h"
#include "qurl_select.h"
#include "qurl_http_def.h"
#include "qurl_http.h"

#if QURL_CFG_ENABLE_HTTP

#if (QURL_CFG_ENABLE_FS == 0)
#include "qurl_default_fs.h"
#endif

/*Function Declaration*/
static qurl_ecode_t http_setup_connect(qurl_conn_t *conn_ptr);
static qurl_ecode_t http_send_prot_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t http_prot_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t http_req(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t http_reqing(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t http_trans_data(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t http_trans_done(qurl_conn_t *conn_ptr, qurl_ecode_t ecode, qurl_bit_t premature);
static int          http_req_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);
static int          http_trans_data_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);

const qurl_port_prot_t g_qurl_port_prot_http = {
    .scheme = "http",
    .setup_connect_func = http_setup_connect,
    .send_prot_connect_func = QOSA_NULL,
    .prot_connect_func = QOSA_NULL,
    .req_func = http_req,
    .reqing_func = http_reqing,
    .req_more_func = QOSA_NULL,
    .trans_data_func = http_trans_data,
    .trans_done_func = http_trans_done,
    .disconnect_func = QOSA_NULL,
    .prot_getsock_func = QOSA_NULL,
    .req_getsock_func = http_req_getsock,
    .req_more_getsock_func = QOSA_NULL,
    .trans_data_getsock_func = http_trans_data_getsock,
    .conn_check_func = QOSA_NULL,
    .attach_func = QOSA_NULL,
    .def_port = QURL_CFG_PORT_HTTP_DEFAULT,
    .protocol = QURL_PROT_TYPE_HTTP,
    .family = QURL_PROT_TYPE_HTTP,
    .flags = QURL_PROT_FLAGS_CREDSPERREQUEST | QURL_PROT_FLAGS_USERPWDCTRL,
};

const qurl_port_prot_t g_qurl_port_prot_https = {
    .scheme = "https",
    .setup_connect_func = http_setup_connect,
    .send_prot_connect_func = http_send_prot_connect,
    .prot_connect_func = http_prot_connect,
    .req_func = http_req,
    .reqing_func = http_reqing,
    .req_more_func = QOSA_NULL,
    .trans_data_func = http_trans_data,
    .trans_done_func = http_trans_done,
    .disconnect_func = QOSA_NULL,
    .prot_getsock_func = QOSA_NULL,
    .req_getsock_func = http_req_getsock,
    .req_more_getsock_func = QOSA_NULL,
    .trans_data_getsock_func = http_trans_data_getsock,
    .conn_check_func = QOSA_NULL,
    .attach_func = QOSA_NULL,
    .def_port = QURL_CFG_PORT_HTTPS_DEFAULT,
    .protocol = QURL_PROT_TYPE_HTTPS,
    .family = QURL_PROT_TYPE_HTTP,
    .flags = QURL_PROT_FLAGS_SSL | QURL_PROT_FLAGS_CREDSPERREQUEST | QURL_PROT_FLAGS_USERPWDCTRL,
};

/*Redirect malloc, free*/
#define http_malloc(s) qurl_malloc(s)
#define http_free(p)                                                                                                                                           \
    do {                                                                                                                                                       \
        if (p != QOSA_NULL)                                                                                                                                    \
        {                                                                                                                                                      \
            qurl_free(p);                                                                                                                                      \
            p = QOSA_NULL;                                                                                                                                     \
        }                                                                                                                                                      \
    } while (0);

/* Check a string for a prefix. Check no more than 'len' bytes */
static qurl_bit_t http_check_prefix_max(const char *prefix, const char *buf, qosa_size_t max_len)
{
    long ch = QURL_MIN(qurl_strlen(prefix), max_len);
    return (qurl_strncasecmp(prefix, buf, ch) == 0);
}

static qurl_ecode_t http_check_prefix(const char *buf, qosa_size_t len)
{
    return http_check_prefix_max("HTTP/", buf, len) ? QURL_OK : QURL_ECODE_HTTP_RESP_HEADER_ERR;
}

/*Determine whether authentication, cookies, or other sensitive data can be sent to a specific host.*/
static qurl_bit_t http_auth_allowed_to_host(qurl_conn_t *conn_ptr)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    return (
        !core_ptr->cfg.bits.is_follow || core_ptr->usr_cfg.bits.allow_auth_to_other_hosts
        || (core_ptr->cfg.first_host && qurl_strcasecmp(core_ptr->cfg.first_host, conn_ptr->host_name) == 0
            && core_ptr->cfg.first_remote_port == conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port
            && core_ptr->cfg.first_protocol == conn_ptr->port_handler->protocol)
    );
}

static qurl_ecode_t http_req_header_host(qurl_conn_t *conn_ptr)
{
    const char       *ptr = QOSA_NULL;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    if (core_ptr->cfg.bits.is_follow == QOSA_FALSE) /*Non-redirection*/
    {
        /*Record some information.*/
        if (core_ptr->cfg.first_host != QOSA_NULL)
        {
            qurl_free(core_ptr->cfg.first_host);
            core_ptr->cfg.first_host = QOSA_NULL;
        }

        core_ptr->cfg.first_host = qurl_strdup(conn_ptr->host_name);
        if (core_ptr->cfg.first_host == QOSA_NULL)
        {
            return QURL_ECODE_NO_MEMORY;
        }
        core_ptr->cfg.first_remote_port = conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port;
        core_ptr->cfg.first_protocol = conn_ptr->port_handler->protocol;
    }

    if (core_ptr->cfg.host != QOSA_NULL)
    {
        qurl_free(core_ptr->cfg.host);
        core_ptr->cfg.host = QOSA_NULL;
    }

    ptr = qurl_trans_check_headers(conn_ptr, "Host");
    if (ptr != QOSA_NULL && (core_ptr->cfg.bits.is_follow == QOSA_FALSE || qurl_strcasecmp(core_ptr->cfg.first_host, conn_ptr->host_name) == 0))
    {
        /*1. Custom headers are only allowed if it is not a redirect or the redirected host is the same.*/
#if QURL_CFG_ENABLE_COOKIES
        /*[lzm][todo][cookies]needs further development*/

#endif /* QURL_CFG_ENABLE_COOKIES */
        if (qurl_strcmp("Host:", ptr))
        {
            qurl_asprintf(&core_ptr->cfg.host, "Host: %s\r\n", &ptr[5]);
            if (core_ptr->cfg.host == QOSA_NULL)
            {
                return QURL_ECODE_NO_MEMORY;
            }
        }
    }
    else
    {
        /*When constructing the Host header attribute, if the hostname is a plain IPv6 address, it must be enclosed in square brackets [brackets].
		 * Specified in RFC2732.*/
        const char *host_ptr = conn_ptr->host_name;

        if (((conn_ptr->basic_port_handler->protocol & (QURL_PROT_TYPE_HTTP))
             && (conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port == QURL_CFG_PORT_HTTP_DEFAULT))
            || ((conn_ptr->basic_port_handler->protocol & (QURL_PROT_TYPE_HTTPS))
                && (conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port == QURL_CFG_PORT_HTTPS_DEFAULT)))
        {
            /*If it is the default port number of the protocol, there is no need to provide a specified port number.*/
            qurl_asprintf(&core_ptr->cfg.host, "Host: %s%s%s\r\n", conn_ptr->bits.ipv6_ip ? "[" : "", host_ptr, conn_ptr->bits.ipv6_ip ? "]" : "");
        }
        else
        {
            qurl_asprintf(
                &core_ptr->cfg.host,
                "Host: %s%s%s:%d\r\n",
                conn_ptr->bits.ipv6_ip ? "[" : "",
                host_ptr,
                conn_ptr->bits.ipv6_ip ? "]" : "",
                conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port
            );
        }
        if (core_ptr->cfg.host == QOSA_NULL)
        {
            return QURL_ECODE_NO_MEMORY;
        }
    }

    return QURL_OK;
}

static qurl_ecode_t http_req_header_user_agent(qurl_conn_t *conn_ptr)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    /*The User-Agent may have already been configured with a default value based on prior logic, as it might have been used in the proxy connection.
	 * If the user has specified this request header field, erase the previously configured one here.*/
    /*First clear*/
    if (core_ptr->cfg.uagent != QOSA_NULL)
    {
        qurl_free(core_ptr->cfg.uagent);
        core_ptr->cfg.uagent = QOSA_NULL;
    }

    if (core_ptr->usr_cfg.user_agent_ptr && !qurl_trans_check_headers(conn_ptr, "User-Agent"))
    {
        qurl_asprintf(&core_ptr->cfg.uagent, "User-Agent: %s\r\n", core_ptr->usr_cfg.user_agent_ptr);
        if (core_ptr->cfg.uagent == QOSA_NULL)
        {
            return QURL_ECODE_NO_MEMORY;
        }
    }

    return QURL_OK;
}

static qurl_ecode_t http_req_header_authorization(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = 0;
    char             *ptr = QOSA_NULL;
    long              len = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    if (core_ptr->cfg.userpwd != QOSA_NULL)
    {
        qurl_free(core_ptr->cfg.userpwd);
        core_ptr->cfg.userpwd = QOSA_NULL;
    }

    if (core_ptr->usr_cfg.http.http_auth && !qurl_trans_check_headers(conn_ptr, "Authorization"))
    {
        if (core_ptr->usr_cfg.http.http_auth && !core_ptr->cfg.http.http_auth)
        {
            core_ptr->cfg.http.http_auth = core_ptr->usr_cfg.http.http_auth;
        }

        /*basic solution*/
        if (core_ptr->cfg.http.http_auth == QURL_HTTP_AUTH_BASIC)
        {
            len = qurl_asprintf(&ptr, "%s:%s", core_ptr->cfg.user ? core_ptr->cfg.user : "", core_ptr->cfg.passwd ? core_ptr->cfg.passwd : "");
            if (ptr == QOSA_NULL)
            {
                return QURL_ECODE_NO_MEMORY;
            }

            ret = qurl_tls_base64_encode((const unsigned char *)ptr, len, conn_ptr->deal_cache_ptr, QURL_CONN_DEAL_CACHE_SIZE - 1, &len);
            qurl_free(ptr);
            ptr = QOSA_NULL;
            if (ret != QURL_OK)
            {
                return ret;
            }
            conn_ptr->deal_cache_ptr[len] = 0x00;

            qurl_asprintf(&core_ptr->cfg.userpwd, "Authorization: Basic %s\r\n", conn_ptr->deal_cache_ptr);
            if (core_ptr->cfg.userpwd == QOSA_NULL)
            {
                return QURL_ECODE_NO_MEMORY;
            }
        }
    }

    return QURL_OK;
}

static qurl_ecode_t http_req_header_referer(qurl_conn_t *conn_ptr)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    /*1. User-defined header priority*/
    /*2. Then proceed to cfg.referer_ptr*/

    /*First clear*/
    if (core_ptr->cfg.ref != QOSA_NULL)
    {
        qurl_free(core_ptr->cfg.ref);
        core_ptr->cfg.ref = QOSA_NULL;
    }

    if (core_ptr->cfg.referer_ptr && !qurl_trans_check_headers(conn_ptr, "Referer"))
    {
        qurl_asprintf(&core_ptr->cfg.ref, "Referer: %s\r\n", core_ptr->cfg.referer_ptr);
        if (core_ptr->cfg.ref == QOSA_NULL)
        {
            return QURL_ECODE_NO_MEMORY;
        }
    }

    return QURL_OK;
}

static qurl_ecode_t http_req_header_accept_encoding(qurl_conn_t *conn_ptr)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    if (core_ptr->cfg.accept_encoding != QOSA_NULL)
    {
        qurl_free(core_ptr->cfg.accept_encoding);
        core_ptr->cfg.accept_encoding = QOSA_NULL;
    }

    if (core_ptr->usr_cfg.accept_encoding_ptr && !qurl_trans_check_headers(conn_ptr, "Accept-Encoding"))
    {
        qurl_asprintf(&core_ptr->cfg.accept_encoding, "Accept-Encoding: %s\r\n", core_ptr->usr_cfg.accept_encoding_ptr);
        if (core_ptr->cfg.accept_encoding == QOSA_NULL)
        {
            return QURL_ECODE_NO_MEMORY;
        }
    }

    return QURL_OK;
}

static qurl_ecode_t http_req_range(qurl_conn_t *conn_ptr)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    /**
	 * The range request header has three levels of impact:
1. Top-level: Determined by user-defined header options: `QURL_OPT_HTTP_HEADER`.
2. Intermediate: Determined by the resume transmission option: `QURL_OPT_RESUME_FROM`.
3. Low-level: Specific option configuration: `QURL_OPT_RANGE`.
	 */

    if (core_ptr->cfg.rangeline != QOSA_NULL)
    {
        qurl_free(core_ptr->cfg.rangeline);
        core_ptr->cfg.rangeline = QOSA_NULL;
    }

    if (!qurl_trans_check_headers(conn_ptr, "Range")
        && ((core_ptr->cfg.http.http_method == QURL_HTTP_METHOD_GET) || (core_ptr->cfg.http.http_method == QURL_HTTP_METHOD_HEAD)))
    {
        if (core_ptr->cfg.bits.use_range)
        {
            qurl_asprintf(&core_ptr->cfg.rangeline, "Range: bytes=%s\r\n", core_ptr->cfg.range_ptr);
        }
    }

    return QURL_OK;
}

static qurl_ecode_t http_req_resume(qurl_conn_t *conn_ptr)
{
    (void)conn_ptr; /*Temporarily skip compilation warnings*/

    /*[lzm][todo]To be developed. Used to implement resume transmission.*/

    return QURL_OK;
}

static qurl_ecode_t http_req_header_client_add(qurl_conn_t *conn_ptr, qurl_dbuf_t dbuf)
{
    qurl_ecode_t       ret = 0;
    char              *ptr = QOSA_NULL;
    qurl_slist_ctrl_t *node = QOSA_NULL;
    qurl_core_ctrl_t  *core_ptr = conn_ptr->core_ptr;

    for (node = core_ptr->usr_cfg.http.http_headers_slist; node; node = node->next)
    {
        const char *name_str = QOSA_NULL;
        const char *value_str = QOSA_NULL;
        long        name_len = 0;
        long        value_len = 0;

        /*1. Find the key and value.*/

        /** 1. Find the key and value based on the ':'.*/
        /** 2. Locate the key based on ';', consider it as having no value.*/
        ptr = qurl_strchr(node->data, ':');
        if (ptr)
        {
            name_str = node->data;
            name_len = ptr - (char *)node->data;
            ptr++; /*Skip spaces*/
            while (*ptr && qurl_ctype_isspace(*ptr))
            {
                ptr++;
            }
            if (*ptr)
            {
                value_str = ptr;
                value_len = qurl_strlen(value_str);
            }
            else
            {
                /*There is no value at this end*/
                continue;
            }
        }
        else
        {
            ptr = qurl_strchr(node->data, ';');

            if (!ptr)
            {
                /*There isn't even a key on this end*/
                continue;
            }

            name_str = node->data;
            name_len = ptr - (char *)node->data;
            ptr++; /* pass the semicolon */
            while (*ptr && qurl_ctype_isspace(*ptr))
            {
                ptr++;
            }
            if (!*ptr)
            {
                /*Ma Dongmei, send null value*/
                value_str = "";
                value_len = 0;
            }
            else
            {
                /*Other formats are pending support, currently ignored for now.*/
                continue;
            }
        }

        /*2. Filter some headers*/
        /** Host */
        if (core_ptr->cfg.host && http_check_prefix_max("Host:", name_str, name_len))
        {
            /*Already processed*/
            continue;
        }
        else if ((core_ptr->cfg.http.http_method == QURL_HTTP_METHOD_POST_FORM || core_ptr->cfg.http.http_method == QURL_HTTP_METHOD_POST_MIME) && http_check_prefix_max("Content-Type:", name_str, name_len))
        {
            /*These are not in the request headers, but are handled in the partial processing.*/
            continue;
        }
        else if ((core_ptr->cfg.http.http_method == QURL_HTTP_METHOD_PUT_FORM || core_ptr->cfg.http.http_method == QURL_HTTP_METHOD_PUT_MIME) && http_check_prefix_max("Content-Type:", name_str, name_len))
        {
            /*These are not in the request headers, but are handled in the partial processing.*/
            continue;
        }
        else if ((core_ptr->cfg.http.http_method == QURL_HTTP_METHOD_PATCH_FORM || core_ptr->cfg.http.http_method == QURL_HTTP_METHOD_PATCH_MIME) && http_check_prefix_max("Content-Type:", name_str, name_len))
        {
            /*These are not in the request headers, but are handled in the partial processing.*/
            continue;
        }
        else if (conn_ptr->bits.authneg && http_check_prefix_max("Content-Length:", name_str, name_len))
        {
            /*During identity verification negotiation, customizing the Content-Length value is not allowed, as the content length must be forcibly set to zero in this scenario.*/
            continue;
        }
        else if (core_ptr->cfg.te && http_check_prefix_max("Connection:", name_str, name_len))
        {
            /** When using chunked encoding (Transfer-Encoding: chunked), custom Connection headers cannot be set.
			 * Because if both Transfer-Encoding and Connection headers are present, it can cause the server to fail to process the request correctly.
			 * In fact, RFC 7230 has clear stipulations:
			 * If both "Connection" and "Transfer-Encoding" headers are present in the message and their values are inconsistent,
			 * then the "Transfer-Encoding" header should be prioritized, and the "Connection" header should be ignored.*/
            continue;
        }
        else if (core_ptr->usr_cfg.http.http_version >= QURL_HTTP_VERSION_2_0 && http_check_prefix_max("Transfer-Encoding:", name_str, name_len))
        {
            /*HTTP/2 does not support chunked encoding for requests.*/
            continue;
        }
        else if ((!http_auth_allowed_to_host(conn_ptr)) && (http_check_prefix_max("Authorization:", name_str, name_len) || http_check_prefix_max("Cookie:", name_str, name_len)))
        {
            /*Do not send these potentially sensitive header information to other hosts.*/
            continue;
        }

        /*3. Writable Send Buffer*/
        ret = qurl_dbuf_push_data(dbuf, (void *)name_str, name_len); /*Key*/
        if (ret != QURL_OK)
        {
            return ret;
        }
        ret = qurl_dbuf_push_data(dbuf, ": ", 2); /*Add a space, please.*/
        if (ret != QURL_OK)
        {
            return ret;
        }
        ret = qurl_dbuf_push_data(dbuf, (void *)value_str, value_len); /*Value*/
        if (ret != QURL_OK)
        {
            return ret;
        }
        ret = qurl_dbuf_push_data(dbuf, "\r\n", 2); /*Add line break*/
        if (ret != QURL_OK)
        {
            return ret;
        }
    }

    return QURL_OK;
}

static void http_method(qurl_conn_t *conn_ptr, const char **str_pptr)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    const char       *req_str = QOSA_NULL;

    if (core_ptr->usr_cfg.bits.opt_nobody)
    {
        req_str = "HEAD";
    }
    else if (core_ptr->usr_cfg.customrequest_ptr)
    {
        req_str = core_ptr->usr_cfg.customrequest_ptr;
    }
    else
    {
        switch (core_ptr->cfg.http.http_method)
        {
            case QURL_HTTP_METHOD_POST:
            case QURL_HTTP_METHOD_POST_FORM:
            case QURL_HTTP_METHOD_POST_MIME: {
                req_str = "POST";
                break;
            }
            case QURL_HTTP_METHOD_PUT:
            case QURL_HTTP_METHOD_PUT_FORM:
            case QURL_HTTP_METHOD_PUT_MIME: {
                req_str = "PUT";
                break;
            }
            case QURL_HTTP_METHOD_PATCH:
            case QURL_HTTP_METHOD_PATCH_FORM:
            case QURL_HTTP_METHOD_PATCH_MIME: {
                req_str = "PATCH";
                break;
            }
            default: /*It is get by default.*/
            case QURL_HTTP_METHOD_GET: {
                req_str = "GET";
                break;
            }
            case QURL_HTTP_METHOD_HEAD: {
                req_str = "HEAD";
                break;
            }
        }
    }

    *str_pptr = req_str;
}

/**
 * @brief Implement a raw head API that can be inserted
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t http_req_raw_head(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    long              len = 0;
    unsigned char    *tmp_ptr = QOSA_NULL;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);

    qurl_logd("enter\r\n");

    core_idle_timeout_feed_dog(core_ptr);
    if (core_ptr->usr_cfg.upload_head_data_ptr)
    {
        len = core_ptr->trans.upload_head_len_max - core_ptr->trans.uploaded_head_len;
        /*[lzm][note]If a customer reports insufficient memory, then handle this. Send customer data in batches, add a length limit to dbuf, and add a progress bar, so that all data is not pushed to dbuf at once.*/
        qurl_dbuf_push_data(core_ptr->send_cache, core_ptr->usr_cfg.upload_head_data_ptr, len);
        core_ptr->trans.uploaded_head_len += len; /*All entries have been enqueued.*/
    }
    else if (core_ptr->usr_cfg.upload_head_file_ptr)
    {
        len = core_ptr->trans.upload_head_len_max - core_ptr->trans.uploaded_head_len;
        len = QURL_MIN(len, QURL_CONN_DEAL_CACHE_SIZE);
        /*[lzm][note]Currently, reading files is done sequentially, so there is no need to add a seek function, but this development may still be required later if issues arise.*/
        len = qurl_fread(core_ptr->cfg.upload_head_file_fd, conn_ptr->deal_cache_ptr, len);
        if (len < 0)
        {
            return QURL_ECODE_FS_READ_ERR;
        }
        qurl_dbuf_push_data(core_ptr->send_cache, conn_ptr->deal_cache_ptr, len);
        core_ptr->trans.uploaded_head_len += len;
        if (core_ptr->trans.upload_head_len_max <= core_ptr->trans.uploaded_head_len)
        {
            qurl_fclose(core_ptr->cfg.upload_head_file_fd);
            core_ptr->cfg.upload_head_file_fd = -1;
        }
    }
    else if (core_ptr->usr_cfg.read_head_from_client_func)
    {
        char raw_cache_len = sizeof(http_ctrl_ptr->autoid_head_raw_cache);

        if (core_ptr->trans.upload_head_len_max <= -1)
        {
            qurl_memcpy(conn_ptr->deal_cache_ptr, http_ctrl_ptr->autoid_head_raw_cache, raw_cache_len);
            len = QURL_CONN_DEAL_CACHE_SIZE - raw_cache_len - 1;
        }
        else
        {
            len = QURL_MIN(len, QURL_CONN_DEAL_CACHE_SIZE - raw_cache_len - 1);
        }
        len = core_ptr->usr_cfg.read_head_from_client_func(conn_ptr->deal_cache_ptr + raw_cache_len, len, core_ptr->usr_cfg.read_head_from_client_arg);
        /*Search for "\r\n\r\n" and truncate everything after it.*/
        if (core_ptr->trans.upload_head_len_max <= -1 && len > 0)
        {
            conn_ptr->deal_cache_ptr[len + raw_cache_len] = 0x00;
            tmp_ptr = (unsigned char *)qurl_strstr((char *)conn_ptr->deal_cache_ptr, "\r\n\r\n");
            if (tmp_ptr == QOSA_NULL)
            {
                /*Not found: tail storage*/
                qurl_memcpy(http_ctrl_ptr->autoid_head_raw_cache, conn_ptr->deal_cache_ptr + len, raw_cache_len);
            }
            else
            {
                /*Recognized the end of the request header: trim excess part + update len*/
                len = (tmp_ptr + qurl_strlen("\r\n\r\n")) - (conn_ptr->deal_cache_ptr + raw_cache_len);
                core_ptr->trans.upload_head_len_max = core_ptr->trans.uploaded_head_len + len;
                qurl_memset(http_ctrl_ptr->autoid_head_raw_cache, 0xff, raw_cache_len);
            }
        }
        qurl_dbuf_push_data(core_ptr->send_cache, (void *)(conn_ptr->deal_cache_ptr + raw_cache_len), len);
        core_ptr->trans.uploaded_head_len += len;
    }
    else
    {
        qurl_logd("no head data upload!!!\r\n");
    }
    QURL_CORE_IDLE_TIMEOUT_CHECK_DOG_FUNC(core_ptr);
    return ret;
}

/**
 * @brief The goal is to confirm the length of the body.
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t http_req_raw_head_body_headers(qurl_conn_t *conn_ptr)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    switch (core_ptr->cfg.http.http_method)
    {
        case QURL_HTTP_METHOD_POST_MIME:
        case QURL_HTTP_METHOD_POST_FORM: {
            if (conn_ptr->bits.authneg)
            {
                /*No data upload is required during the identity authentication phase.*/
                core_ptr->trans.upload_len_max = 0;
                core_ptr->trans.uploaded_len = 0;
            }
            else
            {
                core_ptr->trans.upload_len_max = qurl_http_multiform_get_total_len(&core_ptr->usr_cfg.http.multiform_ctrl);
                core_ptr->trans.uploaded_len = 0;
            }

            break;
        }
        case QURL_HTTP_METHOD_POST: {
            /*Confirm the uploaded size*/
            if (conn_ptr->bits.authneg)
            {
                /*No data upload is required during the identity authentication phase.*/
                core_ptr->trans.upload_len_max = 0;
                core_ptr->trans.uploaded_len = 0;
            }
            else
            {
                /*The uploaded size is taken from the user side.*/
                core_ptr->trans.upload_len_max = core_ptr->cfg.upload_size;
                core_ptr->trans.uploaded_len = 0;
            }

            break;
        }
        case QURL_HTTP_METHOD_PUT_FORM:
        case QURL_HTTP_METHOD_PUT_MIME:
        case QURL_HTTP_METHOD_PATCH_FORM:
        case QURL_HTTP_METHOD_PATCH_MIME: {
            if (conn_ptr->bits.authneg)
            {
                /*No data upload is required during the identity authentication phase.*/
                core_ptr->trans.upload_len_max = 0;
                core_ptr->trans.uploaded_len = 0;
            }
            else
            {
                core_ptr->trans.upload_len_max = qurl_http_multiform_get_total_len(&core_ptr->usr_cfg.http.multiform_ctrl);
                qurl_logd("upload_len_max:[%ld]\r\n", core_ptr->trans.upload_len_max);
                core_ptr->trans.uploaded_len = 0;
            }

            break;
        }
        case QURL_HTTP_METHOD_PUT:
        case QURL_HTTP_METHOD_PATCH: {
            /*Confirm the uploaded size*/
            if (conn_ptr->bits.authneg)
            {
                /*No data upload is required during the identity authentication phase.*/
                core_ptr->trans.upload_len_max = 0;
                core_ptr->trans.uploaded_len = 0;
            }
            else
            {
                /*The uploaded size is taken from the user side.*/
                core_ptr->trans.upload_len_max = core_ptr->cfg.upload_size;
                qurl_logd("upload_len_max:[%ld]\r\n", core_ptr->trans.upload_len_max);
                core_ptr->trans.uploaded_len = 0;
            }

            break;
        }
        default: /*It is get by default.*/
        case QURL_HTTP_METHOD_GET:
        case QURL_HTTP_METHOD_HEAD: {
            /*The GET and HEAD methods do not have a body.*/

            break;
        }
    }

    return QURL_OK;
}

/**
 * @brief The goal is to determine the length of the body and then assemble the request headers related to the body.
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t http_req_body_headers(qurl_conn_t *conn_ptr)
{
    long              len = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    switch (core_ptr->cfg.http.http_method)
    {
        case QURL_HTTP_METHOD_POST_MIME:
        case QURL_HTTP_METHOD_POST_FORM:
        case QURL_HTTP_METHOD_PUT_FORM:
        case QURL_HTTP_METHOD_PUT_MIME:
        case QURL_HTTP_METHOD_PATCH_FORM:
        case QURL_HTTP_METHOD_PATCH_MIME: {
            if (conn_ptr->bits.authneg)
            {
                /*No data upload is required during the identity authentication phase.*/
                core_ptr->trans.upload_len_max = 0;
                core_ptr->trans.uploaded_len = 0;
            }
            else
            {
                core_ptr->trans.upload_len_max = qurl_http_multiform_get_total_len(&core_ptr->usr_cfg.http.multiform_ctrl);
                qurl_logd("upload_len_max:[%ld]\r\n", core_ptr->trans.upload_len_max);
                core_ptr->trans.uploaded_len = 0;
            }
            len = qurl_snprintf((char *)conn_ptr->deal_cache_ptr, QURL_CONN_DEAL_CACHE_SIZE - 1, "Content-Length: %ld\r\n", core_ptr->trans.upload_len_max);
            qurl_dbuf_push_data(core_ptr->send_cache, conn_ptr->deal_cache_ptr, len);
            len = 0;

            /*Expect: This field is under development.*/

            /* Content-Type: */
            if (!qurl_trans_check_headers(conn_ptr, "Content-Type"))
            {
                char *ptr = QOSA_NULL;
                ptr = qurl_http_multiform_get_boundary(&core_ptr->usr_cfg.http.multiform_ctrl);
                len = qurl_snprintf(
                    (char *)conn_ptr->deal_cache_ptr,
                    QURL_CONN_DEAL_CACHE_SIZE - 1,
                    "Content-Type: multipart/form-data; boundary=%s\r\n",
                    ptr ? ptr : ""
                );
                qurl_dbuf_push_data(core_ptr->send_cache, conn_ptr->deal_cache_ptr, len);
                len = 0;
            }

            break;
        }
        case QURL_HTTP_METHOD_POST: {
            /*Confirm the uploaded size*/
            if (conn_ptr->bits.authneg)
            {
                /*No data upload is required during the identity authentication phase.*/
                core_ptr->trans.upload_len_max = 0;
                core_ptr->trans.uploaded_len = 0;
            }
            else
            {
                /*The uploaded size is taken from the user side.*/
                core_ptr->trans.upload_len_max = core_ptr->cfg.upload_size;
                qurl_logd("upload_len_max:[%ld]\r\n", core_ptr->trans.upload_len_max);
                core_ptr->trans.uploaded_len = 0;
            }

            /* Content-Length: */
            if ((core_ptr->cfg.bits.upload) && (conn_ptr->bits.authneg || !qurl_trans_check_headers(conn_ptr, "Content-Length")))
            {
                /** [lzm][note]This logic needs optimization. Ensure no duplicate "Content-Length" occurs during optimization.
				 * Current: If there is a length, authentication is in progress and packets can be assembled, and it is overriding the user's custom configuration.
				 * If not in identity authentication and the user has not defined a custom request header, it must be assembled manually.
				 */
                len = qurl_snprintf((char *)conn_ptr->deal_cache_ptr, QURL_CONN_DEAL_CACHE_SIZE - 1, "Content-Length: %ld\r\n", core_ptr->trans.upload_len_max);
                qurl_dbuf_push_data(core_ptr->send_cache, conn_ptr->deal_cache_ptr, len);
            }

            /* Content-Type: */
            if (!qurl_trans_check_headers(conn_ptr, "Content-Type"))
            {
                len = qurl_snprintf((char *)conn_ptr->deal_cache_ptr, QURL_CONN_DEAL_CACHE_SIZE - 1, "Content-Type: application/x-www-form-urlencoded\r\n");
                qurl_dbuf_push_data(core_ptr->send_cache, conn_ptr->deal_cache_ptr, len);
            }

            break;
        }
        case QURL_HTTP_METHOD_PUT:
        case QURL_HTTP_METHOD_PATCH: {
            /*Confirm the uploaded size*/
            if (conn_ptr->bits.authneg)
            {
                /*No data upload is required during the identity authentication phase.*/
                core_ptr->trans.upload_len_max = 0;
                core_ptr->trans.uploaded_len = 0;
            }
            else
            {
                /*The uploaded size is taken from the user side.*/
                core_ptr->trans.upload_len_max = core_ptr->cfg.upload_size;
                qurl_logd("upload_len_max:[%ld]\r\n", core_ptr->trans.upload_len_max);
                core_ptr->trans.uploaded_len = 0;
            }

            /* Content-Length: */
            if ((core_ptr->cfg.bits.upload) && (conn_ptr->bits.authneg || !qurl_trans_check_headers(conn_ptr, "Content-Length")))
            {
                /** [lzm][note]This logic needs optimization. Ensure no duplicate "Content-Length" occurs during optimization.
				 * Current: If there is a length, authentication is in progress and packets can be assembled, and it is overriding the user's custom configuration.
				 * If not in identity authentication and the user has not defined a custom request header, it must be assembled manually.
				 */
                len = qurl_snprintf((char *)conn_ptr->deal_cache_ptr, QURL_CONN_DEAL_CACHE_SIZE - 1, "Content-Length: %ld\r\n", core_ptr->trans.upload_len_max);
                qurl_dbuf_push_data(core_ptr->send_cache, conn_ptr->deal_cache_ptr, len);
            }

            break;
        }
        default: /*It is get by default.*/
        case QURL_HTTP_METHOD_GET:
        case QURL_HTTP_METHOD_HEAD: {
            /*The GET and HEAD methods do not have a body.*/

            break;
        }
    }

    return QURL_OK;
}

/**
 * @brief Implement a body API that can be inserted
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t http_req_body(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    long              len = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    qurl_logd("enter\r\n");

    switch (core_ptr->cfg.http.http_method)
    {
        case QURL_HTTP_METHOD_POST_MIME:
        case QURL_HTTP_METHOD_POST_FORM:
        case QURL_HTTP_METHOD_PUT_FORM:
        case QURL_HTTP_METHOD_PUT_MIME:
        case QURL_HTTP_METHOD_PATCH_FORM:
        case QURL_HTTP_METHOD_PATCH_MIME: {
            len = qurl_http_multiform_get_len(&core_ptr->usr_cfg.http.multiform_ctrl);
            ret = qurl_http_multiform_send_body(conn_ptr);
            core_ptr->trans.uploaded_len += (len - qurl_http_multiform_get_len(&core_ptr->usr_cfg.http.multiform_ctrl));
            break;
        }
        case QURL_HTTP_METHOD_PUT: /*[lzm][note]First use POST*/
        case QURL_HTTP_METHOD_POST:
        case QURL_HTTP_METHOD_PATCH: {
            len = core_ptr->trans.upload_len_max - core_ptr->trans.uploaded_len;
            if (len > 0
                || (core_ptr->trans.upload_len_max < 0 && core_ptr->usr_cfg.upload_data_ptr == QOSA_NULL && core_ptr->usr_cfg.upload_file_ptr == QOSA_NULL
                    && core_ptr->usr_cfg.read_from_client_func != QOSA_NULL) /*Only callback function mode is supported for unspecified length.*/
            )
            {
                if (core_ptr->usr_cfg.upload_data_ptr)
                {
                    /*[lzm][note]If a customer reports insufficient memory, then handle this. Send customer data in batches, add a length limit to dbuf, and add a progress bar, so that all data is not pushed to dbuf at once.*/
                    ret = qurl_dbuf_push_data(core_ptr->send_cache, core_ptr->usr_cfg.upload_data_ptr, len);
                    core_ptr->trans.uploaded_len += len; /*All entries have been enqueued.*/
                }
                else if (core_ptr->usr_cfg.upload_file_ptr)
                {
                    len = QURL_MIN(len, QURL_CONN_DEAL_CACHE_SIZE);
                    /*[lzm][note]Currently, reading files is done sequentially, so there is no need to add a seek function, but this development may still be required later if issues arise.*/
                    len = qurl_fread(core_ptr->cfg.upload_file_fd, conn_ptr->deal_cache_ptr, len);
                    if (len < 0)
                    {
                        ret = QURL_ECODE_FS_READ_ERR;
                        break;
                    }
                    ret = qurl_dbuf_push_data(core_ptr->send_cache, conn_ptr->deal_cache_ptr, len);
                    core_ptr->trans.uploaded_len += len;
                    if (core_ptr->trans.upload_len_max <= core_ptr->trans.uploaded_len)
                    {
                        qurl_fclose(core_ptr->cfg.upload_file_fd);
                        core_ptr->cfg.upload_file_fd = -1;
                    }
                }
                else if (core_ptr->usr_cfg.read_from_client_func)
                {
                    if (core_ptr->trans.upload_len_max < 0)
                    {
                        len = QURL_CONN_DEAL_CACHE_SIZE;
                    }
                    else
                    {
                        len = QURL_MIN(len, QURL_CONN_DEAL_CACHE_SIZE);
                    }
                    len = core_ptr->usr_cfg.read_from_client_func(conn_ptr->deal_cache_ptr, len, core_ptr->usr_cfg.read_from_client_arg);
                    if (len < 0)
                    {
                        ret = QURL_ECODE_FS_READ_ERR;
                    }
                    else if (core_ptr->trans.upload_len_max < 0 && len == 0)
                    {
                        core_ptr->trans.upload_len_max = core_ptr->trans.uploaded_len;
                    }
                    else
                    {
                        ret = qurl_dbuf_push_data(core_ptr->send_cache, (void *)conn_ptr->deal_cache_ptr, len);
                        core_ptr->trans.uploaded_len += len;
                    }
                }
                else
                {
                    qurl_logd("no data upload!!!\r\n");
                }
            }
            break;
        }
        default: /*It is get by default.*/
        case QURL_HTTP_METHOD_GET:
        case QURL_HTTP_METHOD_HEAD: {
            /*The GET and HEAD methods do not have a body.*/

            break;
        }
    }

    return ret;
}

static void http_version_str(qurl_conn_t *conn_ptr, const char **str_pptr)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    const char       *str_ptr = QOSA_NULL;

    if (core_ptr->usr_cfg.http.http_version == QURL_HTTP_VERSION_NONE || core_ptr->usr_cfg.http.http_version == QURL_HTTP_VERSION_1_1)
    {
        str_ptr = "1.1";
    }
    else if (core_ptr->usr_cfg.http.http_version == QURL_HTTP_VERSION_2_0 || core_ptr->usr_cfg.http.http_version == QURL_HTTP_VERSION_2TLS || core_ptr->usr_cfg.http.http_version == QURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE)
    {
        str_ptr = "2.0";
    }
    else if (core_ptr->usr_cfg.http.http_version == QURL_HTTP_VERSION_3)
    {
        str_ptr = "3.0";
    }
    else
    {
        str_ptr = "1.0";
    }

    *str_pptr = str_ptr;
}

static qurl_ecode_t http_send_req(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    long              len = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    *done_ptr = QOSA_FALSE;

    /*Push to conn*/
    qurl_dbuf_concat_tail(conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].send_cache, core_ptr->send_cache);

    ret = qurl_conn_send_cache(conn_ptr, QURL_SOCKET_INDEX_0);
    if (ret != QURL_OK)
    {
        qurl_loge("req send faild\r\n");
        return ret;
    }

    /*Check if all have been sent to the underlying protocol stack*/
    len = qurl_dbuf_vaild_size(conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].send_cache);
    qurl_logd("unsend data len:[%ld]\r\n", len);
    if (len == 0) /*All transmissions completed*/
    {
        *done_ptr = QOSA_TRUE;
    }

    return QURL_OK;
}

/*Called after receiving a normal status line.*/
static qurl_ecode_t http_http_statusline(qurl_conn_t *conn_ptr)
{
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);

    /**
	 * It is still unclear how to interpret the response code, as this depends on other possible response headers.
	 * Status codes 401 and 407 may indicate errors, but they can also be normal situations, depending on how authentication works.
	 * Other status codes clearly indicate errors, so processing is abandoned here.
	 */
    /**
	 * [lzm][node]: (Resume after data transmission pause) && (HTTP GET request) && (Response status code is 416)
	 * The HTTP server returned status code 416, indicating that the requested range cannot be satisfied. However, because data transmission needs to be resumed after a pause, the program will continue to simulate the request, ignore this connection error, and treat the output as normal data.
	 */

    /*Long connection related*/
    if (http_ctrl_ptr->resp.http_code == 10)
    {
        /**
		 * According to the default behavior of HTTP/1.0, unless the server sends specific header information indicating to keep the connection, the connection should be closed after sending the response body.
		 */
        qurl_logd("HTTP 1.0, assume close after body");
        /*Is there anything to mark?*/
    }
    else if (http_ctrl_ptr->resp.http_code == 11)
    {
        qurl_logd("HTTP 1.1 or later with persistent connection");
    }

    /*Response code marker without body*/
    http_ctrl_ptr->http_bodyless = http_ctrl_ptr->resp.http_code >= 100 && http_ctrl_ptr->resp.http_code < 200;
    switch (http_ctrl_ptr->resp.http_code)
    {
        case 304:
        /**
		 * For responses with a status code of 304, Section 10.3.5 of RFC2616 (the HTTP/1.1 specification) states:
		 * The response "must not include a message body," therefore it always ends with the first empty line after the header fields.
		 */
        case 204:
            /**
			 * For responses with status code 204, Section 10.2.5 of RFC2616 stipulates:
			 * The response "has completed the request but does not need to return an entity body."
			 * Therefore, similar to a 304 response, a 204 response "must not include a message body," so it always ends with the first empty line after the header fields.
			 */
            http_ctrl_ptr->http_bodyless = QOSA_TRUE;
            break;

        default:
            break;
    }

    return QURL_OK;
}

/**
 * @brief If any Transfer-Encoding is present in the response, the Content-Length must be ignored.
 * Please refer to RFC 7230 Section 3.3.3 and RFC 2616 Section 4.4.
 * After receiving all headers but before the final call to the user's header callback, processing occurs here so that the user can obtain the valid content length in the final call.
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t http_size(qurl_conn_t *conn_ptr)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);

    if (http_ctrl_ptr->chunk)
    {
        core_ptr->trans.download_len_max = -1; /*Unrestricted*/
    }
    else
    {
        if (core_ptr->usr_cfg.max_filesize && http_ctrl_ptr->resp.content_length > core_ptr->usr_cfg.max_filesize)
        {
            return QURL_ECODE_FILESIZE_EXCEEDED;
        }
        else
        {
            if (core_ptr->trans.download_len_max < 0)
            {
                core_ptr->trans.download_len_max = http_ctrl_ptr->resp.content_length; /*Reference request header fields*/
            }
            else
            {
                core_ptr->trans.download_len_max += http_ctrl_ptr->resp.content_length; /*Reference request header fields*/
            }
        }
    }

    return QURL_OK;
}

/**
 * @brief Remove leading and trailing whitespace characters from the given HTTP header line and return a copied string.
 * If memory allocation fails, return NULL. If the header value consists entirely of whitespace characters, return an empty string.
 *
 * @param header
 * @return char*
 */
char *http_copy_header_value(const char *header)
{
    const char *start = QOSA_NULL;
    const char *end = QOSA_NULL;
    char       *value = QOSA_NULL;
    int         len = 0;

    /* Find the end of the header name */
    while (*header && (*header != ':'))
    {
        ++header;
    }

    if (*header)
    {
        /*Skip the colon*/
        ++header;
    }

    /*Find a non-space character*/
    start = header;
    while (*start && qurl_ctype_isspace(*start))
    {
        start++;
    }

    /*The data uses host encoding representation, therefore '\r' and '\n' should be used instead of 0x0d and 0x0a.*/
    end = qurl_strchr(start, '\r');
    if (!end)
    {
        end = qurl_strchr(start, '\n');
    }
    if (!end)
    {
        end = qurl_strchr(start, '\0');
    }
    if (!end)
    {
        return QOSA_NULL;
    }

    /*Skip all trailing whitespace characters*/
    while ((end > start) && qurl_ctype_isspace(*end))
    {
        end--;
    }

    /*Get the length of the valid string*/
    len = end - start + 1;

    value = qurl_malloc(len + 1);
    if (!value)
    {
        return QOSA_NULL;
    }

    qurl_memcpy(value, start, len);
    value[len] = 0; /* null-terminate */

    return value;
}

static qurl_bit_t http_is_valid_auth_separator(char ch)
{
    return ch == '\0' || ch == ',' || qurl_ctype_isspace(ch);
}

/**
 * @brief The function selects the most advantageous authentication method from the available methods and the ones we desire.
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_bit_t http_pick_one_auth(qurl_conn_t *conn_ptr)
{
    qurl_bit_t        ret = QOSA_TRUE;
    unsigned long     avail = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    avail = core_ptr->usr_cfg.http.http_auth & core_ptr->cfg.http.http_auth_recv;

    /*Filtering order*/
    /** NEGOTIATE */
    /** BEARER */
    /** DIGEST */
    /** NTLM */
    /** NTLM_WB */
    /** BASIC */
    /** AWS_SIGV4 */

    /*Start screening*/
    if (avail & QURL_HTTP_AUTH_BASIC)
    {
        core_ptr->cfg.http.http_auth = avail;
    }
    else
    {
        core_ptr->cfg.http.http_auth = QURL_HTTP_AUTH_NONE;
        ret = QOSA_FALSE;
    }

    return ret;
}

static qurl_bit_t http_should_fail(qurl_conn_t *conn_ptr)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    /*[lzm][todo]To be developed, to be completed*/

    return core_ptr->cfg.bits.auth_failed;
}

/**
 * When all HTTP headers have been received, the http_auth_act() function will be called,
 * It checks the available authentication methods and decides which one to use, if any.
 * If an authentication method is selected, 'newurl' will be set.
 */
static qurl_ecode_t http_auth_act(qurl_conn_t *conn_ptr)
{
    qurl_bit_t        picked = QOSA_FALSE;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);

    qurl_logd("enter\r\n");

    core_ptr->cfg.http.http_auth = QURL_HTTP_AUTH_NONE; /*Temporary mark cleared*/

    if (http_ctrl_ptr->resp.http_code >= 100 && http_ctrl_ptr->resp.http_code <= 199)
    {
        return QURL_OK;
    }

    if (core_ptr->cfg.bits.auth_failed)
    {
        return QURL_ECODE_HTTP_AUTH_FAILED;
    }

    if ((core_ptr->cfg.user || core_ptr->usr_cfg.username_ptr)                   /*The username is provided in the URL or directly set via the opt parameter.*/
        && ((http_ctrl_ptr->resp.http_code == 401)                               /*Authentication required*/
            || (conn_ptr->bits.authneg && http_ctrl_ptr->resp.http_code < 300))) /*Authentication has not yet completed, but the received http_code is not considered an error; it can proceed.*/
    {
        /*Determine an identity authentication scheme.*/
        picked = http_pick_one_auth(conn_ptr);
        if (picked == QOSA_FALSE)
        {
            core_ptr->cfg.bits.auth_failed = QOSA_TRUE; /*Method not supported*/
        }
        else
        {
            /*Assign new_url for use in re-initiating the request*/
            if (core_ptr->cfg.new_url)
            {
                qurl_free(core_ptr->cfg.new_url);
            }
            core_ptr->cfg.new_url = qurl_strdup(core_ptr->cfg.url_ptr);
            if (core_ptr->cfg.new_url == QOSA_NULL)
            {
                return QURL_ECODE_NO_MEMORY;
            }
        }
    }

    if (http_should_fail(conn_ptr))
    {
        return QURL_ECODE_HTTP_AUTH_FAILED;
    }

    return QURL_OK;
}

static qurl_ecode_t http_read_headers(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = 0;
    qurl_bit_t        recv_cache_done = QOSA_TRUE; /*All data in core_ptr->recv_cache has been read.*/
    long              len = 0;
    long              recv_dc_r_index = 0;         /*!< The read pointer of http_ctrl_ptr->deal_cache_ptr*/
    long              recv_dc_w_index = 0;         /*!< Write pointer for http_ctrl_ptr->deal_cache_ptr*/
    long              parsed_len = 0;              /*length from start_ptr to end_ptr*/
    char             *start_ptr = QOSA_NULL;
    char             *end_ptr = QOSA_NULL;
    char             *temp_ptr = QOSA_NULL;      /*Used when parsing detailed strings*/
    char             *head_line_ptr = QOSA_NULL; /*A row of header pointers, storing response headers for user reading.*/
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);

    /*Processing in QURL_HTTP_STATE_READ_HEADER state*/

    /*1. Check the status line*/
    /*2. Check Header*/
    /*3. Write the received body to the user side.*/

    qurl_logd("enter\r\n");

    len = qurl_dbuf_vaild_size(core_ptr->recv_cache);
    if (len > QURL_CONN_DEAL_CACHE_SIZE)
    {
        recv_cache_done = QOSA_FALSE;
    }
    len = QURL_CONN_DEAL_CACHE_SIZE;
    qurl_dbuf_read_data_try(core_ptr->recv_cache, conn_ptr->deal_cache_ptr, &len, 0);
    recv_dc_w_index += len;

    if ((recv_dc_w_index - recv_dc_r_index) <= 0 || len <= 0) /*No data needs to be parsed || No new data needs to be parsed*/
    {
        return QURL_OK;
    }

    do {
        start_ptr = (char *)conn_ptr->deal_cache_ptr + recv_dc_r_index;
        /*Check for line terminator '\n'*/
        end_ptr = qurl_memchr(start_ptr, 0x0a, (recv_dc_w_index - recv_dc_r_index));

        if (end_ptr == QOSA_NULL)                /*Not yet received a complete line*/
        {
            if (http_ctrl_ptr->header_line == 0) /*Still receiving the first line*/
            {
                /*It can also check whether it is functioning normally.*/
                ret = http_check_prefix(start_ptr, (recv_dc_w_index - recv_dc_r_index));
                if (ret != QURL_OK)
                {
                    /*Header Error*/
                    return ret;
                }
            }

            if (recv_cache_done == QOSA_FALSE) /*There is still unread data*/
            {
                recv_cache_done = QOSA_TRUE;
                len = qurl_dbuf_vaild_size(core_ptr->recv_cache);
                if (len > QURL_CONN_DEAL_CACHE_SIZE)
                {
                    recv_cache_done = QOSA_FALSE;
                }
                len = QURL_CONN_DEAL_CACHE_SIZE;
                qurl_dbuf_read_data_try(core_ptr->recv_cache, conn_ptr->deal_cache_ptr, &len, 0);
                recv_dc_w_index = len;

                if ((recv_dc_w_index - recv_dc_r_index) <= 0 || len <= 0) /*No data needs to be parsed || No new data needs to be parsed*/
                {
                    return QURL_OK;
                }
            }
            else
            {
                break; /* read more and try again */
            }
        }

        /*Complete line received*/

        /*This entire line will be parsed, so first extract the cache*/
        parsed_len = end_ptr - start_ptr + 1;
        recv_dc_r_index += parsed_len;
        /** Store these header fields if needed.*/
        qurl_safe_free(head_line_ptr);
        if (core_ptr->usr_cfg.write_head_to_client_func != QOSA_NULL)
        {
            head_line_ptr = qurl_malloc(parsed_len + 1);
            if (head_line_ptr == QOSA_NULL)
            {
                qurl_slist_del_all(core_ptr->cfg.http.http_resp_headers_slist);
                core_ptr->cfg.http.http_resp_headers_slist = QOSA_NULL;
                qurl_safe_free(head_line_ptr);
                return QURL_ECODE_NO_MEMORY;
            }
            head_line_ptr[parsed_len] = 0x00;
        }
        /** Extract cache*/
        qurl_dbuf_pull_data(core_ptr->recv_cache, head_line_ptr, parsed_len);
        /** Store these header fields if needed.*/
        if (core_ptr->usr_cfg.write_head_to_client_func != QOSA_NULL)
        {
            qurl_slist_t temp_slist = QOSA_NULL;

            temp_slist = qurl_slist_add(core_ptr->cfg.http.http_resp_headers_slist, head_line_ptr);
            if (temp_slist == QOSA_NULL)
            {
                qurl_slist_del_all(core_ptr->cfg.http.http_resp_headers_slist);
                core_ptr->cfg.http.http_resp_headers_slist = QOSA_NULL;
                qurl_safe_free(head_line_ptr);
                return QURL_ECODE_NO_MEMORY;
            }
            core_ptr->cfg.http.http_resp_headers_slist = (qurl_slist_ctrl_t *)temp_slist;
            temp_slist = QOSA_NULL;
            head_line_ptr = QOSA_NULL;
        }

        /*First, check if the parsing is at the end of the response header: the boundary between the response header and the body.*/
        if ((*start_ptr == 0x0a) || (*start_ptr == 0x0d))
        {
            if (*start_ptr == '\r')
            {
                start_ptr++;
            }
            if (*start_ptr == '\n')
            {
                start_ptr++;
            }

            if (http_ctrl_ptr->resp.http_code >= 100 && http_ctrl_ptr->resp.http_code <= 199)
            {
                /*[lzm][todo]User agents may ignore unexpected 1xx status responses.*/
                switch (http_ctrl_ptr->resp.http_code)
                {
                    case 100: {
                        /**
						 * We sent an HTTP PUT or POST request, and this is HTTP/1.1 syntax, indicating that the server has accepted the request and is ready to receive data.
						 * However, now we will receive more response headers, so we need to return to the state of parsing headers!
						 */
                        break;
                    }
                    case 101: {
                        /**
						 * For upgrading the protocol
						 */
                        break;
                    }
                    default:
                        /*Status code 1xx indicates a provisional response, so we will receive another set of headers.*/
                        break;
                }
            }
            else
            {
                /** [lzm][todo]: Check if there is an upgrade requirement during local requests, but no HTTP response status code 1xx indicating upgrade permission was received.
				 * Therefore, it needs to be marked as an upgrade request failure.
				 */
                {
                    ;
                }

                http_ctrl_ptr->header = QOSA_FALSE; /*No more headers to parse*/

                                                    /** [lzm][todo]:
				 * In HTTP 1.1, when the connection is not closed but neither Content-Length nor Transfer-Encoding chunked is received,
				 * According to the provisions of RFC2616 Section 4.4, point 5, we assume that the server will close the connection to indicate the end of the document.
				 */
            }

            if (http_ctrl_ptr->header == QOSA_FALSE) /*No more headers to parse.*/
            {
                ret = http_size(conn_ptr);
                if (ret != QURL_OK)
                {
                    return ret;
                }
            }

            /*[lzm][todo]Here, the necessary response header information that needs to be reported can be implemented and sent to the user side.*/
            /*[lzm][todo]Need to consider whether redirection may cause multiple response headers to be reported.*/
            if (core_ptr->usr_cfg.write_head_to_client_func != QOSA_NULL)
            {
                qurl_bit_t         w_h_flag = QOSA_TRUE;
                long               resp_len = 0;
                long               resp_ret_len = 0;
                qurl_slist_ctrl_t *node = QOSA_NULL;

                for (node = core_ptr->cfg.http.http_resp_headers_slist; node; node = node->next)
                {
                    resp_len = qurl_strlen(node->data);

                    if (w_h_flag && !http_ctrl_ptr->ignore_header)
                    {
                        core_idle_timeout_feed_dog(core_ptr);
                        resp_ret_len
                            = core_ptr->usr_cfg.write_head_to_client_func((unsigned char *)node->data, resp_len, core_ptr->usr_cfg.write_head_to_client_arg);
                        QURL_CORE_IDLE_TIMEOUT_CHECK_DOG_FUNC(core_ptr);
                        if (resp_ret_len < resp_len)
                        {
                            w_h_flag = QOSA_FALSE;
                        }
                    }

                    qurl_safe_free(node->data);
                    node->data = QOSA_NULL;
                }
                qurl_slist_del_all(core_ptr->cfg.http.http_resp_headers_slist);
                core_ptr->cfg.http.http_resp_headers_slist = QOSA_NULL;
                if (w_h_flag == QOSA_FALSE)
                {
                    return QURL_ECODE_WRITE_HEAD_TO_CLIENT_FAILED;
                }
            }

            /*Identity Authentication Check*/
            ret = http_auth_act(conn_ptr);
            if (ret != QURL_OK)
            {
                return ret;
            }

            /*Switch to body state*/
            http_ctrl_ptr->state = QURL_HTTP_STATE_READ_BODY;
            break;
        }

        /*Parse the first line: status line*/
        if (http_ctrl_ptr->header_line == 0)
        {
            int        http_version = 0;
            int        http_code = 0;
            qurl_bit_t fine_statusline = QOSA_FALSE;

            //http_ctrl_ptr->ignore_body = QOSA_FALSE; /* Mark to resume reporting body; body will be blocked later if a valid redirect is parsed */

            ret = http_check_prefix(start_ptr, parsed_len);
            if (ret != QURL_OK)
            {
                /*Header Error*/
                return ret;
            }

            if (conn_ptr->port_handler->protocol & (QURL_PROT_TYPE_HTTP | QURL_PROT_TYPE_HTTPS))
            {
                /**
				 * RFC：https://datatracker.ietf.org/doc/html/rfc7230#section-3.1.2
				 * The above specification describes that the response code must be three digits.
				 * Of course, the future is not necessarily certain.
				 */
                temp_ptr = (char *)conn_ptr->deal_cache_ptr;

                /*Check protocol version and response code*/
                while (*temp_ptr && qurl_ctype_isblank(*temp_ptr))
                {
                    temp_ptr++;
                }
                if (!qurl_strncmp(temp_ptr, "HTTP/", 5))
                {
                    temp_ptr += 5;
                    switch (*temp_ptr)
                    {
                        case '1': {
                            temp_ptr++;
                            if ((temp_ptr[0] == '.') && (temp_ptr[1] == '0' || temp_ptr[1] == '1'))
                            {
                                if (qurl_ctype_isblank(temp_ptr[2]))
                                {
                                    http_version = 10 + (temp_ptr[1] - '0');
                                    temp_ptr += 3;
                                    if (qurl_ctype_isdigit(temp_ptr[0]) && qurl_ctype_isdigit(temp_ptr[1]) && qurl_ctype_isdigit(temp_ptr[2]))
                                    {
                                        http_code = (temp_ptr[0] - '0') * 100 + (temp_ptr[1] - '0') * 10 + (temp_ptr[2] - '0');
                                        temp_ptr += 3;
                                        if (qurl_ctype_isspace(*temp_ptr))
                                        {
                                            fine_statusline = QOSA_TRUE;
                                        }
                                    }
                                }
                            }
                            if (!fine_statusline)
                            {
                                qurl_loge("Unsupported HTTP/1 subversion in response");
                                return QURL_ECODE_HTTP_UNSUPPORTED_PROTOCOL;
                            }
                            break;
                        }
                        case '2':
                        case '3': {
                            if (!qurl_ctype_isblank(temp_ptr[1]))
                            {
                                break;
                            }
                            http_version = (*temp_ptr - '0') * 10;
                            temp_ptr += 2;
                            if (qurl_ctype_isdigit(temp_ptr[0]) && qurl_ctype_isdigit(temp_ptr[1]) && qurl_ctype_isdigit(temp_ptr[2]))
                            {
                                http_code = (temp_ptr[0] - '0') * 100 + (temp_ptr[1] - '0') * 10 + (temp_ptr[2] - '0');
                                temp_ptr += 3;
                                if (!qurl_ctype_isspace(*temp_ptr))
                                {
                                    break;
                                }
                                fine_statusline = QOSA_TRUE;
                            }
                            break;
                        }
                        default: /* unsupported */
                            qurl_loge("Unsupported HTTP version in response");
                            return QURL_ECODE_HTTP_UNSUPPORTED_PROTOCOL;
                    }
                }

                if (fine_statusline)
                {
                    http_ctrl_ptr->resp.http_version = http_version;
                    http_ctrl_ptr->resp.http_code = http_code;
                    core_ptr->info.resp_code = http_code;

                    qurl_logd("http_code:[%d]\r\n", http_ctrl_ptr->resp.http_code);

                    if (http_code < 100)
                    {
                        qurl_loge("Unsupported response code in HTTP response");
                        return QURL_ECODE_HTTP_UNSUPPORTED_PROTOCOL;
                    }

                    switch (http_version)
                    {
                        case 10:
                        case 11:
#ifdef QURL_CFG_ENABLE_HTTP2
                        case 20:
#endif
#ifdef QURL_CFG_ENABLE_QUIC
                        case 30:
#endif
                            break;
                        default:
                            qurl_loge("Unsupported HTTP version (%u.%d) in response", http_version / 10, http_version % 10);
                            return QURL_ECODE_HTTP_UNSUPPORTED_PROTOCOL;
                    }
                }
                else
                {
                    /*Non-standard.*/
                    /*[lzm][todo]Users can also provide aliases for HTTP 200 here.*/
                    ;
                }
            }
            /*else if QURL_PROT_TYPE_RTSP protocol*/

            /*Update data*/

            if (fine_statusline)
            {
                ret = http_http_statusline(conn_ptr);
                if (ret != QURL_OK)
                {
                    return ret;
                }
                http_ctrl_ptr->header_line++;
            }
            else
            {
                http_ctrl_ptr->header = QOSA_FALSE; /*It is not yet an HTTP header line*/
                break;
            }
        }
        else /*http header line*/
        {
            /* Check for Content-Length: header lines to get size */
            if ((http_ctrl_ptr->http_bodyless == QOSA_FALSE) && (core_ptr->usr_cfg.bits.ignore_cl == QOSA_FALSE)
                && http_check_prefix_max("Content-Length:", start_ptr, parsed_len))
            {
                long content_len = 0;
                temp_ptr = start_ptr + qurl_strlen("Content-Length:");
                if (qurl_ctype_isspace(*temp_ptr))
                {
                    temp_ptr++;
                }
                while (*temp_ptr != '\r' && *temp_ptr != '\n' && *temp_ptr != '\0')
                {
                    content_len *= 10;
                    content_len += (*temp_ptr - '0');
                    temp_ptr++;
                }

                http_ctrl_ptr->resp.content_length = content_len;
                core_ptr->info.content_length = content_len;
            }
            /* check for Date: header lines to get the Date info */
            else if (http_check_prefix_max("Date:", start_ptr, parsed_len))
            {
                char *date_ptr = http_copy_header_value(start_ptr);
                if (date_ptr == QOSA_NULL)
                {
                    return QURL_ECODE_NO_MEMORY;
                }
                if (!*date_ptr)
                {
                    qurl_free(date_ptr);
                    date_ptr = QOSA_NULL;
                }
                else
                {
                    if (core_ptr->info.resp_date != QOSA_NULL)
                    {
                        qurl_free(core_ptr->info.resp_date);
                    }
                    core_ptr->info.resp_date = date_ptr;
                }
            }
            /* check for Content-Type: header lines to get the MIME-type */
            else if (http_check_prefix_max("Content-Type:", start_ptr, parsed_len))
            {
                char *content_type_ptr = http_copy_header_value(start_ptr);
                if (content_type_ptr == QOSA_NULL)
                {
                    return QURL_ECODE_NO_MEMORY;
                }
                if (!*content_type_ptr)
                {
                    qurl_free(content_type_ptr);
                    content_type_ptr = QOSA_NULL;
                }
                else
                {
                    if (http_ctrl_ptr->resp.content_type != QOSA_NULL)
                    {
                        qurl_free(http_ctrl_ptr->resp.content_type);
                    }
                    http_ctrl_ptr->resp.content_type = content_type_ptr;
                }
            }
            /* check for Transfer-Encoding: */
            else if (http_check_prefix_max("Transfer-Encoding:", start_ptr, parsed_len))
            {
                /**
				 * One or more encoding methods. We check whether chunked transfer and/or compression algorithms are used.
				 *
				 * Section 3.6.1 of RFC 2616, which details the specification for the "chunked" transfer encoding.
				 * By using "chunked" encoding, the server can transmit the response in multiple data chunks without needing to know the full size of the response in advance.
				 *
				 */

                /*[lzm][note]Currently, this only checks for chunked transfer encoding and outputs RAW data. As for other compression algorithms, development will follow based on demand.*/

                char *transfer_encoding_ptr = http_copy_header_value(start_ptr);
                char *te_ptr = transfer_encoding_ptr;

                if (transfer_encoding_ptr == QOSA_NULL)
                {
                    return QURL_ECODE_NO_MEMORY;
                }
                if (!*transfer_encoding_ptr)
                {
                    qurl_free(transfer_encoding_ptr);
                    transfer_encoding_ptr = QOSA_NULL;
                }
                else
                {
                    while (*te_ptr)
                    {
                        /*The encoding method used by the retrieval server*/
                        /** chunked: chunked transfer*/
                        if (qurl_strncmp("chunked", te_ptr, qurl_strlen(te_ptr)) == 0)
                        {
                            http_ctrl_ptr->chunk = QOSA_TRUE;
                            core_ptr->trans.bits.chunked = QOSA_TRUE;
                            qurl_http_chunked_init(conn_ptr); /*Enable chunked transfer*/
                        }
                        /** Other encodings: To be supported*/

                        /*There may be multiple methods on a single line, so continue reading.*/
                        while (*te_ptr && *te_ptr != ',')
                        {
                            /*Read until the next comma*/
                            te_ptr++;
                        }
                        if (*te_ptr == ',')
                        {
                            /*If the current position is a comma, skip it*/
                            te_ptr++;
                        }
                        while (*te_ptr && qurl_ctype_isspace(*te_ptr))
                        {
                            /*Skip whitespace characters*/
                            te_ptr++;
                        }
                    }

                    if (http_ctrl_ptr->resp.transfer_encoding != QOSA_NULL)
                    {
                        qurl_free(http_ctrl_ptr->resp.transfer_encoding);
                    }
                    http_ctrl_ptr->resp.transfer_encoding = transfer_encoding_ptr;
                }
            }
            /* check for WWW-Authenticate:  */
            else if ((http_ctrl_ptr->resp.http_code == 401) && (http_check_prefix_max("WWW-Authenticate:", start_ptr, parsed_len)))
            {
                char       *www_authenticate_ptr = http_copy_header_value(start_ptr);
                const char *auth_ptr = www_authenticate_ptr;

                if (www_authenticate_ptr == QOSA_NULL)
                {
                    return QURL_ECODE_NO_MEMORY;
                }
                if (!*www_authenticate_ptr)
                {
                    qurl_free(www_authenticate_ptr);
                    www_authenticate_ptr = QOSA_NULL;
                }
                else
                {
                    /**
					 * The code first checks whether a specific single authentication method is required, and if so, initiates the use of that authentication method.
					 */

                    core_ptr->cfg.http.http_auth_recv = QURL_HTTP_AUTH_NONE; /*Clear supported solutions*/
                    while (*auth_ptr)
                    {
                        /*Retrieve supported authentication schemes*/
                        /** Basic */
                        if (qurl_strncmp("Basic", auth_ptr, sizeof("Basic") - 1) == 0 && http_is_valid_auth_separator(auth_ptr[5]))
                        {
                            if (core_ptr->usr_cfg.http.http_auth == QURL_HTTP_AUTH_BASIC)
                            {
                                /*Authentication information has already been sent, but a 401 is still received, indicating that the identity authentication has failed.*/
                                core_ptr->cfg.http.http_auth_recv = QURL_HTTP_AUTH_NONE;
                                core_ptr->cfg.bits.auth_failed = QOSA_TRUE; /*Mark identity authentication failure*/
                            }
                            else
                            {
                                core_ptr->cfg.http.http_auth_recv |= QURL_HTTP_AUTH_BASIC; /*Mark support for this solution*/
                            }
                        }
                        /** Other solutions: To be supported*/

                        /*There may be multiple methods on a single line, so continue reading.*/
                        while (*auth_ptr && *auth_ptr != ',')
                        {
                            /*Read until the next comma*/
                            auth_ptr++;
                        }
                        if (*auth_ptr == ',')
                        {
                            /*If the current position is a comma, skip it*/
                            auth_ptr++;
                        }
                        while (*auth_ptr && qurl_ctype_isspace(*auth_ptr))
                        {
                            /*Skip whitespace characters*/
                            auth_ptr++;
                        }
                    }
                }

                http_ctrl_ptr->resp.www_authenticate = www_authenticate_ptr;
            }
            /* Location： */
            else if ((http_ctrl_ptr->resp.http_code >= 300 && http_ctrl_ptr->resp.http_code < 400) && (http_check_prefix_max("Location:", start_ptr, parsed_len)))
            {
                /*After redirection, the server suggests the URL we should use.*/
                char *location_ptr = http_copy_header_value(start_ptr);

                qurl_logd("location:[%s]\r\n", location_ptr);

                if (location_ptr == QOSA_NULL)
                {
                    return QURL_ECODE_NO_MEMORY;
                }
                if (!*location_ptr)
                {
                    qurl_free(location_ptr);
                    location_ptr = QOSA_NULL;
                }
                else
                {
                    if (http_ctrl_ptr->resp.location != QOSA_NULL)
                    {
                        qurl_free(http_ctrl_ptr->resp.location);
                    }
                    http_ctrl_ptr->resp.location = location_ptr;

                    if (core_ptr->usr_cfg.bits.http_follow_location)
                    {
                        if (core_ptr->cfg.new_url != QOSA_NULL)
                        {
                            qurl_free(core_ptr->cfg.new_url);
                        }
                        core_ptr->cfg.new_url = qurl_strdup(http_ctrl_ptr->resp.location);
                        if (core_ptr->cfg.new_url == QOSA_NULL)
                        {
                            return QURL_ECODE_NO_MEMORY;
                        }

                        core_ptr->cfg.bits.is_follow = QOSA_TRUE; /*Mark that redirection is currently active*/
                        http_ctrl_ptr->ignore_header = QOSA_TRUE; /*This time, do not report the header.*/
                        http_ctrl_ptr->ignore_body = QOSA_TRUE;   /*Do not report body*/
                    }
                }
            }
            /*[lzm][todo]Other HTTP headers to be developed*/
            http_ctrl_ptr->header_line++;
        }
    } while (recv_dc_r_index != recv_dc_w_index);

    return QURL_OK;
}

static qurl_ecode_t http_read_body(qurl_conn_t *conn_ptr)
{
    long              len = 0;
    long              ret_len = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);

    qurl_logd("enter\r\n");

    do {
        /*If the user side consistently fails to receive all data, it may remain stuck in this loop due to user behavior, so a monitoring point can be placed here.*/
        QURL_CORE_CHECK_ABORT_FUNC(core_ptr);

        if (core_ptr->trans.download_len_max > 0)
        {
            len = QURL_MIN(QURL_CONN_DEAL_CACHE_SIZE, (core_ptr->trans.download_len_max - core_ptr->trans.downloaded_len));
        }
        else
        {
            len = QURL_CONN_DEAL_CACHE_SIZE;
        }
        qurl_dbuf_read_data_try(core_ptr->recv_cache, conn_ptr->deal_cache_ptr, &len, 0);

        if (len > 0)
        {
            core_idle_timeout_feed_dog(core_ptr);
            if (core_ptr->usr_cfg.write_to_client_func && !http_ctrl_ptr->ignore_body)
            {
                /** [lzm][note]
				 * Now: When the length returned in the callback function is less than the length passed in, it indicates a failure on the user side call.
				 * Optimization 1: Thanks to the dbuf packet management, we can also change this callback logic: when the user-side return value is:
				 *  ret_len < 0: An error occurred.
				 * 	0 <= ret_len < len: The user has received data normally, but due to limited buffer capacity, not all data could be received. The qurl can retain this unreceived data.
				 *  len <= ret_len: Receive the entire disk.
				 * Optimization 2: Additionally, qurl can append flow control APIs. In the callback, if the user fails to fully receive the data, flow control is automatically triggered, pausing reporting until the user side calls the relevant API to notify the internal process to continue.
				 */
                ret_len = core_ptr->usr_cfg.write_to_client_func(conn_ptr->deal_cache_ptr, len, core_ptr->usr_cfg.write_to_client_arg);
                if (ret_len < len)
                {
                    qurl_loge("write_to_client_func faild:%ld,%ld\r\n", len, ret_len);
                    return QURL_ECODE_CLIENT_WRITE_FAILD;
                }
                core_ptr->trans.client_w_len += len;
            }
            core_ptr->trans.downloaded_len += len;
            QURL_CORE_IDLE_TIMEOUT_CHECK_DOG_FUNC(core_ptr);
        }
        qurl_dbuf_pull_data(core_ptr->recv_cache, QOSA_NULL, len);
    } while (len > 0);

    qurl_logd("download plan:[%ld][%ld]\r\n", core_ptr->trans.download_len_max, core_ptr->trans.downloaded_len);

    return QURL_OK;
}

static qurl_ecode_t http_setup_connect(qurl_conn_t *conn_ptr)
{
    /*1. Create protocol stack operation data structure*/
    /*2. Initialize the protocol operation data structure*/
    /*3. Configure conn according to the current protocol type*/
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);

    qurl_logd("enter\r\n");

    /*1. Create protocol stack operation data structure*/
    qurl_memset(http_ctrl_ptr, 0x00, sizeof(qurl_http_ctrl_t));

    /*2. Initialize the protocol operation data structure*/
    qurl_memset(http_ctrl_ptr->autoid_head_raw_cache, 0xff, sizeof(http_ctrl_ptr->autoid_head_raw_cache));
    http_ctrl_ptr->state = QURL_HTTP_STATE_IDLE; /*Initial state*/
    http_ctrl_ptr->header_line = 0;

    /*3. Configure conn according to the current protocol type*/
    /** Transport Layer Type*/
    if (conn_ptr->core_ptr->usr_cfg.http.http_version == QURL_HTTP_VERSION_3)
    {
        conn_ptr->trans_type = QURL_CONN_TRANS_TYPE_QUIC;
    }
    else
    {
        conn_ptr->trans_type = QURL_CONN_TRANS_TYPE_TCP;
    }

    return QURL_OK;
}

static qurl_ecode_t http_send_prot_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    /*Configure the sending socket to be used*/
    ret = qurl_conn_connect(conn_ptr, QURL_SOCKET_INDEX_0, done_ptr);

    conn_ptr->bits.tls_connecting = !(*done_ptr);

    return ret;
}

static qurl_ecode_t http_prot_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    if (conn_ptr->bits.tls_connecting)
    {
        /*Configure the sending socket to be used*/
        ret = qurl_conn_connect(conn_ptr, QURL_SOCKET_INDEX_0, done_ptr);

        conn_ptr->bits.tls_connecting = !(*done_ptr);
    }
    return ret;
}

/*note: Analyze Curl_http*/
/*This function is executed only once during the request process. The loop execution is for reqing.*/
static qurl_ecode_t http_req(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t      ret = 0;
    long              req_deal_w_index = 0;
    unsigned char    *req_deal_ptr = QOSA_NULL;
    char             *str_ptr = QOSA_NULL;
    const char       *accept_ptr = QOSA_NULL;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);

    qurl_logd("enter\r\n");

    /*Distinguish the HTTP version; if it is HTTP 2.0 or HTTP 3.0, then initialize nghttp2.*/
    /*[lzm][note][http2]Subsequent development*/
    // if(core_ptr->usr_cfg.http.http_version >= QURL_HTTP_VERSION_LAST)
    if (core_ptr->usr_cfg.http.http_version >= QURL_HTTP_VERSION_2_0)
    {
        return QURL_ECODE_HTTP_VER_ERR;
    }

    req_deal_ptr = conn_ptr->deal_cache_ptr;

    http_ctrl_ptr->state = QURL_HTTP_STATE_WRITE_HEADER; /*Since the current function is executed only once in the process, it can be directly updated to the write_header state.*/
    if (core_ptr->cfg.bits.upload_head_raw)
    {
        /*Provided by the user as RAW head*/
        core_ptr->trans.upload_head_len_max = core_ptr->cfg.upload_head_size;
        core_ptr->trans.uploaded_head_len = 0;

        /*Determine the body length*/
        http_req_raw_head_body_headers(conn_ptr);

        /*We traverse and send again in the requesting process.*/
        *done_ptr = QOSA_FALSE;
        return QURL_OK;
    }
    // else
    // {

    /*Determine the request method*/
    http_method(conn_ptr, (const char **)&str_ptr);
    qurl_dbuf_push_data(core_ptr->send_cache, str_ptr, qurl_strlen(str_ptr));
    str_ptr = QOSA_NULL;

    /*Request path*/
    qurl_url_getopt(core_ptr->cfg.uurl_ptr, QURL_URL_OPT_PATH, &str_ptr, QURL_URL_URLENCODE); /*Encoded PATH required*/
    req_deal_w_index += qurl_snprintf((char *)(req_deal_ptr + req_deal_w_index), QURL_CONN_DEAL_CACHE_SIZE - req_deal_w_index, " %s", str_ptr ? str_ptr : "/");
    qurl_dbuf_push_data(core_ptr->send_cache, req_deal_ptr, req_deal_w_index);
    req_deal_w_index = 0;
    if (str_ptr != QOSA_NULL)
    {
        qurl_free(str_ptr);
        str_ptr = QOSA_NULL;
    }
    /*Request URL parameters*/
    qurl_url_getopt(core_ptr->cfg.uurl_ptr, QURL_URL_OPT_QUERY, &str_ptr, QURL_URL_DEFAULT);
    if (str_ptr)
    {
        req_deal_w_index += qurl_snprintf((char *)(req_deal_ptr + req_deal_w_index), QURL_CONN_DEAL_CACHE_SIZE - req_deal_w_index, "?%s", str_ptr);
        qurl_dbuf_push_data(core_ptr->send_cache, req_deal_ptr, req_deal_w_index);
        req_deal_w_index = 0;
        if (str_ptr != QOSA_NULL)
        {
            qurl_free(str_ptr);
            str_ptr = QOSA_NULL;
        }
    }

    /*HTTP version*/
    http_version_str(conn_ptr, (const char **)&str_ptr);
    req_deal_w_index += qurl_snprintf((char *)(req_deal_ptr + req_deal_w_index), QURL_CONN_DEAL_CACHE_SIZE - req_deal_w_index, " HTTP/%s\r\n", str_ptr);
    qurl_dbuf_push_data(core_ptr->send_cache, req_deal_ptr, req_deal_w_index);
    req_deal_w_index = 0;

    /* http head: Host */
    ret = http_req_header_host(conn_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /* http head: User-Agent */
    ret = http_req_header_user_agent(conn_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /* http head: Authorization */
    /*Set authentication header*/
    /*[lzm][todo]It is necessary to determine whether to continue pushing one's own username and password to the redirected address when a redirection occurs. This judgment is to prevent privacy leakage.*/
    /*[lzm][todo]Refer to Curl_http_output_auth for development*/
    /*Temporarily mark as not requiring verification*/
    conn_ptr->bits.authneg = QOSA_FALSE;
    ret = http_req_header_authorization(conn_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /* http head: Referer */
    ret = http_req_header_referer(conn_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /* http head: Accept-Encoding */
    ret = http_req_header_accept_encoding(conn_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /* http body */

    /* http head: Accept */
    accept_ptr = qurl_trans_check_headers(conn_ptr, "Accept") ? QOSA_NULL : "Accept: */*\r\n";
    /*[jamie][eigen][bug][log] s We need to be compatible with this bug in eigen LogPrePass*/
    (void)("*/");
    /* [jamie][eigen][bug][log] e */

    /*HTTP resumable download*/
    ret = http_req_resume(conn_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /* http head: Range */
    ret = http_req_range(conn_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /* http head: TE */

    /* http head: Connection */

    /*form*/

    /* http head: Content-Type */
    /** Content-Type: multipart/form-data */

    /* http head: Transfer-Encoding */

    /* http head: Content-Range */

    /* http head: Alt-Used */

    /* http head: Cookie */

    /*Pack remaining request headers*/
    req_deal_w_index += qurl_snprintf(
        (char *)(req_deal_ptr + req_deal_w_index),
        QURL_CONN_DEAL_CACHE_SIZE - req_deal_w_index,
        "%s" /* host */
        "%s" /* userpwd */
        "%s" /* range */
        "%s" /* user agent */
        "%s" /* accept */
        "%s" /* accept-encoding */
        "%s" /* referer */
        ,
        core_ptr->cfg.host ? core_ptr->cfg.host : "",
        core_ptr->cfg.userpwd ? core_ptr->cfg.userpwd : "",
        core_ptr->cfg.rangeline ? core_ptr->cfg.rangeline : "",
        core_ptr->cfg.uagent ? core_ptr->cfg.uagent : "",
        accept_ptr ? accept_ptr : "",
        core_ptr->cfg.accept_encoding ? core_ptr->cfg.accept_encoding : "",
        core_ptr->cfg.ref ? core_ptr->cfg.ref : ""
    );
    qurl_dbuf_push_data(core_ptr->send_cache, req_deal_ptr, req_deal_w_index);
    req_deal_w_index = 0;

    /*Combined with the user-set request headers*/
    ret = http_req_header_client_add(conn_ptr, core_ptr->send_cache);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /* http body headers */
    http_req_body_headers(conn_ptr);

    /*Request header ok*/
    qurl_dbuf_push_data(core_ptr->send_cache, "\r\n", 2);

    http_ctrl_ptr->state = QURL_HTTP_STATE_WRITE_BODY; /*Switch to write body state*/

    /*Send according to the HTTP protocol*/
    /** Possible http header: Content-Length*/
    /** Possible http header: Expect*/

    /*[202412171919] Fixed issue caused by excessive delay between sending request header and body*/
    /*If the request header has been organized, but there is still a body to send, it is necessary to enter reqing to continue sending.*/
    if (core_ptr->trans.upload_len_max > core_ptr->trans.uploaded_len) /*There is also body data that needs to be uploaded.*/
    {
        *done_ptr = QOSA_FALSE;
    }
    else
    {
        ret = http_send_req(conn_ptr, done_ptr);
        if (ret != QURL_OK)
        {
            goto exit;
        }

        if (*done_ptr)
        {
            http_ctrl_ptr->state = QURL_HTTP_STATE_READ_HEADER; /*Switch to reading HTTP header status*/
            http_ctrl_ptr->header_line = 0;                     /*[202403172000] Fixed the issue where redirection could not be exited.*/
        }
    }

    /*Request completed*/
    ret = QURL_OK;
    // }

exit:
    req_deal_ptr = QOSA_NULL;

    return ret;
}

static qurl_ecode_t http_reqing(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);

    qurl_logd("enter\r\n");

    *done_ptr = QOSA_FALSE;

    if (http_ctrl_ptr->state == QURL_HTTP_STATE_WRITE_HEADER && core_ptr->cfg.bits.upload_head_raw)
    {
        /*Provided by the user as RAW head*/

        ret = http_req_raw_head(conn_ptr);
        if (ret != QURL_OK)
        {
            return ret;
        }

        qurl_logd("upload head plan:[%ld][%ld]\r\n", core_ptr->trans.upload_head_len_max, core_ptr->trans.uploaded_head_len);
        /* head */
        /*[202412171919] Fixed issue caused by excessive delay between sending request header and body*/
        if (core_ptr->trans.upload_head_len_max < 0 || core_ptr->trans.upload_head_len_max > core_ptr->trans.uploaded_head_len) /*There is still head data that needs to be uploaded.*/
        {
            ;
        }
        else
        {
            http_ctrl_ptr->state = QURL_HTTP_STATE_WRITE_BODY; /*Switch to writing HTTP body state*/
        }
    }

    if (http_ctrl_ptr->state != QURL_HTTP_STATE_WRITE_BODY)
    {
        if (http_ctrl_ptr->state > QURL_HTTP_STATE_WRITE_BODY)
        {
            *done_ptr = QOSA_TRUE;
        }
        return QURL_OK;
    }
    /* body */

    ret = http_req_body(conn_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    ret = http_send_req(conn_ptr, done_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    if (*done_ptr)
    {
        qurl_logd("upload plan:[%ld][%ld]\r\n", core_ptr->trans.upload_len_max, core_ptr->trans.uploaded_len);
        /*If the header has been sent, but there is still body data to be sent, it needs to enter the reqing state to continue sending.
[lzm][note][202403272100]Actually, there is no need to worry about upload_len_max<0 here, because HTTP does not support using a callback function returning 0 as the length cutoff point, as HTTP requires content-length, unless it is chunked.*/
        if (core_ptr->trans.upload_len_max < 0 || core_ptr->trans.upload_len_max > core_ptr->trans.uploaded_len) /*There is also body data that needs to be uploaded.*/
        {
            *done_ptr = QOSA_FALSE;
        }
        else
        {
            http_ctrl_ptr->state = QURL_HTTP_STATE_READ_HEADER; /*Switch to reading HTTP header status*/
            http_ctrl_ptr->header_line = 0;                     /*[202403172000] Fixed the issue where redirection could not be exited.*/
        }
    }

    return QURL_OK;
}

/*Reference: Curl_http_readwrite_headers: primarily parses header information, then delivers the body to the user side*/
static qurl_ecode_t http_trans_data(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t      ret = 0;
    long              len = 0;
    qurl_ecode_t      ret_select = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);

    qurl_logd("enter\r\n");

    *done_ptr = QOSA_FALSE;

    /*Check if writable*/
    ret_select = qurl_socket_check(conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd, QURL_SELECT_BIT_READ | QURL_SELECT_BIT_ERROR, 0);
    if (ret_select & QURL_SELECT_BIT_ERROR)
    {
        qurl_loge("select ret:%x\r\n", ret);
        return QURL_ECODE_SOCK_SELECT_FAILED;
    }

    /*Receive and parse data*/
    do {
        /*If there is sustained high-volume inflow, the service will also remain in this loop for processing over an extended period, so monitoring points can be placed.*/
        QURL_CORE_CHECK_ABORT_FUNC(core_ptr);

        ret = qurl_conn_recv_cache(conn_ptr, QURL_SOCKET_INDEX_0, QURL_CONN_DEAL_CACHE_SIZE); /*The second parameter is recommended to be modified.*/
        if (ret != QURL_OK && (qurl_ecode_e)ret != QURL_ECODE_TLS_READ_AGAIN)
        {
            return ret;
        }

        qurl_dbuf_concat_tail(core_ptr->recv_cache, conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].recv_cache);
        /*[lzm][note]Here you can add a check for ret to see if it is due to the limit restriction of dbuf.*/

        len = qurl_dbuf_vaild_size(core_ptr->recv_cache);
        if (len <= 0)
        {
            /* [lab][202405062321] */
            if ((len == 0 && (ret_select & QURL_SELECT_BIT_READ) && (qurl_ecode_e)ret != QURL_ECODE_TLS_READ_AGAIN)
                || conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].bits.rx_closed != QOSA_FALSE)
            {
                /*The connection was disconnected, need to determine if the behavior is normal.*/
                if (http_ctrl_ptr->state != QURL_HTTP_STATE_READ_BODY)
                {
                    /*Even for the HEAD method, the state is marked as BODY after receiving the complete header.*/
                    return QURL_ECODE_SOCK_CONN_CLOSED;
                }
                else
                {
                    if (core_ptr->usr_cfg.bits.opt_nobody)
                    {
                        *done_ptr = QOSA_TRUE;
                        break; /*Normal behavior*/
                    }
                    else if (core_ptr->trans.bits.chunked)
                    {
                        /*Chunked Transfer*/
                        if (conn_ptr->bits.chunked_ok)
                        {
                            /*Transmission complete*/
                            *done_ptr = QOSA_TRUE;
                            qurl_logd("len:[%d]\r\n", core_ptr->trans.downloaded_len);
                            break; /*Behavior normal*/
                        }
                    }
                    else
                    {
                        if ((core_ptr->trans.download_len_max >= 0) && (core_ptr->trans.downloaded_len >= core_ptr->trans.download_len_max))
                        {
                            /*Transmission complete*/
                            *done_ptr = QOSA_TRUE;
                            break; /*Behavior normal*/
                        }
                    }
                }
                return QURL_ECODE_SOCK_CONN_CLOSED;
            }
            return QURL_OK;
        }

        /*Transmission content*/
        if (http_ctrl_ptr->state == QURL_HTTP_STATE_READ_HEADER)
        {
            ret = http_read_headers(conn_ptr);
            if (ret != QURL_OK)
            {
                return ret;
            }
        }
        if (http_ctrl_ptr->state == QURL_HTTP_STATE_READ_BODY)
        {
            if (core_ptr->trans.bits.chunked)
            {
                /*Chunked Transfer*/
                ret = qurl_http_chunked_read_body(conn_ptr);
                if (ret != QURL_OK)
                {
                    return ret;
                }

                if (conn_ptr->bits.chunked_ok)
                {
                    /*Transmission complete*/
                    *done_ptr = QOSA_TRUE;
                    qurl_logd("len:[%d]\r\n", core_ptr->trans.downloaded_len);
                    break;
                }
            }
            else
            {
                ret = http_read_body(conn_ptr);
                if (ret != QURL_OK)
                {
                    return ret;
                }

                if ((core_ptr->trans.download_len_max >= 0) && (core_ptr->trans.downloaded_len >= core_ptr->trans.download_len_max))
                {
                    /*Transmission complete*/
                    *done_ptr = QOSA_TRUE;
                    break;
                }
            }
        }

        ret_select = 0; /*Mark only effective for the first time*/
    } while (1);

    return QURL_OK;
}

static qurl_ecode_t http_trans_done(qurl_conn_t *conn_ptr, qurl_ecode_t ecode, qurl_bit_t premature)
{
    qurl_http_ctrl_t *http_ctrl_ptr = (qurl_http_ctrl_t *)&(conn_ptr->prot_data_u.http_ctrl);
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    (void)(ecode);     /*Temporarily skip compilation warnings*/
    (void)(premature); /*Temporarily skip compilation warnings*/

    qurl_logd("enter\r\n");

    if (http_ctrl_ptr->resp.content_type != QOSA_NULL)
    {
        qurl_free(http_ctrl_ptr->resp.content_type);
        http_ctrl_ptr->resp.content_type = QOSA_NULL;
    }
    if (http_ctrl_ptr->resp.location != QOSA_NULL)
    {
        qurl_free(http_ctrl_ptr->resp.location);
        http_ctrl_ptr->resp.location = QOSA_NULL;
    }

    /*[lzm][note]Place in trans or conn? Or here? Release resources related to HTTP.*/
    qurl_slist_del_all(core_ptr->cfg.http.http_resp_headers_slist);
    core_ptr->cfg.http.http_resp_headers_slist = QOSA_NULL;

    return QURL_OK;
}

static int http_req_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr)
{
    int bitmap = 0; /*Corresponding to the read and write of the monitored sockfd*/

    /*Only need to listen for writable events.*/
    if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd != QURL_SOCKET_BAD)
    {
        socks_ptr[0] = conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd;
        bitmap |= QURL_GETSOCK_WRITESOCK_FUNC(0);
    }

    return bitmap;
}

static int http_trans_data_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr)
{
    int bitmap = 0; /*Corresponding to the read and write of the monitored sockfd*/

    /*Only need to listen for readability.*/
    if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd != QURL_SOCKET_BAD)
    {
        socks_ptr[0] = conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd;
        bitmap |= QURL_GETSOCK_READSOCK_FUNC(0);
    }

    return bitmap;
}

#endif
