#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <lwip/sockets.h>
#include <os/os.h>
#include <os/mem.h>
#include "PPCS_API.h"
#include "PPCS_cs2_comm.h"

#define TAG "PPCS-cs2-comm"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)



INT32 get_socket_type(INT32 skt)
{
    socklen_t length = sizeof(unsigned int);
    int type;
    getsockopt(skt, SOL_SOCKET, SO_TYPE, (char *)&type, &length);

    if (type == SOCK_STREAM)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

const char *get_p2p_error_code_info(int err)
{
    switch (err)
    {
        case 0:
            return "ERROR_PPCS_SUCCESSFUL";

        case -1:
            return "ERROR_PPCS_NOT_INITIALIZED"; // API didn't initialized

        case -2:
            return "ERROR_PPCS_ALREADY_INITIALIZED";

        case -3:
            return "ERROR_PPCS_TIME_OUT";

        case -4:
            return "ERROR_PPCS_INVALID_ID";//Invalid Device ID !!

        case -5:
            return "ERROR_PPCS_INVALID_PARAMETER";

        case -6:
            return "ERROR_PPCS_DEVICE_NOT_ONLINE";

        case -7:
            return "ERROR_PPCS_FAIL_TO_RESOLVE_NAME";

        case -8:
            return "ERROR_PPCS_INVALID_PREFIX";//Prefix of Device ID is not accepted by Server !!

        case -9:
            return "ERROR_PPCS_ID_OUT_OF_DATE";

        case -10:
            return "ERROR_PPCS_NO_RELAY_SERVER_AVAILABLE";

        case -11:
            return "ERROR_PPCS_INVALID_SESSION_HANDLE";

        case -12:
            return "ERROR_PPCS_SESSION_CLOSED_REMOTE";

        case -13:
            return "ERROR_PPCS_SESSION_CLOSED_TIMEOUT";

        case -14:
            return "ERROR_PPCS_SESSION_CLOSED_CALLED";

        case -15:
            return "ERROR_PPCS_REMOTE_SITE_BUFFER_FULL";

        case -16:
            return "ERROR_PPCS_USER_LISTEN_BREAK";//Listen break is called !!

        case -17:
            return "ERROR_PPCS_MAX_SESSION";//Exceed max session !!

        case -18:
            return "ERROR_PPCS_UDP_PORT_BIND_FAILED";//The specified UDP port can not be binded !!

        case -19:
            return "ERROR_PPCS_USER_CONNECT_BREAK";

        case -20:
            return "ERROR_PPCS_SESSION_CLOSED_INSUFFICIENT_MEMORY";

        case -21:
            return "ERROR_PPCS_INVALID_APILICENSE";//API License code is not correct !!

        case -22:
            return "ERROR_PPCS_FAIL_TO_CREATE_THREAD";//Fail to Create Thread !!

        case -23:
            return "ERROR_PPCS_INVALID_DSK";

        case -24:
            return "ERROR_PPCS_FAILED_TO_CONNECT_TCP_RELAY";

        case -25:
            return "ERROR_PPCS_FAIL_TO_ALLOCATE_MEMORY"; // only availeable since P2P API ver: 4.2.0.0

        default:
            return "Unknown, something is wrong";
    }
} // get_p2p_error_code_info


const char *get_listen_error_info(int ret)
{
    switch (ret)
    {
        case ERROR_PPCS_NOT_INITIALIZED:
            return "API didn't initialized";

        case ERROR_PPCS_TIME_OUT:
            return "Listen time out, No client connect me !!";

        case ERROR_PPCS_INVALID_ID:
            return "Invalid Device ID !!";

        case ERROR_PPCS_INVALID_PREFIX:
            return "Prefix of Device ID is not accepted by Server !!";

        case ERROR_PPCS_UDP_PORT_BIND_FAILED:
            return "The specified UDP port can not be binded !!";

        case ERROR_PPCS_MAX_SESSION:
            return "Exceed max session !!";

        case ERROR_PPCS_USER_LISTEN_BREAK:
            return "Listen break is called !!";

        case ERROR_PPCS_INVALID_APILICENSE:
            return "API License code is not correct !!";

        case ERROR_PPCS_FAIL_TO_CREATE_THREAD:
            return "Fail tO Create Thread !!";

        default:
            return get_p2p_error_code_info(ret);
    }
}

void show_network(st_PPCS_NetInfo NetInfo)
{
    LOGD("-------------- NetInfo: -------------------\n");
    LOGD("Internet Reachable     : %s\n", (NetInfo.bFlagInternet == 1) ? "YES" : "NO");
    LOGD("P2P Server IP resolved : %s\n", (NetInfo.bFlagHostResolved == 1) ? "YES" : "NO");
    LOGD("P2P Server Hello Ack   : %s\n", (NetInfo.bFlagServerHello == 1) ? "YES" : "NO");

    switch (NetInfo.NAT_Type)
    {
        case 0:
            LOGD("Local NAT Type         : Unknow\n");
            break;

        case 1:
            LOGD("Local NAT Type         : IP-Restricted Cone\n");
            break;

        case 2:
            LOGD("Local NAT Type         : Port-Restricted Cone\n");
            break;

        case 3:
            LOGD("Local NAT Type         : Symmetric\n");
            break;

        case 4:
            LOGD("Local NAT Type         : Different Wan IP Detected!!\n");
            break;
    }

    LOGD("My Wan IP : %s\n", NetInfo.MyWanIP);
    LOGD("My Lan IP : %s\n", NetInfo.MyLanIP);
    LOGD("-------------------------------------------\n");
}

int is_lan_cmp(const char *IP1, const char *IP2)
{
    short Len_IP1 = strlen(IP1);
    short Len_IP2 = strlen(IP2);

    if (!IP1 || 7 > Len_IP1 || !IP2 || 7 > Len_IP2)
    {
        return -1;
    }

    if (0 == strcmp(IP1, IP2))
    {
        return 1; //YES
    }

    const char *pIndex = IP1 + Len_IP1 - 1;

    while (1)
    {
        if ('.' == *pIndex || pIndex == IP1)
        {
            break;
        }
        else
        {
            pIndex--;
        }
    }

    if (0 == strncmp(IP1, IP2, (int)(pIndex - IP1)))
    {
        return 1; //YES
    }

    return 0; //NO
}

void cs2_p2p_get_time(time_info_t *pt)
{

    struct timeval tmv;
    int ret = gettimeofday(&tmv, NULL);

    if (0 != ret)
    {
        printf("gettimeofday failed!! errno=%d\n", errno);
        memset(pt, 0, sizeof(time_info_t));
        return ;
    }

    //struct tm *ptm = localtime((const time_t *)&tmv.tv_sec);
    struct tm stm = {0};
    struct tm *ptm = localtime_r((const time_t *)&tmv.tv_sec, &stm);

    if (!ptm)
    {
        printf("localtime_r failed!!\n");
        os_memset(pt, 0, sizeof(time_info_t));
        pt->tick_msec = ((unsigned long long)tmv.tv_sec) * 1000 + tmv.tv_usec / 1000; // ->ms
    }
    else
    {
        pt->year = stm.tm_year + 1900;
        pt->mon = stm.tm_mon + 1;
        pt->day = stm.tm_mday;
        pt->week = stm.tm_wday;
        pt->hour = stm.tm_hour;
        pt->min = stm.tm_min;
        pt->sec = stm.tm_sec;
        pt->msec = (int)(tmv.tv_usec / 1000);
        pt->tick_sec = tmv.tv_sec; // 1970年1月1日0点至今的秒数。
        pt->tick_msec = ((unsigned long long)tmv.tv_sec) * 1000 + tmv.tv_usec / 1000; // ->ms
        os_memset(pt->date, 0, sizeof(pt->date));
        snprintf(pt->date, sizeof(pt->date) - 1, "%04d-%02d-%02d %02d:%02d:%02d.%03d", pt->year, pt->mon, pt->day, pt->hour, pt->min, pt->sec, pt->msec);
    }
}

int get_session_info(int SessionID, session_info_t *MySInfo)
{
    memset(MySInfo, 0, sizeof(session_info_t));

    st_PPCS_Session Sinfo;

    int ret = PPCS_Check(SessionID, &Sinfo);

    if (ERROR_PPCS_SUCCESSFUL == ret)
    {
        MySInfo->skt = Sinfo.Skt;
        // Remote addr
        snprintf(MySInfo->remote_ip, sizeof(MySInfo->remote_ip), "%s", inet_ntoa(Sinfo.RemoteAddr.sin_addr));
        MySInfo->remote_port = ntohs(Sinfo.RemoteAddr.sin_port);
        // Lan addr
        snprintf(MySInfo->local_ip, sizeof(MySInfo->local_ip), "%s", inet_ntoa(Sinfo.MyLocalAddr.sin_addr));
        MySInfo->local_port = ntohs(Sinfo.MyLocalAddr.sin_port);
        // Wan addr
        snprintf(MySInfo->wan_ip, sizeof(MySInfo->wan_ip), "%s", inet_ntoa(Sinfo.MyWanAddr.sin_addr));
        MySInfo->wan_port = ntohs(Sinfo.MyWanAddr.sin_port);

        MySInfo->connect_time = Sinfo.ConnectTime;
        memcpy(MySInfo->did, Sinfo.DID, strlen(Sinfo.DID));
        MySInfo->bcord = Sinfo.bCorD;

        if (0 == Sinfo.bMode)
        {
            if (Sinfo.RemoteAddr.sin_addr.s_addr == Sinfo.MyLocalAddr.sin_addr.s_addr || 1 == is_lan_cmp(MySInfo->local_ip, MySInfo->remote_ip))
            {
                MySInfo->bmode = 0;
                1 == get_socket_type(Sinfo.Skt) ? memcpy(MySInfo->mode, "LAN.", 4) : memcpy(MySInfo->mode, "LAN", 3);
            }
            else
            {
                MySInfo->bmode = 1;
                memcpy(MySInfo->mode, "P2P", 3);
            }
        }
        else if (1 == Sinfo.bMode)
        {
            MySInfo->bmode = 2;
            memcpy(MySInfo->mode, "RLY", 3);
        }
        else if (2 == Sinfo.bMode)
        {
            MySInfo->bmode = 3;
            memcpy(MySInfo->mode, "TCP", 3);
        }
        else if (3 == Sinfo.bMode)
        {
            MySInfo->bmode = 4;    //// support by P2P API 5.0.8
            memcpy(MySInfo->mode, "RP2P", 4);
        }
    }

    // else LOGD("PPCS_Check(SessionID=%d) ret=%d [%s]\n", SessionID, ret, get_p2p_error_code_info(ret));
    return ret;
}

int cs2_p2p_listen(const char *did, const char *APILicense, unsigned long Repeat, uint8_t *is_run)
{
#ifdef TIME_SHOW
    time_info_t t1, t2;
#endif

    int ret = -1;
    int SessionID = -99;

    unsigned int TimeOut_Sec = 120;
    unsigned short UDP_Port = 0;// PPCS_Listen 端口填 0 让底层自动分配。
    char bEnableInternet = 2;

#ifdef TIME_SHOW
    os_memset(&t2, 0, sizeof(t2));

    cs2_p2p_get_time(&t1);
#endif

    do
    {
        ret = PPCS_Listen(did, TimeOut_Sec, UDP_Port, bEnableInternet, APILicense);

        LOGD("PPCS_Listen timeout\n");
    }
    while (ERROR_PPCS_TIME_OUT == ret && *is_run);

    if (1 == Repeat)
    {
        LOGD("%02lu-PPCS_Listen(%s,%d,%d,%d,%s) ...\n", Repeat, did, TimeOut_Sec, UDP_Port, bEnableInternet, APILicense);
    }
    else
    {
        LOGD("%02lu-PPCS_Listen(%s,%d,%d,%d,%s) ...\n", Repeat, did, TimeOut_Sec, UDP_Port, bEnableInternet, APILicense);
    }

#ifdef TIME_SHOW
    cs2_p2p_get_time(&t2);
#endif

    if (*is_run == 0)
    {
        LOGD("p2p", "%s is_run is 0, exit\n", __func__);

        if (ret >= 0)
        {
            PPCS_ForceClose(ret);
        }

        return -1;
    }

    if (0 > ret)
    {
#ifdef TIME_SHOW
        LOGD("[%s] %02lu-PPCS_Listen failed:%d ms, ret=%d %s\n", t2.date, Repeat, TU_MS(t1, t2), ret, get_listen_error_info(ret));
#else
        LOGD("%02lu-PPCS_Listen failed, ret=%d %s\n", Repeat, ret, get_listen_error_info(ret));
#endif
        return ret;
    }
    else //// ret >= 0, Listen OK, new client connect in.
    {
        SessionID = ret; // 每个 >=0 的 SessionID 都是一个正常的连接，本 sample 是单用户连接范例，多用户端连接注意要保留区分每一个 PPCS_Listen >=0 的 SessionID, 当连接断开或者 SessionID 不用时，必须要调 PPCS_Close(SessionID)/PPCS_ForceClose(SessionID) 关闭连线释放资源。
        //      PPCS_Share_Bandwidth(0); // 当有连接进来，关闭设备转发功能。

        LOGD("PPCS_Share_Bandwidth(0) is Called!!\n");
    }

#ifdef SESSION_PRINT
    {
        session_info_t session_info;

        if (ERROR_PPCS_SUCCESSFUL != (ret = get_session_info(SessionID, &session_info)))
        {
            LOGD("%02lu-did=%s,Session=%d,RmtAddr=Unknown (PPCS_Check:%d)\n", Repeat, did, SessionID, ret);
            return SessionID;
        }
    }
#endif
    return SessionID;
}



