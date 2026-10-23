#include <string.h>
#include <os/os.h>
#include "dhcp-priv-multi.h"

static beken_thread_t dhcpd_thread;

extern void *net_get_uap_handle(void);

int dhcp_server_start(void *intrfc_handle)
{
	int ret;
	int slot;

	dhcp_d("DHCP server start request \r\n");

	dhcp_enable_nack_dns_server();

	slot = dhcp_server_add_instance(intrfc_handle);
	if (slot < 0)
		return -1;

	if (dhcpd_lock() != 0) {
		dhcp_request_stop(intrfc_handle);
		return -1;
	}

	if (dhcpd_thread_running) {
		dhcpd_unlock();
		return 0;	/* shared thread already serving other instances */
	}

	dhcpd_thread_running = true;
	dhcpd_unlock();
	ret = rtos_create_thread(&dhcpd_thread,
		BEKEN_APPLICATION_PRIORITY,
		"dhcp-server",
		(beken_thread_function_t)dhcp_server,
		DHCP_SERVER_TASK_STACK_SIZE,
		0);
	if (ret) {
		if (dhcpd_lock() == 0) {
			dhcpd_thread_running = false;
			dhcpd_unlock();
		} else {
			dhcpd_thread_running = false;
		}
		dhcp_request_stop(intrfc_handle);
		return -1;
	}

	return 0;
}

/* Stop the DHCP instance bound to a specific interface. */
void dhcp_server_stop_iface(void *intrfc_handle)
{
	dhcp_d("DHCP server stop request (iface)\r\n");
	dhcp_request_stop(intrfc_handle);
}

/* Legacy entry: stop the primary (1st AP) DHCP instance. */
void dhcp_server_stop(void)
{
	dhcp_d("DHCP server stop request\r\n");
	dhcp_request_stop(net_get_uap_handle());
}
// eof
