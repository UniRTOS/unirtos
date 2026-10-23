/*
 * Copyright 2020-2026 Beken
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Beken bridge extensions (port/bk_bridge.c): proxy-ARP, DHCP relay, bridgeif hooks.
 * Sections: port lifecycle | checksum | pending-ARP queue | upstream RX/TX hooks |
 * bridgeif callbacks (strip_rx_ifidx, post_init, deinit).
 */

#include "netif/bridgeif.h"
#include "common/bk_include.h"
#include "common/bk_generic.h"
#include "bk_bridge.h"

#if BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT
#error "BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT=1 breaks the threading invariants " \
       "documented at the top of bk_bridge.c. Before enabling DIRECT: " \
       "(a) add a mutex around g_arp_request and defer etharp_* calls to " \
       "tcpip_thread via tcpip_callback; (b) add an RCU/refcount barrier in " \
       "bridgeif_deinit so in-flight bridgeif_input cannot dereference a " \
       "freed bridgeif_private_t; (c) audit bmsg_tx_sender for MPSC safety."
#endif

#include "lwip/mem.h"
#include "lwip/dhcp.h"
#include "lwip/sys.h"
#include "lwip/etharp.h"
#include "lwip/inet_chksum.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "lwip/prot/dhcp.h"
#include "lwip/prot/iana.h"
#include "lwip/udp.h"
#include "netif/ethernet.h"
#include <string.h>

#include "bk_list.h"
#include "bk_wifi_types.h"
#include "../../dhcpd/dhcp-bootp.h"
#include "os/mem.h"
#include "os/os.h"
#include "net.h"
#include "dhcp.h"

/* lwip private aliases used to live in bridgeif.c */
#define etharp_ip4_addr_cmp(addr1, addr2)    ((addr1).addrw[0] == (addr2).addrw[0] && (addr1).addrw[1] == (addr2).addrw[1])
#define etharp_ip4_addr_isany_val(addr) (addr.addrw[0] == 0 && addr.addrw[1] == 0)
#define BK_LWIP_FAST_CSUM_MOD 1

#if IP_FORWARD && IP_FORWARD_ALLOW_TX_ON_RX_NETIF
#define BRIDGE_MODIFY_DST_ADDR 0
#else
#define BRIDGE_MODIFY_DST_ADDR 1
#endif

#ifndef DHCP_FLAG_BROADCAST
#define DHCP_FLAG_BROADCAST  (1 << 15)
#endif

/* Forwarded from lwip-internal etharp.c; bridgeif used the same prototype. */
extern err_t etharp_update_arp_entry(struct netif *netif, const ip4_addr_t *ipaddr, struct eth_addr *ethaddr, u8_t flags, s16_t *loc);

/*
 * Port numbers handed to us by bridgeif_add_port().  Used only by the FDB
 * lookups in the proxy paths below; -1 means "not yet assigned" (bridge
 * not up).  Single-writer (wifi-event task), multiple-reader (tcpip
 * thread) — small enough to be torn-read-safe on Armv8-M.
 */
static int s_bridge_sta_port = -1;
static int s_bridge_sap_port = -1;

#ifndef CONFIG_WIFI_MAC_SUPPORT_STAS_MAX_NUM
#define CONFIG_WIFI_MAC_SUPPORT_STAS_MAX_NUM  2
#endif
/* Downstream SoftAP clients; align with WiFi MAC remote-STA capacity. */
#define BK_BRIDGE_CLIENT_MAP_MAX  CONFIG_WIFI_MAC_SUPPORT_STAS_MAX_NUM

struct bk_bridge_client_entry {
  ip4_addr_t ip;
  uint8_t    mac[ETH_ALEN];
  u8_t       used;
};

static struct bk_bridge_client_entry s_bridge_clients[BK_BRIDGE_CLIENT_MAP_MAX];

static void arp_request_flush_all(void);
static void bk_bridge_client_map_clear_all(void);
static void bk_bridge_client_map_remove_mac(const uint8_t *mac);
static bool bk_bridge_client_map_lookup(const ip4_addr_t *ip, struct eth_addr *eth);
static void bk_bridge_install_client_arp(const ip4_addr_t *ip, const uint8_t *mac);
static int bk_bridge_dhcp_parse_msg(struct dhcp_msg *msg, int len, int *dhcp_msg_type);
static void bk_bridge_try_learn_dhcp(bridgeif_private_t *br, struct pbuf *p,
                                     struct netif *rx_if);

static inline bool netif_is_upstream(struct netif *netif)
{
  return netif == net_get_sta_handle();
}

/* Remove static ARP table rows whose Ethernet address equals mac. */
static void etharp_remove_static_entry_by_mac(uint8_t *mac)
{
  int i;
  for (i = 0; i < ARP_TABLE_SIZE; i++) {
    ip4_addr_t *ip;
    struct netif *netif;
    struct eth_addr *ethaddr;

    if (etharp_get_entry(i, &ip, &netif, &ethaddr)) {
      if (!memcmp(mac, ethaddr, ETH_HWADDR_LEN)) {
        LWIP_LOGD("remove static ARP entry for %pIn/%pm\n", ip, mac);
        etharp_remove_static_entry(ip);
      }
    }
  }
}

/* Record bridgeif port_num for STA (upstream) and softAP (downstream). */
static void bridge_attach_port(struct netif *portif, int port_num)
{
  if (portif == NULL)
    return;
  if (portif == (struct netif *)net_get_sta_handle())
    s_bridge_sta_port = port_num;
  else if (portif == (struct netif *)net_get_uap_handle())
    s_bridge_sap_port = port_num;
}

static void bk_bridge_client_map_clear_all(void)
{
  int i;

  for (i = 0; i < BK_BRIDGE_CLIENT_MAP_MAX; i++) {
    s_bridge_clients[i].used = 0;
  }
}

static void bk_bridge_client_map_remove_mac(const uint8_t *mac)
{
  int i;

  if (mac == NULL) {
    return;
  }
  for (i = 0; i < BK_BRIDGE_CLIENT_MAP_MAX; i++) {
    if (s_bridge_clients[i].used &&
        !memcmp(s_bridge_clients[i].mac, mac, ETH_ALEN)) {
      s_bridge_clients[i].used = 0;
    }
  }
}

static bool bk_bridge_client_map_lookup(const ip4_addr_t *ip, struct eth_addr *eth)
{
  int i;

  if (ip == NULL || eth == NULL || ip4_addr_isany_val(*ip)) {
    return false;
  }
  for (i = 0; i < BK_BRIDGE_CLIENT_MAP_MAX; i++) {
    if (s_bridge_clients[i].used &&
        ip4_addr_cmp(&s_bridge_clients[i].ip, ip)) {
      memcpy(eth, s_bridge_clients[i].mac, ETH_ALEN);
      return true;
    }
  }
  return false;
}

/* True when a static ARP row for ip->mac is already present. */
static bool bk_bridge_static_arp_matches(const ip4_addr_t *ip, const uint8_t *mac)
{
  const ip4_addr_t *unused_ipaddr;
  struct eth_addr *dethaddr;

  if (etharp_find_addr(NULL, ip, &dethaddr, &unused_ipaddr) < 0) {
    return false;
  }
  return !memcmp(dethaddr, mac, ETH_ALEN);
}

static void bk_bridge_install_client_arp(const ip4_addr_t *ip, const uint8_t *mac)
{
  int i, slot = -1;
  err_t err;

  if (ip == NULL || mac == NULL || ip4_addr_isany_val(*ip)) {
    return;
  }
  if (!memcmp(mac, &ethzero, ETH_ALEN)) {
    return;
  }

  /* Hot path: map and ARP table already match (avoid per-packet ARP scan). */
  for (i = 0; i < BK_BRIDGE_CLIENT_MAP_MAX; i++) {
    if (s_bridge_clients[i].used &&
        ip4_addr_cmp(&s_bridge_clients[i].ip, ip) &&
        !memcmp(s_bridge_clients[i].mac, mac, ETH_ALEN) &&
        bk_bridge_static_arp_matches(ip, mac)) {
      return;
    }
  }

  for (i = 0; i < BK_BRIDGE_CLIENT_MAP_MAX; i++) {
    if (s_bridge_clients[i].used &&
        ip4_addr_cmp(&s_bridge_clients[i].ip, ip)) {
      slot = i;
      break;
    }
    if (s_bridge_clients[i].used &&
        !memcmp(s_bridge_clients[i].mac, mac, ETH_ALEN)) {
      slot = i;
      break;
    }
    if (!s_bridge_clients[i].used && slot < 0) {
      slot = i;
    }
  }
  if (slot < 0) {
    slot = 0;
  }

  if (s_bridge_clients[slot].used &&
      ip4_addr_cmp(&s_bridge_clients[slot].ip, ip) &&
      !memcmp(s_bridge_clients[slot].mac, mac, ETH_ALEN) &&
      bk_bridge_static_arp_matches(ip, mac)) {
    return;
  }

  s_bridge_clients[slot].ip = *ip;
  memcpy(s_bridge_clients[slot].mac, mac, ETH_ALEN);
  s_bridge_clients[slot].used = 1;

  etharp_remove_static_entry_by_mac((uint8_t *)mac);
  err = etharp_add_static_entry(ip, (struct eth_addr *)mac);
  if (err < ERR_OK) {
    LWIP_LOGE("fail add static entry: %pm, yiaddr %pIn err:%d\n", mac, ip, err);
  }
}

static int bk_bridge_dhcp_parse_msg(struct dhcp_msg *msg, int len, int *dhcp_msg_type)
{
  struct bootp_option *opt = (struct bootp_option *)msg->options;
  int rem;

  if (dhcp_msg_type != NULL) {
    *dhcp_msg_type = -1;
  }
  if (len < (int)offsetof(struct dhcp_msg, options)) {
    return 0;
  }
  rem = len - (int)offsetof(struct dhcp_msg, options);

  while (rem > 0 && opt->type != BOOTP_END_OPTION) {
    if (rem < (int)sizeof(struct bootp_option) ||
        opt->length > (u8_t)(rem - (int)sizeof(struct bootp_option))) {
      break;
    }
    if (opt->type == BOOTP_OPTION_DHCP_MESSAGE && opt->length == 1 && dhcp_msg_type != NULL) {
      *dhcp_msg_type = opt->value[0];
    }
    rem -= (int)(sizeof(struct bootp_option) + opt->length);
    opt = (struct bootp_option *)((char *)opt + sizeof(struct bootp_option) + opt->length);
  }
  return 1;
}

static void bk_bridge_dhcp_client_ip(struct dhcp_msg *msg, int len, ip4_addr_t *out_ip)
{
  int dhcp_msg_type = -1;
  struct bootp_option *opt;
  int rem;

  ip4_addr_set_zero(out_ip);
  memcpy(out_ip, &msg->yiaddr, sizeof(*out_ip));
  if (!ip4_addr_isany_val(*out_ip)) {
    return;
  }
  memcpy(out_ip, &msg->ciaddr, sizeof(*out_ip));
  if (!ip4_addr_isany_val(*out_ip)) {
    return;
  }

  if (len < (int)offsetof(struct dhcp_msg, options)) {
    return;
  }
  opt = (struct bootp_option *)msg->options;
  rem = len - (int)offsetof(struct dhcp_msg, options);
  (void)bk_bridge_dhcp_parse_msg(msg, len, &dhcp_msg_type);

  while (rem > 0 && opt->type != BOOTP_END_OPTION) {
    if (rem < (int)sizeof(struct bootp_option) ||
        opt->length > (u8_t)(rem - (int)sizeof(struct bootp_option))) {
      break;
    }
    if (opt->type == BOOTP_OPTION_REQUESTED_IP && opt->length == 4) {
      memcpy(out_ip, opt->value, sizeof(ip4_addr_t));
      return;
    }
    rem -= (int)(sizeof(struct bootp_option) + opt->length);
    opt = (struct bootp_option *)((char *)opt + sizeof(struct bootp_option) + opt->length);
  }
}

static void bk_bridge_try_learn_dhcp(bridgeif_private_t *br, struct pbuf *p,
                                     struct netif *rx_if)
{
  const u16_t ipoff = SIZEOF_ETH_HDR;
  struct ip_hdr *iphdr;
  u16_t iphl, iplen;
  struct udp_hdr *udphdr;
  struct dhcp_msg *msg;
  int pay_len;
  ip4_addr_t client_ip;
  int dhcp_msg_type = -1;

  LWIP_UNUSED_ARG(br);

  if (p->len < ipoff + IP_HLEN + sizeof(struct udp_hdr) + offsetof(struct dhcp_msg, options)) {
    return;
  }
  iphdr = (struct ip_hdr *)((u8_t *)p->payload + ipoff);
  if (IPH_V(iphdr) != 4) {
    return;
  }
  iphl = (u16_t)(IPH_HL(iphdr) * 4);
  iplen = lwip_ntohs(IPH_LEN(iphdr));
  if (iphl < IP_HLEN || iphl > p->len || iplen > p->tot_len) {
    return;
  }
  if (IPH_PROTO(iphdr) != IP_PROTO_UDP) {
    return;
  }
  udphdr = (struct udp_hdr *)((u8_t *)p->payload + ipoff + iphl);
  if (lwip_ntohs(udphdr->src) != DHCP_SERVER_PORT ||
      lwip_ntohs(udphdr->dest) != DHCP_CLIENT_PORT) {
    return;
  }
  pay_len = (int)lwip_ntohs(udphdr->len) - (int)sizeof(*udphdr);
  if (pay_len < (int)offsetof(struct dhcp_msg, options)) {
    return;
  }
  msg = (struct dhcp_msg *)(udphdr + 1);
  bk_bridge_dhcp_parse_msg(msg, pay_len, &dhcp_msg_type);

  if (memcmp(msg->chaddr, rx_if->hwaddr, ETH_ALEN)) {
    bk_bridge_dhcp_client_ip(msg, pay_len, &client_ip);
    if ((dhcp_msg_type == DHCP_OFFER || dhcp_msg_type == DHCP_ACK) &&
        !ip4_addr_isany_val(client_ip)) {
      bk_bridge_install_client_arp(&client_ip, msg->chaddr);
    }
  }
}

/* Remove every static ARP entry installed by the DHCP relay path. */
static void bridge_flush_static_arp(void)
{
  ip4_addr_t ips[ARP_TABLE_SIZE];
  int n = 0;
  int i;

  for (i = 0; i < ARP_TABLE_SIZE; i++) {
    ip4_addr_t *ip;
    struct netif *netif;
    struct eth_addr *ethaddr;

    if (etharp_get_entry(i, &ip, &netif, &ethaddr)) {
      ips[n++] = *ip;
    }
  }
  for (i = 0; i < n; i++) {
    etharp_remove_static_entry(&ips[i]);
  }
  bk_bridge_client_map_clear_all();
}

/* Tear-down: static ARP, pending ARP queue, port snapshots. */
static void bridge_detach_all_ports(void)
{
  bridge_flush_static_arp();
  arp_request_flush_all();
  s_bridge_sta_port = -1;
  s_bridge_sap_port = -1;
}

/*
 * RFC 1624: incremental 1's-complement checksum update for a single
 * 16-bit word change.  Used by the DHCP-broadcast-bit rewrite path so we
 * don't have to recompute the full UDP checksum.
 */
static uint16_t recalc_csum16(uint16_t old_csum, uint16_t old_u16, uint16_t new_u16)
{
   uint32_t csum = (~old_csum & 0xFFFF) + (~old_u16 & 0xFFFF) + new_u16 ;
   csum = (csum >> 16) + (csum & 0xFFFF);
   csum += csum >> 16;
   return ~csum;
}

/* RFC 1624 incremental checksum for a 32-bit field change (lease option). */
static uint16_t recalc_csum32(uint16_t old_csum, uint32_t old_u32, uint32_t new_u32)
{
    return recalc_csum16(recalc_csum16(old_csum, old_u32, new_u32),
                         old_u32 >> 16, new_u32 >> 16);
}

/*
 * Full RFC1071 16-bit one's-complement sum.  Only used by the !FAST_CSUM
 * fallback in the DHCP rewrite path (kept for environments where
 * recalc_csum16's incremental update is not allowed).
 */
static uint32_t checksum32(uint32_t start_value, uint8_t *data, size_t len)
{
    uint32_t csum = start_value;
    uint16_t data16 = 0;
    int i;

    for(i = 0; i < (len / 2 * 2); i += 2) {
        data16 = (data[i] << 8) | data[i + 1];
        csum += data16;
    }

    if(len % 2) {
        data16 = data[len - 1] << 8;
        csum += data16;
    }

    return csum;
}

/* Fold 32-bit one's-complement sum to 16-bit Internet checksum. */
static uint16_t checksum32to16(uint32_t csum)
{
    csum = (csum >> 16) + (csum & 0x0000ffff);
    csum = (csum >> 16) + (csum & 0x0000ffff);
    return (uint16_t) ~(csum & 0xffff);
}

/* Full UDP pseudo-header checksum (!BK_LWIP_FAST_CSUM_MOD DHCP rewrite path). */
static u16_t calculate_chksum_pseudo(uint8_t *payload, struct udp_hdr *uhdr, uint8_t protocol, u16_t ulen,u32_t *src, u32_t *dest)
{
    uint32_t udp_checksum32 = 0;
    uint8_t *uhdr_data = (uint8_t *)uhdr;
    /*
     * IPv4 UDP pseudo-header layout per RFC 768:
     *   src(4) | dst(4) | zero(1) | proto(1) | l4len(2)
     */
    uint8_t pseudo_header[12] = {0};

    memcpy(&pseudo_header[0], src, 4);
    memcpy(&pseudo_header[4], dest, 4);
    pseudo_header[8] = 0x00;
    pseudo_header[9] = protocol;
    pseudo_header[10] = (uint8_t)(ulen >> 8);
    pseudo_header[11] = (uint8_t)(ulen & 0xff);

    udp_checksum32 = checksum32(udp_checksum32, pseudo_header, sizeof(pseudo_header));
    udp_checksum32 = checksum32(udp_checksum32, uhdr_data, sizeof(struct udp_hdr));
    udp_checksum32 = checksum32(udp_checksum32, payload, (ulen-8));
    return checksum32to16(udp_checksum32);
}

/*
 * Build and send a raw ARP packet on `netif`.  Like etharp_request_dst
 * but lets callers craft every address independently — needed for proxy
 * scenarios where eth-hdr addresses and arp-hdr addresses don't match.
 *
 * Pbuf is freed before return; ERR_MEM if allocation failed.
 */
static err_t etharp_raw2(struct netif *netif, u8_t rx_if_idx,
          const struct eth_addr *ethsrc_addr, const struct eth_addr *ethdst_addr,
          const struct eth_addr *hwsrc_addr, const ip4_addr_t *ipsrc_addr,
          const struct eth_addr *hwdst_addr, const ip4_addr_t *ipdst_addr,
          const u16_t opcode,
          u8 flag)
{
  struct pbuf *p;
  err_t result = ERR_OK;
  struct etharp_hdr *hdr;
  LWIP_UNUSED_ARG(rx_if_idx);

  LWIP_ASSERT("netif != NULL", netif != NULL);

  p = pbuf_alloc(PBUF_LINK, SIZEOF_ETHARP_HDR, PBUF_RAM);
  if (p == NULL)
    return ERR_MEM;

  hdr = (struct etharp_hdr *)p->payload;
  hdr->opcode = lwip_htons(opcode);

  memcpy(&hdr->shwaddr, hwsrc_addr, 6);
  memcpy(&hdr->dhwaddr, hwdst_addr, 6);
  /* Copy ip4_addr to ip4_addr2 (packed) without breaking strict-aliasing. */
  memcpy(&hdr->sipaddr, ipsrc_addr, sizeof(ip4_addr_t));
  memcpy(&hdr->dipaddr, ipdst_addr, sizeof(ip4_addr_t));

  hdr->hwtype = PP_HTONS(LWIP_IANA_HWTYPE_ETHERNET);
  hdr->proto = PP_HTONS(ETHTYPE_IP);
  hdr->hwlen = ETH_HWADDR_LEN;
  hdr->protolen = sizeof(ip4_addr_t);

  p->elfags = flag;

#if LWIP_AUTOIP
  /* RFC 3927 §2.5: link-local source ⇒ link-layer broadcast. */
  if (ip4_addr_islinklocal(ipsrc_addr))
    ethernet_output(netif, p, ethsrc_addr, &ethbroadcast, ETHTYPE_ARP);
  else
#endif /* LWIP_AUTOIP */
    ethernet_output(netif, p, ethsrc_addr, ethdst_addr, ETHTYPE_ARP);

  pbuf_free(p);
  return result;
}

/* Pending downstream ARP requests (proxy until upstream reply arrives). */

static struct list_head g_arp_request = LIST_HEAD_INIT(g_arp_request);
static bool s_arp_req_timer_armed;

#define BRIDGE_ARP_REQ_TIMER_MS  1000
#define BRIDGE_ARP_REQ_TIMEOUT_MS  5000
#define time_after(a, b)         ((long)(b) - (long)(a) < 0)

struct arp_req_entry {
  struct list_head node;
  ip4_addr_t dip;
  ip4_addr_t sip;
  uint8_t  shwaddr[ETH_ALEN];
  uint32_t ctime;
};

/* Find pending entry by downstream sender MAC and target IPv4 address. */
static struct arp_req_entry *arp_request_find(uint8_t *shwaddr, uint32_t dip)
{
  const struct list_head *head = &g_arp_request;
  struct list_head *n;
  struct arp_req_entry *entry;

  for (n = head->next; n != head; n = n->next) {
    entry = list_entry(n, struct arp_req_entry, node);
    if (entry->dip.addr == dip && !memcmp(entry->shwaddr, shwaddr, ETH_ALEN)) {
      return entry;
    }
  }
  return NULL;
}

/* Free pending ARP entries older than BRIDGE_ARP_REQ_TIMEOUT_MS. */
static void arp_request_timeout_expire(void)
{
  const struct list_head *head = &g_arp_request;
  struct list_head *n, *next;
  struct arp_req_entry *entry;
  uint32_t ctime = rtos_get_time();
  GLOBAL_INT_DECLARATION();

  GLOBAL_INT_DISABLE();
  for (n = head->next, next = n->next; n != head; n = next, next = n->next) {
    entry = list_entry(n, struct arp_req_entry, node);
    if (time_after(ctime, entry->ctime + BRIDGE_ARP_REQ_TIMEOUT_MS)) {
      list_del(&entry->node);
      GLOBAL_INT_RESTORE();
      LWIP_LOGV("%s: free arp request: shwaddr %pm, sip 0x%x, dip 0x%x\n",
                __func__, entry->shwaddr, entry->sip.addr, entry->dip.addr);
      os_free(entry);
      GLOBAL_INT_DISABLE();
    }
  }
  GLOBAL_INT_RESTORE();
}

/* sys_timeout worker; reschedules while g_arp_request is non-empty. */
static void arp_request_timeout_tmr(void *arg)
{
  LWIP_UNUSED_ARG(arg);

  arp_request_timeout_expire();
  if (!list_empty(&g_arp_request)) {
    sys_timeout(BRIDGE_ARP_REQ_TIMER_MS, arp_request_timeout_tmr, NULL);
  } else {
    s_arp_req_timer_armed = false;
  }
}

/* Arm periodic cleanup timer once (first pending entry). */
static void arp_request_timer_arm(void)
{
  if (!s_arp_req_timer_armed) {
    s_arp_req_timer_armed = true;
    sys_timeout(BRIDGE_ARP_REQ_TIMER_MS, arp_request_timeout_tmr, NULL);
  }
}

/* Drain pending ARP list and cancel cleanup timer (bridge stop/deinit). */
static void arp_request_flush_all(void)
{
  const struct list_head *head = &g_arp_request;
  struct list_head *n, *next;
  struct arp_req_entry *entry;

  if (s_arp_req_timer_armed) {
    sys_untimeout(arp_request_timeout_tmr, NULL);
    s_arp_req_timer_armed = false;
  }

  for (n = head->next, next = n->next; n != head; n = next, next = n->next) {
    entry = list_entry(n, struct arp_req_entry, node);
    list_del(&entry->node);
    os_free(entry);
  }
}

/* Queue downstream ARP request until upstream reply can be relayed. */
static void arp_request_add(uint8_t *shwaddr, uint32_t sip, uint32_t dip)
{
  struct arp_req_entry *pos;
  GLOBAL_INT_DECLARATION();

  GLOBAL_INT_DISABLE();
  pos = arp_request_find(shwaddr, dip);
  if (pos) {
    pos->ctime = rtos_get_time();
    pos->sip.addr = sip;
    GLOBAL_INT_RESTORE();
    LWIP_LOGV("%s: already added arp request: shwaddr %pm, sip %pIn, dip %pIn\n",
      __func__, shwaddr, &sip, &dip);
    return;
  }
  GLOBAL_INT_RESTORE();

  pos = os_malloc(sizeof(*pos));
  if (pos) {
    pos->dip.addr = dip;
    pos->sip.addr = sip;
    memcpy(pos->shwaddr, shwaddr, ETH_ALEN);
    pos->ctime = rtos_get_time();
    GLOBAL_INT_DISABLE();
    list_add_tail(&pos->node, &g_arp_request);
    GLOBAL_INT_RESTORE();
    LWIP_LOGV("%s: add arp request: shwaddr %pm, sip %pIn, dip %pIn\n",
      __func__, shwaddr, &sip, &dip);
    arp_request_timer_arm();
  }
}

/* Match upstream ARP reply IP; send proxy replies to all pending clients. */
static void arp_request_handle_reply(bridgeif_private_t *br, struct netif *netif, ip4_addr_t *dip, uint8_t *dhwaddr)
{
  struct list_head reqs;
  struct arp_req_entry *pos;
  int count = 0;
  GLOBAL_INT_DECLARATION();

  INIT_LIST_HEAD(&reqs);

  GLOBAL_INT_DISABLE();
  {
    const struct list_head *head = &g_arp_request;
    struct list_head *n, *next;

    for (n = head->next, next = n->next; n != head; n = next, next = n->next) {
      pos = list_entry(n, struct arp_req_entry, node);
      if (pos->dip.addr == dip->addr) {
        list_del(&pos->node);
        list_add_tail(&pos->node, &reqs);
        count++;
      }
    }
  }
  GLOBAL_INT_RESTORE();

  if (!count)
    return;

  LWIP_LOGV("send arp reply to %d STA\n", count);

  {
    const struct list_head *head = &reqs;
    struct list_head *n, *next;

    for (n = head->next, next = n->next; n != head; n = next, next = n->next) {
      pos = list_entry(n, struct arp_req_entry, node);
      list_del(&pos->node);
      LWIP_LOGD("external ARP reply, send to STA\n");
      etharp_raw2(br->netif, netif_get_index(netif),
             (struct eth_addr *)dhwaddr, (struct eth_addr *)pos->shwaddr,
             (struct eth_addr *)dhwaddr, dip,
             (struct eth_addr *)pos->shwaddr, &pos->sip,
             ARP_REPLY, 1);
      os_free(pos);
    }
  }
}

/* DHCP relay (upstream RX direction)                                  */

/*
 * Walk the DHCP OPTION list to learn an upstream-assigned client IP and
 * install a static ARP entry on br0 so subsequent unicast traffic to
 * that IP routes back to the correct downstream client.
 *
 * Returns 0 (caller decides further forwarding).
 */
static int bk_bridge_upstream_handle_dhcp(struct netif *rx_if, struct udp_hdr *udphdr, ip4_addr_t *dipaddr, struct dhcp_msg *msg, int len)
{
  struct bootp_option *opt = (struct bootp_option *)msg->options;
  int consumed;
  int rem;
  int dhcp_msg_type = -1;
  ip4_addr_t client_ip;

  LWIP_UNUSED_ARG(dipaddr);

  if (len < (int)offsetof(struct dhcp_msg, options)) {
    return 0;
  }

  rem = len - (int)offsetof(struct dhcp_msg, options);
  bk_bridge_dhcp_parse_msg(msg, len, &dhcp_msg_type);

  if (memcmp(msg->chaddr, rx_if->hwaddr, ETH_ALEN)) {
    while (rem > 0 && opt->type != BOOTP_END_OPTION) {
      if (rem < (int)sizeof(struct bootp_option) ||
          opt->length > (u8_t)(rem - (int)sizeof(struct bootp_option))) {
        break;
      }
      if (opt->type == BOOTP_OPTION_ADDRESS_TIME && opt->length == 4) {
        uint32_t lease_time = ((uint8_t)opt->value[0] << 24) | ((uint8_t)opt->value[1] << 16) |
                              ((uint8_t)opt->value[2] << 8) | ((uint8_t)opt->value[3]);
        LWIP_LOGD("Router's DHCP lease_time %d\n", lease_time);
#if CONFIG_BK_DHCP_RELEASE_TIME
        if (lease_time < BK_DHCP_RELEASE_TIME) {
          uint32_t old_value = lease_time;
          lease_time = BK_DHCP_RELEASE_TIME;
          opt->value[0] = (lease_time >> 24) & 0xFF;
          opt->value[1] = (lease_time >> 16) & 0xFF;
          opt->value[2] = (lease_time >> 8) & 0xFF;
          opt->value[3] = lease_time & 0xFF;
          udphdr->chksum = recalc_csum32(udphdr->chksum, old_value, lease_time);
        }
#endif
      }
      consumed = sizeof(struct bootp_option) + opt->length;
      rem -= consumed;
      opt = (struct bootp_option *)((char *)opt + consumed);
    }

    bk_bridge_dhcp_client_ip(msg, len, &client_ip);
    if ((dhcp_msg_type == DHCP_OFFER || dhcp_msg_type == DHCP_ACK) &&
        !ip4_addr_isany_val(client_ip)) {
      bk_bridge_install_client_arp(&client_ip, msg->chaddr);
    }
  }

  return 0;
}

/*
 * @payload: l4 payload
 * Return 1 to drop the upstream pbuf, 0 to keep going.
 */
static int bk_bridge_upstream_recv_filter_l3(bridgeif_private_t *br, struct pbuf *p,
              uint16_t type, uint8_t proto, struct ip_hdr *iphdr, void *payload)
{
  LWIP_UNUSED_ARG(br);
  LWIP_UNUSED_ARG(p);
  LWIP_UNUSED_ARG(type);

  if (proto == IP_PROTO_UDP) {
    struct udp_hdr *udphdr = payload;
    uint16_t src = lwip_ntohs(udphdr->src);
    uint16_t dst = lwip_ntohs(udphdr->dest);

    if ((IP_MULTICAST(htonl(iphdr->dest.addr)))) {
      /* NetBIOS Datagram Service */
      if (src == 138 && dst == 138) {
        LWIP_LOGV("drop NBDS pkt\n");
        return 1;
      }
      /* NetBIOS Name Service */
      if (dst == 137) {
        LWIP_LOGV("drop NBNS pkt\n");
        return 1;
      }
      /* SSDP discover */
      if (dst == 1900) {
        LWIP_LOGV("drop SSDP pkt\n");
        return 1;
      }
      /* Add other filters here */
    }
  } else if (proto == IP_PROTO_IGMP) {
    LWIP_LOGV("drop IGMP pkt\n");
    return 1;
  }
  /*
   * NB: ICMP echo filtering was disabled upstream — see the commented
   * /+ why drop it? +/ block in the original code.  Left out here.
   */
  return 0;
}

/* Upstream ARP proxy on STA port. Return values match bk_bridge_hook_recv. */
static int bk_bridge_recv_arp(bridgeif_private_t *br, struct pbuf *p,
                              struct netif *rx_if, struct eth_hdr *ethhdr)
{
  struct etharp_hdr *arphdr = (struct etharp_hdr *)(ethhdr + 1);

  if (arphdr->hwtype != PP_HTONS(LWIP_IANA_HWTYPE_ETHERNET) ||
      arphdr->hwlen != ETH_HWADDR_LEN ||
      arphdr->protolen != sizeof(ip4_addr_t) ||
      arphdr->proto != PP_HTONS(ETHTYPE_IP)) {
    return 1;
  }

  if (arphdr->opcode == PP_HTONS(ARP_REQUEST)) {
    const ip4_addr_t *unused_ipaddr;
    struct eth_addr *dst_ethaddr;
    ip4_addr_t sipaddr, dipaddr;

    memcpy(&sipaddr, &arphdr->sipaddr, sizeof(ip4_addr_t));
    memcpy(&dipaddr, &arphdr->dipaddr, sizeof(ip4_addr_t));

    if (!ip4_addr_isany_val(*netif_ip4_addr(br->netif)) &&
        ip4_addr_cmp(&dipaddr, netif_ip4_addr(br->netif))) {
      if (br->netif->input(p, br->netif) != ERR_OK) {
        pbuf_free(p);
      }
      return 2;
    }

    if (etharp_find_addr(NULL, &dipaddr, &dst_ethaddr, &unused_ipaddr) >= 0) {
      bridgeif_portmask_t dst_port_msk = bridgeif_fdb_get_dst_ports(br->fdbd, dst_ethaddr);

      if (s_bridge_sap_port >= 0 && dst_port_msk == BIT(s_bridge_sap_port)) {
        etharp_raw2(rx_if, netif_get_index(rx_if),
                    (struct eth_addr *)rx_if->hwaddr, &arphdr->shwaddr,
                    (struct eth_addr *)rx_if->hwaddr, &dipaddr,
                    &arphdr->shwaddr, &sipaddr, ARP_REPLY, 0);
      }
    } else {
      etharp_raw2(br->netif, netif_get_index(rx_if),
                  (struct eth_addr *)br->netif->hwaddr, &ethhdr->dest,
                  &arphdr->shwaddr, &sipaddr,
                  &arphdr->dhwaddr, &dipaddr, ARP_REQUEST, 1);
    }
    return 1;
  }

  if (arphdr->opcode == PP_HTONS(ARP_REPLY)) {
    ip4_addr_t sipaddr;

    memcpy(&sipaddr, &arphdr->sipaddr, sizeof(ip4_addr_t));
    if (br->netif->input(p, br->netif) == ERR_OK) {
      arp_request_handle_reply(br, rx_if, &sipaddr, (uint8_t *)&arphdr->shwaddr);
    } else {
      pbuf_free(p);
    }
    return 2;
  }

  return 0;
}

/* Upstream IPv4: DHCP relay, L3 filters, optional dst-MAC rewrite. */
static int bk_bridge_recv_ip(bridgeif_private_t *br, struct pbuf *p,
                             struct netif *rx_if, struct eth_hdr *ethhdr)
{
  const s16_t ip_hdr_offset = SIZEOF_ETH_HDR;
  struct ip_hdr *iphdr = (struct ip_hdr *)((u8_t *)p->payload + ip_hdr_offset);
  u16_t iphdr_hlen;
  u16_t iphdr_len;
  u8_t proto;
  ip4_addr_t dipaddr;

  if (IPH_V(iphdr) != 4) {
    return 0;
  }

  iphdr_hlen = (u16_t)(IPH_HL(iphdr) * 4);
  iphdr_len = lwip_ntohs(IPH_LEN(iphdr));
  if (iphdr_hlen < IP_HLEN || iphdr_hlen > p->len || iphdr_len > p->tot_len) {
    return 0;
  }

  memcpy(&dipaddr, &iphdr->dest, sizeof(ip4_addr_t));
  proto = IPH_PROTO(iphdr);

  if (proto == IP_PROTO_UDP) {
    struct udp_hdr *udphdr = (struct udp_hdr *)((u8_t *)p->payload + ip_hdr_offset + iphdr_hlen);
    u16_t src = lwip_ntohs(udphdr->src);
    u16_t dest = lwip_ntohs(udphdr->dest);

    if (src == DHCP_SERVER_PORT && dest == DHCP_CLIENT_PORT) {
      u16_t udp_len = lwip_ntohs(udphdr->len);
      int avail = (int)(p->tot_len - ip_hdr_offset - iphdr_hlen - (int)sizeof(*udphdr));
      int pay_len;

      if (udp_len >= sizeof(struct udp_hdr) && avail > 0) {
        pay_len = (int)(udp_len - sizeof(struct udp_hdr));
        if (pay_len > avail) {
          pay_len = avail;
        }
        bk_bridge_upstream_handle_dhcp(rx_if, udphdr, &dipaddr,
                                     (struct dhcp_msg *)(udphdr + 1), pay_len);
        if (dipaddr.addr == IPADDR_BROADCAST) {
          return 0;
        }
      }
    } else if (src == DHCP_CLIENT_PORT && dest == DHCP_SERVER_PORT) {
      return 1;
    }
  }

  if (bk_bridge_upstream_recv_filter_l3(br, p, ethhdr->type, proto, iphdr,
                                        (u8_t *)p->payload + ip_hdr_offset + iphdr_hlen)) {
    return 1;
  }

#if BRIDGE_MODIFY_DST_ADDR
  if (dipaddr.addr != IPADDR_BROADCAST && !IP_MULTICAST(htonl(dipaddr.addr)) &&
      !ip4_addr_cmp(&dipaddr, netif_ip4_addr(br->netif))) {
    const ip4_addr_t *unused_ipaddr;
    struct eth_addr *ethaddr;
    struct eth_addr client_eth;
    struct netif *sta_if = net_get_sta_handle();

    if (bk_bridge_client_map_lookup(&dipaddr, &client_eth)) {
      memcpy(&ethhdr->dest, client_eth.addr, ETH_HWADDR_LEN);
    } else if (etharp_find_addr(NULL, &dipaddr, &ethaddr, &unused_ipaddr) >= 0 &&
               (sta_if == NULL ||
                memcmp(ethaddr, sta_if->hwaddr, ETH_ALEN) != 0)) {
      memcpy(&ethhdr->dest, ethaddr, ETH_HWADDR_LEN);
    } else {
      etharp_raw2(br->netif, netif_get_index(rx_if),
                  (struct eth_addr *)br->netif->hwaddr, &ethbroadcast,
                  (struct eth_addr *)br->netif->hwaddr, netif_ip4_addr(br->netif),
                  &ethzero, &dipaddr, ARP_REQUEST, 0);
    }
  }
#endif

  return 0;
}

/*
 * bridgeif_input() hook (upstream STA port only).
 * Return: 0 = continue 802.1D forward; 1 = drop pbuf; 2 = hook owns pbuf.
 */
int bk_bridge_hook_recv(bridgeif_private_t *br, struct pbuf *p, struct netif *rx_if)
{
  struct eth_hdr *ethhdr;
  u16_t type;

  if (p->len <= SIZEOF_ETH_HDR) {
    return 0;
  }

  ethhdr = p->payload;
  type = ethhdr->type;

  if (type == PP_HTONS(ETHTYPE_IP)) {
    bk_bridge_try_learn_dhcp(br, p, rx_if);
  }

  if (!netif_is_upstream(rx_if)) {
    return 0;
  }

  if (type == PP_HTONS(ETHTYPE_ARP)) {
    return bk_bridge_recv_arp(br, p, rx_if, ethhdr);
  }
  if (type == PP_HTONS(ETHTYPE_IP)) {
    return bk_bridge_recv_ip(br, p, rx_if, ethhdr);
  }

  return 0;
}

/*
 * Clone frame and transmit on STA; rewrite eth->src to STA MAC.
 * arp_shw: 0 = leave ARP header; 1 = set shwaddr if non-zero; 2 = always set shwaddr.
 * Returns 1 (frame consumed by hook).
 */
static int bridge_xmit_clone_sta(struct netif *tx_if, struct pbuf *p, u8_t arp_shw)
{
  struct pbuf *r = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
  struct eth_hdr *eh;

  if (r == NULL) {
    return 1;
  }

  eh = (struct eth_hdr *)r->payload;
  MEMCPY(&eh->src, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);
  if (arp_shw) {
    struct etharp_hdr *ah = (struct etharp_hdr *)(eh + 1);

    if (arp_shw == 2 || memcmp(&ah->shwaddr, &ethzero, ETH_HWADDR_LEN)) {
      MEMCPY(&ah->shwaddr, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);
    }
  }

  tx_if->linkoutput(tx_if, r);
  pbuf_free(r);
  return 1;
}

/* Gratuitous ARP or ARP probe: forward with STA L2 address only. */
static bool bridge_arp_grat_or_probe(struct etharp_hdr *hdr)
{
  if ((hdr->opcode == PP_HTONS(ARP_REQUEST) || hdr->opcode == PP_HTONS(ARP_REPLY)) &&
      etharp_ip4_addr_cmp(hdr->sipaddr, hdr->dipaddr)) {
    return true;
  }
  if (hdr->opcode == PP_HTONS(ARP_REQUEST) &&
      !memcmp(&hdr->dhwaddr, &ethzero, ETH_HWADDR_LEN) &&
      etharp_ip4_addr_isany_val(hdr->sipaddr)) {
    return true;
  }
  return false;
}

/* Upstream ARP proxy on STA egress. Returns 1 if eaten, 0 to pass through. */
static int bridge_xmit_arp(bridgeif_private_t *br, struct pbuf *p,
                           struct netif *tx_if, struct eth_hdr *ethhdr)
{
  struct etharp_hdr *hdr = (struct etharp_hdr *)(ethhdr + 1);
  const ip4_addr_t *unused_ipaddr;
  struct eth_addr *dethaddr;
  ip4_addr_t sipaddr, dipaddr;

  if (hdr->hwtype != PP_HTONS(LWIP_IANA_HWTYPE_ETHERNET) ||
      hdr->hwlen != ETH_HWADDR_LEN ||
      hdr->protolen != sizeof(ip4_addr_t) ||
      hdr->proto != PP_HTONS(ETHTYPE_IP)) {
    return 1;
  }

  if (!memcmp(&ethhdr->src, br->netif->hwaddr, ETH_ALEN)) {
    return 0;
  }

  if (hdr->opcode == PP_HTONS(ARP_REQUEST)) {
    memcpy(&sipaddr, &hdr->sipaddr, sizeof(ip4_addr_t));
    memcpy(&dipaddr, &hdr->dipaddr, sizeof(ip4_addr_t));

    if (bridge_arp_grat_or_probe(hdr)) {
      return bridge_xmit_clone_sta(tx_if, p, 1);
    }
    if (ip4_addr_cmp(&dipaddr, netif_ip4_addr(br->netif))) {
      return 1;
    }

    if (!ip4_addr_isany_val(sipaddr) &&
        !ip4_addr_cmp(&sipaddr, netif_ip4_addr(br->netif))) {
      bk_bridge_install_client_arp(&sipaddr, (uint8_t *)&hdr->shwaddr);
    }
    if (etharp_find_addr(NULL, &dipaddr, &dethaddr, &unused_ipaddr) >= 0) {
      etharp_raw2(br->netif, netif_get_index(tx_if),
                  (struct eth_addr *)br->netif->hwaddr, &hdr->shwaddr,
                  dethaddr, &dipaddr, &hdr->shwaddr, &sipaddr, ARP_REPLY, 1);
    } else {
      arp_request_add((uint8_t *)&hdr->shwaddr, sipaddr.addr, dipaddr.addr);
    }
    return bridge_xmit_clone_sta(tx_if, p, 1);
  }

  if (hdr->opcode == PP_HTONS(ARP_REPLY)) {
    return bridge_xmit_clone_sta(tx_if, p, 2);
  }

  return 0;
}

/* DHCP Discover/Request from downstream: rewrite src MAC and broadcast flag. */
static int bridge_xmit_dhcp_sta(struct netif *tx_if, struct pbuf *p,
                                struct eth_hdr *ethhdr, struct netif *brif,
                                struct ip_hdr *iphdr, u16_t ipoff, u16_t iphl)
{
  struct pbuf *r = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
  struct eth_hdr *eh;
  struct udp_hdr *udphdr;
  struct dhcp_msg *out;

  if (!memcmp(&ethhdr->src, brif->hwaddr, ETH_ALEN)) {
    return 0;
  }
  if (r == NULL) {
    return 1;
  }

  eh = (struct eth_hdr *)r->payload;
  udphdr = (struct udp_hdr *)((u8_t *)r->payload + ipoff + iphl);
  out = (struct dhcp_msg *)(udphdr + 1);

  if (!out->flags) {
#ifndef BK_LWIP_FAST_CSUM_MOD
    u32_t src_addr = iphdr->src.addr;
    u32_t dest_addr = iphdr->dest.addr;
#else
    u16_t old_value = *(uint16_t *)(&out->flags);
#endif

    out->flags |= lwip_htons(DHCP_FLAG_BROADCAST);

#ifndef BK_LWIP_FAST_CSUM_MOD
    udphdr->chksum = calculate_chksum_pseudo((uint8_t *)out, udphdr, 0x11,
                                             lwip_ntohs(udphdr->len),
                                             &src_addr, &dest_addr);
    udphdr->chksum = lwip_htons(udphdr->chksum);
#else
    udphdr->chksum = recalc_csum16(udphdr->chksum, old_value, *(uint16_t *)(&out->flags));
#endif
  }

  MEMCPY(&eh->src, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);
  tx_if->linkoutput(tx_if, r);
  pbuf_free(r);
  return 1;
}

/* Upstream IPv4 egress: DHCP client rewrite, unicast clone to STA. */
static int bridge_xmit_ip(bridgeif_private_t *br, struct pbuf *p,
                          struct netif *tx_if, struct eth_hdr *ethhdr)
{
  const u16_t ipoff = SIZEOF_ETH_HDR;
  struct ip_hdr *iphdr = (struct ip_hdr *)((u8_t *)p->payload + ipoff);
  u16_t iphl, iplen;
  u8_t proto;

  if (IPH_V(iphdr) != 4) {
    return 1;
  }

  iphl = (u16_t)(IPH_HL(iphdr) * 4);
  iplen = lwip_ntohs(IPH_LEN(iphdr));
  if (iphl < IP_HLEN || iphl > p->len || iplen > p->tot_len) {
    return memcmp(&ethhdr->src, br->netif->hwaddr, ETH_ALEN) ? 1 : 0;
  }

  proto = IPH_PROTO(iphdr);
  if (proto == IP_PROTO_UDP) {
    struct udp_hdr *udphdr = (struct udp_hdr *)((u8_t *)p->payload + ipoff + iphl);
    u16_t src = lwip_ntohs(udphdr->src);
    u16_t dest = lwip_ntohs(udphdr->dest);

    if (src == DHCP_CLIENT_PORT && dest == DHCP_SERVER_PORT) {
      return bridge_xmit_dhcp_sta(tx_if, p, ethhdr, br->netif, iphdr, ipoff, iphl);
    }
  }

  if (!IP_MULTICAST(htonl(iphdr->dest.addr))) {
    ip4_addr_t sipaddr;

    memcpy(&sipaddr, &iphdr->src, sizeof(sipaddr));
    if (!ip4_addr_isany_val(sipaddr) &&
        !ip4_addr_cmp(&sipaddr, netif_ip4_addr(br->netif)) &&
        memcmp(&ethhdr->src, br->netif->hwaddr, ETH_ALEN)) {
      bk_bridge_install_client_arp(&sipaddr, (uint8_t *)&ethhdr->src);
    }
    return bridge_xmit_clone_sta(tx_if, p, 0);
  }

  return 0;
}

/*
 * bridgeif_send_to_port() hook (STA upstream only).
 * Return: 0 = caller continues (clone + linkoutput); 1 = stop (hook sent or dropped).
 */
static int bridge_xmit(bridgeif_private_t *br, struct pbuf *p, struct netif *tx_if)
{
  struct eth_hdr *ethhdr;
  u16_t type;

  /* No rx port stamp (see bk_bridge_hook_strip_rx_ifidx): normal clone/linkoutput. */
  if (!netif_is_upstream(tx_if) || p->if_idx == NETIF_NO_INDEX) {
    return 0;
  }
  if (p->len <= SIZEOF_ETH_HDR) {
    return 1;
  }

  ethhdr = p->payload;
  type = ethhdr->type;

  if (type == PP_HTONS(ETHTYPE_ARP)) {
    return bridge_xmit_arp(br, p, tx_if, ethhdr);
  }
  if (type == PP_HTONS(ETHTYPE_IP)) {
    return bridge_xmit_ip(br, p, tx_if, ethhdr);
  }
  if (type == PP_HTONS(ETHTYPE_IPV6)) {
    return 1;
  }

  return 0;
}

struct bk_bridge_sta_disc_ctx {
  uint8_t mac[ETH_ALEN];
};

/* tcpip_callback worker: purge static ARP rows for disconnected STA MAC. */
static void bk_bridge_sta_disconnected_tcpip(void *ctx)
{
  struct bk_bridge_sta_disc_ctx *c = (struct bk_bridge_sta_disc_ctx *)ctx;

  bk_bridge_client_map_remove_mac(c->mac);
  etharp_remove_static_entry_by_mac(c->mac);
  os_free(c);
}

/* Wi-Fi event hook: schedule static-ARP cleanup on tcpip thread. */
void bk_bridge_hook_sta_disconnected(uint8_t *mac)
{
  struct bk_bridge_sta_disc_ctx *c;

  if (mac == NULL) {
    return;
  }

  c = (struct bk_bridge_sta_disc_ctx *)os_malloc(sizeof(*c));
  if (c == NULL) {
    return;
  }
  memcpy(c->mac, mac, ETH_ALEN);

  if (tcpip_callback(bk_bridge_sta_disconnected_tcpip, c) != ERR_OK) {
    os_free(c);
  }
}

/*
 * bridgeif_send_to_port() hook: run upstream proxy (bridge_xmit), else
 * clone and linkoutput.  AP always clones; want_clone ignored on AP.
 */
err_t bk_bridge_hook_xmit(bridgeif_private_t *br, struct pbuf *p,
                          struct netif *tx_if, int want_clone)
{
  struct pbuf *r;
  err_t ret;

  (void)want_clone;

  if ((void *)tx_if == net_get_sta_handle() && p->elfags == 1) {
    return ERR_OK;
  }
  if (bridge_xmit(br, p, tx_if)) {
    return ERR_OK;
  }

  r = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
  if (r == NULL) {
    return ERR_MEM;
  }
  ret = tx_if->linkoutput(tx_if, r);
  pbuf_free(r);
  return ret;
}

/*
 * bridgeif_input() calls this right before br->netif->input(p, br->netif).
 * Port RX stamps p->if_idx with the receiving STA/uap netif index; frames
 * handed to the bridge CPU port (br0) must drop that stamp so bridge_xmit()
 * does not run upstream ARP/DHCP proxy on stack-originated egress.
 */
void bk_bridge_hook_strip_rx_ifidx(struct pbuf *p)
{
  p->if_idx = NETIF_NO_INDEX;
}

/* After br0 init: reuse STA DHCP client_data when br0 has no dhcp struct. */
void bk_bridge_hook_post_init(struct netif *bridge_netif)
{
  if (netif_dhcp_data(bridge_netif) == NULL) {
    struct dhcp *dhcp = netif_dhcp_data((struct netif *)net_get_sta_handle());
    netif_set_client_data(bridge_netif,
                          LWIP_NETIF_CLIENT_DATA_INDEX_DHCP,
                          dhcp);
  }
}

/* bridgeif_add_port hook: map STA/uap to port_num; save input for deinit. */
void bk_bridge_hook_port_attach(struct netif *portif, int port_num)
{
  bridge_attach_port(portif, port_num);
  portif->input_origin = portif->input;
}

static void *s_fdb;

/* Store FDB handle for debug dump (bk_bridge_print_fdb). */
void bk_bridge_hook_fdb_init(void *fdb)
{
  s_fdb = fdb;
}

/* bridgeif_fdb_for_each callback: log one {mac, port, age}. */
static void print_fdb_one(const struct eth_addr *addr, u8_t port,
                          u32_t ts, void *arg)
{
  (void)arg;
  BK_LOGD(NULL, "%pm, eport %d, ts %d\n", addr, port, ts);
}

/* Debug CLI/helper: print all FDB entries via BK_LOGD. */
void bk_bridge_print_fdb(void)
{
  if (s_fdb == NULL) {
    return;
  }
  bridgeif_fdb_for_each(s_fdb, print_fdb_one, NULL);
}

/*
 * Bridge netif teardown: restore port input handlers, flush proxy ARP/DHCP
 * state, free FDB and bridgeif_private_t.  Replaces weak lwIP stub.
 */
void bridgeif_deinit(struct netif *netif)
{
  bridgeif_private_t *br;
  int i;

  if (netif == NULL || netif->state == NULL) {
    return;
  }
  br = (bridgeif_private_t *)netif->state;

  for (i = 0; i < br->num_ports; i++) {
    struct netif *portif = br->ports[i].port_netif;
    if (portif != NULL) {
      portif->input = portif->input_origin;
    }
  }

  bridge_detach_all_ports();
  bridgeif_fdb_deinit(br);
  s_fdb = NULL;
  mem_free(br);
  netif->state = NULL;
}
