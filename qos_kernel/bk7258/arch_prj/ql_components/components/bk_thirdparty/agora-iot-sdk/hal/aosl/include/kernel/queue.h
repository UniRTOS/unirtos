/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Oct 26th, 2019
 * Module:	AOSL queue object internal definition file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 ~ 2019 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AOSL_QUEUE_INTERNAL_H__
#define __AOSL_QUEUE_INTERNAL_H__


#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_time.h>
#include <kernel/list.h>
#include <kernel/thread.h>
#include <api/aosl_mpq.h>
#include <api/aosl_queue.h>
#include <kernel/refobj.h>


#ifdef __cplusplus
extern "C" {
#endif



struct queue_op {
	struct list_head node;
	const char *f_name;
	aosl_queue_func_t f;
	aosl_ts_t queued_ts;
	atomic_t usage;
	uintptr_t life_id;
	uintptr_t argc;
	uintptr_t argv [0];
};

struct queue_op_q {
	struct list_head qops;
	uintptr_t count;
	uintptr_t qop_life_counter;
};

#define HAS_DEDICATED_Q 0x80000000

#define has_dedicated_q(que) ((((struct queue *)(que))->q_flags & HAS_DEDICATED_Q) != 0)

struct queue {
	/* MUST BE THE FIRST MEMBER */
	struct refobj ref_obj;
	uint32_t q_flags; /* queue flags */
	union {
		struct /* dedicated_q */ {
			aosl_mpq_t dedicated_q;
		};

		struct /* !dedicated_q */ {
			/* these members are protected by lock */
			aosl_mpq_t curr_q; /* only used when not dedicated q op */
			uint32_t curr_q_usage; /* this dedicated q usage count */
		};
	};

	k_lock_t lock;
	uint32_t q_max; /* queue capacity */
	struct queue_op_q queued_ops;
	struct queue_op *run_qop;
};

#define queue_ref_id(inp) ((aosl_ref_t)((struct queue *)(que))->ref_obj.obj_id)


#ifdef __cplusplus
}
#endif



#endif /* __AOSL_QUEUE_INTERNAL_H__ */