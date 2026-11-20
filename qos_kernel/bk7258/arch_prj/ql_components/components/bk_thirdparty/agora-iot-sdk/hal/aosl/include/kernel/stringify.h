#ifndef __KERNEL_STRINGIFY_H__
#define __KERNEL_STRINGIFY_H__

/* Indirect stringification.  Doing two levels allows the parameter to be a
 * macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
 * converts to "bar".
 */

#define __stringify_1(x)	#x
#define __stringify(...)	__stringify_1(__VA_ARGS__)

#endif	/* !__KERNEL_STRINGIFY_H__ */
