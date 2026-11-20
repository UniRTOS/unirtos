/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Sep 24th, 2018
 * Module:	AOSL threading relative internal implementations.
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#include <stdio.h>
#include <string.h>

#include <kernel/kernel.h>
#include <kernel/types.h>
#include <kernel/err.h>
#include <kernel/thread.h>
#include <api/aosl_mm.h>
#include <api/aosl_thread.h>
#include <api/aosl_time.h>


void k_raw_rwlock_init (k_raw_rwlock_t *rw)
{
	rwlock_init (rw);
}

void k_raw_rwlock_rdlock (k_raw_rwlock_t *rw)
{
	rwlock_rdlock (rw);
}

int k_raw_rwlock_tryrdlock (k_raw_rwlock_t *rw)
{
	return rwlock_tryrdlock (rw);
}

void k_raw_rwlock_wrlock (k_raw_rwlock_t *rw)
{
	rwlock_wrlock (rw);
}

int k_raw_rwlock_trywrlock (k_raw_rwlock_t *rw)
{
	return rwlock_trywrlock (rw);
}

void k_raw_rwlock_rdunlock (k_raw_rwlock_t *rw)
{
	rwlock_rdunlock (rw);
}

void k_raw_rwlock_wrunlock (k_raw_rwlock_t *rw)
{
	rwlock_wrunlock (rw);
}

void k_raw_rwlock_destroy (k_raw_rwlock_t *rw)
{
	aosl_hal_mutex_destroy (rw->wait_lock);
}