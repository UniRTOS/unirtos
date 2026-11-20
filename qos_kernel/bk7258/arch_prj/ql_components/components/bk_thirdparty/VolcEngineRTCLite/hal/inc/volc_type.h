/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_TYPE_H__
#define __HAL_VOLC_TYPE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#if defined(__BUILDING_BYTE_RTC_SDK__)
#define __byte_rtc_api__ __declspec(dllexport)
#else
#define __byte_rtc_api__ __declspec(dllimport)
#endif
#else
#define __byte_rtc_api__ __attribute__((visibility("default")))
#endif

#define VOLC_MAX_UINT8 ((uint8_t) 0xff)
#define VOLC_MAX_INT8  ((int8_t) 0x7f)
#define VOLC_MIN_INT8  ((int8_t) 0x80)

#define VOLC_MAX_UINT16 ((uint16_t) 0xffff)
#define VOLC_MAX_INT16  ((int16_t) 0x7fff)
#define VOLC_MIN_INT16  ((int16_t) 0x8000)

#define VOLC_MAX_UINT32 ((uint32_t) 0xffffffff)
#define VOLC_MAX_INT32  ((int32_t) 0x7fffffff)
#define VOLC_MIN_INT32  ((int32_t) 0x80000000)

#define VOLC_MAX_UINT64 ((uint64_t) 0xffffffffffffffff)
#define VOLC_MAX_INT64  ((int64_t) 0x7fffffffffffffff)
#define VOLC_MIN_INT64  ((int64_t) 0x8000000000000000)

////////////////////////////////////////////////////
// Status definitions
////////////////////////////////////////////////////
#define VOLC_STATUS uint32_t

#define VOLC_STATUS_SUCCESS ((VOLC_STATUS) 0x00000000)
#define VOLC_STATUS_FAILED(x)    (((VOLC_STATUS)(x)) != VOLC_STATUS_SUCCESS)
#define VOLC_STATUS_SUCCEEDED(x) (!VOLC_STATUS_FAILED(x))
#define VOLC_STATUS_BASE                                     0x00000000
#define VOLC_STATUS_NULL_ARG                                 VOLC_STATUS_BASE + 0x00000001
#define VOLC_STATUS_INVALID_ARG                              VOLC_STATUS_BASE + 0x00000002
#define VOLC_STATUS_INVALID_ARG_LEN                          VOLC_STATUS_BASE + 0x00000003
#define VOLC_STATUS_NOT_ENOUGH_MEMORY                        VOLC_STATUS_BASE + 0x00000004
#define VOLC_STATUS_BUFFER_TOO_SMALL                         VOLC_STATUS_BASE + 0x00000005
#define VOLC_STATUS_UNEXPECTED_EOF                           VOLC_STATUS_BASE + 0x00000006
#define VOLC_STATUS_FORMAT_ERROR                             VOLC_STATUS_BASE + 0x00000007
#define VOLC_STATUS_INVALID_HANDLE_ERROR                     VOLC_STATUS_BASE + 0x00000008
#define VOLC_STATUS_OPEN_FILE_FAILED                         VOLC_STATUS_BASE + 0x00000009
#define VOLC_STATUS_READ_FILE_FAILED                         VOLC_STATUS_BASE + 0x0000000a
#define VOLC_STATUS_WRITE_TO_FILE_FAILED                     VOLC_STATUS_BASE + 0x0000000b
#define VOLC_STATUS_INTERNAL_ERROR                           VOLC_STATUS_BASE + 0x0000000c
#define VOLC_STATUS_INVALID_OPERATION                        VOLC_STATUS_BASE + 0x0000000d
#define VOLC_STATUS_NOT_IMPLEMENTED                          VOLC_STATUS_BASE + 0x0000000e
#define VOLC_STATUS_OPERATION_TIMED_OUT                      VOLC_STATUS_BASE + 0x0000000f
#define VOLC_STATUS_NOT_FOUND                                VOLC_STATUS_BASE + 0x00000010
#define VOLC_STATUS_CREATE_THREAD_FAILED                     VOLC_STATUS_BASE + 0x00000011
#define VOLC_STATUS_THREAD_NOT_ENOUGH_RESOURCES              VOLC_STATUS_BASE + 0x00000012
#define VOLC_STATUS_THREAD_INVALID_ARG                       VOLC_STATUS_BASE + 0x00000013
#define VOLC_STATUS_THREAD_PERMISSIONS                       VOLC_STATUS_BASE + 0x00000014
#define VOLC_STATUS_THREAD_DEADLOCKED                        VOLC_STATUS_BASE + 0x00000015
#define VOLC_STATUS_THREAD_DOES_NOT_EXIST                    VOLC_STATUS_BASE + 0x00000016
#define VOLC_STATUS_JOIN_THREAD_FAILED                       VOLC_STATUS_BASE + 0x00000017
#define VOLC_STATUS_WAIT_FAILED                              VOLC_STATUS_BASE + 0x00000018
#define VOLC_STATUS_CANCEL_THREAD_FAILED                     VOLC_STATUS_BASE + 0x00000019
#define VOLC_STATUS_THREAD_IS_NOT_JOINABLE                   VOLC_STATUS_BASE + 0x0000001a
#define VOLC_STATUS_DETACH_THREAD_FAILED                     VOLC_STATUS_BASE + 0x0000001b
#define VOLC_STATUS_THREAD_ATTR_INIT_FAILED                  VOLC_STATUS_BASE + 0x0000001c
#define VOLC_STATUS_THREAD_ATTR_SET_STACK_SIZE_FAILED        VOLC_STATUS_BASE + 0x0000001d
#define VOLC_STATUS_MEMORY_NOT_FREED                         VOLC_STATUS_BASE + 0x0000001e
#define VOLC_STATUS_USER_CANCELED                            VOLC_STATUS_BASE + 0x0000001f
#define VOLC_STATUS_INVALID_LENGTH                           VOLC_STATUS_BASE + 0x00000020
#define VOLC_STATUS_MD5_CHECK_FAILED                         VOLC_STATUS_BASE + 0x00000021
#define VOLC_STATUS_CHECK_FAILED                             VOLC_STATUS_BASE + 0x00000022
#define VOLC_STATUS_KCP_SEND_FAILED                          VOLC_STATUS_BASE + 0x00000023

#define VOLC_STATUS_ICE_MESSAGE_MESSAGE_ID_ERROR             VOLC_STATUS_BASE + 0x00000024
#define VOLC_STATUS_ICE_MESSAGE_MESSAGE_INVALID_DATA         VOLC_STATUS_BASE + 0x00000025
#define VOLC_STATUS_ICE_MESSAGE_MESSAGE_TOO_BIG              VOLC_STATUS_BASE + 0x00000026
#define VOLC_STATUS_ICE_MESSAGE_MESSAGE_KEY_ERROR            VOLC_STATUS_BASE + 0x00000027
#define VOLC_STATUS_ICE_MESSAGE_MESSAGE_UNZIP_FAILED         VOLC_STATUS_BASE + 0x00000028

#define VOLC_STATUS_LICENSE_EXPIRED                          VOLC_STATUS_BASE + 0x00000029
#define VOLC_STATUS_LICENSE_ILLEGAL                          VOLC_STATUS_BASE + 0x00000030
#define VOLC_STATUS_LICENSE_UNMATCH                          VOLC_STATUS_BASE + 0x00000031
#define VOLC_STATUS_LICENSE_APPID_UNMATCH                    VOLC_STATUS_BASE + 0x00000032
#define VOLC_STATUS_LICENSE_DEVICETPYE_UNMATCH               VOLC_STATUS_BASE + 0x00000033
#define VOLC_STATUS_LICENSE_FINGERPRINT_UNMATCH              VOLC_STATUS_BASE + 0x00000034
#define VOLC_STATUS_LICENSE_DEVICEID_UNMATCH                 VOLC_STATUS_BASE + 0x00000035
#define VOLC_STATUS_MESSAGE_TOO_BIG                          VOLC_STATUS_BASE + 0x00000100

#define VOLC_STATUS_FAILURE                                  VOLC_STATUS_BASE + 0x00000101
#define VOLC_STATUS_SOCKET_BIND_FAILED                       VOLC_STATUS_BASE + 0x00000102
#define VOLC_STATUS_SOCKET_GETSOCKETNAME_FAILED              VOLC_STATUS_BASE + 0x00000103
#define VOLC_STATUS_SOCKET_CONNECT_FAILED                    VOLC_STATUS_BASE + 0x00000104
#define VOLC_STATUS_SOCKET_LISTEN_FAILED                     VOLC_STATUS_BASE + 0x00000105

/**
 * Error values
 */
#define VOLC_STATUS_UTILS_BASE                            0x40000000
#define VOLC_STATUS_INVALID_BASE64_ENCODE                 VOLC_STATUS_UTILS_BASE + 0x00000001
#define VOLC_STATUS_INVALID_BASE                          VOLC_STATUS_UTILS_BASE + 0x00000002
#define VOLC_STATUS_INVALID_DIGIT                         VOLC_STATUS_UTILS_BASE + 0x00000003
#define VOLC_STATUS_INT_OVERFLOW                          VOLC_STATUS_UTILS_BASE + 0x00000004
#define VOLC_STATUS_EMPTY_STRING                          VOLC_STATUS_UTILS_BASE + 0x00000005
#define VOLC_STATUS_DIRECTORY_OPEN_FAILED                 VOLC_STATUS_UTILS_BASE + 0x00000006
#define VOLC_STATUS_PATH_TOO_LONG                         VOLC_STATUS_UTILS_BASE + 0x00000007
#define VOLC_STATUS_UNKNOWN_DIR_ENTRY_TYPE                VOLC_STATUS_UTILS_BASE + 0x00000008
#define VOLC_STATUS_REMOVE_DIRECTORY_FAILED               VOLC_STATUS_UTILS_BASE + 0x00000009
#define VOLC_STATUS_REMOVE_FILE_FAILED                    VOLC_STATUS_UTILS_BASE + 0x0000000a
#define VOLC_STATUS_REMOVE_LINK_FAILED                    VOLC_STATUS_UTILS_BASE + 0x0000000b
#define VOLC_STATUS_DIRECTORY_ACCESS_DENIED               VOLC_STATUS_UTILS_BASE + 0x0000000c
#define VOLC_STATUS_DIRECTORY_MISSING_PATH                VOLC_STATUS_UTILS_BASE + 0x0000000d
#define VOLC_STATUS_DIRECTORY_ENTRY_STAT_ERROR            VOLC_STATUS_UTILS_BASE + 0x0000000e
#define VOLC_STATUS_STRFTIME_FALIED                       VOLC_STATUS_UTILS_BASE + 0x0000000f
#define VOLC_STATUS_MAX_TIMESTAMP_FORMAT_STR_LEN_EXCEEDED VOLC_STATUS_UTILS_BASE + 0x00000010
#define VOLC_STATUS_UTIL_MAX_TAG_COUNT                    VOLC_STATUS_UTILS_BASE + 0x00000011
#define VOLC_STATUS_UTIL_INVALID_TAG_VERSION              VOLC_STATUS_UTILS_BASE + 0x00000012
#define VOLC_STATUS_UTIL_TAGS_COUNT_NON_ZERO_TAGS_NULL    VOLC_STATUS_UTILS_BASE + 0x00000013
#define VOLC_STATUS_UTIL_INVALID_TAG_NAME_LEN             VOLC_STATUS_UTILS_BASE + 0x00000014
#define VOLC_STATUS_UTIL_INVALID_TAG_VALUE_LEN            VOLC_STATUS_UTILS_BASE + 0x00000015
#define VOLC_STATUS_EXPONENTIAL_BACKOFF_INVALID_STATE     VOLC_STATUS_UTILS_BASE + 0x0000002a
#define VOLC_STATUS_EXPONENTIAL_BACKOFF_RETRIES_EXHAUSTED VOLC_STATUS_UTILS_BASE + 0x0000002b

/**
 * Timer queue error values starting from 0x41100000
 */
#define VOLC_STATUS_EVLOOP_BASE                  VOLC_STATUS_UTILS_BASE + 0x01100000
#define VOLC_STATUS_EVLOOP_TIMER_STOP_SCHEDULING VOLC_STATUS_EVLOOP_BASE + 0x00000001

#define VOLC_STATUS_EVLOOP_PERFORM_NEED_RETRY    VOLC_STATUS_EVLOOP_BASE + 0x00000002
#define VOLC_STATUS_EVLOOP_PERFORM_FAILED        VOLC_STATUS_EVLOOP_BASE + 0x00000003


/////////////////////////////////////////////////////
/// Networking related status codes
/////////////////////////////////////////////////////

/*! \addtogroup NetworkingStatusCodes
 * WEBRTC Networking related codes. Values are derived from STATUS_NETWORKING_BASE (0x58000000)
 *  @{
 */
#define VOLC_STATUS_NETWORKING_BASE                     0x58000000
#define VOLC_STATUS_GET_LOCAL_IP_ADDRESSES_FAILED       VOLC_STATUS_NETWORKING_BASE + 0x00000016
#define VOLC_STATUS_CREATE_UDP_SOCKET_FAILED            VOLC_STATUS_NETWORKING_BASE + 0x00000017
#define VOLC_STATUS_BINDING_SOCKET_FAILED               VOLC_STATUS_NETWORKING_BASE + 0x00000018
#define VOLC_STATUS_GET_PORT_NUMBER_FAILED              VOLC_STATUS_NETWORKING_BASE + 0x00000019
#define VOLC_STATUS_SEND_DATA_FAILED                    VOLC_STATUS_NETWORKING_BASE + 0x0000001a
#define VOLC_STATUS_RESOLVE_HOSTNAME_FAILED             VOLC_STATUS_NETWORKING_BASE + 0x0000001b
#define VOLC_STATUS_HOSTNAME_NOT_FOUND                  VOLC_STATUS_NETWORKING_BASE + 0x0000001c
// #define VOLC_STATUS_SOCKET_CONNECT_FAILED               VOLC_STATUS_NETWORKING_BASE + 0x0000001d
#define VOLC_STATUS_CREATE_SSL_FAILED                   VOLC_STATUS_NETWORKING_BASE + 0x0000001e
#define VOLC_STATUS_SSL_CONNECTION_FAILED               VOLC_STATUS_NETWORKING_BASE + 0x0000001f
#define VOLC_STATUS_SECURE_SOCKET_READ_FAILED           VOLC_STATUS_NETWORKING_BASE + 0x00000020
#define VOLC_STATUS_SOCKET_CONNECTION_NOT_READY_TO_SEND VOLC_STATUS_NETWORKING_BASE + 0x00000021
#define VOLC_STATUS_SOCKET_CONNECTION_CLOSED_ALREADY    VOLC_STATUS_NETWORKING_BASE + 0x00000022
#define VOLC_STATUS_SOCKET_SET_SEND_BUFFER_SIZE_FAILED  VOLC_STATUS_NETWORKING_BASE + 0x00000023
#define VOLC_STATUS_GET_SOCKET_FLAG_FAILED              VOLC_STATUS_NETWORKING_BASE + 0x00000024
#define VOLC_STATUS_SET_SOCKET_FLAG_FAILED              VOLC_STATUS_NETWORKING_BASE + 0x00000025
#define VOLC_STATUS_CLOSE_SOCKET_FAILED                 VOLC_STATUS_NETWORKING_BASE + 0x00000026
/*!@} */

/////////////////////////////////////////////////////
/// DTLS related status codes
/////////////////////////////////////////////////////

/*! \addtogroup DTLSStatusCodes
 * WEBRTC DTLS related codes. Values are derived from VOLC_STATUS_DTLS_BASE (0x59000000)
 *  @{
 */
#define VOLC_STATUS_DTLS_BASE                                  0x59000000
#define VOLC_STATUS_CERTIFICATE_GENERATION_FAILED              VOLC_STATUS_DTLS_BASE + 0x00000001
#define VOLC_STATUS_SSL_CTX_CREATION_FAILED                    VOLC_STATUS_DTLS_BASE + 0x00000002
#define VOLC_STATUS_SSL_REMOTE_CERTIFICATE_VERIFICATION_FAILED VOLC_STATUS_DTLS_BASE + 0x00000003
#define VOLC_STATUS_SSL_PACKET_BEFORE_DTLS_READY               VOLC_STATUS_DTLS_BASE + 0x00000004
#define VOLC_STATUS_SSL_UNKNOWN_SRTP_PROFILE                   VOLC_STATUS_DTLS_BASE + 0x00000005
#define VOLC_STATUS_SSL_INVALID_CERTIFICATE_BITS               VOLC_STATUS_DTLS_BASE + 0x00000006
#define VOLC_STATUS_SSL_SET_HOSTNAME_FAILED                    VOLC_STATUS_DTLS_BASE + 0x00000007

#define VOLC_VALID_CHAR_SET_FOR_JSON "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

#ifndef VOLC_UNUSED_PARAM
#define VOLC_UNUSED_PARAM(expr)                                                                                                                           \
    do {                                                                                                                                             \
        (void) (expr);                                                                                                                               \
    } while (0)
#endif

#ifndef VOLC_ARRAY_SIZE
#define VOLC_ARRAY_SIZE(array) (sizeof(array) / sizeof *(array))
#endif

extern void volc_free(void*);
#ifndef VOLC_SAFE_MEMFREE
#define VOLC_SAFE_MEMFREE(p)                                                                                                                              \
    do {                                                                                                                                             \
        if (p) {                                                                                                                                     \
            volc_free(p);                                                                                                                              \
            (p) = NULL;                                                                                                                              \
        }                                                                                                                                            \
    } while (0)
#endif

#ifndef VOLC_MIN
#define VOLC_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef VOLC_MAX
#define VOLC_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define VOLC_CHK(condition, err)                                                                                                                     \
    do {                                                                                                                                             \
        if (!(condition)) {                                                                                                                          \
            ret = (err);                                                                                                                             \
            goto err_out_label;                                                                                                                      \
        }                                                                                                                                            \
    } while (0)

#define VOLC_CHK_STATUS(condition)                                                                                                                        \
    do {                                                                                                                                             \
        VOLC_STATUS __status = condition;                                                                                                                 \
        if (VOLC_STATUS_FAILED(__status)) {                                                                                                               \
            ret = (__status);                                                                                                                  \
            goto err_out_label;                                                                                                                            \
        }                                                                                                                                            \
    } while (0)


#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_ERRNO_H__ */
