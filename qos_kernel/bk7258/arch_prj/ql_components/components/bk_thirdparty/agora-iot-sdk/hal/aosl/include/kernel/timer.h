/*************************************************************
 * Author		:		Lionfore Hao (haolianfu@agora.io)
 * Date			:		Jul 28th, 2018
 * Module		:		Red-Black tree based timer header file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __TIMER_H__
#define __TIMER_H__

#include <kernel/kernel.h>
#include <kernel/list.h>
#include <kernel/rbtree.h>
#include <kernel/atomic.h>

#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_mpq_timer.h>
#include <api/aosl_time.h>


struct timer_node {
	struct list_head node; /* node for multiplex queue timers table */

	struct aosl_rb_node timer_node;
	struct timer_node *timer_prev;
	struct timer_node *timer_next;

	aosl_ref_t obj_id;
	atomic_t usage;

	aosl_mpq_t q;

	uintptr_t interval;
	aosl_ts_t expire_time;
	aosl_timer_func_t func;
	aosl_obj_dtor_t dtor;

	uintptr_t argc;
	uintptr_t argv [0];
};

extern void __free_timer (struct timer_node *timer);

static __inline__ void __timer_get (struct timer_node *timer)
{
	atomic_inc (&timer->usage);
}

static __inline__ void __timer_put (struct timer_node *timer)
{
	if (atomic_dec_and_test (&timer->usage))
		__free_timer (timer);
}

struct timer_base {
	struct aosl_rb_root active;
	struct timer_node *first;
};

struct mp_queue;

extern void mpq_init_timers (struct mp_queue *q);

extern int __check_and_run_timers (struct mp_queue *q);

extern void mpq_fini_timers (struct mp_queue *q);


/*
 *	These inlines deal with timer wrapping correctly. You are
 *	strongly encouraged to use them
 *	1. Because people otherwise forget
 *	2. Because if the timer wrap changes in future you won't have to
 *	   alter your driver code.
 *
 * time_after(a,b) returns true if the time a is after time b.
 *
 * Do this with "<0" and ">=0" to only test the sign of the result. A
 * good compiler would generate better code (and a really good compiler
 * wouldn't care). Gcc is currently neither.
 */
#define time_after(a,b)		\
	((int64_t)((a) - (b)) > 0)
#define time_before(a,b)	time_after(b,a)

#define time_after_eq(a,b)	\
	((int64_t)((a) - (b)) >= 0)
#define time_before_eq(a,b)	time_after_eq(b,a)

/*
 * Calculate whether a is in the range of [b, c].
 */
#define time_in_range(a,b,c) \
	(time_after_eq(a,b) && time_before_eq(a,c))

/*
 * Calculate whether a is in the range of [b, c).
 */
#define time_in_range_open(a,b,c) \
	(time_after_eq(a,b) && time_before(a,c))

/* Same as above, but does so with platform independent 64bit types.
 * These must be used when utilizing jiffies_64 (i.e. return value of
 * get_jiffies_64() */
#define time_after64(a,b)	\
	((int64_t)((a) - (b)) > 0)
#define time_before64(a,b)	time_after64(b,a)

#define time_after_eq64(a,b)	\
	((int64_t)((a) - (b)) >= 0)
#define time_before_eq64(a,b)	time_after_eq64(b,a)



#endif /* __TIMER_H__ */
