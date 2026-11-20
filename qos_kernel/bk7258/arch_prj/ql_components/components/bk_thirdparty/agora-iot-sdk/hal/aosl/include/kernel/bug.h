#ifndef __BUG_H__
#define __BUG_H__

#include <stdio.h>
#include <stdlib.h>

#include <api/aosl_types.h>
#include <kernel/compiler.h>
#include <kernel/log.h>

#ifdef __CHECKER__
#define BUILD_BUG_ON_NOT_POWER_OF_2(n) (0)
#define BUILD_BUG_ON_ZERO(e) (0)
#define BUILD_BUG_ON_NULL(e) ((void*)0)
#define BUILD_BUG_ON_INVALID(e) (0)
#define BUILD_BUG_ON_MSG(cond, msg) (0)
#define BUILD_BUG_ON(condition) (0)
#define BUILD_BUG() (0)
#else /* __CHECKER__ */

/* Force a compilation error if a constant expression is not a power of 2 */
#define BUILD_BUG_ON_NOT_POWER_OF_2(n)			\
	BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))

/* Force a compilation error if condition is true, but also produce a
   result (of value 0 and type size_t), so the expression can be used
   e.g. in a structure initializer (or where-ever else comma expressions
   aren't permitted). */
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define BUILD_BUG_ON_NULL(e) ((void *)sizeof(struct { int:-!!(e); }))

/*
 * BUILD_BUG_ON_INVALID() permits the compiler to check the validity of the
 * expression but avoids the generation of any code, even if that expression
 * has side-effects.
 */
#define BUILD_BUG_ON_INVALID(e) ((void)(sizeof((intptr_t)(e))))

/**
 * BUILD_BUG_ON_MSG - break compile if a condition is true & emit supplied
 *		      error message.
 * @condition: the condition which the compiler should know is false.
 *
 * See BUILD_BUG_ON for description.
 */
#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)

/**
 * BUILD_BUG_ON - break compile if a condition is true.
 * @condition: the condition which the compiler should know is false.
 *
 * If you have some code which relies on certain constants being equal, or
 * some other compile-time-evaluated condition, you should use BUILD_BUG_ON to
 * detect if someone changes it.
 *
 * The implementation uses gcc's reluctance to create a negative array, but gcc
 * (as of 4.4) only emits that error for obvious cases (e.g. not arguments to
 * inline functions).  Luckily, in 4.3 they added the "error" function
 * attribute just for this type of case.  Thus, we use a negative sized array
 * (should always create an error on gcc versions older than 4.4) and then call
 * an undefined function with the error attribute (should always create an
 * error on gcc 4.3 and later).  If for some reason, neither creates a
 * compile-time error, we'll still have a link-time error, which is harder to
 * track down.
 */
#ifndef __OPTIMIZE__
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#else
#define BUILD_BUG_ON(condition) \
	BUILD_BUG_ON_MSG(condition, "BUILD_BUG_ON failed: " #condition)
#endif

/**
 * BUILD_BUG - break compile if used.
 *
 * If you have some code that you expect the compiler to eliminate at
 * build time, you should use BUILD_BUG to detect if it is
 * unexpectedly used.
 */
#define BUILD_BUG() BUILD_BUG_ON_MSG(1, "BUILD_BUG failed")

#endif	/* __CHECKER__ */

extern void bug_slowpath (const char *file, int line, void *caller, const char *fmt, ...);
#define BUG(...) bug_slowpath (__FILE__, __LINE__, FuncReturnAddress (), __VA_ARGS__)

#define BUG_ON(x) if (unlikely (x)) BUG (NULL)
#define BUG_ON_printf(x, fmt, ...) if (unlikely (x)) BUG (fmt, __VA_ARGS__)



#define panic(...) \
	do { \
		char __buf [512]; \
		snprintf (__buf, sizeof __buf, __VAR_ARGS__); \
		aosl_printf ("PANIC @%s(%d): %s", __FILE__, __LINE__, __buf); \
		exit (-1); \
	} while(0)


extern void warn_slowpath_null(const char *file, int line, void *caller);
#define __WARN() warn_slowpath_null(__FILE__, __LINE__, FuncReturnAddress ())

#ifndef WARN_ON
#define WARN_ON(condition) ({						\
		int __ret_warn_on = !!(condition);				\
		if (unlikely(__ret_warn_on))					\
			__WARN();						\
		unlikely(__ret_warn_on);					\
	})
#endif

extern void warn_slowpath_fmt(const char *file, int line, void *caller, const char *fmt, ...);
#define __WARN_printf(...) warn_slowpath_fmt(__FILE__, __LINE__, FuncReturnAddress (), __VA_ARGS__)

#ifndef WARN
#define WARN(condition, ...) ({						\
		int __ret_warn_on = !!(condition);				\
		if (unlikely(__ret_warn_on))					\
			__WARN_printf(__VA_ARGS__);					\
		unlikely(__ret_warn_on);					\
	})
#endif


#define WARN_ON_ONCE(condition)	({				\
	static int __section(.data.unlikely) __warned;		\
	int __ret_warn_once = !!(condition);			\
								\
	if (unlikely(__ret_warn_once))				\
		if (WARN_ON(!__warned)) 			\
			__warned = 1;			\
	unlikely(__ret_warn_once);				\
})

#define WARN_ONCE(condition, ...)	({			\
	static int __section(.data.unlikely) __warned;		\
	int __ret_warn_once = !!(condition);			\
								\
	if (unlikely(__ret_warn_once))				\
		if (WARN(!__warned, __VA_ARGS__)) 			\
			__warned = 1;			\
	unlikely(__ret_warn_once);				\
})



#endif	/* __BUG_H__ */
