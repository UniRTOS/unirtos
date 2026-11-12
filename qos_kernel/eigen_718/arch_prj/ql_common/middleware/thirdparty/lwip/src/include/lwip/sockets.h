/**
 * @file
 * Socket API (to be used from non-TCPIP threads)
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */


#ifndef LWIP_HDR_SOCKETS_H
#define LWIP_HDR_SOCKETS_H

#include "lwip/opt.h"

#if LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */

#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/inet.h"
#include "lwip/errno.h"

#if !LWIP_TIMEVAL_PRIVATE
#include "osasys.h"
#endif

#if defined(FEATURE_PS_TCPIP_ECSOCK_SKTSOCK_AT_ENABLE) || defined(FEATURE_PS_REF_QR_TCPIP_QSOCK_AT_ENABLE) || defined(FEATURE_PS_REF_CR_TCPIP_SOCK_AT_ENABLE)
#include "cms_sock_mgr.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* If your port already typedef's sa_family_t, define SA_FAMILY_T_DEFINED
   to prevent this code from redefining it. */
#if !defined(sa_family_t) && !defined(SA_FAMILY_T_DEFINED)
typedef u8_t sa_family_t;
#endif
/* If your port already typedef's in_port_t, define IN_PORT_T_DEFINED
   to prevent this code from redefining it. */
#if !defined(in_port_t) && !defined(IN_PORT_T_DEFINED)
typedef u16_t in_port_t;
#endif

typedef void (*socket_callback)(int s, int evt, uint16_t len);
#if LWIP_IPV4
/* members are in network byte order */
struct sockaddr_in {
  u8_t            sin_len;
  sa_family_t     sin_family;
  in_port_t       sin_port;
  struct in_addr  sin_addr;
#define SIN_ZERO_LEN 8
  char            sin_zero[SIN_ZERO_LEN];
};
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
struct sockaddr_in6 {
  u8_t            sin6_len;      /* length of this structure    */
  sa_family_t     sin6_family;   /* AF_INET6                    */
  in_port_t       sin6_port;     /* Transport layer port #      */
  u32_t           sin6_flowinfo; /* IPv6 flow information       */
  struct in6_addr sin6_addr;     /* IPv6 address                */
  u32_t           sin6_scope_id; /* Set of interfaces for scope */
};
#endif /* LWIP_IPV6 */

struct sockaddr {
  u8_t        sa_len;
  sa_family_t sa_family;
  char        sa_data[14];
};

struct sockaddr_storage {
  u8_t        s2_len;
  sa_family_t ss_family;
  char        s2_data1[2];
  u32_t       s2_data2[3];
#if LWIP_IPV6
  u32_t       s2_data3[3];
#endif /* LWIP_IPV6 */
};

/* If your port already typedef's socklen_t, define SOCKLEN_T_DEFINED
   to prevent this code from redefining it. */
#if !defined(socklen_t) && !defined(SOCKLEN_T_DEFINED)
typedef u32_t socklen_t;
#define _SOCKLEN_T
#endif

struct lwip_sock;

#if !LWIP_TCPIP_CORE_LOCKING
/** Maximum optlen used by setsockopt/getsockopt */
#define LWIP_SETGETSOCKOPT_MAXOPTLEN 16

/** This struct is used to pass data to the set/getsockopt_internal
 * functions running in tcpip_thread context (only a void* is allowed) */
struct lwip_setgetsockopt_data {
  /** socket index for which to change options */
  int s;
  /** level of the option to process */
  int level;
  /** name of the option to process */
  int optname;
  /** set: value to set the option to
    * get: value of the option is stored here */
#if LWIP_MPU_COMPATIBLE
  u8_t optval[LWIP_SETGETSOCKOPT_MAXOPTLEN];
#else
  union {
     void *p;
     const void *pc;
  } optval;
#endif
  /** size of *optval */
  socklen_t optlen;
  /** if an error occurs, it is temporarily stored here */
  err_t err;
  /** semaphore to wake up the calling task */
  void* completed_sem;
};
#endif /* !LWIP_TCPIP_CORE_LOCKING */

#if !defined(iovec)
struct iovec {
  void  *iov_base;
  size_t iov_len;
};
#endif

struct msghdr {
  void         *msg_name;
  socklen_t     msg_namelen;
  struct iovec *msg_iov;
  int           msg_iovlen;
  void         *msg_control;
  socklen_t     msg_controllen;
  int           msg_flags;
};

/*
 * The ims data type within the ul pdu.
 * Note: should be same enum as: "UlPduImsDataType" in pspdu.h
*/
typedef enum sockimsdatatype
{
    SOCK_DATA_IMS_DATA_TYPE_MIN     = 0,
    SOCK_DATA_IMS_NONE              = SOCK_DATA_IMS_DATA_TYPE_MIN, /*the ul sock data without ims related data*/
    SOCK_DATA_IMS_CALL_SIGNALING    = 1,    /* The ul sock data with ims voice signaling
                                             * it will triger RRC establishment cause "origaling MMTEL voice" if need;
                                            */
    SOCK_DATA_IMS_CALL_VOICE        = 2,    /* The ul sock data with ims voice RTP data, it will triger RRC establishment cause "origaling MMTEL voice" if need;*/
    SOCK_DATA_IMS_SMS               = 3,    /* The ul sock data with ims SMS signaling data
                                             * It will triger RRC establishment cause "origaling SMSoIP" if need
                                            */
    SOCK_DATA_IMS_OTHERS            = 4,    /* The ul sock data with ims other data(such as rtcp, ims REGISTER related)
                                            */
    SOCK_DATA_IMS_DATA_TYPE_MAX     = SOCK_DATA_IMS_OTHERS,
}sockimsdatatype_t;

typedef enum cms_dl_buf_flag
{
    CMS_DL_BUF_DLFC_PREFER          = 0,
    CMS_DL_BUF_DLFC_ONLY            = 1,
    CMS_DL_BUF_DLCACHE_ONLY         = 2,
}cms_dl_buf_flag_t;



/* Socket protocol types (TCP/UDP/RAW) */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3

/*
 * Option flags per-socket. These must match the SOF_ flags in ip.h (checked in init.c)
 */
#define SO_REUSEADDR   0x0004 /* Allow local address reuse */
#define SO_KEEPALIVE   0x0008 /* keep connections alive */
#define SO_BROADCAST   0x0020 /* permit to send and to receive broadcast messages (see IP_SOF_BROADCAST option) */


/*
 * Additional options, not kept in so_options.
 */
#define SO_DEBUG       0x0001 /* Unimplemented: turn on debugging info recording */
#define SO_ACCEPTCONN  0x0002 /* socket has had listen() */
#define SO_DONTROUTE   0x0010 /* Unimplemented: just use interface addresses */
#define SO_USELOOPBACK 0x0040 /* Unimplemented: bypass hardware when possible */
#define SO_LINGER      0x0080 /* linger on close if data present */
#define SO_DONTLINGER  ((int)(~SO_LINGER))
#define SO_OOBINLINE   0x0100 /* Unimplemented: leave received OOB data in line */
#define SO_REUSEPORT   0x0200 /* Unimplemented: allow local address & port reuse */
#define SO_SNDBUF      0x1001 /* Unimplemented: send buffer size */
#define SO_RCVBUF      0x1002 /* receive buffer size */
#define SO_SNDLOWAT    0x1003 /* Unimplemented: send low-water mark */
#define SO_RCVLOWAT    0x1004 /* Unimplemented: receive low-water mark */
#define SO_SNDTIMEO    0x1005 /* send timeout */
#define SO_RCVTIMEO    0x1006 /* receive timeout */
#define SO_ERROR       0x1007 /* get error status and clear */
#define SO_TYPE        0x1008 /* get socket type */
#define SO_CONTIMEO    0x1009 /* Unimplemented: connect timeout */
#define SO_NO_CHECK    0x100a /* don't create UDP checksum */

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
#define SO_HIB_SLEEP2    0x2001 /* set sock hib/sleep2 mode */
#define SO_SLEEP2    0x2010 /* set sock sleep2 mode */
#endif
#if ENABLE_PSIF
#define SO_SEQUENCE_HANDLER 0x2002 /*set the udp socket as sequence state handle socket*/
#define SO_TCP_MAX_RETRY_TIMES 0x2003 /*set the tcp connection max retry number*/
#define SO_TCP_MAX_TOTAL_RETRY_TIME 0x2004 /*set the tcp connection max retry time*/
#define SO_TCP_REPORT_UL_TOTAL_STATUS 0x2005   /*set the tcp connection report the UL status(sent total length)*/
#define SO_TCP_CLOSE_TIMEOUT_NEED_LOCAL_ABORT 0x2006/*whether enable tcp close local abort(seconds)*/
#define SO_TCP_INIT_RETRY_TIME 0x2007 /*set the tcp connection init retry time value*/

#define SO_TCP_CLOSE_COMPLETE_HANDLER 0x2008 /*set the udp socket as TCP CLOSE COMPLETE EVENT handle socket*/

#define SO_TCP_REPORT_CLOSE_COMPLETE 0x2009   /*set the tcp connection report the close complete event*/

#define SO_TCP_KEEPALIVE_WITH_RAI 0x2011   /*set the tcp connection keepalive pkg with rai*/

#ifdef UNIRTOS_LWIP_MODIFY
// Modified by Quectel: Is it necessary to wait for the TCP TIME_WAIT state
#define SO_TCP_TIME_WAIT        0x200a
#endif /* UNIRTOS_LWIP_MODIFY */

#endif



/*
 * Structure used for manipulating linger option.
 */
struct linger {
       int l_onoff;                /* option on/off */
       int l_linger;               /* linger time in seconds */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define  SOL_SOCKET  0xfff    /* options for socket level */


#define AF_UNSPEC       0
#define AF_INET         2
#if LWIP_IPV6
#define AF_INET6        10
#else /* LWIP_IPV6 */
#define AF_INET6        AF_UNSPEC
#endif /* LWIP_IPV6 */
#define PF_INET         AF_INET
#define PF_INET6        AF_INET6
#define PF_UNSPEC       AF_UNSPEC

#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17
#if LWIP_IPV6
#define IPPROTO_IPV6    41
#define IPPROTO_ICMPV6  58
#endif /* LWIP_IPV6 */
#define IPPROTO_UDPLITE 136
#define IPPROTO_RAW     255

/* Flags we can use with send and recv. */
#define MSG_PEEK       0x01    /* Peeks at an incoming message */
#define MSG_WAITALL    0x02    /* Unimplemented: Requests that the function block until the full amount of data requested can be returned */
#define MSG_OOB        0x04    /* Unimplemented: Requests out-of-band data. The significance and semantics of out-of-band data are protocol-specific */
#define MSG_DONTWAIT   0x08    /* Nonblocking i/o for this operation only */
#define MSG_MORE       0x10    /* Sender will send more */


/*
 * Options for level IPPROTO_IP
 */
#define IP_TOS             1
#define IP_TTL             2

#if LWIP_TCP
/*
 * Options for level IPPROTO_TCP
 */
#define TCP_NODELAY    0x01    /* don't delay send to coalesce packets */
#define TCP_KEEPALIVE  0x02    /* send KEEPALIVE probes when idle for pcb->keep_idle milliseconds */
#define TCP_KEEPIDLE   0x03    /* set pcb->keep_idle  - Same as TCP_KEEPALIVE, but use seconds for get/setsockopt */
#define TCP_KEEPINTVL  0x04    /* set pcb->keep_intvl - Use seconds for get/setsockopt */
#define TCP_KEEPCNT    0x05    /* set pcb->keep_cnt   - Use number of probes sent for get/setsockopt */
#define TCP_WND_SETTING 0x06   /* set pcb->rcv_wnd && pcb->rcv_ann_wnd*/
#endif /* LWIP_TCP */

#ifdef UNIRTOS_LWIP_MODIFY
#define TCP_DELAY_AK  0x07
#define TCP_TWRECYCLE  0x08
#endif/* UNIRTOS_LWIP_MODIFY */

#if LWIP_IPV6
/*
 * Options for level IPPROTO_IPV6
 */
#define IPV6_CHECKSUM       7  /* RFC3542: calculate and insert the ICMPv6 checksum for raw sockets. */
#define IPV6_V6ONLY         27 /* RFC3493: boolean control to restrict AF_INET6 sockets to IPv6 communications only. */
#endif /* LWIP_IPV6 */

#if LWIP_UDP && LWIP_UDPLITE
/*
 * Options for level IPPROTO_UDPLITE
 */
#define UDPLITE_SEND_CSCOV 0x01 /* sender checksum coverage */
#define UDPLITE_RECV_CSCOV 0x02 /* minimal receiver checksum coverage */
#endif /* LWIP_UDP && LWIP_UDPLITE*/


#if LWIP_MULTICAST_TX_OPTIONS
/*
 * Options and types for UDP multicast traffic handling
 */
#define IP_MULTICAST_TTL   5
#define IP_MULTICAST_IF    6
#define IP_MULTICAST_LOOP  7
#endif /* LWIP_MULTICAST_TX_OPTIONS */

#if LWIP_IGMP
/*
 * Options and types related to multicast membership
 */
#define IP_ADD_MEMBERSHIP  3
#define IP_DROP_MEMBERSHIP 4

typedef struct ip_mreq {
    struct in_addr imr_multiaddr; /* IP multicast address of group */
    struct in_addr imr_interface; /* local IP address of interface */
} ip_mreq;
#endif /* LWIP_IGMP */

/*
 * The Type of Service provides an indication of the abstract
 * parameters of the quality of service desired.  These parameters are
 * to be used to guide the selection of the actual service parameters
 * when transmitting a datagram through a particular network.  Several
 * networks offer service precedence, which somehow treats high
 * precedence traffic as more important than other traffic (generally
 * by accepting only traffic above a certain precedence at time of high
 * load).  The major choice is a three way tradeoff between low-delay,
 * high-reliability, and high-throughput.
 * The use of the Delay, Throughput, and Reliability indications may
 * increase the cost (in some sense) of the service.  In many networks
 * better performance for one of these parameters is coupled with worse
 * performance on another.  Except for very unusual cases at most two
 * of these three indications should be set.
 */
#define IPTOS_TOS_MASK          0x1E
#define IPTOS_TOS(tos)          ((tos) & IPTOS_TOS_MASK)
#define IPTOS_LOWDELAY          0x10
#define IPTOS_THROUGHPUT        0x08
#define IPTOS_RELIABILITY       0x04
#define IPTOS_LOWCOST           0x02
#define IPTOS_MINCOST           IPTOS_LOWCOST

/*
 * The Network Control precedence designation is intended to be used
 * within a network only.  The actual use and control of that
 * designation is up to each network. The Internetwork Control
 * designation is intended for use by gateway control originators only.
 * If the actual use of these precedence designations is of concern to
 * a particular network, it is the responsibility of that network to
 * control the access to, and use of, those precedence designations.
 */
#define IPTOS_PREC_MASK                 0xe0
#define IPTOS_PREC(tos)                ((tos) & IPTOS_PREC_MASK)
#define IPTOS_PREC_NETCONTROL           0xe0
#define IPTOS_PREC_INTERNETCONTROL      0xc0
#define IPTOS_PREC_CRITIC_ECP           0xa0
#define IPTOS_PREC_FLASHOVERRIDE        0x80
#define IPTOS_PREC_FLASH                0x60
#define IPTOS_PREC_IMMEDIATE            0x40
#define IPTOS_PREC_PRIORITY             0x20
#define IPTOS_PREC_ROUTINE              0x00


/*
 * Commands for ioctlsocket(),  taken from the BSD file fcntl.h.
 * lwip_ioctl only supports FIONREAD and FIONBIO, for now
 *
 * Ioctl's have the command encoded in the lower word,
 * and the size of any in or out parameters in the upper
 * word.  The high 2 bits of the upper word are used
 * to encode the in/out status of the parameter; for now
 * we restrict parameters to at most 128 bytes.
 */
#if !defined(FIONREAD) || !defined(FIONBIO)
#define IOCPARM_MASK    0x7fU           /* parameters must be < 128 bytes */
#define IOC_VOID        0x20000000UL    /* no parameters */
#define IOC_OUT         0x40000000UL    /* copy out parameters */
#define IOC_IN          0x80000000UL    /* copy in parameters */
#define IOC_INOUT       (IOC_IN|IOC_OUT)
                                        /* 0x20000000 distinguishes new &
                                           old ioctl's */
#define _IO(x,y)        (IOC_VOID|((x)<<8)|(y))

#define _IOR(x,y,t)     (IOC_OUT|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))

#define _IOW(x,y,t)     (IOC_IN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))
#endif /* !defined(FIONREAD) || !defined(FIONBIO) */

#ifndef FIONREAD
#define FIONREAD    _IOR('f', 127, unsigned long) /* get # bytes to read */
#endif
#ifndef FIONBIO
#define FIONBIO     _IOW('f', 126, unsigned long) /* set/clear non-blocking i/o */
#endif

#if ENABLE_PSIF

#ifndef FIOHWODR
#define FIOHWODR     _IOW('f', 125, unsigned long) /* set/clear high water without delay i/o */
#endif

#endif

/* Socket I/O Controls: unimplemented */
#ifndef SIOCSHIWAT
#define SIOCSHIWAT  _IOW('s',  0, unsigned long)  /* set high watermark */
#define SIOCGHIWAT  _IOR('s',  1, unsigned long)  /* get high watermark */
#define SIOCSLOWAT  _IOW('s',  2, unsigned long)  /* set low watermark */
#define SIOCGLOWAT  _IOR('s',  3, unsigned long)  /* get low watermark */
#define SIOCATMARK  _IOR('s',  7, unsigned long)  /* at oob mark? */
#endif

/* commands for fnctl */
#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif

/* File status flags and file access modes for fnctl,
   these are bits in an int. */
#ifndef O_NONBLOCK
#define O_NONBLOCK  1 /* nonblocking I/O */
#endif
#ifndef O_NDELAY
#define O_NDELAY    1 /* same as O_NONBLOCK, for compatibility */
#endif

#ifndef SHUT_RD
  #define SHUT_RD   0
  #define SHUT_WR   1
  #define SHUT_RDWR 2
#endif

/* FD_SET used for lwip_select */
#ifndef FD_SET
#undef  FD_SETSIZE
/* Make FD_SETSIZE match NUM_SOCKETS in socket.c */
#define FD_SETSIZE    MEMP_NUM_NETCONN
#define FDSETSAFESET(n, code) do { \
  if (((n) - LWIP_SOCKET_OFFSET < MEMP_NUM_NETCONN) && (((int)(n) - LWIP_SOCKET_OFFSET) >= 0)) { \
  code; }} while(0)
#define FDSETSAFEGET(n, code) (((n) - LWIP_SOCKET_OFFSET < MEMP_NUM_NETCONN) && (((int)(n) - LWIP_SOCKET_OFFSET) >= 0) ?\
  (code) : 0)
#define FD_SET(n, p)  FDSETSAFESET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET)/8] |=  (1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define FD_CLR(n, p)  FDSETSAFESET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET)/8] &= ~(1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define FD_ISSET(n,p) FDSETSAFEGET(n, (p)->fd_bits[((n)-LWIP_SOCKET_OFFSET)/8] &   (1 << (((n)-LWIP_SOCKET_OFFSET) & 7)))
#define FD_ZERO(p)    memset((void*)(p), 0, sizeof(*(p)))

typedef struct fd_set
{
  unsigned char fd_bits [(FD_SETSIZE+7)/8];
} fd_set;

#elif LWIP_SOCKET_OFFSET
#error LWIP_SOCKET_OFFSET does not work with external FD_SET!
#elif FD_SETSIZE < MEMP_NUM_NETCONN
#error "external FD_SETSIZE too small for number of sockets"
#endif /* FD_SET */

/** LWIP_TIMEVAL_PRIVATE: if you want to use the struct timeval provided
 * by your system, set this to 0 and include <sys/time.h> in cc.h */
#ifndef LWIP_TIMEVAL_PRIVATE
#if defined(__GNUC__)
#define LWIP_TIMEVAL_PRIVATE 0
#else
#define LWIP_TIMEVAL_PRIVATE 1
#endif
#endif


#if LWIP_TIMEVAL_PRIVATE
#if defined(__CC_ARM)
struct timeval {
  long    tv_sec;         /* seconds */
  long    tv_usec;        /* and microseconds */
};
#endif
#endif /* LWIP_TIMEVAL_PRIVATE */

#define lwip_socket_init() /* Compatibility define, no init needed. */
void lwip_socket_thread_init(void); /* LWIP_NETCONN_SEM_PER_THREAD==1: initialize thread-local semaphore */
void lwip_socket_thread_cleanup(void); /* LWIP_NETCONN_SEM_PER_THREAD==1: destroy thread-local semaphore */

#if LWIP_COMPAT_SOCKETS == 2
/* This helps code parsers/code completion by not having the COMPAT functions as defines */
#define lwip_accept       accept
#define lwip_bind         bind
#define lwip_shutdown     shutdown
#define lwip_getpeername  getpeername
#define lwip_getsockname  getsockname
#define lwip_setsockopt   setsockopt
#define lwip_getsockopt   getsockopt
#define lwip_close        closesocket
#define lwip_connect      connect
#define lwip_listen       listen
#define lwip_recv         recv
#define lwip_recvfrom     recvfrom
#define lwip_send         send
#define lwip_sendmsg      sendmsg
#define lwip_sendto       sendto
#define lwip_socket       socket
#define lwip_select       select
#define lwip_ioctlsocket  ioctl

#if LWIP_POSIX_SOCKETS_IO_NAMES
#define lwip_read         read
#define lwip_write        write
#define lwip_writev       writev
#undef lwip_close
#define lwip_close        close
#define closesocket(s)    close(s)
#define lwip_fcntl        fcntl
#define lwip_ioctl        ioctl
#endif /* LWIP_POSIX_SOCKETS_IO_NAMES */
#endif /* LWIP_COMPAT_SOCKETS == 2 */

int lwip_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int lwip_bind(int s, const struct sockaddr *name, socklen_t namelen);

#if ENABLE_PSIF
//support binding cid(related with default bearer or dedicated bearer)
/*
*1.  if the socket bind to one cid (default/dedicated cid),the UL pkg with the socket will output with the binding cid (not which will not do TFT match)
*2.  if the socket not bind to any cid, the UL pkg with the socket will output with the cid(must do TFT match)
*3.  if you create more than one default bearer(exist more than one dedicated bearer for each default beaer). and you need that the UL pkg of the socket output with the cid(must do tft match). you must call lwip_bind() bind the local ip of the default bearer
*/
int lwip_bind_cid(int s, u8_t cid);

int
lwip_bind_cid_withpsstartchk(int s, u8_t cid, int bchk);

int32_t
lwip_alloc_server_port_withpsstartchk(u8_t type, int bchk);



/*
*lwip_alloc_server_port allocate udp/tcp server port
*input: IPPROTO_TCP/IPPROTO_UDP
*return: if fail, return -1; if success, return the server port
*(ps: the server port maybe used by another socket, so you need bind the server port right now by lwip_bind() api)
*/
int32_t lwip_alloc_server_port(u8_t type);

//input :s the socket file description
//out put: return the socket err number,if the socket with the file description is invalid, it will return EBADF
int sock_get_errno(int s);
int lwip_socket_expect(int domain, int type, int protocol, int expect_fd);
int socket_error_is_fatal(int errorno);

int lwip_get_socket_info_by_fd(int fd, ip_addr_t *local_ip, ip_addr_t *remote_ip, u16_t *local_port, u16_t *remote_port, int *type);

#endif

#ifdef UNIRTOS_LWIP_MODIFY
/*---------------------------------------------------------------------------
                      Asynchronous Socket Events
---------------------------------------------------------------------------*/
int lwip_get_error(int s);
int lwip_get_last_close_event(int s);
int32_t lwip_getAckedSize(int nSocket);
int32_t lwip_getRecvAvailSize(int nSocket);

#define QL_TCPIP_MINBACKOFFS            3
#define QL_TCPIP_MAXBACKOFFS            12
#define QL_TCPIP_DEFBACKOFFS            5

#define QL_TCPIP_MINRTO                 1
#define QL_TCPIP_MAXRTO                 60
#define QL_TCPIP_DEFRTO                 6
#define QL_TCPIP_WINDOWSCALE            1

#define QL_TCPIP_SND_WINDOW             16*1024
#define QL_TCPIP_RCV_WINDOW             16*1024

#define QL_TCPIP_TWRECYCLE              0

#define QL_UDP_FILTER_MODE              0

#define QL_TCPIP_MSS                    1360

int unir_get_lwip_tcp_rto_init(void);
int unir_get_lwip_tcp_max_rtx(void);
int quecl_get_lwip_tcp_syn_max_rtx(void);
int unir_get_lwip_tcp_window_scale(void);
int unir_get_lwip_tcp_mss(void);
int unir_get_lwip_tcp_send_window(void);
int unir_get_lwip_tcp_recv_window(void);
int unir_get_lwip_tcp_twrecycle(void);
int unir_get_lwip_udp_filter_mode(void);
void  unir_set_lwip_tcp_rto_init(int lwip_tcp_rto_init);
void  unir_set_lwip_tcp_max_rtx(int lwip_tcp_max_rtx);
void  unir_set_lwip_tcp_syn_max_rtx(int lwip_tcp_syn_max_rtx);
void  unir_set_lwip_tcp_window_scale(int lwip_tcp_window_scale);
void  unir_set_lwip_tcp_send_window(int lwip_tcp_send_window);
void  unir_set_lwip_tcp_recv_window(int lwip_tcp_recv_window);
void  unir_set_lwip_tcp_twrecycle(int lwip_tcp_twrecycle);
void  unir_set_lwip_udp_filter_mode(int lwip_udp_filter_mode);
void  unir_set_lwip_tcp_mss(int lwip_tcp_mss);

/*---------------------------------------------------------------------------
                      Asynchronous Socket Events
---------------------------------------------------------------------------*/
#define QL_DS_WRITE_EVENT 0x01 /* associated with a writeable socket */
#define QL_DS_READ_EVENT  0x02 /* associated with a readable socket */
#define QL_DS_CLOSE_EVENT 0x04 /* associated with a closeable socket */
//#define QL_DS_ACCEPT_EVENT 0x08        /* associated with an accetable socket */
int unir_async_select_mask(u16_t fd, u32_t interest_mask);
int unir_set_socket2(u16_t fd, void (*p)(u16_t, u32_t, void *), void *pam);
#endif /* UNIRTOS_LWIP_MODIFY */

int lwip_shutdown(int s, int how);
int lwip_getpeername (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockname (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen);
int lwip_setsockopt (int s, int level, int optname, const void *optval, socklen_t optlen);
int lwip_close(int s);
int lwip_connect(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_listen(int s, int backlog);
int lwip_recv(int s, void *mem, size_t len, int flags);
int lwip_read(int s, void *mem, size_t len);
int lwip_recvfrom(int s, void *mem, size_t len, int flags,
      struct sockaddr *from, socklen_t *fromlen);
int lwip_send(int s, const void *dataptr, size_t size, int flags);
int lwip_sendmsg(int s, const struct msghdr *message, int flags);
int lwip_sendto(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen);
int lwip_socket(int domain, int type, int protocol);
int lwip_write(int s, const void *dataptr, size_t size);
int lwip_writev(int s, const struct iovec *iov, int iovcnt);
int lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,
                struct timeval *timeout);
int lwip_ioctl(int s, long cmd, void *argp);
int lwip_fcntl(int s, int cmd, int val);

#if LWIP_COMPAT_SOCKETS
#if LWIP_COMPAT_SOCKETS != 2
/** @ingroup socket */
#define accept(s,addr,addrlen)                    lwip_accept(s,addr,addrlen)
/** @ingroup socket */
#define bind(s,name,namelen)                      lwip_bind(s,name,namelen)

/** @ingroup socket ext */
//0->success,others->fail
#if ENABLE_PSIF

/* SOCKET DATA RAI info: Release assistant info  */
#define PS_SOCK_RAI_NO_INFO             0
#define PS_SOCK_RAI_NO_UL_DL_FOLLOWED   1
#define PS_SOCK_ONLY_DL_FOLLOWED        2

struct sockticksinfo
{
    bool discard_ticks_valid;
    uint32_t discard_ticks; /*system ticks, if after "discard_ticks", still not send, PS could discard this packet*/
};

#define bind_cid(s,cid)                           lwip_bind_cid(s,cid)
int ps_send(int s, const void *data, size_t size, int flags, u8_t dataRai, bool exceptdata);
int ps_sendto(int s, const void *data, size_t size, int flags,
              const struct sockaddr *to, socklen_t tolen, u8_t dataRai, bool exceptdata);

//the input parameter is only valid for UDP&RAW socket, it is invalid for TCP socket
int ps_send_with_ticks(int s, const void *data, size_t size, int flags, u8_t dataRai, bool exceptdata, struct sockticksinfo *tick_info);

//the input parameter is only valid for UDP&RAW socket, it is invalid for TCP socket
int ps_sendto_with_ticks(int s, const void *data, size_t size, int flags,
       const struct sockaddr *to, socklen_t tolen, u8_t dataRai, bool exceptdata, struct sockticksinfo *tick_info);
int ps_send_with_sequence(int s, const void *data, size_t size, int flags, u8_t dataRai, bool exceptdata, u8_t sequence);

int ps_sendto_with_sequence(int s, const void *data, size_t size, int flags,
       const struct sockaddr *to, socklen_t tolen, u8_t dataRai, bool exceptdata,u8_t sequence);
/**
 * int ps_send_with_imsdataflag()
 * input: int       s,
 *        const void *data,
 *        size_t    size,
 *        int       flags,
 *        sockimsdatatype_t data_flag
 * return: if fail, return -1; if success, return the sent length
 * Note: UE will send data with some specail flag
 *       you can reference the data_flag define (sockimsdatatype_t)
*/
int ps_send_with_imsdataflag(int s, const void *data, size_t size, int flags, sockimsdatatype_t data_flag, struct sockticksinfo *tick_info);

/**
 * int ps_sendto_with_imsdataflag()
 * input: int s,
 *        const void    *data,
 *        size_t        size,
 *        int           flags,
 *        const struct sockaddr     *to,
 *        socklen_t     tolen,
 *        sockimsdatatype_t     data_flag
 *        struct sockticksinfo *tick_info -->invalid with tcp socket connection
 * return: if fail, return -1; if success, return the sent length
 * Note: UE will send data with some specail ims flag.
 *       you can reference the data_flag define (sockimsdatatype_t)
*/
int ps_sendto_with_imsdataflag(int s, const void *data, size_t size, int flags,
       const struct sockaddr *to, socklen_t tolen, sockimsdatatype_t data_flag, struct sockticksinfo *tick_info);

/**
 *lwip_socket_set_rohc_rtp_cfg
 *input: u8_t   cid,
 *       ip_addr_t *dstIp,
 *       u16_t srcPort,
 *       u16_t dstPort
 * return: if fail, return -1; if success , return 0
 *Note: you can set the rohc rtp stream configuration by this API
**/
int lwip_socket_set_rohc_rtp_cfg(u8_t cid, ip_addr_t *dstIp, u16_t srcPort, u16_t dstPort);

/**
 *lwip_socket_clean_rohc_rtp_cfg
 *input: u8_t   cid,
 * return: if fail, return -1; if success , return 0
 *Note: you can clean the rohc rtp stream configuration by this API
 *
**/
int lwip_socket_clean_rohc_rtp_cfg(u8_t cid);

/**
*/
int lwip_get_tcp_send_buffer_size(int s);

/**
 * UDP data sendto local socket (bind local IP: IPADDR_LOOPBACK(127.0.0.1)).
 * return: if succ, return 0, fail < 0
*/
int lwip_udp_local_sendto(const void *data, int size, u16_t toPort, u16_t fromPort);

/**
 * UDP data sendto local socket (bind local IP: IPADDR_LOOPBACK(127.0.0.1)).
 * return: if succ, return 0, fail < 0
*/
int lwip_udp_socket_local_sendto(int s, const void *data, int size, u16_t toPort);


/**
 * UDP data sendto local socket (bind local IP: IPADDR_LOOPBACK(127.0.0.1)), which could called in ISR
 * if failed, "*data" will be freed via: lwip_free_mem_callback()
*/
//#define lwip_free_mem_callback      tcpip_free_mem_callback
typedef void (*lwip_free_mem_callback)(void *ptr);
void lwip_udp_local_sendto_isr(void *data, u16_t size, u16_t toPort, lwip_free_mem_callback freefunc);

/**
 * int value (< 4 bytes) sendto local socket (bind local IP: IPADDR_LOOPBACK(127.0.0.1)), which could called in ISR
 * Used in some special case, if socket data is in range of "INT32"
*/
void lwip_udp_local_int_sendto_isr(int value, u16_t size, u16_t toPort);


#if defined(FEATURE_PS_TCPIP_ECSOCK_SKTSOCK_AT_ENABLE) || defined(FEATURE_PS_REF_QR_TCPIP_QSOCK_AT_ENABLE) || defined(FEATURE_PS_REF_CR_TCPIP_SOCK_AT_ENABLE)
int lwip_erecvfrom(int s, void **mem, size_t len, int flags,
              struct sockaddr *from, socklen_t *fromlen);
#endif


#endif

/** @ingroup socket */
#define shutdown(s,how)                           lwip_shutdown(s,how)
/** @ingroup socket */
#define getpeername(s,name,namelen)               lwip_getpeername(s,name,namelen)
/** @ingroup socket */
#define getsockname(s,name,namelen)               lwip_getsockname(s,name,namelen)
/** @ingroup socket */
#define setsockopt(s,level,optname,opval,optlen)  lwip_setsockopt(s,level,optname,opval,optlen)
/** @ingroup socket */
#define getsockopt(s,level,optname,opval,optlen)  lwip_getsockopt(s,level,optname,opval,optlen)
/** @ingroup socket */
#define closesocket(s)                            lwip_close(s)
/** @ingroup socket */
#define connect(s,name,namelen)                   lwip_connect(s,name,namelen)
/** @ingroup socket */
#define listen(s,backlog)                         lwip_listen(s,backlog)
/** @ingroup socket */
#define recv(s,mem,len,flags)                     lwip_recv(s,mem,len,flags)
/** @ingroup socket */
#define recvfrom(s,mem,len,flags,from,fromlen)    lwip_recvfrom(s,mem,len,flags,from,fromlen)
/** @ingroup socket */
#define send(s,dataptr,size,flags)                lwip_send(s,dataptr,size,flags)
/** @ingroup socket */
#define sendmsg(s,message,flags)                  lwip_sendmsg(s,message,flags)
/** @ingroup socket */
#define sendto(s,dataptr,size,flags,to,tolen)     lwip_sendto(s,dataptr,size,flags,to,tolen)
/** @ingroup socket */
#define socket(domain,type,protocol)              lwip_socket(domain,type,protocol)
#if ENABLE_PSIF
#define socket_expect(domain, type, protocol, expect_fd) lwip_socket_expect(domain, type, protocol, expect_fd)

int
lwip_socket_expect_withpsstartchk(int domain, int type, int protocol, int expect_fd, int bchk);
#endif
/** @ingroup socket */
#define select(maxfdp1,readset,writeset,exceptset,timeout)     lwip_select(maxfdp1,readset,writeset,exceptset,timeout)
/** @ingroup socket */
#define ioctlsocket(s,cmd,argp)                   lwip_ioctl(s,cmd,argp)

#if ENABLE_PSIF
#define erecvfrom(s,mem,len,flags,from,fromlen)    lwip_erecvfrom(s,mem,len,flags,from,fromlen)
#endif

#if defined(FEATURE_PS_TCPIP_ECSOCK_SKTSOCK_AT_ENABLE) || defined(FEATURE_PS_REF_QR_TCPIP_QSOCK_AT_ENABLE) || defined(FEATURE_PS_REF_CR_TCPIP_SOCK_AT_ENABLE)

int lwip_cmsrecvfrom(int s, DlPduBlock **pp_dlpdu, CmsSockDlBufContext **pp_dlbuf, size_t len, int flags,
              struct sockaddr *from, socklen_t *fromlen, cms_dl_buf_flag_t buf_flag); // buf_flag-->cms_dl_buf_flag_t
#endif




#if ENABLE_PSIF
void sockaddr_to_ipaddr_port(const struct sockaddr* sockaddr, ip_addr_t* ipaddr, u16_t* port);
#endif

#if LWIP_SOCKET_CALL_BACK_ENABLE
int lwip_socket_with_call_back(int domain, int type, int protocol, socket_callback callback);
#endif

int
lwip_getsockopt_withpsstartchk(int s, int level, int optname, void *optval, socklen_t *optlen, int bchk);

int
lwip_setsockopt_withpsstartchk(int s, int level, int optname, const void *optval, socklen_t optlen, int bchk);

int
lwip_socket_withpsstartchk(int domain, int type, int protocol, int bchk);

int
lwip_accept_withpsstartchk(int s, struct sockaddr *addr, socklen_t *addrlen, int bchk);

int
lwip_bind_withpsstartchk(int s, const struct sockaddr *name, socklen_t namelen, int bchk);

int
lwip_close_withpsstartchk(int s, int bchk);

int
lwip_connect_withpsstartchk(int s, const struct sockaddr *name, socklen_t namelen, int bchk);

int
lwip_listen_withpsstartchk(int s, int backlog, int bchk);

int
lwip_recvfrom_withpsstartchk(int s, void *mem, size_t len, int flags,
              struct sockaddr *from, socklen_t *fromlen, int bchk);

int
lwip_read_withpsstartchk(int s, void *mem, size_t len, int bchk);

int
lwip_recv_withpsstartchk(int s, void *mem, size_t len, int flags, int bchk);

int
lwip_send_withpsstartchk(int s, const void *data, size_t size, int flags, int bchk);

int
lwip_sendto_withpsstartchk(int s, const void *data, size_t size, int flags,
       const struct sockaddr *to, socklen_t tolen, int bchk);

int
lwip_getpeername_withpsstartchk(int s, struct sockaddr *name, socklen_t *namelen, int bchk);

int
lwip_getsockname_withpsstartchk(int s, struct sockaddr *name, socklen_t *namelen, int bchk);

int
lwip_sendmsg_withpsstartchk(int s, const struct msghdr *msg, int flags, int bchk);

int
lwip_shutdown_withpsstartchk(int s, int how, int bchk);


#if LWIP_POSIX_SOCKETS_IO_NAMES
/** @ingroup socket */
#define read(s,mem,len)                           lwip_read(s,mem,len)
/** @ingroup socket */
#define write(s,dataptr,len)                      lwip_write(s,dataptr,len)
/** @ingroup socket */
#define writev(s,iov,iovcnt)                      lwip_writev(s,iov,iovcnt)
/** @ingroup socket */
#define close(s)                                  lwip_close(s)
/** @ingroup socket */
#define fcntl(s,cmd,val)                          lwip_fcntl(s,cmd,val)
/** @ingroup socket */
#define ioctl(s,cmd,argp)                         lwip_ioctl(s,cmd,argp)
#endif /* LWIP_POSIX_SOCKETS_IO_NAMES */
#endif /* LWIP_COMPAT_SOCKETS != 2 */


#if LWIP_IPV4 && LWIP_IPV6
/** @ingroup socket */
#define inet_ntop(af,src,dst,size) \
    (((af) == AF_INET6) ? ip6addr_ntoa_r((const ip6_addr_t*)(src),(dst),(size)) \
     : (((af) == AF_INET) ? ip4addr_ntoa_r((const ip4_addr_t*)(src),(dst),(size)) : NULL))
/** @ingroup socket */
#define inet_pton(af,src,dst) \
    (((af) == AF_INET6) ? ip6addr_aton((src),(ip6_addr_t*)(dst)) \
     : (((af) == AF_INET) ? ip4addr_aton((src),(ip4_addr_t*)(dst)) : 0))

#elif LWIP_IPV4 /* LWIP_IPV4 && LWIP_IPV6 */
#define inet_ntop(af,src,dst,size) \
    (((af) == AF_INET) ? ip4addr_ntoa_r((const ip4_addr_t*)(src),(dst),(size)) : NULL)
#define inet_pton(af,src,dst) \
    (((af) == AF_INET) ? ip4addr_aton((src),(ip4_addr_t*)(dst)) : 0)
#else /* LWIP_IPV4 && LWIP_IPV6 */
#define inet_ntop(af,src,dst,size) \
    (((af) == AF_INET6) ? ip6addr_ntoa_r((const ip6_addr_t*)(src),(dst),(size)) : NULL)
#define inet_pton(af,src,dst) \
    (((af) == AF_INET6) ? ip6addr_aton((src),(ip6_addr_t*)(dst)) : 0)
#endif /* LWIP_IPV4 && LWIP_IPV6 */

#endif /* LWIP_COMPAT_SOCKETS */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_SOCKET */

#endif /* LWIP_HDR_SOCKETS_H */
