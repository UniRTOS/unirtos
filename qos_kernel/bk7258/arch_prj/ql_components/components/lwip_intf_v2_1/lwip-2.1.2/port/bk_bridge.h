#ifndef BK_BRIDGE_H
#define BK_BRIDGE_H

#include "netif/bridgeif.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pbuf;
struct netif;

/* bridgeif_input hook: 0=forward, 1=drop, 2=hook consumed pbuf */
int bk_bridge_hook_recv(bridgeif_private_t *br, struct pbuf *p, struct netif *rx_if);
/* bridgeif_send_to_port hook; AP always clones, CP may pass p when want_clone=0 */
err_t bk_bridge_hook_xmit(bridgeif_private_t *br, struct pbuf *p,
                          struct netif *tx_if, int want_clone);
/* Clear p->if_idx before br0->input(); egress proxy skips such pbufs (see bridge_xmit). */
void bk_bridge_hook_strip_rx_ifidx(struct pbuf *p);
void bk_bridge_hook_post_init(struct netif *bridge_netif);   /* br0 init: share STA dhcp client_data */
void bk_bridge_hook_port_attach(struct netif *portif, int port_num); /* record port; save input */
void bk_bridge_hook_fdb_init(void *fdb);                    /* save FDB for bk_bridge_print_fdb */
void bk_bridge_hook_sta_disconnected(uint8_t *mac);         /* flush static ARP after STA down */
void bk_bridge_print_fdb(void);                             /* debug: dump FDB */

#ifdef __cplusplus
}
#endif

#endif /* BK_BRIDGE_H */
