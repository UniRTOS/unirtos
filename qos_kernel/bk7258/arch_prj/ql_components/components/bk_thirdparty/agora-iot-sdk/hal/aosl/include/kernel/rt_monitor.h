/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Jul 26th, 2018
 * Module:	AOSL netlink/interface helpers for linux header file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __LINUX_ROUTE_MONITOR_H__
#define __LINUX_ROUTE_MONITOR_H__

#include <api/aosl_route.h>

#if defined (__linux__)
extern int os_subscribe_net_events (aosl_net_ev_func_t f, void *arg);
extern void os_unsubscribe_net_events (void);
#else
extern int os_subscribe_net_events (aosl_net_ev_func_t f, void *arg) {return -1;}
extern void os_unsubscribe_net_events (void) {}
#endif


#endif /* __LINUX_ROUTE_MONITOR_H__ */
