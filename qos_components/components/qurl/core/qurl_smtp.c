/*
 * Portions of this file are derived from curl/libcurl's lib/smtp.c.
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

#include "qurl_config.h"

#if QURL_CFG_ENABLE_SMTP

#include "qurl_code_in.h"
#include "qurl_code.h"
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
#include "qurl_tls.h"

#include "qurl_sasl.h"
#include "qurl_smtp_def.h"

#define SMTP_UNUSED(x) ((void)x)

#define SMTP_FREE_PTR(p)                                                                                                                                       \
    {                                                                                                                                                          \
        if ((p) != QOSA_NULL)                                                                                                                                  \
        {                                                                                                                                                      \
            qurl_free(p);                                                                                                                                      \
            (p) = QOSA_NULL;                                                                                                                                   \
        }                                                                                                                                                      \
    }

/* Local API functions */
static qurl_ecode_t smtp_setup_connection(qurl_conn_t *conn_ptr);
static qurl_ecode_t smtp_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t smtp_multi_statemach(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t smtp_do(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t smtp_doing(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t smtp_trans_data(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr);
static qurl_ecode_t smtp_done(qurl_conn_t *conn_ptr, qurl_ecode_t ecode, qurl_bit_t premature);
static qurl_ecode_t smtp_disconnect(qurl_conn_t *conn_ptr, qurl_bit_t dead_connection);
static int          smtp_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);
static int          smtp_trans_data_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr);

static qurl_ecode_t smtp_perform_auth(void *conn, const char *mech, const char *initresp);
static qurl_ecode_t smtp_continue_auth(void *conn, const char *resp);
static void         smtp_get_message(char *buffer, char **outptr);
static qurl_ecode_t smtp_parse_address(qurl_conn_t *conn_ptr, const char *fqma, char **address, struct hostname *host);
static qurl_ecode_t smtp_parse_url_path(qurl_conn_t *conn_ptr);
static qurl_ecode_t smtp_parse_url_options(qurl_conn_t *conn_ptr);
static qurl_ecode_t smtp_parse_custom_request(qurl_conn_t *conn_ptr);
static qurl_ecode_t smtp_regular_transfer(qurl_conn_t *conn_ptr, qurl_bit_t *dophase_done);
//static qurl_ecode_t smtp_nonblock_statemach(qurl_conn_t *conn_ptr, qurl_bit_t disconnecting);

#if 1
/* Convert a mechanism name to a token */
extern unsigned int qurl_sasl_decode_mech(const char *ptr, int maxlen, int *len);

/* Parse the URL login options */
extern qurl_ecode_t qurl_sasl_parse_url_auth_option(qurl_sasl_t *sasl, const char *value, int len);

/* Initializes an SASL structure */
extern void qurl_sasl_init(qurl_sasl_t *sasl, const qurl_saslproto_t *params);

/* Check if we have enough auth data and capabilities to authenticate */
extern qurl_bit_t qurl_sasl_can_authenticate(qurl_sasl_t *sasl, void *conn);

/* Calculate the required login details for SASL authentication  */
extern qurl_ecode_t qurl_sasl_start(qurl_sasl_t *sasl, void *conn, qurl_bit_t force_ir, saslprogress *progress);

/* Continue an SASL authentication  */
extern qurl_ecode_t qurl_sasl_continue(qurl_sasl_t *sasl, void *conn, int code, saslprogress *progress);
#endif

/*
 * SMTP protocol handler.
 */

/*Function Declaration*/

const qurl_port_prot_t g_qurl_port_prot_smtp = {
    .scheme = "SMTP",
    .setup_connect_func = smtp_setup_connection,
    .send_prot_connect_func = smtp_connect,
    .prot_connect_func = smtp_multi_statemach,
    .req_func = smtp_do,
    .reqing_func = smtp_doing,
    .req_more_func = QOSA_NULL,
    .trans_data_func = smtp_trans_data,
    .trans_done_func = smtp_done,
    .disconnect_func = smtp_disconnect,
    .prot_getsock_func = smtp_getsock,
    .req_getsock_func = smtp_getsock,
    .req_more_getsock_func = QOSA_NULL,
    .trans_data_getsock_func = smtp_trans_data_getsock,  //smtp_trans_data_getsock,
    .conn_check_func = QOSA_NULL,
    .attach_func = QOSA_NULL,
    .def_port = QURL_CFG_PORT_SMTP_DEFAULT,
    .protocol = QURL_PROT_TYPE_SMTP,
    .family = QURL_PROT_TYPE_SMTP,
    .flags = QURL_PROT_FLAGS_CLOSEACTION | QURL_PROT_FLAGS_NOURLQUERY | QURL_PROT_FLAGS_URLOPTIONS,
};

#if QURL_CFG_ENABLE_TLS
const qurl_port_prot_t g_qurl_port_prot_smtps = {
    .scheme = "SMTPS",
    .setup_connect_func = smtp_setup_connection,
    .send_prot_connect_func = smtp_connect,
    .prot_connect_func = smtp_multi_statemach,
    .req_func = smtp_do,
    .reqing_func = smtp_doing,
    .req_more_func = QOSA_NULL,
    .trans_data_func = smtp_trans_data,
    .trans_done_func = smtp_done,
    .disconnect_func = smtp_disconnect,
    .prot_getsock_func = smtp_getsock,
    .req_getsock_func = smtp_getsock,
    .req_more_getsock_func = QOSA_NULL,
    .trans_data_getsock_func = smtp_trans_data_getsock,  //smtp_trans_data_getsock,
    .conn_check_func = QOSA_NULL,
    .attach_func = QOSA_NULL,
    .def_port = QURL_CFG_PORT_SMTPS_DEFAULT,
    .protocol = QURL_PROT_TYPE_SMTPS,
    .family = QURL_PROT_TYPE_SMTP,
    .flags = QURL_PROT_FLAGS_CLOSEACTION | QURL_PROT_FLAGS_NOURLQUERY | QURL_PROT_FLAGS_URLOPTIONS | QURL_PROT_FLAGS_SSL,
};
#endif

/* SASL parameters for the smtp protocol */
static const qurl_saslproto_t saslsmtp = {
    "smtp",             /* The service name */
    334,                /* Code received when continuation is expected */
    235,                /* Code to receive upon authentication success */
    512 - 8,            /* Maximum initial response length (no max) */
    smtp_perform_auth,  /* Send authentication command */
    smtp_continue_auth, /* Send authentication continuation */
    smtp_get_message    /* Get SASL response message */
};

void qurl_smtp_cfg_init(qurl_core_usr_cfg_smtp_t *cfg)
{
    cfg->mail_from_ptr = QOSA_NULL;
    cfg->mail_rtcp_ptr = QOSA_NULL;
    cfg->mail_auth_ptr = QOSA_NULL;
    cfg->mail_login_ptr = QOSA_NULL;
    cfg->mail_xoauth2_ptr = QOSA_NULL;
    cfg->sasl_authzid_ptr = QOSA_NULL;
    cfg->sasl_ir = 0;
    cfg->rtcp_slist = QOSA_NULL; /*!< Cleared by the upper-layer application*/
}

void qurl_smtp_cfg_delete(qurl_core_usr_cfg_smtp_t *cfg)
{
    SMTP_FREE_PTR(cfg->mail_from_ptr);
    SMTP_FREE_PTR(cfg->mail_rtcp_ptr);
    SMTP_FREE_PTR(cfg->mail_auth_ptr);
    SMTP_FREE_PTR(cfg->mail_login_ptr);
    SMTP_FREE_PTR(cfg->mail_xoauth2_ptr);
    SMTP_FREE_PTR(cfg->sasl_authzid_ptr);
    cfg->sasl_ir = 0;
}

qurl_bit_t qurl_is_ascii_name(const char *hostname)
{
    const unsigned char *ch = (const unsigned char *)hostname;

    if (!hostname)
    {
        return QOSA_TRUE;
    }

    while (*ch)
    {
        if (*ch++ & 0x80)
        {
            return QOSA_FALSE;
        }
    }
    return QOSA_TRUE;
}

#if QURL_CFG_ENABLE_TLS
static void smtp_to_smtps(qurl_conn_t *conn_ptr)
{
    /* Change the connection handler */

    conn_ptr->port_handler = &g_qurl_port_prot_smtps;

    /* Set the connection's upgraded to TLS flag */
    conn_ptr->bits.tls_connecting = QOSA_TRUE;
}
#endif /* QURL_CFG_ENABLE_TLS */

/***********************************************************************
 *
 * smtp_endofresp()
 *
 * Checks for an ending SMTP status code at the start of the given string, but
 * also detects various capabilities from the EHLO response including the
 * supported authentication mechanisms.
 */

static qurl_bit_t smtp_endofresp(qurl_conn_t *conn_ptr, char *buf, long buf_len, int *pong_code_ptr)
{
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    qurl_bit_t        result = QOSA_FALSE;

    qurl_logd("enter,buf_len=%d,pong_code_ptr=%d,buf=%s", buf_len, *pong_code_ptr, buf);
    /* Nothing for us */
    if (buf_len < 4 || !qurl_ctype_isdigit(buf[0]) || !qurl_ctype_isdigit(buf[1]) || !qurl_ctype_isdigit(buf[2]))
    {
        return QOSA_FALSE;
    }

    /* Do we have a command response? This should be the response code followed
     by a space and optionally some text as per RFC-5321 and as outlined in
     Section 4. Examples of RFC-4954 but some e-mail servers ignore this and
     only send the response code instead as per Section 4.2. */
    if (buf[3] == ' ' || buf_len == 5)
    {
        char tmpline[6];

        result = QOSA_TRUE;
        qurl_memset(tmpline, '\0', sizeof(tmpline));
        qurl_memcpy(tmpline, buf, (buf_len == 5 ? 5 : 3));
        *pong_code_ptr = qurl_strtol(tmpline, QOSA_NULL, 10);

        /* Make sure real server never sends internal value */
        if (*pong_code_ptr == 1)
        {
            *pong_code_ptr = 0;
        }
    }
    /* Do we have a multiline (continuation) response? */
    else if (buf[3] == '-' && (smtpc->state == SMTP_EHLO || smtpc->state == SMTP_COMMAND))
    {
        result = QOSA_TRUE;
        *pong_code_ptr = 1; /* Internal response code */
    }

    qurl_logd("end,result=%d", result);
    return result;
}

/***********************************************************************
 *
 * smtp_get_message()
 *
 * Gets the authentication message from the response buffer.
 */
static void smtp_get_message(char *buffer, char **outptr)
{
    int   len = qurl_strlen(buffer);
    char *message = QOSA_NULL;

    qurl_logd("enter");
    if (len > 4)
    {
        /* Find the start of the message */
        len -= 4;
        for (message = buffer + 4; *message == ' ' || *message == '\t'; message++, len--)
            ;

        /* Find the end of the message */
        for (; len--;)
        {
            if (message[len] != '\r' && message[len] != '\n' && message[len] != ' ' && message[len] != '\t')
            {
                break;
            }
        }

        /* Terminate the message */
        if (++len)
        {
            message[len] = '\0';
        }
    }
    else
    {
        /* junk input => zero length output */
        message = &buffer[len];
    }

    *outptr = message;
}

/***********************************************************************
 *
 * state()
 *
 * This is the ONLY way to change SMTP state!
 */
static void state(qurl_conn_t *conn_ptr, smtpstate newstate)
{
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    /* for debug purposes */
    static const char *const names[] = {
        "STOP",
        "SERVERGREET",
        "EHLO",
        "HELO",
        "STARTTLS",
        "UPGRADETLS",
        "AUTH",
        "COMMAND",
        "MAIL",
        "RCPT",
        "DATA",
        "POSTDATA",
        "QUIT",
        /* LAST */
    };

    if (smtpc->state != newstate)
    {
        QURL_UNUSED(names);
        qurl_logd("SMTP %p state change from %s to %s\n", (void *)smtpc, names[smtpc->state], names[newstate]);
    }

    smtpc->state = newstate;
}

/***********************************************************************
 *
 * smtp_perform_ehlo()
 *
 * Sends the EHLO command to not only initialise communication with the ESMTP
 * server but to also obtain a list of server side supported capabilities.
 */
static qurl_ecode_t smtp_perform_ehlo(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      result = QURL_OK;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;

    smtpc->sasl.authmechs = SASL_AUTH_NONE; /* No known auth. mechanism yet */
    smtpc->sasl.authused = SASL_AUTH_NONE;  /* Clear the authentication mechanism used for esmtp connections */
    smtpc->tls_supported = QOSA_FALSE;      /* Clear the TLS capability */
    smtpc->auth_supported = QOSA_FALSE;     /* Clear the AUTH capability */

    qurl_logd("enter");
    /* Send the EHLO command */
    result = qurl_pp_sendf(conn_ptr, &smtpc->pp, "EHLO %s", smtpc->domain);

    if (!result)
    {
        state(conn_ptr, SMTP_EHLO);
    }

    return result;
}

/***********************************************************************
 *
 * smtp_perform_helo()
 *
 * Sends the HELO command to initialise communication with the SMTP server.
 */
static qurl_ecode_t smtp_perform_helo(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      result = QURL_OK;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;

    qurl_logd("enter");
    smtpc->sasl.authused = SASL_AUTH_NONE; /* No authentication mechanism used in smtp connections */

    /* Send the HELO command */
    result = qurl_pp_sendf(conn_ptr, &smtpc->pp, "HELO %s", smtpc->domain);

    if (!result)
    {
        state(conn_ptr, SMTP_HELO);
    }

    return result;
}

/***********************************************************************
 *
 * smtp_perform_starttls()
 *
 * Sends the STLS command to start the upgrade to TLS.
 */
static qurl_ecode_t smtp_perform_starttls(qurl_conn_t *conn_ptr)
{
    /* Send the STARTTLS command */
    qurl_logd("enter");

    qurl_ecode_t result = qurl_pp_sendf(conn_ptr, &conn_ptr->prot_data_u.smtp_ctrl.pp, "%s", "STARTTLS");

    if (!result)
    {
        state(conn_ptr, SMTP_STARTTLS);
    }

    return result;
}

#if QURL_CFG_ENABLE_TLS
/***********************************************************************
 *
 * smtp_perform_upgrade_tls()
 *
 * Performs the upgrade to TLS.
 */
static qurl_ecode_t smtp_perform_upgrade_tls(qurl_conn_t *conn_ptr)
{
    /* Start the SSL connection */
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    qurl_ecode_t      result = QURL_OK;
    qurl_bit_t        tls_done_flag = QOSA_FALSE;
    qurl_tls_cfg_t   *tls_cfg_ptr = QOSA_NULL;
    int               cfg_alloc_flag = QOSA_FALSE;

    qurl_logd("enter");

    /*Create a TLS connection*/
    if (conn_ptr->core_ptr->usr_cfg.tls_cfg_ptr != QOSA_NULL)
    {
        tls_cfg_ptr = conn_ptr->core_ptr->usr_cfg.tls_cfg_ptr;
    }
    else
    {
        tls_cfg_ptr = qurl_malloc(sizeof(qurl_tls_cfg_t));
        if (tls_cfg_ptr == QOSA_NULL)
        {
            result = QURL_ECODE_NO_MEMORY;
            return result;
        }
        qurl_tls_cfg_init(tls_cfg_ptr);
        cfg_alloc_flag = QOSA_TRUE;
    }

    conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].bits.is_tls = 1;
    conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].tls_ctrl_ptr = qurl_tls_new(tls_cfg_ptr);
    if (cfg_alloc_flag)
    {
        qurl_free(tls_cfg_ptr);
    }

    result = qurl_conn_connect(conn_ptr, QURL_SOCKET_INDEX_0, &tls_done_flag);
    smtpc->ssldone = tls_done_flag;
    qurl_logd("result=%x,%d,%d", result, smtpc->state, smtpc->ssldone);
    if (!result)
    {
        if (smtpc->state != SMTP_UPGRADETLS)
        {
            state(conn_ptr, SMTP_UPGRADETLS);
        }

        if (smtpc->ssldone)
        {
            smtp_to_smtps(conn_ptr);
            result = smtp_perform_ehlo(conn_ptr);
        }
    }

    return result;
}
#endif /* QURL_CFG_ENABLE_TLS */

/***********************************************************************
 *
 * smtp_perform_auth()
 *
 * Sends an AUTH command allowing the client to login with the given SASL
 * authentication mechanism.
 */
static qurl_ecode_t smtp_perform_auth(void *conn, const char *mech, const char *initresp)
{
    qurl_ecode_t      result = QURL_OK;
    qurl_conn_t      *conn_ptr = (qurl_conn_t *)conn;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;

    qurl_logd("enter");
    if (initresp)
    { /* AUTH <mech> ...<crlf> */
        /* Send the AUTH command with the initial response */
        result = qurl_pp_sendf(conn_ptr, &smtpc->pp, "AUTH %s %s", mech, initresp);
    }
    else
    {
        /* Send the AUTH command */
        result = qurl_pp_sendf(conn_ptr, &smtpc->pp, "AUTH %s", mech);
    }

    return result;
}

/***********************************************************************
 *
 * smtp_continue_auth()
 *
 * Sends SASL continuation data or cancellation.
 */
static qurl_ecode_t smtp_continue_auth(void *conn, const char *resp)
{
    qurl_conn_t      *conn_ptr = (qurl_conn_t *)conn;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;

    qurl_logd("enter");
    return qurl_pp_sendf(conn_ptr, &smtpc->pp, "%s", resp);
}

/***********************************************************************
 *
 * smtp_perform_authentication()
 *
 * Initiates the authentication sequence, with the appropriate SASL
 * authentication mechanism.
 */
static qurl_ecode_t smtp_perform_authentication(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      result = QURL_OK;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    saslprogress      progress;

    /* Check we have enough data to authenticate with, and the
     server supports authentiation, and end the connect phase if not */
    conn_ptr->bits.user_passwd = (conn_ptr->core_ptr->usr_cfg.username_ptr) ? QOSA_TRUE : QOSA_FALSE;
    qurl_logd("enter=%d,%d,%s", smtpc->auth_supported, conn_ptr->bits.user_passwd, conn_ptr->core_ptr->usr_cfg.username_ptr);
    if (!smtpc->auth_supported || !qurl_sasl_can_authenticate(&smtpc->sasl, conn_ptr))
    {
        state(conn_ptr, SMTP_STOP);
        return result;
    }

    /* Calculate the SASL login details */
    result = qurl_sasl_start(&smtpc->sasl, conn_ptr, QOSA_FALSE, &progress);

    if (!result)
    {
        if (progress == SASL_INPROGRESS)
        {
            state(conn_ptr, SMTP_AUTH);
        }
        else
        {
            /* Other mechanisms not supported */
            qurl_loge("No known authentication mechanisms supported!\n");
            result = QURL_ECODE_SMTP_AUTH_FAILED;
        }
    }

    return result;
}

/***********************************************************************
 *
 * smtp_perform_command()
 *
 * Sends a SMTP based command.
 */
static qurl_ecode_t smtp_perform_command(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t result = QURL_OK;
    struct SMTP *smtp = conn_ptr->core_ptr->prot_data;

    qurl_logd("enter");
    if (smtp->rcpt)
    {
        /* We notify the server we are sending UTF-8 data if a) it supports the
        SMTPUTF8 extension and b) The mailbox contains UTF-8 charaacters, in
        either the local address or host name parts. This is regardless of
        whether the host name is encoded using IDN ACE */
        qurl_bit_t utf8 = QOSA_FALSE;

        if ((!smtp->custom) || (!smtp->custom[0]))
        {
            char           *address = QOSA_NULL;
            struct hostname host = {QOSA_NULL, QOSA_NULL, QOSA_NULL, QOSA_NULL};

            /* Parse the mailbox to verify into the local address and host name parts, converting the host name to an IDN A-label if necessary */
            result = smtp_parse_address(conn_ptr, smtp->rcpt->data, &address, &host);
            if (result)
            {
                return result;
            }

            /* Establish whether we should report SMTPUTF8 to the server for this mailbox as per RFC-6531 sect. 3.1 point 6 */
            utf8 = (conn_ptr->prot_data_u.smtp_ctrl.utf8_supported) && ((host.encalloc) || (!qurl_is_ascii_name(address)) || (!qurl_is_ascii_name(host.name)));

            /* Send the VRFY command (Note: The host name part may be absent when the host is a local system) */
            result = qurl_pp_sendf(
                conn_ptr,
                &conn_ptr->prot_data_u.smtp_ctrl.pp,
                "VRFY %s%s%s%s",
                address,
                host.name ? "@" : "",
                host.name ? host.name : "",
                utf8 ? " SMTPUTF8" : ""
            );

            qurl_free(address);
        }
        else
        {
            /* Establish whether we should report that we support SMTPUTF8 for EXPN commands to the server as per RFC-6531 sect. 3.1 point 6 */
            utf8 = (conn_ptr->prot_data_u.smtp_ctrl.utf8_supported) && (!qurl_strcmp(smtp->custom, "EXPN"));

            /* Send the custom recipient based command such as the EXPN command */
            result = qurl_pp_sendf(conn_ptr, &conn_ptr->prot_data_u.smtp_ctrl.pp, "%s %s%s", smtp->custom, smtp->rcpt->data, utf8 ? " SMTPUTF8" : "");
        }
    }
    else
    {
        /* Send the non-recipient based command such as HELP */
        result = qurl_pp_sendf(conn_ptr, &conn_ptr->prot_data_u.smtp_ctrl.pp, "%s", smtp->custom && smtp->custom[0] != '\0' ? smtp->custom : "HELP");
    }

    if (!result)
    {
        state(conn_ptr, SMTP_COMMAND);
    }

    return result;
}

/***********************************************************************
 *
 * smtp_perform_mail()
 *
 * Sends an MAIL command to initiate the upload of a message.
 */
static qurl_ecode_t smtp_perform_mail(qurl_conn_t *conn_ptr)
{
    char        *from = QOSA_NULL;
    char        *auth = QOSA_NULL;
    char        *size = QOSA_NULL;
    qurl_ecode_t result = QURL_OK;
    qurl_bit_t   utf8 = QOSA_FALSE;

    /* We notify the server we are sending UTF-8 data if a) it supports the
     SMTPUTF8 extension and b) The mailbox contains UTF-8 charaacters, in
     either the local address or host name parts. This is regardless of
     whether the host name is encoded using IDN ACE */

    qurl_logd("enter");
    /* Calculate the FROM parameter */
    if (conn_ptr->core_ptr->usr_cfg.smtp.mail_from_ptr)
    {
        char           *address = QOSA_NULL;
        struct hostname host = {QOSA_NULL, QOSA_NULL, QOSA_NULL, QOSA_NULL};

        /* Parse the FROM mailbox into the local address and host name parts,
        converting the host name to an IDN A-label if necessary */
        result = smtp_parse_address(conn_ptr, conn_ptr->core_ptr->usr_cfg.smtp.mail_from_ptr, &address, &host);
        if (result)
        {
            return result;
        }

        /* Establish whether we should report SMTPUTF8 to the server for this
        mailbox as per RFC-6531 sect. 3.1 point 4 and sect. 3.4 */
        utf8 = (conn_ptr->prot_data_u.smtp_ctrl.utf8_supported) && ((host.encalloc) || (!qurl_is_ascii_name(address)) || (!qurl_is_ascii_name(host.name)));

        if (host.name)
        {
            qurl_asprintf(&from, "<%s@%s>", address, host.name);
        }
        else
        {
            /* An invalid mailbox was provided but we'll simply let the server worry about that and reply with a 501 error */
            qurl_asprintf(&from, "<%s>", address);
        }

        qurl_free(address);
    }
    else
    {
        /* Null reverse-path, RFC-5321, sect. 3.6.3 */
        from = qurl_strdup("<>");
    }

    if (!from)
    {
        return QURL_ECODE_NO_MEMORY;
    }

    /* Calculate the optional AUTH parameter */
    if (conn_ptr->core_ptr->usr_cfg.smtp.mail_auth_ptr && conn_ptr->prot_data_u.smtp_ctrl.sasl.authused)
    {
        if (conn_ptr->core_ptr->usr_cfg.smtp.mail_auth_ptr[0] != '\0')
        {
            char           *address = QOSA_NULL;
            struct hostname host = {QOSA_NULL, QOSA_NULL, QOSA_NULL, QOSA_NULL};

            /* Parse the AUTH mailbox into the local address and host name parts,
            converting the host name to an IDN A-label if necessary */
            result = smtp_parse_address(conn_ptr, conn_ptr->core_ptr->usr_cfg.smtp.mail_auth_ptr, &address, &host);
            if (result)
            {
                qurl_free(from);
                return result;
            }

            /* Establish whether we should report SMTPUTF8 to the server for this mailbox as per RFC-6531 sect. 3.1 point 4 and sect. 3.4 */
            if ((!utf8) && (conn_ptr->prot_data_u.smtp_ctrl.utf8_supported)
                && ((host.encalloc) || (!qurl_is_ascii_name(address)) || (!qurl_is_ascii_name(host.name))))
            {
                utf8 = QOSA_TRUE;
            }

            if (host.name)
            {
                qurl_asprintf(&auth, "<%s@%s>", address, host.name);
            }
            else
            {
                /* An invalid mailbox was provided but we'll simply let the server worry about it */
                qurl_asprintf(&auth, "<%s>", address);
            }

            qurl_free(address);
        }
        else
        {
            /* Empty AUTH, RFC-2554, sect. 5 */
            auth = qurl_strdup("<>");
        }

        if (!auth)
        {
            qurl_free(from);
            return QURL_ECODE_NO_MEMORY;
        }
    }

#if 0
	/*Not supported yet*/
    /* Prepare the mime data if some. */
    if (data->set.mimepost.kind != MIMEKIND_NONE)
    {
        /* Use the whole structure as data. */
        data->set.mimepost.flags &= ~MIME_BODY_ONLY;

        /* Add external headers and mime version. */
        curl_mime_headers(&data->set.mimepost, data->set.headers, 0);
        result = Curl_mime_prepare_headers(&data->set.mimepost, QOSA_NULL, QOSA_NULL, MIMESTRATEGY_MAIL);

        if (!result)
            if (!Curl_checkheaders(conn_ptr, "Mime-Version"))
                result = Curl_mime_add_header(&data->set.mimepost.curlheaders, "Mime-Version: 1.0");

        /* Make sure we will read the entire mime structure. */
        if (!result)
            result = Curl_mime_rewind(&data->set.mimepost);

        if (result)
        {
            qurl_free(from);
            qurl_free(auth);

            return result;
        }

        data->state.infilesize = Curl_mime_size(&data->set.mimepost);

        /* Read from mime structure. */
        data->state.fread_func = (curl_read_callback)Curl_mime_read;
        data->state.in = (void *)&data->set.mimepost;
    }
#endif

#if 0
    /* Calculate the optional SIZE parameter */
    if (conn_ptr->prot_data_u.smtp_ctrl.size_supported && data->state.infilesize > 0)
    {
        size = qurl_asprintf("%" CURL_FORMAT_CURL_OFF_T, data->state.infilesize);

        if (!size)
        {
            qurl_free(from);
            qurl_free(auth);

            return QURL_ECODE_NO_MEMORY;
        }
    }
#endif

    /* If the mailboxes in the FROM and AUTH parameters don't include a UTF-8
     based address then quickly scan through the recipient list and check if
     any there do, as we need to correctly identify our support for SMTPUTF8
     in the envelope, as per RFC-6531 sect. 3.4 */
    if (conn_ptr->prot_data_u.smtp_ctrl.utf8_supported && !utf8)
    {
        struct SMTP       *smtp = conn_ptr->core_ptr->prot_data;
        qurl_slist_ctrl_t *rcpt = smtp->rcpt;

        while (rcpt && !utf8)
        {
            /* Does the host name contain non-ASCII characters? */
            if (!qurl_is_ascii_name(rcpt->data))
            {
                utf8 = QOSA_TRUE;
            }

            rcpt = rcpt->next;
        }
    }

    /* Send the MAIL command */
    result = qurl_pp_sendf(
        conn_ptr,
        &conn_ptr->prot_data_u.smtp_ctrl.pp,
        "MAIL FROM:%s%s%s%s%s%s",
        from,                 /* Mandatory                 */
        auth ? " AUTH=" : "", /* Optional on AUTH support  */
        auth ? auth : "",     /*                           */
        size ? " SIZE=" : "", /* Optional on SIZE support  */
        size ? size : "",     /*                           */
        utf8 ? " SMTPUTF8"    /* Internationalised mailbox */
             : ""
    );                        /* included in our envelope  */

    qurl_free(from);
    qurl_free(auth);
    qurl_free(size);

    if (!result)
    {
        state(conn_ptr, SMTP_MAIL);
    }

    return result;
}

/***********************************************************************
 *
 * smtp_perform_rcpt_to()
 *
 * Sends a RCPT TO command for a given recipient as part of the message upload
 * process.
 */
static qurl_ecode_t smtp_perform_rcpt_to(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t    result = QURL_OK;
    struct SMTP    *smtp = conn_ptr->core_ptr->prot_data;
    char           *address = QOSA_NULL;
    struct hostname host = {QOSA_NULL, QOSA_NULL, QOSA_NULL, QOSA_NULL};

    qurl_logd("enter");
    /* Parse the recipient mailbox into the local address and host name parts,
     converting the host name to an IDN A-label if necessary */
    result = smtp_parse_address(conn_ptr, smtp->rcpt->data, &address, &host);
    if (result)
    {
        return result;
    }

    /* Send the RCPT TO command */
    if (host.name)
    {
        result = qurl_pp_sendf(conn_ptr, &conn_ptr->prot_data_u.smtp_ctrl.pp, "RCPT TO:<%s@%s>", address, host.name);
    }

    else
    {
        /* An invalid mailbox was provided but we'll simply let the server worry
        about that and reply with a 501 error */
        result = qurl_pp_sendf(conn_ptr, &conn_ptr->prot_data_u.smtp_ctrl.pp, "RCPT TO:<%s>", address);
    }

    qurl_free(address);

    if (!result)
    {
        state(conn_ptr, SMTP_RCPT);
    }

    return result;
}

/***********************************************************************
 *
 * smtp_perform_quit()
 *
 * Performs the quit action prior to sclose() being called.
 */
static qurl_ecode_t smtp_perform_quit(qurl_conn_t *conn_ptr)
{
    /* Send the QUIT command */
    qurl_logd("enter");

    qurl_ecode_t result = qurl_pp_sendf(conn_ptr, &conn_ptr->prot_data_u.smtp_ctrl.pp, "%s", "QUIT");

    if (!result)
    {
        state(conn_ptr, SMTP_QUIT);
    }

    return result;
}

/* For the initial server greeting */
static qurl_ecode_t smtp_state_servergreet_resp(qurl_conn_t *conn_ptr, int smtpcode, smtpstate instate)
{
    qurl_ecode_t result = QURL_OK;

    (void)instate; /* no use for this yet */

    qurl_logd("enter");
    if (smtpcode / 100 != 2)
    {
        qurl_logd("Got unexpected smtp-server response: %d", smtpcode);
        result = QURL_ECODE_SMTP_WEIRD_SERVER_REPLY;
    }
    else
    {
        result = smtp_perform_ehlo(conn_ptr);
    }

    return result;
}

/* For STARTTLS responses */
static qurl_ecode_t smtp_state_starttls_resp(qurl_conn_t *conn_ptr, int smtpcode, smtpstate instate)
{
    qurl_ecode_t result = QURL_OK;

    (void)instate; /* no use for this yet */

    qurl_logd("enter=%d", smtpcode);
    if (smtpcode != 220)
    {
        if (conn_ptr->core_ptr->usr_cfg.use_tls != QURL_USETLS_TRY)
        {
            qurl_logd("STARTTLS denied, code %d", smtpcode);
            result = QURL_ECODE_TLS_CONNECT_ERR;
        }
        else
        {
            result = smtp_perform_authentication(conn_ptr);
        }
    }
    else
    {
#if QURL_CFG_ENABLE_TLS
        result = smtp_perform_upgrade_tls(conn_ptr);
#else  /* QURL_CFG_ENABLE_TLS */
        result = QURL_ECODE_TLS_INIT_FAILED;
#endif /* QURL_CFG_ENABLE_TLS */
    }

    return result;
}

/* For EHLO responses */
static qurl_ecode_t smtp_state_ehlo_resp(qurl_conn_t *conn_ptr, int smtpcode, smtpstate instate, int len)
{
    qurl_ecode_t      result = QURL_OK;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    const char       *line = (char *)conn_ptr->deal_cache_ptr;

    qurl_logd("line=%s", line);
    qurl_logd("len=%d,%d", len, smtpcode);
    (void)instate; /* no use for this yet */

    if (smtpcode / 100 != 2 && smtpcode != 1)
    {
        /*Needs to be supplemented*/
        if (conn_ptr->core_ptr->usr_cfg.use_tls <= QURL_USETLS_TRY)
        {
            result = smtp_perform_helo(conn_ptr);
        }
        else
        {
            qurl_loge("Remote access denied: %d", smtpcode);
            result = QURL_ECODE_SMTP_REMOTE_ACCESS_DENIED;
        }
    }
    else if (len >= 4)
    {
        line += 4;
        len -= 4;

        /* Does the server support the STARTTLS capability? */
        if (len >= 8 && !qurl_memcmp(line, "STARTTLS", 8))
        {
            smtpc->tls_supported = QOSA_TRUE;
        }
        /* Does the server support the SIZE capability? */
        else if (len >= 4 && !qurl_memcmp(line, "SIZE", 4))
        {
            smtpc->size_supported = QOSA_TRUE;
        }
        /* Does the server support the UTF-8 capability? */
        else if (len >= 8 && !qurl_memcmp(line, "SMTPUTF8", 8))
        {
            smtpc->utf8_supported = QOSA_TRUE;
        }
        /* Does the server support authentication? */
        else if (len >= 5 && !qurl_memcmp(line, "AUTH ", 5))
        {
            smtpc->auth_supported = QOSA_TRUE;

            /* Advance past the AUTH keyword */
            line += 5;
            len -= 5;

            /* Loop through the data line */
            for (;;)
            {
                int          llen;
                int          wordlen;
                unsigned int mechbit;

                while (len && (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n'))
                {
                    line++;
                    len--;
                }

                if (!len)
                {
                    break;
                }

                /* Extract the word */
                for (wordlen = 0; wordlen < len && line[wordlen] != ' ' && line[wordlen] != '\t' && line[wordlen] != '\r' && line[wordlen] != '\n';)
                {
                    wordlen++;
                }
                /* Test the word for a matching authentication mechanism */
                mechbit = qurl_sasl_decode_mech(line, wordlen, &llen);
                if (mechbit && llen == wordlen)
                {
                    smtpc->sasl.authmechs |= mechbit;
                }

                line += wordlen;
                len -= wordlen;
            }
        }

        qurl_logd("authmechs=%x", smtpc->sasl.authmechs);
        if (smtpcode != 1)
        {
            if (conn_ptr->core_ptr->usr_cfg.use_tls && !smtpc->ssldone)
            {
                /* We don't have a SSL/TLS connection yet, but SSL is requested */
                if (smtpc->tls_supported)
                {
                    /* Switch to TLS connection now */
                    result = smtp_perform_starttls(conn_ptr);
                }
                else if (conn_ptr->core_ptr->usr_cfg.use_tls == QURL_USETLS_TRY)
                {
                    /* Fallback and carry on with authentication */
                    result = smtp_perform_authentication(conn_ptr);
                }
                else
                {
                    qurl_loge("STARTTLS not supported.");
                    result = QURL_ECODE_TLS_CONNECT_ERR;
                }
            }
            else
            {
                result = smtp_perform_authentication(conn_ptr);
            }
        }
    }
    else
    {
        qurl_loge("Unexpectedly short EHLO response");
        result = QURL_ECODE_SMTP_WEIRD_SERVER_REPLY;
    }

    return result;
}

/* For HELO responses */
static qurl_ecode_t smtp_state_helo_resp(qurl_conn_t *conn_ptr, int smtpcode, smtpstate instate)
{
    qurl_ecode_t result = QURL_OK;

    (void)instate; /* no use for this yet */

    qurl_logd("enter");
    if (smtpcode / 100 != 2)
    {
        qurl_loge("Remote access denied: %d", smtpcode);
        result = QURL_ECODE_SMTP_REMOTE_ACCESS_DENIED;
    }
    else
    {
        /* End of connect phase */
        state(conn_ptr, SMTP_STOP);
    }

    return result;
}

/* For SASL authentication responses */
static qurl_ecode_t smtp_state_auth_resp(qurl_conn_t *conn_ptr, int smtpcode, smtpstate instate)
{
    qurl_ecode_t      result = QURL_OK;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    saslprogress      progress;

    (void)instate; /* no use for this yet */

    qurl_logd("enter smtpcode=%d", smtpcode);
    result = qurl_sasl_continue(&smtpc->sasl, conn_ptr, smtpcode, &progress);
    if (!result)
    {
        switch (progress)
        {
            case SASL_DONE:
                state(conn_ptr, SMTP_STOP); /* Authenticated */
                break;
            case SASL_IDLE:                 /* No mechanism left after cancellation */
                qurl_loge("Authentication cancelled");
                result = QURL_ECODE_SMTP_LOGIN_DENIED;
                break;
            default:
                break;
        }
    }

    return result;
}

/* For command responses */
static qurl_ecode_t smtp_state_command_resp(qurl_conn_t *conn_ptr, int smtpcode, smtpstate instate, int len)
{
    qurl_ecode_t result = QURL_OK;
    struct SMTP *smtp = conn_ptr->core_ptr->prot_data;
    // char        *line = QOSA_NULL;

    (void)instate; /* no use for this yet */
    (void)len;

    qurl_logd("enter");
    if ((smtp->rcpt && smtpcode / 100 != 2 && smtpcode != 553 && smtpcode != 1) || (!smtp->rcpt && smtpcode / 100 != 2 && smtpcode != 1))
    {
        qurl_loge("Command failed: %d", smtpcode);
        result = QURL_ECODE_SMTP_RECV_ERROR;
    }
    else
    {
        /* Temporarily add the LF character back and send as body to the client */
        //Todo:
        // if (!conn_ptr->core_ptr->usr_cfg.bits.opt_nobody)
        // {
        //     line[len] = '\n';
        //     line[len] = '\0';
        // }

        if (smtpcode != 1)
        {
            if (smtp->rcpt)
            {
                smtp->rcpt = smtp->rcpt->next;

                if (smtp->rcpt)
                {
                    /* Send the next command */
                    result = smtp_perform_command(conn_ptr);
                }
                else
                {
                    /* End of DO phase */
                    state(conn_ptr, SMTP_STOP);
                }
            }
            else
            {
                /* End of DO phase */
                state(conn_ptr, SMTP_STOP);
            }
        }
    }

    return result;
}

/* For MAIL responses */
static qurl_ecode_t smtp_state_mail_resp(qurl_conn_t *conn_ptr, int smtpcode, smtpstate instate)
{
    qurl_ecode_t result = QURL_OK;

    (void)instate; /* no use for this yet */

    qurl_logd("enter");
    if (smtpcode / 100 != 2)
    {
        qurl_loge("MAIL failed: %d", smtpcode);
        result = QURL_ECODE_SMTP_SEND_ERROR;
    }
    else
    {
        /* Start the RCPT TO command */
        result = smtp_perform_rcpt_to(conn_ptr);
    }

    return result;
}

/* For RCPT responses */
static qurl_ecode_t smtp_state_rcpt_resp(qurl_conn_t *conn_ptr, int smtpcode, smtpstate instate)
{
    qurl_ecode_t result = QURL_OK;
    struct SMTP *smtp = conn_ptr->core_ptr->prot_data;
    qurl_bit_t   is_smtp_err = QOSA_FALSE;
    qurl_bit_t   is_smtp_blocking_err = QOSA_FALSE;

    (void)instate; /* no use for this yet */

    qurl_logd("enter");
    is_smtp_err = (smtpcode / 100 != 2) ? QOSA_TRUE : QOSA_FALSE;

    /* If there's multiple RCPT TO to be issued, it's possible to ignore errors
     and proceed with only the valid addresses. */
    is_smtp_blocking_err = (is_smtp_err && !conn_ptr->core_ptr->usr_cfg.bits.mail_rcpt_allowfails) ? QOSA_TRUE : QOSA_FALSE;

    if (is_smtp_err)
    {
        /* Remembering the last failure which we can report if all "RCPT TO" have
       failed and we cannot proceed. */
        smtp->rcpt_last_error = smtpcode;

        if (is_smtp_blocking_err)
        {
            qurl_loge("RCPT failed: %d", smtpcode);
            result = QURL_ECODE_SMTP_SEND_ERROR;
        }
    }
    else
    {
        /* Some RCPT TO commands have succeeded. */
        smtp->rcpt_had_ok = QOSA_TRUE;
    }

    if (!is_smtp_blocking_err)
    {
        smtp->rcpt = smtp->rcpt->next;

        if (smtp->rcpt)
        {
            /* Send the next RCPT TO command */
            result = smtp_perform_rcpt_to(conn_ptr);
        }
        else
        {
            /* We weren't able to issue a successful RCPT TO command while going
         	over recipients (potentially multiple). Sending back last error. */
            if (!smtp->rcpt_had_ok)
            {
                qurl_loge("RCPT failed: %d (last error)", smtp->rcpt_last_error);
                result = QURL_ECODE_SMTP_SEND_ERROR;
            }
            else
            {
                /* Send the DATA command */
                result = qurl_pp_sendf(conn_ptr, &conn_ptr->prot_data_u.smtp_ctrl.pp, "%s", "DATA");

                if (!result)
                {
                    //state(conn_ptr, SMTP_DATA);
                    //Harry test modification, originally should be SMTP_DATA
                    state(conn_ptr, SMTP_STOP);
                }
            }
        }
    }

    return result;
}

/* For DATA response */
static qurl_ecode_t smtp_state_data_resp(qurl_conn_t *conn_ptr, int smtpcode, smtpstate instate)
{
    qurl_ecode_t result = QURL_OK;

    (void)instate; /* no use for this yet */

    qurl_logd("enter");
    if (smtpcode != 354)
    {
        qurl_loge("DATA failed: %d", smtpcode);
        result = QURL_ECODE_SMTP_SEND_ERROR;
    }
    else
    {
        /* Set the progress upload size */
        //Curl_pgrsSetUploadSize(data, data->state.infilesize);
        /* End of DONE phase */
        //conn_ptr->core_ptr->trans.upload_len_max = conn_ptr->core_ptr->cfg.upload_size;
        conn_ptr->core_ptr->trans.uploaded_len = 0;
        state(conn_ptr, SMTP_STOP);
    }

    return result;
}

/* For POSTDATA responses, which are received after the entire DATA
   part has been sent to the server */
static qurl_ecode_t smtp_state_postdata_resp(qurl_conn_t *conn_ptr, int smtpcode, smtpstate instate)
{
    qurl_ecode_t result = QURL_OK;

    (void)instate; /* no use for this yet */

    if (smtpcode != 250)
    {
        result = QURL_ECODE_SMTP_RECV_ERROR;
    }

    /* End of DONE phase */
    state(conn_ptr, SMTP_STOP);

    return result;
}

//smtp_pp_statemachine
static qurl_ecode_t smtp_statemach_act(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      result = QURL_OK;
    int               smtpcode = 0;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    qurl_pp_ctrl_t   *pp = &smtpc->pp;
    long              len = 0;

    qurl_logd("enter=%d", smtpc->state);
    /* Busy upgrading the connection; right now all I/O is SSL/TLS, not SMTP */
    if (smtpc->state == SMTP_UPGRADETLS)
    {
#if QURL_CFG_ENABLE_TLS
        return smtp_perform_upgrade_tls(conn_ptr);
#else  /* QURL_CFG_ENABLE_TLS */
        return QURL_ECODE_TLS_INIT_FAILED;
#endif /* QURL_CFG_ENABLE_TLS */
    }

    do {
        /*There may be scenarios where it remains resident, so monitoring points can be placed.*/
        QURL_CORE_CHECK_ABORT_FUNC(conn_ptr->core_ptr);

        /* Flush any data that needs to be sent */
        /*Refresh the ping buffer*/
        result = qurl_pp_flushsend(conn_ptr, pp);
        if (result != QURL_OK)
        {
            return result;
        }

        /* Read the response from the server */
        result = qurl_pp_read_pong(conn_ptr, pp, &smtpcode, (long *)&len);
        qurl_logd("result=%d,smtpcode=%d", result, smtpcode);
        if (result)
        {
            if (smtpc->state == SMTP_QUIT)
            {
                state(conn_ptr, SMTP_STOP);  //Modify state, smtp_block_statemach infinite loop
            }
            return result;
        }
        /* Store the latest response for later retrieval if necessary */
        if (smtpc->state != SMTP_QUIT && smtpcode != 1)
        {
            conn_ptr->core_ptr->info.resp_code = smtpcode;
        }

        if (!smtpcode)
        {
            break;
        }

        qurl_logd("smtp state=%d", smtpc->state);
        /* We have now received a full SMTP server response */
        switch (smtpc->state)
        {
            case SMTP_SERVERGREET:
                result = smtp_state_servergreet_resp(conn_ptr, smtpcode, smtpc->state);
                break;

            case SMTP_EHLO:
                qurl_logd("smtp state=%d", smtpc->state);
                result = smtp_state_ehlo_resp(conn_ptr, smtpcode, smtpc->state, len);
                break;

            case SMTP_HELO:
                result = smtp_state_helo_resp(conn_ptr, smtpcode, smtpc->state);
                break;

            case SMTP_STARTTLS:
                result = smtp_state_starttls_resp(conn_ptr, smtpcode, smtpc->state);
                break;

            case SMTP_AUTH:
                result = smtp_state_auth_resp(conn_ptr, smtpcode, smtpc->state);
                break;

            case SMTP_COMMAND:
                result = smtp_state_command_resp(conn_ptr, smtpcode, smtpc->state, len);
                break;

            case SMTP_MAIL:
                result = smtp_state_mail_resp(conn_ptr, smtpcode, smtpc->state);
                break;

            case SMTP_RCPT:
                result = smtp_state_rcpt_resp(conn_ptr, smtpcode, smtpc->state);
                break;

            case SMTP_DATA:
                result = smtp_state_data_resp(conn_ptr, smtpcode, smtpc->state);
                break;

            case SMTP_POSTDATA:
                result = smtp_state_postdata_resp(conn_ptr, smtpcode, smtpc->state);
                break;

            case SMTP_QUIT:
                /* fallthrough, just stop! */
            default:
                /* internal error */
                state(conn_ptr, SMTP_STOP);
                break;
        }
        qurl_logd("smtp state=%d", smtpc->state);
    } while (!result && smtpc->state != SMTP_STOP && qurl_pp_moredata(pp));

    qurl_logd("smtp_statemach_act end");
    return result;
}

/* Called repeatedly until done_ptr from multi.c */
static qurl_ecode_t smtp_multi_statemach(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t      result = QURL_OK;
    qurl_bit_t        tls_done_flag = QOSA_FALSE;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;

    qurl_logd("enter");
    if ((conn_ptr->port_handler->flags & QURL_PROT_FLAGS_SSL) && !smtpc->ssldone)
    {
        qurl_logd("tls ");
        result = qurl_conn_connect(conn_ptr, QURL_SOCKET_INDEX_0, &tls_done_flag);
        smtpc->ssldone = tls_done_flag;
        qurl_logd("tls_done_flag=%d", tls_done_flag);
        if (result != QURL_OK || !smtpc->ssldone)
        {
            return result;
        }
        conn_ptr->bits.tls_connecting = !tls_done_flag;
    }

    qurl_logd("qurl_pp_state_machine start");
    result = qurl_pp_state_machine(conn_ptr, &smtpc->pp, QOSA_FALSE, QOSA_FALSE);
    qurl_logd("qurl_pp_state_machine end");
    *done_ptr = (smtpc->state == SMTP_STOP) ? QOSA_TRUE : QOSA_FALSE;
    qurl_logd("smtp_multi_statemach result=%d,done_ptr=%d", result, *done_ptr);

    return result;
}

static qurl_ecode_t smtp_block_statemach(qurl_conn_t *conn_ptr, qurl_bit_t disconnecting)
{
    qurl_ecode_t      result = QURL_OK;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;

    qurl_logd("enter");
    while (smtpc->state != SMTP_STOP && result == QURL_OK)
    {
        /*There may be scenarios where it remains resident, so monitoring points can be placed.*/
        QURL_CORE_CHECK_ABORT_FUNC(conn_ptr->core_ptr);

        result = qurl_pp_state_machine(conn_ptr, &smtpc->pp, QOSA_TRUE, disconnecting);
    }
    qurl_logd("result=%d", result);
    return result;
}

#ifdef QURL_CFG_ENABLE_SMTP_DEL  //delete
//The original curl library also only has the smtp_block_statemach interface;
//Note the inner loop qurl_pp_state_machine , the maximum default value is 120s
//This interface is not very useful and is currently reserved and unused.
static qurl_ecode_t smtp_nonblock_statemach(qurl_conn_t *conn_ptr, qurl_bit_t disconnecting)
{
    qurl_ecode_t      result = QURL_ECODE_FAILURE;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;

    qurl_logd("enter");
    if (smtpc->state != SMTP_STOP && result == QURL_OK)  //nonblock, Cannot write while, infinite loop
    {
        result = qurl_pp_state_machine(conn_ptr, &smtpc->pp, QOSA_FALSE, disconnecting);
    }

    return result;
}
#endif /* QURL_CFG_ENABLE_SMTP_EXT */

/* Allocate and initialize the SMTP struct for the current Curl_easy if
   required */
static qurl_ecode_t smtp_init(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      result = QURL_OK;
    struct SMTP      *smtp;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    qurl_logd("enter");

    if (core_ptr->bits.prot_data_alloc_flag && core_ptr->prot_data)
    {
        qurl_free(core_ptr->prot_data);
        core_ptr->bits.prot_data_alloc_flag = QOSA_FALSE;
    }

    smtp = core_ptr->prot_data = qosa_calloc(sizeof(struct SMTP), 1);
    if (!smtp)
    {
        result = QURL_ECODE_NO_MEMORY;
    }
    core_ptr->bits.prot_data_alloc_flag = QOSA_TRUE;

    return result;
}

static qurl_ecode_t smtp_upload_data(qurl_conn_t *conn_ptr)
{
    long              len = 0;
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    core_idle_timeout_feed_dog(core_ptr);
    if (core_ptr->usr_cfg.read_from_client_func)
    {
        //len = QURL_MIN(len, QURL_CONN_DEAL_CACHE_SIZE);
        len = core_ptr->usr_cfg.read_from_client_func(conn_ptr->deal_cache_ptr, QURL_CONN_DEAL_CACHE_SIZE, core_ptr->usr_cfg.read_from_client_arg);
        if (len > 0)
        {
            qurl_logd("push len=%d", len);
            ret = qurl_dbuf_push_data(core_ptr->send_cache, (void *)conn_ptr->deal_cache_ptr, len);
        }
        core_ptr->trans.uploaded_len += len;

        //Unable to determine the size of the uploaded data, so exit only when the data read by cb is 0.
        core_ptr->trans.upload_len_max = core_ptr->trans.uploaded_len + 1;
        if (len == 0)
        {
            //Data transmission complete, exit upload
            core_ptr->trans.upload_len_max = core_ptr->trans.uploaded_len;
        }

        qurl_logd("upload len:[%ld][%ld],ret=%d\r\n", core_ptr->trans.upload_len_max, core_ptr->trans.uploaded_len, ret);
        //qosa_task_sleep_sec(1);
        return ret;
    }
#if 0
    else if (len > 0)
    {
        if (core_ptr->usr_cfg.upload_data_ptr)
        {
            /*[lzm][note]If a customer reports insufficient memory, then handle this. Send customer data in batches, add a length limit to dbuf, and add a progress bar, so that all data is not pushed to dbuf at once.*/
            qurl_dbuf_push_data(core_ptr->send_cache, core_ptr->usr_cfg.upload_data_ptr, len);
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
            qurl_dbuf_push_data(core_ptr->send_cache, conn_ptr->deal_cache_ptr, len);
            core_ptr->trans.uploaded_len += len;
            if (core_ptr->trans.upload_len_max <= core_ptr->trans.uploaded_len)
            {
                qurl_fclose(core_ptr->cfg.upload_file_fd);
                core_ptr->cfg.upload_file_fd = -1;
            }
        }
        else if (core_ptr->usr_cfg.read_from_client_func)
        {
            len = QURL_MIN(len, QURL_CONN_DEAL_CACHE_SIZE);
            len = core_ptr->usr_cfg.read_from_client_func(conn_ptr->deal_cache_ptr, len, core_ptr->usr_cfg.read_from_client_arg);
            qurl_dbuf_push_data(core_ptr->send_cache, (void *)conn_ptr->deal_cache_ptr, len);
            core_ptr->trans.uploaded_len += len;
        }
        else
        {
            qurl_logd("no data upload!!!\r\n");
        }
    }
#endif

    QURL_CORE_IDLE_TIMEOUT_CHECK_DOG_FUNC(core_ptr);

    //qurl_logd("upload plan:[%ld][%ld]\r\n", core_ptr->trans.upload_len_max, core_ptr->trans.uploaded_len);
    return ret;
}

/* For the SMTP "protocol connect" and "doing" phases only */
static int smtp_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr)
{
    qurl_logd("enter");
    return qurl_pp_getsock(conn_ptr, &conn_ptr->prot_data_u.smtp_ctrl.pp, socks_ptr);
}

#if 1
static int smtp_trans_data_getsock(qurl_conn_t *conn_ptr, qurl_socket_t *socks_ptr)
{
    int               bitmap = 0; /*Corresponding to the read and write of the monitored sockfd*/
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    /*Only need to listen for readability.*/
    qurl_logd("enter");
    /*It is necessary to monitor whether the control connection is normal. According to RFC959, when the control connection is closed, the data connection becomes invalid.*/
    if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd != QURL_SOCKET_BAD)
    {
        socks_ptr[0] = conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd;
        if (core_ptr->cfg.bits.upload)
        {
            bitmap |= QURL_GETSOCK_WRITESOCK_FUNC(0);
        }
        else
        {
            bitmap |= QURL_GETSOCK_READSOCK_FUNC(0);
        }
    }

    qurl_logd("bitmap=%d", bitmap);
    return bitmap;
}
#endif

/***********************************************************************
 *
 * smtp_connect()
 *
 * This function should do everything that is to be considered a part of
 * the connection phase.
 *
 * The variable pointed to by 'done_ptr' will be QOSA_TRUE if the protocol-layer
 * connect phase is done_ptr when this function returns, or QOSA_FALSE if not.
 */
static qurl_ecode_t smtp_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t      result = QURL_OK;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    qurl_pp_ctrl_t   *pp = &smtpc->pp;
    qurl_pp_cfg_t     pp_cfg = {0};
    *done_ptr = QOSA_FALSE; /* default to not done_ptr yet */

    qurl_logd("enter");

    /* We always support persistent connections in SMTP */

    /* Set the default response time-out */
    pp_cfg.pong_timeout_ms = QURL_CFG_PONG_TIMEOUT_DEFAULT;
    pp_cfg.statemachine_func = smtp_statemach_act;
    pp_cfg.pong_func = smtp_endofresp;

    /* Initialize the SASL storage */
    qurl_sasl_init(&smtpc->sasl, &saslsmtp);

    /* Initialise the pingpong layer */
    qurl_pp_init(pp, &pp_cfg);

    /* Parse the URL options */
    result = smtp_parse_url_options(conn_ptr);
    if (result)
    {
        return result;
    }

    /* Parse the URL path */
    result = smtp_parse_url_path(conn_ptr);
    if (result)
    {
        return result;
    }

    /* Start off waiting for the server greeting response */
    state(conn_ptr, SMTP_SERVERGREET);

    result = smtp_multi_statemach(conn_ptr, done_ptr);

    return result;
}

static qurl_ecode_t smtp_trans_data(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    qurl_logd("enter");
    *done_ptr = QOSA_FALSE;

    if (core_ptr->trans.upload_len_max == 0)
    {
        /*Parse request result 354*/
        state(conn_ptr, SMTP_DATA);
        ret = smtp_block_statemach(conn_ptr, QOSA_FALSE);  //Blocking mode, waiting for server to send down
        if (ret != QURL_OK)
        {
            return ret;
        }
    }

    qurl_logd("smtp_upload_data");
    /*Main text attachment group packaging*/

    if (*done_ptr == QOSA_FALSE)
    {
        /*There may be scenarios where it remains resident, so monitoring points can be placed.*/
        QURL_CORE_CHECK_ABORT_FUNC(conn_ptr->core_ptr);

        ret = smtp_upload_data(conn_ptr);
        if (ret != QURL_OK)
        {
            qurl_logd("smtp_upload_data err ret=%d", ret);
            return ret;
        }

        /*Push to conn*/
        ret = qurl_dbuf_concat_tail(conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].send_cache, core_ptr->send_cache);
        qurl_logd("qurl_dbuf_concat_tail ret=%x", ret);
        ret = qurl_conn_send_cache(conn_ptr, QURL_SOCKET_INDEX_0);
        if (ret != QURL_OK)
        {
            qurl_loge("upload faild=%x", ret);
            return ret;
        }

        qurl_logd("upload_len_max=%d,%d", core_ptr->trans.upload_len_max, core_ptr->trans.uploaded_len);
        if (core_ptr->trans.upload_len_max <= core_ptr->trans.uploaded_len) /*There is also body data that needs to be uploaded.*/
        {
            /*Upload completed*/
            *done_ptr = QOSA_TRUE;
            qurl_logd("done_ptr true");
        }
    }
    qurl_logd("end");

    return QURL_OK;
}

/***********************************************************************
 *
 * smtp_done()
 *
 * The DONE function. This does what needs to be done_ptr after a single DO has
 * performed.
 *
 * Input argument is already checked for validity.
 */
static qurl_ecode_t smtp_done(qurl_conn_t *conn_ptr, qurl_ecode_t ecode, qurl_bit_t premature)
{
    qurl_ecode_t    result = QURL_OK;
    qurl_ecode_t    ret = QURL_OK;
    struct SMTP    *smtp = conn_ptr->core_ptr->prot_data;
    qurl_pp_ctrl_t *pp = &conn_ptr->prot_data_u.smtp_ctrl.pp;
    char           *eob = QOSA_NULL;
    int             len;

    qurl_logd("enter");
    (void)premature;

    if (!smtp)
    {
        return QURL_OK;
    }

    /* Cleanup our per-request based variables */
    qurl_safe_free(smtp->custom);

    if (ecode)
    {
        //connclose(conn_ptr, "SMTP done_ptr with bad status"); /* marked for closure */
        result = ecode; /* use the already set error code */
    }
    //else if (!data->set.connect_only && conn_ptr->core_ptr->usr_cfg.mail_rtcp_ptr && (data->set.upload || data->set.mimepost.kind))
    else if (conn_ptr->core_ptr->usr_cfg.smtp.rtcp_slist)
    {
        /* Calculate the EOB taking into account any terminating CRLF from the
       previous line of the email or the CRLF of the DATA command when there
       is "no mail data". RFC-5321, sect. 4.1.1.4.

       Note: As some SSL backends, such as OpenSSL, will cause Curl_write() to
       fail when using a different pointer following a previous write, that
       returned CURLE_AGAIN, we duplicate the EOB now rather than when the
       bytes written doesn't equal len. */
        //if (smtp->trailing_crlf || !conn_ptr->data->state.infilesize)
        qurl_logd("trailing_crlf=%d", smtp->trailing_crlf);
        if (smtp->trailing_crlf)
        {
            eob = qurl_strdup(&SMTP_EOB[2]);
            len = SMTP_EOB_LEN - 2;
        }
        else
        {
            eob = qurl_strdup(SMTP_EOB);
            len = SMTP_EOB_LEN;
        }

        if (!eob)
        {
            return QURL_ECODE_NO_MEMORY;
        }

        /* Send the end of block data */
        /*Push to conn*/
        //Internal newly requested resource -- needs to release eob
        qurl_dbuf_push_data(conn_ptr->core_ptr->send_cache, eob, len);
        qurl_free(eob); //Subsequent usage will no longer employ immediate release

        qurl_logd("eob len=%d", len);
        conn_ptr->core_ptr->trans.uploaded_len += len; /*All entries have been enqueued.*/
        qurl_dbuf_concat_tail(conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].send_cache, conn_ptr->core_ptr->send_cache);
        ret = qurl_conn_send_cache(conn_ptr, QURL_SOCKET_INDEX_0);
        if (ret != QURL_OK)
        {
            qurl_logd("upload faild");
        }
        if (result != QURL_OK)
        {
            qurl_logd("upload faild\r\n");
            return result;
        }

        qurl_logd("ret=%d,%d", ret, len);

        if (pp->pending_resp)
        {
        }
        else
        {
            /* Successfully sent so adjust the response timeout relative to now */
            pp->ping_time = qurl_time_now();
        }

        state(conn_ptr, SMTP_POSTDATA);

        /* Run the state-machine */
        result = smtp_block_statemach(conn_ptr, QOSA_FALSE);
    }

    /* Clear the transfer mode for the next request */
    smtp->transfer = QURL_PP_TRANSFER_BODY;

    return result;
}

/***********************************************************************
 *
 * smtp_perform()
 *
 * This is the actual DO function for SMTP. Transfer a mail, send a command
 * or get some data according to the options previously setup.
 */
static qurl_ecode_t smtp_perform(qurl_conn_t *conn_ptr, qurl_bit_t *connected, qurl_bit_t *dophase_done)
{
    /* This is SMTP and no proxy */
    qurl_ecode_t result = QURL_OK;
    struct SMTP *smtp = conn_ptr->core_ptr->prot_data;

    QURL_UNUSED(connected);
    qurl_logd("DO phase starts =%d", *connected);

#if 0
    if (data->set.opt_no_body)
    {
        /* Requested no body means no transfer */
        smtp->transfer = FTPTRANSFER_INFO;
    }
#endif

    *dophase_done = QOSA_FALSE; /* not done_ptr yet */

    /* Store the first recipient (or NULL if not specified) */
    smtp->rcpt = conn_ptr->core_ptr->usr_cfg.smtp.rtcp_slist;

    /* Track of whether we've successfully sent at least one RCPT TO command */
    smtp->rcpt_had_ok = QOSA_FALSE;

    /* Track of the last error we've received by sending RCPT TO command */
    smtp->rcpt_last_error = 0;

    /* Initial data character is the first character in line: it is implicitly
     preceded by a virtual CRLF. */
    smtp->trailing_crlf = QOSA_TRUE;
    smtp->eob = 2;

    /* Start the first command in the DO phase */
    //if ((data->set.upload || data->set.mimepost.kind) && data->set.mail_rcpt)
    qurl_logd("smtp->rcpt =%p", smtp->rcpt);
    if (smtp->rcpt)
    {
        /* MAIL transfer */
        result = smtp_perform_mail(conn_ptr);
    }
    else
    {
        /* SMTP based command (VRFY, EXPN, NOOP, RSET or HELP) */
        result = smtp_perform_command(conn_ptr);
    }

    if (result)
    {
        return result;
    }

    /* Run the state-machine */
    result = smtp_multi_statemach(conn_ptr, dophase_done);

    if (*dophase_done)
    {
        qurl_logd("DO phase is complete\n");
    }

    return result;
}

/***********************************************************************
 *
 * smtp_do()
 *
 * This function is registered as 'curl_do' function. It decodes the path
 * parts etc as a wrapper to the actual DO function (smtp_perform).
 *
 * The input argument is already checked for validity.
 */
static qurl_ecode_t smtp_do(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_ecode_t result = QURL_OK;

    qurl_logd("enter");
    *done_ptr = QOSA_FALSE; /* default to false */

    /* Parse the custom request */
    result = smtp_parse_custom_request(conn_ptr);
    if (result)
    {
        return result;
    }

    result = smtp_regular_transfer(conn_ptr, done_ptr);

    return result;
}

/***********************************************************************
 *
 * smtp_disconnect()
 *
 * Disconnect from an SMTP server. Cleanup protocol-specific per-connection
 * resources. BLOCKING.
 */
static qurl_ecode_t smtp_disconnect(qurl_conn_t *conn_ptr, qurl_bit_t dead_connection)
{
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
    qurl_smtp_ctrl_t *smtp_ctrl_ptr = (qurl_smtp_ctrl_t *)&(conn_ptr->prot_data_u.smtp_ctrl);
    qurl_pp_ctrl_t   *pp_ctrl_ptr = (qurl_pp_ctrl_t *)&(smtp_ctrl_ptr->pp);

    qurl_logd("enter");
    SMTP_UNUSED(core_ptr);
    /* We cannot send quit unconditionally. If this connection is stale or
    bad in any way, sending quit and waiting around here will make the
    disconnect wait in vain and cause more problems than we need to. */

    /* The SMTP session may or may not have been allocated/setup at this
    point! */
    if (!dead_connection)
    {
        if (!smtp_perform_quit(conn_ptr))
        {
            (void)smtp_block_statemach(conn_ptr, QOSA_TRUE); /* ignore errors on QUIT */
        }
    }

    qurl_logd("qurl_pp_deinit");
    qurl_pp_deinit(pp_ctrl_ptr);

    /* Cleanup our connection based variables */
    qurl_safe_free(smtpc->domain);

    if (core_ptr->bits.prot_data_alloc_flag && core_ptr->prot_data)
    {
        qurl_free(core_ptr->prot_data);
        core_ptr->prot_data = QOSA_NULL;
        core_ptr->bits.prot_data_alloc_flag = QOSA_FALSE;
    }

    return QURL_OK;
}

/* Call this when the DO phase has completed */
static qurl_ecode_t smtp_dophase_done(qurl_conn_t *conn_ptr, qurl_bit_t connected)
{
    struct SMTP *smtp = conn_ptr->core_ptr->prot_data;

    (void)connected;

    if (smtp->transfer != QURL_PP_TRANSFER_BODY)
    {
        /* no data to transfer */
        //Curl_setup_transfer(conn_ptr->data, -1, -1, QOSA_FALSE, -1);
    }

    return QURL_OK;
}

/* Called from multi.c while DOing */
static qurl_ecode_t smtp_doing(qurl_conn_t *conn_ptr, qurl_bit_t *done_ptr)
{
    qurl_logd("enter");
    qurl_ecode_t result = smtp_multi_statemach(conn_ptr, done_ptr);

    qurl_logd("result=%d", result);
    if (result)
    {
        qurl_logd("DO phase failed\n");
    }
    else if (*done_ptr)
    {
        result = smtp_dophase_done(conn_ptr, QOSA_FALSE /* not connected */);

        qurl_logd("DO phase is complete\n");
    }

    return result;
}

/***********************************************************************
 *
 * smtp_regular_transfer()
 *
 * The input argument is already checked for validity.
 *
 * Performs all commands done_ptr before a regular transfer between a local and a
 * remote host.
 */
static qurl_ecode_t smtp_regular_transfer(qurl_conn_t *conn_ptr, qurl_bit_t *dophase_done)
{
    qurl_ecode_t result = QURL_OK;
    qurl_bit_t   connected = QOSA_FALSE;

    /* Make sure size is unknown at this point */
    //data->req.size = -1;

    qurl_logd("enter");
    /* Carry out the perform */
    result = smtp_perform(conn_ptr, &connected, dophase_done);

    /* Perform post DO phase operations if necessary */
    if (!result && *dophase_done)
    {
        result = smtp_dophase_done(conn_ptr, connected);
    }

    return result;
}

static qurl_ecode_t smtp_setup_connection(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t result;

    qurl_logd("enter");
    /* Clear the TLS upgraded flag */
    conn_ptr->bits.tls_connecting = QOSA_FALSE;

    /* Initialise the SMTP layer */
    result = smtp_init(conn_ptr);
    if (result)
    {
        return result;
    }

    return QURL_OK;
}

/***********************************************************************
 *
 * smtp_parse_url_options()
 *
 * Parse the URL login options.
 */
static qurl_ecode_t smtp_parse_url_options(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t result = QURL_OK;
    //qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    //const char              *ptr = conn_ptr->options;

    qurl_logd("enter");
    SMTP_UNUSED(conn_ptr);

#if 0
    smtpc->sasl.resetprefs = QOSA_TRUE;
    qurl_logd("harry smtp_parse_url_options options=%s", conn_ptr->options);

    while (!result && ptr && *ptr)
    {
        const char *key = ptr;
        const char *value;

        while (*ptr && *ptr != '=') ptr++;

        value = ptr + 1;

        while (*ptr && *ptr != ';') ptr++;

        if (strncasecompare(key, "AUTH=", 5))
            result = Curl_sasl_parse_url_auth_option(&smtpc->sasl, value, ptr - value);
        else
            result = CURLE_URL_MALFORMAT;

        if (*ptr == ';')
            ptr++;
    }
#endif

    return result;
}

/***********************************************************************
 *
 * smtp_parse_url_path()
 *
 * Parse the URL path into separate path components.
 */
static qurl_ecode_t smtp_parse_url_path(qurl_conn_t *conn_ptr)
{
    /* The SMTP struct is already initialised in smtp_connect() */
    qurl_ecode_t      ret = QURL_OK;
    long              path_len = 0;
    qurl_smtp_ctrl_t *smtpc = &conn_ptr->prot_data_u.smtp_ctrl;
    struct SMTP      *smtp = conn_ptr->core_ptr->prot_data;

    qurl_logd("enter");
    SMTP_UNUSED(path_len);
    SMTP_UNUSED(smtpc);
    SMTP_UNUSED(smtp);
    /* Calculate the path if necessary */

    //qurl_logd("harry smtp_parse_url_path=%s", path);
    /* URL decode the path and use it as the domain in our EHLO */
    //qurl_urldecode(conn_ptr->data, path, 0, &smtpc->domain, QOSA_NULL, REJECT_CTRL);

    if (smtpc->domain == QOSA_NULL)
    {
        smtpc->domain = qurl_malloc(qosa_strlen("localhost") + 1);
        qurl_memset(smtpc->domain, 0, qosa_strlen("localhost") + 1);
        qurl_memcpy(smtpc->domain, "localhost", qurl_strlen("localhost"));
    }

    return ret;
}

/***********************************************************************
 *
 * smtp_parse_custom_request()
 *
 * Parse the custom request.
 */
static qurl_ecode_t smtp_parse_custom_request(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t result = QURL_OK;

    qurl_logd("enter");
    SMTP_UNUSED(conn_ptr);
#if 0
    struct SMTP      *smtp = data->req.protop;
    const char       *custom = data->set.str[STRING_CUSTOMREQUEST];

    /* URL decode the custom request */
    smtp_log("smtp_parse_custom_request custom=%s", custom);
    if (custom)
        result = Curl_urldecode(data, custom, 0, &smtp->custom, QOSA_NULL, REJECT_CTRL);
#endif

    return result;
}

/***********************************************************************
 *
 * smtp_parse_address()
 *
 * Parse the fully qualified mailbox address into a local address part and the
 * host name, converting the host name to an IDN A-label, as per RFC-5890, if
 * necessary.
 *
 * Parameters:
 *
 * conn  [in]              - The connection handle.
 * fqma  [in]              - The fully qualified mailbox address (which may or
 *                           may not contain UTF-8 characters).
 * address        [in/out] - A new allocated buffer which holds the local
 *                           address part of the mailbox. This buffer must be
 *                           free'ed by the caller.
 * host           [in/out] - The host name structure that holds the original,
 *                           and optionally encoded, host name.
 *                           Curl_free_idnconverted_hostname() must be called
 *                           once the caller has finished with the structure.
 *
 * Returns QURL_OK on success.
 *
 * Notes:
 *
 * Should a UTF-8 host name require conversion to IDN ACE and we cannot honor
 * that conversion then we shall return success. This allow the caller to send
 * the data to the server as a U-label (as per RFC-6531 sect. 3.2).
 *
 * If an mailbox '@' separator cannot be located then the mailbox is considered
 * to be either a local mailbox or an invalid mailbox (depending on what the
 * calling function deems it to be) then the input will simply be returned in
 * the address part with the host name being NULL.
 */
static qurl_ecode_t smtp_parse_address(qurl_conn_t *conn_ptr, const char *fqma, char **address, struct hostname *host)
{
    qurl_ecode_t result = QURL_OK;
    int          length;

    SMTP_UNUSED(conn_ptr);
    qurl_logd("enter");
    /* Duplicate the fully qualified email address so we can manipulate it,
    ensuring it doesn't contain the delimiters if specified */
    char *dup = qurl_strdup(fqma[0] == '<' ? fqma + 1 : fqma);
    if (!dup)
    {
        return QURL_ECODE_NO_MEMORY;
    }

    length = qurl_strlen(dup);
    if (length)
    {
        if (dup[length - 1] == '>')
        {
            dup[length - 1] = '\0';
        }
    }

    /* Extract the host name from the address (if we can) */
    host->name = qurl_strpbrk(dup, "@");
    if (host->name)
    {
        *host->name = '\0';
        host->name = host->name + 1;

        /* Attempt to convert the host name to IDN ACE */
        //(void)Curl_idnconvert_hostname(conn_ptr, host);

        /* If Curl_idnconvert_hostname() fails then we shall attempt to continue
       and send the host name using UTF-8 rather than as 7-bit ACE (which is
       our preference) */
    }

    /* Extract the local address from the mailbox */
    *address = dup;

    return result;
}

qurl_ecode_t qurl_smtp_escape_eob(qurl_conn_t *conn_ptr, const int nread)
{
    /* When sending a SMTP payload we must detect CRLF. sequences making sure
     they are sent as CRLF.. instead, as a . on the beginning of a line will
     be deleted by the server when not part of an EOB terminator and a
     genuine CRLF.CRLF which isn't escaped will wrongly be detected as end of
     data by the server
    */
    int          i;
    int          si;
    struct SMTP *smtp = conn_ptr->core_ptr->prot_data;
    //char        *scratch = data->state.scratch;
    char *newscratch = QOSA_NULL;
    char *oldscratch = QOSA_NULL;
    int   eob_sent;

    qurl_logd("enter");
    SMTP_UNUSED(i);
    SMTP_UNUSED(si);
    SMTP_UNUSED(smtp);
    SMTP_UNUSED(newscratch);
    SMTP_UNUSED(oldscratch);
    SMTP_UNUSED(eob_sent);
    SMTP_UNUSED(conn_ptr);
    SMTP_UNUSED(nread);
#if 0
    /* Do we need to allocate a scratch buffer? */
    if (!scratch || data->set.crlf)
    {
        oldscratch = scratch;

        scratch = newscratch = qurl_malloc(2 * data->set.upload_buffer_size);
        if (!newscratch)
        {
            qurl_logd("Failed to alloc scratch buffer!");

            return QURL_ECODE_NO_MEMORY;
        }
    }
    DEBUGASSERT(data->set.upload_buffer_size >= (int)nread);


    /* Have we already sent part of the EOB? */
    eob_sent = smtp->eob;

    /* This loop can be improved by some kind of Boyer-Moore style of
     approach but that is saved for later... */
    for (i = 0, si = 0; i < nread; i++)
    {
        if (SMTP_EOB[smtp->eob] == data->req.upload_fromhere[i])
        {
            smtp->eob++;

            /* Is the EOB potentially the terminating CRLF? */
            if (2 == smtp->eob || SMTP_EOB_LEN == smtp->eob)
                smtp->trailing_crlf = QOSA_TRUE;
            else
                smtp->trailing_crlf = QOSA_FALSE;
        }
        else if (smtp->eob)
        {
            /* A previous substring matched so output that first */
            qurl_memcpy(&scratch[si], &SMTP_EOB[eob_sent], smtp->eob - eob_sent);
            si += smtp->eob - eob_sent;

            /* Then compare the first byte */
            if (SMTP_EOB[0] == data->req.upload_fromhere[i])
                smtp->eob = 1;
            else
                smtp->eob = 0;

            eob_sent = 0;

            /* Reset the trailing CRLF flag as there was more data */
            smtp->trailing_crlf = QOSA_FALSE;
        }

        /* Do we have a match for CRLF. as per RFC-5321, sect. 4.5.2 */
        if (SMTP_EOB_FIND_LEN == smtp->eob)
        {
            /* Copy the replacement data to the target buffer */
            qurl_memcpy(&scratch[si], &SMTP_EOB_REPL[eob_sent], SMTP_EOB_REPL_LEN - eob_sent);
            si += SMTP_EOB_REPL_LEN - eob_sent;
            smtp->eob = 0;
            eob_sent = 0;
        }
        else if (!smtp->eob)
            scratch[si++] = data->req.upload_fromhere[i];
    }

    if (smtp->eob - eob_sent)
    {
        /* A substring matched before processing ended so output that now */
        qurl_memcpy(&scratch[si], &SMTP_EOB[eob_sent], smtp->eob - eob_sent);
        si += smtp->eob - eob_sent;
    }

    /* Only use the new buffer if we replaced something */
    if (si != nread)
    {
        /* Upload from the new (replaced) buffer instead */
        data->req.upload_fromhere = scratch;

        /* Save the buffer so it can be freed later */
        data->state.scratch = scratch;

        /* Free the old scratch buffer */
        qurl_free(oldscratch);

        /* Set the new amount too */
        data->req.upload_present = si;
    }
    else
        qurl_free(newscratch);

#endif

    return QURL_OK;
}

#endif /* CURL_DISABLE_SMTP */
