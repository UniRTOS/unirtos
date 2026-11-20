/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Nov 19th, 2018
 * Module:	AOSL task object implementation file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/


#include <stdlib.h>

#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_alloca.h>
#include <api/aosl_mm.h>
#include <api/aosl_mpq.h>
#include <api/aosl_mpqp.h>
#include <api/aosl_integer_wrappings.h>
#include <kernel/mp_queue.h>
#include <kernel/mpq_pool.h>
#include <kernel/task.h>
#include <kernel/bug.h>
#include <kernel/err.h>


static __inline__ void __task_inc_op_seq (struct task *tsk)
{
	tsk->task_op_seq++;
	/* 1 is the first seq number, and 0 is the only invalid seq */
	if (tsk->task_op_seq == 0)
		tsk->task_op_seq = 1;
}

static __inline__ void __task_seq_q_add (struct task_seq_q *q, uintptr_t seq)
{
	struct task_seq *node = (struct task_seq *)aosl_malloc (sizeof (struct task_seq));
	if (node == NULL)
		abort ();

	node->seq = seq;
	node->next = NULL;

	if (q->tail != NULL) {
		q->tail->next = node;
	} else {
		q->head = node;
	}
	q->tail = node;
	q->count++;
}

static __inline__ int __task_seq_q_head_seq (struct task_seq_q *q, uintptr_t *seq_p)
{
	struct task_seq *node = q->head;
	if (node == NULL)
		return -1;

	*seq_p = node->seq;
	return 0;
}

static __inline__ struct task_seq *__task_seq_q_remove_head (struct task_seq_q *q)
{
	struct task_seq *node = q->head;
	if (node == NULL)
		return NULL;

	q->head = node->next;
	if (q->head == NULL)
		q->tail = NULL;

	q->count--;
	node->next = NULL;
	return node;
}

static int __task_seq_q_remove_free_head (struct task_seq_q *q)
{
	struct task_seq *node = __task_seq_q_remove_head (q);
	if (node != NULL) {
		aosl_free (node);
		return 0;
	}

	return -1;
}

static __inline__ struct task_op *__task_op_alloc (const char *f_name, aosl_task_func_t f, uintptr_t argc, uintptr_t *argv,
															struct list_head *prepare_calls, struct resume_calls *resume_calls)
{
	struct task_op *aop;
	uintptr_t l;

	aop = (struct task_op *)aosl_malloc (sizeof (struct task_op) + (sizeof (uintptr_t) * argc));
	if (aop == NULL) {
		abort ();
		return NULL;
	}

	aop->f_name = aosl_strdup (f_name);
	aop->prepare_calls = prepare_calls;
	aop->resume_calls = resume_calls;
	aop->queued_ts = aosl_tick_now ();
	aop->f = f;
	aop->argc = argc;
	for (l = 0; l < argc; l++)
		aop->argv [l] = argv [l];

	return aop;
}

static __inline__ void __task_op_q_add_op (struct task_op_q *q, struct task_op *aop)
{
	aop->next = NULL;
	if (q->tail != NULL) {
		q->tail->next = aop;
	} else {
		q->head = aop;
	}
	q->tail = aop;
	q->count++;
}

static void __opa_ops_monitor (aosl_timer_t timer_id, const aosl_ts_t *now_p, uintptr_t argc, uintptr_t argv []);

static void __start_opa_ops_monitor_timer (struct task *tsk)
{
	aosl_mpq_t opa_timer_q;

	if (task_single_thread_op (tsk)) {
		opa_timer_q = tsk->st_op_q;
	} else {
		opa_timer_q = aosl_mpq_main ();
	}

	if (aosl_mpq_invalid (opa_timer_q))
		abort ();

	tsk->opa_timer = aosl_mpq_set_timer_on_q (opa_timer_q, 600, __opa_ops_monitor, NULL, 1, task_ref_id (tsk));
	if (aosl_mpq_timer_invalid (tsk->opa_timer))
		abort ();
}

static __inline__ void __task_op_wait_add (struct task *tsk, const char *f_name, aosl_task_func_t f, uintptr_t argc, uintptr_t *argv,
																	struct list_head *prepare_calls, struct resume_calls *resume_calls)
{
	struct task_op *aop = __task_op_alloc (f_name, f, argc, argv, prepare_calls, resume_calls);
	__task_op_q_add_op (&tsk->waiting_ops, aop);
	if (task_type (tsk) == aosl_task_type_opa && tsk->waiting_ops.count > 3) {
		/**
		 * For the OPA type of task, if we found that the waiting ops
		 * count greater than 3, then start the monitor timer to track
		 * whether the async operation done callback need to issue more
		 * ops to trigger the done callbacks, such as the video decoding
		 * with B frame.
		 * -- Lionfore Hao May 1st, 2020
		 **/
		if (aosl_mpq_timer_invalid (tsk->opa_timer))
			__start_opa_ops_monitor_timer (tsk);
	}
}

static __inline__ struct task_op *__task_op_q_remove_head (struct task_op_q *q)
{
	struct task_op *node = q->head;
	if (node == NULL)
		return NULL;

	q->head = node->next;
	if (q->head == NULL)
		q->tail = NULL;

	q->count--;
	node->next = NULL;
	return node;
}

static __inline__ struct task_stop_op *__task_stop_op_alloc (aosl_task_func_t stop_f, uintptr_t argc, uintptr_t *argv)
{
	struct task_stop_op *stop_op;
	uintptr_t l;

	stop_op = (struct task_stop_op *)aosl_malloc (sizeof (struct task_stop_op) + (sizeof (uintptr_t) * argc));
	if (stop_op == NULL) {
		abort ();
		return NULL;
	}

	stop_op->stop_f = stop_f;
	stop_op->argc = argc;
	for (l = 0; l < argc; l++)
		stop_op->argv [l] = argv [l];

	return stop_op;
}

static __inline__ void __task_stop_op_q_add_stop_op (struct task_stop_op_q *q, struct task_stop_op *stop_op)
{
	stop_op->next = NULL;
	if (q->tail != NULL) {
		q->tail->next = stop_op;
	} else {
		q->head = stop_op;
	}
	q->tail = stop_op;
	q->count++;
}

static __inline__ void __task_stop_op_q_add (struct task_stop_op_q *q, aosl_task_func_t stop_f, uintptr_t argc, uintptr_t *argv)
{
	struct task_stop_op *stop_op = __task_stop_op_alloc (stop_f, argc, argv);
	__task_stop_op_q_add_stop_op (q, stop_op);
}

static __inline__ struct task_stop_op *__task_stop_op_q_remove_head (struct task_stop_op_q *q)
{
	struct task_stop_op *node = q->head;
	if (node == NULL)
		return NULL;

	q->head = node->next;
	if (q->head == NULL)
		q->tail = NULL;

	q->count--;
	node->next = NULL;
	return node;
}

static int cmp_seq (struct aosl_rb_node *rb_node, struct aosl_rb_node *node, va_list args)
{
	struct task_sorted_op *rb_entry = aosl_rb_entry (rb_node, struct task_sorted_op, rb_node);
	uintptr_t seq;

	if (node != NULL) {
		seq = aosl_rb_entry (node, struct task_sorted_op, rb_node)->op_seq;
	} else {
		seq = va_arg (args, uintptr_t);
	}

	if (aosl_uintptr_after (rb_entry->op_seq, seq))
		return 1;

	if (aosl_uintptr_before (rb_entry->op_seq, seq))
		return -1;

	return 0;
}

static int task_ctor (struct refobj *robj, void *arg, aosl_ref_dtor_t dtor, int modify_async, int rdlock_recursive, int caller_free, va_list args)
{
	int err;
	struct task *tsk;
	aosl_task_type_t type = (aosl_task_type_t)va_arg (args, int);
	int serial = va_arg (args, int);
	aosl_mpq_t st_op_q = va_arg (args, aosl_mpq_t);
	int resume = va_arg (args, int);
	aosl_task_res_wait_t wait_f = va_arg (args, aosl_task_res_wait_t);
	uint32_t f_type = type;

	err = refobj_type_obj.ctor (robj, arg, dtor, modify_async, rdlock_recursive, caller_free, args);
	if (err < 0)
		return err;

	tsk = (struct task *)robj;
	if (serial)
		f_type |= TASK_F_SERIAL;

	if (!aosl_mpq_invalid (st_op_q)) {
		tsk->st_op_q = st_op_q;
		f_type |= TASK_F_SINGLE_THREAD_OP;
		if (resume)
			f_type |= TASK_F_SUPPORT_RESUME;
	} else {
		tsk->curr_done_q = AOSL_MPQ_INVALID;
		tsk->curr_done_q_usage = 0;
	}

	tsk->f_type = f_type;
	k_lock_init (&tsk->lock);
	tsk->waiting_ops.head = NULL;
	tsk->waiting_ops.tail = NULL;
	tsk->waiting_ops.count = 0;
	tsk->waiting_stop_ops.head = NULL;
	tsk->waiting_stop_ops.tail = NULL;
	tsk->waiting_stop_ops.count = 0;
	/* 1 is the first seq number, and 0 is the only invalid seq */
	tsk->task_op_seq = 1;
	aosl_rb_root_init (&tsk->done_backs, cmp_seq);
	tsk->pending_seqs.head = NULL;
	tsk->pending_seqs.tail = NULL;
	tsk->pending_seqs.count = 0;

	if (type == aosl_task_type_opa) {
		aosl_rb_root_init (&tsk->pending_async_ops, cmp_seq);
		tsk->wait_f = wait_f;
		tsk->wait_f_q = AOSL_MPQ_INVALID;
		tsk->curr_opa_op_q = AOSL_MPQ_INVALID;
		tsk->curr_opa_op_q_usage = 0;
		if (wait_f != NULL) {
			aosl_mpq_t q;
			q = aosl_mpq_create_flags (AOSL_MPQ_FLAG_NONBLOCK, AOSL_THRD_PRI_NORMAL, 0, 10000, "AsyncObjectWait", NULL, NULL, NULL);
			if (aosl_mpq_invalid (q)) {
				err = -aosl_errno;
				if (err == 0)
					err = -AOSL_EINVAL;

				/**
				 * Call the destructor of the base class here because
				 * we have already constructed the base class.
				 * -- Lionfore Hao Jun 29th, 2019
				 **/
				refobj_type_obj.dtor (robj);
				return err;
			}

			tsk->wait_f_q = q;
		}

		tsk->opa_timer = AOSL_MPQ_TIMER_INVALID;
	}

	return 0;
}

struct prepare_node {
	struct list_head node;
	aosl_stack_id_t stack_id;
	const char *f_name;
	aosl_prepare_func_t f;
	uintptr_t argc;
	uintptr_t argv [0];
};

static __inline__ void __free_prepare_node (struct prepare_node *node)
{
	if (node->f_name != NULL)
		aosl_free ((void *)node->f_name);

	aosl_free ((void *)node);
}

static aosl_stack_id_t invoke_prepare_calls (struct list_head *prepare_calls, const aosl_ts_t *queued_ts_p, int free_only)
{
	aosl_ts_t time_stamp = 0;
	uint32_t wait_us = 0;
	struct mp_queue *this_q = THIS_MPQ ();
	aosl_stack_id_t err_stack = AOSL_STACK_INVALID;

	if (____sys_perf_f != NULL) {
		time_stamp = aosl_tick_us ();
		if (queued_ts_p != NULL)
			wait_us = (uint32_t)(time_stamp - (*queued_ts_p * 1000));
	}

	for (;;) {
		struct prepare_node *node;
		int r;
		struct mpq_stack *curr_stack;
		struct mpq_stack stack;

		node = list_remove_head_entry (prepare_calls, struct prepare_node, node);
		if (node == NULL)
			break;

		mpq_stack_init (&stack, node->stack_id);

		if (____sys_perf_f != NULL)
			time_stamp = aosl_tick_us ();

		curr_stack = this_q->q_stack_curr;
		this_q->q_stack_curr = &stack;
		r = node->f (free_only, node->argc, node->argv);
		this_q->q_stack_curr = curr_stack;

		if (____sys_perf_f != NULL)
			____sys_perf_f (node->f_name, free_only, wait_us, (uint32_t)(aosl_tick_us () - time_stamp));

		mpq_stack_fini (&stack);

		if (r < 0) {
			/**
			 * Once we encountered a failed prepare case, then do
			 * free only for the left prepare function calls in
			 * the stack chain, and return error.
			 * -- Lionfore Hao Sep 15th, 2020
			 **/
			free_only = 1;
			if (aosl_stack_invalid (err_stack))
				err_stack = node->stack_id;
		}

		__free_prepare_node (node);
	}

	return err_stack;
}

static aosl_stack_id_t do_prepare_calls (struct list_head *prepare_calls, const aosl_ts_t *queued_ts_p)
{
	if (prepare_calls != NULL) {
		aosl_stack_id_t err_stack;

		err_stack = invoke_prepare_calls (prepare_calls, queued_ts_p, 0/* !free_only */);
		aosl_free ((void *)prepare_calls);
		if (!aosl_stack_invalid (err_stack))
			THIS_MPQ ()->q_stack_curr->err_stack_id = err_stack;

		return err_stack;
	}

	return AOSL_STACK_INVALID;
}

void free_prepare_calls (struct list_head *prepare_calls, const aosl_ts_t *queued_ts_p)
{
	if (prepare_calls != NULL) {
		invoke_prepare_calls (prepare_calls, queued_ts_p, 1/* free_only */);
		aosl_free ((void *)prepare_calls);
	}
}

struct resume_node {
	struct list_head node;
	aosl_stack_id_t stack_id;
	const char *f_name;
	aosl_resume_func_t f;
	uintptr_t argc;
	uintptr_t argv [0];
};

static __inline__ void __free_resume_node (struct resume_node *node)
{
	if (node->f_name != NULL)
		aosl_free ((void *)node->f_name);

	aosl_free ((void *)node);
}

static void invoke_resume_calls (struct list_head *resume_calls, const aosl_ts_t *queued_ts_p, int free_only)
{
	aosl_ts_t time_stamp = 0;
	uint32_t wait_us = 0;
	struct mp_queue *this_q = THIS_MPQ ();
	aosl_stack_id_t err_stack;

	if (____sys_perf_f != NULL) {
		time_stamp = aosl_tick_us ();
		if (queued_ts_p != NULL)
			wait_us = (uint32_t)(time_stamp - (*queued_ts_p * 1000));
	}

	for (;;) {
		struct resume_node *node;
		int do_free_only;
		struct mpq_stack *curr_stack;
		struct mpq_stack stack;
		struct resume_calls *stack_resume_calls;

		node = list_remove_head_entry (resume_calls, struct resume_node, node);
		if (node == NULL)
			break;

		mpq_stack_init (&stack, node->stack_id);
		curr_stack = this_q->q_stack_curr;

		if (____sys_perf_f != NULL)
			time_stamp = aosl_tick_us ();

		do_free_only = free_only;
		err_stack = curr_stack->err_stack_id;
		if (!do_free_only) {
			if (curr_stack->task_exec_err < 0) {
				do_free_only = (aosl_stack_invalid (err_stack) || node->stack_id >= err_stack);
			} else {
				do_free_only = (!aosl_stack_invalid (err_stack) && node->stack_id >= err_stack);
			}
		}

		this_q->q_stack_curr = &stack;
		node->f (do_free_only, node->argc, node->argv);
		this_q->q_stack_curr = curr_stack;

		if (____sys_perf_f != NULL)
			____sys_perf_f (node->f_name, free_only, wait_us, (uint32_t)(aosl_tick_us () - time_stamp));

		__free_resume_node (node);

		stack_resume_calls = stack.resume_calls;
		mpq_stack_fini (&stack);

		if (!IS_ERR_OR_NULL (stack_resume_calls)) {
			list_splice_tail_init (resume_calls, &stack_resume_calls->list);
			break;
		}
	}
}

static __inline__ void resume_calls_get (struct resume_calls *resume_calls)
{
	atomic_inc (&resume_calls->usage);
}

void resume_calls_put (struct resume_calls *resume_calls, const aosl_ts_t *queued_ts_p)
{
	if (atomic_dec_and_test ((atomic_t *)&resume_calls->usage)) {
		invoke_resume_calls (&resume_calls->list, queued_ts_p, (resume_calls->task_count > 0)/* free_only */);
		aosl_free ((void *)resume_calls);
	}
}

static __inline__ void resume_calls_task_exec (struct resume_calls *resume_calls)
{
	resume_calls_get (resume_calls);
	resume_calls->task_count++;
}

static __inline__ void resume_calls_task_done (struct resume_calls *resume_calls, const aosl_ts_t *queued_ts_p)
{
	resume_calls->task_count--;
	resume_calls_put (resume_calls, queued_ts_p);
}

static __inline__ void resume_calls_task_free (struct resume_calls *resume_calls)
{
	resume_calls_put (resume_calls, NULL);
}

static void __inline__ __free_task_op (struct task_op *aop)
{
	if (aop->f_name != NULL)
		aosl_free ((void *)aop->f_name);

	if (aop->prepare_calls != NULL)
		free_prepare_calls (aop->prepare_calls, &aop->queued_ts);

	if (aop->resume_calls != NULL)
		resume_calls_put (aop->resume_calls, &aop->queued_ts);

	aosl_free ((void *)aop);
}

static void __inline__ __free_task_stop_op (struct task_stop_op *stop_op)
{
	aosl_free ((void *)stop_op);
}

static void __inline__ __free_task_sorted_op (struct task_sorted_op *sorted_op)
{
	if (sorted_op->f_name != NULL)
		aosl_free ((void *)sorted_op->f_name);

	if (sorted_op->resume_calls != NULL)
		resume_calls_put (sorted_op->resume_calls, &sorted_op->queued_ts);

	aosl_free ((void *)sorted_op);
}

static void __inline__ tsk_invoke_f (const char *f_name, const aosl_ts_t *queued_ts_p, aosl_task_func_t f, aosl_refobj_t robj,
						aosl_task_act_t act, uintptr_t seq, uintptr_t argc, uintptr_t *argv, struct resume_calls *resume_calls)
{
	aosl_ts_t time_stamp = 0;
	uint32_t wait_us = 0;

	if (____sys_perf_f != NULL) {
		time_stamp = aosl_tick_us ();
		wait_us = (uint32_t)(time_stamp - (*queued_ts_p * 1000));
	}

	f (robj, act, seq, argc, argv);

	if (____sys_perf_f != NULL)
		____sys_perf_f (f_name, (int)(act == aosl_task_act_free), wait_us, (uint32_t)(aosl_tick_us () - time_stamp));

	if (act == aosl_task_act_done && resume_calls != NULL)
		resume_calls_task_done (resume_calls, queued_ts_p);
}

static void task_dtor (struct refobj *robj)
{
	struct task *tsk = (struct task *)robj;
	struct task_stop_op *stop_op;
	struct task_op *aop;

	while ((stop_op = __task_stop_op_q_remove_head (&tsk->waiting_stop_ops)) != NULL) {
		stop_op->stop_f (AOSL_FREE_ONLY_OBJ, aosl_task_act_free, 0, stop_op->argc, stop_op->argv);
		__free_task_stop_op (stop_op);
	}

	while ((aop = __task_op_q_remove_head (&tsk->waiting_ops)) != NULL) {
		tsk_invoke_f (aop->f_name, &aop->queued_ts, aop->f, AOSL_FREE_ONLY_OBJ, aosl_task_act_free, 0, aop->argc, aop->argv, NULL);
		__free_task_op (aop);
	}

	if (task_type (tsk) == aosl_task_type_opa) {
		while (tsk->pending_async_ops.rb_node != NULL) {
			struct aosl_rb_node *rb_node = tsk->pending_async_ops.rb_node;
			struct task_sorted_op *sorted_op = aosl_rb_entry (rb_node, struct task_sorted_op, rb_node);

			aosl_rb_erase (&tsk->pending_async_ops, rb_node);
			tsk_invoke_f (sorted_op->f_name, &sorted_op->queued_ts, sorted_op->f, AOSL_FREE_ONLY_OBJ, aosl_task_act_free, sorted_op->op_seq, sorted_op->argc, sorted_op->argv, NULL);
			__free_task_sorted_op (sorted_op);
		}

		/**
		 * If this OPA type of task object has wait f mpq, then
		 * destroy and wait it here.
		 * -- Lionfore Hao Dec 15th, 2018
		 **/
		if (!aosl_mpq_invalid (tsk->wait_f_q))
			aosl_mpq_destroy_wait (tsk->wait_f_q);

		if (!aosl_mpq_timer_invalid (tsk->opa_timer))
			aosl_mpq_kill_timer (tsk->opa_timer);
	}

	for (;;) {
		if (__task_seq_q_remove_free_head (&tsk->pending_seqs) < 0)
			break;
	}

	while (tsk->done_backs.rb_node != NULL) {
		struct aosl_rb_node *rb_node = tsk->done_backs.rb_node;
		struct task_sorted_op *done_op = aosl_rb_entry (rb_node, struct task_sorted_op, rb_node);

		aosl_rb_erase (&tsk->done_backs, rb_node);
		tsk_invoke_f (done_op->f_name, &done_op->queued_ts, done_op->f, AOSL_FREE_ONLY_OBJ, aosl_task_act_free, done_op->op_seq, done_op->argc, done_op->argv, NULL);
		__free_task_sorted_op (done_op);
	}

	k_lock_destroy (&tsk->lock);

	/* Call the destructor of base class */
	refobj_type_obj.dtor (robj);
}

static struct refobj_type task_type_obj = {
	.obj_size = sizeof (struct task),
	.ctor = task_ctor,
	.dtor = task_dtor,
};

__export_in_so__ aosl_ref_t aosl_task_create (void *arg, aosl_ref_dtor_t dtor,
						aosl_task_type_t type, int serial, aosl_mpq_t st_op_q,
									int resume, aosl_task_res_wait_t wait_f)
{
	struct refobj *robj;
	int modify_async;

	switch (type) {
	case aosl_task_type_cpu:
	case aosl_task_type_gpu:
	case aosl_task_type_gen:
	case aosl_task_type_ltw:
		if (wait_f != NULL) {
			aosl_errno = AOSL_EINVAL;
			return -1;
		}
	case aosl_task_type_opa:
		break;
	default:
		aosl_errno = AOSL_EINVAL;
		return -1;
	}

	if (type == aosl_task_type_ltw) {
		/**
		 * Only the LTW type of task is modifying async,
		 * OPA type is not the same, because we would
		 * retrieve the ref object via ref id in the
		 * callback function of OPA task.
		 * -- Lionfore Hao Apr 11th, 2020
		 **/
		modify_async = 1;
	} else {
		modify_async = 0;
	}

	robj = refobj_create (&task_type_obj, arg, dtor, modify_async, 1/* recursive */, 1/* caller_free */, (int)type, serial, st_op_q, resume, wait_f);
	if (IS_ERR_OR_NULL (robj)) {
		aosl_errno = (int)-PTR_ERR (robj);
		return AOSL_REF_INVALID;
	}

	return robj->obj_id;
}

__export_in_so__ aosl_task_type_t aosl_task_get_type (aosl_ref_t task)
{
	struct task *tsk;
	aosl_task_type_t type;

	tsk = (struct task *)refobj_get (task);
	if (tsk == NULL) {
		aosl_errno = AOSL_EINVAL;
		return (aosl_task_type_t)-1;
	}

	type = task_type (tsk);
	refobj_put (&tsk->ref_obj);
	return type;
}

static void tsk_lock_if_need (struct task *tsk)
{
	aosl_mpq_t this_qid;

	/**
	 * For the OPA type of task object, we must hold the lock to
	 * do the task object operations if the owner mpq thread has
	 * been destroyed, and the done back operation was executed
	 * on the OPA thread.
	 * -- Lionfore Hao Jan 29th, 2019
	 * For the OPA type of task object, always lock it regardless
	 * whether this is a single thread op type task, because we
	 * will operate this task object in at least 3 threads even
	 * this is a single thread op type task.
	 * -- Lionfore Hao Apr 28th, 2020
	 **/
	if (task_type (tsk) == aosl_task_type_opa)
		goto __lock_it;

	if (!task_single_thread_op (tsk))
		goto __lock_it;

	this_qid = this_mpq_id ();
	if (!aosl_mpq_invalid (this_qid) && this_qid == tsk->st_op_q)
		return;

	/**
	 * We must hold the lock to do the task object operations
	 * for the cases satisfying the following 2 conditions:
	 * 1. the ower mpq thread has been destroyed, and the done
	 *    back operation was executed on the pool thread;
	 * 2. the task object is not serial type;
	 * -- Lionfore Hao Jan 29th, 2019
	 **/
	if (task_serial (tsk))
		return;

__lock_it:
	k_lock_lock (&tsk->lock);
}

static void tsk_unlock_if_need (struct task *tsk)
{
	aosl_mpq_t this_qid;

	/**
	 * For the OPA type of task object, we must hold the lock to
	 * do the task object operations if the owner mpq thread has
	 * been destroyed, and the done back operation was executed
	 * on the OPA thread.
	 * -- Lionfore Hao Jan 29th, 2019
	 * For the OPA type of task object, always lock it regardless
	 * whether this is a single thread op type task, because we
	 * will operate this task object in at least 3 threads even
	 * this is a single thread op type task.
	 * -- Lionfore Hao Apr 28th, 2020
	 **/
	if (task_type (tsk) == aosl_task_type_opa)
		goto __unlock_it;

	if (!task_single_thread_op (tsk))
		goto __unlock_it;

	this_qid = this_mpq_id ();
	if (!aosl_mpq_invalid (this_qid) && this_qid == tsk->st_op_q)
		return;

	/**
	 * We must hold the lock to do the task object operations
	 * for the cases satisfying the following 2 conditions:
	 * 1. the ower mpq thread has been destroyed, and the done
	 *    back operation was executed on the pool thread;
	 * 2. the task object is not serial type;
	 * -- Lionfore Hao Jan 29th, 2019
	 **/
	if (task_serial (tsk))
		return;

__unlock_it:
	k_lock_unlock (&tsk->lock);
}

static __inline__ int __task_stopped_state (struct task *tsk)
{
	return (int)(tsk->waiting_stop_ops.count > 0);
}

static __inline__ int __task_executing_state (struct task *tsk)
{
	if (task_type (tsk) == aosl_task_type_opa)
		return (int)(tsk->pending_async_ops.count > 0);

	return (int)(tsk->pending_seqs.count > 0);
}

static __inline__ void ____task_do_all_waiting_stop_ops (struct task *tsk)
{
	struct task_stop_op *stop_op;
	while ((stop_op = __task_stop_op_q_remove_head (&tsk->waiting_stop_ops)) != NULL) {
		stop_op->stop_f (tsk, aosl_task_act_done, 0, stop_op->argc, stop_op->argv);
		__free_task_stop_op (stop_op);
	}
}

static __inline__ struct task_op *__task_waiting_ops_remove_head (struct task *tsk, int force)
{
	if (!force && __task_stopped_state (tsk))
		return NULL;

	return __task_op_q_remove_head (&tsk->waiting_ops);
}

static void __task_do_done_backs_orderly (struct task *tsk, uintptr_t op_seq, struct task_sorted_op *sorted_op, ...)
{
	uintptr_t head_seq;
	va_list args;
	const char *f_name;
	const aosl_ts_t *queued_ts_p;
	aosl_task_func_t f;
	uintptr_t argc;
	uintptr_t *argv;
	struct resume_calls *resume_calls;
	struct mp_queue *this_q;

	if (__task_seq_q_head_seq (&tsk->pending_seqs, &head_seq) < 0)
		abort ();

	if (aosl_uintptr_before (op_seq, head_seq))
		abort ();

	if (aosl_uintptr_after (op_seq, head_seq)) {
		if (sorted_op == NULL) {
			uintptr_t l;
			uintptr_t argv_size;

			va_start (args, sorted_op);
			f_name = va_arg (args, const char *);
			queued_ts_p = va_arg (args, const aosl_ts_t *);
			f = va_arg (args, aosl_task_func_t);
			argc = va_arg (args, uintptr_t);
			argv = va_arg (args, uintptr_t *);
			resume_calls = va_arg (args, struct resume_calls *);
			va_end (args);

			argv_size = (sizeof (uintptr_t) * argc);
			sorted_op = (struct task_sorted_op *)aosl_malloc (sizeof (struct task_sorted_op) + argv_size);
			if (sorted_op == NULL)
				abort ();

			sorted_op->op_seq = op_seq;
			sorted_op->f_name = aosl_strdup (f_name);
			sorted_op->resume_calls = resume_calls;
			sorted_op->queued_ts = *queued_ts_p;
			sorted_op->f = f;
			sorted_op->argc = argc;
			for (l = 0; l < argc; l++)
				sorted_op->argv [l] = argv [l];
		}

		aosl_rb_insert_node (&tsk->done_backs, &sorted_op->rb_node);
		return;
	}

	if (sorted_op != NULL) {
		f_name = sorted_op->f_name;
		resume_calls = sorted_op->resume_calls;
		queued_ts_p = &sorted_op->queued_ts;
		f = sorted_op->f;
		argc = sorted_op->argc;
		argv = sorted_op->argv;
	} else {
		va_start (args, sorted_op);
		f_name = va_arg (args, const char *);
		queued_ts_p = va_arg (args, const aosl_ts_t *);
		f = va_arg (args, aosl_task_func_t);
		argc = va_arg (args, uintptr_t);
		argv = va_arg (args, uintptr_t *);
		resume_calls = va_arg (args, struct resume_calls *);
		va_end (args);
	}

	tsk_unlock_if_need (tsk);

	this_q = THIS_MPQ ();
	tsk_invoke_f (f_name, queued_ts_p, f, (aosl_refobj_t)&tsk->ref_obj, aosl_task_act_done, op_seq, argc, argv, resume_calls);
	mpq_stack_fini (this_q->q_stack_curr);

	if (sorted_op != NULL) {
		/**
		 * We have processed the resume calls and may freed it
		 * when do the done action, so set this to NULL here
		 * to indicate no need to do the free.
		 * -- Lionfore Hao Oct 23rd, 2020
		 **/
		sorted_op->resume_calls = NULL;
		__free_task_sorted_op (sorted_op);
	}

	tsk_lock_if_need (tsk);
	/**
	 * We MUST guarantee removing the head seq after invoked
	 * the callback function, DO NOT do this before previous
	 * unlock, otherwise we could not guarantee the order of
	 * done back when using in the none single thread cases.
	 * -- Lionfore Hao Apr 17th, 2020
	 **/
	__task_seq_q_remove_free_head (&tsk->pending_seqs);

	for (;;) {
		struct aosl_rb_node *rb_node = aosl_rb_first (&tsk->done_backs);
		if (rb_node == NULL)
			break;

		sorted_op = aosl_rb_entry (rb_node, struct task_sorted_op, rb_node);

		if (__task_seq_q_head_seq (&tsk->pending_seqs, &head_seq) < 0)
			abort ();

		if (aosl_uintptr_before (sorted_op->op_seq, head_seq))
			abort ();

		if (aosl_uintptr_after (sorted_op->op_seq, head_seq))
			break;

		aosl_rb_erase (&tsk->done_backs, rb_node);
		tsk_unlock_if_need (tsk);

		tsk_invoke_f (sorted_op->f_name, &sorted_op->queued_ts, sorted_op->f, (aosl_refobj_t)&tsk->ref_obj, aosl_task_act_done, sorted_op->op_seq, sorted_op->argc, sorted_op->argv, sorted_op->resume_calls);
		mpq_stack_fini (this_q->q_stack_curr);

		/**
		 * We have processed the resume calls and may freed it
		 * when do the done action, so set this to NULL here
		 * to indicate no need to do the free.
		 * -- Lionfore Hao Oct 23rd, 2020
		 **/
		sorted_op->resume_calls = NULL;
		__free_task_sorted_op (sorted_op);

		tsk_lock_if_need (tsk);
		/**
		 * We MUST guarantee removing the head seq after invoked
		 * the callback function, DO NOT do this before previous
		 * unlock, otherwise we could not guarantee the order of
		 * done back when using in the none single thread cases.
		 * -- Lionfore Hao Apr 17th, 2020
		 **/
		__task_seq_q_remove_free_head (&tsk->pending_seqs);
	}
}

static int ____task_exec_argv (struct task *tsk, const char *f_name, aosl_task_func_t f, uintptr_t argc, uintptr_t *argv,
								struct list_head *prepare_calls, struct resume_calls *resume_calls, const aosl_ts_t *ts_p);

static __inline__ void __task_sync_op_waiting_ops_head_exec (struct task *tsk)
{
	struct task_op *aop = __task_waiting_ops_remove_head (tsk, 0/* !force */);
	if (aop != NULL) {
		if (____task_exec_argv (tsk, aop->f_name, aop->f, aop->argc, aop->argv, aop->prepare_calls, aop->resume_calls, &aop->queued_ts) < 0) {
			/**
			 * Failed to do the exec, so just do free action here.
			 * -- Lionfore Hao Apr 6th, 2020
			 **/
			tsk_invoke_f (aop->f_name, &aop->queued_ts, aop->f, (aosl_refobj_t)&tsk->ref_obj, aosl_task_act_free, 0, aop->argc, aop->argv, NULL);
		} else {
			/**
			 * The aop->resume_calls will be used, so do not free it
			 * when freeing task_op object.
			 * -- Lionfore Hao Sep 6th, 2020
			 **/
			aop->resume_calls = NULL;
		}

		aop->prepare_calls = NULL;
		__free_task_op (aop);
	}
}

static int tsk_done_locked (struct task *tsk, const char *f_name, aosl_mpq_func_argv_t done_f, uintptr_t argc, uintptr_t *argv)
{
	aosl_mpq_t done_q;
	int err;

	if (task_single_thread_op (tsk)) {
		done_q = tsk->st_op_q;
	} else {
		done_q = tsk->curr_done_q;
	}

	if (aosl_mpq_invalid (done_q)) {
		err = -AOSL_EINVAL;
		if (!task_single_thread_op (tsk)) {
			done_q = genp_queue_no_fail_argv (AOSL_MPQ_INVALID, task_ref_id (tsk), f_name, done_f, argc, argv);
			if (!aosl_mpq_invalid (done_q)) {
				tsk->curr_done_q = done_q;
				tsk->curr_done_q_usage++;
				err = 0;
			} else {
				err = -aosl_errno;
			}
		}
	} else {
		err = mpq_queue_no_fail_argv (done_q, AOSL_MPQ_INVALID, task_ref_id (tsk), f_name, done_f, argc, argv);
		if (err < 0) {
			err = -aosl_errno;
		} else if (!task_single_thread_op (tsk)) {
			tsk->curr_done_q_usage++;
		}
	}

	return err;
}

static void __task_sync_op_do_or_done (const aosl_ts_t *queued_ts_p, aosl_refobj_t robj, uintptr_t argc, uintptr_t *argv)
{
	struct task *tsk = (struct task *)robj;
	const char *f_name = (const char *)argv [1];
	aosl_task_func_t f = (aosl_task_func_t)argv [2];
	uintptr_t op_seq = argv [3];
	struct resume_calls *resume_calls = (struct resume_calls *)argv [4];

	if (!aosl_is_free_only (robj)) {
		if (argv [0] == 0) {
			/**
			 * For modify async ref object, do NOT read lock it when
			 * doing the exec operation, because the exec might be a
			 * long time wait operation.
			 * No need to read lock for non modify async ref object
			 * due to mpq would do this automatically.
			 * -- Lionfore Hao Dec 29th, 2020
			 **/
			tsk_invoke_f (f_name, queued_ts_p, f, (aosl_refobj_t)&tsk->ref_obj, aosl_task_act_exec, op_seq, argc - 5, &argv [5], NULL);
			argv [0] = 1;
			if (!task_single_thread_op (tsk)) {
				if (task_type (tsk) == aosl_task_type_gen /*&& task_serial (tsk)*/) {
					/**
					 * For the GEN & serial type of tasks, we can done it
					 * directly here, but for other cases we must not do
					 * it in this manner:
					 * 1. for CPU/LTW type, we need to done it in GEN pool;
					 * 2. for non serial type, if we done it directly here,
					 *    then we must hold the tsk lock even invoking the
					 *    callback function, this might lead to dead lock;
					 * -- Lionfore Hao May 21st, 2019
					 * We moved the head removing operation of pending_seqs
					 * back to after the invoking of 'f', no need to hold
					 * the tsk lock now.
					 * -- Lionfore Hao May 22nd, 2019
					 **/
					goto __done_directly;
				}

				tsk_lock_if_need (tsk);
				tsk_done_locked (tsk, "__task_sync_op_do_or_done", __task_sync_op_do_or_done, argc, argv);
				tsk_unlock_if_need (tsk);
			}
		} else {
__done_directly:
			/**
			 * For modify async ref object, we will read lock it when
			 * done back. No need to read lock for non modify async
			 * ref object due to mpq would do this automatically.
			 * -- Lionfore Hao Dec 29th, 2020
			 **/
			if (refobj_is_modify_async (&tsk->ref_obj)) {
				if (refobj_rdlock (&tsk->ref_obj) < 0)
					goto __free_only;
			}

			tsk_lock_if_need (tsk);

			__task_do_done_backs_orderly (tsk, op_seq, NULL, f_name, queued_ts_p, f, argc - 5, &argv [5], resume_calls);

			if (!task_single_thread_op (tsk)) {
				/**
				 * For the GEN & serial type of tasks, we done directly
				 * to here, in this case, the current done mpq will be
				 * invalid, and for all other cases, the done mpq must
				 * be equal to this mpq.
				 * -- Lionfore Hao May 21st, 2019
				 **/
				if (tsk->curr_done_q == this_mpq_id ()) {
					tsk->curr_done_q_usage--;
					if (tsk->curr_done_q_usage == 0)
						tsk->curr_done_q = AOSL_MPQ_INVALID;
				}
			}

			if (tsk->pending_seqs.count == 0)
				____task_do_all_waiting_stop_ops (tsk);

			__task_sync_op_waiting_ops_head_exec (tsk);

			tsk_unlock_if_need (tsk);
			if (refobj_is_modify_async (&tsk->ref_obj))
				refobj_rdunlock (&tsk->ref_obj);

			if (f_name != NULL)
				aosl_free ((void *)f_name);
		}

		return;
	}

__free_only:
	tsk_invoke_f (f_name, queued_ts_p, f, AOSL_FREE_ONLY_OBJ, aosl_task_act_free, op_seq, argc - 5, &argv [5], NULL);
	if (f_name != NULL)
		aosl_free ((void *)f_name);

	if (resume_calls != NULL)
		resume_calls_task_free (resume_calls);
}

static int ____task_sync_op_exec_argv (struct task *tsk, const char *f_name, aosl_task_func_t f, uintptr_t argc, uintptr_t *argv, struct resume_calls *resume_calls)
{
	aosl_ref_t ref_obj_id;
	uintptr_t *local_argv;
	uintptr_t l;
	aosl_mpq_t dq;
	aosl_mpq_t qid;

	ref_obj_id = task_ref_id (tsk);
	local_argv = alloca (sizeof (uintptr_t) * (5 + argc));
	local_argv [0] = 0; /* indicates whether the target function has been executed */
	local_argv [1] = (uintptr_t)aosl_strdup (f_name);
	local_argv [2] = (uintptr_t)f;
	local_argv [3] = (uintptr_t)tsk->task_op_seq;
	local_argv [4] = (uintptr_t)resume_calls;
	for (l = 0; l < argc; l++)
		local_argv [5 + l] = argv [l];

	if (task_single_thread_op (tsk)) {
		dq = tsk->st_op_q;
	} else {
		dq = AOSL_MPQ_INVALID;
	}

	switch (task_type (tsk)) {
	case aosl_task_type_cpu:
		qid = aosl_mpqp_queue_argv (aosl_cpup (), dq, ref_obj_id, "__task_sync_op_do_or_done", __task_sync_op_do_or_done, 5 + argc, local_argv);
		break;
	case aosl_task_type_gpu:
		qid = aosl_mpqp_queue_argv (aosl_gpup (), dq, ref_obj_id, "__task_sync_op_do_or_done", __task_sync_op_do_or_done, 5 + argc, local_argv);
		break;
	case aosl_task_type_gen:
		qid = aosl_mpqp_queue_argv (aosl_genp (), dq, ref_obj_id, "__task_sync_op_do_or_done", __task_sync_op_do_or_done, 5 + argc, local_argv);
		break;
	case aosl_task_type_ltw:
		qid = aosl_mpqp_queue_argv (aosl_ltwp (), dq, ref_obj_id, "__task_sync_op_do_or_done", __task_sync_op_do_or_done, 5 + argc, local_argv);
		break;
	default:
		abort ();
		aosl_errno = AOSL_EINVAL;
		return -1;
	}

	if (aosl_mpq_invalid (qid))
		return -1;

	__task_seq_q_add (&tsk->pending_seqs, tsk->task_op_seq);
	__task_inc_op_seq (tsk);
	return 0;
}

static void ____task_async_op_wait_f (const aosl_ts_t *queued_ts_p, aosl_refobj_t robj, uintptr_t argc, uintptr_t argv [])
{
	if (!aosl_is_free_only (robj)) {
		aosl_task_res_wait_t wait_f = (aosl_task_res_wait_t)argv [0];
		uintptr_t op_seq = (uintptr_t)argv [1];
		wait_f ((aosl_refobj_t)robj, op_seq, argc - 2, &argv [2]);
	}
}

static void __task_async_op_do (const aosl_ts_t *queued_ts_p, aosl_refobj_t robj, uintptr_t argc, uintptr_t *argv)
{
	struct task *tsk = (struct task *)robj;
	struct task_sorted_op *aop = (struct task_sorted_op *)argv [0];

	if (!aosl_is_free_only (robj)) {
		if (refobj_rdlock (&tsk->ref_obj) < 0)
			return;

		tsk_invoke_f (aop->f_name, queued_ts_p, aop->f, (aosl_refobj_t)&tsk->ref_obj, aosl_task_act_exec, aop->op_seq, aop->argc, aop->argv, NULL);

		if (tsk->wait_f != NULL) {
			uintptr_t *local_argv;
			uintptr_t l;

			local_argv = alloca (sizeof (uintptr_t) * (2 + argc));
			local_argv [0] = (uintptr_t)tsk->wait_f;
			local_argv [1] = (uintptr_t)aop->op_seq;
			for (l = 0; l < aop->argc; l++)
				local_argv [2 + l] = aop->argv [l];

			aosl_mpq_queue_argv (tsk->wait_f_q, AOSL_MPQ_INVALID, task_ref_id (tsk), "____task_async_op_wait_f", ____task_async_op_wait_f, 2 + aop->argc, local_argv);
		}

		tsk_lock_if_need (tsk);
		tsk->curr_opa_op_q_usage--;
		if (tsk->curr_opa_op_q_usage == 0)
			tsk->curr_opa_op_q = AOSL_MPQ_INVALID;
		tsk_unlock_if_need (tsk);
		refobj_rdunlock (&tsk->ref_obj);
		return;
	}

	/**
	 * MUST NOT do the free only operation in these cases, because the aop
	 * had already been inserted into the pending_async_ops, and would be
	 * freed when destroying the tsk object, so if we do this operation
	 * again here, then would lead to potential race condition double free.
	 * -- Lionfore Hao Apr 16th, 2020
	 * tsk_invoke_f (aop->f_name, queued_ts_p, aop->f, AOSL_FREE_ONLY_OBJ, aosl_task_act_free, aop->op_seq, aop->argc, aop->argv, NULL);
	 **/
}

static __inline__ int ____task_async_op_exec_argv (struct task *tsk, const char *f_name, aosl_task_func_t f, uintptr_t argc, uintptr_t argv [], struct resume_calls *resume_calls)
{
	aosl_ref_t ref_obj_id = task_ref_id (tsk);
	uintptr_t l;
	uintptr_t argv_size = (sizeof (uintptr_t) * argc);
	struct task_sorted_op *aop;
	aosl_mpq_t tq;
	int need_put_best_q = 0;
	int err;

	aop = (struct task_sorted_op *)aosl_malloc (sizeof (struct task_sorted_op) + argv_size);
	if (aop == NULL) {
		abort ();
		return -1;
	}

	aop->op_seq = tsk->task_op_seq;
	aop->f_name = aosl_strdup (f_name);
	aop->resume_calls = resume_calls;
	aop->queued_ts = aosl_tick_now ();
	aop->f = f;
	aop->argc = argc;
	for (l = 0; l < argc; l++)
		aop->argv [l] = argv [l];

	tq = tsk->curr_opa_op_q;
	if (aosl_mpq_invalid (tq)) {
		tq = genp_best_q_get ();
		if (aosl_mpq_invalid (tq))
			abort ();

		tsk->curr_opa_op_q = tq;
		need_put_best_q = 1;
	}
	tsk->curr_opa_op_q_usage++;

	err = aosl_mpq_queue (tq, AOSL_MPQ_INVALID, ref_obj_id, "__task_async_op_do", __task_async_op_do, 1, aop);

	if (need_put_best_q)
		mpqp_best_q_put (tq);

	if (err < 0) {
		err = -aosl_errno;
		if (err == 0)
			err = -AOSL_EINVAL;

		tsk->curr_opa_op_q_usage--;
		if (tsk->curr_opa_op_q_usage == 0)
			tsk->curr_opa_op_q = AOSL_MPQ_INVALID;

		__free_task_sorted_op (aop);
	} else {
		aosl_rb_insert_node (&tsk->pending_async_ops, &aop->rb_node);

		if (task_serial (tsk)) {
			/**
			 * For OPA cases, pending seqs only used for sorting
			 * the async done backs, no other purpose.
			 * -- Lionfore Hao Apr 17th, 2020
			 **/
			__task_seq_q_add (&tsk->pending_seqs, aop->op_seq);
		}

		__task_inc_op_seq (tsk);
	}

	return err;
}

static __inline__ void __task_async_op_waiting_ops_head_exec (struct task *tsk, int force)
{
	struct task_op *aop = __task_waiting_ops_remove_head (tsk, force);
	if (aop != NULL) {
		if (____task_exec_argv (tsk, aop->f_name, aop->f, aop->argc, aop->argv, aop->prepare_calls, aop->resume_calls, &aop->queued_ts) < 0) {
			/**
			 * Failed to do the exec, so just do free action here.
			 * -- Lionfore Hao Apr 6th, 2020
			 **/
			tsk_invoke_f (aop->f_name, &aop->queued_ts, aop->f, (aosl_refobj_t)&tsk->ref_obj, aosl_task_act_free, 0, aop->argc, aop->argv, NULL);
		} else {
			/**
			 * The aop->resume_calls will be used, so do not free it
			 * when freeing task_op object.
			 * -- Lionfore Hao Sep 6th, 2020
			 **/
			aop->resume_calls = NULL;
		}

		aop->prepare_calls = NULL;
		__free_task_op (aop);
	}
}

static int ____task_exec_argv (struct task *tsk, const char *f_name, aosl_task_func_t f, uintptr_t argc, uintptr_t *argv,
								struct list_head *prepare_calls, struct resume_calls *resume_calls, const aosl_ts_t *ts_p)
{
	if (prepare_calls != NULL) {
		aosl_stack_id_t err_stack = do_prepare_calls (prepare_calls, ts_p);
		if (!aosl_stack_invalid (err_stack))
			return -AOSL_ECONNABORTED;
	}

	if (task_type (tsk) != aosl_task_type_opa)
		return ____task_sync_op_exec_argv (tsk, f_name, f, argc, argv, resume_calls);

	return ____task_async_op_exec_argv (tsk, f_name, f, argc, argv, resume_calls);
}

static __inline__ struct resume_calls *mpq_stack_resume_calls (struct mp_queue *q, struct mpq_stack *stk)
{
	struct resume_calls *resume_calls = stk->resume_calls;
	if (resume_calls == NULL) {
		resume_calls = (struct resume_calls *)aosl_malloc (sizeof *resume_calls);
		if (resume_calls == NULL)
			return NULL;

		atomic_set (&resume_calls->usage, 1);
		resume_calls->task_count = 0;
		INIT_LIST_HEAD (&resume_calls->list);
		stk->resume_calls = resume_calls;
	}

	return resume_calls;
}

static int __task_exec_argv (aosl_ref_t task, const char *f_name, aosl_task_func_t f, uintptr_t argc, uintptr_t *argv)
{
	int need_waiting = 0;
	struct mp_queue *this_q;
	struct task *tsk;
	struct mpq_stack *curr_stack;
	struct list_head *prepare_calls = NULL;
	struct resume_calls *resume_calls = NULL;
	int err;

	this_q = THIS_MPQ ();
	/**
	 * 1. Do not allow non aosl thread exec task;
	 * 2. If the aosl mpq is exiting now, do not allow it too;
	 * -- Lionfore Hao Dec 18th, 2020
	 **/
	if (this_q == NULL || this_q->exiting) {
		aosl_errno = AOSL_EPERM;
		return -1;
	}

	curr_stack = this_q->q_stack_curr;

	if (curr_stack->prepare_calls_count > 0 && curr_stack->task_exec_count > 0) {
		abort ();
		aosl_errno = AOSL_EPERM;
		return -1;
	}

	tsk = (struct task *)refobj_get (task);
	if (tsk == NULL) {
		aosl_errno = AOSL_EINVAL;
		return -1;
	}

	if (task_single_thread_op (tsk) && this_mpq_id () != tsk->st_op_q) {
		abort ();
		err = -AOSL_EINVAL;
		goto __put_refobj;
	}

	if (!task_single_thread_op (tsk)) {
		/**
		 * We need to lock the task ref object if the task is not
		 * single thread op type, because other thread may destroy
		 * it while we are using it.
		 * -- Lionfore Hao May 3rd, 2019
		 **/
		if (refobj_rdlock (&tsk->ref_obj) < 0) {
			err = -AOSL_EINVAL;
			goto __put_refobj;
		}
	}

	if (task_support_resume (tsk)) {
		resume_calls = mpq_stack_resume_calls (this_q, curr_stack);
		if (resume_calls == NULL) {
			err = -AOSL_ENOMEM;
			goto __refobj_unlock;
		}
	}

	tsk_lock_if_need (tsk);

	if (!__task_stopped_state (tsk)) {
		if (task_serial (tsk)) {
			if (task_type (tsk) != aosl_task_type_opa) {
				need_waiting = (int)(tsk->pending_seqs.count > 0);
			} else {
				/**
				 * For the OPA case, the async operation issuing action is executed
				 * serially in the owner thread, so no serial waiting needed, just
				 * issue the async operation when requested, so comment this line:
				 * need_waiting = (int)(tsk->pending_async_ops.count > 0);
				 * -- Lionfore Hao Dec 1st, 2018
				 **/
			}
		}
	}

	if (task_support_resume (tsk)) {
		prepare_calls = curr_stack->prepare_calls;
		curr_stack->prepare_calls = NULL;
		resume_calls_task_exec (resume_calls);
	}

	if (__task_stopped_state (tsk) || need_waiting) {
		__task_op_wait_add (tsk, f_name, f, argc, argv, prepare_calls, resume_calls);
		err = 0;
	} else {
		err = ____task_exec_argv (tsk, f_name, f, argc, argv, prepare_calls, resume_calls, NULL);
	}

	tsk_unlock_if_need (tsk);

	if (task_support_resume (tsk)) {
		curr_stack->task_exec_err = err;

		if (err < 0) {
			resume_calls_task_free (resume_calls);
		} else {
			curr_stack->task_exec_count++;
		}
	}

__refobj_unlock:
	if (!task_single_thread_op (tsk))
		refobj_rdunlock (&tsk->ref_obj);

__put_refobj:
	refobj_put (&tsk->ref_obj);
	return_err (err);
}

static int __task_exec_args (aosl_ref_t task, const char *f_name, aosl_task_func_t f, uintptr_t argc, va_list args)
{
	uintptr_t *argv = NULL;

	if (argc > 0) {
		uintptr_t l;

		argv = alloca (sizeof (uintptr_t) * argc);
		for (l = 0; l < argc; l++)
			argv [l] = va_arg (args, uintptr_t);
	}

	return __task_exec_argv (task, f_name, f, argc, argv);
}

__export_in_so__ int aosl_task_exec (aosl_ref_t task, const char *f_name, aosl_task_func_t f, uintptr_t argc, ...)
{
	va_list args;
	int err;

	va_start (args, argc);
	err = __task_exec_args (task, f_name, f, argc, args);
	va_end (args);

	return err;
}

__export_in_so__ int aosl_task_exec_args (aosl_ref_t task, const char *f_name, aosl_task_func_t f, uintptr_t argc, va_list args)
{
	return __task_exec_args (task, f_name, f, argc, args);
}

__export_in_so__ int aosl_task_exec_argv (aosl_ref_t task, const char *f_name, aosl_task_func_t f, uintptr_t argc, uintptr_t argv [])
{
	return __task_exec_argv (task, f_name, f, argc, argv);
}

static void __opa_ops_monitor (aosl_timer_t timer_id, const aosl_ts_t *now_p, uintptr_t argc, uintptr_t argv [])
{
	aosl_ref_t task_id = (aosl_ref_t)argv [0];
	struct task *tsk;

	tsk = (struct task *)refobj_get (task_id);
	if (tsk != NULL) {
		if (refobj_rdlock (&tsk->ref_obj) < 0)
			goto __put_refobj;

		/**
		 * For the OPA type of tasks, there might be a case that
		 * not one async operation with one done callback, such
		 * as decoding a video stream with B frames, so we use
		 * this monitor timer to detect these senarios, continue
		 * issuing async operations periodically, until we got
		 * all the done backs of the issued async operations.
		 * -- Lionfore Hao May 1st, 2020
		 **/
		tsk_lock_if_need (tsk);
		__task_async_op_waiting_ops_head_exec (tsk, 1/* force */);
		tsk_unlock_if_need (tsk);
		refobj_rdunlock (&tsk->ref_obj);

__put_refobj:
		refobj_put (&tsk->ref_obj);
	}
}

static void ____task_async_op_done_locked (struct task *tsk, uintptr_t aop_seq, const aosl_ts_t *queued_ts_p)
{
	struct aosl_rb_node *rb_node;

	if (aop_seq == 0) {
		rb_node = aosl_rb_first (&tsk->pending_async_ops);
	} else {
		rb_node = aosl_find_rb_node (&tsk->pending_async_ops, NULL, aop_seq);
	}

	if (rb_node != NULL) {
		struct task_sorted_op *aop = aosl_rb_entry (rb_node, struct task_sorted_op, rb_node);

		aosl_rb_erase (&tsk->pending_async_ops, rb_node);

		if (!task_serial (tsk)) {
			struct mp_queue *this_q = THIS_MPQ ();

			tsk_unlock_if_need (tsk);
			tsk_invoke_f (aop->f_name, queued_ts_p, aop->f, (aosl_refobj_t)&tsk->ref_obj, aosl_task_act_done, aop->op_seq, aop->argc, aop->argv, aop->resume_calls);
			mpq_stack_fini (this_q->q_stack_curr);

			/**
			 * We have processed the resume calls and may freed it
			 * when do the done action, so set this to NULL here
			 * to indicate no need to do the free.
			 * -- Lionfore Hao Oct 23rd, 2020
			 **/
			aop->resume_calls = NULL;
			__free_task_sorted_op (aop);
			tsk_lock_if_need (tsk);
		} else {
			aop->queued_ts = *queued_ts_p;
			__task_do_done_backs_orderly (tsk, aop->op_seq, aop);
		}

		if (!task_single_thread_op (tsk)) {
			BUG_ON (tsk->curr_done_q != this_mpq_id ());
			tsk->curr_done_q_usage--;
			if (tsk->curr_done_q_usage == 0)
				tsk->curr_done_q = AOSL_MPQ_INVALID;
		}

		if (tsk->pending_async_ops.count == 0) {
			if (!aosl_mpq_timer_invalid (tsk->opa_timer)) {
				aosl_mpq_kill_timer (tsk->opa_timer);
				tsk->opa_timer = AOSL_MPQ_TIMER_INVALID;
			}

			____task_do_all_waiting_stop_ops (tsk);
		}

		__task_async_op_waiting_ops_head_exec (tsk, 0/* !force */);
	}
}

static void ____task_async_op_done (struct task *tsk, uintptr_t aop_seq, const aosl_ts_t *queued_ts_p)
{
	if (refobj_rdlock (&tsk->ref_obj) < 0)
		return;

	tsk_lock_if_need (tsk);
	____task_async_op_done_locked (tsk, aop_seq, queued_ts_p);
	tsk_unlock_if_need (tsk);
	refobj_rdunlock (&tsk->ref_obj);
}

static void __task_async_op_done (const aosl_ts_t *queued_ts_p, aosl_refobj_t robj, uintptr_t argc, uintptr_t argv [])
{
	if (!aosl_is_free_only (robj))
		____task_async_op_done ((struct task *)robj, argv [0] /* aop_seq */, queued_ts_p);
}

static int task_async_done (aosl_ref_t task, uintptr_t opaque)
{
	struct task *tsk;
	int err;

	tsk = (struct task *)refobj_get (task);
	if (tsk == NULL) {
		aosl_errno = AOSL_EINVAL;
		return -1;
	}

	if (task_type (tsk) != aosl_task_type_opa) {
		err = -AOSL_EINVAL;
		goto __put_refobj;
	}

	/**
	 * We need to lock the task ref object if the task is not
	 * single thread op type, because other thread may destroy
	 * it while we are using it.
	 * -- Lionfore Hao May 3rd, 2019
	 * We need to lock the task ref object even if the task is
	 * single thread op type, because this function is invoked
	 * in another thread, rather than the dedicated st op q.
	 * -- Lionfore Hao Apr 28th, 2020
	 **/
	if (refobj_rdlock (&tsk->ref_obj) < 0) {
		err = -AOSL_EINVAL;
		goto __put_refobj;
	}

	tsk_lock_if_need (tsk);
	err = tsk_done_locked (tsk, "__task_async_op_done", __task_async_op_done, 1, &opaque);
	tsk_unlock_if_need (tsk);
	refobj_rdunlock (&tsk->ref_obj);

__put_refobj:
	refobj_put (&tsk->ref_obj);
	return_err (err);
}

__export_in_so__ int aosl_task_async_done (aosl_ref_t task)
{
	return task_async_done (task, 0);
}

__export_in_so__ int aosl_task_async_done_opaque (aosl_ref_t task, uintptr_t opaque)
{
	return task_async_done (task, opaque);
}

static int ____task_stop_exec_argv (struct task *tsk, aosl_task_func_t stop_f, uintptr_t argc, uintptr_t *argv)
{
	if (__task_executing_state (tsk)) {
		__task_stop_op_q_add (&tsk->waiting_stop_ops, stop_f, argc, argv);
	} else {
		stop_f (tsk, aosl_task_act_done, 0, argc, argv);
	}

	return 0;
}

static int __task_stop_exec_argv (aosl_ref_t task, aosl_task_func_t stop_f, uintptr_t argc, uintptr_t argv [])
{
	struct task *tsk;
	int err;

	tsk = (struct task *)refobj_get ((aosl_ref_t)task);
	if (tsk == NULL) {
		aosl_errno = AOSL_EINVAL;
		return -1;
	}

	if (task_single_thread_op (tsk) && this_mpq_id () != tsk->st_op_q) {
		abort ();
		err = -AOSL_EINVAL;
		goto __put_refobj;
	}

	if (!task_single_thread_op (tsk)) {
		/**
		 * We need to lock the task ref object if the task is not
		 * single thread op type, because other thread may destroy
		 * it while we are using it.
		 * -- Lionfore Hao May 3rd, 2019
		 **/
		if (refobj_rdlock (&tsk->ref_obj) < 0) {
			err = -AOSL_EINVAL;
			goto __put_refobj;
		}
	}

	tsk_lock_if_need (tsk);

	if (__task_stopped_state (tsk)) {
		__task_stop_op_q_add (&tsk->waiting_stop_ops, stop_f, argc, argv);
		err = 0;
	} else {
		err = ____task_stop_exec_argv (tsk, stop_f, argc, argv);
	}

	tsk_unlock_if_need (tsk);

	if (!task_single_thread_op (tsk))
		refobj_rdunlock (&tsk->ref_obj);

__put_refobj:
	refobj_put (&tsk->ref_obj);
	return_err (err);
}

static int __task_stop_exec_args (aosl_ref_t task, aosl_task_func_t stop_f, uintptr_t argc, va_list args)
{
	uintptr_t *argv = NULL;

	if (argc > 0) {
		uintptr_t l;

		argv = alloca (sizeof (uintptr_t) * argc);
		for (l = 0; l < argc; l++)
			argv [l] = va_arg (args, uintptr_t);
	}

	return __task_stop_exec_argv (task, stop_f, argc, argv);
}

__export_in_so__ int aosl_task_exclusive_exec (aosl_ref_t task, aosl_task_func_t exclusive_f, uintptr_t argc, ...)
{
	va_list args;
	int err;

	va_start (args, argc);
	err = __task_stop_exec_args (task, exclusive_f, argc, args);
	va_end (args);

	return err;
}

__export_in_so__ int aosl_task_exclusive_exec_args (aosl_ref_t task, aosl_task_func_t exclusive_f, uintptr_t argc, va_list args)
{
	return __task_stop_exec_args (task, exclusive_f, argc, args);
}

__export_in_so__ int aosl_task_exclusive_exec_argv (aosl_ref_t task, aosl_task_func_t exclusive_f, uintptr_t argc, uintptr_t argv [])
{
	return __task_stop_exec_argv (task, exclusive_f, argc, argv);
}

__export_in_so__ int aosl_task_waiting_ops_count (aosl_ref_t task)
{
	struct task *tsk;
	int count;

	tsk = (struct task *)refobj_get (task);
	if (tsk == NULL) {
		aosl_errno = AOSL_EINVAL;
		return -1;
	}

	count = (int)tsk->waiting_ops.count;
	refobj_put (&tsk->ref_obj);
	return count;
}

__export_in_so__ int aosl_task_remove_waiting_ops_head (aosl_ref_t task)
{
	struct task *tsk;
	struct task_op *aop;
	int err = -AOSL_ENOENT;

	tsk = (struct task *)refobj_get (task);
	if (tsk == NULL) {
		aosl_errno = AOSL_EINVAL;
		return -1;
	}

	if (task_single_thread_op (tsk) && this_mpq_id () != tsk->st_op_q) {
		abort ();
		err = -AOSL_EINVAL;
		goto __put_refobj;
	}

	if (!task_single_thread_op (tsk)) {
		/**
		 * We need to lock the task ref object if the task is not
		 * single thread op type, because other thread may destroy
		 * it while we are using it.
		 * -- Lionfore Hao May 3rd, 2019
		 **/
		if (refobj_rdlock (&tsk->ref_obj) < 0) {
			err = -AOSL_EINVAL;
			goto __put_refobj;
		}
	}

	tsk_lock_if_need (tsk);
	aop = __task_op_q_remove_head (&tsk->waiting_ops);
	tsk_unlock_if_need (tsk);

	if (!task_single_thread_op (tsk))
		refobj_rdunlock (&tsk->ref_obj);

	if (aop != NULL) {
		/**
		 * Passing tsk rather than free only object here, because
		 * we still hold the reference of this tsk object. This is
		 * the only case that we just do freeing action when task
		 * is still valid.
		 * -- Lionfore Hao Jul 31st, 2019
		 **/
		tsk_invoke_f (aop->f_name, &aop->queued_ts, aop->f, (aosl_refobj_t)&tsk->ref_obj, aosl_task_act_free, 0, aop->argc, aop->argv, NULL);
		__free_task_op (aop);
		err = 0;
	}

__put_refobj:
	refobj_put (&tsk->ref_obj);
	return_err (err);
}

static int __task_prepare_argv (aosl_stack_id_t stack_id, const char *f_name, aosl_prepare_func_t f, uintptr_t argc, uintptr_t argv [])
{
	struct mp_queue *this_q = THIS_MPQ ();
	struct prepare_node *node;
	struct mpq_stack *curr_stack;
	uintptr_t l;

	if (this_q == NULL)
		return -AOSL_EPERM;

	curr_stack = this_q->q_stack_curr;
	if (curr_stack->prepare_calls == NULL) {
		struct list_head *prepare_calls = (struct list_head *)aosl_malloc (sizeof *prepare_calls);
		if (prepare_calls == NULL)
			return -AOSL_ENOMEM;

		INIT_LIST_HEAD (prepare_calls);
		curr_stack->prepare_calls = prepare_calls;
	}

	node = (struct prepare_node *)aosl_malloc (sizeof *node + (argc * sizeof (uintptr_t)));
	if (node == NULL)
		return -AOSL_ENOMEM;

	node->stack_id = (aosl_stack_id_t)(((uintptr_t)this_q->q_stack_base.id - (uintptr_t)stack_id) + (uintptr_t)curr_stack->id);
	node->f_name = aosl_strdup (f_name);
	node->f = f;
	node->argc = argc;
	for (l = 0; l < argc; l++)
		node->argv [l] = argv [l];

	list_add_tail (&node->node, curr_stack->prepare_calls);
	curr_stack->prepare_calls_count++;
	return 0;
}

static int __task_prepare_args (aosl_stack_id_t stack_id, const char *f_name, aosl_prepare_func_t f, uintptr_t argc, va_list args)
{
	uintptr_t *argv = NULL;

	if (argc > AOSL_VAR_ARGS_MAX)
		return -AOSL_E2BIG;

	if (argc > 0) {
		uintptr_t l;

		argv = alloca (sizeof (uintptr_t) * argc);
		for (l = 0; l < argc; l++)
			argv [l] = va_arg (args, uintptr_t);
	}

	return __task_prepare_argv (stack_id, f_name, f, argc, argv);
}

__export_in_so__ int aosl_task_prepare (aosl_stack_id_t stack_id, const char *f_name, aosl_prepare_func_t f, uintptr_t argc, ...)
{
	va_list args;
	int err;

	va_start (args, argc);
	err = __task_prepare_args (stack_id, f_name, f, argc, args);
	va_end (args);
	return_err (err);
}

__export_in_so__ int aosl_task_prepare_args (aosl_stack_id_t stack_id, const char *f_name, aosl_prepare_func_t f, uintptr_t argc, va_list args)
{
	return_err (__task_prepare_args (stack_id, f_name, f, argc, args));
}

__export_in_so__ int aosl_task_prepare_argv (aosl_stack_id_t stack_id, const char *f_name, aosl_prepare_func_t f, uintptr_t argc, uintptr_t argv [])
{
	return_err (__task_prepare_argv (stack_id, f_name, f, argc, argv));
}

static int __task_resume_argv (aosl_stack_id_t stack_id, const char *f_name, aosl_resume_func_t f, uintptr_t argc, uintptr_t argv [])
{
	struct mp_queue *this_q = THIS_MPQ ();
	int free_only;
	aosl_stack_id_t err_stack;
	struct mpq_stack *curr_stack;
	struct mpq_stack stack;

	if (this_q == NULL)
		return -AOSL_EPERM;

	curr_stack = this_q->q_stack_curr;
	stack_id = (aosl_stack_id_t)(((uintptr_t)this_q->q_stack_base.id - (uintptr_t)stack_id) + (uintptr_t)curr_stack->id);

	if (!IS_ERR_OR_NULL (curr_stack->resume_calls)) {
		struct resume_node *node;
		uintptr_t l;
		
		node = (struct resume_node *)aosl_malloc (sizeof *node + (argc * sizeof (uintptr_t)));
		if (node == NULL) {
			abort ();
			return -AOSL_ENOMEM;
		}

		node->stack_id = stack_id;
		node->f_name = aosl_strdup (f_name);
		node->f = f;
		node->argc = argc;
		for (l = 0; l < argc; l++)
			node->argv [l] = argv [l];

		list_add_tail (&node->node, &curr_stack->resume_calls->list);
		return 0;
	}

	if (curr_stack->prepare_calls != NULL) {
		struct list_head *prepare_calls = curr_stack->prepare_calls;
		curr_stack->prepare_calls = NULL; /* MUST before invoke prepare calls */
		do_prepare_calls (prepare_calls, NULL);
	}

	/**
	 * We need to consider the following cases:
	 * 1. task exec failed due to prepare failed;
	 * 2. task no exec at all;
	 * Invoke the callback function directly for
	 * these cases.
	 * -- Lionfore Hao Oct 13th, 2020
	 **/
	err_stack = curr_stack->err_stack_id;
	if (curr_stack->task_exec_err < 0) {
		free_only = (aosl_stack_invalid (err_stack) || stack_id >= err_stack);
	} else {
		free_only = (!aosl_stack_invalid (err_stack) && stack_id >= err_stack);
	}

	mpq_stack_init (&stack, stack_id);
	this_q->q_stack_curr = &stack;
	f (free_only, argc, argv);
	this_q->q_stack_curr = curr_stack;
	mpq_stack_fini (&stack);
	return 0;
}

static int __task_resume_args (aosl_stack_id_t stack_id, const char *f_name, aosl_resume_func_t f, uintptr_t argc, va_list args)
{
	uintptr_t *argv = NULL;

	if (argc > AOSL_VAR_ARGS_MAX)
		return -AOSL_E2BIG;

	if (argc > 0) {
		uintptr_t l;

		argv = alloca (sizeof (uintptr_t) * argc);
		for (l = 0; l < argc; l++)
			argv [l] = va_arg (args, uintptr_t);
	}

	return __task_resume_argv (stack_id, f_name, f, argc, argv);
}

__export_in_so__ int aosl_task_resume (aosl_stack_id_t stack_id, const char *f_name, aosl_resume_func_t f, uintptr_t argc, ...)
{
	va_list args;
	int err;

	va_start (args, argc);
	err = __task_resume_args (stack_id, f_name, f, argc, args);
	va_end (args);
	return_err (err);
}

__export_in_so__ int aosl_task_resume_args (aosl_stack_id_t stack_id, const char *f_name, aosl_resume_func_t f, uintptr_t argc, va_list args)
{
	return_err (__task_resume_args (stack_id, f_name, f, argc, args));
}

__export_in_so__ int aosl_task_resume_argv (aosl_stack_id_t stack_id, const char *f_name, aosl_resume_func_t f, uintptr_t argc, uintptr_t argv [])
{
	return_err (__task_resume_argv (stack_id, f_name, f, argc, argv));
}