#pragma once

#include <os/os.h>
#include "PPCS_API.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CS2_IMG_MAX_TX_BUFFER_THD   ((PPCS_TX_BUFFER_THD * 12)/16)
#define CS2_AUD_MAX_TX_BUFFER_THD   (PPCS_TX_BUFFER_THD - CS2_IMG_MAX_TX_BUFFER_THD)

#define CMD_P2P_CHANNEL     (0)
#define AUD_P2P_CHANNEL     (1)
#define VIDEO_P2P_CHANNEL     (2)

#define SIZE_DID            64  // Device ID Size
#define SIZE_APILICENSE     24  // APILicense Size
#define SIZE_INITSTRING     256 // InitString Size
#define SIZE_WAKEUP_KEY     17  // WakeUp Key Size
#define SIZE_ARMINO_KEY     17  // WakeUp Key Size

#define TIME_USE                (int)((end.tick_msec) - (begin.tick_msec))
#define TU_MS(begin, end)     (int)((end.tick_msec) - (begin.tick_msec))

#define SIZE_DATE      32   //// "[YYYY-MM-DD hh:mm:ss.xxx]"


typedef struct
{
	int year;
	int mon;
	int day;
	int week;
	int hour;
	int min;
	int sec;
	int msec;
	unsigned long tick_sec;
	unsigned long long tick_msec;
	char date[SIZE_DATE];
} time_info_t;

typedef struct
{
	int  skt;                       // Sockfd
	// struct sockaddr_in RemoteAddr;  // Remote IP:Port
	// struct sockaddr_in MyLocalAddr; // My Local IP:Port
	// struct sockaddr_in MyWanAddr;   // My Wan IP:Port
	char remote_ip[16];
	int remote_port;
	char local_ip[16];
	int local_port;
	char wan_ip[16];
	int wan_port;
	unsigned int connect_time;       // Connection build in ? Sec Before
	char did[24];                   // Device ID
	char bcord;   // I am Client or Device, 0: Client, 1: Device
	char bmode; // my define mode by PPCS_Check bmode(0:P2P(Including: LAN TCP/UDP), 1:RLY, 2:TCP); Mydefine: 0:LAN, 1:P2P, 2:RLY, 3:TCP, 4:RP2P.
	char mode[12];   // Connection mode: LAN/P2P/RLY/TCP.
	char reserved[2];
} session_info_t;

typedef struct
{
	char did[SIZE_DID];
	char apilicense[SIZE_APILICENSE];
	char initstring[SIZE_INITSTRING];
	char key[SIZE_ARMINO_KEY];
	char pwakeupkey[SIZE_WAKEUP_KEY];
	bool cs2_started;
} p2p_cs2_key_t;


INT32 get_socket_type(INT32 skt);
const char *get_p2p_error_code_info(int err);
int is_lan_cmp(const char *IP1, const char *IP2);
void cs2_p2p_get_time(time_info_t *pt);
int get_session_info(int SessionID, session_info_t *MySInfo);
int cs2_p2p_listen(const char *did, const char *APILicense, unsigned long Repeat, uint8_t *is_run);

void show_network(st_PPCS_NetInfo NetInfo);
#ifdef __cplusplus
}
#endif
