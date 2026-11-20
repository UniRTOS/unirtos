/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date  :	Oct 30th, 2018
 * Module:	rwlock implementation
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#ifndef __KERNEL_RWLOCK_H__
#define __KERNEL_RWLOCK_H__

#include <api/aosl_types.h>
#include <kernel/list.h>
#include <hal/aosl_hal_thread.h>

extern void rwlock_read_lock_failed (intptr_t *rw_count);
extern void rwlock_write_lock_failed (intptr_t *rw_count);
extern void rwlock_wake (intptr_t *rw_count);
extern void rwlock_downgrade_wake (intptr_t *rw_count);

typedef struct {
	intptr_t count;
	aosl_mutex_t wait_lock;
	struct list_head wait_list;
} k_raw_rwlock_t;


/* In all implementations count != 0 means locked */
static inline int rwlock_is_locked (k_raw_rwlock_t *rw)
{
	return rw->count != 0;
}

#include <kernel/rwlock-generic.h>

extern void rwlock_init (k_raw_rwlock_t *rw);

/*
 * lock for reading
 */
static __inline__ void rwlock_rdlock (k_raw_rwlock_t *rw)
{
	__rwlock_rdlock (rw);
}

/*
 * trylock for reading -- returns 1 if successful, 0 if contention
 */
static __inline__ int rwlock_tryrdlock (k_raw_rwlock_t *rw)
{
	return __rwlock_tryrdlock (rw);
}

/*
 * lock for writing
 */
static __inline__ void rwlock_wrlock (k_raw_rwlock_t *rw)
{
	__rwlock_wrlock (rw);
}

/*
 * trylock for writing -- returns 1 if successful, 0 if contention
 */
static __inline__ int rwlock_trywrlock (k_raw_rwlock_t *rw)
{
	return __rwlock_trywrlock (rw);
}

/*
 * release a read lock
 */
static __inline__ void rwlock_rdunlock (k_raw_rwlock_t *rw)
{
	__rwlock_rdunlock (rw);
}

/*
 * release a write lock
 */
static __inline__ void rwlock_wrunlock (k_raw_rwlock_t *rw)
{
	__rwlock_wrunlock (rw);
}

/*
 * downgrade write lock to read lock
 */
static __inline__ void rwlock_wr2rd_lock (k_raw_rwlock_t *rw)
{
	/*
	 * lockdep: a downgraded write will live on as a write
	 * dependency.
	 */
	__rwlock_wr2rd_lock (rw);
}

#endif /* __KERNEL_RWLOCK_H__ */