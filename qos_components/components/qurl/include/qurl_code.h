/** @file         qurl_code.h
 *  @brief        Brief description
 *  @details      Detailed description
 *  @author       lzm
 *  @date         2023-10-19 15:52:48
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Modification log:
 **********************************************************
 */

#ifndef QURL_CODE_H
#define QURL_CODE_H

#include "qosa_def.h"

#define QURL_CFG_ECODE_BASIC_NUM (0x80010000)

/**
 * @enum  qurl_ecode_e
 * @brief qurl global error codes
 */
typedef enum
{
    QURL_OK = 0,
    QURL_ECODE_OK = 0,

    QURL_ECODE_FAILURE = (1 | QURL_CFG_ECODE_BASIC_NUM | 0x80000000),
    QURL_ECODE_PARAM_INVALID,
    QURL_ECODE_NO_MEMORY,
    QURL_ECODE_NO_PERMISSION,
    QURL_ECODE_OVER_LIMIT,
    QURL_ECODE_BUF_TOO_SMALL,
    QURL_ECODE_NO_URL,
    QURL_ECODE_NO_DATA_SEND,
    QURL_ECODE_PERFORM_TIMEOUT,
    QURL_ECODE_LOCK_TIMEOUT,
    QURL_ECODE_EVENT_TIMEOUT,
    QURL_ECODE_TLS_BAD_CONTENT_ENCODING,
    QURL_ECODE_TRANS_TOO_MANY_REDIRECTS, /*!< Too many redirects */
    QURL_ECODE_CLIENT_WRITE_FAILD,       /*!< Failed to write data to upper layer user */
    QURL_ECODE_PROT_NO_MATCH,            /*!< No matching protocol interface */
    QURL_ECODE_NO_SUPPORT,               /*!< Function or configuration not supported, such as unsupported protocol */
    QURL_ECODE_NOT_FIND,
    QURL_ECODE_UNKNOWN_OPTION,
    QURL_ECODE_FILESIZE_EXCEEDED,
    QURL_ECODE_DOWNLOAD_RESUME_ERR, /*!< Resume transfer error */
    QURL_ECODE_UPLOAD_ERR,          /*!< Upload time error */
    QURL_ECODE_RANGE_ERR,           /*!< Range error */
    QURL_ECODE_WRITE_HEAD_TO_CLIENT_FAILED,
    QURL_ECODE_NETWORK_ERR,         /*!< Network error */
    QURL_ECODE_STATE_INVALID,       /*!< Invalid state */
    QURL_ECODE_ABORT,               /*!< Aborted */

    QURL_ECODE_THREAD_CREATE_FAILED = (QURL_ECODE_FAILURE + 0x0020),
    QURL_ECODE_LOCK_CREATE_FAILD,
    QURL_ECODE_LOCK_DELETE_FAILD,
    QURL_ECODE_LOCK_LOCK_FAILD,
    QURL_ECODE_LOCK_UNLOCK_FAILD,
    QURL_ECODE_EVENT_FAILD,

    QURL_ECODE_CONN_CONNECT_FAILD = (QURL_ECODE_FAILURE + 0x0030),
    QURL_ECODE_CONN_CACHE_NOT_MATCH,
    QURL_ECODE_CONN_ALREADY_ATTACH,
    QURL_ECODE_CONN_NOT_ATTACH,
    QURL_ECODE_CONN_NOT_RESOLVE,
    QURL_ECODE_CONN_NOT_FOUND_IP,
    QURL_ECODE_CONN_CONNECT_FAILED,
    QURL_ECODE_CONN_FAMILY_NOT_SUPPORTED,
    QURL_ECODE_CONN_NOT_FOUND_SOCKFD,
    QURL_ECODE_CONN_SEND_FAILD,

    QURL_ECODE_URL_MALFORMED = (QURL_ECODE_FAILURE + 0x0040),
    QURL_ECODE_URL_MALFORMED_INPUT,
    QURL_ECODE_URL_TOO_LONG,
    QURL_ECODE_URL_BAD_PORT_NUMBER,
    QURL_ECODE_URL_USER_NOT_ALLOWED,
    QURL_ECODE_URL_NO_SCHEME,
    QURL_ECODE_URL_NO_USER,
    QURL_ECODE_URL_NO_PASSWORD,
    QURL_ECODE_URL_NO_OPTIONS,
    QURL_ECODE_URL_NO_HOST,
    QURL_ECODE_URL_NO_PORT,
    QURL_ECODE_URL_NO_QUERY,
    QURL_ECODE_URL_NO_FRAGMENT,
    QURL_ECODE_URL_DECODE,

    QURL_ECODE_SOCK_SOCK_FAILED = (QURL_ECODE_FAILURE + 0x0050),
    QURL_ECODE_SOCK_BIND_FAILED,
    QURL_ECODE_SOCK_CTRL_FAILED,
    QURL_ECODE_SOCK_LISTEN_FAILED,
    QURL_ECODE_SOCK_ACCEPT_FAILED,
    QURL_ECODE_SOCK_CONNECT_FAILED,
    QURL_ECODE_SOCK_SELECT_FAILED,
    QURL_ECODE_SOCK_WRITE_FAILED,
    QURL_ECODE_SOCK_READ_FAILED,
    QURL_ECODE_SOCK_CLOSE_FAILED,
    QURL_ECODE_SOCK_CONN_CLOSED, /*!< Connection closed */
    QURL_ECODE_SOCK_SETOPT_FAILED,

    QURL_ECODE_FS_OPEN_ERR = (QURL_ECODE_FAILURE + 0x0060), /*!< File open error */
    QURL_ECODE_FS_READ_ERR,

    QURL_ECODE_PP_ERR = (QURL_ECODE_FAILURE + 0x0070),          /*!< Pingpong error */
    QURL_ECODE_PP_TIMEOUT,                                      /*!< Pingpong timeout */

    QURL_ECODE_TLS_INIT_FAILED = (QURL_ECODE_FAILURE + 0x0080), /*!< TLS initialization failed */
    QURL_ECODE_TLS_CREATE_FAILED,                               /*!< TLS creation failed */
    QURL_ECODE_TLS_READ_ERR,                                    /*!< TLS read error */
    QURL_ECODE_TLS_WRITE_ERR,                                   /*!< TLS write error */
    QURL_ECODE_TLS_WRITE_LEN_ERR,                               /*!< TLS write data length error */
    QURL_ECODE_TLS_READ_AGAIN,                                  /*!< More data needed to continue reading. E.g., block decryption. */
    QURL_ECODE_TLS_WRITE_AGAIN,                                 /*!< More space needed to write and send. E.g., block decryption. */
    QURL_ECODE_TLS_NEGOTIATE_ERR,                               /*!< Handshake error. */
    QURL_ECODE_TLS_NEGOTIATE_TIMEOUT,                           /*!< Handshake timeout. */
    QURL_ECODE_TLS_RANDOM_ERR,                                  /*!< Random number error. */
    QURL_ECODE_TLS_CONFIG_ERR,                                  /*!< Configuration error. */
    QURL_ECODE_TLS_CONNECT_ERR,                                 /*!< Connection error. */
    QURL_ECODE_TLS_GET_SESSION_FAILED,                          /*!< Failed to get SESSION */
    QURL_ECODE_TLS_SET_SESSION_FAILED,                          /*!< Failed to set SESSION */
    QURL_ECODE_TLS_PEER_CLOSE_NOTIFY,                           /*!< Notification ready to close, normal result code */

    QURL_ECODE_HTTP_VER_ERR = (QURL_ECODE_FAILURE + 0x0090),    /*!< HTTP version error */
    QURL_ECODE_HTTP_METHOD_ERR,                                 /*!< HTTP request method error */
    QURL_ECODE_HTTP_RESP_HEADER_ERR,                            /*!< HTTP response header error */
    QURL_ECODE_HTTP_UNSUPPORTED_PROTOCOL,                       /*!< HTTP response unsupported protocol */
    QURL_ECODE_HTTP_AUTH_FAILED,                                /*!< HTTP authentication failed */
    QURL_ECODE_HTTP_CHUNKED_BAD,                                /*!< Chunked error */
    QURL_ECODE_HTTP_CHUNKED_HEX_TOO_LONG,                       /*!< Chunked hex field too long */
    QURL_ECODE_HTTP_CHUNKED_HEX_ILLEGAL,                        /*!< Chunked hex field illegal */
    QURL_ECODE_HTTP_MULTIFORM_ERR,                              /*!< Multi-form operation exception */
    QURL_ECODE_HTTP_MULTIFORM_STATE_ERR,                        /*!< Multi-form state machine operation exception */

    QURL_ECODE_FTP_ERR = (QURL_ECODE_FAILURE + 0x00A0),         /*!< General error */
    QURL_ECODE_FTP_WEIRD_SERVER_REPLY,                          /*!< Received abnormal or unparsable server reply */
    QURL_ECODE_FTP_LOGIN_DENIED,                                /*!< Login failed, check username, password and account information */
    QURL_ECODE_FTP_REMOTE_ACCESS_DENIED, /*!< Server denies access to a service due to lack of permissions. This error code is usually not returned when login fails. */
    QURL_ECODE_FTP_REMOTE_FILE_NOT_FOUND, /*!< Remote file not found */
    QURL_ECODE_FTP_COULDNT_USE_REST,      /*!< Cannot use REST */
    QURL_ECODE_FTP_WEIRD_PASV_REPLY,      /*!< PASV response failed */
    QURL_ECODE_FTP_WEIRD_227_FORMAT,      /*!< 227 response code format error */
    QURL_ECODE_FTP_CMD_QUOTE_ERR,         /*!< QUOTE command failed */
    QURL_ECODE_FTP_CMD_TYPE_ERR,          /*!< TYPE command failed */
    QURL_ECODE_FTP_CMD_PRET_ERR,          /*!< PRET command failed */
    QURL_ECODE_FTP_CMD_PROT_ERR,          /*!< PROT command failed */
    QURL_ECODE_FTP_CANT_GET_HOST,         /*!< Cannot get hostname */
    QURL_ECODE_FTP_COULDNT_RETR_FILE,     /*!< Cannot retrieve file from FTP server. */
    QURL_ECODE_FTP_REMOTE_DISK_FULL,      /*!< Server disk space insufficient. */
    QURL_ECODE_FTP_PARTIAL_FILE, /*!< Only part of the file was downloaded or uploaded. This may be due to network interruption or other issues during transmission. */
    QURL_ECODE_FTP_ACCEPT_ERR,                           /*!< Active mode data connection accept failed */
    QURL_ECODE_FTP_QUOTE_ERR,                            /*!< QUOTE send failed */

    QURL_ECODE_SMTP_ERR = (QURL_ECODE_FAILURE + 0x00C0), /*!< General error */
    QURL_ECODE_SMTP_URL_MALFORMAT,                       /*!< Received abnormal or unparsable server reply */
    QURL_ECODE_SMTP_AUTH_FAILED,
    QURL_ECODE_SMTP_UNSPPORTED_PROTOCOL,
    QURL_ECODE_SMTP_WEIRD_SERVER_REPLY,
    QURL_ECODE_SMTP_REMOTE_ACCESS_DENIED,
    QURL_ECODE_SMTP_LOGIN_DENIED,
    QURL_ECODE_SMTP_RECV_ERROR,
    QURL_ECODE_SMTP_SEND_ERROR,

} qurl_ecode_e;

/**
 * @enum  qurl_opt_e
 * @brief qurl configuration options
 */
typedef enum
{
    QURL_OPT_URL = 0x1000,        /*!< URL */
    QURL_OPT_USERNAME,            /*!< Username: "user" */
    QURL_OPT_PASSWORD,            /*!< Password: "password" */
    QURL_OPT_ACCOUNT,             /*!< Account: "xxx" */
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    QURL_OPT_NETWORK_ID,          /*!< User specified network id. <0 means not specified. >=0: refer to platform adaptation instructions. */
#endif
    QURL_OPT_PORT,                /*!< User specified port number */
    QURL_OPT_TIMEOUT_MS,          /*!< Overall operation timeout */
    QURL_OPT_IDLE_TIMEOUT_MS,     /*!< Internal idle timeout during operation */
    QURL_OPT_ROUSE_CHECK_TIME_MS, /*!< Wake-up check time. Default 1000ms. Mainly used for long-time blocking, will wake up periodically according to this time value to check other events, such as checking if events triggered by qurl_core_abort() are received. */
    QURL_OPT_RESUME_FROM,         /*!< Resume transfer at specified offset, unit: byte. When <0, means calculate from the end backwards. */
    QURL_OPT_RANGE,               /*!< Download range: such as setting HTTP Range string; such as ftp specifying range */
    QURL_OPT_REDIRS_CNT_MAX,      /*!< Maximum number of redirects to follow */
    QURL_OPT_UPLOAD_HEAD_RAW, /*!< Mark upload HEAD raw data, no internal synthesis required. Internal will get from HEAD_DATA/HEAD_FILE/HEAD_CB, if none of the three are set, it will not throw an exception, this will be judged as getting HEAD and BODY together from DATA/FILE/CB. */
    QURL_OPT_WRITE_HEAD_CB,   /*!< qurl   --> client response header callback function, type: qurl_write_head_cb */
    QURL_OPT_WRITE_HEAD_CB_ARG,     /*!< qurl   --> client response header callback function parameter, type: void * */
    QURL_OPT_WRITE_CB,              /*!< qurl   --> client callback function, type: qurl_write_cb */
    QURL_OPT_WRITE_CB_ARG,          /*!< qurl   --> client callback function parameter, type: void * */
    QURL_OPT_READ_HEAD_CB,          /*!< client --> qurl request header callback function, type: qurl_read_head_cb */
    QURL_OPT_READ_HEAD_CB_ARG,      /*!< client --> qurl request header callback function parameter, type: void * */
    QURL_OPT_READ_CB,               /*!< client --> qurl callback function, type: qurl_read_cb */
    QURL_OPT_READ_CB_ARG,           /*!< client --> qurl callback function parameter, type: void * */
    QURL_OPT_UPLOAD_HEAD_DATA,      /*!< Specify upload request header data, pass pointer. */
    QURL_OPT_UPLOAD_HEAD_SIZE,      /*!< Specify upload request header size, such as http's body, ftp's file size, etc. */
    QURL_OPT_UPLOAD_HEAD_FILE,      /*!< Specify upload request header data, input file name, upload its file content. */
    QURL_OPT_UPLOAD_DATA,           /*!< Specify upload data, pass pointer. */
    QURL_OPT_UPLOAD_SIZE,           /*!< Specify upload size, such as http's body, ftp's file size, etc. */
    QURL_OPT_UPLOAD_FILE,           /*!< Specify upload data, input file name, upload its file content. */
    QURL_OPT_NOBODY,                /*!< Do not need to get body part */
    QURL_OPT_DIRLISTONLY,           /*!< Only need directory names, no details */
    QURL_OPT_IGNORE_CONTENT_LENGTH, /*!< Ignore data length when downloading */
    QURL_OPT_TRANSFERTEXT,          /*!< Transfer data format: 0/1 --> text/ASCII */
    QURL_OPT_APPEND,                /*!< Incremental upload */
    QURL_OPT_WILDCARDMATCH,         /*!< Wildcard matching */
    QURL_OPT_FILETIME,  /*!< Try to get the file time of the remote document. Later, you can use qurl_core_getinfo() function to get this time (if possible). */
    QURL_OPT_TIMEVALUE, /*!< Used to set a time value, which will be compared with the last modification time of the remote document to determine whether the document needs to be re-downloaded. This option is usually used together with QURL_OPT_TIMECONDITION option. */
    QURL_OPT_TIMECONDITION, /*!< Get file time condition, reference: qurl_timecond_t */
    QURL_OPT_MAXFILESIZE, /*!< Set maximum SIZE of download file: x<0: invalid, return parameter setting error; x==0: no limit (default); x>0: specify limit value */
    QURL_OPT_CUSTOMREQUEST, /*!< User-defined request command, such as HTTP's POST, GET methods; FTP replace LIST or NLIST command. */

    /* Middleware related */
    QURL_OPT_BOUND_THREAD
        = (QURL_OPT_URL + 0x0100
        ), /*!< Lock specified thread. NULL: lock current thread. Note: this only updates the lockable thread, whether to lock will maintain the original state. */
    QURL_OPT_BOUND_THREAD_CTRL,    /*!< qurl business lock thread: 0: unbind. 1: bind user specified thread. 2: bind qurl business create thread. */
    QURL_OPT_REUSE_FRESH,          /*!< Force new conn. That is, will not find existing connections from conn pool. */
    QURL_OPT_REUSE_FORBID,         /*!< Force release conn. That is, will not share current conn to conn pool. */
    QURL_OPT_REUSE_HOLD,           /*!< Persistent connection flag. */
    QURL_OPT_CONN_IDLE_TIMEOUT_MS, /*!< conn unused timeout. Similar to CURLOPT_MAXAGE_CONN in curl */
    QURL_OPT_CONN_MAXLIFETIME_MS,  /*!< conn maximum life */

    /* Protocol related */
    /** Multi-protocol related */

    /** socket */
    QURL_OPT_SOCKET_SO_LINGER = (QURL_OPT_URL + 0x0200), /*!< SO_LINGER configuration, input parameter is qurl_so_linger_t. */
    QURL_OPT_SOCKET_SO_KEEPALIVE,                        /*!< SO_KEEPALIVE configuration, enable or disable TCP keep-alive function. */
    QURL_OPT_SOCKET_TCP_KEEPIDLE,                        /*!< TCP_KEEPIDLE configuration, keep-alive function idle time. */
    QURL_OPT_SOCKET_TCP_KEEPINTVL,                       /*!< TCP_KEEPINTVL configuration, keep-alive function message interval. */
    QURL_OPT_SOCKET_TCP_KEEPCNT,                         /*!< TCP_KEEPINTVL configuration, keep-alive function message count. */

    /** TLS */
    QURL_OPT_TLS_CFG = (QURL_OPT_URL + 0x0280), /*!< Configure TLS CFG. Note: passing a pointer, entity resources are maintained by the user. */
    QURL_OPT_TLS_USETLS,                        /*!< Specify connection to use TLS, reference qurl_usetls_e */

    /** http protocol */
    QURL_OPT_HTTP_VERSION
        = (QURL_OPT_URL + 0x0300
        ), /*!< By setting the QURL_OPT_HTTP_VERSION option, you can specify the specific HTTP protocol version to use. The value of this option should be one of the QURL_HTTP_VERSION* enumerations listed below. */
    QURL_OPT_HTTP_GET,          /*!< Specify as http get request method */
    QURL_OPT_HTTP_POST,         /*!< Specify as http post request method */
    QURL_OPT_HTTP_POST_FORM,    /*!< Specify as http post request method, dedicated to multi-form */
    QURL_OPT_HTTP_PUT,          /*!< Specify as http put request method */
    QURL_OPT_HTTP_PUT_FORM,     /*!< Specify as http put request method, dedicated to multi-form */
    QURL_OPT_HTTP_PATCH,        /*!< Specify as http patch request method */
    QURL_OPT_HTTP_PATCH_FORM,   /*!< Specify as http patch request method, dedicated to multi-form */
    QURL_OPT_HTTP_AUTH,         /*!< HTTP authentication scheme. Option reference: qurl_http_auth_e */
    QURL_OPT_FOLLOWLOCATION,    /*!< Whether to allow following redirects */
    QURL_OPT_UNRESTRICTED_AUTH, /*!< Whether to allow providing authentication (username and password) for new addresses after following redirects */
    QURL_OPT_AUTOREFERER,       /*!< Whether to provide Referer field after following redirects */
    QURL_OPT_POSTREDIR, /*!< Keep POST request as POST request after 30x request; each bit represents a request, from 301 to 303. Variable reference: qurl_http_redir_e */
    QURL_OPT_FORM,            /*!< Form settings, reference qurl_http_form_cfg_t */
    QURL_OPT_HTTP_HEADER,     /*!< User-defined http request header */
    QURL_OPT_REFERER,         /*!< Set HTTP Referer string */
    QURL_OPT_ACCEPT_ENCODING, /*!< Set HTTP Accept-Encoding string */
    QURL_OPT_USER_AGENT,      /*!< Set HTTP User-Agent string */

    /** ftp protocol */
    QURL_OPT_FTP_FILEMETHOD = (QURL_OPT_URL + 0x0400), /*!< Specify FTP file acquisition method, reference: qurl_ftp_filemethod_e */
    QURL_OPT_FTP_AUTH,                                 /*!< Specify FTP AUTH authentication mechanism, reference: qurl_ftp_auth_e */
    QURL_OPT_FTP_SKIP_PASV_IP, /*!< Whether to ignore the specified data connection IP issued by the server in PASV mode. Default enabled */
    QURL_OPT_FTP_PORT, /*!< Whether to enable active mode and specify port number. (FTP engine defaults to passive). Receive format: (ipv4|ipv6|domain name|network interface)?(:port(-range)?)? Note: network interface not yet implemented */
    QURL_OPT_FTP_USE_EPRT, /*!< Indicates whether to enable or disable the FTP engine's EPRT command. By default, it will try to use EPRT, then try to use the traditional PORT command. */
    QURL_OPT_FTP_USE_EPSV, /*!< Indicates whether to enable or disable the FTP engine's EPSV command. By default, the FTP engine will use EPSV first, then consider PASV. */
    QURL_OPT_FTP_USE_PRET, /*!< Send PRET before sending PASV. */
    QURL_OPT_FTP_TLS_CCC,  /*!< Control whether FTP connection uses CCC and switch to what state, reference: qurl_ftp_ccc_e */
    QURL_OPT_QUOTE,        /*!< User input command list. After FTP connection is established, and before each file transfer, send the specified QUOTE command */
    QURL_OPT_POSTQUOTE,    /*!< User input command list. After FTP transfer is completed, but still in connection state, send the specified QUOTE command. */
    QURL_OPT_PREQUOTE, /*!< User input command list. After FTP connection is established, but before actual file transfer, send the specified QUOTE command. */

    QURL_OPT_SMTP_MAIL_FROM,            /*!< Set sender of email */
    QURL_OPT_SMTP_MAIL_RCPT,            /*!< Set recipient of email */
    QURL_OPT_SMTP_MAILAUTH,             /*!< Set authentication initiator of email */
    QURL_OPT_SMTP_MAIl_RCPT_ALLOWFAILS, /*!< Set allowed recipients who failed to receive */
    QURL_OPT_SMTP_LOGIN_OPTIONS,        /*!< LOGIN authentication */
    QURL_OPT_SMTP_XOAUTH2_BEARER,       /*!< XOAUTH2 authentication */
                                        /** smtp protocol */
} qurl_opt_e;

/**
 * @enum  qurl_info_e
 * @brief qurl information options
 */
typedef enum
{
    QURL_INFO_URL = 0x2000, /*!< URL */
    QURL_INFO_RESP_CODE,

    QURL_INFO_RESP_CONTENT_LENGTH, /*!< http: is Content-Length in header field, -1 means no such field, such as chunck. ftp: SIZE command, etc. */
    QURL_INFO_FILETIME,            /*!< Greenwich time */
    QURL_INFO_RESP_DATE,           /*!< http: is Date in header field */
    QURL_INFO_START_POS,           /*!< ftp: starting position of segmented transfer */
} qurl_info_e;

/* Security related */
/** TLS version */
typedef enum
{
    QURL_TLS_VERSION_DEFAULT = 0, /*!< Default version */
    QURL_TLS_VERSION_SSL3_0,
    QURL_TLS_VERSION_TLS1_0,
    QURL_TLS_VERSION_TLS1_1,
    QURL_TLS_VERSION_TLS1_2,
    QURL_TLS_VERSION_TLS1_3,
    QURL_TLS_VERSION_ALL,
    QURL_TLS_VERSION_TLS_LAST, /*!< Only for overflow judgment */
} qurl_tls_version_e;
/** TLS verification method */
typedef enum
{
    QURL_TLS_VERIFY_NONE = 0x0000,          /*!< TLS no authentication, do not verify server certificate */
    QURL_TLS_VERIFY_SERVER = 0x0001,        /*!< TLS verify server certificate */
    QURL_TLS_VERIFY_CLIENT_SERVER = 0x0002, /*!< TLS two-way authentication */
    QURL_TLS_VERIFY_PSK = 0x0003,           /*!< PSK connection method */
} qurl_tls_verify_e;

/* File related */
/** Get file time condition */
typedef enum
{
    QURL_TIMECOND_NONE,         /*!< No time condition. */
    QURL_TIMECOND_IFMODSINCE,   /*!< Transfer file if it has been modified since the specified date or later. */
    QURL_TIMECOND_IFUNMODSINCE, /*!< Transfer file if it has not been modified since the specified date or later. */
    QURL_TIMECOND_LASTMOD,      /*!< Transfer file if the file's last modification date is later than the specified date. */
    QURL_TIMECOND_LAST          /*!< Only for overflow judgment */
} qurl_timecond_t;

/* Protocol related */
/** HTTP redirect related OPT: QURL_OPT_POSTREDIR: [qurl][lab][202310201702] */
typedef enum
{
    QURL_HTTP_REDIR_GET_ALL = ((unsigned long)0),
    QURL_HTTP_REDIR_POST_301 = (((unsigned long)1) << 0),
    QURL_HTTP_REDIR_POST_302 = (((unsigned long)1) << 1),
    QURL_HTTP_REDIR_POST_303 = (((unsigned long)1) << 2),
    QURL_HTTP_REDIR_POST_ALL = (QURL_HTTP_REDIR_POST_301 | QURL_HTTP_REDIR_POST_302 | QURL_HTTP_REDIR_POST_303),
} qurl_http_redir_e;

/** HTTP authentication scheme options */
typedef enum
{
    QURL_HTTP_AUTH_NONE = ((unsigned long)0),          /*!< No HTTP authentication. */
    QURL_HTTP_AUTH_BASIC = (((unsigned long)1) << 0),  /*!< HTTP basic authentication (default). */
    QURL_HTTP_AUTH_DIGEST = (((unsigned long)1) << 1), /*!< HTTP digest authentication. */
    QURL_HTTP_AUTH_ONLY = (((unsigned long)1) << 31),  /*!< Use with other single types, force no authentication or use only that single type. */
    QURL_HTTP_AUTH_ALL = (QURL_HTTP_AUTH_BASIC | QURL_HTTP_AUTH_DIGEST), /*!< Bitmask of all available authentication types except CURLAUTH_DIGEST_IE. */
} qurl_http_auth_e;

/** HTTP form type: combined with qurl_http_form_cfg_t */
typedef enum
{
    QURL_HTTP_FORM_CONTENT_DATA,    /*!< Form's content_ptr is data pointer, that is, client passes pointer in, entity resources are maintained by client. */
    QURL_HTTP_FORM_CONTENT_DATA_CP, /*!< Form's content_ptr field data pointer, but internally will copy the content pointed to by this pointer, that is, entity resources are maintained by qurl internally. */
    QURL_HTTP_FORM_CONTENT_CB,      /*!< Form's content_ptr is callback function parameter, length is content_len in qurl_http_form_cfg_t */
    QURL_HTTP_FORM_CONTENT_FILE, /*!< Form's content_ptr is file name, content_len field in qurl_http_form_cfg_t is invalid, internally will read file length */
} qurl_http_form_e;

/* Multi-connection protocol */
/** Whether connection uses TLS: FTP, IMAP, POP or others. [lzm][note] This function to be deleted */
typedef enum
{
    QURL_USETLS_NONE,    /*!< Do not attempt to use TLS, that is, plaintext communication. */
    QURL_USETLS_TRY,     /*!< Try to use TLS, if TLS connection fails, continue with non-TLS connection. */
    QURL_USETLS_CONTROL, /*!< Use TLS only for control connection, if TLS connection fails, operation fails. */
    QURL_USETLS_ALL,     /*!< Use TLS for all communications (including control connection and data connection), if TLS connection fails, operation fails. */
    QURL_USETLS_LAST,    /*!< Only for overflow judgment */
} qurl_usetls_e;

/* FTP */
typedef enum
{
    QURL_FTP_FILEMETHOD_MULTICWD, /*!< Perform multiple CWD (Change Working Directory) operations according to RFC1738 definition, then perform file operations */
    QURL_FTP_FILEMETHOD_NOCWD,    /*!< Perform SIZE, RETR or STOR operations directly on the complete path without additional CWD operations. */
    QURL_FTP_FILEMETHOD_SINGLECWD, /*!< First perform one CWD operation directly to the target path, then perform SIZE, RETR or STOR operations on the file. */
} qurl_ftp_filemethod_e;

/**
 * FTP security authentication mechanism: OPT: QURL_OPT_FTP_AUTH
 * Only priority selection, such as selecting SSL, if AUTH SSL fails, will select AUTH TLS to continue.
 */
typedef enum
{
    QURL_FTP_AUTH_DEFAULT, /*!< Internal decision. */
    QURL_FTP_AUTH_SSL,     /*!< AUTH SSL. */
    QURL_FTP_AUTH_TLS,     /*!< AUTH TLS. */
    QURL_FTP_AUTH_LAST,    /*!< Only for overflow judgment */
} qurl_ftp_auth_e;

/**
 * FTP use CCC command options: OPT: QURL_OPT_FTP_TLS_CCC
 */
typedef enum
{
    QURL_FTP_CCC_NONE,    /*!< Do not use CCC, maintain default behavior. */
    QURL_FTP_CCC_PASSIVE, /*!< Let server initiate closing connection. */
    QURL_FTP_CCC_ACTIVE,  /*!< Actively close connection. */
    QURL_FTP_CCC_LAST,    /*!< Only for overflow judgment */
} qurl_ftp_ccc_e;

#endif /* Head define end*/
