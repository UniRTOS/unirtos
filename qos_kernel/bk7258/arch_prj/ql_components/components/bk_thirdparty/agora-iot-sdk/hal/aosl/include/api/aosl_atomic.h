/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Jul 23rd, 2018
 * Module:	AOSL atomic operation API definitions.
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AOSL_ATOMIC_H__
#define __AOSL_ATOMIC_H__

#include <api/aosl_types.h>
#include <api/aosl_defs.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef int aosl_atomic_t;

extern __aosl_api__ int aosl_atomic_read (const aosl_atomic_t *v);
extern __aosl_api__ void aosl_atomic_set (aosl_atomic_t *v, int i);

extern __aosl_api__ void aosl_atomic_inc (aosl_atomic_t *v);
extern __aosl_api__ void aosl_atomic_dec (aosl_atomic_t *v);

extern __aosl_api__ int aosl_atomic_add_return (int i, aosl_atomic_t *v);
extern __aosl_api__ int aosl_atomic_sub_return (int i, aosl_atomic_t *v);

extern __aosl_api__ int aosl_atomic_inc_and_test (aosl_atomic_t *v);
extern __aosl_api__ int aosl_atomic_dec_and_test (aosl_atomic_t *v);

extern __aosl_api__ int aosl_atomic_cmpxchg (aosl_atomic_t *v, int oldval, int newval);
extern __aosl_api__ int aosl_atomic_xchg (aosl_atomic_t *v, int newval);

extern __aosl_api__ void aosl_mb (void);
extern __aosl_api__ void aosl_rmb (void);
extern __aosl_api__ void aosl_wmb (void);


#ifdef __cplusplus
}
#endif


#endif /* __AOSL_ATOMIC_H__ */