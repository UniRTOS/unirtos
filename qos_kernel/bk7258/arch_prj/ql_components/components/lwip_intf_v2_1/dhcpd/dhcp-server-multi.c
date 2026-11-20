#include <string.h>
#include <os/os.h>
#include "dhcp-bootp.h"
#include "dns.h"
#include "dhcp-priv-multi.h"
#include <os/str.h>
#include <os/mem.h>
#include "lwip/etharp.h"
#include "lwip/sockets.h"
#include "opt.h"
#include "net.h"

#define os_mem_alloc os_malloc
#define os_mem_free  os_free
#define SEND_RESPONSE(w,x,y,z)	send_response(w,x,y,z)

#define BOOTP_MIN_REPLY_SIZE             300

#define DEFAULT_DHCP_ADDRESS_TIMEOUT	(60U*60U*1U) /* 1 hour */
#define CLIENT_IP_NOT_FOUND              0x00000000

uint32_t dhcp_address_timeout = DEFAULT_DHCP_ADDRESS_TIMEOUT;
static beken_mutex_t dhcpd_mutex;
static volatile uint8_t dhcpd_mutex_state;
volatile bool dhcpd_thread_running;
static int (*dhcp_nack_dns_server_handler)(char *msg, int len,
					   struct sockaddr_in *fromaddr);

/* Multi-instance DHCP server for SoftAP + P2P GO coexistence. */
struct dhcp_server_data dhcp_inst[DHCP_SERVER_INST_MAX];
volatile bool dhcp_inst_used[DHCP_SERVER_INST_MAX];
volatile bool dhcp_inst_stopping[DHCP_SERVER_INST_MAX];
volatile bool dhcp_inst_inited[DHCP_SERVER_INST_MAX];
static struct dhcp_server_data *g_dhcps_cur = &dhcp_inst[0];
#define dhcps (*g_dhcps_cur)

static int dhcpd_mutex_ensure_init(void)
{
	while (1) {
		if (dhcpd_mutex_state == 2)
			return 0;

		if (dhcpd_mutex_state == 0 &&
		    __sync_bool_compare_and_swap(&dhcpd_mutex_state, 0, 1)) {
			if (rtos_init_mutex(&dhcpd_mutex) != 0) {
				dhcpd_mutex_state = 0;
				return -1;
			}
			__sync_synchronize();
			dhcpd_mutex_state = 2;
			return 0;
		}

		rtos_delay_milliseconds(1);
	}
}

int dhcpd_lock(void)
{
	if (dhcpd_mutex_ensure_init() != 0)
		return -1;
	return rtos_lock_mutex(&dhcpd_mutex);
}

void dhcpd_unlock(void)
{
	if (dhcpd_mutex_state == 2)
		rtos_unlock_mutex(&dhcpd_mutex);
}

extern int net_get_if_macaddr(void *macaddr, void *intrfc_handle);
extern int net_get_if_ip_addr(uint32_t *ip, void *intrfc_handle);
extern int net_get_if_ip_mask(uint32_t *nm, void *intrfc_handle);
extern int net_get_if_gw_addr(uint32_t *ip, void *intrfc_handle);
static void get_broadcast_addr(struct sockaddr_in *addr);
static int get_ip_addr_from_interface(uint32_t *ip, void *interface_handle);
static int get_netmask_from_interface(uint32_t *nm, void *interface_handle);
static int get_mac_addr_from_interface(void *mac, void *interface_handle);
static int get_gateway_from_interface(uint32_t *gw, void *interface_handle);
static int send_gratuitous_arp(uint32_t ip);
static bool ac_add(uint8_t *chaddr, uint32_t client_ip);
static uint32_t ac_lookup_mac(uint8_t *chaddr);
static uint8_t *ac_lookup_ip(uint32_t client_ip);
static bool ac_not_full();

static bool ac_add(uint8_t *chaddr, uint32_t client_ip)
{
	/* adds ip-mac mapping in cache */
	if (ac_not_full()) {
		dhcps.ip_mac_mapping[dhcps.count_clients].client_mac[0]
			= chaddr[0];
		dhcps.ip_mac_mapping[dhcps.count_clients].client_mac[1]
			= chaddr[1];
		dhcps.ip_mac_mapping[dhcps.count_clients].client_mac[2]
			= chaddr[2];
		dhcps.ip_mac_mapping[dhcps.count_clients].client_mac[3]
			= chaddr[3];
		dhcps.ip_mac_mapping[dhcps.count_clients].client_mac[4]
			= chaddr[4];
		dhcps.ip_mac_mapping[dhcps.count_clients].client_mac[5]
			= chaddr[5];
		dhcps.ip_mac_mapping[dhcps.count_clients].client_ip = client_ip;
		dhcps.count_clients++;
		return 0;
	}
	return -1;
}

static uint32_t ac_lookup_mac(uint8_t *chaddr)
{
	/* returns ip address, if mac address is present in cache */
	int i;
	for (i = 0; i < dhcps.count_clients && i < MAC_IP_CACHE_SIZE; i++) {
		if ((dhcps.ip_mac_mapping[i].client_mac[0] == chaddr[0]) &&
		    (dhcps.ip_mac_mapping[i].client_mac[1] == chaddr[1]) &&
		    (dhcps.ip_mac_mapping[i].client_mac[2] == chaddr[2]) &&
		    (dhcps.ip_mac_mapping[i].client_mac[3] == chaddr[3]) &&
		    (dhcps.ip_mac_mapping[i].client_mac[4] == chaddr[4]) &&
		    (dhcps.ip_mac_mapping[i].client_mac[5] == chaddr[5])) {
			return dhcps.ip_mac_mapping[i].client_ip;
		}
	}
	return CLIENT_IP_NOT_FOUND;
}

static uint8_t *ac_lookup_ip(uint32_t client_ip)
{
	/* returns mac address, if ip address is present in cache */
	int i;
	for (i = 0; i < dhcps.count_clients && i < MAC_IP_CACHE_SIZE; i++) {
		if ((dhcps.ip_mac_mapping[i].client_ip)	== client_ip) {
			return dhcps.ip_mac_mapping[i].client_mac;
		}
	}
	return NULL;
}

static bool ac_not_full()
{
	/* returns true if cache is not full */
	return (dhcps.count_clients < MAC_IP_CACHE_SIZE);
}

static bool ac_valid_ip(uint32_t requested_ip)
{
	/* skip over our own address, the network address or the
	 * broadcast address
	 */
	if (requested_ip == ntohl(dhcps.my_ip) ||
	       (requested_ip == ntohl(dhcps.my_ip &
					  dhcps.netmask)) ||
	       (requested_ip == ntohl((dhcps.my_ip |
					   (0xffffffff & ~dhcps.netmask))))) {
		return false;
	}
	if (ac_lookup_ip(htonl(requested_ip)) != NULL)
		return false;
	return true;
}

static void write_u32(char *dest, uint32_t be_value)
{
	*dest++ = be_value & 0xFF;
	*dest++ = (be_value >> 8) & 0xFF;
	*dest++ = (be_value >> 16) & 0xFF;
	*dest = be_value >> 24;
}

/* Configure the DHCP dynamic IP lease time*/
int dhcp_server_lease_timeout(uint32_t val)
{
	if ((val == 0) || (val > (60U*60U*24U*49700U))) {
		return -EINVAL;
	} else {
		dhcp_address_timeout = val;
		return 0;
	}
}

/* calculate the address to give out to the next DHCP DISCOVER request
 *
 * DHCP clients will be assigned addresses in sequence in the subnet's address space.
 */
static unsigned int next_yiaddr()
{
#ifdef CONFIG_DHCP_SERVER_DEBUG
	struct in_addr ip;
#endif
	uint32_t new_ip;
	struct bootp_header *hdr = (struct bootp_header *)dhcps.msg;

	/* if device requesting for ip address is already registered,
	 * if yes, assign previous ip address to it
	 */
	new_ip = ac_lookup_mac(hdr->chaddr);
	if (new_ip == (CLIENT_IP_NOT_FOUND)) {
		/* next IP address in the subnet */
		dhcps.current_ip = ntohl(dhcps.my_ip & dhcps.netmask) |
			((dhcps.current_ip + 1) & ntohl(~dhcps.netmask));
		while (!ac_valid_ip(dhcps.current_ip)) {
			dhcps.current_ip = ntohl(dhcps.my_ip & dhcps.netmask) |
				((dhcps.current_ip + 1) &
				 ntohl(~dhcps.netmask));
		}

		new_ip = htonl(dhcps.current_ip);

		if (ac_add(hdr->chaddr, new_ip) !=
		    0)
			dhcp_w("No space to store new mapping..\r\n");
	}

#ifdef CONFIG_DHCP_SERVER_DEBUG
	ip.s_addr = new_ip;
	dhcp_d("New client IP will be %s\r\n", inet_ntoa(ip));
	ip.s_addr = dhcps.my_ip & dhcps.netmask;
#endif

	return new_ip;
}
#if IP_NAPT
extern const ip_addr_t *sta_dns;
#endif
#include "bk_wifi_types.h"
extern ap_param_t *g_ap_param_ptr;
static unsigned int make_response(char *msg, enum dhcp_message_type type)
{
	struct bootp_header *hdr;
	struct bootp_option *opt;
	char *offset = msg;

	hdr = (struct bootp_header *)offset;
	hdr->op = BOOTP_OP_RESPONSE;
	hdr->htype = 1;
	hdr->hlen = 6;
	hdr->hops = 0;
	hdr->ciaddr = 0;
	hdr->yiaddr = (type == DHCP_MESSAGE_ACK) ? dhcps.client_ip : 0;
	hdr->yiaddr = (type == DHCP_MESSAGE_OFFER) ?
	next_yiaddr() : hdr->yiaddr;
#ifndef CONFIG_FUZZ_TEST
	hdr->siaddr = 0;
#else
	hdr->siaddr = dhcps.my_ip;
#endif
	hdr->riaddr = 0;
	offset += sizeof(struct bootp_header);

	opt = (struct bootp_option *)offset;
	opt->type = BOOTP_OPTION_DHCP_MESSAGE;
	*(uint8_t *) opt->value = type;
	opt->length = 1;
	offset += sizeof(struct bootp_option) + opt->length;

	if (type == DHCP_MESSAGE_NAK)
		return (unsigned int)(offset - msg);

	opt = (struct bootp_option *)offset;
	opt->type = BOOTP_OPTION_SUBNET_MASK;
	write_u32(opt->value, dhcps.netmask);
	opt->length = 4;
	offset += sizeof(struct bootp_option) + opt->length;

	opt = (struct bootp_option *)offset;
	opt->type = BOOTP_OPTION_ADDRESS_TIME;
	write_u32(opt->value, htonl(dhcp_address_timeout));
	opt->length = 4;
	offset += sizeof(struct bootp_option) + opt->length;

	opt = (struct bootp_option *)offset;
	opt->type = BOOTP_OPTION_DHCP_SERVER_ID;
	write_u32(opt->value, dhcps.my_ip);
	opt->length = 4;
	offset += sizeof(struct bootp_option) + opt->length;

	opt = (struct bootp_option *)offset;
	opt->type = BOOTP_OPTION_ROUTER;
	write_u32(opt->value, dhcps.router_ip);
	opt->length = 4;
	offset += sizeof(struct bootp_option) + opt->length;

	if (g_ap_param_ptr && !g_ap_param_ptr->disable_dns_server) {
		opt = (struct bootp_option *)offset;
		opt->type = BOOTP_OPTION_NAMESERVER;
		if (dhcp_nack_dns_server_handler) {
#if !IP_NAPT
			write_u32(opt->value, dhcps.router_ip);
#else
			if(!sta_dns)
				write_u32(opt->value, dhcps.router_ip);
			else
				write_u32(opt->value, ip4_addr_get_u32(ip_2_ip4(sta_dns)));
#endif
		}
		else
			write_u32(opt->value, 0);
		opt->length = 4;
		offset += sizeof(struct bootp_option) + opt->length;
	}
	opt = (struct bootp_option *)offset;
	opt->type = BOOTP_END_OPTION;
	offset++;

	if ((offset - msg) < BOOTP_MIN_REPLY_SIZE) {
		memset(offset, 0, BOOTP_MIN_REPLY_SIZE - (offset - msg));
		offset = msg + BOOTP_MIN_REPLY_SIZE;
	}

	return (unsigned int)(offset - msg);
}

int dhcp_get_ip_from_mac(uint8_t *client_mac, uint32_t *client_ip)
{
	*client_ip = ac_lookup_mac(client_mac);
	if (*client_ip == CLIENT_IP_NOT_FOUND) {
		return -1;
	}
	return 0;
}

extern void ap_set_default_netif(void);
extern void reset_default_netif(void);

static int send_response(int sock, struct sockaddr *addr, char *msg, int len)
{
	int nb;
	unsigned int sent = 0;

	while (sent < len) {
		nb = lwip_sendto(sock, msg + sent, len - sent, 0, addr,
			    sizeof(struct sockaddr_in));
		if (nb < 0) {
			dhcp_e("failed to send response, addr %p:%d\r\n", 
				((struct sockaddr_in *)addr)->sin_addr,
				((struct sockaddr_in *)addr)->sin_port);
			return -1;
		}
		sent += nb;
	}

	dhcp_d("sent response, %d bytes %s\r\n", sent,
	    inet_ntoa(((struct sockaddr_in *)addr)->sin_addr));
	return 0;
}

#define ERROR_REFUSED 5

static const char *dhcp_msg_type_str(uint8_t type)
{
	switch (type) {
	case DHCP_MESSAGE_OFFER:
		return "OFFER";
	case DHCP_MESSAGE_ACK:
		return "ACK";
	case DHCP_MESSAGE_NAK:
		return "NAK";
	default:
		return "UNKNOWN";
	}
}

static int process_dns_message(char *msg, int len, struct sockaddr_in *fromaddr)
{
	struct dns_header *hdr;
	char *endp = msg + len;
	int nq;

	if (len < sizeof(struct dns_header)) {
		dhcp_e("DNS request is not complete, hence ignoring it\r\n");
		return -1;
	}

	hdr = (struct dns_header *)msg;

	dhcp_d("DNS transaction id: 0x%x\r\n", htons(hdr->id));

	if (hdr->flags.fields.qr) {
		dhcp_e("ignoring this dns message (not a query)\r\n");
		return -1;
	}

	nq = ntohs(hdr->num_questions);
	dhcp_d("we were asked %d questions\r\n", nq);

	if (nq <= 0) {
		dhcp_e("ignoring this dns msg (not a query or 0 questions)\r\n");
		return -1;
	}

	/* make the header represent a response */
	hdr->flags.fields.qr = 1;
	hdr->flags.fields.opcode = 0;
	/* Errors are never authoritative (unless they are
	   NXDOMAINS, which this is not) */
	hdr->flags.fields.aa = 0;
	hdr->flags.fields.tc = 0;
	hdr->flags.fields.rd = 1;
	hdr->flags.fields.ra = 0;
	hdr->flags.fields.rcode = ERROR_REFUSED;
	hdr->flags.num = htons(hdr->flags.num);
	/* number of entries in questions section */
	hdr->num_questions  = htons(0x01);
	hdr->answer_rrs = 0; /* number of resource records in answer section */
	hdr->authority_rrs = 0;
	hdr->additional_rrs = 0;
	SEND_RESPONSE(dhcps.dnssock, (struct sockaddr *)fromaddr,
		      msg, endp - msg);

	return -1;
}

static int process_dhcp_message(char *msg, int len)
{
	struct bootp_header *hdr;
	struct bootp_option *opt;
	uint8_t response_type = DHCP_NO_RESPONSE;
	unsigned int consumed = 0;
	bool got_ip = 0;
	bool need_ip = 0;
	bool got_client_ip = 0;
	bool force_broadcast = false;
	uint32_t new_ip;

	if (!msg ||
	    len < sizeof(struct bootp_header) + sizeof(struct bootp_option) + 1)
		return -1;

	hdr = (struct bootp_header *)msg;

	switch (hdr->op) {
	case BOOTP_OP_REQUEST:
		dhcp_d("bootp request\r\n");
		break;
	case BOOTP_OP_RESPONSE:
		dhcp_d("bootp response\r\n");
		break;
	default:
		dhcp_e("invalid op code: %d\r\n", hdr->op);
		return -1;
	}

	if (hdr->htype != 1 || hdr->hlen != 6) {
		dhcp_e("invalid htype or hlen\r\n");
		return -1;
	}

	dhcp_d("client MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", hdr->chaddr[0],
	    hdr->chaddr[1], hdr->chaddr[2], hdr->chaddr[3], hdr->chaddr[4],
	    hdr->chaddr[5]);

	dhcp_d("magic cookie: 0x%X\r\n", hdr->cookie);

	len -= sizeof(struct bootp_header);
	opt = (struct bootp_option *)(msg + sizeof(struct bootp_header));
	while (len > 0 && opt->type != BOOTP_END_OPTION) {
		if (opt->type == BOOTP_OPTION_DHCP_MESSAGE && opt->length == 1) {
			dhcp_d("found DHCP message option\r\n");
			switch (*(uint8_t *) opt->value) {
			case DHCP_MESSAGE_DISCOVER:
				LWIP_LOGV("DHCP discover\r\n");
				response_type = DHCP_MESSAGE_OFFER;
				break;

			case DHCP_MESSAGE_REQUEST:
				need_ip = 1;
				if (hdr->ciaddr != 0x0000000) {
					dhcps.client_ip = hdr->ciaddr;
					got_client_ip = 1;
				}
				break;

			default:
				LWIP_LOGD("ignoring message type %d\r\n",
				    *(uint8_t *) opt->value);
				break;
			}
		}
		if (opt->type == BOOTP_OPTION_REQUESTED_IP && opt->length == 4) {
			dhcp_d("found REQUESTED IP option %hhu.%hhu.%hhu.%hhu\r\n",
			    (uint8_t)opt->value[0],
			    (uint8_t)opt->value[1], 
			    (uint8_t)opt->value[2], 
			    (uint8_t)opt->value[3]);
			memcpy((uint8_t *) &dhcps.client_ip, (uint8_t *) opt->value, 4);
			got_client_ip = 1;
		}

		if (got_client_ip) {
			/* requested address outside of subnet */
			if ((dhcps.client_ip & dhcps.netmask) ==
			    (dhcps.my_ip & dhcps.netmask)) {

				/* When client requests an IP address,
				 * DHCP-server checks if the valid
				 * IP-MAC entry is present in the
				 * ip-mac cache, if yes, also checks
				 * if the requested IP is same as the
				 * IP address present in IP-MAC entry,
				 * if yes, it allows the device to
				 * continue with the requested IP
				 * address.
				 */
				new_ip = ac_lookup_mac(hdr->chaddr);
				if (new_ip != (CLIENT_IP_NOT_FOUND)) {
					/* if new_ip is equal to requested ip */
					if (new_ip == dhcps.client_ip) {
						got_ip = 1;
					} else {
						got_ip = 0;
					}
				} else if (ac_valid_ip
					   (ntohl(dhcps.client_ip))) {
					/* When client requests with an IP
					 * address that is within subnet range
					 * and not assigned to any other client,
					 * then dhcp-server allows that device
					 * to continue with that IP address.
					 * And if IP-MAC cache is not full then
					 * adds this entry in cache.
					 */
					if (ac_not_full()) {
						ac_add(hdr->chaddr,
						       dhcps.client_ip);
					} else {
						dhcp_w("No space to store new \r\n"
						       "mapping..\r\n");
					}
					got_ip = 1;
				}
			}
		}

		/* look at the next option (if any) */
		consumed = sizeof(struct bootp_option) + opt->length;
		len -= consumed;
		opt = (struct bootp_option *)((char *)opt + consumed);
		if (need_ip)
			response_type = got_ip ? DHCP_MESSAGE_ACK :
			    DHCP_MESSAGE_NAK;
	}

	if (response_type != DHCP_NO_RESPONSE) {
        uint32_t dst_ip = 0, retry = 0;
        int send_byte;
    	struct bootp_header *hdr;
		force_broadcast = (response_type == DHCP_MESSAGE_OFFER) ||
			(response_type == DHCP_MESSAGE_NAK) ||
			(((struct bootp_header *)msg)->flags & htons(1 << 15));
		send_byte = make_response(msg, (enum dhcp_message_type)response_type);
        hdr = (struct bootp_header *)msg;

        dst_ip = hdr->yiaddr;

        if(dst_ip != 0 && !force_broadcast) {
            dhcps.uaddr.sin_addr.s_addr = dst_ip;

            //dhcps.baddr.sin_addr.s_addr = dst_ip;
            dhcp_d("change dhcps.uaddr: %s\r\n", inet_ntoa(dhcps.uaddr.sin_addr));

            etharp_add_static_entry((ip4_addr_t *)&dst_ip, (struct eth_addr *)hdr->chaddr);

            retry = 0;
			LWIP_LOGI("DHCP tx %s unicast yiaddr=%s\r\n",
				  dhcp_msg_type_str(response_type),
				  inet_ntoa(dst_ip));
            hdr->flags &= ~(htons(1<<15));
            while(1) {
        		SEND_RESPONSE(dhcps.sock,
        				    (struct sockaddr *)&dhcps.uaddr, msg, send_byte);

                if(++retry == 1)
                    break;
            }
#ifndef CONFIG_FUZZ_TEST
            etharp_remove_static_entry((ip4_addr_t *)&dst_ip);
#endif
        }

		LWIP_LOGI("DHCP tx %s broadcast yiaddr=%s\r\n",
			  dhcp_msg_type_str(response_type),
			  inet_ntoa(dst_ip));
        hdr->flags |= (htons(1<<15));
        retry = 0;
        while(1) {
    		SEND_RESPONSE(dhcps.sock,
    				    (struct sockaddr *)&dhcps.baddr, msg, send_byte);

            if(++retry == 3)
                break;
        }
        
		if (response_type == DHCP_MESSAGE_ACK)
		{
			send_gratuitous_arp(dhcps.my_ip);
			#if CONFIG_WIFI_CSI_EN
			// send msg to mac to mind dhcp done
			extern int rw_msg_ap_dhcp_done_ind(uint8_t *hdr);
			rw_msg_ap_dhcp_done_ind(hdr->chaddr);
			#endif
		}
		return 0;
	}

	dhcp_d("ignoring DHCP packet\r\n");
	return 0;
}

static void dhcp_clean_sockets(void)
{
	int ret;

	if (dhcps.sock != -1) 
	{
		ret = lwip_close(dhcps.sock);
		if (ret != 0) {
			dhcp_w("Failed to close dhcp socket\r\n");
		}
		dhcps.sock = -1;
	}
	
	if ( dhcp_nack_dns_server_handler )
	{
		if ( dhcps.dnssock != -1 )
		{
			ret = lwip_close(dhcps.dnssock);
			if ( ret != 0 )
			{
				dhcp_w("Failed to close dns socket\r\n");
			}
			dhcps.dnssock = -1;
		}
	}
	
	if ( dhcps.ctrlsock != -1 )
	{
	    ret = lwip_close(dhcps.ctrlsock);
		if ( ret != 0 )
		{
			dhcp_w("Failed to close ctrol port socket\r\n");
		}
		dhcps.ctrlsock = -1;
	}
}

static bool dhcp_inst_any_used(void)
{
	int i;
	for (i = 0; i < DHCP_SERVER_INST_MAX; i++)
		if (dhcp_inst_used[i])
			return true;
	return false;
}

static void dhcp_release_instance(int idx)
{
	if (!dhcp_inst_inited[idx]) {
		dhcp_inst_used[idx] = false;
		dhcp_inst_stopping[idx] = false;
		return;
	}

	g_dhcps_cur = &dhcp_inst[idx];
	dhcp_clean_sockets();
	if (dhcps.msg) {
		os_mem_free(dhcps.msg);
		dhcps.msg = NULL;
	}
	dhcp_inst_inited[idx] = false;
	dhcp_inst_used[idx] = false;
	dhcp_inst_stopping[idx] = false;
}

static void dhcp_cleanup_stopping_instances(void)
{
	int i;

	for (i = 0; i < DHCP_SERVER_INST_MAX; i++) {
		if (!dhcp_inst_stopping[i])
			continue;
		dhcp_release_instance(i);
	}
}

static void dhcp_collect_read_fds(fd_set *rfds, int *max_sock)
{
	int i;

	FD_ZERO(rfds);
	*max_sock = 0;
	for (i = 0; i < DHCP_SERVER_INST_MAX; i++) {
		struct dhcp_server_data *d = &dhcp_inst[i];

		if (!dhcp_inst_used[i])
			continue;
		if (d->sock >= 0) {
			FD_SET(d->sock, rfds);
			if (d->sock > *max_sock)
				*max_sock = d->sock;
		}
		if (dhcp_nack_dns_server_handler && d->dnssock >= 0) {
			FD_SET(d->dnssock, rfds);
			if (d->dnssock > *max_sock)
				*max_sock = d->dnssock;
		}
	}
}

static void dhcp_dispatch_ready_fds(fd_set *rfds, struct sockaddr_in *caddr,
				    socklen_t flen)
{
	int i, len;

	for (i = 0; i < DHCP_SERVER_INST_MAX; i++) {
		if (!dhcp_inst_used[i])
			continue;
		g_dhcps_cur = &dhcp_inst[i];

		if (dhcps.sock >= 0 && FD_ISSET(dhcps.sock, rfds)) {
			len = lwip_recvfrom(dhcps.sock, dhcps.msg, SERVER_BUFFER_SIZE,
					    0, (struct sockaddr *)caddr, &flen);
			if (len > 0) {
				dhcp_d("recved msg on dhcp sock len: %d\r\n", len);
				process_dhcp_message(dhcps.msg, len);
			}
		}

		if (dhcp_nack_dns_server_handler &&
		    dhcps.dnssock >= 0 && FD_ISSET(dhcps.dnssock, rfds)) {
			len = lwip_recvfrom(dhcps.dnssock, dhcps.msg, SERVER_BUFFER_SIZE,
					    0, (struct sockaddr *)caddr, &flen);
			if (len > 0) {
				dhcp_d("recved msg on dns sock len: %d\r\n", len);
				dhcp_nack_dns_server_handler(dhcps.msg, len, caddr);
			}
		}
	}
}

/* Single DHCP server thread. */
void dhcp_server(void* data)
{
	int ret, i;
	struct sockaddr_in caddr;
	int max_sock;
	socklen_t flen = sizeof(caddr);
	fd_set rfds;
	struct timeval tv;

	while (1) {
		if (dhcpd_lock() != 0)
			break;

		/* Tear down instances flagged for stop. */
		dhcp_cleanup_stopping_instances();

		if (!dhcp_inst_any_used()) {
			dhcpd_unlock();
			break;
		}

		dhcp_collect_read_fds(&rfds, &max_sock);
		dhcpd_unlock();

		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = lwip_select(max_sock + 1, &rfds, NULL, NULL, &tv);

		if (ret < 0) {
			dhcp_e("select failed\r\n", -1);
			break;
		}
		if (ret == 0)
			continue;	/* timeout: re-evaluate instance membership */

		if (dhcpd_lock() != 0)
			break;
		dhcp_dispatch_ready_fds(&rfds, &caddr, flen);
		dhcpd_unlock();
	}

	/* No instances left: close everything and exit. */
	if (dhcpd_lock() == 0) {
	for (i = 0; i < DHCP_SERVER_INST_MAX; i++) {
		if (dhcp_inst_inited[i] || dhcp_inst_used[i] || dhcp_inst_stopping[i])
			dhcp_release_instance(i);
	}
		g_dhcps_cur = &dhcp_inst[0];
		dhcpd_thread_running = false;
		dhcpd_unlock();
	} else {
		dhcpd_thread_running = false;
	}
	rtos_delete_thread(NULL);
}

static int create_and_bind_udp_socket(struct sockaddr_in *address,
					void *intrfc_handle)
{
	int one = 1;
	int ret;

	int sock = lwip_socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		dhcp_e("failed to create a socket\r\n");
		return -1;
	}

	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
			 sizeof(int));
	if (ret == -1) {
		/* This is unimplemented in lwIP, hence do not return */
		dhcp_e("failed to set SO_REUSEADDR\r\n");
	}

	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
		       (char *)&one, sizeof(one)) == -1) {
		dhcp_e("failed to set SO_BROADCAST\r\n");
		lwip_close(sock);
		return -1;
	}


	ret = lwip_bind(sock, (struct sockaddr *)address,
					sizeof(struct sockaddr));

	if (ret) {
		dhcp_e("failed to bind server socket\r\n");
		dhcp_e("socket err: %d\r\n", errno);
		lwip_close(sock);
		return -1;
	}
	return sock;
}

/* Initialize one DHCP server instance. */
int dhcp_server_init(struct dhcp_server_data *d, void *intrfc_handle)
{
	int ret = 0;
	struct ifreq ifreq_name = {0};
	struct netif *netif = (struct netif *)intrfc_handle;

	memset(d, 0, sizeof(*d));
	d->sock = d->dnssock = d->ctrlsock = -1;
	d->msg = (char*)os_mem_alloc(SERVER_BUFFER_SIZE);
	if (d->msg == NULL)
		return -1;

	get_broadcast_addr(&d->baddr);
	d->baddr.sin_port = htons(DHCP_CLIENT_PORT);

	get_broadcast_addr(&d->uaddr);
	d->uaddr.sin_port = htons(DHCP_CLIENT_PORT);

	if (get_ip_addr_from_interface(&d->my_ip, intrfc_handle) < 0) {
		dhcp_e("failed to look up our IP address from interface\r\n");
		ret = -1;
		goto out;
	}

	if (get_netmask_from_interface(&d->netmask, intrfc_handle) < 0) {
		dhcp_e("failed to look up our netmask from interface\r\n");
		ret = -1;
		goto out;
	}

	if (get_gateway_from_interface(&d->router_ip, intrfc_handle) < 0) {
		dhcp_e("failed to look up our gateway from interface\r\n");
		ret = -1;
		goto out;
	}

	d->saddr.sin_family = AF_INET;
	d->saddr.sin_addr.s_addr = INADDR_ANY;
	d->saddr.sin_port = htons(DHCP_SERVER_PORT);
	d->sock = create_and_bind_udp_socket(&d->saddr, intrfc_handle);

	if (d->sock < 0) {
		ret = -1;
		goto out;
	}

	ifreq_name.ifr_name[0] = netif->name[0];
	ifreq_name.ifr_name[1] = netif->name[1];
	sprintf(&ifreq_name.ifr_name[2], "%u", netif->num);

	if (setsockopt(d->sock, SOL_SOCKET, SO_BINDTODEVICE, &ifreq_name, sizeof(struct ifreq)) != 0) {
		dhcp_e("failed to set SO_BINDTODEVICE\r\n");
		ret = -1;
		goto out;
	}

	if (dhcp_nack_dns_server_handler) {
		d->dnsaddr.sin_family = AF_INET;
		d->dnsaddr.sin_addr.s_addr = INADDR_ANY;
		d->dnsaddr.sin_port = htons(NAMESERVER_PORT);
		d->dnssock = create_and_bind_udp_socket(&d->dnsaddr,
							   intrfc_handle);
		if (d->dnssock < 0) {
			ret = -1;
			goto out;
		}
		if (setsockopt(d->dnssock, SOL_SOCKET, SO_BINDTODEVICE, &ifreq_name, sizeof(struct ifreq)) != 0) {
			dhcp_e("failed to set SO_BINDTODEVICE\r\n");
			ret = -1;
			goto out;
		}
	}

	d->prv = intrfc_handle;

	d->current_ip = ntohl(d->my_ip & d->netmask) | ((99) & ntohl(~d->netmask));

	return 0;

out:
	if (d->sock >= 0) { lwip_close(d->sock); d->sock = -1; }
	if (d->dnssock >= 0) { lwip_close(d->dnssock); d->dnssock = -1; }
	os_mem_free(d->msg);
	d->msg = NULL;
	return ret;
}

/* Allocate or reuse one instance slot for intrfc_handle. */
int dhcp_server_add_instance(void *intrfc_handle)
{
	int i, slot = -1;

	if (dhcpd_lock() != 0)
		return -1;

	for (i = 0; i < DHCP_SERVER_INST_MAX; i++) {
		if (dhcp_inst_used[i] && dhcp_inst[i].prv == intrfc_handle)
			goto out_found;	/* already running on this interface */
	}
	for (i = 0; i < DHCP_SERVER_INST_MAX; i++) {
		if (!dhcp_inst_used[i] && !dhcp_inst_stopping[i]) {
			slot = i;
			break;
		}
	}
	if (slot < 0) {
		dhcp_e("no free DHCP instance slot\r\n");
		dhcpd_unlock();
		return -1;
	}

	if (dhcp_server_init(&dhcp_inst[slot], intrfc_handle) != 0) {
		dhcpd_unlock();
		return -1;
	}

	/* Publish last. */
	dhcp_inst_inited[slot] = true;
	dhcp_inst_used[slot] = true;
	dhcpd_unlock();
	return slot;

out_found:
	dhcpd_unlock();
	return i;
}

void dhcp_enable_nack_dns_server(void)
{
	dhcp_nack_dns_server_handler = process_dns_message;
}

/* Request teardown of the instance bound to intrfc_handle. */
int dhcp_request_stop(void *intrfc_handle)
{
	int i, hit = 0;

	if (dhcpd_lock() != 0)
		return -1;

	for (i = 0; i < DHCP_SERVER_INST_MAX; i++) {
		if (!dhcp_inst_used[i])
			continue;
		if (intrfc_handle == NULL || dhcp_inst[i].prv == intrfc_handle) {
			dhcp_inst_stopping[i] = true;
			hit = 1;
		}
	}

	dhcpd_unlock();
	return hit ? 0 : -1;
}

static int send_gratuitous_arp(uint32_t ip)
{
	int sock;
	struct arp_packet pkt;
	struct sockaddr_in to_addr;
	to_addr.sin_family = AF_INET;
	to_addr.sin_addr.s_addr = ip;
	pkt.frame_type = htons(ARP_FRAME_TYPE);
	pkt.hw_type = htons(ETHER_HW_TYPE);
	pkt.prot_type = htons(IP_PROTO_TYPE);
	pkt.hw_addr_size = ETH_HW_ADDR_LEN;
	pkt.prot_addr_size = IP_ADDR_LEN;
	pkt.op = htons(OP_ARP_REQUEST);

	write_u32(pkt.sndr_ip_addr, ip);
	write_u32(pkt.rcpt_ip_addr, ip);

	memset(pkt.targ_hw_addr, 0xff, ETH_HW_ADDR_LEN);
	memset(pkt.rcpt_hw_addr, 0xff, ETH_HW_ADDR_LEN);
    get_mac_addr_from_interface((void*)pkt.sndr_hw_addr, dhcps.prv);
    get_mac_addr_from_interface((void*)pkt.src_hw_addr, dhcps.prv);   
    
	sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		dhcp_e("Could not open socket to send Gratuitous ARP\r\n");
		return -1;
	}
	memset(pkt.padding, 0, sizeof(pkt.padding));

	if (lwip_sendto(sock, (char *)&pkt, sizeof(pkt), 0,
		   (struct sockaddr *)&to_addr, sizeof(to_addr)) < 0) {
		dhcp_e("Failed to send Gratuitous ARP\r\n");
		lwip_close(sock);
		return -1;
	}
	dhcp_d("Gratuitous ARP sent\r\n");
	lwip_close(sock);
	return 0;
}

static void get_broadcast_addr(struct sockaddr_in *addr)
{
	addr->sin_family = AF_INET;
	/* limited broadcast addr (255.255.255.255) */
	addr->sin_addr.s_addr = 0xffffffff;
	addr->sin_len = sizeof(struct sockaddr_in);
}

static int get_mac_addr_from_interface(void *mac, void *interface_handle)
{
	return net_get_if_macaddr(mac, interface_handle);
}

static int get_ip_addr_from_interface(uint32_t *ip, void *interface_handle)
{
	return net_get_if_ip_addr(ip, interface_handle);
}

static int get_netmask_from_interface(uint32_t *nm, void *interface_handle)
{
	return net_get_if_ip_mask(nm, interface_handle);
}

static int get_gateway_from_interface(uint32_t *gw, void *interface_handle)
{
	return net_get_if_gw_addr(gw, interface_handle);
}

uint8_t* dhcp_lookup_mac(uint8_t *chaddr)
{
	/* returns ip address, if mac address is present in cache */
	int i;
    struct in_addr ip;
	for (i = 0; i < dhcps.count_clients && i < MAC_IP_CACHE_SIZE; i++) {
		if ((dhcps.ip_mac_mapping[i].client_mac[0] == chaddr[0]) &&
		    (dhcps.ip_mac_mapping[i].client_mac[1] == chaddr[1]) &&
		    (dhcps.ip_mac_mapping[i].client_mac[2] == chaddr[2]) &&
		    (dhcps.ip_mac_mapping[i].client_mac[3] == chaddr[3]) &&
		    (dhcps.ip_mac_mapping[i].client_mac[4] == chaddr[4]) &&
		    (dhcps.ip_mac_mapping[i].client_mac[5] == chaddr[5])) {

            ip.s_addr = dhcps.ip_mac_mapping[i].client_ip;
 
			return (uint8_t*)inet_ntoa(ip);
		}
	}
	return 0;
}
