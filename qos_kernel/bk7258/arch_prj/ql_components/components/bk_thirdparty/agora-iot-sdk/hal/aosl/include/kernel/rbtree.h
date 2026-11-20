#ifndef __KERNEL_RBTREE_H__
#define __KERNEL_RBTREE_H__


#include <api/aosl_rbtree.h>


extern void aosl_rb_insert_color (struct aosl_rb_node *, struct aosl_rb_root *);

/* Find logical next and previous nodes in a tree */
extern struct aosl_rb_node *aosl_rb_next (struct aosl_rb_node *);
extern struct aosl_rb_node *aosl_rb_prev (struct aosl_rb_node *);
extern struct aosl_rb_node *aosl_rb_first (struct aosl_rb_root *);
extern struct aosl_rb_node *aosl_rb_last (struct aosl_rb_root *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
extern void aosl_rb_replace_node (struct aosl_rb_node *victim, struct aosl_rb_node *new_node, struct aosl_rb_root *root);

static inline void
rb_link_node (struct aosl_rb_node *node, struct aosl_rb_node *parent, struct aosl_rb_node **rb_link)
{
	node->rb_parent_color = (uintptr_t) parent;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}

static __inline__ void rb_insert (struct aosl_rb_root *root, struct aosl_rb_node *node,
			struct aosl_rb_node **rb_link, struct aosl_rb_node *rb_parent)
{
	rb_link_node (node, rb_parent, rb_link);
	aosl_rb_insert_color (node, root);
}

/* Postorder iteration - always visit the parent after its children */
extern struct aosl_rb_node *rb_first_postorder(const struct aosl_rb_root *);
extern struct aosl_rb_node *rb_next_postorder(const struct aosl_rb_node *);

#define aosl_rb_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? aosl_rb_entry(____ptr, type, member) : NULL; \
	})

/**
 * rbtree_postorder_for_each_entry_safe - iterate over aosl_rb_root in post order of
 * given type safe against removal of aosl_rb_node entry
 *
 * @pos:	the 'type *' to use as a loop cursor.
 * @n:		another 'type *' to use as temporary storage
 * @root:	'aosl_rb_root *' of the rbtree.
 * @field:	the name of the aosl_rb_node field within 'type'.
 */
#define rbtree_postorder_for_each_entry_safe(pos, n, root, field) \
	for (pos = aosl_rb_entry_safe(rb_first_postorder(root), typeof(*pos), field); \
	     pos && ({ n = aosl_rb_entry_safe(rb_next_postorder(&pos->field), \
			typeof(*pos), field); 1; }); \
	     pos = n)





#endif /* __KERNEL_RBTREE_H__ */
