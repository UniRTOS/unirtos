#pragma once

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#ifndef UNUSED_ATTR
#define UNUSED_ATTR __attribute__((unused))
#endif

#define INVALID_DEVICE          (NULL)


typedef void *device_object_t;

#ifndef __containerof
#define __containerof(ptr, type, member) ((type *)(void *)((char *)ptr - offsetof(type, member)))
#endif

#ifndef likely
#define likely(x)                    __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)                  __builtin_expect(!!(x), 0)
#endif


#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */