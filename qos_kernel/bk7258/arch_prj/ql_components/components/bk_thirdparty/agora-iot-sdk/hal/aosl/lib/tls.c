/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Jul 24th, 2020
 * Module:	AOSL RB tree based TLS implementation for those
 *          OS having no TLS support.
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#include <string.h>

#include <kernel/types.h>
#include <kernel/err.h>
#include <kernel/bug.h>
#include <kernel/thread.h>
#include <kernel/bitmap.h>
#include <api/aosl_mm.h>
#include <api/aosl_thread.h>
#include <api/aosl_rbtree.h>

struct k_tls_slot {
	uintptr_t seq;
	/* void (*dtor) (void *); */
};

#define STATIC_TLS_KEY_ID_SIZE 16

static k_rwlock_t tls_key_id_lock;
static bitmap_t *tls_key_id_bits = NULL;

static struct k_tls_slot static_tls_slot_table [STATIC_TLS_KEY_ID_SIZE];
static struct k_tls_slot *tls_slot_table = static_tls_slot_table;

static int tls_key_id_size = STATIC_TLS_KEY_ID_SIZE;

/* The max simultaneous TLS key count we supported */
#define TLS_KEY_ID_MAX_SIZE 512

static int alloc_tls_key (void)
{
	int key;

	k_rwlock_wrlock (&tls_key_id_lock);
	key = bitmap_find_first_zero_bit (tls_key_id_bits);
	if (key < 0) {
		int new_size;
		bitmap_t *new_bits;
		struct k_tls_slot *new_table;

		if (tls_key_id_size >= TLS_KEY_ID_MAX_SIZE) {
			k_rwlock_wrunlock (&tls_key_id_lock);
			return -AOSL_EOVERFLOW;
		}

		new_size = tls_key_id_size + 8;

		new_bits = bitmap_create (new_size);
		if (!new_bits) {
			k_rwlock_wrunlock (&tls_key_id_lock);
			return -AOSL_ENOMEM;
		}

		new_table = (struct k_tls_slot *)aosl_malloc (sizeof (struct k_tls_slot) * new_size);
		if (!new_table) {
			k_rwlock_wrunlock (&tls_key_id_lock);
			bitmap_destroy (new_bits);
			return -AOSL_ENOMEM;
		}

		bitmap_copy (new_bits, tls_key_id_bits);
		memcpy (new_table, tls_slot_table, sizeof (struct k_tls_slot) * tls_key_id_size);
		memset (new_table + tls_key_id_size, 0, (new_size - tls_key_id_size) * sizeof (struct k_tls_slot));

		bitmap_destroy (tls_key_id_bits);

		if (tls_slot_table != static_tls_slot_table) {
			aosl_free (tls_slot_table);
		}

		tls_key_id_bits = new_bits;
		tls_slot_table = new_table;
		tls_key_id_size = new_size;

		key = bitmap_find_first_zero_bit (tls_key_id_bits);
		BUG_ON (key >= 0);
	}

	bitmap_set (tls_key_id_bits, key);
	tls_slot_table [key].seq++;
	/* tls_slot_table [key].dtor = ?; */
	k_rwlock_wrunlock (&tls_key_id_lock);

	return key;
}

static int free_tls_key (int key)
{
	if (key < 0 || key >= tls_key_id_size)
		return -AOSL_EINVAL;

	k_rwlock_wrlock (&tls_key_id_lock);
	if (key >= 0 && key < tls_key_id_size) {
		bitmap_clear (tls_key_id_bits, key);
		tls_slot_table [key].seq++;
		/* tls_slot_table [key].dtor = NULL; */
	}
	k_rwlock_wrunlock (&tls_key_id_lock);
	return 0;
}

int k_tls_key_create (k_tls_key_t *key)
{
	int err = alloc_tls_key ();
	if (err < 0)
		return err;

	*key = (k_tls_key_t)err;
	return 0;
}

static struct aosl_rb_root thread_tree;
static k_rwlock_t thread_tree_lock;

#define __KEY_USED(k) (((k).seq & 1) != 0)
/**
 * Check whether a key is usable.  We cannot reuse an allocated key if
 * the sequence counter would overflow after the next destroy call.
 * This would mean that we potentially free memory for a key with the
 * same sequence.  This is *very* unlikely to happen, A program would
 * have to create and destroy a key 2^31 times (on 32-bit platforms,
 * on 64-bit platforms that would be 2^63).  If it should happen we
 * simply don't use this specific key anymore.
 **/
#define __KEY_USABLE(k) (((uintptr_t)(k).seq) < ((uintptr_t)((p).seq + 2)))

struct k_tls_value {
	uintptr_t seq;
	void *val;
};

struct tls_thread_node {
	struct aosl_rb_node rb_node;
	k_thread_t thread_id;
	size_t tls_key_table_size;
	struct k_tls_value *tls_key_table;
};

static int cmp_thread (struct aosl_rb_node *rb_node, struct aosl_rb_node *node, va_list args)
{
	struct tls_thread_node *rb_entry = aosl_rb_entry (rb_node, struct tls_thread_node, rb_node);
	k_thread_t thread_id;

	if (node != NULL) {
		thread_id = aosl_rb_entry (node, struct tls_thread_node, rb_node)->thread_id;
	} else {
		thread_id = va_arg (args, k_thread_t);
	}

	if (rb_entry->thread_id > thread_id)
		return 1;

	if (rb_entry->thread_id < thread_id)
		return -1;

	return 0;
}

static struct k_tls_value* get_tls_value (k_tls_key_t key)
{
	k_thread_t this_thread;
	struct aosl_rb_node *node;
	struct tls_thread_node *thread_node;
	struct k_tls_value *tls_val;
	struct k_tls_slot key_slot;

	if (key < 0 || key >= tls_key_id_size)
		return NULL;

	k_rwlock_rdlock (&tls_key_id_lock);
	key_slot = tls_slot_table [key];
	k_rwlock_rdunlock (&tls_key_id_lock);

	if (!__KEY_USED (key_slot))
		return NULL;

	this_thread = k_thread_self ();
	k_rwlock_rdlock (&thread_tree_lock);
	node = aosl_find_rb_node (&thread_tree, NULL, this_thread);
	if (node != NULL) {
		thread_node = aosl_rb_entry (node, struct tls_thread_node, rb_node);
	} else {
		thread_node = NULL;
	}
	k_rwlock_rdunlock (&thread_tree_lock);

	if (thread_node == NULL || key >= (int)thread_node->tls_key_table_size)
		return NULL;

	tls_val = &thread_node->tls_key_table [key];
	if (tls_val->val != NULL) {
		if (tls_val->seq != key_slot.seq) {
			tls_val->val = NULL;
			return NULL;
		}
	}

	return tls_val;
}

void *k_tls_key_get (k_tls_key_t key)
{
	struct k_tls_value* tls_val;
	if (NULL == (tls_val = get_tls_value (key)))
		return NULL;

	return tls_val->val;
}

void **k_tls_key_get_ref (k_tls_key_t key)
{
	struct k_tls_value* tls_val;
	if (NULL == (tls_val = get_tls_value (key)))
		return NULL;

	return &tls_val->val;
}

int k_tls_key_set (k_tls_key_t key, void *value)
{
	k_thread_t this_thread;
	struct aosl_rb_node *node;
	struct tls_thread_node *thread_node;
	struct k_tls_slot key_slot;

	if (key < 0 || key >= tls_key_id_size)
		return -AOSL_EINVAL;

	k_rwlock_rdlock (&tls_key_id_lock);
	key_slot = tls_slot_table [key];
	k_rwlock_rdunlock (&tls_key_id_lock);

	if (!__KEY_USED (key_slot))
		return -AOSL_EINVAL;

	this_thread = k_thread_self ();
	k_rwlock_rdlock (&thread_tree_lock);
	node = aosl_find_rb_node (&thread_tree, NULL, this_thread);
	if (node != NULL) {
		thread_node = aosl_rb_entry (node, struct tls_thread_node, rb_node);
	} else {
		thread_node = NULL;
	}
	k_rwlock_rdunlock (&thread_tree_lock);

	if (thread_node == NULL) {
		thread_node = (struct tls_thread_node *)aosl_malloc (sizeof *thread_node);
		if (thread_node == NULL)
			abort ();

		thread_node->thread_id = this_thread;
		thread_node->tls_key_table = (struct k_tls_value *)aosl_malloc (sizeof (struct k_tls_value) * tls_key_id_size);
		if (thread_node->tls_key_table == NULL)
			abort ();

		memset (thread_node->tls_key_table, 0, sizeof (struct k_tls_value) * tls_key_id_size);
		thread_node->tls_key_table_size = tls_key_id_size;

		k_rwlock_wrlock (&thread_tree_lock);
		/**
		 * No find the thread node again when we get the write lock,
		 * because the thread node can only be created by the thread
		 * itself, so no racing condition after we released the read
		 * lock and before the write lock.
		 * -- Lionfore Hao Jul 23rd, 2020
		 **/
		aosl_rb_insert_node (&thread_tree, &thread_node->rb_node);
		k_rwlock_wrunlock (&thread_tree_lock);
	}

	if (key >= (int)thread_node->tls_key_table_size) {
		int new_size = tls_key_id_size;
		struct k_tls_value *new_table = (struct k_tls_value *)aosl_malloc (sizeof (struct k_tls_value) * new_size);
		if (new_table == NULL) {
			abort ();
			return -AOSL_ENOMEM;
		}

		if (thread_node->tls_key_table_size > 0) {
			BUG_ON (thread_node->tls_key_table_size > new_size);
			memcpy (new_table, thread_node->tls_key_table, sizeof (struct k_tls_value) * thread_node->tls_key_table_size);
			memset (new_table + thread_node->tls_key_table_size, 0, sizeof (struct k_tls_value) * (new_size - thread_node->tls_key_table_size));
			aosl_free (thread_node->tls_key_table);
		}

		thread_node->tls_key_table = new_table;
		thread_node->tls_key_table_size = new_size;
	}

	BUG_ON (key >= (int)thread_node->tls_key_table_size);
	thread_node->tls_key_table [key].val = value;
	thread_node->tls_key_table [key].seq = key_slot.seq;
	return 0;
}

int k_tls_key_delete (k_tls_key_t key)
{
	return free_tls_key ((int)key);
}

void rb_tls_init (void)
{
	int i;
	tls_key_id_bits = bitmap_create (STATIC_TLS_KEY_ID_SIZE);
	k_rwlock_init (&tls_key_id_lock);
	aosl_rb_root_init (&thread_tree, cmp_thread);
	k_rwlock_init (&thread_tree_lock);
	for (i = 0; i < tls_key_id_size; i++) {
		tls_slot_table [i].seq = 0;
		/* tls_slot_table [i].dtor = NULL; */
	}
}

static void tls_thread_node_destroy()
{
	struct aosl_rb_node *node;
	struct tls_thread_node *thread_node;

	for (;;) {
		if (NULL == (node = aosl_rb_first (&thread_tree))) {
			break;
		}

		aosl_rb_erase (&thread_tree, node);
		thread_node = aosl_rb_entry(node, struct tls_thread_node, rb_node);
		if (thread_node->tls_key_table) {
			aosl_free(thread_node->tls_key_table);
		}
		aosl_free(thread_node);
	}
}

void rb_tls_fini (void)
{
	tls_thread_node_destroy ();
	aosl_rb_root_init (&thread_tree, cmp_thread);

	k_rwlock_wrlock (&tls_key_id_lock);
	if (tls_slot_table != static_tls_slot_table) {
		aosl_free (tls_slot_table);
		tls_slot_table = static_tls_slot_table;
	}

	bitmap_destroy (tls_key_id_bits);
	tls_key_id_bits = NULL;
	tls_key_id_size = STATIC_TLS_KEY_ID_SIZE;
	k_rwlock_wrunlock (&tls_key_id_lock);

	k_rwlock_destroy (&tls_key_id_lock);
	k_rwlock_destroy (&thread_tree_lock);
}