#ifndef __RTSA_CRYPTO_BASE_H__
#define __RTSA_CRYPTO_BASE_H__

#include <stddef.h>

/**
 * @brief Convert a backend-specific error code into a readable string.
 * @param[in] ret Backend error code.
 * @param[in] buf Destination buffer.
 * @param[in] len Destination buffer size.
 * @return:
 * - = 0: Success, after writing a best-effort error string into buf.
 */
int rtsa_crypto_strerror(int ret, char *buf, size_t len);

#endif
