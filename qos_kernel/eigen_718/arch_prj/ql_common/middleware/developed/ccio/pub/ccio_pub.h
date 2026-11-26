/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: ccio_pub.h
*
*  Description:
*
*  History: 2021/1/19 created by xuwang
*
*  Notes: public & universal definition of Channel Centre for Input/Output(CCIO) service
*
******************************************************************************/
#ifndef CCIO_PUB_H
#define CCIO_PUB_H

/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/
#include "sctdef.h"
#include "RTE_Device.h"
#include "ccio_base.h"


#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/
#if (defined OPEN_CPU_MODE)
#define CCIO_NET_PERFORMACE_ENABLE  0
#else
#define CCIO_NET_PERFORMACE_ENABLE  1
#endif

#if (CCIO_NET_PERFORMACE_ENABLE == 1)
#define CCIO_NET_FM_RAMCODE  PLAT_FM_RAMCODE
#else
#define CCIO_NET_FM_RAMCODE
#endif


#define CCIO_CHAN_MSG_TYPE_MASK   0xFF000000
#define CCIO_CHAN_MSG_CTRL_MASK   0x00FF0000
#define CCIO_CHAN_MSG_CODE_MASK   0x0000FFFF

/* universal msg type flags */
#define CCIO_CHAN_CMSG_FLAG       0x7C000000
#define CCIO_CHAN_DMSG_FLAG       0x7D000000
#define CCIO_CHAN_EMSG_FLAG       0x7E000000

/* customized msg type flag */
#define CCIO_CHAN_CMSG_CUST_FLAG  0x3C000000
#define CCIO_CHAN_DMSG_CUST_FLAG  0x3D000000
#define CCIO_CHAN_EMSG_CUST_FLAG  0x3E000000

/* msg ctrl flag1(0x000F0000): Class Of Service */
#define CCIO_CHAN_MSG_CF_COS0     0x00000000  /* msg with cos 0, e.g. Rx: ctrl/err msg,       Tx: primary channel, mandary */
#define CCIO_CHAN_MSG_CF_COS1     0x00010000  /* msg with cos 1, e.g. Rx: ppp/eth/eos data,   Tx: secondary channel, optional */
#define CCIO_CHAN_MSG_CF_COS2     0x00020000  /* msg with cos 2, e.g. Rx: at/diag/opaq/audio, Tx: customized channel, optional */

/* msg ctrl flag2(0x00F00000) */
#define CCIO_CHAN_MSG_CF_DIN      0x00400000  /* input data to device? for data msg only! */
#define CCIO_CHAN_MSG_CF_IRQ      0x00800000  /* is triggered by IRQ? */

#define CCIO_CHAN_MSG_IS_DIN(msgId)       ((msgId) & CCIO_CHAN_MSG_CF_DIN ? 1 : 0)
#define CCIO_CHAN_MSG_IS_IRQ(msgId)       ((msgId) & CCIO_CHAN_MSG_CF_IRQ ? 1 : 0)

#define CCIO_CHAN_MSG_ID_BUILD(id, isIrq, isDin, cos)    ((id) | ((isIrq) ? CCIO_CHAN_MSG_CF_IRQ : 0) \
                                                               | ((isDin) ? CCIO_CHAN_MSG_CF_DIN : 0) \
                                                               | ((cos) & 0x000F0000))
#define CCIO_CHAN_MSG_BUILD(chmsg, id, chdev, flgs, dat, xtr)   \
            do\
            {\
                ((CcioChanMessage_t*)(chmsg))->msgId       = id;\
                ((CcioChanMessage_t*)(chmsg))->lanDev      = (CcioDevice_t*)(chdev);\
                ((CcioChanMessage_t*)(chmsg))->flags       = (flgs);\
                ((CcioChanMessage_t*)(chmsg))->info.data   = (uint32_t)(dat);\
                ((CcioChanMessage_t*)(chmsg))->extras.data = (uint32_t)(xtr);\
            }while(0)

#define CCIO_CHAN_MSG_TBL_ENTRY(id, idDesc, func)   {id, /*(const int8_t*)(idDesc),*/ (chanMsgHandleFunc)func}

/* ccio internal log API */
#if 1
#define CCIO_TRACE(subId, debugLevel, format, ...)      \
                  ECPLAT_PRINTF(UNILOG_CCIO, subId, debugLevel, format, ##__VA_ARGS__)
#else
#define CCIO_TRACE(subId, debugLevel, format, ...)      \
                  ECPLAT_PRINTF2(UNILOG_CCIO, subId, debugLevel, format, ##__VA_ARGS__)
#endif
#define CCIO_TRACE_OPT(subId, debugLevel, format, ...)  \
                  ECPLAT_PRINTF_OPT(UNILOG_CCIO, subId, debugLevel, format, ##__VA_ARGS__)
#define CCIO_DUMP(subId, debugLevel, format, dumpLen, dump)  \
                  ECPLAT_DUMP(UNILOG_CCIO, subId, debugLevel, format, dumpLen, dump)

/*----------------------------------------------------------------------------*
 *                   DATA TYPE DEFINITION                                     *
 *----------------------------------------------------------------------------*/
#define CCIO_ENTITY_FLAGS_UNDEF   0x00
#define CCIO_ENTITY_FLAGS_INIT    0x01    /* created */
#define CCIO_ENTITY_FLAGS_ACT     0x02    /* activated */
#define CCIO_ENTITY_FLAGS_MUX     0x80    /* multiplexed */
typedef uint8_t CcioEntityFlags_bm;

typedef enum
{
    CUST_RBUF_FOR_DIAG = 0,
    CUST_RBUF_FOR_AT_NORM,
    CUST_RBUF_FOR_AT_CALI,
    CUST_RBUF_FOR_OPAQ,
    CUST_RBUF_FOR_AUDIO,
    CUST_RBUF_FOR_PPP,
    CUST_RBUF_FOR_ETH,
    CUST_RBUF_FOR_CCID,
    CUST_RBUF_FOR_SPI,
#ifdef UNIRTOS_COMMON_MODIFY
    CUST_RBUF_FOR_AT_CMUX,  /* Quectel: dedicated small-buffer pool per CMUX virtual AT DLCI. */
#endif

    CUST_RBUF_USAGE_MAXNUM
}CcioRbufUsage_e;

/**
 * extra info about customized rbuf.
 */
typedef struct
{
    uint8_t   xtraSize;
    uint8_t   isPreGet;
    uint8_t   rsvd[2];
    uint16_t  avlbThres;
    uint16_t  totalSize; /* real size, including xtra size */
}CcioRbufXtras_t;


/* prototype definition */
struct CcioDevice;
struct CcioEntity;

/*-Function prototype for device init functions by driver,
 * @param dev: the device to be initialized.
 */
typedef int32_t (*chdevInitFunc)(struct CcioDevice *dev, uint8_t llsn, void *extras);

/*-This function is called by channel monitor when it wants
 * to send a packet on the device driver.
 * Note: packets to be Tx out should be inserted into the "txHead/txTail" under "CcioDevice_t"
 */
typedef int32_t (*chdevOutFunc)(struct CcioDevice *dev, void *extras);

/*-This function is called by I/O device driver
 * to pass a packet up to the channel monitor.
 */
typedef int32_t (*chdevInCallback)(struct CcioDevice *dev, uint32_t xferCnt, void *extras);
/**This function is called by channel monitor to set configs of the device
 * @param flags: device config event.
 * @param args : device config info.
 */
typedef int32_t (*chdevExecFunc)(uint32_t flags, void *args);
/*-Function prototype for notifying entity that device status is changed.
 * @param flags: device status event.
 * @param args : device status info.
 */
typedef int32_t (*chdevStatusCallback)(uint32_t flags, void *args);

#ifdef UNIRTOS_COMMON_MODIFY
/*-Function prototype for notifying entity that device  is destroyed.
 * @param uartIdx: uartid that was destroyed.
 * @param args : User parameter pointer
 */
typedef void (*chdevDestroyCallback)(uint32_t uartIdx, void *args);

/* Quectel CMUX virtual AT device marker for CcioDevice_t.custFlags (4-bit field).
 * When set with hwtype=CCIO_HT_VIRT + subtype=CSIO_DT_AT, atCmdSetChanName emits
 * "MUXATx" (x from custExtras) so parser aligns atCid with CMS_CHAN_CMUX_x. */
#define CCIO_CUST_FLAG_CMUX_VIRT_AT   0xE
#endif /* UNIRTOS_COMMON_MODIFY */

typedef struct CcioDevice
{
    struct CcioDevice *next;

    uint32_t devId;             /* guid generated by monitor! */

    uint32_t speedType     :2;  /* refer to 'CcioSpeedType_e', via driver */
    uint32_t hwtype        :3;  /* refer to 'CcioHwType_e', via driver */
    uint32_t type          :3;  /* refer to 'CcioDevType_e', via driver */
    uint32_t subtype       :3;  /* more details about device, via driver */
    uint32_t llsn          :6;  /* lower-level serial number, via driver */
    uint32_t dedicate      :1;  /* common/dedicate device(attr), via driver */
    uint32_t isPwOn        :1;  /* the device is power on or off, via monitor */
    uint32_t wkState       :3;  /* refer to 'CcioDevWorkState_e', via monitor */
    uint32_t bmHwAcm       :2;  /* refer to 'CcioDevHwAcm_e', via monitor */
    uint32_t isWaitTxCmplt :1;  /* wait Tx complete IRQ or not, via driver */
    uint32_t asgnTxCos     :2;  /* the assigned tx channel(primary/secondary/customed), via monitor */
    uint32_t owner         :3;
    uint32_t rsvBits       :2;

    uint32_t rbufFlags     :4;  /* which rbuf will be used? refer to 'CcioRbufUsage_e' */
    uint32_t needCleanRb   :1;
    uint32_t waitCleanUlpcb:1;
    uint32_t codecType     :1;
    uint32_t custFlags     :4;  /* flags for customers' private purpose */
    uint32_t custExtras    :16; /* extra info for customers' private purpose */
    uint32_t rsvBits2      :5;
    union
    {
        void    *semaId;        /* for sync, 'osSemaphoreId_t', via driver */
        void    *efId;          /* for sync, 'osEventFlagsId_t', via driver */
    }sync;
    void    *chent;             /* for fast accessing, via monitor */

    chdevOutFunc         chdevOutFn;     /* via driver */
    chdevInCallback      chdevInCb;      /* via monitor */
    chdevExecFunc        chdevExecFn;    /* via driver */
    chdevStatusCallback  chdevStatusCb;  /* via monitor */

    void      *txHead;     /* chain head for tx dlpdu, via driver */
    void      *txTail;     /* chain tail for tx dlpdu, via driver */
    uint32_t   txCount;    /* chain node count */

    void  *extras;     /* privacy field for driver. */
#ifdef UNIRTOS_COMMON_MODIFY
    chdevDestroyCallback chdevDestroyCb;    /* Quectel add for destroy callback */
    void  *argv;    /* Quectel add for save destroy pointer */
    uint32_t uart_id;   /* Quectel add for save uart id */
#endif /* UNIRTOS_COMMON_MODIFY */
}CcioDevice_t;


/**This function is called by CCIO channel monitor
 * when it wants to pass a packet for further handling.
 */
typedef int32_t (*chentInCallback)(void *chent, void *custPbuf, void *extras);
/**This function is called by the l2 layer or same layer entity
 * when it wants to send a packet via a certain of I/O channel Entity.
 */
typedef int32_t (*chentOutFunc)(void *chent, void *custPbuf, void *extras);
/**Function prototype for notifying user that entity status is changed.
 * @param flags: entity status event.
 * @param args : entity status info.
 */
typedef int32_t (*chentStatusCallback)(uint32_t flags, void *args);


typedef struct CcioEntity
{
    struct CcioEntity *next;

    uint32_t chanId;        /* same as chdev->devId */
    uint32_t open     :1;   /* set it TRUE if device has been assigned, via monitor */
    uint32_t chanType :3;   /* channel type set by user, same as chdev->type */
    uint32_t servType :3;   /* service type set by user, same as chdev->subtype */
    uint32_t txDelay  :1;   /* atcmd/netdata is delayed to be delivered to Tx task */
    uint32_t rsvdBit  :24;
    void    *chdev;         /* init & enable it, via monitor */
    void    *rbuf;          /* rbuf to recv data, via monitor */

    chentOutFunc         chentOutFn;     /* via monitor */
    chentInCallback      chentInCb;      /* via monitor */
    chentStatusCallback  chentStatusCb;  /* via user */
}CcioEntity_t;

typedef struct EnetEntity
{
    struct EnetEntity *next;

    void    *baseEnt;       /* for fast access */
    void    *dlcCtx;        /* dlc context */
    uint8_t  flags;         /* CcioEntityFlags_bm */
    uint8_t  netCid;        /* netif context Id between ps, use CcioLanMediaType_e instead for now */
    uint8_t  lanMedia;      /* refer to 'CcioLanMediaType_e', via monitor/user */
    uint8_t  lanState;      /* refer to 'CcioLanMediaState_e', via monitor/user */

    uint8_t  pdp4Cid;       /* ipv4 pdp context Id, via monitor */
    uint8_t  pdp6Cid;       /* ipv6 pdp context Id, via monitor */
    uint16_t isWanAvlb  :1;
    uint16_t isLanUp    :1;
    uint16_t notifAgain :1;
    uint16_t isLanSusp  :1; /* lan device is suspended or not? */
    uint16_t rsvdBits   :12;

    /* TODO: statistic, via monitor */
}EnetEntity_t;

typedef struct
{
    /* inherited field & MUST be placed on the top! */
    CcioEntity_t  base;

    EnetEntity_t  eth;

    void    *extras;        /* for user context. */

    /* TODO: statistic, via monitor */
}EtherEntity_t;

typedef struct
{
    uint32_t      msgId;
    CcioDevice_t *lanDev;      /* src/dest lan chdev, NULL for monitor */
    /*
     * [flags/info/xtras]: customed field, fill whatever you want!
     */
    uint32_t      flags;
    union
    {
        uint32_t  data;
        uint8_t  *dataPtr;
    }info;
    union
    {
        uint32_t  data;
        uint8_t  *dataPtr;
    }extras;
}CcioChanMessage_t;

typedef int32_t (*chanMsgBuildFunc)(CcioChanMessage_t *chmsg, uint32_t msgId, void *args);
typedef int32_t (*chanMsgHandleFunc)(CcioChanMessage_t *chmsg);

typedef void (*chanProviderInitFunc)(CcioDevice_t *chdev);
typedef void (*chanProviderDeinitFunc)(CcioEntity_t *chent);
typedef void (*chanProviderAlterFunc)(CcioEntity_t *chent, uint32_t newType);

/*----------------------------------------------------------------------------*
 *                    GLOBAL FUNCTIONS DECLEARATION                           *
 *----------------------------------------------------------------------------*/
int32_t ccioSendMsgToRxTask(uint32_t msgId, void *args, chanMsgBuildFunc buildFn);
int32_t ccioSendMsgToTxTask(uint32_t msgId, void *args, chanMsgBuildFunc buildFn);


#ifdef __cplusplus
}
#endif
#endif

