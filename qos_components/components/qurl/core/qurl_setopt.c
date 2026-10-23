/** @file         qurl_setopt.c
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-09-06 17:07:28
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qurl_setopt.h"

#include "qurl_platform_port.h"
#include "qurl_http_def.h"
#include "qurl_http.h"

#if (QURL_CFG_ENABLE_FS == 0)
#include "qurl_default_fs.h"
#endif

#if QURL_CFG_ENABLE_HTTP
#include "qurl_http_multiform.h"
#endif

#if QURL_CFG_ENABLE_SMTP
#include "qurl_smtp.h"
#endif

qurl_ecode_t qurl_setopt_reset_for_core(qurl_core_ctrl_t *core_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    /*1. Operating Conditions*/
    if (core_ptr->state != QURL_STATE_INIT)
    {
        qurl_loge("state:[%s]\r\n", core_ptr->state);
        return QURL_ECODE_STATE_INVALID;
    }

    /*2. Various configurations requiring resource release*/
    /* QURL_OPT_URL */
    if (core_ptr->usr_cfg.url_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.url_ptr);
        core_ptr->usr_cfg.url_ptr = QOSA_NULL;
    }

    /* QURL_OPT_USERNAME */
    if (core_ptr->usr_cfg.username_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.username_ptr);
        core_ptr->usr_cfg.username_ptr = QOSA_NULL;
    }

    /* QURL_OPT_PASSWORD */
    if (core_ptr->usr_cfg.password_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.password_ptr);
        core_ptr->usr_cfg.password_ptr = QOSA_NULL;
    }

    /* QURL_OPT_ACCOUNT */
    if (core_ptr->usr_cfg.account_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.account_ptr);
        core_ptr->usr_cfg.account_ptr = QOSA_NULL;
    }
    /* QURL_OPT_NETWORK_ID */
    /* QURL_OPT_PORT */
    /* QURL_OPT_TIMEOUT_MS */
    /* QURL_OPT_IDLE_TIMEOUT_MS */
    /* QURL_OPT_ROUSE_CHECK_TIME_MS */
    /* QURL_OPT_RESUME_FROM */

    /* QURL_OPT_RANGE */
    if (core_ptr->usr_cfg.range_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.range_ptr);
        core_ptr->usr_cfg.range_ptr = QOSA_NULL;
    }

    /* QURL_OPT_REDIRS_CNT_MAX */
    /* QURL_OPT_UPLOAD_HEAD_RAW */
    /* QURL_OPT_WRITE_HEAD_CB */
    /* QURL_OPT_WRITE_HEAD_CB_ARG */
    /* QURL_OPT_WRITE_CB */
    /* QURL_OPT_WRITE_CB_ARG */
    /* QURL_OPT_READ_HEAD_CB */
    /* QURL_OPT_READ_HEAD_CB_ARG */
    /* QURL_OPT_READ_CB */
    /* QURL_OPT_READ_CB_ARG */
    /* QURL_OPT_UPLOAD_HEAD_DATA */
    /* QURL_OPT_UPLOAD_HEAD_SIZE */

    /* QURL_OPT_UPLOAD_HEAD_FILE */
    /*[lzm][note]Should attention be paid to closing the file fd*/
    if (core_ptr->usr_cfg.upload_head_file_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.upload_head_file_ptr);
        core_ptr->usr_cfg.upload_head_file_ptr = QOSA_NULL;
    }

    /* QURL_OPT_UPLOAD_DATA */
    /* QURL_OPT_UPLOAD_SIZE */

    /* QURL_OPT_UPLOAD_FILE */
    /*[lzm][note]Should attention be paid to closing the file fd*/
    if (core_ptr->usr_cfg.upload_file_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.upload_file_ptr);
        core_ptr->usr_cfg.upload_file_ptr = QOSA_NULL;
    }

    /* QURL_OPT_NOBODY */
    /* QURL_OPT_DIRLISTONLY */
    /* QURL_OPT_IGNORE_CONTENT_LENGTH */
    /* QURL_OPT_TRANSFERTEXT */
    /* QURL_OPT_WILDCARDMATCH */
    /* QURL_OPT_FILETIME */
    /* QURL_OPT_TIMEVALUE */
    /* QURL_OPT_TIMECONDITION */
    /* QURL_OPT_MAXFILESIZE */

    /* QURL_OPT_CUSTOMREQUEST */
    if (core_ptr->usr_cfg.customrequest_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.customrequest_ptr);
        core_ptr->usr_cfg.customrequest_ptr = QOSA_NULL;
    }

    /* QURL_OPT_REUSE_HOLD */
    /* QURL_OPT_BOUND_THREAD */
    /* QURL_OPT_BOUND_THREAD_CTRL */
    /* QURL_OPT_TLS_CFG */
    /* QURL_OPT_TLS_USETLS */

#if QURL_CFG_ENABLE_HTTP
    /* QURL_OPT_HTTP_VERSION */
    /* QURL_OPT_HTTP_GET */
    /* QURL_OPT_HTTP_POST */
    /* QURL_OPT_HTTP_POST_FORM */
    /* QURL_OPT_HTTP_PUT_FORM */
    /* QURL_OPT_HTTP_PUT */
    /* QURL_OPT_HTTP_AUTH */
    /* QURL_OPT_FOLLOWLOCATION */
    /* QURL_OPT_UNRESTRICTED_AUTH */
    /* QURL_OPT_AUTOREFERER */
    /* QURL_OPT_POSTREDIR */

    /* QURL_OPT_FORM */
    qurl_http_multiform_delete(&core_ptr->usr_cfg.http.multiform_ctrl);

    /* QURL_OPT_HTTP_HEADER */

    /* QURL_OPT_REFERER */
    if (core_ptr->usr_cfg.referer_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.referer_ptr);
        core_ptr->usr_cfg.referer_ptr = QOSA_NULL;
    }

    /* QURL_OPT_ACCEPT_ENCODING */
    if (core_ptr->usr_cfg.accept_encoding_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.accept_encoding_ptr);
        core_ptr->usr_cfg.accept_encoding_ptr = QOSA_NULL;
    }

    /* QURL_OPT_USER_AGENT */
    if (core_ptr->usr_cfg.user_agent_ptr != QOSA_NULL)
    {
        qurl_free(core_ptr->usr_cfg.user_agent_ptr);
        core_ptr->usr_cfg.user_agent_ptr = QOSA_NULL;
    }
#endif /* QURL_CFG_ENABLE_HTTP */

#if QURL_CFG_ENABLE_FTP
    /* QURL_OPT_FTP_FILEMETHOD */
    /* QURL_OPT_FTP_AUTH */
    /* QURL_OPT_FTP_PORT */
    qurl_safe_free(core_ptr->usr_cfg.ftp.ftp_port_ptr);

    /* QURL_OPT_FTP_USE_EPRT */
    /* QURL_OPT_FTP_USE_EPSV */
    /* QURL_OPT_FTP_USE_PRET */
    /* QURL_OPT_QUOTE */
    /* QURL_OPT_POSTQUOTE */
    /* QURL_OPT_PREQUOTE */
#endif /* QURL_CFG_ENABLE_FTP */
#if QURL_CFG_ENABLE_SMTP
    /* QURL_OPT_SMTP_MAIL_FROM */
    /* QURL_OPT_SMTP_MAIL_RCPT */
    /* QURL_OPT_SMTP_LOGIN_OPTIONS */
    /* QURL_OPT_SMTP_XOAUTH2_BEARER */
    /* QURL_OPT_SMTP_MAILAUTH */
    /* QURL_OPT_SMTP_MAIl_RCPT_ALLOWFAILS */
    qurl_smtp_cfg_delete(&core_ptr->usr_cfg.smtp);
#endif /* QURL_CFG_ENABLE_SMTP */

    /*3. Reset usr_cfg*/
    qurl_memset(&core_ptr->usr_cfg, 0x00, sizeof(qurl_core_usr_cfg_t));

    qurl_logd("reset ok\r\n");

    return ret;
}

qurl_ecode_t qurl_setopt_for_core(qurl_core_ctrl_t *core_ptr, qurl_opt_e opt, va_list param)
{
    qurl_ecode_t ret = QURL_OK;

    if (core_ptr->state != QURL_STATE_INIT
        //Release state restriction section
        && (opt != QURL_OPT_TIMEOUT_MS && opt != QURL_OPT_IDLE_TIMEOUT_MS))
    {
        return QURL_ECODE_STATE_INVALID;
    }

    switch (opt)
    {
        case QURL_OPT_URL: {
            char *url_str = va_arg(param, char *);

            if (core_ptr->usr_cfg.url_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.url_ptr);
                core_ptr->usr_cfg.url_ptr = QOSA_NULL;
            }
            if (url_str != QOSA_NULL && qurl_strlen(url_str) != 0)
            {
                core_ptr->usr_cfg.url_ptr = qurl_malloc(qurl_strlen(url_str) + 1);
                if (core_ptr->usr_cfg.url_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.url_ptr, url_str, qurl_strlen(url_str));
                core_ptr->usr_cfg.url_ptr[qurl_strlen(url_str)] = 0x00;
                qurl_logd("new url:[%s]\r\n", core_ptr->usr_cfg.url_ptr);
            }
            break;
        }
        case QURL_OPT_USERNAME: {
            char *username_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.username_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.username_ptr);
                core_ptr->usr_cfg.username_ptr = QOSA_NULL;
            }
            if (username_ptr != QOSA_NULL && qurl_strlen(username_ptr) != 0)
            {
                core_ptr->usr_cfg.username_ptr = qurl_malloc(qurl_strlen(username_ptr) + 1);
                if (core_ptr->usr_cfg.username_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.username_ptr, username_ptr, qurl_strlen(username_ptr));
                core_ptr->usr_cfg.username_ptr[qurl_strlen(username_ptr)] = 0x00;
                qurl_logd("new username:[%s]\r\n", core_ptr->usr_cfg.username_ptr);
            }
            break;
        }
        case QURL_OPT_PASSWORD: {
            char *password_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.password_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.password_ptr);
                core_ptr->usr_cfg.password_ptr = QOSA_NULL;
            }
            if (password_ptr != QOSA_NULL && qurl_strlen(password_ptr) != 0)
            {
                core_ptr->usr_cfg.password_ptr = qurl_malloc(qurl_strlen(password_ptr) + 1);
                if (core_ptr->usr_cfg.password_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.password_ptr, password_ptr, qurl_strlen(password_ptr));
                core_ptr->usr_cfg.password_ptr[qurl_strlen(password_ptr)] = 0x00;
                qurl_logd("new password:[%s]\r\n", core_ptr->usr_cfg.password_ptr);
            }
            break;
        }
        case QURL_OPT_ACCOUNT: {
            char *account_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.account_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.account_ptr);
                core_ptr->usr_cfg.account_ptr = QOSA_NULL;
            }
            if (account_ptr != QOSA_NULL && qurl_strlen(account_ptr) != 0)
            {
                core_ptr->usr_cfg.account_ptr = qurl_malloc(qurl_strlen(account_ptr) + 1);
                if (core_ptr->usr_cfg.account_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.account_ptr, account_ptr, qurl_strlen(account_ptr));
                core_ptr->usr_cfg.account_ptr[qurl_strlen(account_ptr)] = 0x00;
                qurl_logd("new account:[%s]\r\n", core_ptr->usr_cfg.account_ptr);
            }
            break;
        }
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
        case QURL_OPT_NETWORK_ID: {
            long nw_id = 0;

            nw_id = va_arg(param, long);
            core_ptr->usr_cfg.nw_id = nw_id;
            break;
        }
#endif
        case QURL_OPT_PORT: {
            long port = 0;

            port = va_arg(param, long);
            if ((port < 0) || (port > 65535))
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.port = port;
            break;
        }
        case QURL_OPT_TIMEOUT_MS: {
            long timeout = 0;

            timeout = va_arg(param, long);
            if (timeout < 0)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.timeout_ms = timeout;
            break;
        }
        case QURL_OPT_IDLE_TIMEOUT_MS: {
            long timeout = 0;

            timeout = va_arg(param, long);
            if (timeout < 0)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.idle_timeout_ms = timeout;
            break;
        }
        case QURL_OPT_ROUSE_CHECK_TIME_MS: {
            long time = 0;

            time = va_arg(param, long);
            if (time < 0)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.rouse_check_time_ms = time;
            break;
        }
        case QURL_OPT_RESUME_FROM: {
            long resume_from = 0;

            resume_from = va_arg(param, long);
            if (resume_from < 0)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.resume_from = resume_from;
            break;
        }
        case QURL_OPT_RANGE: {
            char *range_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.range_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.range_ptr);
                core_ptr->usr_cfg.range_ptr = QOSA_NULL;
            }
            if (range_ptr != QOSA_NULL && qurl_strlen(range_ptr) != 0)
            {
                core_ptr->usr_cfg.range_ptr = qurl_malloc(qurl_strlen(range_ptr) + 1);
                if (core_ptr->usr_cfg.range_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.range_ptr, range_ptr, qurl_strlen(range_ptr));
                core_ptr->usr_cfg.range_ptr[qurl_strlen(range_ptr)] = 0x00;
                qurl_logd("new Range:[%s]\r\n", core_ptr->usr_cfg.range_ptr);
            }
            break;
        }
        case QURL_OPT_REDIRS_CNT_MAX: {
            long redirs_cnt_max = 0;

            redirs_cnt_max = va_arg(param, long);
            if (redirs_cnt_max < -1)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.redirs_cnt_max = redirs_cnt_max;
            break;
        }
        case QURL_OPT_UPLOAD_HEAD_RAW: {
            long upload_head_raw = 0;

            upload_head_raw = va_arg(param, long);
            core_ptr->usr_cfg.bits.upload_head_raw = (upload_head_raw != 0) ? QOSA_TRUE : QOSA_FALSE;
            break;
        }
        case QURL_OPT_WRITE_HEAD_CB: {
            core_ptr->usr_cfg.write_head_to_client_func = va_arg(param, qurl_write_head_cb);
            break;
        }
        case QURL_OPT_WRITE_HEAD_CB_ARG: {
            core_ptr->usr_cfg.write_head_to_client_arg = va_arg(param, void *);
            break;
        }
        case QURL_OPT_WRITE_CB: {
            core_ptr->usr_cfg.write_to_client_func = va_arg(param, qurl_write_cb);
            break;
        }
        case QURL_OPT_WRITE_CB_ARG: {
            core_ptr->usr_cfg.write_to_client_arg = va_arg(param, void *);
            break;
        }
        case QURL_OPT_READ_HEAD_CB: {
            core_ptr->usr_cfg.read_head_from_client_func = va_arg(param, qurl_read_head_cb);
            break;
        }
        case QURL_OPT_READ_HEAD_CB_ARG: {
            core_ptr->usr_cfg.read_head_from_client_arg = va_arg(param, void *);
            break;
        }
        case QURL_OPT_READ_CB: {
            core_ptr->usr_cfg.read_from_client_func = va_arg(param, qurl_read_cb);
            break;
        }
        case QURL_OPT_READ_CB_ARG: {
            core_ptr->usr_cfg.read_from_client_arg = va_arg(param, void *);
            break;
        }
        case QURL_OPT_UPLOAD_HEAD_DATA: {
            core_ptr->usr_cfg.upload_head_data_ptr = va_arg(param, void *);
            break;
        }
        case QURL_OPT_UPLOAD_HEAD_SIZE: {
            long size = 0;

            size = va_arg(param, long);
            if (size < -1)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.upload_head_size = size;
            break;
        }
        case QURL_OPT_UPLOAD_HEAD_FILE: {
            char *filename_ptr = va_arg(param, char *);

            /*[lzm][note]Should attention be paid to closing the file fd*/
            if (core_ptr->usr_cfg.upload_head_file_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.upload_head_file_ptr);
                core_ptr->usr_cfg.upload_head_file_ptr = QOSA_NULL;
            }
            if (filename_ptr != QOSA_NULL && qurl_strlen(filename_ptr) != 0)
            {
                core_ptr->usr_cfg.upload_head_file_ptr = qurl_malloc(qurl_strlen(filename_ptr) + 1);
                if (core_ptr->usr_cfg.upload_head_file_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.upload_head_file_ptr, filename_ptr, qurl_strlen(filename_ptr));
                core_ptr->usr_cfg.upload_head_file_ptr[qurl_strlen(filename_ptr)] = 0x00;
                qurl_logd("upload_head_file_ptr:[%s]\r\n", core_ptr->usr_cfg.upload_head_file_ptr);
            }
            break;
        }
        case QURL_OPT_UPLOAD_DATA: {
            core_ptr->usr_cfg.upload_data_ptr = va_arg(param, void *);
            break;
        }
        case QURL_OPT_UPLOAD_SIZE: {
            long size = 0;

            size = va_arg(param, long);
            if (size < -1)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.upload_size = size;
            break;
        }
        case QURL_OPT_UPLOAD_FILE: {
            char *filename_ptr = va_arg(param, char *);

            /*[lzm][note]Should attention be paid to closing the file fd*/
            if (core_ptr->usr_cfg.upload_file_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.upload_file_ptr);
                core_ptr->usr_cfg.upload_file_ptr = QOSA_NULL;
            }
            if (filename_ptr != QOSA_NULL && qurl_strlen(filename_ptr) != 0)
            {
                core_ptr->usr_cfg.upload_file_ptr = qurl_malloc(qurl_strlen(filename_ptr) + 1);
                if (core_ptr->usr_cfg.upload_file_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.upload_file_ptr, filename_ptr, qurl_strlen(filename_ptr));
                core_ptr->usr_cfg.upload_file_ptr[qurl_strlen(filename_ptr)] = 0x00;
                qurl_logd("upload_file_ptr:[%s]\r\n", core_ptr->usr_cfg.upload_file_ptr);
            }
            break;
        }
        case QURL_OPT_NOBODY: {
            long nobody = 0;

            nobody = va_arg(param, long);
            core_ptr->usr_cfg.bits.opt_nobody = (nobody != 0) ? QOSA_TRUE : QOSA_FALSE;

            /*Modify user configuration based on settings*/
#if QURL_CFG_ENABLE_HTTP
            if (core_ptr->usr_cfg.bits.opt_nobody)
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_HEAD;
            }
            else if (core_ptr->usr_cfg.http.http_method == QURL_HTTP_METHOD_HEAD)
            {
                /*HEAD cannot retrieve the body; it needs to be changed to GET.*/
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_GET;
            }
#endif /* QURL_CFG_ENABLE_HTTP */
            break;
        }
        case QURL_OPT_DIRLISTONLY: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.list_only = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;

            break;
        }
        case QURL_OPT_IGNORE_CONTENT_LENGTH: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.ignore_cl = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;

            break;
        }
        case QURL_OPT_TRANSFERTEXT: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.prefer_ascii = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;

            break;
        }
        case QURL_OPT_APPEND: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.remote_append = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;

            break;
        }
        case QURL_OPT_WILDCARDMATCH: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.wildcard_match = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;
            break;
        }
        case QURL_OPT_FILETIME: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.get_filetime = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;
            break;
        }
        case QURL_OPT_TIMEVALUE: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.timevalue = (qurl_time_t)arg;
            break;
        }
        case QURL_OPT_TIMECONDITION: {
            long arg = 0;

            arg = va_arg(param, long);
            if (arg < QURL_TIMECOND_NONE || arg >= QURL_TIMECOND_LAST)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.timecondition = (unsigned char)(qurl_timecond_t)arg;
            break;
        }
        case QURL_OPT_MAXFILESIZE: {
            long arg = 0;

            arg = va_arg(param, long);
            if (arg < 0)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.max_filesize = arg;

            break;
        }
        case QURL_OPT_CUSTOMREQUEST: {
            char *tmp_ptr = va_arg(param, char *);
            long  tmp_len = 0;

            if (core_ptr->usr_cfg.customrequest_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.customrequest_ptr);
                core_ptr->usr_cfg.customrequest_ptr = QOSA_NULL;
            }
            if (tmp_ptr != QOSA_NULL && qurl_strlen(tmp_ptr) > 0)
            {
                tmp_len = qurl_strlen(tmp_ptr);
                core_ptr->usr_cfg.customrequest_ptr = qurl_malloc(tmp_len + 1);
                if (core_ptr->usr_cfg.customrequest_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.customrequest_ptr, tmp_ptr, tmp_len);
                core_ptr->usr_cfg.customrequest_ptr[tmp_len] = 0x00;
            }
            break;
        }
        case QURL_OPT_BOUND_THREAD: {
            long thread_id = 0;

            thread_id = va_arg(param, long);
            if (thread_id == 0)
            {
                thread_id = (long)qurl_get_thread_id();
            }

            core_ptr->usr_cfg.thread_id = (unsigned long)thread_id;

            break;
        }
        case QURL_OPT_BOUND_THREAD_CTRL: {
            long arg = 0;

            arg = va_arg(param, long);
            if (arg < 0 || arg > 2)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }

            switch (arg)
            {
                case 0: {
                    core_ptr->usr_cfg.bits.bound_thread = QOSA_FALSE;
                }
                break;
                case 1: {
                    core_ptr->usr_cfg.bits.bound_thread = QOSA_TRUE;
                    core_ptr->owner_thread_id = core_ptr->usr_cfg.thread_id;
                }
                break;
                case 2: {
                    core_ptr->usr_cfg.bits.bound_thread = QOSA_TRUE;
                    core_ptr->owner_thread_id = core_ptr->create_thread_id;
                }
                break;
            }
            break;
        }
        case QURL_OPT_REUSE_FRESH: {
            long reuse_fresh = 0;

            reuse_fresh = va_arg(param, long);
            core_ptr->usr_cfg.bits.reuse_fresh = (reuse_fresh != 0) ? QOSA_TRUE : QOSA_FALSE;
            break;
        }
        case QURL_OPT_REUSE_FORBID: {
            long reuse_forbid = 0;

            reuse_forbid = va_arg(param, long);
            core_ptr->usr_cfg.bits.reuse_forbid = (reuse_forbid != 0) ? QOSA_TRUE : QOSA_FALSE;
            break;
        }
        case QURL_OPT_REUSE_HOLD: {
            long reuse_hold = 0;

            reuse_hold = va_arg(param, long);
            core_ptr->usr_cfg.bits.reuse_hold = (reuse_hold != 0) ? QOSA_TRUE : QOSA_FALSE;
            break;
        }
        case QURL_OPT_CONN_IDLE_TIMEOUT_MS: {
            long timeout = 0;

            timeout = va_arg(param, long);
            if (timeout < 0)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.conn_idle_timeout_ms = timeout;
            break;
        }
        case QURL_OPT_CONN_MAXLIFETIME_MS: {
            long timeout = 0;

            timeout = va_arg(param, long);
            if (timeout < 0)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.conn_idle_timeout_ms = timeout;
            break;
        }
#if QURL_CFG_PORT_SOCKET_OPT_SO_LINGER
        case QURL_OPT_SOCKET_SO_LINGER: {
            qurl_so_linger_t *so_linger_ptr = QOSA_NULL;

            so_linger_ptr = va_arg(param, qurl_so_linger_t *);
            qurl_memcpy(&core_ptr->usr_cfg.so_linger, so_linger_ptr, sizeof(qurl_so_linger_t));
            break;
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_SO_LINGER */
#if QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE
        case QURL_OPT_SOCKET_SO_KEEPALIVE: {
            long so_keepalive = 0;

            so_keepalive = va_arg(param, long);
            core_ptr->usr_cfg.bits.so_keepalive = (so_keepalive != 0) ? QOSA_TRUE : QOSA_FALSE;
            break;
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE */
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE
        case QURL_OPT_SOCKET_TCP_KEEPIDLE: {
            long tcp_keepidle = 0;

            tcp_keepidle = va_arg(param, long);
            if (tcp_keepidle < 0)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.tcp_keepidle = tcp_keepidle;
            break;
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE */
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL
        case QURL_OPT_SOCKET_TCP_KEEPINTVL: {
            long tcp_keepintvl = 0;

            tcp_keepintvl = va_arg(param, long);
            if (tcp_keepintvl < 0)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.tcp_keepintvl = tcp_keepintvl;
            break;
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL */
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT
        case QURL_OPT_SOCKET_TCP_KEEPCNT: {
            long tcp_keepcnt = 0;

            tcp_keepcnt = va_arg(param, long);
            if (tcp_keepcnt < 0)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.tcp_keepcnt = tcp_keepcnt;
            break;
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT */
#if QURL_CFG_ENABLE_TLS
        case QURL_OPT_TLS_CFG: {
            qurl_tls_cfg_t *tls_cfg_ptr = QOSA_NULL;

            tls_cfg_ptr = va_arg(param, qurl_tls_cfg_t *);
            core_ptr->usr_cfg.tls_cfg_ptr = tls_cfg_ptr;
            break;
        }
        case QURL_OPT_TLS_USETLS: {
            long arg = 0;

            arg = va_arg(param, long);
            if (arg < QURL_USETLS_NONE || arg >= QURL_USETLS_LAST)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.use_tls = arg;
            break;
        }
#endif /* QURL_CFG_ENABLE_TLS */
#if QURL_CFG_ENABLE_HTTP
        case QURL_OPT_HTTP_VERSION: {
            long http_version = 0;

            http_version = va_arg(param, long);
            if (http_version < QURL_HTTP_VERSION_NONE || http_version >= QURL_HTTP_VERSION_LAST)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.http.http_version = http_version;
            break;
        }
        case QURL_OPT_HTTP_GET: {
            long get = 0;

            get = va_arg(param, long);

            /*Modify user configuration based on settings*/
            if (get != 0)
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_GET;
                core_ptr->usr_cfg.bits.opt_nobody = QOSA_FALSE; /*Default receive body*/
            }
            break;
        }
        case QURL_OPT_HTTP_POST: {
            long post = 0;

            post = va_arg(param, long);

            /*Modify user configuration based on settings*/
            if (post != 0)
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_POST;
                core_ptr->usr_cfg.bits.opt_nobody = QOSA_FALSE; /*Default receive body*/
            }
            else
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_GET;
            }
            break;
        }
        case QURL_OPT_HTTP_POST_FORM: {
            long post_form = 0;

            post_form = va_arg(param, long);

            /*Modify user configuration based on settings*/
            if (post_form != 0)
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_POST_FORM;
                core_ptr->usr_cfg.bits.opt_nobody = QOSA_FALSE; /*Default receive body*/
            }
            else
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_GET;
            }
            break;
        }
        case QURL_OPT_HTTP_PUT_FORM: {
            long put_form = 0;

            put_form = va_arg(param, long);

            /*Modify user configuration based on settings*/
            if (put_form != 0)
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_PUT_FORM;
                core_ptr->usr_cfg.bits.opt_nobody = QOSA_FALSE; /*Default receive body*/
            }
            else
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_GET;
            }
            break;
        }
        case QURL_OPT_HTTP_PATCH_FORM: {
            long patch_form = 0;

            patch_form = va_arg(param, long);

            /*Modify user configuration based on settings*/
            if (patch_form != 0)
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_PATCH_FORM;
                core_ptr->usr_cfg.bits.opt_nobody = QOSA_FALSE; /*Default receive body*/
            }
            else
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_GET;
            }
            break;
        }
        case QURL_OPT_HTTP_PUT: {
            long get = 0;

            get = va_arg(param, long);

            /*Modify user configuration based on settings*/
            if (get != 0)
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_PUT;
                core_ptr->usr_cfg.bits.opt_nobody = QOSA_FALSE; /*Default receive body*/
            }
            break;
        }
        case QURL_OPT_HTTP_PATCH: {
            long patch = 0;

            patch = va_arg(param, long);

            /*Modify user configuration based on settings*/
            if (patch != 0)
            {
                core_ptr->usr_cfg.http.http_method = QURL_HTTP_METHOD_PATCH;
                core_ptr->usr_cfg.bits.opt_nobody = QOSA_FALSE; /*Default receive body*/
            }
            break;
        }
        case QURL_OPT_HTTP_AUTH: {
            unsigned long http_auth = 0;

            http_auth = va_arg(param, unsigned long);

            if (http_auth == QURL_HTTP_AUTH_NONE)
            {
                core_ptr->usr_cfg.http.http_auth = QURL_HTTP_AUTH_NONE;
                break;
            }

            if (!(http_auth & QURL_HTTP_AUTH_ALL))
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }

            core_ptr->usr_cfg.http.http_auth = http_auth;

            break;
        }
        case QURL_OPT_FOLLOWLOCATION: {
            long follow_location = 0;

            follow_location = va_arg(param, long);
            core_ptr->usr_cfg.bits.http_follow_location = (follow_location != 0) ? QOSA_TRUE : QOSA_FALSE;
            break;
        }
        case QURL_OPT_UNRESTRICTED_AUTH: {
            long allow_auth_to_other_hosts = 0;

            allow_auth_to_other_hosts = va_arg(param, long);
            core_ptr->usr_cfg.bits.allow_auth_to_other_hosts = (allow_auth_to_other_hosts != 0) ? QOSA_TRUE : QOSA_FALSE;
            break;
        }
        case QURL_OPT_AUTOREFERER: {
            long auto_referer = 0;

            auto_referer = va_arg(param, long);
            core_ptr->usr_cfg.bits.http_auto_referer = (auto_referer != 0) ? QOSA_TRUE : QOSA_FALSE;
            break;
        }
        case QURL_OPT_POSTREDIR: {
            long keep_post = 0;

            keep_post = va_arg(param, long);
            if (keep_post < QURL_HTTP_REDIR_POST_ALL)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.http.keep_post = keep_post & QURL_HTTP_REDIR_POST_ALL;
            break;
        }
        case QURL_OPT_FORM: {
            unsigned char         id = 0;
            qurl_http_form_cfg_t *form_cfg_ptr = NULL;

            id = (unsigned char)(va_arg(param, long));
            form_cfg_ptr = va_arg(param, qurl_http_form_cfg_t *);

            ret = qurl_http_multiform_set_form(&core_ptr->usr_cfg.http.multiform_ctrl, id, form_cfg_ptr);
            break;
        }
        case QURL_OPT_HTTP_HEADER: {
            core_ptr->usr_cfg.http.http_headers_slist = va_arg(param, qurl_slist_t);
            break;
        }
        case QURL_OPT_REFERER: {
            char *referer_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.referer_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.referer_ptr);
                core_ptr->usr_cfg.referer_ptr = QOSA_NULL;
            }
            if (referer_ptr != QOSA_NULL && qurl_strlen(referer_ptr) != 0)
            {
                core_ptr->usr_cfg.referer_ptr = qurl_malloc(qurl_strlen(referer_ptr) + 1);
                if (core_ptr->usr_cfg.referer_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.referer_ptr, referer_ptr, qurl_strlen(referer_ptr));
                core_ptr->usr_cfg.referer_ptr[qurl_strlen(referer_ptr)] = 0x00;
                core_ptr->cfg.referer_ptr = core_ptr->usr_cfg.referer_ptr; /*Directly for cfg use*/
                core_ptr->cfg.bits.referer_alloc_flag = QOSA_FALSE;
                qurl_logd("new Referer:[%s]\r\n", core_ptr->usr_cfg.referer_ptr);
            }
            break;
        }
        case QURL_OPT_ACCEPT_ENCODING: {
            char *ae_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.accept_encoding_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.accept_encoding_ptr);
                core_ptr->usr_cfg.accept_encoding_ptr = QOSA_NULL;
            }
            if (ae_ptr != QOSA_NULL && qurl_strlen(ae_ptr) != 0)
            {
                core_ptr->usr_cfg.accept_encoding_ptr = qurl_malloc(qurl_strlen(ae_ptr) + 1);
                if (core_ptr->usr_cfg.accept_encoding_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.accept_encoding_ptr, ae_ptr, qurl_strlen(ae_ptr));
                core_ptr->usr_cfg.accept_encoding_ptr[qurl_strlen(ae_ptr)] = 0x00;
                qurl_logd("new Accept-Encoding:[%s]\r\n", core_ptr->usr_cfg.accept_encoding_ptr);
            }
            break;
        }
        case QURL_OPT_USER_AGENT: {
            char *ua_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.user_agent_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.user_agent_ptr);
                core_ptr->usr_cfg.user_agent_ptr = QOSA_NULL;
            }
            if (ua_ptr != QOSA_NULL && qurl_strlen(ua_ptr) != 0)
            {
                core_ptr->usr_cfg.user_agent_ptr = qurl_malloc(qurl_strlen(ua_ptr) + 1);
                if (core_ptr->usr_cfg.user_agent_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.user_agent_ptr, ua_ptr, qurl_strlen(ua_ptr));
                core_ptr->usr_cfg.user_agent_ptr[qurl_strlen(ua_ptr)] = 0x00;
                qurl_logd("new User-Agent:[%s]\r\n", core_ptr->usr_cfg.user_agent_ptr);
            }
            break;
        }
#endif /* QURL_CFG_ENABLE_HTTP */
#if QURL_CFG_ENABLE_FTP
        case QURL_OPT_FTP_FILEMETHOD: {
            long arg = 0;

            arg = va_arg(param, long);
            if (arg < QURL_FTP_FILEMETHOD_MULTICWD || arg > QURL_FTP_FILEMETHOD_SINGLECWD)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.ftp.ftp_filemethod = (unsigned char)(qurl_ftp_filemethod_e)arg;
            break;
        }
        case QURL_OPT_FTP_AUTH: {
            long arg = 0;

            arg = va_arg(param, long);
            if (arg < QURL_FTP_AUTH_DEFAULT || arg >= QURL_FTP_AUTH_LAST)
            {
                ret = QURL_ECODE_PARAM_INVALID;
                break;
            }
            core_ptr->usr_cfg.ftp.ftp_auth = (unsigned char)(qurl_ftp_auth_e)arg;
            break;
        }
        case QURL_OPT_FTP_SKIP_PASV_IP: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.ftp_skip_ip = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;

            break;
        }
        case QURL_OPT_FTP_PORT: {
            char *arg = 0;
            int   len = 0;

            qurl_safe_free(core_ptr->usr_cfg.ftp.ftp_port_ptr);

            arg = va_arg(param, char *);
            if (arg != QOSA_NULL)
            {
                len = qurl_strlen(arg);
                core_ptr->usr_cfg.ftp.ftp_port_ptr = qurl_malloc(len + 1);
                if (core_ptr->usr_cfg.ftp.ftp_port_ptr == QOSA_NULL)
                {
                    core_ptr->usr_cfg.bits.ftp_use_port = QOSA_FALSE;
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                core_ptr->usr_cfg.ftp.ftp_port_ptr[len] = 0x00;
                qurl_memcpy(core_ptr->usr_cfg.ftp.ftp_port_ptr, arg, len);
            }
            core_ptr->usr_cfg.bits.ftp_use_port = (core_ptr->usr_cfg.ftp.ftp_port_ptr != QOSA_NULL) ? QOSA_TRUE : QOSA_FALSE;

            break;
        }
        case QURL_OPT_FTP_USE_EPRT: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.ftp_use_eprt = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;

            break;
        }
        case QURL_OPT_FTP_USE_EPSV: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.ftp_use_epsv = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;

            break;
        }
        case QURL_OPT_FTP_USE_PRET: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.ftp_use_pret = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;

            break;
        }
        case QURL_OPT_QUOTE: {
            core_ptr->usr_cfg.ftp.quote_slist = va_arg(param, qurl_slist_t);
            break;
        }
        case QURL_OPT_POSTQUOTE: {
            core_ptr->usr_cfg.ftp.postquote_slist = va_arg(param, qurl_slist_t);
            break;
        }
        case QURL_OPT_PREQUOTE: {
            core_ptr->usr_cfg.ftp.prequote_slist = va_arg(param, qurl_slist_t);
            break;
        }
#endif /* QURL_CFG_ENABLE_FTP */
#if QURL_CFG_ENABLE_SMTP
        case QURL_OPT_SMTP_MAIL_FROM: {
            char *mail_from_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.smtp.mail_from_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.smtp.mail_from_ptr);
                core_ptr->usr_cfg.smtp.mail_from_ptr = QOSA_NULL;
            }
            if (mail_from_ptr != QOSA_NULL && qurl_strlen(mail_from_ptr) != 0)
            {
                core_ptr->usr_cfg.smtp.mail_from_ptr = qurl_malloc(qurl_strlen(mail_from_ptr) + 1);
                if (core_ptr->usr_cfg.smtp.mail_from_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.smtp.mail_from_ptr, mail_from_ptr, qurl_strlen(mail_from_ptr));
                core_ptr->usr_cfg.smtp.mail_from_ptr[qurl_strlen(mail_from_ptr)] = 0x00;
                qurl_logd("new account:[%s]\r\n", core_ptr->usr_cfg.smtp.mail_from_ptr);
            }

            break;
        }
        case QURL_OPT_SMTP_MAIL_RCPT: {
            core_ptr->usr_cfg.smtp.rtcp_slist = va_arg(param, qurl_slist_t);
            break;
        }
        case QURL_OPT_SMTP_LOGIN_OPTIONS: {
            char *mail_login_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.smtp.mail_login_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.smtp.mail_login_ptr);
                core_ptr->usr_cfg.smtp.mail_login_ptr = QOSA_NULL;
            }
            if (mail_login_ptr != QOSA_NULL && qurl_strlen(mail_login_ptr) != 0)
            {
                core_ptr->usr_cfg.smtp.mail_login_ptr = qurl_malloc(qurl_strlen(mail_login_ptr) + 1);
                if (core_ptr->usr_cfg.smtp.mail_login_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.smtp.mail_login_ptr, mail_login_ptr, qurl_strlen(mail_login_ptr));
                core_ptr->usr_cfg.smtp.mail_login_ptr[qurl_strlen(mail_login_ptr)] = 0x00;
                qurl_logd("new account:[%s]\r\n", core_ptr->usr_cfg.smtp.mail_login_ptr);
            }

            break;
        }
        case QURL_OPT_SMTP_XOAUTH2_BEARER: {
            char *mail_xoauth2_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.smtp.mail_xoauth2_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.smtp.mail_xoauth2_ptr);
                core_ptr->usr_cfg.smtp.mail_xoauth2_ptr = QOSA_NULL;
            }
            if (mail_xoauth2_ptr != QOSA_NULL && qurl_strlen(mail_xoauth2_ptr) != 0)
            {
                core_ptr->usr_cfg.smtp.mail_xoauth2_ptr = qurl_malloc(qurl_strlen(mail_xoauth2_ptr) + 1);
                if (core_ptr->usr_cfg.smtp.mail_xoauth2_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.smtp.mail_xoauth2_ptr, mail_xoauth2_ptr, qurl_strlen(mail_xoauth2_ptr));
                core_ptr->usr_cfg.smtp.mail_xoauth2_ptr[qurl_strlen(mail_xoauth2_ptr)] = 0x00;
                qurl_logd("new account:[%s]\r\n", core_ptr->usr_cfg.smtp.mail_xoauth2_ptr);
            }

            break;
        }
        case QURL_OPT_SMTP_MAILAUTH: {
            char *mail_auth_ptr = va_arg(param, char *);

            if (core_ptr->usr_cfg.smtp.mail_auth_ptr != QOSA_NULL)
            {
                qurl_free(core_ptr->usr_cfg.smtp.mail_auth_ptr);
                core_ptr->usr_cfg.smtp.mail_auth_ptr = QOSA_NULL;
            }
            if (mail_auth_ptr != QOSA_NULL && qurl_strlen(mail_auth_ptr) != 0)
            {
                core_ptr->usr_cfg.smtp.mail_auth_ptr = qurl_malloc(qurl_strlen(mail_auth_ptr) + 1);
                if (core_ptr->usr_cfg.smtp.mail_auth_ptr == QOSA_NULL)
                {
                    ret = QURL_ECODE_NO_MEMORY;
                    break;
                }
                qurl_memcpy(core_ptr->usr_cfg.smtp.mail_auth_ptr, mail_auth_ptr, qurl_strlen(mail_auth_ptr));
                core_ptr->usr_cfg.smtp.mail_auth_ptr[qurl_strlen(mail_auth_ptr)] = 0x00;
                qurl_logd("new account:[%s]\r\n", core_ptr->usr_cfg.smtp.mail_auth_ptr);
            }

            break;
        }
        case QURL_OPT_SMTP_MAIl_RCPT_ALLOWFAILS: {
            long arg = 0;

            arg = va_arg(param, long);
            core_ptr->usr_cfg.bits.mail_rcpt_allowfails = (arg != 0) ? QOSA_TRUE : QOSA_FALSE;

            break;
        }

#endif
        default:
            ret = QURL_ECODE_PARAM_INVALID;
            break;
    }

    qurl_logd("opt:[%x],ret:[%x]\r\n", opt, ret);
    return ret;
}
