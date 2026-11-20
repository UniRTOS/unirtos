#include "volc_network.h"
#include "volc_errno.h"
#include "volc_type.h"
#include "port/net.h"
#include <components/netif.h>
#include <netdb.h>

#ifndef EAI_SYSTEM
#define EAI_SYSTEM -11
#endif

uint32_t _volc_ip_address_from_string(volc_ip_addr_t* p_ip_address, char* p_ip_address_string){
    int port = 0;
    sscanf(p_ip_address_string,"%d.%d.%d.%d:%d",&p_ip_address->address[0],&p_ip_address->address[1],&p_ip_address->address[2],&p_ip_address->address[3],&port);
    p_ip_address->family = VOLC_IP_FAMILY_TYPE_IPV4;
    p_ip_address->port = port; 
    return VOLC_STATUS_SUCCESS;
};

uint32_t volc_get_local_ip(volc_ip_addr_t* dest_ip_list, uint32_t* p_dest_ip_list_len, volc_network_callback_t callback, uint64_t custom_data) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    uint32_t ip_count = 0;
#if LWIP_IPV4
    netif_ip4_config_t ip4_config;
    char ipv4[64] = {0};

    if (uap_ip_is_start()) {
        ret = bk_netif_get_ip4_config(NETIF_IF_AP, &ip4_config);
        if (BK_OK == ret) {
            snprintf(ipv4,64,"%s:0",ip4_config.ip);
            if(_volc_ip_address_from_string(&dest_ip_list[ip_count], ipv4) == VOLC_STATUS_SUCCESS) {
                ip_count++;
            }
        }
    }

    if (sta_ip_is_start()) {
        ret = bk_netif_get_ip4_config(NETIF_IF_STA, &ip4_config);
        if (BK_OK == ret) {
            snprintf(ipv4,64,"%s:0",ip4_config.ip);
            if(_volc_ip_address_from_string(&dest_ip_list[ip_count], ipv4) == VOLC_STATUS_SUCCESS) {
                ip_count++;
            }
        }
    }

#if CONFIG_NET_PAN
    if (pan_ip_is_start()) {
        ret = bk_netif_get_ip4_config(NETIF_IF_PAN, &ip4_config);
        if (BK_OK == ret) {
            snprintf(ipv4,64,"%s:0",ip4_config.ip);
            if(_volc_ip_address_from_string(&dest_ip_list[ip_count], ipv4) == VOLC_STATUS_SUCCESS) {
                ip_count++;
            }
        }
    }
#endif

#if CONFIG_LWIP_PPP_SUPPORT
    if (ppp_ip_is_start()) {
        ret = bk_netif_get_ip4_config(NETIF_IF_PPP, &ip4_config);
        if (BK_OK == ret) {
            snprintf(ipv4,64,"%s:0",ip4_config.ip);
            if(_volc_ip_address_from_string(&dest_ip_list[ip_count], ipv4) == VOLC_STATUS_SUCCESS) {
                ip_count++;
            }
        }
    }
#endif

#endif

    *p_dest_ip_list_len = ip_count;
    return VOLC_STATUS_SUCCESS;
}

// getIpWithHostName
uint32_t volc_get_ip_with_host_name(const char* hostname, volc_ip_addr_t* dest_ip) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    int32_t err_code;
    //char* err_str;
    struct addrinfo *res, *rp;
    bool resolved = false;
    struct sockaddr_in* ipv4Addr;
    #if LWIP_IPV6
    struct sockaddr_in6* ipv6Addr;
    #endif
    VOLC_CHK(hostname != NULL, VOLC_STATUS_NULL_ARG);
    err_code = getaddrinfo(hostname, NULL, NULL, &res);
    if (err_code != 0) {
        //err_str = err_code == EAI_SYSTEM ? strerror(errno) : (char *) gai_strerror(err_code);
        ret = VOLC_STATUS_RESOLVE_HOSTNAME_FAILED;
        goto err_out_label;
    }

    for (rp = res; rp != NULL && !resolved; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            ipv4Addr = (struct sockaddr_in*) rp->ai_addr;
            dest_ip->family = VOLC_IP_FAMILY_TYPE_IPV4;
            memcpy(dest_ip->address, &ipv4Addr->sin_addr, VOLC_IPV4_ADDRESS_LENGTH);
            resolved = true;
        } else if (rp->ai_family == AF_INET6) {
            #if LWIP_IPV6
            ipv6Addr = (struct sockaddr_in6*) rp->ai_addr;
            dest_ip->family = VOLC_IP_FAMILY_TYPE_IPV6;
            memcpy(dest_ip->address, &ipv6Addr->sin6_addr, VOLC_IPV6_ADDRESS_LENGTH);
            resolved = true;
            #endif
        }
    }

    freeaddrinfo(res);
    if (!resolved) {
        ret = VOLC_STATUS_HOSTNAME_NOT_FOUND;
    }
err_out_label:
    return ret;
}
int volc_inet_pton (int __af, const char *__restrict __cp,void *__restrict __buf) {
    int af = AF_INET;
    if(__af == VOLC_IP_FAMILY_TYPE_IPV6 ) {
        af = AF_INET6;
    }
    return inet_pton(af,__cp,__buf);
};

 uint16_t volc_htons(uint16_t hostshort){
    return htons(hostshort);
 };
uint16_t volc_ntohs(uint16_t netshort){
    return ntohs(netshort);
}
