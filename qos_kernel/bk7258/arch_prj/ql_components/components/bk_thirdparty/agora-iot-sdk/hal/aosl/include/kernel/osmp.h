/*************************************************************
 * Author		:		Lionfore Hao (haolianfu@agora.io)
 * Date			:		Feb 26th, 2020
 * Module		:		OS relative multiplex queue header
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 ~ 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __KERNEL_OSMP_H__
#define __KERNEL_OSMP_H__

#include <api/aosl_types.h>
#include <hal/aosl_hal_iomp.h>


struct mp_queue;
struct iofd;

extern int os_mp_init (struct mp_queue *q);
extern void os_mp_fini (struct mp_queue *q);

extern int os_add_event_fd (struct mp_queue *q, struct iofd *f);
extern int os_del_event_fd (struct mp_queue *q, struct iofd *f);

extern int os_poll_dispatch (struct mp_queue *q, intptr_t timeo);

#endif /* __KERNEL_OSMP_H__ */
