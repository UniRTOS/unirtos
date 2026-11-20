#ifndef __KERNEL_COMPILER_H__
#define __KERNEL_COMPILER_H__

#ifndef likely
#define likely(x) (x)
#endif

#ifndef unlikely
#define unlikely(x) (x)
#endif

#ifndef FuncReturnAddress
#define FuncReturnAddress() __builtin_return_address(0)
#endif

#endif /* __KERNEL_COMPILER_H__ */
