/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Nov 19th, 2018
 * Module:	AOSL task object internal definition file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AOSL_TASK_INTERNAL_H__
#define __AOSL_TASK_INTERNAL_H__


#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_time.h>
#include <kernel/thread.h>
#include <api/aosl_mpq.h>
#include <api/aosl_task.h>
#include <kernel/rbtree.h>
#include <kernel/refobj.h>


#ifdef __cplusplus
extern "C" {
#endif



struct task_sorted_op {
	struct aosl_rb_node rb_node;
	uintptr_t op_seq;
	const char *f_name;
	struct resume_calls *resume_calls;
	aosl_ts_t queued_ts;
	aosl_task_func_t f;
	uintptr_t argc;
	uintptr_t argv [0];
};

struct task_seq {
	struct task_seq *next;
	uintptr_t seq;
};

struct task_seq_q {
	struct task_seq *head;
	struct task_seq *tail;
	uintptr_t count;
};

struct task_op {
	struct task_op *next;
	const char *f_name;
	struct list_head *prepare_calls;
	struct resume_calls *resume_calls;
	aosl_ts_t queued_ts;
	aosl_task_func_t f;
	uintptr_t argc;
	uintptr_t argv [0];
};

struct task_op_q {
	struct task_op *head;
	struct task_op *tail;
	uintptr_t count;
};

struct task_stop_op {
	struct task_stop_op *next;
	aosl_task_func_t stop_f;
	uintptr_t argc;
	uintptr_t argv [0];
};

struct task_stop_op_q {
	struct task_stop_op *head;
	struct task_stop_op *tail;
	uintptr_t count;
};

#define TASK_F_SERIAL 0x80000000
#define TASK_F_SINGLE_THREAD_OP 0x40000000
#define TASK_F_SUPPORT_RESUME 0x20000000

#define task_type(tsk) ((aosl_task_type_t)((tsk)->f_type & 0xffff))
#define task_serial(tsk) (((tsk)->f_type & TASK_F_SERIAL) != 0)
#define task_single_thread_op(tsk) (((tsk)->f_type & TASK_F_SINGLE_THREAD_OP) != 0)
#define task_support_resume(tsk) (((tsk)->f_type & TASK_F_SUPPORT_RESUME) != 0)

struct task {
	/* MUST BE THE FIRST MEMBER */
	struct refobj ref_obj;
	uint32_t f_type; /* aosl_task_type_t + flags */

	union {
		struct /* task_single_thread_op */ {
			aosl_mpq_t st_op_q; /* only used when single thread op */
		};
		struct /* !task_single_thread_op */ {
			/* these members are protected by lock */
			aosl_mpq_t curr_done_q; /* only used when not single thread op */
			uint32_t curr_done_q_usage; /* this done q usage count */
		};
	};

	k_lock_t lock;
	struct task_op_q waiting_ops;
	struct task_stop_op_q waiting_stop_ops;
	uintptr_t task_op_seq;
	struct aosl_rb_root done_backs;
	struct task_seq_q pending_seqs;

	union {
#ifndef CONFIG_TOOLCHAIN_MS
		struct /* CPU/GEN */ {
		};
#endif

		struct /* OPA */ {
			struct aosl_rb_root pending_async_ops;
			aosl_task_res_wait_t wait_f;
			aosl_mpq_t wait_f_q;
			aosl_timer_t opa_timer;
			struct {
				/* these members are protected by lock */
				aosl_mpq_t curr_opa_op_q;
				uint32_t curr_opa_op_q_usage;
			};
		};
	};
};


static __inline__ aosl_ref_t task_ref_id (struct task *tsk)
{
	return tsk->ref_obj.obj_id;
}

#ifdef __cplusplus
}
#endif



#endif /* __AOSL_TASK_INTERNAL_H__ */