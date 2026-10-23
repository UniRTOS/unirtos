/** @file         qurl_def_in.h
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-09-01 13:39:36
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#ifndef QURL_DEF_IN_H
#define QURL_DEF_IN_H

#include "qurl_config.h"
#include "qurl_code_in.h"
#include "qurl_def.h"
#include "qurl_def_basic.h"
#include "qurl_list.h"
#include "qurl_slist.h"
#include "qurl_dbuf.h"

#include "qurl_tls_def.h"

/** Declare some data structures*/
struct qurl_conn_s;
typedef struct qurl_conn_s qurl_conn_t;
struct qurl_port_prot_s;
typedef struct qurl_port_prot_s qurl_port_prot_t;
struct qurl_core_ctrl_s;
typedef struct qurl_core_ctrl_s qurl_core_ctrl_t;
struct qurl_timeval_s;
typedef struct qurl_timeval_s qurl_timeval_t;
struct qurl_url_s;
typedef struct qurl_url_s qurl_url_t;

/** Define some callback types*/
typedef qurl_ecode_t (*qurl_io_open_cb)(qurl_conn_t *conn_ptr);
typedef qurl_ecode_t (*qurl_io_connect_cb)(qurl_conn_t *conn_ptr);
typedef qurl_ecode_t (*qurl_io_read_cb)(qurl_conn_t *conn_ptr);
typedef qurl_ecode_t (*qurl_io_write_cb)(qurl_conn_t *conn_ptr);
typedef qurl_ecode_t (*qurl_io_close_cb)(qurl_conn_t *conn_ptr);

#if QURL_CFG_ENABLE_HTTP
#include "qurl_http_def.h"
#endif
#if QURL_CFG_ENABLE_FTP
#include "qurl_ftp_def.h"
#endif
#if QURL_CFG_ENABLE_SMTP
#include "qurl_sasl.h"
#include "qurl_smtp_def.h"
#endif

/**
 * @struct qurl_core_cfg_bits_s
 * @brief
 */
struct qurl_core_usr_cfg_bits_s
{
    qurl_bit_t bound_thread;             /*!< Whether the thread is locked*/
    qurl_bit_t reuse_fresh;              /*!< Force create a new connection. Used for binding connection logic.*/
    qurl_bit_t reuse_forbid;             /*!< Forcefully delete the connection. Used for unbinding connection logic.*/
    qurl_bit_t reuse_hold;               /*!< Persistently occupies the connection. Used for unbinding connection logic.*/
    qurl_bit_t tcp_fastopen;             /*!< use TCP Fast Open */
#if QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE
    qurl_bit_t so_keepalive;             /*!< Keep-alive function switch flag*/
#endif                                   /* QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE */
    qurl_bit_t path_as_is;               /*!< Whether to allow ".." in URLs to represent parent directories*/
    qurl_bit_t disallow_username_in_url; /*!< Whether to allow including a username in the URL.*/
    qurl_bit_t
        upload_head_raw; /*!< Indicates that HEAD RAW is provided, and HEAD will not be assembled internally. It can be provided from HEAD_DATA, HEAD_FILE, or CB. If none are provided, no exception will be thrown, meaning it is assumed that both HEAD and BODY are provided together in DATA, FILE, or CB.*/
    qurl_bit_t opt_nobody;                /*!< No need to fetch the body. Configured by the option QURL_OPT_NOBODY.*/
    qurl_bit_t list_only;                 /*!< Only the directory name is needed, not the details. For example, FTP's NLST, not LIST*/
    qurl_bit_t ignore_cl;                 /*!< Ignore the content length.*/
    qurl_bit_t prefer_ascii;              /*!< Specify the preferred data transfer method. ASCII or binary*/
    qurl_bit_t remote_append;             /*!< Write by appending, not overwriting*/
    qurl_bit_t get_filetime;              /*!< Get remote file time and fetch operation*/
    qurl_bit_t http_follow_location;      /*!< Whether to allow following redirects.*/
    qurl_bit_t allow_auth_to_other_hosts; /*!< Whether to allow redirection followed by providing (username and password) for authentication.*/
    qurl_bit_t http_auto_referer;         /*!< Automatically sets the correct referrer page when following redirects.*/
    qurl_bit_t wildcard_match;            /*!< Whether wildcards are supported*/
    qurl_bit_t ftp_skip_ip;               /*!< Skip the IP address passed to us by the FTP server.*/
    qurl_bit_t ftp_use_port; /*!< Whether to use the FTP PORT command. The PORT command is used to inform the FTP server to use active mode for data transfer.*/
    qurl_bit_t ftp_use_epsv; /*!< Whether to attempt using the EPSV command. The EPSV command is used to inform the FTP server to use extended passive mode during data transmission.*/
    qurl_bit_t ftp_use_eprt; /*!< Whether to attempt using the EPRT command. The EPRT command is used to inform the FTP server to use extended active mode during data transmission.*/
    qurl_bit_t ftp_use_pret; /*!< Whether to use the PRET command before the PASV command. The PRET command is used to inform the FTP server to prepare for data reception in passive mode.*/
#if QURL_CFG_ENABLE_SMTP
    qurl_bit_t mail_rcpt_allowfails;
#endif
};
typedef struct qurl_core_usr_cfg_bits_s qurl_core_usr_cfg_bits_t;

/**
 * @struct  qurl_core_usr_cfg_t
 * @brief User Configuration Information
 */
struct qurl_core_usr_cfg_s
{
    /*Controls Related*/
    unsigned long thread_id; /*!< User-specified bindable thread ID*/
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    /*Network Configuration*/
    int nw_id; /*!< Network ID, < 0 indicates not specified*/
#endif
    /*Transport Protocol Configuration*/
    unsigned short port; /*!< Specify the port number*/

    /*Timeout Configuration*/
    long timeout_ms;           /*!< 0 means no timeout */
    long idle_timeout_ms;      /*!< 0 means no timeout */
    long conn_idle_timeout_ms; /*!< 0 means no timeout */
    long conn_maxlifetime_ms;  /*!< 0 means no timeout */

    /*Wake-up time*/
    long rouse_check_time_ms; /*!< 0 means no timeout */

    /*Maximum number of attempts*/
    long redirs_cnt_max; /*!< Maximum number of HTTP(S) redirect follow-ups.*/

    /*Transmission Configuration*/
    long resume_from; /*!< Specifies the starting point for resuming transmission. If negative, it indicates counting backwards from the end, e.g., -100 means the last 100 bytes need to be downloaded/uploaded. Unit: byte*/

    /*Data Transmission Related*/
    /** Upload header data*/
    void *upload_head_data_ptr; /*!< Specify the data to be uploaded*/
    long  upload_head_size;     /*!< Specify the upload size, unit: byte*/
    char *upload_head_file_ptr; /*!< Specify the file to upload*/
    /** Upload data*/
    void *upload_data_ptr; /*!< Specify the data to be uploaded*/
    long  upload_size;     /*!< Specify the upload size, unit: byte*/
    char *upload_file_ptr; /*!< Specify the file to upload*/
    /** Download data*/
    qurl_time_t timevalue; /*!< Used in conjunction with timecondition, this time value will be compared with the last modification time of the remote document to determine whether the document needs to be re-downloaded.*/
    unsigned char timecondition; /*!< Time condition. Refer to qurl_timecond_t*/
    long          max_filesize;  /*!< User specifies the maximum file download SIZE*/

    /*Transmission Control*/
    long use_tls; /*!< The requirement for using TLS connection, applicable to FTP, IMAP, POP3. Refer to qurl_usetls_e*/

                  /*Socket-related*/
#if QURL_CFG_PORT_SOCKET_OPT_SO_LINGER
    qurl_so_linger_t so_linger; /*!< The linger option for sockets*/
#endif                          /* QURL_CFG_PORT_SOCKET_OPT_SO_LINGER */
                                /** Keep-alive function, the following values take effect only when tcp_keepalive is enabled.*/
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE
    long tcp_keepidle;          /*!< TCP keepalive idle time*/
#endif                          /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE */
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL
    long tcp_keepintvl;         /*!< TCP keepalive interval*/
#endif                          /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL */
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT
    long tcp_keepcnt;           /*!< Number of TCP keepalive attempts*/
#endif                          /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT */

    /*Callback with the user*/
    /** Callback function parameters*/
    void *write_head_to_client_arg;
    void *write_to_client_arg;
    void *read_head_from_client_arg;
    void *read_from_client_arg;
    /** Callback function*/
    qurl_write_head_cb write_head_to_client_func;  /*!< User-side set user receive head data processing function, such as HTTP response header*/
    qurl_write_cb      write_to_client_func;       /*!< User-side configured user data reception processing function*/
    qurl_read_head_cb  read_head_from_client_func; /*!< User-side configured user send head data processing function*/
    qurl_read_cb       read_from_client_func;      /*!< User-side configured user data transmission processing function*/

    /* TLS */
    qurl_tls_cfg_t *tls_cfg_ptr; /*!< Configuration information used for TLS connection*/

                                 /*Protocol stack specific configuration:
		 * History: This was originally a union, designed to accommodate subsequent compatibility with numerous protocol stacks while still maintaining minimal memory consumption.
		 * However, this would lead to complex interface maintenance.
		 * For example, if the user configures the HTTP settings and also configures the FTP settings, this will cause confusion.
		 * 	Either detect the protocol in the URL first and then restrict it. In any case, it will lead to interface maintenance troubles, and when adding new protocol stacks later, such issues will also need to be considered more.
		 * 	So we simply removed it.
		 */
    // union {
#if QURL_CFG_ENABLE_HTTP
    qurl_core_usr_cfg_http_t http;
#endif /* QURL_CFG_ENABLE_HTTP */
#if QURL_CFG_ENABLE_FTP
    qurl_core_usr_cfg_ftp_t ftp;
#endif /* QURL_CFG_ENABLE_FTP */
#if QURL_CFG_ENABLE_SMTP
    qurl_core_usr_cfg_smtp_t smtp;
#endif /* QURL_CFG_ENABLE_SMTP */
    // }protocol_u;

    /*String-related*/
    char *url_ptr;             /*!< URL */
    char *username_ptr;        /*!< Username*/
    char *password_ptr;        /*!< Password*/
    char *account_ptr;         /*!< Account. Usage protocol: ftp ...*/
    char *customrequest_ptr;   /*!< User-defined method*/
    char *referer_ptr;         /*!< http header: Referer */
    char *accept_encoding_ptr; /*!< http header: Accept-Encoding */
    char *user_agent_ptr;      /*!< http header: User-Agent */
    char *range_ptr;           /*!< http header: Range */

    /*Operating Characteristics Configuration*/
    qurl_core_usr_cfg_bits_t bits;
};
typedef struct qurl_core_usr_cfg_s qurl_core_usr_cfg_t;

/**
 * @struct qurl_core_cfg_bits_s
 * @brief
 */
struct qurl_core_cfg_bits_s
{
    qurl_bit_t nw_ipv4;            /*!< Whether the network has specified a local IPv4 address*/
    qurl_bit_t nw_ipv6;            /*!< Whether the network has specified a local IPv6 address*/
    qurl_bit_t url_alloc_flag;     /*!< Whether the current url_ptr is malloc'd by itself*/
    qurl_bit_t referer_alloc_flag; /*!< Whether the current referer_ptr is malloc'd by itself*/
    qurl_bit_t range_alloc_flag;   /*!< Whether the current range_ptr is allocated by itself via malloc*/
    qurl_bit_t upload_head_raw;    /*!< true: The user uploads request header data. RAW data*/
    qurl_bit_t upload_raw;         /*!< true: allows the request header to be provided in the body.*/
    qurl_bit_t upload;             /*!< true: upload data. false: download data*/
    qurl_bit_t use_range;          /*!< Whether range download is effective*/
    qurl_bit_t prefer_ascii;       /*!< Specify the preferred data transfer method. ASCII or binary*/
    qurl_bit_t remote_append;      /*!< Write by appending, not overwriting*/
    qurl_bit_t allow_port;         /*!< Whether to allow using the port number set by usr_cfg.port*/
    qurl_bit_t auth_failed;        /*!< Mark identity authentication failure*/
    qurl_bit_t is_follow;          /*!< Already a redirection*/
    qurl_bit_t wildcard_match;     /*!< Whether wildcards are supported*/
};
typedef struct qurl_core_cfg_bits_s qurl_core_cfg_bits_t;

/**
 * @struct  qurl_core_cfg_t
 * @brief Configuration information generated during qurl runtime
 */
struct qurl_core_cfg_s
{
    char       *url_ptr;  /*!< work URL */
    qurl_url_t *uurl_ptr; /*!< URL parsing result*/

    /* network */
    char nw_ipv4_str[QURL_INET4_ADDRSTRLEN];
    char nw_ipv6_str[QURL_INET6_ADDRSTRLEN];

    /*Before redirection*/
    char       *first_host;        /*!< [dyn] Hostname*/
    int         first_remote_port; /*!< Port number*/
    qurl_prot_t first_protocol;    /*!< Protocol*/
    /*Redirected*/
    char *new_url; /*!< [dyn] New URL*/
    /*referer field*/
    char *referer_ptr; /*!< referer */

    /*Data Transmission Section*/
    long resume_from; /*!< Specifies the starting point for resuming transmission. If negative, it indicates counting backwards from the end, e.g., -100 means the last 100 bytes need to be downloaded/uploaded. Unit: byte*/
    /** Upload Section*/
    long upload_head_size; /*!< The length of the request header data to be uploaded. HTTP: automatically recognized by default at cb \r\n\r\n. Refer to cfg.bits.upload_head_raw to determine whether to upload.*/
    int  upload_head_file_fd; /*!< File handle of the uploaded request header file*/
    long upload_size;         /*!< The length of data to be uploaded. Refer to cfg.bits.upload to determine whether to upload.*/
    int  upload_file_fd;      /*!< File handle of the uploaded file*/
    /** Download Section*/
    long  download_max; /*!< Download size limit, -1: unlimited.*/
    char *range_ptr;    /*!< range. Possible allocation.*/

    /*Info generated midway*/
    char *scheme;        /*!< The scheme of the current connection*/
    char *wouldredirect; /*!< The redirected URL, provided for the user to use with getopt.*/

                         /*Application Protocol Configuration*/
    /*Protocol Stack Specific Configuration*/
    // union {
#if QURL_CFG_ENABLE_HTTP
    qurl_core_cfg_http_t http;
#endif /* QURL_CFG_ENABLE_HTTP */
#if QURL_CFG_ENABLE_FTP
    qurl_core_cfg_ftp_t ftp;
#endif /* QURL_CFG_ENABLE_FTP */
    // }protocol_u;

    /*[dyn] Dynamic string field in the request header: must be freed using qurl_free*/
    /** [lzm][note]This is only a cache used when assembling request packets. It will be deleted later according to the plan.*/
    char *proxyuserpwd;    /*!< Proxy server username and password*/
    char *uagent;          /*!< User Agent Information*/
    char *accept_encoding; /*!< Client accepted encoding format*/
    char *userpwd;         /*!< Encoded username and password for user authentication*/
    char *rangeline;       /*!< Requested range*/
    char *ref;             /*!< URL of the referenced page*/
    char *host;            /*!< Hostname*/
    char *cookiehost;      /*!< Cookie's hostname*/
    char *rtsp_transport;  /*!< RTSP Transport Protocol*/
    char *te;              /*!< The TE field in the request header*/
    char *user;            /*!< Username used for transmission*/
    char *passwd;          /*!< Password used for transmission*/
    char *proxyuser;       /*!< Proxy server username*/
    char *proxypasswd;     /*!< Proxy server password*/

    qurl_core_cfg_bits_t bits;
};
typedef struct qurl_core_cfg_s qurl_core_cfg_t;

/**
 * @struct qurl_core_trans_bits_t
 * @brief 。
 */
struct qurl_core_trans_bits_s
{
    qurl_bit_t chunked; /*!< Marked as chunked transfer*/
};
typedef struct qurl_core_trans_bits_s qurl_core_trans_bits_t;

/**
 * @struct  qurl_core_trans_t
 * @brief Transmission records generated during qurl runtime
 */
struct qurl_core_trans_s
{
    long redirs_cnt; /*!< The number of times to follow HTTP(S) redirects.*/

    /*Download Section*/
    long download_len_max; /*!< The maximum data length that ultimately needs to be downloaded this time. -1: Unlimited/unknown size.*/
    long downloaded_len;   /*!< The length already downloaded this time*/

    /*Send Section*/
    long upload_head_len_max; /*!< Length of the head raw data to be uploaded*/
    long uploaded_head_len;   /*!< Length of the uploaded head raw data*/
    long upload_len_max;      /*!< Length to be uploaded*/
    long uploaded_len;        /*!< Length already uploaded*/

    /*User Side*/
    long client_w_len; /*!< Length of data written to the user: recorded in real-time*/

    /* bits */
    qurl_core_trans_bits_t bits;
};
typedef struct qurl_core_trans_s qurl_core_trans_t;

/**
 * @struct  qurl_core_info_t
 * @brief Information data of the qurl core
 */
struct qurl_core_info_s
{
    int         resp_code;      /*!< Server response result code*/
    int         content_length; /*!< HTTP Content-Length header*/
    char       *resp_date;      /*!< HTTP Date header*/
    qurl_time_t filetime;       /*!< Stores the time information of the file. If the time cannot be obtained, it is set to -1.*/
    int         start_pos;      /*!< Starting position for FTP segmented transmission*/

    qurl_bit_t timecond;        /*true: blocked by a condition, resulting in the file not being fetched.*/
};
typedef struct qurl_core_info_s qurl_core_info_t;

/**
 * @struct qurl_core_ctrl_bits_t
 * @brief 。
 */
struct qurl_core_ctrl_bits_s
{
    /* ipc */
    qurl_bit_t abort_flag; /*!< Whether it is marked as terminated*/
    /*Timeout*/
    qurl_bit_t timeout; /*!< Operation timeout*/
    /* alloc flag */
    qurl_bit_t prot_data_alloc_flag; /*Is the prot_data pointer allocated?*/
};
typedef struct qurl_core_ctrl_bits_s qurl_core_ctrl_bits_t;

/**
 * @struct  qurl_core_ctrl_t
 * @brief Single-path operation handle for qurl
 */
struct qurl_core_ctrl_s
{
    /*Core Local Status Information*/
    unsigned long create_thread_id; /*!< Ancestor*/
    unsigned long owner_thread_id;  /*!< Validity flag*/
    int           sec_index;        /*!< [note]The lock within the safety here is a local lock. Refer to [tab][202309081813]*/
    qurl_state_e  state;            /*!< Status*/
    qurl_ecode_t  err_code;         /*!< Error code*/

    /* ipc */
    /** State machine IPC handle*/
    qurl_queue_t ipc_event; /*!< Specifically for state machine blocking*/
    /** The event received in this polling*/
    qurl_event_t recv_event; /*!< Specifically for state machine blocking*/
    /*Total Start Time*/
    qurl_timeval_t start_timeval;
    /*Idle start time*/
    qurl_timeval_t idle_start_timeval;

    /*Binding*/
    qurl_conn_t *conn_ptr;  /*!< Bound connection*/
    void        *prot_data; /*!< Core data bound to the protocol interface*/

    /*Configuration: User configuration can only be modified by the user.
		 * During operation, it first retrieves from the configuration cache; if not available, it then obtains from the user configuration.
		 * During development, if it is necessary to disregard user configurations based on a strategy during runtime, a copy option can be added to the configuration cache.*/
    qurl_core_usr_cfg_t usr_cfg; /*!< User Configuration*/
    qurl_core_cfg_t     cfg;     /*!< Configure cache*/
    qurl_core_trans_t   trans;   /*!< Running log/status*/

    /*Information*/
    qurl_core_info_t info;

    /*Buffer: The lower-level node is the send/receive buffer of the connection.*/
    qurl_dbuf_t send_cache; /*!< Send buffer*/
    qurl_dbuf_t recv_cache; /*!< Send buffer*/

    /* bits */
    qurl_core_ctrl_bits_t bits;
};
typedef struct qurl_core_ctrl_s qurl_core_ctrl_t;

/**
 * @struct  qurl_core_sec_t
 * @brief Security of the qurl core
 */
struct qurl_core_sec_s
{
    /*1. Lock*/
    qurl_mutex_t lock; /*!< [tab][202309151726]Changed to core global lock [tab][202309081813]Local lock, please mark the resources that this lock needs to maintain: [tab][qurl][sec][lock]*/

    /*2. Owner.[tab][qurl][sec][lock]*/
    qurl_core_ctrl_t *owner;

    /*3. Status.[tab][qurl][sec][lock]*/
    qurl_sec_state_e state;
};
typedef struct qurl_core_sec_s qurl_core_sec_t;

/** about PORT */
/**
 * @struct qurl_conn_t
 * @brief The only field and variable data in this connection.
 */
struct qurl_conn_conn_bits_s
{
    qurl_bit_t sock_addrinfo_alloc; /*!< Whether sock_addrinfo_ptr is an entity*/
    qurl_bit_t trans_connecting;    /*!< Transport layer connection: TCP, UDP. This flag is mainly used in non-state machines.*/
    qurl_bit_t trans_connected;     /*!< Whether the transport layer is connected: TCP, UDP*/
    qurl_bit_t tls_connected;       /*!< Whether the TLS connection has been established: TLS.*/
    qurl_bit_t prot_connected;      /*!< Whether the application protocol has completed connection: e.g., FTP requires successful login*/
    qurl_bit_t rx_closed; /*!< TCP/TLS normal behavior closure. After the connection is established, confirm that RX is closed during read, such as upon receiving a FIN. For TLS, upon receiving a "close_notify" alert: the peer informs us that the connection is about to close.*/
    qurl_bit_t is_server; /*!< Whether the current connection is a TCP server*/
    qurl_bit_t is_tls;    /*!< Whether the current connection is TLS*/
};
typedef struct qurl_conn_conn_bits_s qurl_conn_conn_bits_t;

/**
 * @struct  qurl_conn_conn_t
 * @brief The data structure for each connection within conn
 * @note Resources are divided into two levels: dynamic resources and static resources. The following static resources, marked with [s], are inherent to conn_conn and are generally maintained by create and delete.
 */
struct qurl_conn_conn_s
{
    /* socket */
    qurl_socket_t sock_fd; /*!< Entity.*/

    /*Port Number Related*/
    int local_port;  /*!< Local port number*/
    int remote_port; /*!< Remote port number*/

    /*Address Information*/
    unsigned char    addrinfo_cnt;      /*!< Number of parsed results*/
    unsigned char    resolve_addrtype;  /*[lzm][todo]To be implemented assignment*/
    qurl_addrinfo_t *addrinfo_ptr;      /*!< Host address, match identifier*/
    qurl_addrinfo_t *sock_addrinfo_ptr; /*!< Points to an entry in addrinfo_ptr, which should be freed by addrinfo_ptr itself.*/
    char            *ip_str;            /*!< String representation of the currently connected IP address*/

                                        /* TLS [s] */
#if QURL_CFG_ENABLE_TLS
    qurl_tls_ctrl_t *tls_ctrl_ptr; /*!< TLS control block pointer: [lzm][todo] may need to be extended when developing FTP*/
#endif                             /* QURL_CFG_ENABLE_TLS */

    /*Buffer [s]*/
    qurl_dbuf_t send_cache; /*!< Send buffer*/
    qurl_dbuf_t recv_cache; /*!< Send buffer*/

    qurl_conn_conn_bits_t bits;
};
typedef struct qurl_conn_conn_s qurl_conn_conn_t;

/**
 * @struct  qurl_conn_conn_server_t
 * @brief The data structure for each connection within conn, specific to the TCP server.
 */
struct qurl_conn_conn_server_s
{
    /* socket */
    qurl_socket_t sock_fd; /*!< Entity.*/

    /*Port Number Related*/
    int local_port; /*!< Local port number*/
    int port_min;   /*!< Specify the local port number range*/
    int port_max;   /*!< Specify the local port number range*/

    /*Address Information*/
    unsigned char    addrinfo_cnt;      /*!< Number of parsed results*/
    qurl_addrinfo_t *addrinfo_ptr;      /*!< Local server address information*/
    qurl_addrinfo_t *sock_addrinfo_ptr; /*!< Points to an entry in addrinfo_ptr, which can be freed by addrinfo_ptr.*/
    char            *ip_str;            /*!< The string form of the currently connected local IP address*/
};
typedef struct qurl_conn_conn_server_s qurl_conn_conn_server_t;

/**
 * @struct qurl_conn_t
 * @brief The only field and variable data in this connection.
 */
struct qurl_conn_bits_s
{
    qurl_bit_t reuse;               /*!< if set, this is a re-used connection */
    qurl_bit_t tcp_fastopen;        /*!< use TCP Fast Open */
    qurl_bit_t req_more;            /*!< Whether to switch to the req_more state after reqing.*/
    qurl_bit_t tls_connecting;      /*!< In TLS connection*/
    qurl_bit_t ftp_use_control_tls; /*!< FTP control connection uses TLS*/
    qurl_bit_t ftp_use_data_tls;    /*!< FTP data connection uses TLS*/
    qurl_bit_t
        authneg; /*!< auth negotiation Returns TRUE when the authentication phase has already started. This means we are creating a request with authentication header information, but it is not yet the final request for authentication negotiation.*/
    qurl_bit_t ipv6_ip;    /*!< Communicate with a remote site at a specified pure IPv6 IP address*/
    qurl_bit_t ipv6;       /*!< Communicate with the site using an IPv6 address*/
    qurl_bit_t chunked_ok; /*!< Block reception completed*/
    qurl_bit_t is_reuse;   /*!< The current connection is being reused, connection cache and persistent*/
    qurl_bit_t ftp_use_epsv; /*!< Whether to attempt using the EPSV command. The EPSV command is used to inform the FTP server to use extended passive mode during data transfer.*/
    qurl_bit_t ftp_use_eprt; /*!< Whether to attempt using the EPRT command. The EPRT command is used to inform the FTP server to use extended active mode during data transmission.*/
#if QURL_CFG_ENABLE_SMTP
    qurl_bit_t user_passwd;
#endif
};
typedef struct qurl_conn_bits_s qurl_conn_bits_t;
/**
 * @struct qurl_conn_t
 * @brief The only field and variable data in this connection.
 */
struct qurl_conn_s
{
    /*Connection status information*/
    qurl_conn_state_e state;
    qurl_core_ctrl_t *core_ptr; /*!< Owner of this connection*/
    /*Creation time*/
    qurl_timeval_t created_timeval;
    /*Last used time*/
    qurl_timeval_t lastused_timeval;

    /*Connection Information & Match Identifier*/
    char                  *host_name;  /*!< Host name, allocated, match identifier*/
    qurl_conn_trans_type_e trans_type; /*!< Transport layer protocol stack. Note: Can be set via port setup_connect_func*/
    /** The internal connection owned by conn*/
    qurl_conn_conn_t        conn_conn[QURL_SOCKET_INDEX_LAST]; /*!< Internal connection information*/
    qurl_conn_conn_server_t conn_conn_server;                  /*!< For TCP server-specific use. For example, in FTP active mode.*/
    /** Account Information*/
    char *username_ptr; /*!< Username, allocated, match identifier*/
    char *passwd_ptr;   /*!< Password, allocated, match identifier*/

    /*Dual interface handle. Applicable scenarios such as: HTTP redirection, where the new address protocol may be HTTPS*/
    const qurl_port_prot_t *port_handler;       /*!< Protocol interface handle in use*/
    const qurl_port_prot_t *basic_port_handler; /*!< Initial protocol interface handle.*/
    union
    {
#if QURL_CFG_ENABLE_HTTP
        qurl_http_ctrl_t http_ctrl;
#endif
#if QURL_CFG_ENABLE_FTP
        qurl_ftp_ctrl_t ftp_ctrl;
#endif
#if QURL_CFG_ENABLE_SMTP
        qurl_smtp_ctrl_t smtp_ctrl;
#endif
    } prot_data_u; /*!< Connection data bound to the protocol interface*/

    /* temp */
    qurl_addrinfo_t *temp_addrinfo_ptr[2]; /*!< Used for happy eyeballs.*/
    qurl_socket_t    temp_sock[2];         /*!< For happy eyeballs. Virtual entity. Temporary sockets for happy eyeballs*/
    int              temp_family[2];       /*!< For happy eyeballs. Family used for the temporary sockets.*/
    int              temp_remote_port;

    /*Buffer*/
    unsigned char *deal_cache_ptr; /*!< Data processing cache. Note, it can only be used immediately to avoid ambiguity in the cache.*/

    /*bit flag*/
    qurl_conn_bits_t bits;

    /*Event for the final connection closure*/
    int index0_last_close_event; /*!< HTTP or FTP control connection last closure reason*/
    int index1_last_close_event; /*!< FTP data connection last closure reason*/
    int index_error_judgment;    /*!< Wait for the corresponding result code from the control command*/
};
typedef struct qurl_conn_s qurl_conn_t;

/**
 * @struct qurl_port_io_t
 * @brief I/O interface groups, such as network I/O: socket or vtls or fs.
 * @brief Managed by the data transmission data structure conn, provided for use by the protocol stack.
 */
struct qurl_port_io_s
{
    const char *name;                 /*!< The name identifier of the IO interface group, used for matching.*/

    qurl_io_open_cb    *open_func;    /*!< Open the io object*/
    qurl_io_connect_cb *connect_func; /*!< Establish connection*/
    qurl_io_read_cb    *read_func;    /*!< Read data*/
    qurl_io_write_cb   *write_func;   /*!< Send data*/
    qurl_io_close_cb   *close_func;   /*!< Close this object*/
};
typedef struct qurl_port_io_s qurl_port_io_t;

/**
 * @struct qurl_port_prot_t
 * @brief Port callback function data structure, primarily used for protocol interfaces
 */
struct qurl_port_prot_s
{
    const char *scheme; /*!< URL scheme name. */

    /*Called when creating a connection. Preprocessing for specific application protocols before connection.*/
    /** [lzm][note]Currently, the protocol stack handles clearing the cache of the transmit and receive dbuf.*/
    qurl_ecode_t (*setup_connect_func)(qurl_conn_t *conn_ptr);

    /*Protocol Connection Section*/
    /** A step in the connection process, which can be used to set up the protocol. If the connection is not completed, the caller should continue to call qurl_prot_connect() until it is finished.*/
    /** Start sending protocol connection*/
    qurl_ecode_t (*send_prot_connect_func)(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
    /** Connecting to protocol*/
    qurl_ecode_t (*prot_connect_func)(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);

    /*Request Section*/
    /** Must be set, send the first part of the request*/
    qurl_ecode_t (*req_func)(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
    /** Sending the first part of the request*/
    qurl_ecode_t (*reqing_func)(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
    /** Split qurl_req() into two parts, allowing req_more() after req. For example: FTP-related operations can be performed after executing the PASV/PORT command.*/
    qurl_ecode_t (*req_more_func)(qurl_conn_t *conn_ptr, int *completep_ptr);

    /*Protocol interaction has been performed.*/
    qurl_ecode_t (*trans_data_func)(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);

    /** Must be set*/
    qurl_ecode_t (*trans_done_func)(qurl_conn_t *conn_ptr, qurl_ecode_t ecode, qurl_bit_t done);

    /*As one of the protocol-related functions for the disconnection step, called by qurl_disconnect().
		 * If the handler is called because the connection is considered to have failed, then dead_connection is set to TRUE.
		 * After disconnection, the connection will be (re)associated with the transmission.*/
    qurl_ecode_t (*disconnect_func)(qurl_conn_t *conn_ptr, qurl_bit_t dead_connection);

    /*Multiplexing is called during the PROTOCONNECT phase, returning the fd set for use.*/
    int (*prot_getsock_func)(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);
    /*Multiplexing is called during the DOING phase, returning the fd set for use.*/
    int (*req_getsock_func)(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);
    /*Multiplexing is called during the DO_MORE phase, returning the fd set for use.*/
    int (*req_more_getsock_func)(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);
    /*Called from the multiplexing interface during the REQ_DONE, PERFORM, and WAITPERFORM phases to return the fd set. If this function is not set, qurl will use a generic default.*/
    int (*trans_data_getsock_func)(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);

    /*This function can perform various checks on the connection.*/
    unsigned int (*conn_check_func)(qurl_core_ctrl_t *core_ptr, qurl_conn_t *conn_ptr, unsigned int check_flags);

    /*Attach this transmission to this connection*/
    void (*attach_func)(qurl_core_ctrl_t *core_ptr, qurl_conn_t *conn_ptr);

    int          def_port; /* default port */
    qurl_prot_t  protocol; /*Protocol Type*/
    qurl_prot_t  family;   /*Protocol Family*/
    unsigned int flags;    /*Additional specific features*/
};
typedef struct qurl_port_prot_s qurl_port_prot_t;

#endif /* Head define end*/
