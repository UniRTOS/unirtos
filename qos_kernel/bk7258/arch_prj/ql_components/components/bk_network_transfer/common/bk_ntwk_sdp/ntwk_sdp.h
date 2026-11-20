
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define UDP_SDP_LOCAL_PORT              (10000)
#define UDP_SDP_REMOTE_PORT             (52110)

#define ADV_ALLOC_LEN    (1024)

typedef struct
{
	char *data;
	uint16_t length;
} sdp_data_t;

int ntwk_sdp_start(const char *name, uint32_t cmd_port, uint32_t img_port, uint32_t aud_port);
int ntwk_sdp_stop(void);
int ntwk_sdp_reload(uint32_t time_ms);

#ifdef __cplusplus
}
#endif
