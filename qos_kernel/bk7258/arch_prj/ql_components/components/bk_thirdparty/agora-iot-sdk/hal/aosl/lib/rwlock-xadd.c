/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date  :	Oct 29th, 2018
 * Module:	rwlock implementation for Microsoft Windows
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#include <kernel/compiler.h>
#include <kernel/list.h>
#include <kernel/thread.h>
#include <kernel/rwlock.h>
#include <api/aosl_atomic.h>

/*
 * Initialize an rwlock:
 */
void rwlock_init (k_raw_rwlock_t *rw)
{
	rw->count = RWLOCK_UNLOCKED_VALUE;
	rw->wait_lock = aosl_hal_mutex_create(0);
	INIT_LIST_HEAD (&rw->wait_list);
}

enum rwlock_waiter_type {
	RWLOCK_WAITING_FOR_WRITE,
	RWLOCK_WAITING_FOR_READ
};

struct rwlock_waiter {
	struct list_head list;
	k_event_t *event;
	enum rwlock_waiter_type type;
};

enum rwlock_wake_type {
	RWLOCK_WAKE_ANY,		/* Wake whatever's at head of wait list */
	RWLOCK_WAKE_READERS,	/* Wake readers only */
	RWLOCK_WAKE_READ_OWNED	/* Waker thread holds the read lock */
};

/*
 * handle the lock release when processes blocked on it that can now run
 * - if we come here from 'xx'unlock(), then:
 *   - the 'active part' of count (&0x0000ffff) reached 0 (but may have changed)
 *   - the 'waiting part' of count (&0xffff0000) is -ve (and will still be so)
 * - there must be someone on the queue
 * - the spinlock must be held by the caller
 * - woken process blocks are discarded from the list after having task zeroed
 * - writers are only woken if downgrading is false
 */
static void __rwlock_do_wake (k_raw_rwlock_t *rw, enum rwlock_wake_type wake_type)
{
	struct rwlock_waiter *waiter;
	k_event_t *event;
	struct list_head *next;
	intptr_t oldcount, woken, loop, adjustment;

	waiter = list_entry(rw->wait_list.next, struct rwlock_waiter, list);
	if (waiter->type == RWLOCK_WAITING_FOR_WRITE) {
		if (wake_type == RWLOCK_WAKE_ANY)
			/* Wake writer at the front of the queue, but do not
			 * grant it the lock yet as we want other writers
			 * to be able to steal it.  Readers, on the other hand,
			 * will block as they will notice the queued writer.
			 */
			k_event_pulse (waiter->event);

		return;
	}

	/* Writers might steal the lock before we grant it to the next reader.
	 * We prefer to do the first reader grant before counting readers
	 * so we can bail out early if a writer stole the lock.
	 */
	adjustment = 0;
	if (wake_type != RWLOCK_WAKE_READ_OWNED) {
		adjustment = RWLOCK_ACTIVE_READ_BIAS;
 try_reader_grant:
		oldcount = rwlock_atomic_update(adjustment, rw) - adjustment;
		if (oldcount < RWLOCK_WAITING_BIAS) {
			/* A writer stole the lock. Undo our reader grant. */
			if (rwlock_atomic_update(-adjustment, rw) & RWLOCK_ACTIVE_MASK)
				return;

			/* Last active locker left. Retry waking readers. */
			goto try_reader_grant;
		}
	}

	/* Grant an infinite number of read locks to the readers at the front
	 * of the queue.  Note we increment the 'active part' of the count by
	 * the number of readers before waking any processes up.
	 */
	woken = 0;
	do {
		woken++;

		if (waiter->list.next == &rw->wait_list)
			break;

		waiter = list_entry(waiter->list.next, struct rwlock_waiter, list);
	} while (waiter->type != RWLOCK_WAITING_FOR_WRITE);

	adjustment = woken * RWLOCK_ACTIVE_READ_BIAS - adjustment;
	if (waiter->type != RWLOCK_WAITING_FOR_WRITE)
		/* hit end of list above */
		adjustment -= RWLOCK_WAITING_BIAS;

	if (adjustment)
		rwlock_atomic_add(adjustment, rw);

	next = rw->wait_list.next;
	loop = woken;
	do {
		waiter = list_entry(next, struct rwlock_waiter, list);
		next = waiter->list.next;
		event = waiter->event;
		aosl_wmb();
		waiter->event = NULL;
		k_event_pulse (event);
	} while (--loop);

	/**
	 * Removed the woken up threads from the list here,
	 * so no need to delete the list node in the target
	 * thread again.
	 * -- Lionfore Hao Oct 30th, 2018
	 **/
	rw->wait_list.next = next;
	next->prev = &rw->wait_list;
}

/*
 * wait for the read lock to be granted
 */
void rwlock_read_lock_failed (intptr_t *rw_count)
{
	k_raw_rwlock_t *rw = container_of (rw_count, k_raw_rwlock_t, count);
	intptr_t count, adjustment = -RWLOCK_ACTIVE_READ_BIAS;
	k_event_t event;
	struct rwlock_waiter waiter;

	k_event_init (&event);

	/* set up my own style of waitqueue */
	waiter.event = &event;
	waiter.type = RWLOCK_WAITING_FOR_READ;

	aosl_hal_mutex_lock (rw->wait_lock);

	if (list_empty (&rw->wait_list))
		adjustment += RWLOCK_WAITING_BIAS;

	list_add_tail (&waiter.list, &rw->wait_list);

	/* we're now waiting on the lock, but no longer actively locking */
	count = rwlock_atomic_update (adjustment, rw);

	/* If there are no active locks, wake the front queued process(es).
	 *
	 * If there are no writers and we are first in the queue,
	 * wake our own waiter to join the existing active readers !
	 */
	if (count == RWLOCK_WAITING_BIAS || (count > RWLOCK_WAITING_BIAS && adjustment != -RWLOCK_ACTIVE_READ_BIAS))
		__rwlock_do_wake (rw, RWLOCK_WAKE_ANY);

	aosl_hal_mutex_unlock (rw->wait_lock);

	/* wait to be given the lock */
	for (;;) {
		aosl_rmb ();
		if (waiter.event == NULL)
			break;

		k_event_wait (&event);
	}

	k_event_destroy (&event);
}

/*
 * wait until we successfully acquire the write lock
 */
void rwlock_write_lock_failed (intptr_t *rw_count)
{
	k_raw_rwlock_t *rw = container_of (rw_count, k_raw_rwlock_t, count);
	intptr_t count, adjustment = -RWLOCK_ACTIVE_WRITE_BIAS;
	k_event_t event;
	struct rwlock_waiter waiter;

	k_event_init (&event);

	/* set up my own style of waitqueue */
	waiter.event = &event;
	waiter.type = RWLOCK_WAITING_FOR_WRITE;

	aosl_hal_mutex_lock (rw->wait_lock);

	if (list_empty(&rw->wait_list))
		adjustment += RWLOCK_WAITING_BIAS;

	list_add_tail (&waiter.list, &rw->wait_list);

	/* we're now waiting on the lock, but no longer actively locking */
	count = rwlock_atomic_update(adjustment, rw);

	/* If there were already threads queued before us and there are no
	 * active writers, the lock must be read owned; so we try to wake
	 * any read locks that were queued ahead of us. */
	if (count > RWLOCK_WAITING_BIAS && adjustment == -RWLOCK_ACTIVE_WRITE_BIAS)
		__rwlock_do_wake(rw, RWLOCK_WAKE_READERS);

	/* wait until we successfully acquire the lock */
	for (;;) {
		if (!(count & RWLOCK_ACTIVE_MASK)) {
			/* Try acquiring the write lock. */
			count = RWLOCK_ACTIVE_WRITE_BIAS;
			if (!list_is_singular (&rw->wait_list))
				count += RWLOCK_WAITING_BIAS;

			if (rw->count == RWLOCK_WAITING_BIAS && atomic_intptr_cmpxchg (&rw->count, RWLOCK_WAITING_BIAS, count) == RWLOCK_WAITING_BIAS)
				break;
		}

		aosl_hal_mutex_unlock (rw->wait_lock);
		k_event_wait (&event);
		aosl_hal_mutex_lock (rw->wait_lock);
		count = rw->count;
	}

	__list_del_entry (&waiter.list);
	aosl_hal_mutex_unlock (rw->wait_lock);

	k_event_destroy (&event);
}

/*
 * handle waking up a waiter on the semaphore
 * - rwlock_rdunlock/rwlock_wrunlock has decremented the active part of count if we come here
 */
void rwlock_wake (intptr_t *rw_count)
{
	k_raw_rwlock_t *rw = container_of (rw_count, k_raw_rwlock_t, count);

	aosl_hal_mutex_lock (rw->wait_lock);

	/* do nothing if list empty */
	if (!list_empty (&rw->wait_list))
		__rwlock_do_wake(rw, RWLOCK_WAKE_ANY);

	aosl_hal_mutex_unlock (rw->wait_lock);
}

/*
 * downgrade a write lock into a read lock
 * - caller incremented waiting part of count and discovered it still negative
 * - just wake up any readers at the front of the queue
 */
void rwlock_downgrade_wake (intptr_t *rw_count)
{
	k_raw_rwlock_t *rw = container_of (rw_count, k_raw_rwlock_t, count);

	aosl_hal_mutex_lock (rw->wait_lock);

	/* do nothing if list empty */
	if (!list_empty(&rw->wait_list))
		__rwlock_do_wake(rw, RWLOCK_WAKE_READ_OWNED);

	aosl_hal_mutex_unlock (rw->wait_lock);
}