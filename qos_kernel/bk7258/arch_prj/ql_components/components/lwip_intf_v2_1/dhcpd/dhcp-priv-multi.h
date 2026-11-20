#ifndef __DHCP_PRIV_MULTI_H__
#define __DHCP_PRIV_MULTI_H__

#include "dhcp-priv.h"

#define DHCP_SERVER_INST_MAX   2

int dhcp_server_init(struct dhcp_server_data *d, void *intrfc_handle);
int dhcp_server_add_instance(void *intrfc_handle);
int dhcp_request_stop(void *intrfc_handle);
int dhcpd_lock(void);
void dhcpd_unlock(void);

extern volatile bool dhcpd_thread_running;

#endif
