/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Aug 17th, 2020
 * Module:	Internal used net relative functionals header file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __KERNEL_FILE_H__
#define __KERNEL_FILE_H__


#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_rbtree.h>
#include <kernel/atomic.h>
#include <kernel/list.h>

typedef void (*k_obj_dtor_t) (void *obj);

struct file_obj {
	aosl_fd_t fd;
	atomic_t usage;
	int mpq_fd;
	uint32_t life_id;
	k_obj_dtor_t dtor;
};


extern void fileobj_init (void);

extern int install_fd (aosl_fd_t fd, struct file_obj *f);
extern int remove_fd (struct file_obj *f);

static __inline__ void __fget (struct file_obj *f)
{
	atomic_inc (&f->usage);
}

extern struct file_obj *fget (aosl_fd_t fd);
extern void fput (struct file_obj *f);



#endif /* __KERNEL_FILE_H__ */
