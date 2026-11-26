/*****************************************************************/ /**
* @file qcm_ping_app.c
* @brief
* @author larson.li@quectel.com
* @date 2024-04-22
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-04-22 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#include "qcm_ping_app.h"
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "qosa_asyn_dns.h"
#include "qcm_socket_adp.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_datacall.h"
#endif
#include "qosa_rtc.h"
#include "qcm_proj_config.h"
#include "qosa_sockets.h"
#include "qosa_ip_addr.h"
#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
#include "lwip_public.h"
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */

/*Adaptation of ICMP header files completed*/
#define PING_TASK_PRIO    QOSA_PRIORITY_NORMAL
#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
//In QT2131, change the value of PING_MSGQ_MAX_CNT to EXT_NPING_MAX_COUNT + 5. This will be able to accommodate an entire round of results along with a small amount of other messages.
#define PING_MSGQ_MAX_CNT (EXT_NPING_MAX_COUNT + 5)
#else  /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */
#define PING_MSGQ_MAX_CNT (5)
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */

#define QOS_LOG_TAG       LOG_TAG_PING_API

#if 1
//TODO:   Note here！！！！！！！！
//The structure definition used by ping is kept separate in the ping app, which will be very advantageous for cross-platform portability.
//However, when porting on the module platform, if the flash space usage is indeed insufficient
//You can replace this with the lwip-related interfaces from the module platform.

#define QCM_ICMP_ER                0  /* echo reply */
#define QCM_ICMP_DUR               3  /* destination unreachable */
#define QCM_ICMP_SQ                4  /* source quench */
#define QCM_ICMP_RD                5  /* redirect */
#define QCM_ICMP_ECHO              8  /* echo */
#define QCM_ICMP_TE                11 /* time exceeded */
#define QCM_ICMP_PP                12 /* parameter problem */
#define QCM_ICMP_TS                13 /* timestamp */
#define QCM_ICMP_TSR               14 /* timestamp reply */
#define QCM_ICMP_IRQ               15 /* information request */
#define QCM_ICMP_IR                16 /* information reply */
#define QCM_ICMP_AM                17 /* address mask request */
#define QCM_ICMP_AMR               18 /* address mask reply */

#define QCM_IP_PROTOCOL_ICMP       1  /* ICMP protocol */

#define QCM_IP6_HLEN               40
#define QCM_IP6_NEXTH_HOPBYHOP     0
#define QCM_IP6_NEXTH_TCP          6
#define QCM_IP6_NEXTH_UDP          17
#define QCM_IP6_NEXTH_ENCAPS       41
#define QCM_IP6_NEXTH_ROUTING      43
#define QCM_IP6_NEXTH_FRAGMENT     44
#define QCM_IP6_NEXTH_ICMP6        58
#define QCM_IP6_NEXTH_NONE         59
#define QCM_IP6_NEXTH_DESTOPTS     60
#define QCM_IP6_NEXTH_UDPLITE      136

#define QCM_IP6_FRAG_OFFSET_MASK   0xfff8
#define QCM_IP6_FRAG_MORE_FLAG     0x0001

/*Adapt ICMP header file*/
#define QCM_ICMPH_CODE_SET(hdr, t) ((hdr)->code = (t))
#define QCM_ICMPH_TYPE_SET(hdr, t) ((hdr)->type = (t))
#define QCM_IPH_HL(hdr)            ((hdr)->_v_hl & 0x0f)
#define QCM_IP6H_NEXTH(hdr)        ((hdr)->_nexth)

enum qcm_icmp6_type
{
    /** Destination unreachable */
    QCM_ICMP6_TYPE_DUR = 1,
    /** Packet too big */
    QCM_ICMP6_TYPE_PTB = 2,
    /** Time exceeded */
    QCM_ICMP6_TYPE_TE = 3,
    /** Parameter problem */
    QCM_ICMP6_TYPE_PP = 4,
    /** Private experimentation */
    QCM_ICMP6_TYPE_PE1 = 100,
    /** Private experimentation */
    QCM_ICMP6_TYPE_PE2 = 101,
    /** Reserved for expansion of error messages */
    QCM_ICMP6_TYPE_RSV_ERR = 127,

    /** Echo request */
    QCM_ICMP6_TYPE_EREQ = 128,
    /** Echo reply */
    QCM_ICMP6_TYPE_EREP = 129,
    /** Multicast listener query */
    QCM_ICMP6_TYPE_MLQ = 130,
    /** Multicast listener report */
    QCM_ICMP6_TYPE_MLR = 131,
    /** Multicast listener done */
    QCM_ICMP6_TYPE_MLD = 132,
    /** Router solicitation */
    QCM_ICMP6_TYPE_RS = 133,
    /** Router advertisement */
    QCM_ICMP6_TYPE_RA = 134,
    /** Neighbor solicitation */
    QCM_ICMP6_TYPE_NS = 135,
    /** Neighbor advertisement */
    QCM_ICMP6_TYPE_NA = 136,
    /** Redirect */
    QCM_ICMP6_TYPE_RD = 137,
    /** Multicast router advertisement */
    QCM_ICMP6_TYPE_MRA = 151,
    /** Multicast router solicitation */
    QCM_ICMP6_TYPE_MRS = 152,
    /** Multicast router termination */
    QCM_ICMP6_TYPE_MRT = 153,
    /** Private experimentation */
    QCM_ICMP6_TYPE_PE3 = 200,
    /** Private experimentation */
    QCM_ICMP6_TYPE_PE4 = 201,
    /** Reserved for expansion of informational messages */
    QCM_ICMP6_TYPE_RSV_INF = 255
};

typedef struct _qcm_ip4_addr
{
    qosa_uint32_t addr;
} qcm_ip4_addr_t;

struct qcm_icmp_echo_hdr
{
    qosa_uint8_t  type;
    qosa_uint8_t  code;
    qosa_uint16_t chksum;
    qosa_uint16_t id;
    qosa_uint16_t seqno;
};

struct qcm_ip_hdr
{
    /*version /header length*/
    qosa_uint8_t _v_hl;
    /*type of service*/
    qosa_uint8_t _tos;
    /*total length*/
    qosa_uint16_t _len;
    /*identification*/
    qosa_uint16_t _id;
    /*fragment offset field*/
    qosa_uint16_t _offset;
#define OSA_IP_RF      0X8000U /*reserved fragment flag*/
#define OSA_IP_DF      0X4000U /*do not fragment flag*/
#define OSA_IP_MF      0X2000U /*more fragment flag*/
#define OSA_IP_OFFMASK 0X1fffU /*mask for fragment bits*/
    /*time to live*/
    qosa_uint8_t _ttl;
    /*protocol*/
    qosa_uint8_t _proto;
    /*checksum*/
    qosa_uint16_t _chksum;
    /*source and destination IP addresses*/
    qcm_ip4_addr_t src;
    qcm_ip4_addr_t dest;
};

typedef struct _qcm_ip6_addr
{
    qosa_uint32_t addr[4];
} qcm_ip6_addr_t;

struct qcm_ip6_hdr
{
    /** version / traffic class / flow label */
    qosa_uint32_t _v_tc_fl;
    /** payload length */
    qosa_uint16_t _plen;
    /** next header */
    qosa_uint8_t _nexth;
    /** hop limit */
    qosa_uint8_t _hoplim;
    /** source and destination IP addresses */
    qcm_ip6_addr_t src;
    qcm_ip6_addr_t dest;
};

struct qcm_icmp6_echo_hdr
{
    qosa_uint8_t  type;
    qosa_uint8_t  code;
    qosa_uint16_t chksum;
    qosa_uint16_t id;
    qosa_uint16_t seqno;
};
struct qcm_ip6_frag_hdr
{
    /* next header */
    qosa_uint8_t _nexth;
    /* reserved */
    qosa_uint8_t reserved;
    /* fragment offset */
    qosa_uint16_t _fragment_offset;
    /* fragmented packet identification */
    qosa_uint32_t _identification;
};

struct qcm_ping_in_addr
{
    qosa_uint32_t s_addr;
};

struct qcm_ping_in6_addr
{
    union
    {
        qosa_uint32_t u32_addr[4];
        qosa_uint8_t  u8_addr[16];
    } un;
};

#define QCM_PING_NTOHS(x) QCM_PING_HTONS(x)
qosa_uint16_t QCM_PING_HTONS(qosa_uint16_t hostshort)
{
    qosa_uint16_t netshort;

    netshort = ((hostshort & 0x00ff) << 8) | ((hostshort & 0xff00) >> 8);

    return netshort;
}

qosa_uint16_t qcm_ping_calc_chksum(const void *dataptr, int len)
{
    qosa_uint32_t        acc;
    qosa_uint16_t        src;
    const unsigned char *octetptr;

    acc = 0;
    /* dataptr may be at odd or even addresses */
    octetptr = (const qosa_uint8_t *)dataptr;
    while (len > 1)
    {
        /* declare first octet as most significant
       thus assume network order, ignoring host order */
        src = (*octetptr) << 8;
        octetptr++;
        /* declare second octet as least significant */
        src |= (*octetptr);
        octetptr++;
        acc += src;
        len -= 2;
    }
    if (len > 0)
    {
        /* accumulate remaining octet */
        src = (*octetptr) << 8;
        acc += src;
    }
    /* add deferred carry bits */
    acc = (acc >> 16) + (acc & 0x0000ffffUL);
    if ((acc & 0xffff0000UL) != 0)
    {
        acc = (acc >> 16) + (acc & 0x0000ffffUL);
    }
    /* This maybe a little confusing: reorder sum using lwip_htons()
     instead of lwip_ntohs() since it has a little less call overhead.
     The caller must invert bits for Internet sum ! */
    return QCM_PING_HTONS((qosa_uint16_t)acc);
}

qosa_uint16_t QCM_PING_CHKSUM(const void *dataptr, qosa_uint16_t len)
{
    return (qosa_uint16_t) ~(qosa_uint32_t)qcm_ping_calc_chksum(dataptr, len);
}

#ifdef CONFIG_QOSA_ASR160X_PLATFORM_FUNC
static qosa_uint16_t QCM_PING_IP6_CHKSUM(const void *dataptr, qosa_uint16_t len, const struct in6_addr *src_addr, const struct in6_addr *dest_addr)
{
    qosa_uint32_t acc       = 0;
    qosa_uint32_t addr      = 0;
    qosa_uint8_t addr_part  = 0;
    qosa_uint8_t swapped    = 0;
    qosa_uint16_t proto     = 0;
    qosa_uint16_t proto_len = 0;

    proto = QCM_IP6_NEXTH_ICMP6;
    proto_len = len;

	for (addr_part = 0; addr_part < 4; addr_part++) {
        addr = (src_addr->s6_addr32[addr_part]);
        acc += (addr & 0xffffUL);
        acc += ((addr >> 16) & 0xffffUL);
        addr = (dest_addr->s6_addr32[addr_part]);
        acc += (addr & 0xffffUL);
        acc += ((addr >> 16) & 0xffffUL);
    }

    acc = FOLD_U32T(acc);
    acc = FOLD_U32T(acc);

    acc += LWIP_CHKSUM((void *)dataptr, len);
    acc = FOLD_U32T(acc);

    if (len % 2 != 0) {
        swapped = 1 - swapped;
        acc = SWAP_BYTES_IN_WORD(acc);
    }
    if (swapped) {
        acc = SWAP_BYTES_IN_WORD(acc);
    }

    acc += (qosa_uint32_t)htons(proto);
    acc += (qosa_uint32_t)htons(proto_len);

    acc = FOLD_U32T(acc);
    acc = FOLD_U32T(acc);

    return (qosa_uint16_t)~(acc & 0xffffUL);
}
#endif /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC */
#endif

/**
 * @enum ping_msg_e
 * @brief For ping event enumeration
 */
typedef enum
{
    QCM_PING_SEND_ICMP_REQUEST = 1000,
    QCM_PING_RECV_ICMP_REPLY = 1001,
    QCM_PING_RETRY_TIMER_EXP = 1002,
    QCM_PING_USER_STOP = 1003,
    QCM_PING_SOCKET_CLOSE = 1004,
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    QCM_PING_PDP_ACTIVE_REQ = 1005,
#endif
    QCM_PING_DNS_PARSE_REQ = 1006,
    QCM_PING_DNS_PARSE_RESP = 1007,  /*!< DNS resolution returned*/
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    QCM_PING_PDP_ACTIVE_RESP = 1008, /*!< PDP activation result returned*/
#endif
#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
    QCM_PING_PLATFORM_RESULT_IND = 1009, /*!< Per-packet result reported by the platform ping interface*/
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */
} qcm_ping_msg_e;

/**
 * @struct ping_task_msg_t
 * @brief Used for passing messages to the ping task via AT commands
 */
typedef struct
{
    qcm_ping_msg_e msgid; /*!< Ping task message event delivery*/
    void          *argv;  /*!< Message content*/
} qcm_ping_task_msg_t;

/**
 * @struct ql_ping_session_t
 * @brief Used as the information handle for the entire structure passed to the ping task
 */
typedef struct
{
    qosa_task_t           ping_task;             /*!< Task handle corresponding to the current session*/
    qosa_uint32_t         session_id;            /*!< Corresponds to the ID sequence of the current session*/
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    qosa_int32_t          pdp_cid;               /*!< Corresponds to the PDP CID sequence number passed by the user*/
    qosa_int32_t          sim_id;                /*!< Corresponds to SIM card ID*/
#endif
    qosa_int32_t          sock_hd;               /*!< Corresponds to the socket handle created for UDP*/
    qosa_int32_t          icmp_echo_msg_id;      /*!< Corresponds to the message ID of the ICMP message packet, incrementing progressively*/
    qosa_uint16_t         icmp_echo_req_seq_num; /*!< Corresponds to the sequence number of the ICMP message request*/
    char                 *host_name;             /*!< Record user request to ping server host name*/
    qosa_ip_addr_t        remote_ip;             /*!< Converted IP address*/
    qosa_uint8_t          ai_family;             /*!< The address type for requesting data is IPv4 or IPv6*/
    qcm_ping_config_type  user_opts;             /*!< User settings configured via AT commands*/
    qosa_int32_t          final_result;          /*!< Whether the ping command result is successful or failed*/
    qosa_timer_t          ping_response_timer;   /*!< Internal timer timeout function for the ping command*/
    qcm_ping_event_cb     notify_fcn;            /*!< User-registered callback function*/
    void                 *notify_arg;            /*!< Structure for user parameters carried during user registration*/
    qosa_msgq_t           ping_task_msgq;        /*!< ping task message queue*/
    qcm_ping_summary_type summary_info;          /*!< Summary information of ping execution*/
#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
    qosa_uint16_t         round_remain;          /*!< Results still expected from the current platform ping round*/
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */
} qcm_ping_session_t;

/**
 * @struct qcm_ping_dns_resp_t
 * @brief DNS result return for ping command execution
 */
typedef struct
{
    qosa_uint8_t            session_id; /*!< Session handle corresponding to the current ping operation*/
    qosa_dns_error_e        status;     /*!< Corresponds to the DNS resolution operation return status*/
    struct qosa_addrinfo_s *info;       /*!< Corresponding DNS resolution content*/
} qcm_ping_dns_resp_t;

/**
 * @struct qcm_ping_req_session_id_t
 * @brief Used for internal message passing within the ping task to prevent secondary issues caused by thread preemption.
 */
typedef struct
{
    qosa_uint8_t ping_session_id; /*!< Client ID corresponding to the ping session*/
} qcm_ping_req_session_id_t;

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
  
/**
 * @struct qcm_ping_pdp_resp_t
 * @brief Result returned when the user PDP is activated
 */
typedef struct
{
    qosa_uint8_t            ping_session_id; /*!< ping operation id*/
    qosa_datacall_act_cnf_t act_info;        /*!< PDP activation operation response content*/
} qcm_ping_pdp_resp_t;
#endif
/** Maximum number of concurrent ping sessions supported*/
#define QCM_TCPIP_PING_MAX_SESSION_CNT 3

/** Global record of client information*/
static qcm_ping_session_t *ping_mgr[QCM_TCPIP_PING_MAX_SESSION_CNT] = {QOSA_NULL};

/**
 * @brief Used for obtaining timestamps in the ping function; the ICMP protocol retrieves the number of milliseconds from midnight 0:00 to the current time.
 */
static qosa_uint32_t qcm_get_ping_timestamp_ms(void)
{
#if defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC)
    qosa_uint32_t ping_time_ms = qosa_get_system_tick_cnt() * (1000 / CONFIG_QOSA_SYS_TICK_PER_SECOND);

    QLOGV("reply time=%x", ping_time_ms);
    return ping_time_ms;
#else
    qosa_time_t curent_ms = 0;
    curent_ms = qosa_get_system_time_milliseconds();

    //Convert seconds to milliseconds, and add the microsecond part (converted to milliseconds)
    qosa_uint32_t ping_time_ms = (qosa_uint32_t)curent_ms;
    QLOGV("reply time=%x %x", ping_time_ms, curent_ms);
    return ping_time_ms;
#endif /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC||CONFIG_QOSA_ASR1903_PLATFORM_FUNC */
}

/**
 * @brief The app internally proactively notifies the caller of the current processing result event.
 *
 * @param[in] qcm_ping_session_t * ping_session
 *          - Corresponding ping session pointer
 *
 * @param[in] qcm_ping_event_type event_id
 *          - Corresponding to the ping message event type
 *
 * @param[in] qcm_ping_error_e evt_code
 *          - Corresponding to the internal execution status codes of the app
 *
 * @param[in] void * evt_param
 *          - Corresponding evt parameter for carrying processing
 *
 */
static void qcm_ping_info_event_cb(qcm_ping_session_t *ping_session, qcm_ping_event_type event_id, qcm_ping_error_e evt_code, void *evt_param)
{
    qcm_ping_resp_t  ping_info = {0};
    qcm_ping_error_e result_code = QCM_PING_OK;

    if (ping_session == QOSA_NULL || QOSA_NULL == ping_mgr[ping_session->session_id])
    {
        return;
    }

    result_code = evt_code;
    if (event_id == QCM_PING_STATS)
    {
        //Only when PING is successful will the ping information be copied.
        if (evt_code == QCM_PING_OK)
        {
            if (evt_param != QOSA_NULL)
            {
                qosa_memcpy(&ping_info.type.status, evt_param, sizeof(qcm_ping_stats_type));
            }
            else
            {
                QLOGE("evt_param == QOSA_NULL");
                result_code = QCM_PING_ERR_UNKNOW;
            }
        }
    }
    else
    {
        qosa_memcpy(&ping_info.type.summary, evt_param, sizeof(qcm_ping_summary_type));

        //larson.li 2024-08-09 22:40:18 If not all have failed to be sent during the final summary, then not only ERROR information should be reported, but the summary should still be reported as well.
        if (ping_info.type.summary.num_pkts_recvd != 0)
        {
            result_code = QCM_PING_OK;
        }
    }

    ping_session->notify_fcn(event_id, result_code, &ping_info, ping_session->notify_arg);
}

#ifndef CONFIG_QOSA_QT2131_PLATFORM_FUNC
/**
 * @brief Retrieve the corresponding ping session handle via the socket file descriptor
 *
 * @param[in] qosa_int32_t sock_hd
 *          - socket real handle
 *
 * @return qcm_ping_session_t *
 *       - If successful, returns the corresponding ping session handle
 *       - Returns QOSA_NULL on failure
 */
static qcm_ping_session_t *qcm_ping_socket_get_session_id(qosa_int32_t sock_hd)
{
    qosa_int32_t id = 0;
    for (id = 0; id < QCM_TCPIP_PING_MAX_SESSION_CNT; id++)
    {
        if ((ping_mgr[id] != QOSA_NULL) && (ping_mgr[id]->sock_hd == sock_hd))
        {
            break;
        }
    }
    if (id < QCM_TCPIP_PING_MAX_SESSION_CNT)
    {
        return ping_mgr[id];
    }
    else
    {
        return QOSA_NULL;
    }
}
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */

/**
 * @brief Get the corresponding ping operation address pointer based on the session ID
 *
 * @param[in] qosa_uint8_t session_id
 *          - Specifically designated session ID, used to look up the corresponding global record's handle information based on this ID
 *
 * @return qcm_ping_session_t *
 *       - If successful, returns the corresponding ping session handle
 *       - Returns QOSA_NULL on failure
 */
static qcm_ping_session_t *qcm_ping_get_session_ptr(qosa_uint8_t session_id)
{
    qcm_ping_session_t *ping_session = QOSA_NULL;

    if(session_id >= QCM_TCPIP_PING_MAX_SESSION_CNT)
    {
        return QOSA_NULL;
    }
    ping_session = ping_mgr[session_id];
    if (ping_session == QOSA_NULL || ping_session->ping_task == QOSA_NULL)
    {
        QLOGE("get NULL");
        return QOSA_NULL;
    }
    return ping_session;
}

/**
 * @brief Used to send messages to the ping main task message queue, guiding the next action to be taken.
 *
 * @param[in] qcm_ping_msg_e msgid
 *          - Message event type corresponding to the ping task
 *
 * @param[in] qcm_ping_session_t * ping_session
 *          - Corresponds to which ping session handle to send the message to
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK on success
 *       - Returns other on failure
 */
static qcm_ping_error_e qcm_ping_req_event(qcm_ping_msg_e msgid, qcm_ping_session_t *ping_session)
{
    qcm_ping_task_msg_t        ping_msg = {0};
    qcm_ping_req_session_id_t *send_ptr = QOSA_NULL;
    int ret = QCM_PING_OK;
    send_ptr = (qcm_ping_req_session_id_t *)qosa_malloc(sizeof(qcm_ping_req_session_id_t));
    if (send_ptr == QOSA_NULL)
    {
        return QCM_PING_ERR_UNKNOW;
    }

    send_ptr->ping_session_id = ping_session->session_id;

    ping_msg.msgid = msgid;
    ping_msg.argv = (void *)send_ptr;
    QLOGV("tcpip msgid = %d ", msgid);

    if (ping_session->ping_task_msgq != QOSA_NULL)
    {
        ret = qosa_msgq_release(ping_session->ping_task_msgq, sizeof(qcm_ping_task_msg_t), (qosa_uint8_t *)&ping_msg, QOSA_NO_WAIT);
        if (ret != QCM_PING_OK)
        {
            QLOGE("qosa_msgq_release failed, ret = %d", ret);
            qosa_free(send_ptr);
            ret = (ret == QOSA_ERROR_MSGQ_INVALID_ERR) ? QCM_PING_ERR_INVALID_PARAM : QCM_PING_ERR_UNKNOW;
            return ret;
        }
    }
    else
    {
        qosa_free(send_ptr);
        return QCM_PING_ERR_INVALID_PARAM;
    }

    return QCM_PING_OK;
}

#ifndef CONFIG_QOSA_QT2131_PLATFORM_FUNC
/**
 * @brief
 */
/**
 * @brief Register to the socket adapter underlying event callback function for reporting underlying socket status events.
 *
 * @param[in] qosa_int32_t sock_hd
 *          - Corresponding socket handle for reporting
 *
 * @param[in] qosa_int32_t event
 *          - Corresponding socket event type
 *
 * @param[in] qosa_int32_t code
 *          - Corresponding status result code
 *
 * @param[in] void * user_data
 *          - User parameters carried during registration, primarily the ping session address here
 *
 */
static void qcm_ping_socket_event_cb(int sock_hd, int event, int code, void *user_data)
{
    qcm_ping_session_t *ping_session = QOSA_NULL;
    QOSA_UNUSED(user_data); /*user_data points to ping_session*/
    QLOGV("enter ping event cb");
    ping_session = qcm_ping_socket_get_session_id(sock_hd);

    //If the user's registered callback parameter does not match the obtained session handle, exit directly to avoid operating on an illegal pointer.
    if (ping_session != user_data)
    {
        QLOGE("n=%p u=%p", ping_session, user_data);
        qcm_socket_close(sock_hd);
        return;
    }

    if (!(ping_session != QOSA_NULL && ping_session->ping_task != QOSA_NULL))
    {
        return;
    }
    QLOGV("evt:%d,%d,%x", sock_hd, event, code);

    if (event & QCM_SOCK_READ_EVENT)
    {
        qcm_ping_req_event(QCM_PING_RECV_ICMP_REPLY, ping_session);
    }
    if (event & QCM_SOCK_WRITE_EVENT)
    {
        qcm_ping_req_event(QCM_PING_SEND_ICMP_REQUEST, ping_session);
    }
    if (event & QCM_SOCK_CLOSE_EVENT)
    {
        qcm_ping_req_event(QCM_PING_SOCKET_CLOSE, ping_session);
    }
}
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
/**
 * @brief
 */
/**
 * @brief Callback for PDP execution result notification, used to notify PDP activation results
 *
 * @param[in] void * user_argv
 *          - User parameters registered during PDP activation
 *
 * @param[in] qcm_datacall_act_status_t * act_info
 *          - Corresponding PDP result information
 *
 * @return void
 */
static void qcm_ping_pdp_acitve_response_cb(void *user_argv, qosa_datacall_act_cnf_t *act_info)
{
    qcm_ping_task_msg_t        ping_msg = {0};
    qcm_ping_pdp_resp_t       *pdp_resp_ptr = QOSA_NULL;
    qcm_ping_session_t        *ping_session = QOSA_NULL;
    qcm_ping_req_session_id_t *send_ptr = QOSA_NULL;
    int ret = QCM_PING_OK;
    send_ptr = (qcm_ping_req_session_id_t *)user_argv;
    ping_session = qcm_ping_get_session_ptr(send_ptr->ping_session_id);
    if (ping_session == QOSA_NULL)
    {
        goto exit;
    }

    pdp_resp_ptr = (qcm_ping_pdp_resp_t *)qosa_malloc(sizeof(qcm_ping_pdp_resp_t));
    if (pdp_resp_ptr == QOSA_NULL)
    {
        goto exit;
    }
    qosa_memcpy(&pdp_resp_ptr->act_info, act_info, sizeof(qosa_datacall_act_cnf_t));
    pdp_resp_ptr->ping_session_id = send_ptr->ping_session_id;

    ping_msg.msgid = QCM_PING_PDP_ACTIVE_RESP;
    ping_msg.argv = (void *)pdp_resp_ptr;
    ret = qosa_msgq_release(ping_session->ping_task_msgq, sizeof(qcm_ping_task_msg_t), (qosa_uint8_t *)&ping_msg, QOSA_NO_WAIT);
    if (ret != QCM_PING_OK)
    {
        QLOGE("session:%d ret:%d", pdp_resp_ptr->ping_session_id, ret);
        qosa_free(pdp_resp_ptr);
    }
exit:
    qosa_free(send_ptr);
}

/**
 * @brief Execute PDP activation request action
 *
 * @param[in] void * argv
 *          - Corresponds to a pointer of type qcm_ping_req_session_id_t structure
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK on success
 *       - Returns other on failure
 */
static qcm_ping_error_e qcm_ping_service_data_call(void *argv)
{
    qcm_ping_error_e           result = QCM_PING_OK;
    qosa_datacall_errno_e      datacall_ret = QOSA_DATACALL_OK;
    qcm_ping_session_t        *ping_session = QOSA_NULL;
    qcm_ping_req_session_id_t *send_ptr = QOSA_NULL;
    qcm_ping_req_session_id_t *req_ptr = QOSA_NULL;
    qosa_datacall_conn_t       conn = 0;
    qosa_datacall_ip_info_t    info = {0};
    int                        ip_info = 0;

    send_ptr = (qcm_ping_req_session_id_t *)argv;
    ping_session = qcm_ping_get_session_ptr(send_ptr->ping_session_id);
    qosa_free(send_ptr);
    if (ping_session == QOSA_NULL)
    {
        result = QCM_PING_ERR_INVALID_PARAM;
        return result;
    }

    //Check module network registration status
    if (qosa_datacall_wait_attached(ping_session->sim_id, 0) == QOSA_FALSE)
    {
        QLOGE("network no register");
        result = QCM_PING_ERR_PDP_ACTIVE_FAILURE;
        return result;
    }
    //Check SIM PDPID activation status
    conn = qosa_datacall_conn_new(ping_session->sim_id, ping_session->pdp_cid, QOSA_DATACALL_CONN_TCPIP);
    if (qosa_datacall_get_status(conn) == QOSA_TRUE)
    {
        qosa_datacall_get_ip_info(conn, &info);
        ip_info = qcm_socket_checkip_is_ip46(ping_session->host_name);

        if (ip_info == 0 || (ip_info == QCM_AF_INET && info.ip_type != QOSA_PDP_IPV6) || (ip_info == QCM_AF_INET6 && info.ip_type != QOSA_PDP_IPV4))
        {
            //If already activated successfully, proceed directly to DNS resolution
            result = qcm_ping_req_event(QCM_PING_DNS_PARSE_REQ, ping_session);
            if (result != QCM_PING_OK)
            {
                result = QCM_PING_ERR_PDP_ACTIVE_FAILURE;  //Report error, exit
            }
        }
        else
        {
            result = QCM_PING_ERR_OPERATION_NOT_ALLOWED;  //Report error, exit
        }

        return result;
    }
    else
    {
        req_ptr = (qcm_ping_req_session_id_t *)qosa_malloc(sizeof(qcm_ping_req_session_id_t));
        if (req_ptr == QOSA_NULL)
        {
            result = QCM_PING_ERR_MEMORY;
            return result;
        }
        req_ptr->ping_session_id = ping_session->session_id;

        //Start executing activation action
        datacall_ret = qosa_datacall_start_async(
            conn,
            ping_session->user_opts.ping_response_time_out - 1,
            (datacall_callback_cb_ptr)qcm_ping_pdp_acitve_response_cb,
            req_ptr
        );
        if (datacall_ret != QOSA_DATACALL_OK)
        {
            result = QCM_PING_ERR_PDP_ACTIVE_FAILURE;
            return result;
        }
        //Waiting for activation to complete
        return result;
    }
}
#endif
/**
 * @brief DNS Result Notification Callback Function
 *
 * @param[in] void * argv
 *          - Corresponds to a pointer of type qcm_ping_req_session_id_t structure
 *
 * @param[in] struct qosa_addrinfo_s * * info
 *          - Corresponding DNS resolution content address
 *
 * @param[in] qosa_dns_error_e status
 *          - Corresponding DNS resolution result
 *
 * @return void
 */
void qcm_ping_dns_result_cb(void *argv, struct qosa_addrinfo_s **info, qosa_dns_error_e status)
{
    qcm_ping_task_msg_t        ping_msg = {0};
    qcm_ping_dns_resp_t       *dns_ptr = QOSA_NULL;
    qcm_ping_session_t        *ping_session = QOSA_NULL;
    qcm_ping_req_session_id_t *send_ptr = QOSA_NULL;
    int ret = QCM_PING_OK;
    send_ptr = (qcm_ping_req_session_id_t *)argv;
    ping_session = qcm_ping_get_session_ptr(send_ptr->ping_session_id);
    qosa_free(send_ptr);
    if (ping_session == QOSA_NULL)
    {
        goto exit;
    }

    dns_ptr = (qcm_ping_dns_resp_t *)qosa_malloc(sizeof(qcm_ping_dns_resp_t));
    if (dns_ptr == QOSA_NULL)
    {
        QLOGE("malloc faild");
        goto exit;
    }

    dns_ptr->session_id = ping_session->session_id;
    dns_ptr->status = status;
    if (status != QOSA_DNS_RESULT_OK)
    {
        dns_ptr->info = QOSA_NULL;
    }
    else
    {
        dns_ptr->info = *info;
    }

    ping_msg.msgid = QCM_PING_DNS_PARSE_RESP;
    ping_msg.argv = (void *)dns_ptr;
    QLOGV("tcpip msgid = %d ", QCM_PING_DNS_PARSE_RESP);
    ret = qosa_msgq_release(ping_session->ping_task_msgq, sizeof(qcm_ping_task_msg_t), (qosa_uint8_t *)&ping_msg, QOSA_NO_WAIT);
    if (ret == QCM_PING_OK)
    {
        return;
    }
exit:
    if (dns_ptr != QOSA_NULL)
    {
        dns_ptr->info = QOSA_NULL;
        qosa_free(dns_ptr);
    }
    if ((status == QOSA_DNS_RESULT_OK) && (*info != QOSA_NULL))
    {
        qosa_dns_result_free(*info);
    }
}

/**
 * @brief Execute the DNS resolution function
 *
 * @param[in] void * argv
 *          - Corresponds to a pointer of type qcm_ping_req_session_id_t structure
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK on success
 *       - Returns other on failure
 */
static qcm_ping_error_e qcm_ping_service_dns_parse(void *argv)
{
    qcm_ping_error_e           result = QCM_PING_OK;
    qosa_dns_error_e           dns_ret = QOSA_DNS_RESULT_OK;
    struct qosa_addrinfo_s     addrinfo_list = {0};
    qcm_ping_session_t        *ping_session = QOSA_NULL;
    qcm_ping_req_session_id_t *send_ptr = QOSA_NULL;
    qcm_ping_req_session_id_t *req_ptr = QOSA_NULL;
    int                        ip_type = 0;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    qosa_datacall_ip_info_t    info = {0};
    qosa_datacall_conn_t       conn = 0;
    qosa_datacall_errno_e      ret = QOSA_DATACALL_OK;
#endif
    send_ptr = (qcm_ping_req_session_id_t *)argv;
    ping_session = qcm_ping_get_session_ptr(send_ptr->ping_session_id);
    qosa_free(send_ptr);
    if (ping_session == QOSA_NULL)
    {
        result = QCM_PING_ERR_DNS_FAILURE;
        goto exit;
    }
    QLOGV("host_name :%s", ping_session->host_name);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    //Get the datacall IP type to determine whether it is IPv4 or IPv6.
    conn = qosa_datacall_conn_new(ping_session->sim_id, ping_session->pdp_cid, QOSA_DATACALL_CONN_TCPIP);
    ret = qosa_datacall_get_ip_info(conn, &info);
    if (ret == QOSA_DATACALL_ERR_NO_ACTIVE)
    {
        //Activation failed, return directly
        result = QCM_PING_ERR_PDP_ACTIVE_FAILURE;
        goto exit;
    }
#endif
    //QLOGV("host_name :%s ip_type=%d", ping_session->host_name, info.ip_type);
    //Get whether the user input is of IP type
    ip_type = qcm_socket_checkip_is_ip46(ping_session->host_name);
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    if (ip_type == QCM_PF_INET || ip_type == QCM_PF_INET6)
#else
    if (ip_type == OSA_PF_INET || ip_type == QCM_PF_INET6)
#endif
    {
        //If it is an IP type and matches the datacall IP type, or is a V4V6 type, proceed directly to the next request action.
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
        if (ip_type == QCM_PF_INET)
#else
        if (ip_type == OSA_PF_INET && (info.ip_type == QOSA_PDP_IPV4 || info.ip_type == QOSA_PDP_IPV4V6))
#endif
        {            
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
            inet_pton(QCM_PF_INET, ping_session->host_name, &ping_session->remote_ip.addr.ipv4_addr);
            ping_session->remote_ip.ip_vsn = QOSA_IP_V4;
#else
            inet_pton(OSA_PF_INET, ping_session->host_name, &ping_session->remote_ip.addr.ipv4_addr);
            ping_session->remote_ip.ip_vsn = QOSA_PDP_IPV4;
#endif     
        }
        //If it is an IPV6 type and matches the datacall IP type, or is a V4V6 type, proceed directly to the next request action.
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
        else if (ip_type == QCM_PF_INET6)
#else
        else if (ip_type == QCM_PF_INET6 && (info.ip_type == QOSA_PDP_IPV6 || info.ip_type == QOSA_PDP_IPV4V6))
#endif
        {
            inet_pton(QCM_PF_INET6, ping_session->host_name, &ping_session->remote_ip.addr.ipv6_addr);
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
            ping_session->remote_ip.ip_vsn = QOSA_IP_V6;
#else
            ping_session->remote_ip.ip_vsn = QOSA_PDP_IPV6;
#endif
        }
        else
        {
            //If it is an IP type and does not match the datacall IP type, directly return an error.
            result = QCM_PING_ERR_OPERATION_NOT_ALLOWED;
            goto exit;
        }
        ping_session->ai_family = ip_type;
        //QLOGV("ip_type = %d", ip_type);
        //QLOGV("ai_family = %d", ping_session->ai_family);
        //Proceed directly to the next request action.
        qcm_ping_req_event(QCM_PING_SEND_ICMP_REQUEST, ping_session);
    }
    else
    {
        req_ptr = (qcm_ping_req_session_id_t *)qosa_malloc(sizeof(qcm_ping_req_session_id_t));
        if (req_ptr == QOSA_NULL)
        {
            result = QCM_PING_ERR_MEMORY;
            goto exit;
        }
        req_ptr->ping_session_id = ping_session->session_id;

        //DNS resolution success callback or direct return
        dns_ret = qosa_dns_asyn_getaddrinfo(
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
            ping_session->sim_id,
            ping_session->pdp_cid,
#endif
            ping_session->host_name,
            &addrinfo_list,
            qcm_ping_dns_result_cb,
            (void *)req_ptr
        );
        if (dns_ret != QOSA_DNS_RESULT_OK)
        {
            qosa_free(req_ptr);
            result = QCM_PING_ERR_DNS_FAILURE;  //Report error
        }
    }

exit:
    return result;
}

#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
#define QCM_PING_QT2131_BUSY_RETRY_CNT (10)
#define QCM_PING_QT2131_BUSY_RETRY_MS  (50)
//Extra wait on top of the per-packet timeouts before a platform round is considered hung
#define QCM_PING_QT2131_GUARD_MS       (2000)

/**
 * @struct qcm_ping_qt2131_result_t
 * @brief Platform ping result forwarded from the platform callback to the ping task
 */
typedef struct
{
    qosa_uint8_t      session_id; /*!< Session that owns the platform round*/
    soc_ping_callback type;       /*!< Result type reported by the platform*/
    qosa_uint8_t      ttl;        /*!< TTL of the echo reply*/
    qosa_uint32_t     rtt;        /*!< RTT of the echo reply, ms*/
    soc_ret           fail_cause; /*!< Failure cause when type is SOC_PING_CALLBACK_FAILURE*/
} qcm_ping_qt2131_result_t;

//The platform supports a single ping instance and its callback carries no user data.
static qosa_int32_t  g_qt2131_ping_session_id = -1;   /*!< Session id owning the platform round, -1 if none or released*/
static qosa_uint16_t g_qt2131_ping_remain = 0;        /*!< Results still expected from the platform round*/
static qosa_mutex_t  g_qt2131_ping_mutex = QOSA_NULL; /*!< Protects g_qt2131_ping_session_id and g_qt2131_ping_remain*/

static qcm_ping_error_e qcm_ping_qt2131_err_map(soc_ret ret)
{
    QLOGV("ret: %x", ret);
    switch (ret)
    {
        case SOC_RET_ERROR_IN_PARAMETERS:
            return QCM_PING_ERR_INVALID_PARAM;
        case SOC_RET_MEMORY_ERROR:
            return QCM_PING_ERR_MEMORY;
        case SOC_RET_IN_PROGRESS:
            return QCM_PING_ERR_OPERATION_NOT_ALLOWED;
        case SOC_RET_CID_NOT_DEFINED:
        case SOC_RET_CID_NOT_ACTIVE:
        case SOC_RET_NO_NETWORK_SERVICE:
            return QCM_PING_ERR_PDP_ACTIVE_FAILURE;
        default:
            return QCM_PING_ERR_SOCKET_WRITE_FAILURE;
    }
}

static void qcm_ping_qt2131_release_session_id(qosa_uint32_t session_id)
{
    qosa_mutex_lock(g_qt2131_ping_mutex, QOSA_WAIT_FOREVER);
    //Keep g_qt2131_ping_remain so that a still running platform round keeps blocking new requests
    if (g_qt2131_ping_session_id == (qosa_int32_t)session_id)
    {
        g_qt2131_ping_session_id = -1;
    }
    qosa_mutex_unlock(g_qt2131_ping_mutex);
}

static void qt2131_ping_result_callback(const char *host, ip_addr_t *ip_addr, soc_ping_callback type, soc_ping_result *ping_result)
{
    qosa_int32_t              session_id = -1;
    qcm_ping_session_t       *ping_session = QOSA_NULL;
    qcm_ping_qt2131_result_t *res_ptr = QOSA_NULL;
    qcm_ping_task_msg_t       ping_msg = {0};

    QOSA_UNUSED(host);
    QOSA_UNUSED(ip_addr);

    QLOGV("type: %d", type);

    //The summary is computed by the ping task itself
    if (type == SOC_PING_CALLBACK_AVERAGE)
    {
        return;
    }

    //Hold the lock until the result is posted, so qcm_ping_free_session cannot free the session in between
    qosa_mutex_lock(g_qt2131_ping_mutex, QOSA_WAIT_FOREVER);
    if (g_qt2131_ping_remain == 0)
    {
        goto exit;
    }
    session_id = g_qt2131_ping_session_id;

    if (type == SOC_PING_CALLBACK_FAILURE)
    {
        g_qt2131_ping_remain = 0;
    }
    else
    {
        g_qt2131_ping_remain = g_qt2131_ping_remain - 1;
    }

    if (g_qt2131_ping_remain == 0)
    {
        g_qt2131_ping_session_id = -1;
    }

    QLOGV("type=%d session_id=%d", type, session_id);
    if (session_id < 0)
    {
        goto exit;
    }
    ping_session = qcm_ping_get_session_ptr((qosa_uint8_t)session_id);
    if (ping_session == QOSA_NULL)
    {
        goto exit;
    }

    res_ptr = (qcm_ping_qt2131_result_t *)qosa_malloc(sizeof(qcm_ping_qt2131_result_t));
    if (res_ptr == QOSA_NULL)
    {
        QLOGE("malloc failed");
        goto exit;
    }
    qosa_memset(res_ptr, 0, sizeof(qcm_ping_qt2131_result_t));
    res_ptr->session_id = (qosa_uint8_t)session_id;
    res_ptr->type = type;
    if (ping_result != QOSA_NULL)
    {
        if (type == SOC_PING_CALLBACK_OK)
        {
            res_ptr->ttl = ping_result->ping_result_ok.ttl;
            res_ptr->rtt = ping_result->ping_result_ok.rtt;
        }
        else if (type == SOC_PING_CALLBACK_FAILURE)
        {
            res_ptr->fail_cause = ping_result->ping_fail_cause;
        }
    }

    ping_msg.msgid = QCM_PING_PLATFORM_RESULT_IND;
    ping_msg.argv = (void *)res_ptr;
    if (qosa_msgq_release(ping_session->ping_task_msgq, sizeof(qcm_ping_task_msg_t), (qosa_uint8_t *)&ping_msg, QOSA_NO_WAIT) != 0)
    {
        QLOGE("msgq release failed");
        qosa_free(res_ptr);
    }

exit:
    qosa_mutex_unlock(g_qt2131_ping_mutex);
}

/**
 * @brief Start a platform ping round, the platform limits one round to EXT_NPING_MAX_COUNT packets.
 */
static qcm_ping_error_e qcm_ping_qt2131_start_round(qcm_ping_session_t *ping_session)
{
    soc_ret       ret = SOC_RET_OK;
    qosa_uint32_t retry = 0;
    qosa_uint32_t timeout_ms = ping_session->user_opts.ping_response_time_out * 1000;
    qosa_uint32_t remain = ping_session->user_opts.num_pings - ping_session->summary_info.num_pkts_sent;
    qosa_uint16_t count = (remain > EXT_NPING_MAX_COUNT) ? EXT_NPING_MAX_COUNT : (qosa_uint16_t)remain;
    char          ip_str[CONFIG_QOSA_INET6_ADDRSTRLEN] = {0};

    if (ping_session->user_opts.num_data_bytes > PING_IPV4_MAX_VAL)
    {
        return QCM_PING_ERR_INVALID_PARAM;
    }

    if (ping_session->remote_ip.ip_vsn == QOSA_PDP_IPV4)
    {
        qcm_inet_ntop(QCM_AF_INET, &ping_session->remote_ip.addr.ipv4_addr, ip_str, CONFIG_QOSA_INET6_ADDRSTRLEN);
    }
    else
    {
        qcm_inet_ntop(QCM_AF_INET6, &ping_session->remote_ip.addr.ipv6_addr, ip_str, CONFIG_QOSA_INET6_ADDRSTRLEN);
    }

    qosa_mutex_lock(g_qt2131_ping_mutex, QOSA_WAIT_FOREVER);
    if (g_qt2131_ping_remain != 0)
    {
        qosa_mutex_unlock(g_qt2131_ping_mutex);
        QLOGE("platform ping busy");
        return QCM_PING_ERR_OPERATION_NOT_ALLOWED;
    }
    //Claim ownership before sending, the platform may call back before uapi_icmp_ping_send returns
    g_qt2131_ping_session_id = (qosa_int32_t)ping_session->session_id;
    g_qt2131_ping_remain = count;
    qosa_mutex_unlock(g_qt2131_ping_mutex);
    ping_session->round_remain = count;

    do {
        //QuecOS cid starts from 1, platform cid starts from 0
        ret = uapi_icmp_ping_send(
            ip_str,
            (uint16_t)ping_session->user_opts.num_data_bytes,
            timeout_ms,
            (uint8_t)(ping_session->pdp_cid - 1),
            count,
            qt2131_ping_result_callback
        );
        QLOGV("ret=%x", ret);
        if (ret != SOC_RET_IN_PROGRESS)
        {
            break;
        }
        //The platform frees its context only after the last callback of the previous round returns
        qosa_task_sleep_ms(QCM_PING_QT2131_BUSY_RETRY_MS);
        retry++;
    } while (retry < QCM_PING_QT2131_BUSY_RETRY_CNT);

    QLOGV("ret=%x ip=%s count=%d retry=%d", ret, ip_str, count, retry);
    if (ret != SOC_RET_OK)
    {
        qosa_mutex_lock(g_qt2131_ping_mutex, QOSA_WAIT_FOREVER);
        g_qt2131_ping_session_id = -1;
        g_qt2131_ping_remain = 0;
        qosa_mutex_unlock(g_qt2131_ping_mutex);
        ping_session->round_remain = 0;
        return qcm_ping_qt2131_err_map(ret);
    }

    // After uapi_icmp_ping_send is successful, the ping_response_timer included in the session is activated.
    if (qosa_timer_start(ping_session->ping_response_timer, (timeout_ms * count) + QCM_PING_QT2131_GUARD_MS, QOSA_FALSE) != QOSA_OK)
    {
        QLOGE("guard timer start failed");
    }
    return QCM_PING_OK;
}

/**
 * @brief Process a per-packet result reported by the platform ping interface
 *
 * @param[in] void * argv
 *          - Corresponds to a pointer of type qcm_ping_qt2131_result_t structure
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK to continue, QCM_PING_SEND_END when all pings are done
 *       - Returns other on failure
 */
static qcm_ping_error_e qcm_ping_qt2131_result_handler(void *argv)
{
    qcm_ping_qt2131_result_t *res_ptr = (qcm_ping_qt2131_result_t *)argv;
    qcm_ping_session_t       *ping_session = QOSA_NULL;
    qcm_ping_stats_type       ping_stats;
    qcm_ping_error_e          result = QCM_PING_OK;

    ping_session = qcm_ping_get_session_ptr(res_ptr->session_id);
    if (ping_session == QOSA_NULL)
    {
        result = QCM_PING_ERR_INVALID_PARAM;
        goto exit;
    }

    if (res_ptr->type == SOC_PING_CALLBACK_FAILURE)
    {
        QLOGE("platform ping failed, cause=%x", res_ptr->fail_cause);
        ping_session->round_remain = 0;
        result = qcm_ping_qt2131_err_map(res_ptr->fail_cause);
        goto exit;
    }

    ping_session->summary_info.num_pkts_sent++;
    if (res_ptr->type == SOC_PING_CALLBACK_OK)
    {
        ping_session->summary_info.num_pkts_recvd++;
        if (ping_session->summary_info.max_rtt <= 0 || ping_session->summary_info.max_rtt < res_ptr->rtt)
        {
            ping_session->summary_info.max_rtt = res_ptr->rtt;
        }
        if (ping_session->summary_info.min_rtt <= 0 || ping_session->summary_info.min_rtt > res_ptr->rtt)
        {
            ping_session->summary_info.min_rtt = res_ptr->rtt;
        }
        ping_session->summary_info.avg_rtt = (qosa_uint32_t)((qosa_int32_t)ping_session->summary_info.avg_rtt + ((qosa_int32_t)res_ptr->rtt - (qosa_int32_t)ping_session->summary_info.avg_rtt) / (qosa_int32_t)ping_session->summary_info.num_pkts_recvd);

        qosa_memset(&ping_stats, 0, sizeof(ping_stats));
        ping_stats.ping_rtt = res_ptr->rtt;
        ping_stats.ping_size = ping_session->user_opts.num_data_bytes;
        ping_stats.ping_ttl = res_ptr->ttl;
        if (ping_session->remote_ip.ip_vsn == QOSA_PDP_IPV4)
        {
            qcm_inet_ntop(QCM_AF_INET, &ping_session->remote_ip.addr.ipv4_addr, ping_stats.resolved_ip_addr, CONFIG_QOSA_INET6_ADDRSTRLEN);
        }
        else
        {
            qcm_inet_ntop(QCM_AF_INET6, &ping_session->remote_ip.addr.ipv6_addr, ping_stats.resolved_ip_addr, CONFIG_QOSA_INET6_ADDRSTRLEN);
        }
        qcm_ping_info_event_cb(ping_session, QCM_PING_STATS, QCM_PING_OK, &ping_stats);
    }
    else
    {
        ping_session->summary_info.num_pkts_lost++;
    }

    QLOGV("sent=%d recvd=%d lost=%d", ping_session->summary_info.num_pkts_sent, ping_session->summary_info.num_pkts_recvd, ping_session->summary_info.num_pkts_lost);
    if (ping_session->round_remain > 0)
    {
        ping_session->round_remain--;
    }
    if (ping_session->round_remain == 0)
    {
        if (qosa_timer_is_running(ping_session->ping_response_timer))
        {
            qosa_timer_stop(ping_session->ping_response_timer);
        }
        if (ping_session->summary_info.num_pkts_sent < ping_session->user_opts.num_pings)
        {
            result = qcm_ping_qt2131_start_round(ping_session);
        }
        else
        {
            //Same as the raw socket implementation: all packets lost ends with a timeout error
            if (0 == ping_session->summary_info.num_pkts_recvd)
            {
                result = QCM_PING_ERR_TIMEOUT;
            }
            else
            {
                result = QCM_PING_SEND_END;
            }
        }
    }

exit:
    qosa_free(res_ptr);
    return result;
}

/**
 * @brief Check the PDP and start the first platform ping round.
 *
 * @param[in] void * argv
 *          - Corresponds to a pointer of type qcm_ping_req_session_id_t structure
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK on success
 *       - Returns other on failure
 */
static qcm_ping_error_e qcm_ping_send_icmp_request(void *argv)
{
    qcm_ping_error_e           result = QCM_PING_OK;
    qcm_ping_session_t        *ping_session = QOSA_NULL;
    qcm_ping_req_session_id_t *send_ptr = QOSA_NULL;
    qosa_datacall_ip_info_t    info = {0};
    qosa_datacall_conn_t       conn = 0;

    send_ptr = (qcm_ping_req_session_id_t *)argv;
    if (send_ptr == QOSA_NULL)
    {
        QLOGE("INVALID PARAM");
        result = QCM_PING_ERR_INVALID_PARAM;
        goto exit;
    }
    ping_session = qcm_ping_get_session_ptr(send_ptr->ping_session_id);
    qosa_free(send_ptr);
    if (ping_session == QOSA_NULL)
    {
        QLOGE("INVALID PARAM");
        result = QCM_PING_ERR_INVALID_PARAM;
        goto exit;
    }

    // get the local address
    conn = qosa_datacall_conn_new(ping_session->sim_id, ping_session->pdp_cid, QOSA_DATACALL_CONN_TCPIP);
    if (QOSA_DATACALL_OK != qosa_datacall_get_ip_info(conn, &info))
    {
        QLOGE("...result=%d simid=%d pdpcid=%d", result, ping_session->sim_id, ping_session->pdp_cid);
        result =  QCM_PING_ERR_PDP_ACTIVE_FAILURE;
        goto exit;
    }

    result = qcm_ping_qt2131_start_round(ping_session);

exit:
    return result;
}
#else  /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */

/**
 * @brief ICMP packet assembly and transmission function, used for ICMP self-packet assembly, socket creation and connection, and sending.
 *
 * @param[in] void * argv
 *          - Corresponds to a pointer of type qcm_ping_req_session_id_t structure
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK on success
 *       - Returns other on failure
 */
static qcm_ping_error_e qcm_ping_send_icmp_request(void *argv)
{
    qcm_ping_error_e           result = QCM_PING_OK;
    qosa_int32_t               ret = QCM_PING_OK;
    char                       out_buf[256] = {0};
    qcm_ping_session_t        *ping_session = QOSA_NULL;
    qcm_ping_req_session_id_t *send_ptr = QOSA_NULL;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
    qosa_datacall_ip_info_t     info = {0};
    qosa_datacall_conn_t        conn = 0;
#endif

    send_ptr = (qcm_ping_req_session_id_t *)argv;
    if (send_ptr == QOSA_NULL)
    {
        result = QCM_PING_ERR_INVALID_PARAM;
        goto exit;
    }
    ping_session = qcm_ping_get_session_ptr(send_ptr->ping_session_id);
    qosa_free(send_ptr);
    if (ping_session == QOSA_NULL)
    {
        result = QCM_PING_ERR_INVALID_PARAM;
        goto exit;
    }

    QLOGV("sock_hd :%d", ping_session->sock_hd);

    if (ping_session->sock_hd == 0)
    {
        qosa_int32_t ttl = ping_session->user_opts.ttl;

        //Create socket with port number 0
        ping_session->sock_hd = qcm_socket_create(
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
            ping_session->sim_id,
            ping_session->pdp_cid,
#endif
            ping_session->ai_family,
            QCM_SOCK_RAW,
            ping_session->ai_family == QCM_AF_INET ? QCM_IP_PROTOCOL_ICMP : QCM_IP6_NEXTH_ICMP6,
            0,
            QOSA_FALSE
        );
        QLOGV("qcm_socket_create    ping_session->sock_hd %d", ping_session->sock_hd);
        if (ping_session->sock_hd <= 0)
        {
            result = QCM_PING_ERR_SOCKET_NEW_FAILURE;
            if (ping_session->sock_hd == QCM_SOCK_INVALID_PARAM)
                result = QCM_PING_ERR_DNS_FAILURE;
            goto exit;
        }

        qcm_socket_register_event(
            ping_session->sock_hd,
            QCM_SOCK_CLOSE_EVENT | QCM_SOCK_READ_EVENT | QCM_SOCK_WRITE_EVENT,
            qcm_ping_socket_event_cb,
            (void *)ping_session
        );

        if (qcm_socket_connect(ping_session->sock_hd, &ping_session->remote_ip, 0) != 0)
        {
            QLOGE("qcm_socket_connect failed");
            result = QCM_PING_ERR_SOCKET_CONNECT_FAILURE;
            goto exit;
        }
        qcm_socket_set_opt(ping_session->sock_hd, QCM_IP_TTL_OPT, (void *)&ttl);
    }

    *((qosa_uint32_t *)(out_buf + 8)) = qcm_get_ping_timestamp_ms();  //Time acquisition interface replacement
    *((qosa_uint32_t *)(out_buf + 12)) = ping_session->session_id;
    QLOGV("icmp request send at :%d", *((qosa_uint32_t *)(out_buf + 8)));
    QLOGV("icmp request send session_id :%d", *((qosa_uint32_t *)(out_buf + 12)));

#if 0
    for(i = 0 ; i < (ping_session->user_opts.num_data_bytes - 16);i++)
    {
        out_buf[i + 16] = i;
    }
#endif

    // get the local address
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
    conn = qosa_datacall_conn_new(ping_session->sim_id, ping_session->pdp_cid, QOSA_DATACALL_CONN_TCPIP);
    if (QOSA_DATACALL_OK != qosa_datacall_get_ip_info(conn, &info))
    {
        QLOGE("...result=%d simid=%d pdpcid=%d", result, ping_session->sim_id, ping_session->pdp_cid);
        return QCM_PING_ERR_PDP_ACTIVE_FAILURE;
    }
#endif
    //Assemble ICMP packet
    if (ping_session->ai_family == QCM_AF_INET)
    {
        struct qcm_icmp_echo_hdr *idur = QOSA_NULL;
        idur = (struct qcm_icmp_echo_hdr *)out_buf;

        QCM_ICMPH_CODE_SET(idur, 0);  //Zero out the structure's code location
        idur->id = QCM_PING_HTONS((++ping_session->icmp_echo_msg_id));
        idur->seqno = QCM_PING_HTONS((++ping_session->icmp_echo_req_seq_num));

        QCM_ICMPH_TYPE_SET(idur, QCM_ICMP_ECHO);  //Set the struct type position to 8, request bit
        idur->chksum = QCM_PING_CHKSUM(idur, ping_session->user_opts.num_data_bytes);
    }
    else
    {
        struct qcm_icmp6_echo_hdr *idur6 = QOSA_NULL;
        idur6 = (struct qcm_icmp6_echo_hdr *)out_buf;
        QCM_ICMPH_CODE_SET(idur6, 0);
        idur6->id = QCM_PING_HTONS(++(ping_session->icmp_echo_msg_id));
        idur6->seqno = QCM_PING_HTONS(++(ping_session->icmp_echo_req_seq_num));

        idur6->chksum = 0;

        QCM_ICMPH_TYPE_SET(idur6, QCM_ICMP6_TYPE_EREQ);  //IPv6 request bit

#ifdef CONFIG_QOSA_ASR160X_PLATFORM_FUNC
        /* Calculate ICMPv6 checksum with pseudo-header */
        idur6->chksum = QCM_PING_IP6_CHKSUM(idur6,
            ping_session->user_opts.num_data_bytes,
            &info.ipv6_ip.apptcpip_ipv6_addr,
            &ping_session->remote_ip.apptcpip_ipv6_addr);
#endif /* CONFIG_QOSA_ASR160X_PLATFORM_FUNC */
    }
    ret = qcm_socket_sendto(ping_session->sock_hd, &ping_session->remote_ip, 0, out_buf, ping_session->user_opts.num_data_bytes);
    //Ensure the number of bytes sent matches the user-configured byte count.
    QLOGV("sendto byte :%d", ret);
    if (ret > 0 && (qosa_uint32_t)ret == ping_session->user_opts.num_data_bytes)
    {
        ping_session->summary_info.num_pkts_sent++;
        qosa_bool_t start = qosa_timer_is_running(ping_session->ping_response_timer);
        if (start)
        {
            qosa_timer_stop(ping_session->ping_response_timer);
        }
        //Start a one-shot timer
        ret = qosa_timer_start(ping_session->ping_response_timer, ping_session->user_opts.ping_response_time_out * 1000, 0);
        if (ret != QCM_PING_OK)
        {
            result = QCM_PING_ERR_UNKNOW;
            QLOGE("timer start failed");
            goto exit;
        }
    }
    else
    {
        result = QCM_PING_ERR_SOCKET_WRITE_FAILURE;
        goto exit;
    }

exit:
    return result;
}
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */

/**
 * @brief Timeout handling function: after sending an ICMP packet, start a timer; upon timeout, execute the timeout event, report a URC, and if the number of transmissions is insufficient, resend.
 *
 * @param[in] void * argv
 *          - Corresponds to a pointer of type qcm_ping_req_session_id_t structure
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK on success
 *       - Returns other on failure
 */
static qcm_ping_error_e qcm_ping_timeout_send_request(void *argv)
{
    qcm_ping_error_e           result = QCM_PING_OK;
    qcm_ping_session_t        *ping_session = QOSA_NULL;
    qcm_ping_req_session_id_t *send_ptr = QOSA_NULL;
    qosa_uint8_t               ping_session_id = 0;

    send_ptr = (qcm_ping_req_session_id_t *)argv;
    if (send_ptr == QOSA_NULL)
    {
        result = QCM_PING_ERR_INVALID_PARAM;
        return result;
    }
    ping_session = qcm_ping_get_session_ptr(send_ptr->ping_session_id);
    ping_session_id = send_ptr->ping_session_id;
    qosa_free(send_ptr);

    if (ping_session == QOSA_NULL || ping_session->ping_task == QOSA_NULL)
    {
        result = QCM_PING_ERR_INVALID_PARAM;
        return result;
    }
#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
    QOSA_UNUSED(ping_session_id);
    //Stale expiry queued before the round finished or the timer was restarted for the next round
    if (ping_session->round_remain == 0 || qosa_timer_is_running(ping_session->ping_response_timer))
    {
        // If round_remain is equal == 0, or if the timer is currently running (indicating that a new round has been restarted), this timeout message is a leftover from the previous round and should be ignored directly.
        QLOGV("Ignore the information from the previous round.");
        return QCM_PING_OK;
    }
    //Platform results that never arrived are counted as lost; the global busy state is left to the platform callbacks
    QLOGE("platform ping round timeout, remain=%d", ping_session->round_remain);
    //For all the packages that have not been reported by the platform in this round, they shall all be regarded as "sent but no response received".
    ping_session->summary_info.num_pkts_sent += ping_session->round_remain;
    ping_session->summary_info.num_pkts_lost += ping_session->round_remain;
    ping_session->round_remain = 0;
    result = QCM_PING_ERR_TIMEOUT;
#else  /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */
    //After timeout, the number of lost packets +1
    ping_session->summary_info.num_pkts_lost++;
    if (ping_session->summary_info.num_pkts_recvd > 0)
    {
        //If a success has already been reported, then all subsequent timeouts must report the timeout URC.
        //If no errors are reported, and if all fail, then directly report one error in the summary.
        //2024-08-16 17:38:44 FAE-96197 URC content reported when all errors are closed
        //qcm_ping_info_event_cb(ping_session, QCM_PING_STATS, QCM_PING_ERR_TIMEOUT, QOSA_NULL);
    }

    //If the number of sends is less than the user-configured count, send again.
    if (ping_session->summary_info.num_pkts_sent < ping_session->user_opts.num_pings)
    {
        QLOGV("icmp=%d,%d", ping_session->summary_info.num_pkts_sent, ping_session->user_opts.num_pings);
        send_ptr = qosa_malloc(sizeof(qcm_ping_req_session_id_t));
        if (send_ptr == QOSA_NULL)
        {
            result = QCM_PING_ERR_MEMORY;
            return result;
        }
        send_ptr->ping_session_id = ping_session_id;
        result = qcm_ping_send_icmp_request(send_ptr);
        return result;
    }

    //If the number of received packets plus the number of dropped packets is greater than or equal to the number of sent packets timeout, as long as entering this function, lost increments and is not zero.
    if (ping_session && (ping_session->summary_info.num_pkts_recvd + ping_session->summary_info.num_pkts_lost) >= ping_session->summary_info.num_pkts_sent)
    {
        QLOGE("timeout");
        result = QCM_PING_ERR_TIMEOUT;
    }
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */
    return result;
}

/**
 * @brief Process DNS resolution results
 *
 * @param[in] void * argv
 *          - Corresponds to a pointer of type qcm_ping_dns_resp_t structure
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK on success
 *       - Returns other on failure
 */
static qcm_ping_error_e qcm_ping_dns_response_handler(void *argv)
{
    qcm_ping_error_e        result = QCM_PING_OK;
    qcm_ping_dns_resp_t    *dns_ptr = (qcm_ping_dns_resp_t *)argv;
    qcm_ping_session_t     *ping_session = QOSA_NULL;
    struct qosa_addrinfo_s *addrinfo_ptr = QOSA_NULL;
    qosa_uint8_t            want_family = 0;

    ping_session = qcm_ping_get_session_ptr(dns_ptr->session_id);
    if (ping_session == QOSA_NULL)
    {
        result = QCM_PING_ERR_INVALID_PARAM;
        goto exit;
    }

    if (dns_ptr->info == QOSA_NULL || dns_ptr->status != QOSA_DNS_RESULT_OK)
    {
        result = QCM_PING_ERR_DNS_FAILURE;
    }
    else
    {
        //DNS resolution successful
        //dns_ping_type: -1 - use the first result; 0 - select the IPv4 result; 1 - select the IPv6 result
        addrinfo_ptr = dns_ptr->info;
        if (ping_session->user_opts.dns_ping_type != -1)
        {
            want_family = (ping_session->user_opts.dns_ping_type == 0) ? QCM_AF_INET : QCM_AF_INET6;
            while (addrinfo_ptr != QOSA_NULL && addrinfo_ptr->ai_family != want_family)
            {
                addrinfo_ptr = addrinfo_ptr->ai_next;
            }
            if (addrinfo_ptr == QOSA_NULL)
            {
                //No address matching the requested family exists, fall back to the first DNS result.
                QLOGW("no matched addr family, dns_ping_type=%d, use the first result", ping_session->user_opts.dns_ping_type);
                addrinfo_ptr = dns_ptr->info;
            }
        }

        if (addrinfo_ptr->ai_family == QCM_AF_INET)
        {
            inet_pton(QCM_AF_INET, addrinfo_ptr->ip_addr, &ping_session->remote_ip.addr.ipv4_addr);
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
            ping_session->remote_ip.ip_vsn = QOSA_IP_V4;
#else
            ping_session->remote_ip.ip_vsn = QOSA_PDP_IPV4;
#endif     
        }
        else
        {
            inet_pton(QCM_AF_INET6, addrinfo_ptr->ip_addr, &ping_session->remote_ip.addr.ipv6_addr);
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
            ping_session->remote_ip.ip_vsn = QOSA_IP_V6;
#else
            ping_session->remote_ip.ip_vsn = QOSA_PDP_IPV6;
#endif
        }
        ping_session->ai_family = addrinfo_ptr->ai_family;

        qosa_dns_result_free(dns_ptr->info);
        qcm_ping_req_event(QCM_PING_SEND_ICMP_REQUEST, ping_session);
    }

exit:
    qosa_free(dns_ptr);

    return result;
}

/**
 * @brief Processing PDP activation result
 *
 * @param[in] void * argv
 *          - Corresponds to a pointer of type qcm_ping_dns_resp_t structure
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK on success
 *       - Returns other on failure
 */
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   

static qcm_ping_error_e qcm_ping_datacall_response_handler(void *argv)
{
    qcm_ping_error_e     result = QCM_PING_OK;
    qcm_ping_pdp_resp_t *pdp_resp_ptr = (qcm_ping_pdp_resp_t *)argv;
    qcm_ping_session_t  *ping_session = QOSA_NULL;
    int                  ip_info = 0;

    ping_session = qcm_ping_get_session_ptr(pdp_resp_ptr->ping_session_id);
    if (ping_session == QOSA_NULL)
    {
        result = QCM_PING_ERR_INVALID_PARAM;
        goto exit;
    }

    if (pdp_resp_ptr->act_info.opt == QOSA_PDP_OPT_ACTIVE)
    {
        if (pdp_resp_ptr->act_info.status == QOSA_TRUE)
        {
            ip_info = qcm_socket_checkip_is_ip46(ping_session->host_name);
            if (ip_info == 0 || (ip_info == QCM_AF_INET && pdp_resp_ptr->act_info.ip_info.ip_type != QOSA_PDP_IPV6)
                || (ip_info == QCM_AF_INET6 && pdp_resp_ptr->act_info.ip_info.ip_type != QOSA_PDP_IPV4))
            {
                //Activation successful, proceed to the next step of DNS resolution.
                qcm_ping_req_event(QCM_PING_DNS_PARSE_REQ, ping_session);
            }
            else
            {
                result = QCM_PING_ERR_OPERATION_NOT_ALLOWED;
            }
        }
        else
        {
            //Activation failed, return directly
            result = QCM_PING_ERR_PDP_ACTIVE_FAILURE;
        }
    }
exit:
    qosa_free(pdp_resp_ptr);
    return result;
}
#endif
/**
 * @brief ICMP reply function, called when data arrives from the peer server to parse the data, calculate TTL and RTT, and report URC, etc.
 *
 * @param[in] void * argv
 *          - Corresponds to a pointer of type qcm_ping_req_session_id_t structure
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK on success
 *       - Returns other on failure
 */
static qcm_ping_error_e qcm_ping_read_icmp_reply(void *argv)
{
    qosa_uint8_t               in_buf[QOSA_ARRAY_BYTE_256 + 1] = {0};
    qosa_uint8_t              *icmp_buf = QOSA_NULL;
    qosa_int32_t               ret = QCM_PING_OK;
    qosa_uint16_t              iphdr_hlen = 0;
    qosa_uint32_t              icmp_request_send_time = 0;
    qosa_uint32_t              session_id = 0;
    qosa_uint32_t              rtt = 0;
    qcm_ping_stats_type        ping_stats;
    qosa_int32_t               reply_type = 0;
    qosa_int32_t               reply_id = 0;
    qosa_int32_t               reply_seqno = 0;
    qcm_ping_error_e           result = QCM_PING_OK;
    qcm_ping_session_t        *ping_session = QOSA_NULL;
    qcm_ping_req_session_id_t *send_ptr = QOSA_NULL;

    send_ptr = (qcm_ping_req_session_id_t *)argv;
    ping_session = qcm_ping_get_session_ptr(send_ptr->ping_session_id);
    qosa_free(send_ptr);
    send_ptr = QOSA_NULL;
    if (ping_session == QOSA_NULL)
    {
        result = QCM_PING_ERR_INVALID_PARAM;
        goto exit;
    }

    QLOGV("enter");
    if (ping_session == QOSA_NULL || ping_session->sock_hd == 0)
    {
        result = QCM_PING_ERR_INVALID_PARAM;
        goto exit;
    }
    //Receive remote echo messages, UDP reception
    ret = qcm_socket_recvfrom(ping_session->sock_hd, QOSA_NULL, QOSA_NULL, (char *)in_buf, QOSA_ARRAY_BYTE_256);
    if (ret <= 0)
    {
        result = (QCM_SOCK_WODBLOCK != ret) ? QCM_PING_ERR_SOCKET_READ_FAILURE : QCM_PING_OK;
        goto exit;
    }
    QLOGV("recv %d bytes", ret);
    if (ping_session->ai_family == QCM_AF_INET)
    {
        struct qcm_icmp_echo_hdr *idur = QOSA_NULL;
#ifndef CONFIG_QOSA_QCX216_PLATFORM_FUNC
        struct qcm_ip_hdr *iphdr = QOSA_NULL;

        iphdr = (struct qcm_ip_hdr *)in_buf;

        /********************************************************************************/
        /*IP Version (4b)  *   IP header length (4b) *          Type of Service (8b)*/
        /********************************************************************************/
        //Calculate IP header length
        iphdr_hlen = QCM_IPH_HL(iphdr);
        iphdr_hlen *= 4;  //Normally it is 20 bytes, with a length that is a multiple of 4.

        //If the received length is less than the IP header length, or if the received length minus the IP header length is less than 16 bytes, it carries its own session_id + time.
        if (ret <= iphdr_hlen || (ret - iphdr_hlen) < 16)
        {
            result = QCM_PING_ERR_SOCKET_READ_FAILURE;
            goto exit;
        }

        //After offsetting the IP header, the ICMP header follows directly. Cast the ICMP header to the specified structure type.
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC */

        icmp_buf = (in_buf + iphdr_hlen);
        idur = (struct qcm_icmp_echo_hdr *)(icmp_buf);
        //Retrieve time
        qosa_memcpy(&icmp_request_send_time, (icmp_buf + 8), 4);
        //Retrieve session_id
        qosa_memcpy(&session_id, (icmp_buf + 12), 4);
        reply_type = idur->type;
        reply_id = QCM_PING_NTOHS(idur->id);
        reply_seqno = QCM_PING_NTOHS(idur->seqno);
    }
    else
    {
        struct qcm_icmp6_echo_hdr *idur6 = QOSA_NULL;
#ifndef CONFIG_QOSA_QCX216_PLATFORM_FUNC
        struct qcm_ip6_hdr *ip6hdr = QOSA_NULL;
        qosa_uint16_t       hlen;
        qosa_uint8_t        nexth;
        qosa_uint8_t       *temp_buf = in_buf;

        ip6hdr = (struct qcm_ip6_hdr *)(in_buf);

        iphdr_hlen = QCM_IP6_HLEN;
        nexth = QCM_IP6H_NEXTH(ip6hdr);
        QLOGV("nexth:%d", nexth);
        while (nexth != QCM_IP6_NEXTH_NONE)
        {
            QLOGV("nexth:%d", nexth);
            //Loop through the IPv6 header content to find the IP6_NEXTH_ICMP6 header field by offset.
            if (nexth == QCM_IP6_NEXTH_HOPBYHOP || nexth == QCM_IP6_NEXTH_DESTOPTS || nexth == QCM_IP6_NEXTH_ROUTING)
            {
                hlen = 8 * (1 + *((qosa_uint8_t *)temp_buf + 1));
                temp_buf += iphdr_hlen;
                nexth = *((qosa_uint8_t *)temp_buf);
                iphdr_hlen += hlen;
            }
            else if (nexth == QCM_IP6_NEXTH_FRAGMENT)
            {
                struct qcm_ip6_frag_hdr *frag_hdr;
                hlen = 8;
                frag_hdr = (struct qcm_ip6_frag_hdr *)temp_buf;

                nexth = frag_hdr->_nexth;
                if ((frag_hdr->_fragment_offset & QCM_PING_HTONS(QCM_IP6_FRAG_OFFSET_MASK | QCM_IP6_FRAG_MORE_FLAG)) == 0)
                {
                    iphdr_hlen += hlen;
                    temp_buf += iphdr_hlen;
                }
                else
                {
                    //TODO:
                }
            }
            else if (nexth == QCM_IP6_NEXTH_ICMP6)
            {
                break;  //Find it, then break, and then bring out the data from the head.
            }
            else
            {
                result = QCM_PING_ERR_SOCKET_READ_FAILURE;
                goto exit;  //If this entire header does not find IP6_NEXTH_ICMP6, error flow
            }
        }
        QLOGV("iphdr_hlen: %d", iphdr_hlen);
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC */

        icmp_buf = (in_buf + iphdr_hlen);

        idur6 = (struct qcm_icmp6_echo_hdr *)(icmp_buf);

        qosa_memcpy(&icmp_request_send_time, (icmp_buf + 8), 4);
        qosa_memcpy(&session_id, (icmp_buf + 12), 4);
        reply_type = idur6->type;
        reply_id = QCM_PING_NTOHS(idur6->id);
        reply_seqno = QCM_PING_NTOHS(idur6->seqno);
    }

    QLOGV("reply_type: %d", reply_type);
    QLOGV("reply_id:%d, icmp_echo_msg_id:%d ", reply_id, ping_session->icmp_echo_msg_id);
    QLOGV("reply_seqno: %d,seq_num:%d", reply_seqno, ping_session->icmp_echo_req_seq_num);
    QLOGV("session_id:%d, session_id:%d", session_id, ping_session->session_id);

    //Confirm that the received reply message is sent by this device.
    if (!(reply_type == QCM_ICMP_ER || reply_type == QCM_ICMP6_TYPE_EREP) || reply_id != ping_session->icmp_echo_msg_id
        || reply_seqno != ping_session->icmp_echo_req_seq_num || session_id != ping_session->session_id)
    {
        result = QCM_PING_OK;
        goto exit;
    }
    if (qosa_timer_is_running(ping_session->ping_response_timer))
    {
        qosa_timer_stop(ping_session->ping_response_timer);
    }

    //rtt concept: time from when the sender transmits until the sender receives the reply acknowledgment message
    rtt = qcm_get_ping_timestamp_ms() - icmp_request_send_time;
    QLOGV("reply time=%x", icmp_request_send_time);
    QLOGV("reply dump:id:%d,seqno:%d,rtt:%d", reply_id, reply_seqno, rtt);
    qosa_memset(&ping_stats, 0, sizeof(ping_stats));
    //Save the reply message to ping_stats
    ping_stats.ping_rtt = rtt;
    ping_stats.ping_size = ping_session->user_opts.num_data_bytes;
    ping_stats.ping_ttl = ping_session->user_opts.ttl;
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    if (ping_session->remote_ip.ip_vsn == QOSA_IP_V4)
#else
    if (ping_session->remote_ip.ip_vsn == QOSA_PDP_IPV4)
#endif
    {
        qcm_inet_ntop(QCM_AF_INET, &ping_session->remote_ip.addr.ipv4_addr, ping_stats.resolved_ip_addr, CONFIG_QOSA_INET6_ADDRSTRLEN);
    }
    else
    {
        qcm_inet_ntop(QCM_AF_INET6, &ping_session->remote_ip.addr.ipv6_addr, ping_stats.resolved_ip_addr, CONFIG_QOSA_INET6_ADDRSTRLEN);
    }

    ping_session->summary_info.num_pkts_recvd++;

    //Find the longest RTT latency
    if (ping_session->summary_info.max_rtt <= 0 || ping_session->summary_info.max_rtt < rtt)
    {
        ping_session->summary_info.max_rtt = rtt;
    }

    //Find the shortest RTT latency
    if (ping_session->summary_info.min_rtt <= 0 || ping_session->summary_info.min_rtt > rtt)
    {
        ping_session->summary_info.min_rtt = rtt;
    }

    ping_session->summary_info.avg_rtt = (qosa_uint32_t)((qosa_int32_t)ping_session->summary_info.avg_rtt + ((qosa_int32_t)rtt - (qosa_int32_t)ping_session->summary_info.avg_rtt) / (qosa_int32_t)ping_session->summary_info.num_pkts_recvd);

    qcm_ping_info_event_cb(ping_session, QCM_PING_STATS, QCM_PING_OK, &ping_stats);

    //If the ping count is less than the user-set count, send again.
    QLOGV("ping_session->summary_info.num_pkts_sent :%d", ping_session->summary_info.num_pkts_sent);
    QLOGV("ping_session->user_opts.num_pings:%d", ping_session->user_opts.num_pings);
    if (ping_session->summary_info.num_pkts_sent < ping_session->user_opts.num_pings)
    {
        send_ptr = (qcm_ping_req_session_id_t *)qosa_malloc(sizeof(qcm_ping_req_session_id_t));
        if (send_ptr == QOSA_NULL)
        {
            return QCM_PING_ERR_MEMORY;
        }
        send_ptr->ping_session_id = ping_session->session_id;
        result = qcm_ping_send_icmp_request((void *)send_ptr);
    }
    //The number of received packets plus the number of lost packets > the number of sent packets error
    if (ping_session && (ping_session->summary_info.num_pkts_recvd + ping_session->summary_info.num_pkts_lost) >= ping_session->user_opts.num_pings)
    {
        //If the sum of lost and successful data exceeds the number configured for the logged-in client, return to end, and report the summary via callback during task polling.
        result = QCM_PING_SEND_END;
    }
exit:
    return result;
}

/**
 * @brief Maximum timeout function for ping execution timeout
 *
 * @param[in] void * argv
 *          - Corresponds specifically to the ping session handle
 *
 * @return void
 */
static void qcm_ping_response_timer_callback(void *argv)
{
    qcm_ping_session_t *ping_session = (qcm_ping_session_t *)argv;

    //Send a timer processing event to the time processing function
    qcm_ping_req_event(QCM_PING_RETRY_TIMER_EXP, ping_session);
}

/**
 * @brief Used to request resources, returns an echo request handle
 *
 * @return qcm_ping_session_t *
 *       - Returns the PING session address
 */
static qcm_ping_session_t *qcm_ping_alloc_session(void)
{
    qcm_ping_session_t *ping_sess_ptr = QOSA_NULL;
    qosa_uint32_t       session_id = 1;

    for (session_id = 0; session_id < QCM_TCPIP_PING_MAX_SESSION_CNT; session_id++)
    {
        if (ping_mgr[session_id] == QOSA_NULL)
        {
            break;
        }
    }

    if (session_id >= QCM_TCPIP_PING_MAX_SESSION_CNT)
    {
        return QOSA_NULL; /*!Too many PING sessions*/
    }

    ping_sess_ptr = qosa_malloc(sizeof(qcm_ping_session_t));
    if (ping_sess_ptr != QOSA_NULL)
    {
        qosa_memset(ping_sess_ptr, 0x00, sizeof(qcm_ping_session_t));

        ping_sess_ptr->session_id = session_id;
        ping_sess_ptr->icmp_echo_msg_id = 0;
        if (QCM_PING_OK != qosa_timer_create(&ping_sess_ptr->ping_response_timer, qcm_ping_response_timer_callback, (void *)ping_sess_ptr))
        {
            QLOGE("timer_create failed");
            qosa_free(ping_sess_ptr);
            return QOSA_NULL;
        }

        QLOGV("ping_response_timer: %p", ping_sess_ptr->ping_response_timer);
    }

    ping_mgr[session_id] = ping_sess_ptr;

    return ping_sess_ptr;
}

/**
 * @brief To release ping session resources, close the socket, stop the timer, release handles, and clean up resources.
 *
 * @param[in] qcm_ping_session_t * ping_session
 *          - The ping session handle to be destroyed
 *
 * @return void
 */
static void qcm_ping_free_session(qcm_ping_session_t *ping_session)
{
    int                 ret = 0;
    qosa_uint32_t       msgq_cnt = 0;
    qcm_ping_task_msg_t ping_msg = {0};

    if (ping_session == QOSA_NULL)
        return;
#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
    qcm_ping_qt2131_release_session_id(ping_session->session_id);
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */
    if (ping_session->sock_hd > 0)
    {
        qcm_socket_close(ping_session->sock_hd);
    }
    if (ping_session->ping_response_timer)
    {
        if (qosa_timer_is_running(ping_session->ping_response_timer))
        {
            qosa_timer_stop(ping_session->ping_response_timer);
        }
        qosa_timer_delete(ping_session->ping_response_timer);
        ping_session->ping_response_timer = QOSA_NULL;
    }
    qosa_free(ping_session->host_name);
    ping_session->host_name = QOSA_NULL;

    ret = qosa_msgq_get_cnt(ping_session->ping_task_msgq, &msgq_cnt);
    while (ret == 0 && msgq_cnt != 0)
    {
        ret = qosa_msgq_wait(ping_session->ping_task_msgq, (qosa_uint8_t *)&ping_msg, sizeof(qcm_ping_task_msg_t), QOSA_NO_WAIT);
        if (ret == 0)
        {
            qosa_free(ping_msg.argv);
        }
        msgq_cnt--;
    }
    qosa_msgq_delete(ping_session->ping_task_msgq);  //(Originally, releasing the message queue here would cause a dump), but now testing shows it does not dump under normal conditions.
    ping_mgr[ping_session->session_id] = QOSA_NULL;
    qosa_free(ping_session);
}

/**
 * @brief The core business task executed by the ping application
 */
static void qcm_ping_app_task(void *argv)
{
    qcm_ping_error_e    result = QCM_PING_OK;
    int                 ret = 0;
    qcm_ping_task_msg_t ping_msg = {0};
    qcm_ping_session_t *ping_session = (qcm_ping_session_t *)argv;
    QLOGV("ping task running");

    for (;;)
    {
        qosa_memset(&ping_msg, 0, sizeof(qcm_ping_task_msg_t));
        ret = qosa_msgq_wait(ping_session->ping_task_msgq, (qosa_uint8_t *)&ping_msg, sizeof(qcm_ping_task_msg_t), QOSA_WAIT_FOREVER);
        if (ret != 0)
        {
            QLOGE("msgq wait error");
            qosa_task_sleep_sec(1);
            continue;
        }

        QLOGV("enter ping task,msgid   = %d", ping_msg.msgid);
        switch (ping_msg.msgid)
        {
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
            //PDP Request
            case QCM_PING_PDP_ACTIVE_REQ: {
                result = qcm_ping_service_data_call(ping_msg.argv);
            }
            break;
#endif     
            //DNS Resolution
            case QCM_PING_DNS_PARSE_REQ: {
                result = qcm_ping_service_dns_parse(ping_msg.argv);
            }
            break;

            //Send ICMP request
            case QCM_PING_SEND_ICMP_REQUEST: {
                result = qcm_ping_send_icmp_request(ping_msg.argv);
            }
            break;
            //Accept ICMP echo messages
            case QCM_PING_RECV_ICMP_REPLY: {
                result = qcm_ping_read_icmp_reply(ping_msg.argv);
            }
            break;
            //Execution timeout event
            case QCM_PING_RETRY_TIMER_EXP: {
                result = qcm_ping_timeout_send_request(ping_msg.argv);
            }
            break;
            case QCM_PING_USER_STOP:
            case QCM_PING_SOCKET_CLOSE: {
                result = (ping_session->final_result != QCM_PING_OK) ? ping_session->final_result : QCM_PING_ERR_UNKNOW;
            }
            break;
            case QCM_PING_DNS_PARSE_RESP: {
                result = qcm_ping_dns_response_handler(ping_msg.argv);
                break;
            }
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
            case QCM_PING_PDP_ACTIVE_RESP: {
                result = qcm_ping_datacall_response_handler(ping_msg.argv);
            }
            break;
#endif
#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
            case QCM_PING_PLATFORM_RESULT_IND: {
                result = qcm_ping_qt2131_result_handler(ping_msg.argv);
            }
            break;
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */
            default:
                break;
        }
        QLOGV("exit result = %d", result);
        if (result != QCM_PING_OK)
            break;
    }
    QLOGV(" result = %d", result);
    ping_session->final_result = (result == QCM_PING_SEND_END) ? QCM_PING_OK : result;

    //Call AT callback function to report URC
    qcm_ping_info_event_cb(ping_session, QCM_PING_SUMMARY, ping_session->final_result, &(ping_session->summary_info));
    qcm_ping_free_session(ping_session);

    //The task will be automatically destroyed after the callback function ends.
}

/**
 * @brief Enable the ping function, set the ping packets to be sent according to the configuration parameters.
 *
 * @param[in] qosa_int32_t cid
 *          - PDP CID information.
 *
 * @param[in] qosa_int32_t sim_id
 *          - SIM ID information
 *
 * @param[in] const char * host
 *          - The peer address for the ping operation
 *
 * @param[in] qcm_ping_config_type * ping_options
 *          - ping request transmission data settings, request byte count, timeout duration, maximum request attempts
 *
 * @param[in] qcm_ping_event_cb cb_fcn
 *          - Event callback function for ping operation, set event type as ping start, and handle judgment of ping callback results
 *
 * @param[in] void * user_param
 *          - User parameters carried
 *
 * @return qcm_ping_error_e
 *       - Returns QCM_PING_OK on success
 *       - Returns other on failure
 */
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
qcm_ping_error_e
qcm_ping_start(const char *host_name, qcm_ping_config_type *ping_options, qcm_ping_event_cb cb_fun, void *user_param)
#else
qcm_ping_error_e
qcm_ping_start(qosa_int32_t cid, qosa_int32_t sim_id, const char *host_name, qcm_ping_config_type *ping_options, qcm_ping_event_cb cb_fun, void *user_param)
#endif
{
    qcm_ping_session_t *ping_session = QOSA_NULL;
    char                thread_name[QOSA_ARRAY_BYTE_32] = {0};
    qosa_int32_t        ret = QCM_PING_OK;
    static qosa_uint8_t task_name_cnt = 0;

    if (host_name == QOSA_NULL || ping_options == QOSA_NULL || cb_fun == QOSA_NULL)
    {
        return QCM_PING_ERR_INVALID_PARAM;
    }

#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
    if (g_qt2131_ping_mutex == QOSA_NULL && qosa_mutex_create(&g_qt2131_ping_mutex) != QOSA_OK)
    {
        QLOGE("mutex create failed");
        g_qt2131_ping_mutex = QOSA_NULL;
        return QCM_PING_ERR_UNKNOW;
    }
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */

    //Apply for a ping echo request handle, with a maximum of 3 simultaneously.
    ping_session = qcm_ping_alloc_session();
    if (ping_session == QOSA_NULL)
    {
        return QCM_PING_ERR_MEMORY;
    }

    //Assign a value to the send request handle
    qosa_memcpy(&(ping_session->user_opts), ping_options, sizeof(qcm_ping_config_type));
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    ping_session->pdp_cid = cid;
    ping_session->sim_id = sim_id;
#endif
    ping_session->notify_fcn = cb_fun;
    ping_session->notify_arg = user_param;
    ping_session->host_name = qosa_malloc(qosa_strlen(host_name) + 1);
    if (ping_session->host_name == QOSA_NULL)
    {
        qcm_ping_free_session(ping_session);
        return QCM_PING_ERR_MEMORY;
    }

    //ql_ping_log("at_resp_cb :%p",cb_fun );
    qosa_memset(ping_session->host_name, 0, qosa_strlen(host_name) + 1);
    qosa_memcpy(ping_session->host_name, host_name, qosa_strlen(host_name));

    //Create a message queue for ping task communication
    ret = qosa_msgq_create(&ping_session->ping_task_msgq, sizeof(qcm_ping_task_msg_t), PING_MSGQ_MAX_CNT);
    if (ret != QCM_PING_OK)
    {
        ret = QCM_PING_ERR_UNKNOW;
        goto exit;
    }

    QLOGV("host_name :%s", ping_session->host_name);

    qosa_snprintf(thread_name, QOSA_ARRAY_BYTE_32, "ping_%d", task_name_cnt++);

    //Create ping main task
    ret = qosa_task_create(&ping_session->ping_task, CONFIG_QCM_PING_TASK_STACK_SIZE, PING_TASK_PRIO, thread_name, qcm_ping_app_task, (void *)ping_session);
    if (ret != QCM_PING_OK)
    {
        ret = QCM_PING_ERR_UNKNOW;
        goto exit;
    }
#if defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) || defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)
    qcm_ping_req_event(QCM_PING_DNS_PARSE_REQ, ping_session);
#else
    qcm_ping_req_event(QCM_PING_PDP_ACTIVE_REQ, ping_session);
#endif
    return QCM_PING_OK;

exit:
    qcm_ping_free_session(ping_session);
    ping_session = QOSA_NULL;
    return ret;
}
