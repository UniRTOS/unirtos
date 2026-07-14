/*****************************************************************/ /**
* @file qosa_def.h
* @brief Basic definitions and data types for UniRTOS
* @author larson.li@quectel.com
* @date 2023-04-19
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-04-19 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_DEF_H__
#define __QOSA_DEF_H__

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>

//Platform compilation configuration
#include "qosa_platform_cfg.h"

// system support configuration, every platform is different
#include "qosa_system_config.h"


// Define the data type of CPU related data type
typedef signed long   qosa_base_t;  /**< Nbit CPU related date type */
typedef unsigned long qosa_ubase_t; /**< Nbit unsigned CPU related date type */

#ifdef CONFIG_QOSA_PLATFORM_SUPPORT_C99
#include <stdbool.h>
#include <stdint.h>

typedef bool    qosa_bool_t;  /**< boolean type */
typedef int8_t  qosa_int8_t;  /**<  8bit integer type */
typedef int16_t qosa_int16_t; /**< 16bit integer type */

/** The size in bytes depends on the compiler and platform */
#if defined(CONFIG_QOSA_RDA8910_R06_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC)                   \
    || defined(CONFIG_QOSA_UIS8852_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
/**< 32bit integer type */
typedef signed int qosa_int32_t;
#else                           /* CONFIG_QOSA_RDA8910_R06_PLATFORM_FUNC */
/**< 32bit integer type */
typedef int32_t      qosa_int32_t;
#endif                          /*.CONFIG_QOSA_RDA8910_R06_PLATFORM_FUNC */

typedef uint8_t  qosa_uint8_t;  /**<  8bit unsigned integer type */
typedef uint16_t qosa_uint16_t; /**< 16bit unsigned integer type */
typedef uint32_t qosa_uint32_t; /**< 32bit unsigned integer type */
typedef int64_t  qosa_int64_t;  /**< 64bit integer type */
typedef uint64_t qosa_uint64_t; /**< 64bit unsigned integer type */
typedef size_t   qosa_size_t;   /**< Type for size number */
typedef ssize_t  qosa_ssize_t;  /**< Used for a count of bytes or an error indication */

#else                           /* CONFIG_QOSA_PLATFORM_SUPPORT_C99 */

typedef unsigned char    qosa_bool_t;   /**< boolean type */
typedef signed char    qosa_int8_t;   /**<  8bit integer type */
typedef signed short   qosa_int16_t;  /**< 16bit integer type */
typedef signed int     qosa_int32_t;  /**< 32bit integer type */
typedef unsigned char  qosa_uint8_t;  /**<  8bit unsigned integer type */
typedef unsigned short qosa_uint16_t; /**< 16bit unsigned integer type */
typedef unsigned int   qosa_uint32_t; /**< 32bit unsigned integer type */

#ifdef CONFIG_QOSA_HAVE_SYS_TPYE64
typedef signed long    qosa_int64_t;  /**< 64bit integer type */
typedef unsigned long  qosa_uint64_t; /**< 64bit unsigned integer type */
#else  /* CONFIG_QOSA_HAVE_SYS_TPYE64 */
typedef signed long long   qosa_int64_t;  /**< 64bit integer type */
typedef unsigned long long qosa_uint64_t; /**< 64bit unsigned integer type */
#endif /* CONFIG_QOSA_HAVE_SYS_TPYE64 */

/** The size in bytes depends on the compiler and platform */
#if defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC) /* [huanyi][20260319]temporary changes for lwm2m */
typedef size_t   qosa_size_t;   /**< Type for size number */
#else /* CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC */
typedef qosa_ubase_t qosa_size_t;     /**< Type for size number */
#endif /* CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC */

typedef qosa_base_t  qosa_ssize_t;    /**< Used for a count of bytes or an error indication */
#endif /* CONFIG_QOSA_PLATFORM_SUPPORT_C99 */

#ifdef CONFIG_QOSA_HAVE_SYS_TPYE64
/** Save function pointer address */
typedef qosa_int64_t qosa_ptr;
#else  /* CONFIG_QOSA_HAVE_SYS_TPYE64 */
/** Save function pointer address */
typedef qosa_int32_t qosa_ptr;
#endif /* CONFIG_QOSA_HAVE_SYS_TPYE64 */

typedef qosa_int64_t qosa_time_t;

/* Used for 64-bit printing */
#define Q_64_LLD          "lld"
#define Q_64_LLU          "llu"

#define QOSA_TRUE         1                    /**< boolean true  */
#define QOSA_FALSE        0                    /**< boolean fails */
#define QOSA_NULL         0                    /**< null pointer definition */

#define QOSA_UINT8_MAX    0xFF                 /**< Maximum number of UINT8 */
#define QOSA_UIN16_MAX    0xFFFF               /**< Maximum number of UINT16 */
#define QOSA_UINT32_MAX   0xFFFFFFFF           /**< Maximum number of UINT32 */

#define QOSA_UNUSED(x)    ((void)x)            //Fix false positives for defined but unused parameters

#define QOSA_NO_WAIT      ((qosa_uint32_t)0)   //no timeout
#define QOSA_WAIT_FOREVER ((qosa_uint32_t)-1)  //wait forever

/** Determine if it is a lowercase letter */
#define ISLOWHEXALHA(x)   (((x) >= 'a') && ((x) <= 'f'))
/** Determine if it is an uppercase letter */
#define ISUPHEXALHA(x)    (((x) >= 'A') && ((x) <= 'F'))
/** Determine if it is an uppercase letter */
#define ISUPPER(x)        (((x) >= 'A') && ((x) <= 'Z'))
/** Check if character is a lowercase letter */
#define ISLOWER(x)        (((x) >= 'a') && ((x) <= 'z'))
/** Check if digit is a number */
#define ISDIGIT(x)        (((x) >= '0') && ((x) <= '9'))
/** Check if it is empty */
#define ISBLANK(x)        (((x) == ' ') || ((x) == '\t'))
/** Check if it is end */
#define ISSPACE(x)        (ISBLANK(x) || (((x) >= 0xa) && ((x) <= 0x0d)))

#define ISLOWCNTRL(x)     ((x) >= 0 && ((x) <= 0x1f))
#define IS7F(x)           ((x) == 0x7f)
#define ISLOWPRINT(x)     (((x) >= 9) && ((x) <= 0x0d))
/** Test if character is printable (including space) */
#define ISPRINT(x)        (ISLOWPRINT(x) || (((x) >= ' ') && ((x) <= 0x7e)))
/** Test if character is printable */
#define ISGRAPH(x)        (ISLOWPRINT(x) || (((x) > ' ') && ((x) <= 0x7e)))
/**
 * Check if the passed character is a control character, control character ASCII code is between 0x00 (NUL) and
 *  0x1f (US), and 0x7f (DEL)
 */
#define ISCNTRL(x)        (ISLOWCNTRL(x) || IS7F(x))
/** Check if parameter x is an English letter */
#define ISALPHA(x)        (ISLOWER(x) || ISUPPER(x))
/** Check if parameter x is a hexadecimal digit */
#define ISXDIGIT(x)       (ISDIGIT(x) || ISLOWHEXALHA(x) || ISUPHEXALHA(x))
/** Determine if character variable x is a letter or digit, return non-zero if yes, otherwise return zero*/
#define ISALNUM(x)        (ISDIGIT(x) || ISLOWER(x) || ISUPPER(x))
//toupper function
/** If character x is lowercase letter, convert to uppercase letter */
#define UPCASE(x)         (ISLOWER(x) ? ((x)-0x20) : (x))
//tolower function
/** If character x is uppercase letter, convert to lowercase letter */
#define LOWCASE(x)        (ISLOWER(x) ? (x) : ((x) + 0x20))

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif

#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
#include "float.h" //QT2131 is already compatible with DBL_EPSILON.
#else
#ifndef DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-016  // smallest such that 1.0+DBL_EPSILON != 1.0
#endif
#endif

/** Get array space size */
#define QOSA_ARRAY_SIZE(array)       (sizeof(array) / sizeof(array[0]))

/** Get absolute value, same as abs function */
#define QOSA_ABS(x)                  ((x) > 0 ? (x) : -(x))

/**
 * @ingroup Align length to multiple of 4, increase 13->16 if not enough
 *
 * @def RT_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. RT_ALIGN(13, 4)
 * would return 16.
 */
#define QOSA_ALIGN(size, align)      (((size) + (align)-1) & ~((align)-1))

/**
  * @ingroup Align length to multiple of 4, decrease 13->12 if not enough
  *
  * @def RT_ALIGN_DOWN(size, align)
  * Return the down number of aligned at specified width. RT_ALIGN_DOWN(13, 4)
  * would return 12.
  */
#define QOSA_ALIGN_DOWN(size, align) ((size) & ~((align)-1))

#define QOSA_EXTERN
#define QOSA_INLINE         static __inline

/** error code offset, reserve for at error code. */
#define QOSA_AT_ERR_CMS_OFS (1 << 14)
#define QOSA_AT_ERR_OFS     (1 << 15)

/*! base component error mask */
typedef enum
{
    QOSA_COMPONENT_NONE = 0,
    QOSA_COMPONENT_COMMON = 0x8000,        /*!< system common error base */
    QOSA_COMPONENT_OSI = 0x8000,           /*!< system osa error base */
    QOSA_COMPONENT_RT_DEVICE = 0x8001,     /*!< device error base */
    QOSA_COMPONENT_URC = 0x8002,           /*!< urc mode error base */
    QOSA_COMPONENT_EVENT_NOTIFY = 0x8003,  /*!< Asynchronous Event Notification Function error base */
    QOSA_COMPONENT_FILE_API = 0x8004,      /*!< UniRTOS file api Function error base */
    QOSA_COMPONENT_NV_ITEM_CFG = 0x8005,   /*!< NVM configuration parameters error base*/
    QOSA_COMPONENT_DEV = 0x8006,           /*!< Corresponds to dev platform interface handling error status */
    QOSA_COMPONENT_POWER = 0x8007,         /*!< Corresponds to dev platform interface handling error status */
    QOSA_COMPONENT_SIM = 0x8008,           /*!< SIM error base */
    QOSA_COMPONENT_NETWORK = 0x8009,       /*!< NETWORK error base */
    QOSA_COMPONENT_USBNET = 0x8010,        /*!< USBNET error base */
    QOSA_COMPONENT_SMS = 0x800a,           /*!< SMS error base */
    QOSA_COMPONENT_API_ADC = 0x8011,       /*!< ADC API error base */
    QOSA_COMPONENT_API_UART = 0x8012,      /*!< UART API error base */
    QOSA_COMPONENT_BSP_GPIO = 0x8013,      /*!< GPIO result error base */
    QOSA_COMPONENT_BSP_PWM = 0x8014,       /*!< PWM result error base */
    QOSA_COMPONENT_API_RTC = 0x8015,       /*!< RTC API error base */
    QOSA_COMPONENT_API_I2C = 0x8016,       /*!< I2C API error base */
    QOSA_COMPONENT_API_LPM = 0x8017,       /*!< LPM API error base */
    QOSA_COMPONENT_API_SPI = 0x8018,       /*!< SPI API error base */
    QOSA_COMPONENT_API_CAMERA = 0x8019,    /*!< CAMERA API error base */
    QOSA_COMPONENT_BSP_SPI_NOR = 0x801A,   /*!< SPI Nor API error base */
    QOSA_COMPONENT_BSP_BUILT_NOR = 0x801B, /*!< Built-in Nor API error base */
    QOSA_COMPONENT_LOG = 0x801C,           /*!< Built-in Nor API error base */
    QOSA_COMPONENT_BSP_UART = 0x801D,      /*!< UART error base */
    QOSA_COMPONENT_AUDIO = 0x801E,         /*!< Audio error base */
    QOSA_COMPONENT_API_LCD = 0x801F,       /*!< CAMERA API error base */
    QOSA_COMPONENT_API_USB = 0x8020,       /*!< CAMERA API error base */

    QOSA_COMPONENT_PING = 0x8104,          /*!< Ping result error base */
    QOSA_COMPONENT_NTP = 0x8105,           /*!< NTP result error base */
    QOSA_COMPONENT_DNS = 0x8106,           /*!< ASYN DNS result error base */
    QOSA_COMPONENT_VTLS = 0x8107,          /*!< VTLS result error base */
    QOSA_COMPONENT_DATACALL = 0x8108,      /*!< PDP data call error base */
    QOSA_COMPONENT_LWM2M = 0x8109,         /*!< LwM2M error base */
    QOSA_COMPONENT_MQTT = 0x8110,          /*!< MQTT error base */
    QOSA_COMPONENT_MMS = 0x8111,           /*!< MMS error base */
    QOSA_COMPONENT_FOTA = 0x8112,          /*!< FOTA error base */
    QOSA_COMPONENT_NC_KEEP = 0x8113,       /*!< net connect keepalive error base */
    QOSA_COMPONENT_LBS = 0x8114,           /*!< LBS result error base */
    QOSA_COMPONENT_NW = 0x8115,            /*!< NW result error base */
    QOSA_COMPONENT_WIFISCAN = 0x8116,      /*!< WIFISCAN result error base */
    QOSA_COMPONENT_SBFOTA = 0x8117,        /*!< SBFOTA result error base */
    QOSA_COMPONENT_FTM = 0x8118,           /*!< RFTEST result error base */
    QOSA_COMPONENT_QVSIM = 0x8119,         /*!< QVSIM result error base */
    QOSA_COMPONENT_CODE_DECODER = 0x811A,  /*!< Code decoder error base */
    QOSA_COMPONENT_SECBOOT = 0x811B,       /*!< Secure boot error base */
    QOSA_COMPONENT_IMS = 0x811C,           /*!< IMS error base */

    QOSA_COMPONENT_ESIM = 0x8500,          /*!< ESIM result error base */

    QOSA_COMPONENT_MAX = 0x9FFF            /*!< component base end */
} qosa_base_component_e;

/*! common error code */
typedef enum
{
    QOSA_OK = 0,
    QOSA_ERROR_GENERAL = 1 | (QOSA_COMPONENT_COMMON << 16), /*!< generic error,this error
                                                            is only returned when a
                                                            special exception occurs */
    QOSA_ERROR_NO_MEMORY,                                   /*!< memory malloc failed */
    QOSA_ERROR_PARAM_INVALID,                               /*!< Parameter input error */
    QOSA_ERROR_PARAM_IS_NULL,                               /*!< Parameter is empty error */
    QOSA_ERROR_TIMEOUT,                                     /*!< Operation timeout error */
    QOSA_ERROR_NO_RESOURCE,                                 /*!< No resources */
    QOSA_ERROR_VALUE_INVALID,                               /*!< Invalid value */
    QOSA_ERROR_OPERATION,                                   /*!< Invalid operation */
    QOSA_ERROR_INVALID_SIZE,                                /*!< Invalid size */
    QOSA_ERROR_REQUEST,                                     /*!< Invalid request */
    QOSA_ERROR_NO_SPACE,                                    /*!< Queue space full*/
    QOSA_ERROR_DELETED,                                     /*!< Delete error */
    QOSA_ERROR_UNSUPPORT,                                   /*!< Operation not supported */
    QOSA_ERROR_DUPLICATE_NAME,                              /*!< Duplicate name */
} qosa_common_error_e;

/***************************  task priority defination   ******************************/
/** If priority is positive 0 minimum 100+ maximum, should be positive number, otherwise should be modified to -1 when adapting, main purpose is to adjust priority granularity */
#define QOSA_PRIORITY_GRANULARITY_INTERVAL CONFIG_QOSA_PRIORITY_GRANULARITY_NUM

typedef enum
{
    QOSA_PRIORITY_REALTIME = CONFIG_QOSA_PRIORITY_REALTIME_CFG,
    QOSA_PRIORITY_HIGH = CONFIG_QOSA_PRIORITY_HIGH_CFG,
    QOSA_PRIORITY_ABOVE_NORMAL = CONFIG_QOSA_PRIORITY_ABOVE_NORMAL_CFG,
    QOSA_PRIORITY_NORMAL = CONFIG_QOSA_PRIORITY_NORMAL_CFG,
    QOSA_PRIORITY_BELOW_NORMAL = CONFIG_QOSA_PRIORITY_BELOW_NORMAL_CFG,
    QOSA_PRIORITY_LOW = CONFIG_QOSA_PRIORITY_LOW_CFG,
    QOSA_PRIORITY_IDLE = CONFIG_QOSA_PRIORITY_IDLE_CFG,  // reserved
} qosa_thread_priority_e;

QOSA_EXTERN void               qosa_free(void* ptr);
QOSA_EXTERN void*              qosa_realloc(void* ptr, qosa_size_t nbytes);
QOSA_EXTERN void*              qosa_calloc(qosa_size_t count, qosa_size_t size);
QOSA_EXTERN void*              qosa_memset(void* s, int c, qosa_size_t size);
QOSA_EXTERN void*              qosa_memcpy(void* dest, const void* src, qosa_size_t n);
QOSA_EXTERN void*              qosa_memmove(void* dest, const void* src, qosa_size_t n);
QOSA_EXTERN int                qosa_memcmp(const void* s1, const void* s2, qosa_size_t n);
QOSA_EXTERN char*              qosa_strstr(const char* s1, const char* s2);
QOSA_EXTERN qosa_int32_t       qosa_strcasecmp(const char* a, const char* b);
QOSA_EXTERN qosa_size_t        qosa_strlcpy(char* dst, const char* src, qosa_size_t n);
QOSA_EXTERN char*              qosa_strncpy(char* dst, const char* src, qosa_size_t n);
QOSA_EXTERN char*              qosa_strcpy(char* dst, const char* src);
QOSA_EXTERN qosa_int32_t       qosa_strncmp(const char* cs, const char* ct, qosa_size_t count);
QOSA_EXTERN qosa_int32_t       qosa_strcmp(const char* cs, const char* ct);
QOSA_EXTERN qosa_size_t        qosa_strlen(const char* s);
QOSA_EXTERN qosa_size_t        qosa_strnlen(const char* s, qosa_size_t maxlen);
QOSA_EXTERN char*              qosa_strdup(const char* s);
QOSA_EXTERN int                qosa_vsnprintf(char* buf, qosa_size_t size, const char* fmt, va_list args);
QOSA_EXTERN int                qosa_snprintf(char* buf, qosa_size_t size, const char* fmt, ...);
QOSA_EXTERN int                qosa_vsprintf(char* buf, const char* format, va_list arg_ptr);
QOSA_EXTERN int                qosa_sprintf(char* buf, const char* format, ...);
QOSA_EXTERN void               qosa_assert(const char* funcname, qosa_size_t line, int a);
QOSA_EXTERN int                qosa_strncasecmp(const char* s1, const char* s2, qosa_size_t size);
QOSA_EXTERN char*              qosa_strchr(const char* _String, int const _Ch);
QOSA_EXTERN unsigned long int  qosa_strtoul(const char* str, char** endptr, int base);
QOSA_EXTERN long int           qosa_strtol(const char* str, char** endptr, int base);
QOSA_EXTERN unsigned long long qosa_strtoull(const char* str, char** endptr, int base);
QOSA_EXTERN int                qosa_sscanf(const char* str, const char* format, ...);
QOSA_EXTERN char*              qosa_strrchr(const char* _String, int const _Ch);
QOSA_EXTERN long long          qosa_strtoll(const char* str, char** endptr, int base);
QOSA_EXTERN double             qosa_strtod(const char* str, char** endptr);
QOSA_EXTERN char*              qosa_strcat(char* dest, const char* src);
QOSA_EXTERN char*              qosa_strncat(char* dest, const char* src, qosa_size_t size);
QOSA_EXTERN int                qosa_toupper(int c);
QOSA_EXTERN int                qosa_tolower(int c);
QOSA_EXTERN void               qosa_touppercase(char* str);
QOSA_EXTERN char*              qosa_strtok(char* s, const char* delim);
QOSA_EXTERN int                qosa_strspn(char* s, const char* accept);
QOSA_EXTERN double             qosa_atof(const char* nptr);
QOSA_EXTERN int                qosa_atoi(const char* nptr);
QOSA_EXTERN int                qosa_strcspn(const char* s, const char* reject);
QOSA_EXTERN int                qosa_get_errno(void);
QOSA_EXTERN char*              qosa_strpbrk(const char* cs, const char* ct);
QOSA_EXTERN int                qosa_system(char* command);
QOSA_EXTERN void*              qosa_memchr(void* s, int c, qosa_size_t size);
QOSA_EXTERN long               qosa_atol(const char* nptr);
QOSA_EXTERN int                qosa_rand(void);
QOSA_EXTERN int                qosa_stricmpl(char* str1, char* str2, int maxlength);
QOSA_EXTERN char*              qosa_strnstr(const char* s1, const char* s2, size_t n);

#ifndef CONFIG_QOSA_HAVE_MALLOC
QOSA_EXTERN void* qosa_malloc(qosa_size_t nbytes);
#else
#include <stdlib.h>
// larson.li add 2024-08-27 15:53:21
// The malloc function is quite special, and many mod platforms have their own memory leak analysis tools.
// If function packages are used to replace them, it will cause the platform's tools to be unable to analyze
// the specific application function
#define qosa_malloc(nbytes)                                                                                                                                    \
    ({                                                                                                                                                         \
        void* ptr = malloc(nbytes);                                                                                                                            \
        if (ptr != NULL)                                                                                                                                       \
        {                                                                                                                                                      \
            qosa_memset(ptr, 0, nbytes);                                                                                                                       \
        }                                                                                                                                                      \
        ptr;                                                                                                                                                   \
    })
#endif

#define QOSA_ASSERT(a)                       qosa_assert(__func__, __LINE__, a)

/** Purpose: Known 'whole' and 'one part' of the whole, calculate the address of the whole based on the address of the part. */
#define list_container_of(ptr, type, member) ((type*)((char*)(ptr) - (unsigned long)(&((type*)0)->member)))

#if !defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) && !defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) && !defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
/* the type we use for storing a single boolean bit */
#undef BIT
#ifdef _MSC_VER
typedef bool bit;
#define BIT(x) bool x
#else
typedef unsigned int bit;
#define BIT(x) bit x : 1
#endif
#endif /* CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC */

#ifndef QOSA_LITTLE_ENDIAN
#define QOSA_LITTLE_ENDIAN 1234
#endif

#ifndef QOSA_BIG_ENDIAN
#define QOSA_BIG_ENDIAN 4321
#endif

#ifndef QOSA_BYTE_ORDER
#define QOSA_BYTE_ORDER QOSA_LITTLE_ENDIAN
#endif

#if QOSA_BYTE_ORDER == QOSA_BIG_ENDIAN
#define UNIRTOS_BIG_ENDIAN 1
#endif
#endif /* __QOSA_DEF_H__ */
