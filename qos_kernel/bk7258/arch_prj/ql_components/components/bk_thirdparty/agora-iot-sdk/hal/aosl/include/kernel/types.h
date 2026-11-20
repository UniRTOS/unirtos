#ifndef __AOSL_KERNEL_TYPES_H__
#define __AOSL_KERNEL_TYPES_H__

#include <api/aosl_types.h>

#ifdef __CHECKER__
#define __aosl_bitwise__ __attribute__((bitwise))
#else
#define __aosl_bitwise__
#endif
#ifdef __CHECK_ENDIAN__
#define __aosl_bitwise __aosl_bitwise__
#else
#define __aosl_bitwise
#endif

typedef uint16_t __aosl_bitwise aosl_le16;
typedef uint16_t __aosl_bitwise aosl_be16;
typedef uint32_t __aosl_bitwise aosl_le32;
typedef uint32_t __aosl_bitwise aosl_be32;
typedef uint64_t __aosl_bitwise aosl_le64;
typedef uint64_t __aosl_bitwise aosl_be64;

typedef uint16_t __aosl_bitwise aosl_sum16;
typedef uint32_t __aosl_bitwise aosl_wsum;

#endif /* __AOSL_KERNEL_TYPES_H__ */
