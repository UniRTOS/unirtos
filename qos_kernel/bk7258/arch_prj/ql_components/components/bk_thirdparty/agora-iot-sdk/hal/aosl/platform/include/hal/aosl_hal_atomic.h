/*************************************************************
 * Author:	zhangguanxian@agora.io
 * Date	 :	2025/12/16
 * Module:	atomic hal definitions.
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2025 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#ifndef __AOSL_HAL_ATOMIC_H__
#define __AOSL_HAL_ATOMIC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * atomic_read - read atomic variable
 * @v: pointer of type intptr_t
 *
 * Atomically reads the value of @v.
 * Doesn't imply a read memory barrier.
 */
intptr_t aosl_hal_atomic_read(const intptr_t *v);

/**
 * atomic_set - set atomic variable
 * @v: pointer to type intptr_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
void aosl_hal_atomic_set(intptr_t *v, intptr_t i);

/**
 * atomic_inc - increment atomic variable
 * @v: pointer to type intptr_t
 *
 * Atomically increments @v by 1.
 */
intptr_t aosl_hal_atomic_inc(intptr_t *v);

/**
 * atomic_dec - decrement atomic variable
 * @v: pointer to type intptr_t
 *
 * Atomically decrements @v by 1.
 */
intptr_t aosl_hal_atomic_dec(intptr_t *v);

/**
 * atomic add and return
 * @i: integer value to add
 * @v: pointer to type intptr_t
 *
 * Atomically adds @i to @v and returns @i + @v
 */
intptr_t aosl_hal_atomic_add(intptr_t i, intptr_t *v);

/**
 * atomic sub and return
 * @i: integer value to sub
 * @v: pointer to type intptr_t
 *
 * Atomically subs @i to @v and returns @v - @i
 */
intptr_t aosl_hal_atomic_sub(intptr_t i, intptr_t *v);

/**
 * atomic compile and exchange
 * @v: pointer to type intptr_t
 * @old: old value to compare
 * @new: new value to set
 *
 * Atomically sets @v to @new and returns the old value of @v
 */
intptr_t aosl_hal_atomic_cmpxchg(intptr_t *v, intptr_t old, intptr_t new);

/**
 * atomic exchange
 * @v: pointer to type intptr_t
 * @new: new value to set
 *
 * Atomically sets @v to @new and returns the old value of @v
 */
intptr_t aosl_hal_atomic_xchg(intptr_t *v, intptr_t new);

/**
 * @brief Memory barrier
 */
void aosl_hal_mb();
/**
 * @brief Read memory barrier
 */
void aosl_hal_rmb();
/**
 * @brief Write memory barrier
 */
void aosl_hal_wmb();

#ifdef __cplusplus
}
#endif

#endif /* __AOSL_HAL_ATOMIC_H__ */
