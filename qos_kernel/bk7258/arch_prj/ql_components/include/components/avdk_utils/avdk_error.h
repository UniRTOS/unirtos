#pragma once

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

typedef int avdk_err_t;

#define AVDK_ERR_OK              0   /* No Error */
#define AVDK_ERR_GENERIC        -1  /* Generic Error */
#define AVDK_ERR_INVAL          -2  /* Invalid argument */
#define AVDK_ERR_NOMEM          -3  /* Out of memory */
#define AVDK_ERR_BUSY           -4  /* Device or resource busy */
#define AVDK_ERR_NODEV          -5  /* No such device */
#define AVDK_ERR_TIMEOUT        -6  /* operation timeout */
#define AVDK_ERR_HWERROR        -7  /* hardware error */
#define AVDK_ERR_RDYDONE        -8  /* already down */
#define AVDK_ERR_SHUTDOWN       -9  /* shut down */
#define AVDK_ERR_UNKNOWN        -10  /* unknown */
#define AVDK_ERR_UNSUPPORTED    -11  /* unsupported */
#define AVDK_ERR_NO_RESOURCE    -12  /* no resource */
#define AVDK_ERR_IO             -13  /* I/O error */

// End of file / end of stream
// Used by container parsers and data sources to indicate clean EOF (not an error).
#define AVDK_ERR_EOF             1   /* end of file */


#define AVDK_ERR_INVAL_NULL_TEXT     "invalid argument: NULL pointer"
#define AVDK_ERR_NOMEM_TEXT     "out of memory"
#define AVDK_ERR_BUSY_TEXT      "device or resource busy"
#define AVDK_ERR_NODEV_TEXT     "no such device"
#define AVDK_ERR_TIMEOUT_TEXT   "operation timeout"
#define AVDK_ERR_HWERROR_TEXT   "hardware error"
#define AVDK_ERR_RDYDONE_TEXT   "already down"
#define AVDK_ERR_SHUTDOWN_TEXT  "shut down"
#define AVDK_ERR_UNKNOWN_TEXT   "unknown"
#define AVDK_ERR_UNSUPPORTED_TEXT   "unsupported"
#define AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT   "unsupported function"
#define AVDK_ERR_NO_RESOURCE_TEXT    "no resource"
#define AVDK_ERR_IO_TEXT             "I/O error"

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */