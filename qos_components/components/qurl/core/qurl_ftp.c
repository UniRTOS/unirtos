/*
 * Portions of this file are derived from curl/libcurl's lib/ftp.c.
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
/** @file         qurl_ftp.c
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-11-28 20:34:04
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
#include "qurl_port.h"
#include "qurl_select.h"
#include "qurl_parsedate.h"

#if QURL_CFG_ENABLE_FTP

#include "qurl_ftp_def.h"
#include "qurl_pingpong.h"

#if (QURL_CFG_ENABLE_FS == 0)
#include "qurl_default_fs.h"
#endif

/*Macro Configuration*/
#define QURL_FTP_INLINE inline

/*Data Structure*/
/** FTP active mode command*/
typedef enum
{
    QURL_FTP_CMD_PORT_EPRT = 0,
    QURL_FTP_CMD_PORT_PORT,
    QURL_FTP_CMD_PORT_DONE,
} qurl_ftp_cmd_port_e;

static qurl_ecode_t ftp_setup_connect(qurl_conn_t *conn_ptr);
static qurl_ecode_t ftp_send_prot_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t ftp_prot_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t ftp_req(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t ftp_reqing(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t ftp_req_more(qurl_conn_t *conn_ptr, int *completep_ptr);
static qurl_ecode_t ftp_trans_data(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t ftp_trans_done(qurl_conn_t *conn_ptr, qurl_ecode_t ecode, qurl_bit_t premature);
static qurl_ecode_t ftp_disconnect(qurl_conn_t *conn_ptr, qurl_bit_t dead_connection);
static int          ftp_port_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);
static int          ftp_port_req_more_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);
static int          ftp_trans_data_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);

static qurl_ecode_t                 ftp_state_type_resp(qurl_conn_t *conn_ptr, int ftpcode, qurl_ftp_state_e instate);
static QURL_FTP_INLINE qurl_ecode_t ftp_state_quote(qurl_conn_t *conn_ptr, qurl_bit_t init, qurl_ftp_state_e instate);
static qurl_ecode_t                 ftp_block_statemach(qurl_conn_t *conn_ptr);
static qurl_ecode_t                 ftp_reqphase_done(qurl_conn_t *conn_ptr, qurl_bit_t connected);
static qurl_ecode_t                 ftp_get_respons(qurl_conn_t *conn_ptr, long *nread_ptr, int *ftp_code_ptr);
static qurl_ecode_t                 ftp_train_logo_judgment(qurl_conn_t *conn_ptr, qurl_core_ctrl_t *core_ptr, qurl_bit_t premature);
/*Function Declaration*/
const qurl_port_prot_t g_qurl_port_prot_ftp = {
    .scheme = "ftp",
    .setup_connect_func = ftp_setup_connect,
    .send_prot_connect_func = ftp_send_prot_connect,
    .prot_connect_func = ftp_prot_connect,
    .req_func = ftp_req,
    .reqing_func = ftp_reqing,
    .req_more_func = ftp_req_more,
    .trans_data_func = ftp_trans_data,
    .trans_done_func = ftp_trans_done,
    .disconnect_func = ftp_disconnect,
    .prot_getsock_func = ftp_port_getsock,
    .req_getsock_func = ftp_port_getsock,
    .req_more_getsock_func = ftp_port_req_more_getsock,
    .trans_data_getsock_func = ftp_trans_data_getsock,
    .conn_check_func = QOSA_NULL,
    .attach_func = QOSA_NULL,
    .def_port = QURL_CFG_PORT_FTP_DEFAULT,
    .protocol = QURL_PROT_TYPE_FTP,
    .family = QURL_PROT_TYPE_FTP,
    .flags = QURL_PROT_FLAGS_CREDSPERREQUEST | QURL_PROT_FLAGS_USERPWDCTRL,
};

const qurl_port_prot_t g_qurl_port_prot_ftps = {
    .scheme = "ftps",
    .setup_connect_func = ftp_setup_connect,
    .send_prot_connect_func = ftp_send_prot_connect,
    .prot_connect_func = ftp_prot_connect,
    .req_func = ftp_req,
    .reqing_func = ftp_reqing,
    .req_more_func = ftp_req_more,
    .trans_data_func = ftp_trans_data,
    .trans_done_func = ftp_trans_done,
    .disconnect_func = ftp_disconnect,
    .prot_getsock_func = ftp_port_getsock,
    .req_getsock_func = ftp_port_getsock,
    .req_more_getsock_func = ftp_port_req_more_getsock,
    .trans_data_getsock_func = ftp_trans_data_getsock,
    .conn_check_func = QOSA_NULL,
    .attach_func = QOSA_NULL,
    .def_port = QURL_CFG_PORT_FTPS_DEFAULT,
    .protocol = QURL_PROT_TYPE_FTPS,
    .family = QURL_PROT_TYPE_FTP,
    .flags = QURL_PROT_FLAGS_SSL | QURL_PROT_FLAGS_CREDSPERREQUEST | QURL_PROT_FLAGS_USERPWDCTRL,
};

/*Internal auxiliary functions*/
/** Check for a three-digit FTP status code at the beginning of the given string*/
#define FTP_STATUSCODE(line) (qurl_ctype_isdigit(line[0]) && qurl_ctype_isdigit(line[1]) && qurl_ctype_isdigit(line[2]))

/*Check the last line in the FTP server response*/
#define FTP_LASTLINE(line)   (FTP_STATUSCODE(line) && (' ' == line[3]))

static QURL_FTP_INLINE void ftp_state_change(qurl_ftp_ctrl_t *ftp_ctrl_ptr, qurl_ftp_state_e new_state)
{
    qurl_ftp_state_e old_state = ftp_ctrl_ptr->state;

    ftp_ctrl_ptr->state = new_state;

    QURL_UNUSED(old_state);
    qurl_logv("ftp state change:[%d]->[%d]\r\n", old_state, new_state);
}

static QURL_FTP_INLINE int twodigit(const char *p)
{
    return (p[0] - '0') * 10 + (p[1] - '0');
}

static QURL_FTP_INLINE qurl_bit_t ftp_213_date(const char *p, int *year, int *month, int *day, int *hour, int *minute, int *second)
{
    long len = qurl_strlen(p);

    if (len < 14)
    {
        return QOSA_FALSE;
    }
    *year = twodigit(&p[0]) * 100 + twodigit(&p[2]);
    *month = twodigit(&p[4]);
    *day = twodigit(&p[6]);
    *hour = twodigit(&p[8]);
    *minute = twodigit(&p[10]);
    *second = twodigit(&p[12]);

    if ((*month > 12) || (*day > 31) || (*hour > 23) || (*minute > 59) || (*second > 60))
    {
        return QOSA_FALSE;
    }
    return QOSA_TRUE;
}

static void ftp_free_dirs(qurl_ftp_ctrl_t *ftp_ctrl_ptr)
{
    if (ftp_ctrl_ptr->dirs_pptr)
    {
        int i = 0;
        for (i = 0; i < ftp_ctrl_ptr->dir_depth; i++)
        {
            qurl_safe_free(ftp_ctrl_ptr->dirs_pptr[i]);
            ftp_ctrl_ptr->dirs_pptr[i] = QOSA_NULL;
        }
        qurl_safe_free(ftp_ctrl_ptr->dirs_pptr);
        ftp_ctrl_ptr->dirs_pptr = QOSA_NULL;
        ftp_ctrl_ptr->dir_depth = 0;
    }
    qurl_safe_free(ftp_ctrl_ptr->file_ptr);

    /* no longer of any use */
    qurl_safe_free(ftp_ctrl_ptr->newhost_ptr);
}

/**
 * @brief Return TRUE when the TYPE command needs to be sent.
 *
 * @param conn_ptr
 * @param ascii_wanted
 * @return QURL_FTP_INLINE
 */
static QURL_FTP_INLINE qurl_bit_t ftp_need_type(qurl_conn_t *conn_ptr, qurl_bit_t ascii_wanted)
{
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    return ftp_ctrl_ptr->transfer_type != (ascii_wanted ? 'A' : 'I');
}

/**
 * @brief Set TYPE. We only handle ASCII or BINARY.
 *
 * @param conn_ptr
 * @param ascii
 * @param new_state
 * @return QURL_FTP_INLINE
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_nb_type(qurl_conn_t *conn_ptr, qurl_bit_t ascii, qurl_ftp_state_e new_state)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    char want = (char)(ascii ? 'A' : 'I');

    if (ftp_ctrl_ptr->transfer_type == want)
    {
        ftp_state_change(ftp_ctrl_ptr, new_state);
        return ftp_state_type_resp(conn_ptr, 200, new_state);
    }

    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "TYPE %c", want);
    if (ret == QURL_OK)
    {
        ftp_state_change(ftp_ctrl_ptr, new_state);

        /* keep track of our current transfer type */
        ftp_ctrl_ptr->transfer_type = want;
    }

    return ret;
}

static QURL_FTP_INLINE char *ftp_control_address(qurl_conn_t *conn_ptr)
{
    return conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].ip_str;
}

static QURL_FTP_INLINE qurl_bit_t ftp_match_pasv_6nums(const char *p, unsigned int *array) /* 6 numbers */
{
    int i = 0;
    for (i = 0; i < 6; i++)
    {
        unsigned long num = 0;
        char         *end_ptr = NULL;
        if (i)
        {
            if (*p != ',')
                return QOSA_FALSE;
            p++;
        }
        if (!qurl_ctype_isdigit(*p))
        {
            return QOSA_FALSE;
        }
        num = qurl_strtoul(p, &end_ptr, 10);
        if (num > 255)
        {
            return QOSA_FALSE;
        }
        array[i] = (unsigned int)num;
        p = end_ptr;
    }
    return QOSA_TRUE;
}

/**
 * @brief Initialize transmission
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_init_transfer(qurl_conn_t *conn_ptr)
{
    qurl_bit_t       connected = QOSA_FALSE;
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    qurl_logv("enter\r\n");

    if (conn_ptr->bits.ftp_use_data_tls != QOSA_FALSE)
    {
        conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].bits.is_tls = QOSA_TRUE;
    }
    /*[lzm][note]Whether active or passive, it's the client sending a client hello to the FTP server. The logic can be modified.*/
    ret = qurl_conn_connect(conn_ptr, QURL_SOCKET_INDEX_1, &connected);
    if (ret != QURL_OK || connected == QOSA_FALSE)
    {
        return ret;
    }

    pp_ctrl_ptr->pending_resp = QOSA_TRUE; /*Awaiting server response*/
    ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);

    return ret;
}

/**
 * @brief In active mode, call this function to accept the connection and close the listening socket after receiving a connection request from the TCP server.
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_accept_server_connect(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    /*Ensure listening is established before entering.*/
    ret = qurl_conn_accept(conn_ptr, QURL_SOCKET_INDEX_1);
    if (ret != QURL_OK)
    {
        return ret;
    }

    //Start establishing FTP data connection (active mode)
    //Received a connection request from the server and successfully created a new socket through accept.
    //Initialize index1_last_close_event
    //Subsequently, only need to monitor the connection closure status of this accepted sub-socket.
    conn_ptr->index1_last_close_event = 0;

    /*Close TCP server*/
    ret = qurl_conn_conn_free(conn_ptr, QURL_SOCKET_SERVER_INDEX_0);
    if (ret != QURL_OK)
    {
        return ret;
    }

    return ret;
}

/**
 * @brief In active mode, the function checks whether the data connection has been established, while also checking the control connection for negative responses regarding connection failure.
 *
 * @param conn_ptr
 * @param received_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_received_server_connect(qurl_conn_t *conn_ptr, qurl_bit_t *received_ptr)
{
    char             chr = 0x00;
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;
    long             pong_len = 0;
    long             nread = 0;
    int              ftpcode = 0;

    qurl_logd("enter\r\n");

    /*Check whether the server control connection response result code is received*/
    pong_len = 1;
    qurl_dbuf_read_data_try(pp_ctrl_ptr->ping_cache, (void *)&chr, &pong_len, 0);

    if (chr > '3')
    {
        ftp_get_respons(conn_ptr, &nread, &ftpcode);
        return QURL_ECODE_FTP_ACCEPT_ERR;
    }

    /*First, check the data connection listener*/
    ret = qurl_socket_check(conn_ptr->conn_conn_server.sock_fd, (QURL_SELECT_BIT_READ | QURL_SELECT_BIT_ERROR), 0);
    if (ret < 0)
    {
        qurl_loge("socket faild:0x%x\r\n", ret);
        return QURL_ECODE_FTP_ACCEPT_ERR;
    }
    else if (ret > 0)
    {
        *received_ptr = QOSA_TRUE;
        return QURL_OK;
    }

    /*No data connection has been established yet, check if there is data on the control connection.*/
    ret = qurl_socket_check(conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd, (QURL_SELECT_BIT_READ | QURL_SELECT_BIT_ERROR), 0);
    if (ret < 0)
    {
        qurl_loge("socket faild:0x%x\r\n", ret);
        return QURL_ECODE_FTP_ACCEPT_ERR;
    }
    else if (ret > 0)
    {
        ftp_get_respons(conn_ptr, &nread, &ftpcode);
        if (ftpcode / 100 > 3)
        {
            return QURL_ECODE_FTP_ACCEPT_ERR;
        }
        return QURL_OK;
    }

    return QURL_OK;
}

/**
 * @brief In active mode, check whether a data connection has been established.
 *
 * @param conn_ptr
 * @param connected_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_allow_server_connect(qurl_conn_t *conn_ptr, qurl_bit_t *connected_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    qurl_logi("enter\r\n");

    *connected_ptr = QOSA_FALSE;

    ret = ftp_received_server_connect(conn_ptr, connected_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    if (*connected_ptr == QOSA_TRUE)
    {
        ret = ftp_accept_server_connect(conn_ptr);
        if (ret != QURL_OK)
        {
            return ret;
        }

        ret = ftp_init_transfer(conn_ptr);
        if (ret != QURL_OK)
        {
            return ret;
        }
    }

    return ret;
}

/**
 * @brief API called before closing the control connection, CMD:QUIT
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_quit(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    qurl_logi("enter\r\n");

    if (ftp_ctrl_ptr->bits.ctrl_valid)
    {
        /*Connection must be valid to send.*/
        ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "QUIT");
        if (ret == QURL_OK)
        {
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_QUIT);
            ret = ftp_block_statemach(conn_ptr);
        }
        else
        {
            ftp_ctrl_ptr->bits.ctrl_valid = QOSA_FALSE;
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
        }
    }

    return ret;
}

/** Send command*/
/**
 * @brief User login, username: CMD:USER
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_user(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "USER %s", conn_ptr->username_ptr ? conn_ptr->username_ptr : "");
    if (ret == QURL_OK)
    {
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_USER);
        return ret;
    }

    return ret;
}

/**
 * @brief User login, password: CMD:PASS
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_pass(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "PASS %s", conn_ptr->passwd_ptr ? conn_ptr->passwd_ptr : "");
    if (ret == QURL_OK)
    {
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_PASS);
        return ret;
    }

    return ret;
}

/**
 * @brief Account Information: CMD:ACCT
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_acct(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    if (core_ptr->usr_cfg.account_ptr != QOSA_NULL)
    {
        ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "ACCT %s", core_ptr->usr_cfg.account_ptr);
        if (ret == QURL_OK)
        {
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_ACCT);
            return ret;
        }
    }
    else
    {
        qurl_loge("ACCT requested but none available\r\n");
        ret = QURL_ECODE_FTP_LOGIN_DENIED; /*No account, login failed.*/
    }

    return ret;
}

/**
 * @brief Get current path: CMD:PWD
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_pwd(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "PWD");
    if (ret == QURL_OK)
    {
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_PWD);
        return ret;
    }

    return ret;
}

static QURL_FTP_INLINE qurl_ecode_t ftp_state_use_port(qurl_conn_t *conn_ptr, qurl_ftp_cmd_port_e ftpcmd)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    static const char   mode[][5] = {"EPRT", "PORT"};
    unsigned short      port_min = 0;
    unsigned short      port_max = 0;
    unsigned short      port = 0;
    char               *addr = QOSA_NULL;
    char               *host = QOSA_NULL;
    char               *ftpport_str = core_ptr->usr_cfg.ftp.ftp_port_ptr;
    struct addrinfo     hints = {0};
    struct addrinfo    *ai_ptr = QOSA_NULL;
    qurl_addrinfo_t    *rp_ptr = QOSA_NULL;
    qurl_sockaddr_t    *local_sa_ptr = QOSA_NULL;
    qurl_sockaddr_ex_t  ctrl_local_ex_sa = {0};
    qurl_sockaddr_in_t *ctrl_local_sai4 = (qurl_sockaddr_in_t *)&ctrl_local_ex_sa.addr_u.sai;
#if QURL_CFG_ENABLE_IPV6
    qurl_sockaddr_in6_t *ctrl_local_sai6 = (qurl_sockaddr_in6_t *)&ctrl_local_ex_sa.addr_u.sai6;
#endif /* QURL_CFG_ENABLE_IPV6 */
    int  count = 0;
    char ip_str[QURL_INET6_ADDRSTRLEN + 1] = {0};

    qurl_logi("enter\r\n");

    /*1. Determine the local server address and port number. The specified format to receive is: (ipv4|ipv6|domain|interface)?(:port(-range)?)?*/
    /*2. Create a socket. Specify the local address and port number. Listen on the socket.*/
    /*3. Send EPRT or PORT command*/

    /*1. Determine the local server address and port number. The specified format to receive is: (ipv4|ipv6|domain|interface)?(:port(-range)?)?*/
    if (ftpport_str && qurl_strlen(ftpport_str) > 1)
    {
#if QURL_CFG_ENABLE_IPV6
        long addrlen = INET6_ADDRSTRLEN > qurl_strlen(ftpport_str) ? INET6_ADDRSTRLEN : qurl_strlen(ftpport_str);
#else
        qosa_size_t addrlen = INET_ADDRSTRLEN > qurl_strlen(ftpport_str) ? INET_ADDRSTRLEN : qurl_strlen(ftpport_str);
#endif
        char *ip_start = ftpport_str;
        char *ip_end = QOSA_NULL;
        char *port_start = QOSA_NULL;
        char *port_sep = QOSA_NULL;

        addr = qurl_malloc(addrlen + 1);
        if (!addr)
        {
            ret = QURL_ECODE_NO_MEMORY;
            goto exit;
        }
        addr[addrlen] = 0x00;

#if QURL_CFG_ENABLE_IPV6
        if (*ftpport_str == '[')
        {
            /* [ipv6]:port(-range) */
            ip_start = ftpport_str + 1;
            ip_end = qurl_strchr(ftpport_str, ']');
            if (ip_end)
            {
                qurl_strncpy(addr, ip_start, ip_end - ip_start);
            }
        }
        else
#endif
            if (*ftpport_str == ':')
        {
            /* :port */
            ip_end = ftpport_str;
        }
        else
        {
            ip_end = qurl_strchr(ftpport_str, ':');
            if (ip_end)
            {
                /* either ipv6 or (ipv4|domain|interface):port(-range) */
#if QURL_CFG_ENABLE_IPV6
                if (qurl_inet_pton(AF_INET6, ftpport_str, &ctrl_local_sai6->sin6_addr) == 1)
                {
                    /* ipv6 */
                    port_min = port_max = 0;
                    qurl_strcpy(addr, ftpport_str);
                    ip_end = QOSA_NULL; /* this got no port ! */
                }
                else
#endif
                {
                    /* (ipv4|domain|interface):port(-range) */
                    qurl_strncpy(addr, ftpport_str, ip_end - ip_start);
                }
            }
            else
            {
                /* ipv4|interface */
                qurl_strcpy(addr, ftpport_str);
            }
        }

        /* parse the port */
        if (ip_end)
        {
            port_start = qurl_strchr(ip_end, ':');
            if (port_start)
            {
                port_min = (unsigned short)qurl_strtoul(port_start + 1, NULL, 10);
                port_sep = qurl_strchr(port_start, '-');
                if (port_sep)
                {
                    port_max = (unsigned short)qurl_strtoul(port_sep + 1, NULL, 10);
                }
                else
                {
                    port_max = port_min;
                }
            }
        }

        /*Correct the following errors:
		 *  :1234-1230
		 *  :-4711
		 */
        if (port_min > port_max)
        {
            port_min = port_max = 0;
        }

        if (*addr != '\0')
        {
            /*[lzm][todo]Attempt to obtain the address for the given interface name*/
            host = addr;
        }
        else
        {
            /*Only the port number is given, so the address defaults to the host.*/
            host = QOSA_NULL;
        }
    }

    if (host == QOSA_NULL)
    {
        /*Using the control connection*/

        const char *ctrl_pcal_ip_str = QOSA_NULL;

        qurl_socklen_t addr_len = 0;

        if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_addrinfo_ptr->ai_family == AF_INET)
        {
            addr_len = sizeof(qurl_sockaddr_in_t);
            ret = qurl_socket_getsockname(conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd, (qurl_sockaddr_t *)ctrl_local_sai4, &addr_len);
            if (ret == -1)
            {
                ret = QURL_ECODE_FTP_CANT_GET_HOST;
                goto exit;
            }
            ctrl_pcal_ip_str = qurl_inet_ntop(AF_INET, &(ctrl_local_ex_sa.addr_u.sai.sin_addr), ip_str, QURL_INET6_ADDRSTRLEN);
        }
#if QURL_CFG_ENABLE_IPV6
        else
        {
            addr_len = sizeof(qurl_sockaddr_in6_t);
            ret = qurl_socket_getsockname(conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd, (qurl_sockaddr_t *)ctrl_local_sai6, &addr_len);
            if (ret == -1)
            {
                ret = QURL_ECODE_FTP_CANT_GET_HOST;
                goto exit;
            }
            ctrl_pcal_ip_str = qurl_inet_ntop(AF_INET6, &(ctrl_local_ex_sa.addr_u.sai6.sin6_addr), ip_str, QURL_INET6_ADDRSTRLEN);
        }
#endif
        host = (char *)ctrl_pcal_ip_str;
        if (host == QOSA_NULL)
        {
            ret = QURL_ECODE_FTP_CANT_GET_HOST;
            goto exit;
        }
    }

    /*Parsing*/
    /*Parse this connection address: blocking mode*/
    hints.ai_family = 0; /*[lzm][todo]To be assigned*/
    hints.ai_socktype = SOCK_STREAM;
#if !defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) && !defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
    //Only for the ASR1903 platform and lwip1.4.1 lwip such as 1606. Once ASR1903 is upgraded to lwip2.0 later, it can be changed back to support.
    hints.ai_flags = AI_ADDRCONFIG;
#endif /* !defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) && !defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC) */
    ret = qurl_socket_resolve(host, QOSA_NULL, &hints, &ai_ptr, core_ptr->usr_cfg.nw_id);
    if (ret != 0)
    {
        ret = QURL_ECODE_FTP_CANT_GET_HOST;
        goto exit;
    }
    if (ai_ptr == QOSA_NULL)
    {
        ret = QURL_ECODE_FTP_CANT_GET_HOST;
        goto exit;
    }
    for (rp_ptr = ai_ptr; rp_ptr != QOSA_NULL; rp_ptr = rp_ptr->ai_next)
    {
        char  ipstr[QURL_INET6_ADDRSTRLEN] = {0};
        void *ai_addr = QOSA_NULL;
        count++;
        if (rp_ptr->ai_family == AF_INET)
        {  // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp_ptr->ai_addr;
            ai_addr = &(ipv4->sin_addr);
        }
        else
        {  // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp_ptr->ai_addr;
            ai_addr = &(ipv6->sin6_addr);
        }
        qurl_inet_ntop(rp_ptr->ai_family, ai_addr, ipstr, sizeof(ipstr));
        qurl_logd("dns success[%d][%s]\n", count, ipstr);
    }
    conn_ptr->conn_conn_server.addrinfo_cnt = count;
    if (conn_ptr->conn_conn_server.addrinfo_ptr != QOSA_NULL)
    {
        qurl_socket_addinfo_free(conn_ptr->conn_conn_server.addrinfo_ptr);
        conn_ptr->conn_conn_server.addrinfo_ptr = QOSA_NULL;
    }
    conn_ptr->conn_conn_server.addrinfo_ptr = ai_ptr;
    conn_ptr->conn_conn_server.sock_addrinfo_ptr = QOSA_NULL;

    ai_ptr = QOSA_NULL;
    qurl_safe_free(addr);
    addr = QOSA_NULL;
    host = QOSA_NULL;
    qurl_memset(ip_str, 0x00, sizeof(ip_str));

    /*2. Create socket, bind, listen*/
    /** Configuration*/
    conn_ptr->conn_conn_server.port_min = port_min;
    conn_ptr->conn_conn_server.port_max = port_max;
    qurl_logd("port_min:%d, port_max:%d\r\n", port_min, port_max);
    /** Create and bind listener*/
    ret = qurl_conn_listen(conn_ptr, 1); /*Only listen to one data connection*/
    if (ret != QURL_OK)
    {
        goto exit;
    }

    /*3. Send FTP Command*/
    port = conn_ptr->conn_conn_server.local_port;
    host = conn_ptr->conn_conn_server.ip_str;
    if (host)
    {
        qurl_memcpy(ip_str, host, sizeof(ip_str) - 1);
    }
    host = QOSA_NULL;
    local_sa_ptr = conn_ptr->conn_conn_server.sock_addrinfo_ptr->ai_addr;
    for (; ftpcmd != QURL_FTP_CMD_PORT_DONE; ftpcmd++)
    {
        if (!conn_ptr->bits.ftp_use_eprt && (QURL_FTP_CMD_PORT_EPRT == ftpcmd))
        {
            /*Use PORT when EPRT is not enabled*/
            continue;
        }

        if ((QURL_FTP_CMD_PORT_PORT == ftpcmd) && local_sa_ptr->sa_family != AF_INET)
        {
            /* PORT is IPv4 only */
            continue;
        }

        if (QURL_FTP_CMD_PORT_EPRT == ftpcmd)
        {
            /*
			 * Two fine examples from RFC2428;
			 *
			 * EPRT |1|132.235.1.2|6275|
			 *
			 * EPRT |2|1080::8:800:200C:417A|5282|
			 */

            ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "%s |%d|%s|%hu|", mode[ftpcmd], local_sa_ptr->sa_family == AF_INET ? 1 : 2, ip_str, port);
            if (ret != QURL_OK)
            {
                qurl_loge("Failure sending EPRT command: %x\r\n", ret);
                goto exit;
            }
            break;
        }
        if (QURL_FTP_CMD_PORT_PORT == ftpcmd)
        {
            /* large enough for [IP address],[num],[num] */
            char  target[sizeof(ip_str) + 20] = {0};
            char *source = ip_str;
            char *dest = target;

            /* translate x.x.x.x to x,x,x,x */
            while (source && *source)
            {
                if (*source == '.')
                {
                    *dest = ',';
                }
                else
                {
                    *dest = *source;
                }
                dest++;
                source++;
            }
            *dest = 0;
            qurl_snprintf(dest, 20, ",%d,%d", (int)(port >> 8), (int)(port & 0xff));

            ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "%s %s", mode[ftpcmd], target);
            if (ret != QURL_OK)
            {
                qurl_loge("Failure sending PORT command: %x\r\n", ret);
                goto exit;
            }
            break;
        }
    }

    /*Store the command to be sent*/
    ftp_ctrl_ptr->count1 = ftpcmd;
    ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_PORT);

    return ret;

exit:
    if (ret != QURL_OK)
    {
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
    }
    if (conn_ptr->conn_conn_server.addrinfo_ptr != QOSA_NULL)
    {
        qurl_socket_addinfo_free(conn_ptr->conn_conn_server.addrinfo_ptr);
        conn_ptr->conn_conn_server.addrinfo_ptr = QOSA_NULL;
    }
    if (conn_ptr->conn_conn_server.sock_fd != QURL_SOCKET_BAD)
    {
        qurl_socket_close(conn_ptr->conn_conn_server.sock_fd);
        conn_ptr->conn_conn_server.sock_fd = QURL_SOCKET_BAD;
    }
    qurl_safe_free(addr);

    qurl_loge("exit:%x\r\n", ret);
    return ret;
}

static QURL_FTP_INLINE qurl_ecode_t ftp_state_use_pasv(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;
    int               modeoff = 0;
    static const char mode[][5] = {"EPSV", "PASV"};

    /**
	 * PASV (Passive Mode) is the traditional passive mode defined in RFC959.
	 *	After using the PASV command, the server will return a response in the form of "227 Entering Passive Mode (a1,a2,a3,a4,p1,p2)".
	 *	where a1.a2.a3.a4 is the server's IP address, and p1.p2 is the data port on which the server listens.
	 *
	 *	LPSV (Long Passive Mode) is an extended passive mode defined in RFC1639.
	 *	After using the LPSV command, the server will return a response in the form of "228 Entering Long Passive Mode (4,4,a1,a2,a3,a4,2,p1,p2)".
	 *	where a1.a2.a3.a4 is the IPv4 address of the server, and p1.p2 is the data port on which the server is listening.
	 *
	 *	EPSV (Extended Passive Mode) is a more flexible passive mode defined in RFC2428.
	 *	After using the EPSV command, the server will return a response in the form of "229 Entering Extended Passive Mode (|||port|)".
	 *	where port is the data port number that the server listens on.
	 *	where "|||" indicates that the server will listen on all available IPv4 or IPv6 addresses. Obtained from the control connection.
	 *
	 */

#if QURL_CFG_ENABLE_IPV6
    if (!conn_ptr->bits.ftp_use_epsv && conn_ptr->bits.ipv6)
        /*If the current server does not support the EPSV command, but the connection is to an IPv6 host, the client will ignore the request and re-enable the EPSV command.
This is because IPv6 only supports the EPSV command and does not support the PASV command.*/
        conn_ptr->bits.ftp_use_epsv = QOSA_TRUE;
#endif

    modeoff = conn_ptr->bits.ftp_use_epsv ? 0 : 1;

    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "%s", mode[modeoff]);
    if (ret == QURL_OK)
    {
        ftp_ctrl_ptr->count1 = modeoff;
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_PASV);
        qurl_logd("Connect data stream passively\r\n");
    }

    return ret;
}

/**
 * @brief Transmission Preparation
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_prepare_transfer(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    if (ftp_core_ptr->transfer != QURL_PP_TRANSFER_BODY)
    {
        /*User-defined data to be sent before transmission*/
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_RETR_PREQUOTE);
        ret = ftp_state_quote(conn_ptr, QOSA_TRUE, QURL_FTP_STATE_RETR_PREQUOTE);
    }
    else if (core_ptr->usr_cfg.bits.ftp_use_port)
    {
        /*Active Mode*/
        ret = ftp_state_use_port(conn_ptr, QURL_FTP_CMD_PORT_EPRT);
    }
    else
    {
        /*Default selection: Passive mode*/
        /** Whether to use the PRET command to inform the server to prepare for data reception. The PRET command is used before PASV.*/
        if (core_ptr->usr_cfg.bits.ftp_use_pret)
        {
            if (!ftp_ctrl_ptr->file_ptr)
            {
                ret = qurl_pp_sendf(
                    conn_ptr,
                    pp_ctrl_ptr,
                    "PRET %s",
                    core_ptr->usr_cfg.customrequest_ptr ? core_ptr->usr_cfg.customrequest_ptr : (core_ptr->usr_cfg.bits.list_only ? "NLST" : "LIST")
                );
            }
            else if (core_ptr->cfg.bits.upload)
            {
                /*Upload data*/
                ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "PRET STOR %s", ftp_ctrl_ptr->file_ptr);
            }
            else
            {
                /*Download data*/
                ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "PRET RETR %s", ftp_ctrl_ptr->file_ptr);
            }

            if (ret == QURL_OK)
            {
                ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_PRET);
            }
        }
        else
        {
            ret = ftp_state_use_pasv(conn_ptr);
        }
    }

    return ret;
}

/**
 * @brief REST：CMD:REST
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_rest(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    if ((ftp_core_ptr->transfer != QURL_PP_TRANSFER_BODY) && (ftp_ctrl_ptr->file_ptr))
    {
        /*If a request similar to "head" is being made (for a file)*/

        /*Determine whether the server can respond to REST commands, thereby determining if it supports range requests.*/

        ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "REST %d", 0);
        if (ret == QURL_OK)
        {
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_REST);
            return ret;
        }
    }
    else
    {
        ret = ftp_state_prepare_transfer(conn_ptr);
    }

    return ret;
}

/**
 * @brief Get file SIZE: CMD:SIZE
 * The SIZE command is not specified in RFC 959, so many servers may not support it!
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_size(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    if ((ftp_core_ptr->transfer == QURL_PP_TRANSFER_INFO) && (ftp_ctrl_ptr->file_ptr))
    {
        /*If a request similar to "head" is being made (for a file)*/

        /*We know that ftp_ctrl_ptr->file_ptr is a valid pointer to a filename.*/

        ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "SIZE %s", ftp_ctrl_ptr->file_ptr);
        if (ret == QURL_OK)
        {
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_SIZE);
            return ret;
        }
    }
    else
    {
        ret = ftp_state_rest(conn_ptr);
    }

    return ret;
}

/**
 * @brief Set the data transmission type: CMD:TYPE
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_type(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;

    /*If we choose NOBODY and HEADER, it means we only need file information. In FTP, this is typically the file size and date.*/
    if (core_ptr->usr_cfg.bits.opt_nobody && ftp_ctrl_ptr->file_ptr && ftp_need_type(conn_ptr, core_ptr->usr_cfg.bits.prefer_ascii))
    {
        /*The SIZE command is not specified in RFC 959, so many servers may not support it! However, this is the only method we have to obtain the file size!*/
        /*If the types are inconsistent, then it is necessary to issue the TYPE command to determine the transmission type.*/
        ftp_core_ptr->transfer = QURL_PP_TRANSFER_INFO;

        ret = ftp_nb_type(conn_ptr, core_ptr->usr_cfg.bits.prefer_ascii, QURL_FTP_STATE_TYPE);
        if (ret != QURL_OK)
        {
            return ret;
        }
    }
    else
    {
        ret = ftp_state_size(conn_ptr);
    }

    return ret;
}

/**
 * @brief Get the modification date and time of a file on the server: CMD:MDTM
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_mdtm(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    /*This command is triggered when obtaining the file update time or when there is a local file time limit set.*/
    if ((core_ptr->usr_cfg.bits.get_filetime || core_ptr->usr_cfg.timecondition) && ftp_ctrl_ptr->file_ptr)
    {
        ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "MDTM %s", ftp_ctrl_ptr->file_ptr);
        if (ret == QURL_OK)
        {
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_MDTM);
            return ret;
        }
    }
    else
    {
        ret = ftp_state_type(conn_ptr);
    }

    return ret;
}

/**
 * @brief Get directory file list: CMD:LIST
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_list(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    char             *lst_arg_ptr = QOSA_NULL;
    char             *cmd_ptr = QOSA_NULL;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    if ((core_ptr->usr_cfg.ftp.ftp_filemethod == QURL_FTP_FILEMETHOD_NOCWD) && ftp_core_ptr->path)
    {
        const char *slash_pos_ptr = QOSA_NULL;
        char       *raw_path_ptr = QOSA_NULL;
        ret = qurl_url_decode(ftp_core_ptr->path, 0, &raw_path_ptr, QOSA_NULL, QURL_URL_REJECT_CTRL);
        if (ret)
        {
            return ret;
        }

        /*If the format is dir/file, remove the file part; otherwise, remove the trailing slash of dir/dir/ (except for absolute paths /).*/
        slash_pos_ptr = qurl_strrchr(raw_path_ptr, '/');
        if (slash_pos_ptr)
        {
            long n = slash_pos_ptr - raw_path_ptr;
            if (n == 0)
            {
                ++n;
            }

            lst_arg_ptr = raw_path_ptr;
            lst_arg_ptr[n] = '\0';
        }
        else
        {
            qurl_free(raw_path_ptr);
        }
    }

    qurl_asprintf(
        &cmd_ptr,
        "%s%s%s",
        core_ptr->usr_cfg.customrequest_ptr ? core_ptr->usr_cfg.customrequest_ptr : (core_ptr->usr_cfg.bits.list_only ? "NLST" : "LIST"),
        lst_arg_ptr ? " " : "",
        lst_arg_ptr ? lst_arg_ptr : ""
    );
    qurl_free(lst_arg_ptr);

    if (!cmd_ptr)
    {
        return QURL_ECODE_NO_MEMORY;
    }

    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "%s", cmd_ptr);
    qurl_free(cmd_ptr);
    if (!ret)
    {
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_LIST);
    }

    return ret;
}

static QURL_FTP_INLINE qurl_ecode_t ftp_state_retr_prequote(qurl_conn_t *conn_ptr)
{
    /* We've sent the TYPE, now we must send the list of prequote strings */
    return ftp_state_quote(conn_ptr, QOSA_TRUE, QURL_FTP_STATE_RETR_PREQUOTE);
}

static QURL_FTP_INLINE qurl_ecode_t ftp_state_stor_prequote(qurl_conn_t *conn_ptr)
{
    /* We've sent the TYPE, now we must send the list of prequote strings */
    return ftp_state_quote(conn_ptr, QOSA_TRUE, QURL_FTP_STATE_STOR_PREQUOTE);
}

/**
 * @brief Preparation function before uploading data
 * Sent after the RETR and STOR_PREQUOTE commands.
 * @param conn_ptr
 * @param sizechecked[in] Whether the file SIZE has been obtained
 * @return QURL_FTP_INLINE
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_ul_setup(qurl_conn_t *conn_ptr, qurl_bit_t sizechecked)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;
    qurl_bit_t        append = core_ptr->cfg.bits.remote_append;

    /*[lzm][todo]QURL_FTP_STATE_STOR_SIZE to be developed.*/
    (void)(sizechecked);

    /*1. Continue uploading CMD:APPE*/
    /*2. Direct upload CMD: STOR*/
    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, append ? "APPE %s" : "STOR %s", ftp_ctrl_ptr->file_ptr);
    if (ret == QURL_OK)
    {
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOR);
    }

    return ret;
}

/**
 * @brief Change working directory: CMD:CWD
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_cwd(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    if (ftp_ctrl_ptr->bits.cwd_done)
    {
        /*No need for cwd anymore*/
        ret = ftp_state_mdtm(conn_ptr);
    }
    else
    {
        ftp_ctrl_ptr->count2 = 0; /* count2 counts failed CWDs */

        if (conn_ptr->bits.is_reuse && ftp_ctrl_ptr->entrypath_ptr &&
            /*When we have an absolute path, there is no need to use entrypath.*/
            !(ftp_ctrl_ptr->dir_depth && ftp_ctrl_ptr->dirs_pptr[0][0] == '/'))
        {
            /*This is a reused connection. Since we need to switch to the directory where the transfer is taking place, we must first return to the original directory location after login:*/
            ftp_ctrl_ptr->cwd_count = 0; /*We will count this as the first path, then add 1 to all subsequent paths in the ftp_ctrl_ptr->dirs_pptr[] array.*/
            ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "CWD %s", ftp_ctrl_ptr->entrypath_ptr);
            if (ret == QURL_OK)
            {
                ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_CWD);
            }
        }
        else
        {
            if (ftp_ctrl_ptr->dir_depth)
            {
                ftp_ctrl_ptr->cwd_count = 1;
                /*After sending the first CWD command, the remaining commands will be sent upon receiving the CWD response...*/
                ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "CWD %s", ftp_ctrl_ptr->dirs_pptr[ftp_ctrl_ptr->cwd_count - 1]);
                if (ret == QURL_OK)
                {
                    ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_CWD);
                }
            }
            else
            {
                /* No CWD necessary */
                ret = ftp_state_mdtm(conn_ptr);
            }
        }
    }

    return ret;
}

/**
 * @brief Data Security Level Command: CMD:PROT
 *
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_prot(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "PROT %c", core_ptr->usr_cfg.use_tls == QURL_USETLS_CONTROL ? 'C' : 'P');
    if (ret == QURL_OK)
    {
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_PROT);
        return ret;
    }

    return ret;
}

/**
 * @brief Retrieve files from the server: CMD:RETR command
 *
 * @param filesize[in] file size
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_retr(qurl_conn_t *conn_ptr, long filesize)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;
    long              download_len = 0;

    if (core_ptr->usr_cfg.max_filesize && (filesize > core_ptr->usr_cfg.max_filesize))
    {
        return QURL_ECODE_FILESIZE_EXCEEDED;
    }
    download_len = filesize;

    if (core_ptr->cfg.resume_from)
    {
        core_ptr->info.start_pos = core_ptr->cfg.resume_from;
        if (filesize == -1)
        {
            qurl_logd("ftp server doesn't support SIZE\r\n");
        }
        else
        {
            /*Get the file SIZE, then let's confirm if there is still a portion of the file data that needs to be downloaded.*/
            if (core_ptr->cfg.resume_from < 0)
            {
                if (filesize < -core_ptr->cfg.resume_from)
                {
                    return QURL_ECODE_DOWNLOAD_RESUME_ERR;
                }
                download_len = -core_ptr->cfg.resume_from;
                core_ptr->cfg.resume_from = filesize + core_ptr->cfg.resume_from; /*Updated to positive*/
            }
            else
            {
#ifdef CONFIG_QCM_QURL_FTP_OFFSET_SETTING
                if (filesize <= core_ptr->cfg.resume_from)
#else
                if (filesize < core_ptr->cfg.resume_from)
#endif
                {
                    return QURL_ECODE_DOWNLOAD_RESUME_ERR;
                }
                download_len = filesize - core_ptr->cfg.resume_from;
            }
        }

        // or core_ptr->trans.download_len_max = download_len;
        if (core_ptr->trans.download_len_max < 0)
        {
            core_ptr->trans.download_len_max = download_len;
        }
        else
        {
            /*[lzm][note]Analyze whether += is applicable here*/
            core_ptr->trans.download_len_max += download_len;
        }
        if (download_len == 0)
        {
            /*No data needs to be downloaded*/
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
        }
        ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "REST %d", core_ptr->cfg.resume_from);
        if (ret == QURL_OK)
        {
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_RETR_REST);
        }
    }
    else
    {
        ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "RETR %s", ftp_ctrl_ptr->file_ptr);
        if (ret == QURL_OK)
        {
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_RETR);
        }
    }

    return ret;
}

/**
 * @brief User-defined commands organized by the user, which can also be standard commands: CMD:QUOTE
 *
 * @param conn_ptr[in]
 * @param init[in] Whether it is the first time to execute the quote series commands of this FTP service
 * @param instate[in]
 * @return qurl_ecode_t
 */
static QURL_FTP_INLINE qurl_ecode_t ftp_state_quote(qurl_conn_t *conn_ptr, qurl_bit_t init, qurl_ftp_state_e instate)
{
    qurl_bit_t         quote_flag = QOSA_FALSE; /*The flag indicating a quote needs to be sent*/
    qurl_ecode_t       ret = QURL_OK;
    qurl_core_ctrl_t  *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t   *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_ftp_core_t   *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;
    qurl_pp_ctrl_t    *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;
    qurl_slist_ctrl_t *item_ptr = QOSA_NULL;

    switch (instate)
    {
        case QURL_FTP_STATE_QUOTE:
        default: {
            item_ptr = core_ptr->usr_cfg.ftp.quote_slist; /*Custom command table sent after establishing a TCP connection*/
        }
        break;
        case QURL_FTP_STATE_RETR_PREQUOTE:
        case QURL_FTP_STATE_STOR_PREQUOTE: {
            item_ptr = core_ptr->usr_cfg.ftp.prequote_slist;
        }
        break;
        case QURL_FTP_STATE_POSTQUOTE: {
            item_ptr = core_ptr->usr_cfg.ftp.postquote_slist;
        }
        break;
    }

    /**
	 * count1 is used as an iterator to traverse the commands to be sent.
	 *
	 * count2 is used to store information about whether command failures are allowed.
	 */
    if (init)
    {
        ftp_ctrl_ptr->count1 = 0;
    }
    else
    {
        ftp_ctrl_ptr->count1++;
    }

    if (item_ptr)
    {
        int i = 0;
        /*Skip already sent commands*/
        while ((i < ftp_ctrl_ptr->count1) && item_ptr)
        {
            item_ptr = item_ptr->next;
            i++;
        }
        if (item_ptr)
        {
            char *cmd_ptr = item_ptr->data;
            if (cmd_ptr[0] == '*')
            {
                cmd_ptr++;
                ftp_ctrl_ptr->count2 = 1; /*Allow sending failure*/
            }
            else
            {
                ftp_ctrl_ptr->count2 = 0; /*If sending fails, cancel the connection.*/
            }

            ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "%s", cmd_ptr);
            if (ret != QURL_OK)
            {
                return ret;
            }
            ftp_state_change(ftp_ctrl_ptr, instate);
            quote_flag = QOSA_TRUE;
        }
    }

    if (!quote_flag)
    {
        /*No more quote commands need to be sent, continue*/
        switch (instate)
        {
            case QURL_FTP_STATE_QUOTE:
            default:
                ret = ftp_state_cwd(conn_ptr);
                break;
            case QURL_FTP_STATE_RETR_PREQUOTE: {
                if (ftp_core_ptr->transfer != QURL_PP_TRANSFER_BODY)
                {
                    ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
                }
                else
                {
                    if (ftp_ctrl_ptr->known_filesize != -1)
                    {
                        ret = ftp_state_retr(conn_ptr, ftp_ctrl_ptr->known_filesize);
                    }
                    else
                    {
                        if (core_ptr->usr_cfg.bits.ignore_cl || core_ptr->cfg.bits.prefer_ascii)
                        {
                            /**
							 * 'ignorecl' is used to support downloading files that are in the process of growing.
							 * It prevents the state machine from requesting the file size from the server.
							 * If the file size is unknown, the download will continue until the server terminates it; otherwise, if the number of bytes received exceeds the reported file size, the client will stop.
							 * Set option QURL_OPT_IGNORE_CONTENT_LENGTH to 1 to enable this behavior.
							 *  Additionally: For "TYPE A" transfer requests, the size is meaningless because the server does not report the converted size. Therefore, this step is skipped.
							 */
                            ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "RETR %s", ftp_ctrl_ptr->file_ptr);
                            if (ret == QURL_OK)
                            {
                                ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_RETR);
                            }
                        }
                        else
                        {
                            ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "SIZE %s", ftp_ctrl_ptr->file_ptr);
                            if (ret == QURL_OK)
                            {
                                ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_RETR_SIZE);
                            }
                        }
                    }
                }
            }
            break;
            case QURL_FTP_STATE_STOR_PREQUOTE: {
                ret = ftp_state_ul_setup(conn_ptr, QOSA_FALSE);
            }
            break;
            case QURL_FTP_STATE_POSTQUOTE:
                break;
        }
    }

    return ret;
}

static qurl_ecode_t ftp_send_quote(qurl_conn_t *conn_ptr, qurl_slist_ctrl_t *quote_slist)
{
    qurl_ecode_t       ret = QURL_OK;
    qurl_slist_ctrl_t *item_ptr = QOSA_NULL;
    qurl_ftp_ctrl_t   *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t    *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    item_ptr = quote_slist;
    while (item_ptr)
    {
        if (item_ptr->data)
        {
            qurl_bit_t acceptfail = QOSA_FALSE;
            long       nread = 0;
            int        ftpcode = 0;
            char      *cmd = item_ptr->data;

            if (cmd[0] == '*')
            {
                cmd++;
                acceptfail = QOSA_TRUE;
            }

            ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "%s", cmd);
            if (ret == QURL_OK)
            {
                pp_ctrl_ptr->ping_time = qurl_time_now(); /* timeout relative now */
                ret = ftp_get_respons(conn_ptr, &nread, &ftpcode);
            }
            if (ret != QURL_OK)
            {
                return ret;
            }

            if (!acceptfail && (ftpcode >= 400))
            {
                qurl_loge("QUOT string not accepted: %s", cmd);
                return QURL_ECODE_FTP_QUOTE_ERR;
            }
        }

        item_ptr = item_ptr->next;
    }

    return QURL_OK;
}

/** Post-response related processing*/

/**
 * @brief Processing after successful login, following commands such as CMD:USER, CMD:PASS, and CMD:ACCT.
 *
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_state_loggedin(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    if (conn_ptr->bits.ftp_use_control_tls)
    {
        /*PBSZ command*/
        /** Referring to RFC 2228, the `PBSZ` command must be sent after the successful completion of secure data exchange (in this case, TLS negotiation) and before the `PROT` command.*/
        /*Send the PBSZ 0 command to indicate disabling buffer encapsulation and proceeding with raw data transmission directly.*/
        ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "PBSZ %d", 0);
        if (ret == QURL_OK)
        {
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_PBSZ);
        }
    }
    else
    {
        /*After successful FTP login, retrieve the current path.*/
        ret = ftp_state_pwd(conn_ptr);
    }

    return ret;
}

/**
 * @brief Response handling for CMD:USER CMD:PASS
 *
 * @param conn_ptr
 * @param ftpcode
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_state_user_resp(qurl_conn_t *conn_ptr, int ftpcode)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    if (ftp_ctrl_ptr->state == QURL_FTP_STATE_USER && ftpcode == 331)
    {
        /*Password required*/
        ret = ftp_state_pass(conn_ptr);
    }
    else if (ftpcode / 100 == 2)
    {
        /*When the result code is 2yz, it indicates a normal login.*/
        /** 230: Login completed*/
        ret = ftp_state_loggedin(conn_ptr);
    }
    else if (ftpcode == 332)
    {
        /*Account required*/
        ret = ftp_state_acct(conn_ptr);
    }
    else
    {
        /*Other response codes*/
        /** [lzm][todo] alternative To be added based on customer requirements.*/

        qurl_loge("Access denied:[%d]\r\n", ftpcode);
        ret = QURL_ECODE_FTP_LOGIN_DENIED; /*No account, login failed.*/
    }

    return ret;
}

/**
 * @brief Response Handling for CMD:ACCT
 *
 * @param conn_ptr
 * @param ftpcode
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_state_acct_resp(qurl_conn_t *conn_ptr, int ftpcode)
{
    qurl_ecode_t ret = QURL_OK;

    if (ftpcode == 230)
    {
        /** 230: Login completed*/
        ret = ftp_state_loggedin(conn_ptr);
    }
    else
    {
        ret = QURL_ECODE_FTP_LOGIN_DENIED;
    }

    return ret;
}

/**
 * @brief Response handling for CMD:PWD
 *
 * @param conn_ptr
 * @param ftpcode
 * @param size
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_state_pwd_resp(qurl_conn_t *conn_ptr, int ftpcode, const char *resp_buf, long size)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    if (ftpcode == 257)
    {
        qurl_bit_t  entry_extracted = QOSA_FALSE;
        char       *dir_ptr = QOSA_NULL;
        const char *tmp_ptr = &resp_buf[4];

        dir_ptr = qurl_malloc(size + 1);
        if (dir_ptr == QOSA_NULL)
        {
            return QURL_ECODE_NO_MEMORY;
        }

        /*
		 * Refer to RFC959 Appendix 2 for the PWD response format: 257<space>[rubbish]"<directory-name>"<space><commentary>
		 * Directory names can contain any characters; embedded double quotes should be escaped by doubling the quotes (i.e., the "quote doubling" convention).
		 */

        /*Scan for the first double quotation mark in the non-standard response*/
        while (tmp_ptr < &resp_buf[size] && *tmp_ptr != '\n' && *tmp_ptr != '\0' && *tmp_ptr != '"')
        {
            tmp_ptr++;
        }
        if ('\"' == *tmp_ptr)
        {
            /*Starting with double quotes, parse the directory*/
            char *store_ptr = QOSA_NULL;
            tmp_ptr++;
            for (store_ptr = dir_ptr; *tmp_ptr && tmp_ptr < &resp_buf[size];)
            {
                if ('\"' == *tmp_ptr)
                {
                    if ('\"' == tmp_ptr[1])
                    {
                        /*Handling consecutive double quotes: "qurl by lzm, ""Hello world!""" is actually parsed as: qurl by lzm, "Hello world!"*/
                        *store_ptr = tmp_ptr[1];
                        tmp_ptr++;
                    }
                    else
                    {
                        /* end of path */
                        entry_extracted = QOSA_TRUE;
                        break; /*Finished searching*/
                    }
                }
                else
                    *store_ptr = *tmp_ptr;
                store_ptr++;
                tmp_ptr++;
            }
            *store_ptr = '\0'; /* null-terminate */
        }

        if (entry_extracted)
        {
            /**
			 * If the path name does not resemble an absolute path (i.e., does not start with '/'), some server-dependent adjustments may be required.
			 * For example, this is the case when connecting to an OS400 FTP server: it supports two name syntaxes, where the default syntax is incompatible with standard paths.
			 * Additionally, when encountering a standard path in a command, the server automatically switches to the conventional path syntax: this results in the entrypath used later in the CWD command having incorrect syntax.
			 * Therefore, the method adopted is to check the server's operating system: this operation is performed only when the path name appears unusual, in order to minimize additional overhead on other systems.
			 */

            /*[lzm][todo]Process path syntax*/

            /*Save the current path*/
            if (ftp_ctrl_ptr->entrypath_ptr != QOSA_NULL)
            {
                qurl_free(ftp_ctrl_ptr->entrypath_ptr);
            }
            ftp_ctrl_ptr->entrypath_ptr = dir_ptr;
            core_ptr->cfg.ftp.ftp_most_recent_entrypath_ptr = ftp_ctrl_ptr->entrypath_ptr;
            qurl_logd("PWD:%s\r\n", dir_ptr);
        }
        else
        {
            if (dir_ptr != QOSA_NULL)
            {
                qurl_free(dir_ptr);
                dir_ptr = QOSA_NULL;
            }
        }
    }
    else
    {
        ret = QURL_ECODE_FTP_LOGIN_DENIED;
    }

    ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
    return ret;
}

/**
 * @brief Response handling for CMD:MDTM
 *
 * @param conn_ptr
 * @param ftpcode
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_state_mdtm_resp(qurl_conn_t *conn_ptr, int ftpcode)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;
    const char       *tmp_ptr = (const char *)&conn_ptr->deal_cache_ptr[4];

    switch (ftpcode)
    {
        case 213: {
            /*We have obtained a time. The format should be: "YYYYMMDDHHMMSS[.sss]",
The final .sss part is optional, representing the fractional seconds*/
            int year = 0;
            int month = 0;
            int day = 0;
            int hour = 0;
            int minute = 0;
            int second = 0;

            if (ftp_213_date(tmp_ptr, &year, &month, &day, &hour, &minute, &second))
            {
                /*Sometimes, reformatting is necessary*/
                char timebuf[24] = {0};
                qurl_snprintf(timebuf, sizeof(timebuf), "%04d%02d%02d %02d:%02d:%02d GMT", year, month, day, hour, minute, second);
                /*Now, convert it to a time() value:*/
                core_ptr->info.filetime = qurl_pd_getdate_capped(timebuf);
                qurl_logd("filetime:%s\r\n", timebuf);
                qurl_logd("filetime:%lld\r\n", core_ptr->info.filetime);
            }
        }
        break;
        default:
            qurl_loge("unsupported MDTM reply format\r\n");
            break;
        case 550: /*550 is used to indicate various different issues, such as "file or directory does not exist" or "permission denied." It does not necessarily mean the file is completely absent.*/
            qurl_logd(
                "MDTM failed: file does not exist or permission problem,"
                " continuing"
            );
            break;
    }

    if (core_ptr->usr_cfg.timecondition)
    {
        if ((core_ptr->info.filetime > 0) && (core_ptr->usr_cfg.timevalue > 0))
        {
            switch (core_ptr->usr_cfg.timecondition)
            {
                case QURL_TIMECOND_IFMODSINCE:
                default: {
                    if (core_ptr->info.filetime <= core_ptr->usr_cfg.timevalue)
                    {
                        qurl_logd("the requested document is not new enough\r\n");
                        ftp_core_ptr->transfer = QURL_PP_TRANSFER_NONE; /* mark to not transfer data */
                        core_ptr->info.timecond = QOSA_TRUE;
                        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
                        return QURL_OK;
                    }
                    break;
                }
                case QURL_TIMECOND_IFUNMODSINCE: {
                    if (core_ptr->info.filetime > core_ptr->usr_cfg.timevalue)
                    {
                        qurl_logd("the requested document is not old enough\r\n");
                        ftp_core_ptr->transfer = QURL_PP_TRANSFER_NONE; /* mark to not transfer data */
                        core_ptr->info.timecond = QOSA_TRUE;
                        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
                        return QURL_OK;
                    }
                    break;
                }
            } /* switch */
        }
        else
        {
            qurl_logd("skipping time comparison\r\n");
        }
    }

    if (ret == QURL_OK)
    {
        ret = ftp_state_type(conn_ptr);
    }

    return ret;
}

/**
 * @brief Response handling for CMD:TYPE
 *
 * @param conn_ptr
 * @param ftpcode
 * @param instate
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_state_type_resp(qurl_conn_t *conn_ptr, int ftpcode, qurl_ftp_state_e instate)
{
    qurl_ecode_t ret = QURL_OK;

    if (ftpcode / 100 != 2)
    {
        return QURL_ECODE_FTP_CMD_TYPE_ERR;
    }
    if (ftpcode != 200)
    {
        qurl_logd("TYPE resp code:%d\r\n", ftpcode);
    }

    if (instate == QURL_FTP_STATE_TYPE)
    {
        ret = ftp_state_size(conn_ptr);
    }
    else if (instate == QURL_FTP_STATE_LIST_TYPE)
    {
        ret = ftp_state_list(conn_ptr);
    }
    else if (instate == QURL_FTP_STATE_RETR_TYPE)
    {
        ret = ftp_state_retr_prequote(conn_ptr);
    }
    else if (instate == QURL_FTP_STATE_STOR_TYPE)
    {
        ret = ftp_state_stor_prequote(conn_ptr);
    }

    return ret;
}

/**
 * @brief Response handling for CMD:SIZE
 *
 * @param conn_ptr
 * @param ftpcode
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_state_size_resp(qurl_conn_t *conn_ptr, int ftpcode, qurl_ftp_state_e instate)
{
    qurl_ecode_t      ret = QURL_OK;
    long              filesize = -1;
    const char       *tmp_ptr = (const char *)conn_ptr->deal_cache_ptr;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    qurl_logd("enter:%s\r\n", tmp_ptr);

    /*Get size from ASCII string*/
    if (ftpcode == 213)
    {
        /*To allow the server to add "garbage" in the response string, we scan all digits at the end of the response and parse them as a single number only.*/
        char *start = (char *)&tmp_ptr[4];
        char *fdigit = qurl_strchr(start, '\r');
        if (fdigit)
        {
            do {
                fdigit--;
            } while (qurl_ctype_isdigit(*fdigit) && (fdigit > start));
            if (!qurl_ctype_isdigit(*fdigit))
            {
                fdigit++;
            }
        }
        else
        {
            fdigit = start;
        }
        /*Ignore parsing errors, this will result in the size still being unknown*/
        (void)qurl_strtoofft(fdigit, QOSA_NULL, 10, &filesize);
    }
    else if (ftpcode == 550)
    {
        /* "No such file or directory" */
        /*When probing for commands to use, allow SIZE failure checks for (resume) uploads.*/
        if (instate != QURL_FTP_STATE_STOR_SIZE)
        {
            qurl_logd("The file does not exist\r\n");
            return QURL_ECODE_FTP_REMOTE_FILE_NOT_FOUND;
        }
    }

    if (instate == QURL_FTP_STATE_SIZE)
    {
        /*[lzm][note]Should we confirm the download_len_max here?*/
        core_ptr->info.content_length = filesize;
        ret = ftp_state_rest(conn_ptr);
    }
    else if (instate == QURL_FTP_STATE_RETR_SIZE)
    {
        /*[lzm][note]Should we confirm the download_len_max here?*/
        core_ptr->info.content_length = filesize;
        ret = ftp_state_retr(conn_ptr, filesize);
    }
    else if (instate == QURL_FTP_STATE_STOR_SIZE)
    {
        core_ptr->cfg.resume_from = filesize;
        ret = ftp_state_ul_setup(conn_ptr, QOSA_TRUE);
    }

    return ret;
}

/**
 * @brief Response handling for CMD:REST
 *
 * @param conn_ptr
 * @param ftpcode
 * @param instate
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_state_rest_resp(qurl_conn_t *conn_ptr, int ftpcode, qurl_ftp_state_e instate)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    switch (instate)
    {
        case QURL_FTP_STATE_REST:
        default: {
            ret = ftp_state_prepare_transfer(conn_ptr);
            break;
        }
        case QURL_FTP_STATE_RETR_REST: {
            if (ftpcode != 350)
            {
                qurl_loge("Couldn't use REST\r\n");
                ret = QURL_ECODE_FTP_COULDNT_USE_REST;
            }
            else
            {
                ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "RETR %s", ftp_ctrl_ptr->file_ptr);
                if (ret == QURL_OK)
                {
                    ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_RETR);
                }
            }
            break;
        }
    }

    return ret;
}

/**
 * @brief Response handling for CMD:STOR
 *
 * @param conn_ptr
 * @param ftpcode
 * @param instate
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_state_stor_resp(qurl_conn_t *conn_ptr, int ftpcode, qurl_ftp_state_e instate)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    qurl_logd("enter\r\n");
    if (ftpcode >= 400)
    {
        qurl_loge("Failed FTP upload: %d\r\n", ftpcode);
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
        /* oops, we never close the sockets! */
        return QURL_ECODE_UPLOAD_ERR;
    }

    ftp_ctrl_ptr->state_saved = instate;

    /*PORT indicates we are waiting for the server to connect to us*/
    if (core_ptr->usr_cfg.bits.ftp_use_port)
    {
        qurl_bit_t connected = QOSA_FALSE;

        /* no longer in STOR state */
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);

        ret = ftp_allow_server_connect(conn_ptr, &connected);
        if (ret)
        {
            return ret;
        }

        if (connected == QOSA_FALSE)
        {
            qurl_logd("Data conn was not available immediately\r\n");
            ftp_ctrl_ptr->bits.wait_data_conn = QOSA_TRUE;
        }

        return QURL_OK;
    }
    else
    {
        ret = ftp_init_transfer(conn_ptr);
    }

    return ret;
}

/**
 * @brief Response handling for CMD:LIST and CMD:RETR
 *
 * @param conn_ptr
 * @param ftpcode
 * @param instate
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_state_get_resp(qurl_conn_t *conn_ptr, int ftpcode, qurl_ftp_state_e instate)
{
    qurl_ecode_t      ret = QURL_OK;
    long              size = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    const char       *resp_str = (const char *)conn_ptr->deal_cache_ptr;

    qurl_logd("enter\r\n");
    if ((ftpcode == 150) || (ftpcode == 125))
    {
        /**
		 * "125" Data connection opened, transmission starting.
		 * "150" File status okay, about to open data connection.
		 *
		 * A:150 Opening BINARY mode data connection for /etc/passwd (2241 bytes).
		 * A data connection has been opened in binary mode and is transferring the file /etc/passwd. The number in parentheses, 2241, indicates the file size.
		 *
		 * B:150 Opening ASCII mode data connection for /bin/ls
		 * Here, a data connection is opened in ASCII mode, and the file path to be transmitted is /bin/ls.
		 *
		 * C:150 ASCII data connection for /bin/ls (137.167.104.91,37445) (0 bytes).
		 * This message indicates that an ASCII data connection has been established, the file being transferred is /bin/ls, and it provides the remote host's IP address (137.167.104.91) and port number (37445), with a file size of 0 bytes.
		 *
		 * D:150 Opening ASCII mode data connection for [file] (0.0.0.0,0) (545 bytes)
		 * This message indicates that a data connection is being opened in ASCII mode to transfer a file. The file path is not provided, but the local host IP address (0.0.0.0) and port number (0) are given, with a file size of 545 bytes.
		 *
		 * E:125 Data connection already open; Transfer starting.
		 * This message indicates that the data connection has been opened and transmission is about to begin.
		 */

        size = -1;

        /**
		 * In binary mode, when using the SIZE command to obtain the file size, some FTP servers may return a file size of 0.
		 * To address this foolish behavior, the code attempts to parse the RETR response even when the returned size is 0.
		 *
		 * Reconfirm the download size: core_ptr->trans.download_len_max
		 */
        if ((instate != QURL_FTP_STATE_LIST) && !core_ptr->cfg.bits.prefer_ascii && !core_ptr->usr_cfg.bits.ignore_cl && (core_ptr->trans.download_len_max < 1))
        {
            /*
			 * When processing FTP directory listings, in some cases the size may not be displayed, and a size of 0 is typically used.
			 * When using ASCII transmission, the actual amount of data transferred may differ from the size information in the directory listing, so using this size information in such cases would only cause more confusion.
			 * For example, the D example above makes parsing somewhat tricky.
			 */
            char *bytes_ptr = QOSA_NULL;
            bytes_ptr = qurl_strstr(resp_str, " bytes");
            if (bytes_ptr == QOSA_NULL)
            {
                bytes_ptr = qurl_strstr(resp_str, " Bytes");
            }
            if (bytes_ptr)
            {
                long in = (long)(--bytes_ptr - resp_str);
                /*This is a hint containing information about file size! ;-)*/
                while (--in)
                {
                    /*Scan for the left parenthesis and break there.*/
                    if ('(' == *bytes_ptr)
                        break;
                    /*Skip numbers*/
                    if (!qurl_ctype_isdigit(*bytes_ptr))
                    {
                        bytes_ptr = QOSA_NULL;
                        break;
                    }
                    /*Take another step back*/
                    bytes_ptr--;
                }
                /*If we only have numbers:*/
                if (bytes_ptr)
                {
                    ++bytes_ptr;
                    /* get the number! */
                    (void)qurl_strtoofft(bytes_ptr, QOSA_NULL, 10, &size);
                }
            }
        }
        else if (core_ptr->trans.download_len_max > -1)
        {
            size = core_ptr->trans.download_len_max;
        }

        /*Check if it exceeds our defined length*/
        if (size > core_ptr->cfg.download_max && core_ptr->cfg.download_max > 0)
        {
            size = core_ptr->cfg.download_max;
        }
        else if ((instate != QURL_FTP_STATE_LIST) && (core_ptr->cfg.bits.prefer_ascii))
        {
            size = -1; /*Temporary solution for servers underestimating ASCII mode file size*/
        }

        /*Confirm download size*/
        core_ptr->trans.download_len_max = size;

        /*FTP Download*/
        ftp_ctrl_ptr->state_saved = instate;

        if (core_ptr->usr_cfg.bits.ftp_use_port)
        {
            /*Active Mode*/
            qurl_bit_t connected = QOSA_FALSE;

            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);

            ret = ftp_allow_server_connect(conn_ptr, &connected);
            if (ret != QURL_OK)
            {
                return ret;
            }
            if (connected == QOSA_FALSE)
            {
                qurl_logd("data conn was not available immediately\r\n");
                ftp_ctrl_ptr->bits.wait_data_conn = QOSA_TRUE; /*Waiting for data connection.*/
            }
        }
        else
        {
            /*Passive Mode*/
            ret = ftp_init_transfer(conn_ptr);
        }
    }
    else
    {
        if ((instate == QURL_FTP_STATE_LIST) && (ftpcode == 450))
        {
            /*If no matching file is found in the directory list*/
            ftp_core_ptr->transfer = QURL_PP_TRANSFER_NONE; /*No need to download any data*/
            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
        }
        else
        {
            qurl_logd("RETR response: %d\r\n", ftpcode);
            return instate == QURL_FTP_STATE_RETR && ftpcode == 550 ? QURL_ECODE_FTP_REMOTE_FILE_NOT_FOUND : QURL_ECODE_FTP_COULDNT_RETR_FILE;
        }
    }

    return ret;
}

static qurl_ecode_t ftp_state_port_resp(qurl_conn_t *conn_ptr, int ftpcode)
{
    qurl_ecode_t        ret = QURL_OK;
    qurl_ftp_ctrl_t    *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_ftp_cmd_port_e fcmd = (qurl_ftp_cmd_port_e)ftp_ctrl_ptr->count1;

    /*The FTP specification requires a response of 200 for active mode, but we can appropriately relax this condition.*/
    if (ftpcode / 100 != 2)
    {
        /*Active mode command failed*/
        if (fcmd == QURL_FTP_CMD_PORT_EPRT)
        {
            qurl_logd("disabling EPRT usage\r\n");
            conn_ptr->bits.ftp_use_eprt = QOSA_FALSE; /*Retry using PORT*/
        }
        fcmd++;

        if (fcmd == QURL_FTP_CMD_PORT_DONE)
        {
            qurl_logd("failed to do PORT\r\n");
            return QURL_ECODE_FTP_CMD_PROT_ERR;
        }
        else
        {
            ret = ftp_state_use_port(conn_ptr, fcmd);
        }
    }
    else
    {
        qurl_logd("Connect data stream actively\r\n");
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
        ret = ftp_reqphase_done(conn_ptr, QOSA_FALSE);
    }

    return ret;
}

/**
 * @brief It is called within the ftp_state_pasv_resp function,
 * Used to switch to PASV mode when encountering EPSV (Extended Passive Mode) issues.
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_epsv_disable(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    if (conn_ptr->bits.ipv6)
    {
        /*We cannot perform IPv6 when EPSV is disabled, so this is a failure.*/
        qurl_loge("Failed EPSV attempt, exiting\r\n");
        return QURL_ECODE_FTP_WEIRD_SERVER_REPLY;
    }

    qurl_logd("Failed EPSV attempt. Disabling EPSV\r\n");
    conn_ptr->bits.ftp_use_epsv = QOSA_FALSE;
    // qurl_conn_close(conn_ptr, 1); /* [lzm][todo] */
    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "%s", "PASV");
    if (ret == QURL_OK)
    {
        ftp_ctrl_ptr->count1++;
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_PASV);
    }

    return ret;
}

static qurl_ecode_t ftp_state_pasv_resp(qurl_conn_t *conn_ptr, int ftpcode)
{
    qurl_bit_t        connected = QOSA_FALSE;
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    const char       *str = (const char *)&conn_ptr->deal_cache_ptr[4]; /*Parse starting from the first character after the response code*/
    struct addrinfo   hints = {0};
    struct addrinfo  *ai_ptr = QOSA_NULL;
    qurl_addrinfo_t  *rp_ptr = QOSA_NULL;
    int               count = 0;
    char              port_str[7] = {0};

    if (ftp_ctrl_ptr->newhost_ptr != QOSA_NULL)
    {
        qurl_free(ftp_ctrl_ptr->newhost_ptr);
        ftp_ctrl_ptr->newhost_ptr = QOSA_NULL;
    }

    if ((ftp_ctrl_ptr->count1 == 0) && (ftpcode == 229))
    {
        /* EPSV */
        /*Parse the IP and port from the response, and store the parsed IP and port in the ip structure.*/
        char *ptr = qurl_strchr(str, '(');
        if (ptr)
        {
            char sep = 0;
            ptr++;
            /* |||12345| */
            sep = ptr[0];
            /*Check if it is a number*/
            if ((ptr[1] == sep) && (ptr[2] == sep) && qurl_ctype_isdigit(ptr[3]))
            {
                char         *end_ptr = QOSA_NULL;
                unsigned long num = qurl_strtoul(&ptr[3], &end_ptr, 10);
                if (*end_ptr != sep)
                {
                    ptr = QOSA_NULL;
                }
                else if (num > 0xffff)
                {
                    qurl_loge("Illegal port number in EPSV reply\r\n");
                    return QURL_ECODE_FTP_WEIRD_PASV_REPLY;
                }
                if (ptr)
                {
                    ftp_ctrl_ptr->newport = (unsigned short)(num & 0xffff);                  //Storage Port
                    ftp_ctrl_ptr->newhost_ptr = qurl_strdup(ftp_control_address(conn_ptr));  //Store IP address
                    if (ftp_ctrl_ptr->newhost_ptr == QOSA_NULL)
                    {
                        return QURL_ECODE_NO_MEMORY;
                    }
                }
            }
            else
            {
                ptr = QOSA_NULL;
            }
        }
        //If parsing fails, return an error
        if (ptr == QOSA_NULL)
        {
            qurl_loge("Weirdly formatted EPSV reply\r\n");
            return QURL_ECODE_FTP_WEIRD_PASV_REPLY;
        }
    }
    else if ((ftp_ctrl_ptr->count1 == 1) && (ftpcode == 227))
    {
        /* PASV */
        /*Parse the IP and port from the response, and store the parsed IP and port in the ip structure positive PASV response*/
        unsigned int ip[6] = {0};

        /*
		 * Scan the numbers in the response and store them in the ip array.
		 *
		 * Found reply-strings include:
		 * "227 Entering Passive Mode (127,0,0,1,4,51)"
		 * "227 Data transfer will passively listen to 127,0,0,1,4,51"
		 * "227 Entering passive mode. 127,0,0,1,4,51"
		 */
        while (*str)
        {
            if (ftp_match_pasv_6nums(str, ip))
            {
                break;
            }
            str++;
        }

        if (!*str)
        {
            qurl_loge("Couldn't interpret the 227-response\r\n");
            return QURL_ECODE_FTP_WEIRD_227_FORMAT;
        }

        /*If successfully parsed, store the IP and port*/
        if (core_ptr->usr_cfg.bits.ftp_skip_ip)
        {
            /*If told to ignore the remote IP, use the control connection's*/
            // qurl_logd("Skip %u.%u.%u.%u for data connection, re-use %s instead\r\n",
            //       ip[0], ip[1], ip[2], ip[3],
            //      conn->host.name);
            ftp_ctrl_ptr->newhost_ptr = qurl_strdup(ftp_control_address(conn_ptr));
        }
        else
        {
            qurl_asprintf(&ftp_ctrl_ptr->newhost_ptr, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
        }

        if (ftp_ctrl_ptr->newhost_ptr == QOSA_NULL)
        {
            return QURL_ECODE_NO_MEMORY;
        }

        ftp_ctrl_ptr->newport = (unsigned short)(((ip[4] << 8) + ip[5]) & 0xffff);
    }
    else if (ftp_ctrl_ptr->count1 == 0)
    {
        /*Use PASV if EPSV fails*/
        return ftp_epsv_disable(conn_ptr);
    }
    else
    {
        qurl_logd("Bad PASV/EPSV response: %d\r\n", ftpcode);
        return QURL_ECODE_FTP_WEIRD_PASV_REPLY;
    }

    if (conn_ptr->bits.tcp_fastopen && !conn_ptr->bits.reuse && !ftp_ctrl_ptr->newhost_ptr[0])
    {
        /*Supports quick open && first time using this conn && unable to obtain a valid address: then use the control connection*/
        qurl_free(ftp_ctrl_ptr->newhost_ptr);
        ftp_ctrl_ptr->newhost_ptr = QOSA_NULL;

        ftp_ctrl_ptr->newhost_ptr = qurl_strdup(ftp_control_address(conn_ptr));  //Store IP address
        if (ftp_ctrl_ptr->newhost_ptr == QOSA_NULL)
        {
            return QURL_ECODE_NO_MEMORY;
        }
    }

    qurl_logd("data conn [ip][port]:[%s][%d]\r\n", ftp_ctrl_ptr->newhost_ptr, ftp_ctrl_ptr->newport);

    /*Parse this connection address: It only obtains the addrinfo format through the IP address, so a blocking function is used directly here, but it will return immediately.*/
    qurl_snprintf(port_str, sizeof(port_str), "%u", ftp_ctrl_ptr->newport);
    hints.ai_family = conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].resolve_addrtype;
    hints.ai_socktype = SOCK_STREAM;
#if !defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) && !defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
    //To be deleted after ASR LWIP replacement
    hints.ai_flags = AI_ADDRCONFIG;
#endif /* !defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) && !defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC) */
    ret = qurl_socket_resolve(ftp_ctrl_ptr->newhost_ptr, port_str, &hints, &ai_ptr, core_ptr->usr_cfg.nw_id);
    if (ret != 0)
    {
        return QURL_ECODE_FTP_CANT_GET_HOST;
    }
    if (ai_ptr == QOSA_NULL)
    {
        return QURL_ECODE_FTP_CANT_GET_HOST;
    }
    for (rp_ptr = ai_ptr; rp_ptr != QOSA_NULL; rp_ptr = rp_ptr->ai_next)
    {
        char  ipstr[QURL_INET6_ADDRSTRLEN] = {0};
        void *addr = QOSA_NULL;
        count++;
        if (rp_ptr->ai_family == AF_INET)
        {  // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp_ptr->ai_addr;
            addr = &(ipv4->sin_addr);
        }
        else
        {  // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp_ptr->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        qurl_inet_ntop(rp_ptr->ai_family, addr, ipstr, sizeof(ipstr));
        qurl_logd("dns success[%d][%s]\n", count, ipstr);
    }
    /*After parsing, release it once first.*/
    if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].addrinfo_ptr != QOSA_NULL)
    {
        qurl_socket_addinfo_free(conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].addrinfo_ptr);
        conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].addrinfo_ptr = QOSA_NULL;
    }
    /*Reassign*/
    conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].addrinfo_cnt = count;
    conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].remote_port = (int)ftp_ctrl_ptr->newport;
    conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].addrinfo_ptr = ai_ptr;

    //Start establishing FTP data connection (passive mode), here initialize index1_last_close_event
    conn_ptr->index1_last_close_event = 0;

    /*Trigger connection*/
    ret = qurl_conn_connect(conn_ptr, QURL_SOCKET_INDEX_1, &connected);
    if (ret != QURL_OK)
    {
        if (ftp_ctrl_ptr->count1 == 0 && ftpcode == 229)
        {
            return ftp_epsv_disable(conn_ptr);
        }
    }

    conn_ptr->bits.req_more = QOSA_TRUE; /*Need to switch to the req_more state for execution*/
    ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);

    return QURL_OK;
}

/* other */

static qurl_ecode_t ftp_pp_read_pong(qurl_conn_t *conn_ptr, qurl_pp_ctrl_t *pp_ctrl_ptr, int *pong_code_ptr, long *size_ptr)
{
    int               ftpcode = 0;
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    ret = qurl_pp_read_pong(conn_ptr, pp_ctrl_ptr, &ftpcode, size_ptr);

    core_ptr->info.resp_code = ftpcode;

    if (pong_code_ptr)
    {
        *pong_code_ptr = ftpcode;
    }

    if (421 == ftpcode)
    {
        /*
		 * 421 indicates "Service not available, closing control connection." FTP servers use this to indicate that the idle session timeout has been exceeded.
		 * If we ignore the response, it may hang in some cases.
		 * This response code can appear at any time, so it is a good idea to handle it uniformly.
		 */
        qurl_loge("We got a 421 - timeout\r\n");
        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
        return QURL_ECODE_PERFORM_TIMEOUT;
    }

    return ret;
}

/**
 * @brief pingpong FTP protocol stack state machine
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_pp_statemachine(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t             ret = QURL_OK;
    int                      ftpcode = 0;
    long                     size = 0;
    long                     tmp_len = 0;
    static const char *const ftp_auth_str[] = {"SSL", "TLS"};
    qurl_core_ctrl_t        *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t         *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t          *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    /*Refresh the ping buffer*/
    ret = qurl_pp_flushsend(conn_ptr, pp_ctrl_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /*Read the response. Internally, data will be received, and the response parsing function set by the protocol stack will be called back, so there may be FTP response codes.*/
    ret = ftp_pp_read_pong(conn_ptr, pp_ctrl_ptr, &ftpcode, &size);
    if (ret != QURL_OK)
    {
        return ret;
    }

    qurl_logd("state:[%d]\r\n", ftp_ctrl_ptr->state);
    /*The FTP state machine processing corresponding to the response code will only proceed after obtaining the response code.*/
    if (ftpcode)
    {
        switch (ftp_ctrl_ptr->state)
        {
            case QURL_FTP_STATE_WAIT220: {
                qurl_logd("enter\r\n");
                if (ftpcode == 230)
                {
                    if (core_ptr->usr_cfg.use_tls <= QURL_USETLS_TRY || conn_ptr->bits.ftp_use_control_tls)
                    {
                        /*It only makes sense if it's done openly or already uses TLS encryption.*/
                        ret = ftp_state_user_resp(conn_ptr, ftpcode);
                        break;
                    }
                }
                else if (ftpcode != 220)
                {
                    ret = QURL_ECODE_FTP_WEIRD_SERVER_REPLY;
                    break;
                }

                if (core_ptr->usr_cfg.use_tls && !conn_ptr->bits.ftp_use_control_tls)
                {
                    /*We do not have an SSL/TLS control connection, but are requesting FTPS. Now attempting to connect using FTPS.*/
                    qurl_bit_t break_flag = QOSA_FALSE;

                    ftp_ctrl_ptr->count3 = 0; /*Used to count the value for the second execution of AUTH*/
                    switch (core_ptr->usr_cfg.ftp.ftp_auth)
                    {
                        case QURL_FTP_AUTH_DEFAULT:
                        case QURL_FTP_AUTH_SSL:
                            ftp_ctrl_ptr->count2 = 1; /*The second time is TLS*/
                            ftp_ctrl_ptr->count1 = 0;
                            break;
                        case QURL_FTP_AUTH_TLS:
                            ftp_ctrl_ptr->count2 = -1; /*The second time is SSL*/
                            ftp_ctrl_ptr->count1 = 1;
                            break;
                        default:
                            qurl_loge("unsupported parameter to QURL_OPT_FTP_AUTH: %d", (int)core_ptr->usr_cfg.ftp.ftp_auth);
                            ret = QURL_ECODE_UNKNOWN_OPTION; /* we don't know what to do */
                            break_flag = QOSA_TRUE;
                    }
                    if (break_flag != QOSA_FALSE)
                    {
                        break;
                    }
                    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "AUTH %s", ftp_auth_str[ftp_ctrl_ptr->count1]);
                    if (ret == QURL_OK)
                    {
                        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_AUTH);
                    }
                }
                else
                {
                    qurl_logd("enter\r\n");
                    ret = ftp_state_user(conn_ptr);
                }
                break;
            }
            case QURL_FTP_STATE_AUTH: {
                /*Arriving here indicates that an AUTH has already been sent once.*/
                tmp_len = qurl_dbuf_vaild_size(pp_ctrl_ptr->pong_cache);
                if (tmp_len > 0)
                {
                    /*There is extra data after \r\n, the response does not meet expectations, and an error is returned.*/
                    ret = QURL_ECODE_FTP_WEIRD_SERVER_REPLY;
                    break;
                }

                /*RFC2228 (page 5) states:
If the server is willing to accept the specified security mechanism and does not require any security data, it must reply with response code 234/334*/

                if (ftpcode == 234 || ftpcode == 334)
                {
                    qurl_bit_t tls_done_flag = QOSA_FALSE;

                    /*1. Switch ftp to ftps*/
                    if (qurl_strcasecmp("ftps", conn_ptr->port_handler->scheme))
                    {
                        ret = qurl_port_prot_match_by_scheme("ftps", &conn_ptr->port_handler);
                        if (ret != QURL_OK)
                        {
                            break;
                        }

#if QURL_CFG_ENABLE_TLS
                        ret = qurl_conn_upgrade_to_tls(conn_ptr, core_ptr);
                        if (ret != QURL_OK)
                        {
                            break;
                        }
#endif /* QURL_CFG_ENABLE_TLS */
                        /*[lzm][note]The control connection is changed to non-TLS by default. Whether it is TLS is determined by the FTP PROT command; the TLS connection is established by ftp_init_transfer().*/
                        conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].bits.is_tls = QOSA_FALSE;
                    }
                    /*2. Initiate (blocking?) TLS handshake. If non-blocking, exit and return to QURL_STATE_PROT_CONNECT state to continue.*/
                    ret = qurl_conn_connect(conn_ptr, QURL_SOCKET_INDEX_0, &tls_done_flag);
                    if (ret != QURL_OK)
                    {
                        break;
                    }
                    conn_ptr->bits.tls_connecting = !tls_done_flag;
                    conn_ptr->bits.ftp_use_control_tls = QOSA_TRUE;
                    /*3. Send user, enter user state*/
                    ret = ftp_state_user(conn_ptr);
                }
                else if (ftp_ctrl_ptr->count3 < 1)
                {
                    /*Choose another solution*/
                    ftp_ctrl_ptr->count3++;
                    ftp_ctrl_ptr->count1 += ftp_ctrl_ptr->count2;
                    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "AUTH %s", ftp_auth_str[ftp_ctrl_ptr->count1]);
                }
                else
                {
                    if (core_ptr->usr_cfg.use_tls > QURL_USETLS_TRY)
                    {
                        ret = QURL_ECODE_FTP_LOGIN_DENIED;
                    }
                    else
                    {
                        /*If the control connection can be in plaintext, then proceed.*/
                        ret = ftp_state_user(conn_ptr);
                    }
                }
                break;
            }
            case QURL_FTP_STATE_USER:
            case QURL_FTP_STATE_PASS: {
                ret = ftp_state_user_resp(conn_ptr, ftpcode);
                break;
            }
            case QURL_FTP_STATE_ACCT: {
                ret = ftp_state_acct_resp(conn_ptr, ftpcode);
                break;
            }
            case QURL_FTP_STATE_PBSZ: {
                ret = ftp_state_prot(conn_ptr);
                break;
            }
            case QURL_FTP_STATE_PROT: {
                if (ftpcode / 100 == 2)
                {
                    /*We have enabled SSL for the data connection!*/
                    conn_ptr->bits.ftp_use_data_tls = (core_ptr->usr_cfg.use_tls != QURL_USETLS_CONTROL) ? QOSA_TRUE : QOSA_FALSE;
                }
                /*If the FTP server decides to reject our 'P' request, it typically responds with a 500*/
                else if (core_ptr->usr_cfg.use_tls > QURL_USETLS_CONTROL)
                {
                    ret = QURL_ECODE_FTP_CMD_PROT_ERR;
                    break;
                }
                if (core_ptr->usr_cfg.ftp.ftp_ccc)
                {
                    /*CCC - Clear Command Channel*/
                    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "%s", "CCC");
                    if (ret == QURL_OK)
                    {
                        ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_CCC);
                    }
                }
                else
                {
                    ret = ftp_state_pwd(conn_ptr);
                }
                break;
            }
            case QURL_FTP_STATE_CCC: {
                /* [lzm][todo] */
                break;
            }
            case QURL_FTP_STATE_PWD: {
                ret = ftp_state_pwd_resp(conn_ptr, ftpcode, (const char *)conn_ptr->deal_cache_ptr, size);
                break;
            }
            case QURL_FTP_STATE_SYST: {
                /* [lzm][todo] */
                break;
            }
            case QURL_FTP_STATE_NAMEFMT: {
                if (ftpcode == 250)
                {
                    ret = ftp_state_pwd(conn_ptr);
                    break;
                }
                ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
                break;
            }
            case QURL_FTP_STATE_QUOTE:
            case QURL_FTP_STATE_RETR_PREQUOTE:
            case QURL_FTP_STATE_STOR_PREQUOTE:
            case QURL_FTP_STATE_POSTQUOTE: {
                if ((ftpcode >= 400) && !ftp_ctrl_ptr->count2)
                {
                    qurl_loge("QUOT command failed with %d", ftpcode);
                    ret = QURL_ECODE_FTP_CMD_QUOTE_ERR;
                }
                else
                {
                    ret = ftp_state_quote(conn_ptr, QOSA_FALSE, ftp_ctrl_ptr->state);
                }
                break;
            }
            case QURL_FTP_STATE_CWD: {
                if (ftpcode / 100 != 2)
                {
                    /*CWD command failed*/
                    if (core_ptr->usr_cfg.ftp.ftp_create_missing_dirs && ftp_ctrl_ptr->cwd_count && !ftp_ctrl_ptr->count2)
                    {
                        /* try making it */
                        ftp_ctrl_ptr->count2++; /*The counter is used to prevent loops between CWD (Change Working Directory) and MKD (Make Directory).*/

                        /*count3 is set to allow MKD to fail once per directory.
In the case where CWD fails and then MKD also fails (because another session preemptively created the directory),
This allows a second attempt to CWD to that directory.*/
                        ftp_ctrl_ptr->count3 = (core_ptr->usr_cfg.ftp.ftp_create_missing_dirs == 2) ? 1 : 0;

                        ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "MKD %s", ftp_ctrl_ptr->dirs_pptr[ftp_ctrl_ptr->cwd_count - 1]);
                        if (ret == QURL_OK)
                        {
                            ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_MKD);
                        }
                    }
                    else
                    {
                        /* return failure */
                        ftp_ctrl_ptr->bits.cwd_faild = QOSA_TRUE; /*This path failed*/
                        ret = QURL_ECODE_FTP_REMOTE_ACCESS_DENIED;
                    }
                }
                else
                {
                    /* success */
                    ftp_ctrl_ptr->count2 = 0;
                    if (++ftp_ctrl_ptr->cwd_count <= ftp_ctrl_ptr->dir_depth)
                    {
                        /* send next CWD */
                        ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "CWD %s", ftp_ctrl_ptr->dirs_pptr[ftp_ctrl_ptr->cwd_count - 1]);
                    }
                    else
                    {
                        ret = ftp_state_mdtm(conn_ptr);
                    }
                }
                break;
            }
            case QURL_FTP_STATE_MKD: {
                if ((ftpcode / 100 != 2) && !ftp_ctrl_ptr->count3--)
                {
                    /*Failed to confirm creation*/
                    qurl_loge("Failed to MKD dir: %d", ftpcode);
                    ret = QURL_ECODE_FTP_REMOTE_ACCESS_DENIED;
                }
                else
                {
                    ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_MKD);
                    ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "CWD %s", ftp_ctrl_ptr->dirs_pptr[ftp_ctrl_ptr->cwd_count - 1]);
                }
                break;
            }
            case QURL_FTP_STATE_MDTM: {
                ret = ftp_state_mdtm_resp(conn_ptr, ftpcode);
                break;
            }
            case QURL_FTP_STATE_TYPE:
            case QURL_FTP_STATE_LIST_TYPE:
            case QURL_FTP_STATE_RETR_TYPE:
            case QURL_FTP_STATE_STOR_TYPE: {
                ret = ftp_state_type_resp(conn_ptr, ftpcode, ftp_ctrl_ptr->state);
                break;
            }
            case QURL_FTP_STATE_SIZE:
            case QURL_FTP_STATE_RETR_SIZE:
            case QURL_FTP_STATE_STOR_SIZE: {
                ret = ftp_state_size_resp(conn_ptr, ftpcode, ftp_ctrl_ptr->state);
                break;
            }
            case QURL_FTP_STATE_REST:
            case QURL_FTP_STATE_RETR_REST: {
                ret = ftp_state_rest_resp(conn_ptr, ftpcode, ftp_ctrl_ptr->state);
                break;
            }
            case QURL_FTP_STATE_PRET: {
                if (ftpcode != 200)
                {
                    ret = QURL_ECODE_FTP_CMD_PRET_ERR;
                    break;
                }
                ret = ftp_state_use_pasv(conn_ptr);
                break;
            }
            case QURL_FTP_STATE_PASV: {
                ret = ftp_state_pasv_resp(conn_ptr, ftpcode);
                break;
            }
            case QURL_FTP_STATE_PORT: {
                ret = ftp_state_port_resp(conn_ptr, ftpcode);
                break;
            }
            case QURL_FTP_STATE_LIST:
            case QURL_FTP_STATE_RETR: {
                ret = ftp_state_get_resp(conn_ptr, ftpcode, ftp_ctrl_ptr->state);
                break;
            }
            case QURL_FTP_STATE_STOR: {
                ret = ftp_state_stor_resp(conn_ptr, ftpcode, ftp_ctrl_ptr->state);
                break;
            }
            case QURL_FTP_STATE_QUIT:
                /* fallthrough, just stop! */
            default:
                /* internal error */
                ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_STOP);
                break;
        }
    }

    qurl_logd("ret:[%x]\r\n", ret);
    return ret;
}

/**
 * @brief pingpong ftp processing response line
 *
 * @param conn_ptr
 * @param buf
 * @param buf_len
 * @param pong_code_ptr
 * @return qurl_bit_t
 */
static qurl_bit_t ftp_pp_pong(qurl_conn_t *conn_ptr, char *buf, long buf_len, int *pong_code_ptr)
{
    (void)conn_ptr;

    if ((buf_len > 3) && FTP_LASTLINE(buf))
    {
        *pong_code_ptr = qurl_strtol(buf, QOSA_NULL, 10);
        qurl_logd("code:[%d]\r\n", *pong_code_ptr);
        return QOSA_TRUE;
    }

    return QOSA_FALSE;
}

static qurl_ecode_t ftp_statemach(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    qurl_logd("state:[%d]\r\n", ftp_ctrl_ptr->state);
    ret = qurl_pp_state_machine(conn_ptr, &ftp_ctrl_ptr->pp_ctrl, QOSA_FALSE, QOSA_FALSE);
    qurl_logd("state:[%d]\r\n", ftp_ctrl_ptr->state);

    *done_ptr = (ftp_ctrl_ptr->state == QURL_FTP_STATE_STOP) ? QOSA_TRUE : QOSA_FALSE;

    return ret;
}

static qurl_ecode_t ftp_block_statemach(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    while (ftp_ctrl_ptr->state != QURL_FTP_STATE_STOP)
    {
        ret = qurl_pp_state_machine(conn_ptr, &ftp_ctrl_ptr->pp_ctrl, QOSA_TRUE, QOSA_TRUE);
        if (ret != QURL_OK)
        {
            break;
        }
    }

    return ret;
}

/**
 * @brief Parse path
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_parse_url_path(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    long              path_len = 0;
    char             *raw_path_ptr = QOSA_NULL;
    const char       *file_name_ptr = QOSA_NULL; /*Complete file path*/
    const char       *slash_pos_ptr = QOSA_NULL; /* / */
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;

    ftp_ctrl_ptr->bits.ctrl_valid = QOSA_FALSE;
    ftp_ctrl_ptr->bits.cwd_faild = QOSA_FALSE;

    ret = qurl_url_decode(ftp_core_ptr->path, 0, &raw_path_ptr, (qosa_size_t *)&path_len, QURL_URL_REJECT_CTRL);
    if (ret != QURL_OK)
    {
        return ret;
    }

    switch (core_ptr->usr_cfg.ftp.ftp_filemethod)
    {
        case QURL_FTP_FILEMETHOD_NOCWD: {
            /*Fastest, but not fully compliant with standards*/
            if ((path_len > 0) && (raw_path_ptr[path_len - 1] != '/'))
            {
                /*This is a non-directory path*/
                file_name_ptr = raw_path_ptr;
            }
            /*else branch: ftp_ctrl_ptr->file is not used anywhere else, only for file operations. In other words, it will not be used for directory operations. Therefore, we can safely keep the filename as NULL here and use it as a parameter for the directory/file decision.*/
        }
        break;
        case QURL_FTP_FILEMETHOD_SINGLECWD: {
            /*Get Multipath*/
            slash_pos_ptr = qurl_strrchr(raw_path_ptr, '/');
            if (slash_pos_ptr)
            {
                /*Get the path before the last slash, excluding / */
                long dirlen = slash_pos_ptr - raw_path_ptr;
                if (dirlen == 0)
                {
                    dirlen = 1;
                }

                ftp_ctrl_ptr->dirs_pptr = qurl_malloc(sizeof(ftp_ctrl_ptr->dirs_pptr[0]));
                if (!ftp_ctrl_ptr->dirs_pptr)
                {
                    qurl_free(raw_path_ptr);
                    return QURL_ECODE_NO_MEMORY;
                }

                ftp_ctrl_ptr->dirs_pptr[0] = qurl_malloc(dirlen + 1);
                if (!ftp_ctrl_ptr->dirs_pptr[0])
                {
                    qurl_free(raw_path_ptr);
                    return QURL_ECODE_NO_MEMORY;
                }

                qurl_strncpy(ftp_ctrl_ptr->dirs_pptr[0], raw_path_ptr, dirlen);
                ftp_ctrl_ptr->dir_depth = 1;       /*Single directory*/
                file_name_ptr = slash_pos_ptr + 1; /*The text after the slash is the filename.*/
            }
            else
            {
                file_name_ptr = raw_path_ptr; /* file name only (or empty) */
            }
            break;
        }
        default: /* allow pretty much anything */
        case QURL_FTP_FILEMETHOD_MULTICWD: {
            /*Current position: start of the next path component*/
            const char *cur_pos_ptr = raw_path_ptr;
            /*Number of entries allocated for the 'dirs_pptr' array*/
            long        dir_alloc_cnt = 0;
            const char *str = raw_path_ptr;

            for (; *str != 0; ++str)
            {
                if (*str == '/')
                {
                    ++dir_alloc_cnt;
                }
            }

            if (dir_alloc_cnt)
            {
                ftp_ctrl_ptr->dirs_pptr = qurl_malloc(dir_alloc_cnt * sizeof(ftp_ctrl_ptr->dirs_pptr[0]));
                if (!ftp_ctrl_ptr->dirs_pptr)
                {
                    qurl_free(raw_path_ptr);
                    return QURL_ECODE_NO_MEMORY;
                }

                /*Parse the URL path into separate path components*/
                while ((slash_pos_ptr = qurl_strchr(cur_pos_ptr, '/')) != QOSA_NULL)
                {
                    long comp_len = slash_pos_ptr - cur_pos_ptr;

                    /*If the path starts with a slash: add it as a directory*/
                    if ((comp_len == 0) && (ftp_ctrl_ptr->dir_depth == 0))
                        ++comp_len;

                    /** We skip empty path components, such as "x//y",
Because the FTP command CWD requires a parameter, and a non-existent parameter: does not work on many servers; has no effect on other servers.*/
                    if (comp_len > 0)
                    {
                        char *comp = qurl_malloc(comp_len + 1);
                        if (!comp)
                        {
                            qurl_free(raw_path_ptr);
                            return QURL_ECODE_NO_MEMORY;
                        }
                        qurl_strncpy(comp, cur_pos_ptr, comp_len);
                        comp[comp_len] = 0;
                        ftp_ctrl_ptr->dirs_pptr[ftp_ctrl_ptr->dir_depth++] = comp;
                    }
                    cur_pos_ptr = slash_pos_ptr + 1;
                }
            }
            file_name_ptr = cur_pos_ptr; /*The remainder is the filename, which may not exist.*/
        }
        break;
    }

    if (file_name_ptr && *file_name_ptr)
    {
        ftp_ctrl_ptr->file_ptr = qurl_strdup(file_name_ptr);
    }
    else
    {
        ftp_ctrl_ptr->file_ptr = QOSA_NULL;
    }

    if (core_ptr->cfg.upload_size && !ftp_ctrl_ptr->file_ptr && (ftp_core_ptr->transfer == QURL_PP_TRANSFER_BODY))
    {
        /*Since the data body is to be uploaded, the target path must be a file; if it does not exist, an error should be reported.*/
        qurl_free(raw_path_ptr);
        return QURL_ECODE_URL_MALFORMED;
    }

    /*Initialization of cwd completion flag*/
    ftp_ctrl_ptr->bits.cwd_done = QOSA_FALSE;

    if ((core_ptr->usr_cfg.ftp.ftp_filemethod == QURL_FTP_FILEMETHOD_NOCWD) && (raw_path_ptr[0] == '/'))
    {
        ftp_ctrl_ptr->bits.cwd_done = QOSA_TRUE; /*Absolute paths can bypass the CWD and access directly.*/
    }
    else
    {
        /*The newly created FTP connection is already in the entry path.*/
        const char *old_path_ptr = conn_ptr->bits.is_reuse ? ftp_ctrl_ptr->prevpath_ptr : "";
        if (old_path_ptr)
        {
            long n = path_len;
            if (core_ptr->usr_cfg.ftp.ftp_filemethod == QURL_FTP_FILEMETHOD_NOCWD)
            {
                n = 0; /*CWD enters the relative path directly.*/
            }
            else
            {
                n -= ftp_ctrl_ptr->file_ptr ? qurl_strlen(ftp_ctrl_ptr->file_ptr) : 0;
            }

            if ((qurl_strlen(old_path_ptr) == (qosa_size_t)n) && !qurl_strncmp(raw_path_ptr, old_path_ptr, n))
            {
                /*If the current path and the target path are the same, there is no need to execute CWD.*/
                ftp_ctrl_ptr->bits.cwd_done = QOSA_TRUE;
                qurl_logd("Request has same path as previous transfer\r\n");
            }
        }
    }

    qurl_free(raw_path_ptr);
    return QURL_OK;
}

static qurl_ecode_t ftp_perform(
    qurl_conn_t *conn_ptr,
    qurl_bit_t  *connected_ptr, /*Data Connection*/
    qurl_bit_t  *done_ptr
)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;

    if (core_ptr->usr_cfg.bits.opt_nobody)
    {
        ftp_core_ptr->transfer = QURL_PP_TRANSFER_INFO; /*Do not pass body*/
    }

    *done_ptr = QOSA_FALSE; /*Initialize the completion flag for this phase*/

    /*First, check if the QUOTE command needs to be sent; if so, this will be the first user-specified command.*/
    ret = ftp_state_quote(conn_ptr, QOSA_TRUE, QURL_FTP_STATE_QUOTE);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /*Run the state machine*/
    ret = ftp_statemach(conn_ptr, done_ptr);

    /*[lzm][todo]Check if the data connection is already established*/
    *connected_ptr = qurl_conn_is_connected(conn_ptr, QURL_SOCKET_INDEX_1);

    if (*done_ptr)
    {
        qurl_logd("req phase is complete1\r\n");
    }

    return ret;
}

/**
 * @brief Function for implementing data transmission
 *
 * @param conn_ptr
 * @param done_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_regular_transfer(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t     ret = QURL_OK;
    qurl_bit_t       connected = QOSA_FALSE;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    ftp_ctrl_ptr->bits.ctrl_valid = QOSA_TRUE; /*Before preparing the data connection, first mark the control connection as valid.*/

    ret = ftp_perform(conn_ptr, &connected, done_ptr);
    if (ret == QURL_OK)
    {
        if (!*done_ptr)
        {
            return ret;
        }

        ret = ftp_reqphase_done(conn_ptr, connected);

        if (ret != QURL_OK)
        {
            return ret;
        }
    }
    else
    {
        ftp_free_dirs(ftp_ctrl_ptr);
    }

    return ret;
}

/**
 * @brief Called when the req phase is completed
 *
 * @param conn_ptr
 * @param connected[in] Whether the data connection has been established
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_reqphase_done(qurl_conn_t *conn_ptr, qurl_bit_t connected)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;

    if (connected)
    {
        int completed = 0;
        ret = ftp_req_more(conn_ptr, &completed);
        if (ret)
        {
            return ret;
        }
    }

    if (ftp_core_ptr->transfer != QURL_PP_TRANSFER_BODY)
    {
        /* no data to transfer */
        // Curl_setup_transfer(conn_ptr->data, -1, -1, QOSA_FALSE, -1);
    }
    else if (!connected)
    {
        /*Data connection needs to be established.*/
        conn_ptr->bits.req_more = QOSA_TRUE;
    }

    ftp_ctrl_ptr->bits.ctrl_valid = QOSA_TRUE; /*Control connection valid*/

    return ret;
}

static qurl_ecode_t ftp_upload_data(qurl_conn_t *conn_ptr)
{
    long              len = 0;
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    qurl_logd("enter\r\n");

    len = core_ptr->trans.upload_len_max - core_ptr->trans.uploaded_len;
    if (len > 0
        || (core_ptr->trans.upload_len_max < 0 && core_ptr->usr_cfg.upload_data_ptr == QOSA_NULL && core_ptr->usr_cfg.upload_file_ptr == QOSA_NULL
            && core_ptr->usr_cfg.read_from_client_func != QOSA_NULL) /*Only callback function mode is supported for unspecified length.*/
    )
    {
        core_idle_timeout_feed_dog(core_ptr);
        if (core_ptr->usr_cfg.upload_data_ptr)
        {
            /*[lzm][note]If encountering customer reports of insufficient memory, then handle this. Send customer data in batches, add length limit to dbuf, add progress bar, so that all data is not pushed to dbuf at once.*/
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
                return QURL_ECODE_FS_READ_ERR;
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
                ret = QURL_ECODE_NETWORK_ERR;
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
        QURL_CORE_IDLE_TIMEOUT_CHECK_DOG_FUNC(core_ptr);
    }

    qurl_logd("upload plan:[%ld][%ld]\r\n", core_ptr->trans.upload_len_max, core_ptr->trans.uploaded_len);
    return ret;
}

static qurl_ecode_t ftp_download_data(qurl_conn_t *conn_ptr)
{
    long              len = 0;
    long              ret_len = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

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
            if (core_ptr->usr_cfg.write_to_client_func)
            {
                /** [lzm][note]
				 * Now: When the length returned in the callback function is less than the length passed in, it indicates a failure on the user side call.
				 * Optimization 1: Thanks to the dbuf packet management, we can also change this callback logic: when the user-side return value is:
				 *  ret_len < 0: An error occurred.
				 * 	0 <= ret_len < len: The user has received data normally, but due to limited buffer capacity, not all data could be received. The qurl can retain this unreceived data.
				 *  len <= ret_len: Receive the entire disk.
				 * Optimization 2: Additionally, qurl can append flow control APIs. In the callback, if the user fails to fully receive the data, flow control is automatically triggered, pausing reporting until the user side calls the relevant API to notify the internal system to continue.
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

/**
 * @brief Blocking wait for server response code.
 *
 * @param conn_ptr
 * @param nread_ptr
 * @param ftp_code_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_get_respons(qurl_conn_t *conn_ptr, long *nread_ptr, int *ftp_code_ptr)
{
    qurl_bit_t        deal_flag = QOSA_FALSE;
    qurl_ecode_t      ret = QURL_OK;
    int               value_to_be_ignored = 0;
    long              nread = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    qurl_logd("enter\r\n");

    if (ftp_code_ptr)
    {
        *ftp_code_ptr = 0;
    }
    else
    {
        ftp_code_ptr = &value_to_be_ignored;
    }

    *nread_ptr = 0;

    while (!*ftp_code_ptr && ret == QURL_OK)
    {
        qurl_time_t interval_ms = 0;
        qurl_time_t timeout = qurl_pp_state_timeout(conn_ptr, pp_ctrl_ptr, QOSA_FALSE);

        /*Blocking and waiting for server responses can also cause a hang-up phenomenon, so monitoring points can be placed.*/
        QURL_CORE_CHECK_ABORT_FUNC(core_ptr);

        deal_flag = QOSA_FALSE;

        if (timeout <= 0)
        {
            return QURL_ECODE_PERFORM_TIMEOUT;
        }

        interval_ms = 1000; /* 1s */
        if (interval_ms > timeout)
        {
            interval_ms = timeout;
        }

        /*Check if there is any unretrieved data*/
        /** Check if there is data in the lower-level cache*/
        if (qurl_conn_recv_cache_paending(conn_ptr, QURL_SOCKET_INDEX_0) > 0)
        {
            deal_flag = QOSA_TRUE;
        }
        /** Check if the pong cache of pp also has unretrieved data*/
        else if (qurl_pp_moredata(pp_ctrl_ptr))
        {
            deal_flag = QOSA_TRUE;
        }
        /*Check if there is new data in the socket interface*/
        else
        {
            long ping_len = 0;
            long send_len = 0;

            ping_len = qurl_dbuf_vaild_size(pp_ctrl_ptr->ping_cache);
            send_len = qurl_dbuf_vaild_size(conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].send_cache);

            ret = qurl_socket_check(
                conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd,
                (ping_len > 0 || send_len > 0) ? (QURL_SELECT_BIT_WRITE | QURL_SELECT_BIT_ERROR) : (QURL_SELECT_BIT_READ | QURL_SELECT_BIT_ERROR),
                interval_ms
            );
            if (ret < 0)
            {
                qurl_loge("socket faild:0x%x\r\n", ret);
                return QURL_ECODE_PP_ERR;
            }
            if (ret > 0)
            {
                deal_flag = QOSA_TRUE;
            }
            ret = QURL_OK;
        }

        if (deal_flag)
        {
            ret = ftp_pp_read_pong(conn_ptr, pp_ctrl_ptr, ftp_code_ptr, &nread);
            if (ret != QURL_OK)
            {
                break;
            }
        }
        else
        {
            continue;
        }
    }

    pp_ctrl_ptr->pending_resp = QOSA_FALSE;

    return ret;
}

/* API */
/**
 * @brief Create protocol stack service
 *
 * @param conn_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_setup_connect(qurl_conn_t *conn_ptr)
{
    /*1. Create protocol stack operation data structure*/
    /*2. Initialize the protocol operation data structure*/
    /*3. Configure conn according to the current protocol type*/
    char             *type = QOSA_NULL;
    char             *tmp_ptr = QOSA_NULL;
    qurl_ftp_core_t  *ftp_core_ptr = QOSA_NULL;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    qurl_logd("enter\r\n");

    /*1. Create protocol stack operation data structure*/
    ftp_core_ptr = qurl_malloc(sizeof(qurl_ftp_core_t));
    if (ftp_core_ptr == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }
    qurl_memset(ftp_core_ptr, 0x00, sizeof(qurl_ftp_core_t));

    /*Protocol stack data structure initialization*/
    ftp_ctrl_ptr->state = QURL_FTP_STATE_STOP;

    qurl_url_getopt(core_ptr->cfg.uurl_ptr, QURL_URL_OPT_PATH, &tmp_ptr, QURL_URL_URLENCODE); /*Encoded PATH required*/
    /*Need to remove the first slash*/
    if (tmp_ptr[0] == '/')
    {
        ftp_core_ptr->path = qurl_strdup(&tmp_ptr[1]);
        qurl_free(tmp_ptr);
    }
    else
    {
        ftp_core_ptr->path = tmp_ptr;
    }
    tmp_ptr = QOSA_NULL;

    type = qurl_strstr(ftp_core_ptr->path, ";type=");
    if (type)
    {
        *type = 0; /*It is in the middle of the hostname*/

        switch (type[6])
        {
            case 'a': /* ASCII mode */
            case 'A': /* ASCII mode */
                core_ptr->cfg.bits.prefer_ascii = QOSA_TRUE;
                break;

            case 'd': /* directory mode */
            case 'D': /* directory mode */
                core_ptr->usr_cfg.bits.list_only = QOSA_TRUE;
                break;

            case 'i': /* binary mode */
            case 'I': /* binary mode */
            default:
                /* switch off ASCII */
                core_ptr->cfg.bits.prefer_ascii = QOSA_FALSE;
                break;
        }
    }

    ftp_core_ptr->transfer = QURL_PP_TRANSFER_BODY;
    ftp_ctrl_ptr->known_filesize = -1;

    /*Register*/
    if (core_ptr->bits.prot_data_alloc_flag && core_ptr->prot_data != QOSA_NULL)
    {
        qurl_ftp_core_t *old_ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;
        if (old_ftp_core_ptr->path)
        {
            qurl_free(old_ftp_core_ptr->path);
            old_ftp_core_ptr->path = QOSA_NULL;
        }
        qurl_free(core_ptr->prot_data);
    }
    core_ptr->prot_data = (void *)ftp_core_ptr;
    core_ptr->bits.prot_data_alloc_flag = QOSA_TRUE;

    /**
	 * [lzm][note] Clear the data left over from the last time.
	 * Scenario: Under persistent connections, if the last operation was interrupted due to a timeout or notification from another thread, preventing the reception of the remaining response,
	 * These responses must not persist into this connection.
	 * [lzm][todo]
	 * 1. Of course, we can also retrieve these legacy data and utilize them through analysis,
	 * If it is a result code related to disconnecting the control connection, we can handle it accordingly.
	 */
    if (ftp_ctrl_ptr->bits.pp_valid != QOSA_FALSE)
    {
        qurl_logd("index_error_judgment:%d\r\n", conn_ptr->index_error_judgment);
        if (conn_ptr->index_error_judgment == 1)
        {
            ftp_train_logo_judgment(conn_ptr, core_ptr, 0);
            conn_ptr->index_error_judgment = 0;
        }
        qurl_pp_clear_pingpong(conn_ptr, &ftp_ctrl_ptr->pp_ctrl);
    }

    /*[lzm][note]The control connection is changed to non-TLS by default. Whether it is TLS is determined by the FTP PROT command; the TLS connection is established by ftp_init_transfer().*/
    conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].bits.is_tls = QOSA_FALSE;

    return QURL_OK;
}

/**
 * @brief Send protocol connection
 * FTP: Login successful and PWD executed.
 * FTPS：
 *
 * @param conn_ptr
 * @param done_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_send_prot_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_bit_t       tls_done_flag = QOSA_FALSE;
    qurl_ecode_t     ret = QURL_OK;
    qurl_pp_cfg_t    pp_cfg = {0};
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    qurl_logd("enter\r\n");

    *done_ptr = QOSA_FALSE;

    /* TLS */
    if (conn_ptr->port_handler->flags & QURL_PROT_FLAGS_SSL)
    {
        /*Configure the sending socket to be used*/
        ret = qurl_conn_connect(conn_ptr, QURL_SOCKET_INDEX_0, &tls_done_flag);
        if (ret != QURL_OK)
        {
            return ret;
        }
        conn_ptr->bits.tls_connecting = !tls_done_flag;
        conn_ptr->bits.ftp_use_control_tls = QOSA_TRUE;
    }

    /** pingpong */
    if (ftp_ctrl_ptr->bits.pp_valid == QOSA_FALSE)
    {
        pp_cfg.pong_timeout_ms = QURL_CFG_PONG_TIMEOUT_DEFAULT;
        pp_cfg.statemachine_func = ftp_pp_statemachine;
        pp_cfg.pong_func = ftp_pp_pong;

        ret = qurl_pp_init(&ftp_ctrl_ptr->pp_ctrl, &pp_cfg);
        if (ret != QURL_OK)
        {
            return ret;
        }
        ftp_ctrl_ptr->bits.pp_valid = QOSA_TRUE;
    }

    ftp_state_change(ftp_ctrl_ptr, QURL_FTP_STATE_WAIT220);

    ret = ftp_statemach(conn_ptr, done_ptr);

    return ret;
}

static qurl_ecode_t ftp_prot_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_bit_t   tls_done_flag = QOSA_FALSE;
    qurl_ecode_t ret = QURL_OK;

    qurl_logd("enter\r\n");

    *done_ptr = QOSA_FALSE;

    if (conn_ptr->bits.tls_connecting)
    {
        /*Configure the sending socket to be used*/
        ret = qurl_conn_connect(conn_ptr, QURL_SOCKET_INDEX_0, &tls_done_flag);
        if (ret != QURL_OK)
        {
            return ret;
        }
        conn_ptr->bits.tls_connecting = !tls_done_flag;
    }

    ret = ftp_statemach(conn_ptr, done_ptr);

    return ret;
}

/**
 * @brief Enter the target path and confirm the data connection information.
 *
 * @param conn_ptr
 * @param done_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_req(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    qurl_logd("enter\r\n");

    /*[lzm][todo]Implement support for wildcard path parsing*/
    if (core_ptr->cfg.bits.wildcard_match)
    {
        return QURL_ECODE_NO_SUPPORT;
    }
    else
    {
        /*Determine the path*/
        ret = ftp_parse_url_path(conn_ptr);
        if (ret != QURL_OK)
        {
            return ret;
        }
    }

    ret = ftp_regular_transfer(conn_ptr, done_ptr);

    return ret;
}

/**
 * @brief Enter the target path and confirm the data connection information.
 *
 * @param conn_ptr
 * @param done_ptr
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_reqing(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    qurl_logd("enter\r\n");

    /*Enter the target path and confirm the data connection information.*/
    ret = ftp_statemach(conn_ptr, done_ptr);
    if (ret != QURL_OK)
    {
        return ret;
    }
    else if (*done_ptr)
    {
        /*If needed, req_more will be executed here.*/
        ret = ftp_reqphase_done(conn_ptr, QOSA_FALSE);
    }

    return ret;
}

/**
 * @brief Data Connection
 *
 * @param conn_ptr
 * @param completep_ptr[out] is a return parameter indicating the processing status.
 *  0, indicates incomplete processing;
 *  1, indicates that processing is complete;
 *  -1 indicates that a return is needed (typically used when sending a PASV command to retry a failed EPSV command).
 * @return qurl_ecode_t
 */
static qurl_ecode_t ftp_req_more(qurl_conn_t *conn_ptr, int *completep_ptr)
{
    qurl_bit_t        connected = 0;
    qurl_bit_t        complete = 0;
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    qurl_logd("enter\r\n");

    /*Check if the data connection is valid*/
    if (!qurl_conn_is_connected(conn_ptr, QURL_SOCKET_INDEX_1))
    {
        if (core_ptr->usr_cfg.bits.ftp_use_port)
        {
            /*Active mode: Creates a TCP server only when a command requiring a data connection is issued later.*/
        }
        else
        {
            /*Passive Mode*/

            /** Create a socket and actively connect to the server*/
            ret = qurl_conn_connect(conn_ptr, QURL_SOCKET_INDEX_1, &connected);
            if (ret != QURL_OK)
            {
                if (ftp_ctrl_ptr->count1 == 0)
                {
                    return ftp_epsv_disable(conn_ptr);
                }
            }
            if (!connected)
            {
                return ret;
            }
        }
    }

    /*Operating state machine*/
    if (ftp_ctrl_ptr->state != QURL_FTP_STATE_STOP)
    {
        ret = ftp_statemach(conn_ptr, &complete);
        *completep_ptr = complete;

        if (ret != QURL_OK || ftp_ctrl_ptr->bits.wait_data_conn == QOSA_FALSE)
        {
            /*If it fails, or if there is no data to wait for, then return directly.*/
            return ret;
        }

        /*Reset the completion flag. Because if the flag wait_data_conn indicates waiting for a data connection, the FTP state machine will be set to STOP, and complete returns true,
But the data connection is not yet completed. So the reset here is to handle the situation where (wait_data_conn && complete (the complete at this point is virtual)).*/
        *completep_ptr = QOSA_FALSE;
    }

    /*Preparation before transmission*/
    if (ftp_core_ptr->transfer <= QURL_PP_TRANSFER_INFO)
    {
        /** When data transmission is required,
		 * If the file name has not yet been provided, the SIZE command must be executed later to obtain the file size, and the correct transfer mode must be determined before that.
		 */

        if (ftp_ctrl_ptr->bits.wait_data_conn)
        {
            /*Active*/
            qurl_bit_t serv_conned = QOSA_FALSE;

            ret = ftp_received_server_connect(conn_ptr, &serv_conned);
            if (ret != QURL_OK)
            {
                return ret;
            }
            if (serv_conned)
            {
                ret = ftp_accept_server_connect(conn_ptr);
                ftp_ctrl_ptr->bits.wait_data_conn = QOSA_FALSE; /*Connection established*/
                if (ret == QURL_OK)
                {
                    ret = ftp_init_transfer(conn_ptr);
                }

                if (ret != QURL_OK)
                {
                    return ret;
                }

                *completep_ptr = 1; /*This state is now complete when the server connects back to us.*/
            }
        }
        else if (core_ptr->cfg.bits.upload)
        {
            /*Upload*/

            /*Determine upload length*/
            core_ptr->trans.upload_len_max = core_ptr->cfg.upload_size;
            core_ptr->trans.uploaded_len = 0;

            /** Correct transmission mode + Enter STOR command to upload data*/
            ret = ftp_nb_type(conn_ptr, core_ptr->cfg.bits.prefer_ascii, QURL_FTP_STATE_STOR_TYPE);
            if (ret != QURL_OK)
            {
                return ret;
            }

            ret = ftp_statemach(conn_ptr, &complete);

            *completep_ptr = (int)complete;
        }
        else
        {
            /*Download*/

            /*Determine download length*/
            /** [lzm][todo]This will clear the previous download records, so if there is time for optimization, this part needs to be improved by adding a record value to track the actual downloaded length, and support not specifying the length.*/
            core_ptr->trans.download_len_max = -1;

            /*Based on the range, determine the maximum download length limit for this session.*/
            if (core_ptr->cfg.bits.use_range && core_ptr->cfg.range_ptr)
            {
                long         from = 0;
                long         to = 0;
                char        *ptr = QOSA_NULL;
                char        *ptr2 = QOSA_NULL;
                qurl_ecode_e ret_from = QURL_OK;
                qurl_ecode_e ret_to = QURL_OK;

                ret_from = qurl_strtoofft(core_ptr->cfg.range_ptr, &ptr, 10, &from);
                if (ret_from == QURL_ECODE_OVER_LIMIT)
                {
                    return ret_from;
                }
                while (*ptr && (qurl_ctype_isblank(*ptr) || (*ptr == '-')))
                {
                    ptr++;
                }
                ret_to = qurl_strtoofft(ptr, &ptr2, 10, &to);
                if (ret_to == QURL_ECODE_OVER_LIMIT)
                {
                    return ret_to;
                }
                if ((ret_to == QURL_ECODE_PARAM_INVALID) && ret_from == QURL_OK)
                {
                    /* X - */
                    core_ptr->cfg.resume_from = from;
                }
                else if ((ret_from == QURL_ECODE_PARAM_INVALID) && ret_to == QURL_OK)
                {
                    /* -Y */
                    core_ptr->cfg.download_max = to;
                    core_ptr->cfg.resume_from = 0;
                    qurl_logd("RANGE the last %d bytes\r\n", to);
                }
                else
                {
                    /* X-Y */
                    long totalsize = 0;

                    /*Ensure the range is reasonable - it should be followed.*/
                    if (from > to)
                    {
                        return QURL_ECODE_RANGE_ERR;
                    }

                    totalsize = to - from;
                    /*[lzm][note]Overflow check can be performed*/

                    core_ptr->cfg.download_max = totalsize + 1; /* include last byte */
                    core_ptr->cfg.resume_from = from;
                    qurl_logd("RANGE from %d getting %d bytes", from, core_ptr->cfg.download_max);
                }
            }
            else
            {
                core_ptr->cfg.download_max = -1;
            }

            if (ret == QURL_OK && core_ptr->cfg.download_max >= 0)
            {
                /*With a confirmed download size, there is no need to check the length attached to the final successful transmission of 226 or 250.*/
                ftp_ctrl_ptr->bits.dont_check = QOSA_TRUE;
            }

            if (core_ptr->usr_cfg.bits.list_only || ftp_ctrl_ptr->file_ptr == QOSA_NULL)
            {
                /*If no filename is specified, or a filename is not required, then we consider this a request for directory content, requiring the use of LIST.
At this point, the program first checks whether the file body transfer is requested. If it is, it calls the ftp_nb_type function to set the ASCII transfer type and saves the result in the result variable.
If the function returns an error value, exit. If file body transfer is not requested, skip this step.*/
                if (ftp_core_ptr->transfer == QURL_PP_TRANSFER_BODY)
                {
                    /*Correct transmission mode + Enter LIST command*/
                    ret = ftp_nb_type(conn_ptr, QOSA_TRUE, QURL_FTP_STATE_LIST_TYPE);
                    if (ret != QURL_OK)
                    {
                        return ret;
                    }
                }
            }
            else
            {
                /*Need to transmit file content*/
                /*Correct transmission mode + Enter RETR command to read file content*/
                ret = ftp_nb_type(conn_ptr, core_ptr->cfg.bits.prefer_ascii, QURL_FTP_STATE_RETR_TYPE);
                if (ret != QURL_OK)
                {
                    return ret;
                }
            }
            ret = ftp_statemach(conn_ptr, &complete);
            *completep_ptr = (int)complete;
        }
        return ret;
    }

    if (ftp_ctrl_ptr->bits.wait_data_conn == QOSA_FALSE)
    {
        *completep_ptr = 1; /*Upon arriving here, the req_more state completes its task.*/
    }

    return ret;
}

static qurl_ecode_t ftp_trans_data(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    long              len = 0;
    qurl_ecode_t      ret = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ecode_t      ret_select = 0;

    qurl_logd("enter\r\n");

    *done_ptr = QOSA_FALSE;

    if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd == QURL_SOCKET_BAD)
    {
        /*No data needs to be transmitted*/
        *done_ptr = QOSA_TRUE;
        return QURL_OK;
    }

    if (core_ptr->cfg.bits.upload)
    {
        ret = ftp_upload_data(conn_ptr);
        if (ret != QURL_OK)
        {
            return ret;
        }

        /*Push to conn*/
        qurl_dbuf_concat_tail(conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].send_cache, core_ptr->send_cache);

        ret = qurl_conn_send_cache(conn_ptr, QURL_SOCKET_INDEX_1);
        if (ret != QURL_OK)
        {
            qurl_loge("upload faild\r\n");
            return ret;
        }

        if (core_ptr->trans.upload_len_max >= 0 && core_ptr->trans.upload_len_max <= core_ptr->trans.uploaded_len) /*There is also body data that needs to be uploaded.*/
        {
            /*Upload completed*/
            *done_ptr = QOSA_TRUE;
        }
    }
    else
    {
        /*Check if writable*/
        ret_select = qurl_socket_check(conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd, QURL_SELECT_BIT_READ | QURL_SELECT_BIT_ERROR, 0);
        if (ret_select & QURL_SELECT_BIT_ERROR)
        {
            /*[lzm][note]To be optimized. This may be a case where the server has finished sending data and actively closed the data connection. This is normal behavior.*/
            //The accuracy through the errno method is not high, especially in abnormal situations such as multiple paths, and should be used with caution in other locations.
            //QFTPGET may encounter situations where the server does not send a 226 response and directly sends a FIN to terminate the connection.
            ret = qurl_get_errno();
            qurl_logd("select ret:%x\r\n", ret);
            /*this happens when a socket got closed while waiting*/
            if (ret == EBADF)
            {
                return QURL_OK;  //QURL_ECODE_SOCK_CLOSE_FAILED Received from server, underlying FIN event does not report an error
            }
            return QURL_ECODE_SOCK_SELECT_FAILED;
        }

        do {
            /*If there is sustained high-volume inflow, the service will also remain in this loop for processing over an extended period, so monitoring points can be placed.*/
            QURL_CORE_CHECK_ABORT_FUNC(core_ptr);

            ret = qurl_conn_recv_cache(conn_ptr, QURL_SOCKET_INDEX_1, QURL_CONN_DEAL_CACHE_SIZE); /*The second parameter is recommended to be modified.*/
            if (ret != QURL_OK && (qurl_ecode_e)ret != QURL_ECODE_TLS_READ_AGAIN)
            {
                /*[lzm][note]To be optimized. This may be a case where the server has finished sending data and actively closed the data connection. This is normal behavior.*/
                qurl_logd("read ret:%x\r\n", ret);
                return ret;
            }
            qurl_dbuf_concat_tail(core_ptr->recv_cache, conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].recv_cache);
            /*[lzm][note]Here you can add a check for ret to see if it is due to the limit restriction of dbuf.*/

            len = qurl_dbuf_vaild_size(core_ptr->recv_cache);
            if (len <= 0)
            {
                /* [lab][202405062321] */
                if ((len == 0 && (ret_select & QURL_SELECT_BIT_READ) && (qurl_ecode_e)ret != QURL_ECODE_TLS_READ_AGAIN)
                    || conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].bits.rx_closed != QOSA_FALSE)
                {
                    /*Readable, but no data indicates: transmission complete.*/
                    *done_ptr = QOSA_TRUE;
                    qurl_logd("download plan:[%ld][%ld]\r\n", core_ptr->trans.download_len_max, core_ptr->trans.downloaded_len);
                    if ((core_ptr->trans.download_len_max >= 0) && (core_ptr->trans.downloaded_len < core_ptr->trans.download_len_max))
                    {
                        ret = QURL_ECODE_SOCK_CONN_CLOSED;
                    }
                    else
                    {
                        ret = QURL_OK;
                    }
                }
                else
                {
                    ret = QURL_OK;
                }
                return ret;
            }

            /*Transmission content*/
            ret = ftp_download_data(conn_ptr);
            if (ret != QURL_OK)
            {
                return ret;
            }

            if ((core_ptr->trans.download_len_max >= 0) && (core_ptr->trans.downloaded_len >= core_ptr->trans.download_len_max))
            {
                /*Download complete*/
                *done_ptr = QOSA_TRUE;
                break;
            }

            //Platform differences, lwip socket may not return errors, GET/PUT may block until timeout, does not check network status here, may involve time-consuming checks, affecting transmission efficiency
            ret_select = 0; /*Mark only effective for the first time*/
        } while (1);
    }

    return QURL_OK;
}

/**
 * @brief   Status determination and log recording after FTP transmission
 *
 * @param[in] conn_ptr Connection control block pointer
 *          - Contains FTP connection related information
 *
 * @param[in] core_ptr Core control block pointer
 *
 * @param[in] premature  Whether to end early flag
 *
 * @return qurl_ecode_t error code
 *
 */
static qurl_ecode_t ftp_train_logo_judgment(qurl_conn_t *conn_ptr, qurl_core_ctrl_t *core_ptr, qurl_bit_t premature)
{
    qurl_ecode_t     ret = 0;
    long             nread = 0;
    int              ftpcode = 0;
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t  *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;
    qurl_time_t      old_timeout_ms = pp_ctrl_ptr->pong_timeout_ms;
    if (conn_ptr->index_error_judgment == 1)
    {
        pp_ctrl_ptr->pong_timeout_ms = old_timeout_ms / 2; /*Reset*/
    }
    pp_ctrl_ptr->ping_time = qurl_time_now();              /*Starting from the occurrence, wait for the server response code.*/

    ret = ftp_get_respons(conn_ptr, &nread, &ftpcode);

    pp_ctrl_ptr->pong_timeout_ms = old_timeout_ms; /*Restore Settings*/

    if (nread == 0 && ret == (qurl_ecode_t)QURL_ECODE_PERFORM_TIMEOUT)
    {
        ftp_ctrl_ptr->bits.ctrl_valid = QOSA_FALSE;
    }

    if (ret != QURL_OK)
    {
        return ret;
    }
    if (ftp_ctrl_ptr->bits.dont_check && core_ptr->cfg.download_max > 0)
    {
        /*We already have the size length, so there is no need to check the content of this result code.*/
        return ret;
    }

    if (ftp_ctrl_ptr->bits.dont_check == QOSA_FALSE)
    {
        /*We need to check our detailed content, such as how much data we have reported, etc.*/
        switch (ftpcode)
        {
            case 226:
            case 250:
                break;
            case 552:
                qurl_logd("Exceeded storage allocation\r\n");
                ret = QURL_ECODE_FTP_REMOTE_DISK_FULL;
                break;
            case 425:
                qurl_logd("Connection aborted\r\n");  //When using qftpput, if the data session has already completed with 'done', and the abort command issued by the server control session ends, no error is reported.
                break;
            default:
                qurl_logd("server did not report OK, got %d\r\n", ftpcode);
                ret = QURL_ECODE_FTP_PARTIAL_FILE;
                break;
        }
    }
    /* log */
    if (ret != QURL_OK || premature)
    {
        ;
    }
    else if (core_ptr->cfg.bits.upload)
    {
        /*Upload*/
    }
    else
    {
        /*Download*/
    }
    return ret;
}

static qurl_ecode_t ftp_trans_done(qurl_conn_t *conn_ptr, qurl_ecode_t ecode, qurl_bit_t premature)
{
    qurl_ecode_t      ret = 0;
    char             *raw_path = QOSA_NULL;
    int               path_len = 0;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&ftp_ctrl_ptr->pp_ctrl;

    if (ftp_core_ptr == QOSA_NULL)
    {
        return QURL_OK;
    }

    qurl_logd("ecode:[%x]\r\n", ecode);

    switch (ecode)
    {
        case QURL_ECODE_FTP_CMD_QUOTE_ERR:
        case QURL_ECODE_FTP_CMD_TYPE_ERR:
        case QURL_ECODE_FTP_CMD_PRET_ERR:
        case QURL_ECODE_FTP_CMD_PROT_ERR:
        case QURL_ECODE_FTP_REMOTE_ACCESS_DENIED:
        case QURL_ECODE_FTP_REMOTE_FILE_NOT_FOUND:
        case QURL_ECODE_FTP_COULDNT_RETR_FILE:
        case QURL_ECODE_PERFORM_TIMEOUT: /*It's just an operation timeout, but it's still available.*/
        case QURL_OK: {
            /*When these errors occur, the connection can still be maintained.*/
            if (!premature)
            {
                break;
            }
        }
        break;
        default: {
            /*The current connection is no longer usable.*/
            ftp_ctrl_ptr->bits.ctrl_valid = QOSA_FALSE;
            ftp_ctrl_ptr->bits.cwd_faild = QOSA_TRUE; /*Set to TRUE to prevent us from remembering the current path, as this connection is closing.*/

            ret = ecode;                              /*Use the pre-configured error code as the result to return*/
        }
        break;
    }

    /*[lzm][todo]To be developed wildcard*/

    /*Maintain connection: first record the current working directory of FTP*/
    if (ret == QURL_OK)
    {
        /*Record the RAW URL for persistent transmission*/
        ret = qurl_url_decode(ftp_core_ptr->path, 0, &raw_path, (qosa_size_t *)&path_len, QURL_URL_REJECT_CTRL);
    }

    if (ret != QURL_OK)
    {
        ftp_ctrl_ptr->bits.ctrl_valid = QOSA_FALSE; /*Control connection failure*/
        qurl_safe_free(ftp_ctrl_ptr->prevpath_ptr);
        ftp_ctrl_ptr->prevpath_ptr = QOSA_NULL;
    }
    else
    {
        if ((core_ptr->usr_cfg.ftp.ftp_filemethod == QURL_FTP_FILEMETHOD_NOCWD) && (raw_path[0] == '/'))
        {
            /*CWD was not used, so the FTP working directory was not changed; no need to update the record of the working directory.*/
            qurl_free(raw_path);
            raw_path = QOSA_NULL;
        }
        else
        {
            qurl_safe_free(ftp_ctrl_ptr->prevpath_ptr);
            ftp_ctrl_ptr->prevpath_ptr = QOSA_NULL;

            if (ftp_ctrl_ptr->bits.cwd_faild == QOSA_FALSE)
            {
                if (core_ptr->usr_cfg.ftp.ftp_filemethod == QURL_FTP_FILEMETHOD_NOCWD)
                {
                    path_len = 0;
                }
                else
                {
                    path_len -= ftp_ctrl_ptr->file_ptr ? qurl_strlen(ftp_ctrl_ptr->file_ptr) : 0;
                }

                raw_path[path_len] = '\0';
                ftp_ctrl_ptr->prevpath_ptr = raw_path;
            }
            else
            {
                /*No path*/
                qurl_free(raw_path);
                raw_path = QOSA_NULL;
            }
        }

        if (ftp_ctrl_ptr->prevpath_ptr)
        {
            qurl_logd("Remembering we are in dir \"%s\"\r\n", ftp_ctrl_ptr->prevpath_ptr);
        }
    }

    /*Number of paths released*/
    ftp_free_dirs(ftp_ctrl_ptr);

    //FTP control connection establishment failed (SYN failure): core_perform state:[TRANS_DONE] => qurl_conn_core_trans_done => ftp_trans_done also passes through here

    do {
        //First, qurl_conn_conn_free is for QURL_SOCKET_SERVER_INDEX_0 (not QURL_SOCKET_INDEX_0)
        //conn_conn_server  TCP server dedicated (main listening socket), such as in FTP active mode (also for data connection)
        qurl_socket_t index1_sock_fd = conn_ptr->conn_conn_server.sock_fd;

        qurl_logd("index0_sock_fd: %d", index1_sock_fd);
        if (QURL_SOCKET_BAD == conn_ptr->conn_conn_server.sock_fd)
        {
            break;
        }

        conn_ptr->index1_last_close_event = qurl_socket_get_last_close_event(index1_sock_fd);
    } while (0);

    /*Release data connection resources*/
    /** Release TCP server socket*/
    qurl_logd("conn_free SOCKET_SERVER_INDEX_0");
    qurl_conn_conn_free(conn_ptr, QURL_SOCKET_SERVER_INDEX_0);
    /** Close the socket to notify the server that we have completed.*/
    if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd != QURL_SOCKET_BAD)
    {
        if (ret == QURL_OK && ftp_ctrl_ptr->bits.dont_check && core_ptr->cfg.download_max > 0)
        {
            ret = qurl_pp_sendf(conn_ptr, pp_ctrl_ptr, "ABOR");
            if (ret != QURL_OK)
            {
                ftp_ctrl_ptr->bits.ctrl_valid = QOSA_FALSE;
            }
        }
    }

    {
        //QURL_SOCKET_INDEX_1 The second socket connection (usually for FTP data connection)
        //Before releasing the data connection socket, record the reason for the close event.

        //If it is a PORT, then this socket is a listened child socket, and the server connects to the module through this socket (the module provides the IP and port, and the server connects).
        //The server communicates with the server through a sub-socket, and the closing event of the sub-socket reflects the real reason for the closure.
        //This records the child socket close event (overriding the close reason of the main listening socket above).

        //1 CLOSE_EVENT_FIN: TCP connection disconnection event caused by the server actively sending a FIN packet.
        //3 CLOSE_EVENT_RST: After the connection is established, the server actively sends an RST.
        //4 CLOSE_EVENT_ACK_TIMEOUT: TCP connection disconnection event caused by internal ACK retransmission timeout
        qurl_socket_t index1_sock_fd = conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd;

        qurl_logd("index1_sock_fd: %d", index1_sock_fd);
        if (QURL_SOCKET_BAD != conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd)
        {
            //The sock_fd still exists and has not been destroyed.
            conn_ptr->index1_last_close_event = qurl_socket_get_last_close_event(index1_sock_fd);
        }
    }

    /** Some connection records need to be released. [lzm][note]If they need to be retained for secondary use, this part can be modified. Currently not in use, so free them for now.*/
    qurl_logd("conn_free SOCKET_SERVER_INDEX_1");
    qurl_conn_conn_free(conn_ptr, QURL_SOCKET_INDEX_1);

    /**
	 * Additionally, result codes need to be received and processed, such as the server's response of 226 after the transfer is complete.
	 *
	 * If an abnormal exit occurs, such as a timeout or an ABORT, some data may remain in the link and not be received. This is because the interface needs to return urgently and cannot wait for the response of this data.
	 * But don't worry, the next time you enter, ftp_setup_connect will help continue receiving and processing; currently, it is directly cleared.
	 * But the gap cannot be too small; otherwise, when clearing, the server's response may not have reached the client yet, causing confusion in the next operation.
	 */
    if ((ftp_core_ptr->transfer == QURL_PP_TRANSFER_BODY) && pp_ctrl_ptr->pending_resp)
    {
        qurl_logd("index_error_judgment:%d\r\n", conn_ptr->index_error_judgment);
        if (ret == QURL_OK && !premature && ftp_ctrl_ptr->bits.ctrl_valid)
        {
            ret = ftp_train_logo_judgment(conn_ptr, core_ptr, premature);
            conn_ptr->index_error_judgment = 0;
            qurl_logd("ret:%d\r\n", ret);
        }
        else
        {
            conn_ptr->index_error_judgment = 1;
        }
    }

    /*Clear relevant data to prepare for the next connection.*/
    ftp_core_ptr->transfer = QURL_PP_TRANSFER_BODY;
    ftp_ctrl_ptr->bits.dont_check = QOSA_FALSE;

    if (ecode == QURL_OK && ret == QURL_OK && !premature && core_ptr->usr_cfg.ftp.postquote_slist)
    {
        ret = ftp_send_quote(conn_ptr, core_ptr->usr_cfg.ftp.postquote_slist);
    }

    return ret;
}

static qurl_ecode_t ftp_disconnect(qurl_conn_t *conn_ptr, qurl_bit_t dead_connection)
{
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_ftp_ctrl_t  *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&(ftp_ctrl_ptr->pp_ctrl);
    qurl_ftp_core_t  *ftp_core_ptr = (qurl_ftp_core_t *)core_ptr->prot_data;

    if (dead_connection)
    {
        ftp_ctrl_ptr->bits.ctrl_valid = QOSA_FALSE;
    }

    ftp_quit(conn_ptr);

    if (ftp_ctrl_ptr->entrypath_ptr)
    {
        if (core_ptr->cfg.ftp.ftp_most_recent_entrypath_ptr == ftp_ctrl_ptr->entrypath_ptr)
        {
            core_ptr->cfg.ftp.ftp_most_recent_entrypath_ptr = QOSA_NULL;
        }
        qurl_free(ftp_ctrl_ptr->entrypath_ptr);
        ftp_ctrl_ptr->entrypath_ptr = QOSA_NULL;
    }

    ftp_free_dirs(ftp_ctrl_ptr);
    qurl_safe_free(ftp_ctrl_ptr->prevpath_ptr);
    if (ftp_ctrl_ptr->bits.pp_valid != QOSA_FALSE)
    {
        qurl_pp_deinit(pp_ctrl_ptr);
        ftp_ctrl_ptr->bits.pp_valid = QOSA_FALSE;
    }

    if (ftp_core_ptr->path)
    {
        qurl_free(ftp_core_ptr->path);
        ftp_core_ptr->path = QOSA_NULL;
    }
    qurl_free(ftp_core_ptr);
    core_ptr->prot_data = QOSA_NULL;
    conn_ptr->index_error_judgment = 0;
    if (core_ptr->bits.prot_data_alloc_flag)
    {
        core_ptr->bits.prot_data_alloc_flag = QOSA_FALSE;
    }

    return QURL_OK;
}

static int ftp_port_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr)
{
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    qurl_logd("enter\r\n");

    return qurl_pp_getsock(conn_ptr, &ftp_ctrl_ptr->pp_ctrl, socks_ptr);
}

static int ftp_port_req_more_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr)
{
    int              bitmap = 0; /*Corresponding to the read and write of the monitored sockfd*/
    qurl_ftp_ctrl_t *ftp_ctrl_ptr = (qurl_ftp_ctrl_t *)&(conn_ptr->prot_data_u.ftp_ctrl);

    /*
	 * [lzm][todo]Need to consider whether the control connection will actively send status when the data connection TLS handshake fails. Do we need to monitor if the control connection's select is readable in the QURL_FTP_STATE_STOP state here.
	 * curl fix link: https://github.com/curl/curl/commit/d8f01e098503b0e26ffba391fe9bf189b4b5d715
	 */

    /*Monitor control connection or data connection or control+data*/
    qurl_logd("enter\r\n");
    /*Active mode is waiting for the FTP server to connect, or passive mode has started connecting but has not yet succeeded, then listen.*/
    if ((ftp_ctrl_ptr->bits.wait_data_conn == QOSA_TRUE || conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].bits.trans_connecting == QOSA_TRUE)
        && (!qurl_conn_is_connected(conn_ptr, QURL_SOCKET_INDEX_1)))
    {
        /*Data*/
        if (ftp_ctrl_ptr->bits.wait_data_conn == QOSA_TRUE)
        {
            /*Active Mode*/
            if (conn_ptr->conn_conn_server.sock_fd == QURL_SOCKET_BAD)
            {
                return bitmap;
            }
            socks_ptr[0] = conn_ptr->conn_conn_server.sock_fd;
        }
        else
        {
            /*Passive Mode*/
            if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd == QURL_SOCKET_BAD)
            {
                return bitmap;
            }
            socks_ptr[0] = conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd;
            bitmap |= QURL_GETSOCK_WRITESOCK_FUNC(0);
        }

        bitmap |= QURL_GETSOCK_READSOCK_FUNC(0);
    }
    else if (ftp_ctrl_ptr->state == QURL_FTP_STATE_STOP)
    {
        /*Data*/
        if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd == QURL_SOCKET_BAD)
        {
            return bitmap;
        }
        socks_ptr[0] = conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd;

        bitmap |= QURL_GETSOCK_WRITESOCK_FUNC(0);
        bitmap |= QURL_GETSOCK_READSOCK_FUNC(0);
    }
    else
    {
        /*Control*/
        return qurl_pp_getsock(conn_ptr, &ftp_ctrl_ptr->pp_ctrl, socks_ptr);
    }

    return bitmap;
}

static int ftp_trans_data_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr)
{
    int               bitmap = 0; /*Corresponding to the read and write of the monitored sockfd*/
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    qurl_logd("enter\r\n");

    /*It is necessary to monitor whether the control connection is normal. According to RFC959, when the control connection is closed, the data connection becomes invalid.*/
    if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd != QURL_SOCKET_BAD)
    {
        socks_ptr[0] = conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd;
    }

    /*Listen separately for upload and download*/
    if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd != QURL_SOCKET_BAD)
    {
        socks_ptr[1] = conn_ptr->conn_conn[QURL_SOCKET_INDEX_1].sock_fd;

        if (core_ptr->cfg.bits.upload)
        {
            bitmap |= QURL_GETSOCK_WRITESOCK_FUNC(1);
        }
        else
        {
            bitmap |= QURL_GETSOCK_READSOCK_FUNC(1);
        }
    }

    return bitmap;
}

#endif
