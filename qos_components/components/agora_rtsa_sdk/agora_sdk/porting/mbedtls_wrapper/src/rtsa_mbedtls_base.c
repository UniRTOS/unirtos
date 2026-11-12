#include "mbedtls/error.h"
#include "rtsa_crypto_base.h"

#if defined(RTSA_CRYPTO_WRAPPER_ENABLE_TLS)
#include "rtsa_crypto_tls.h"
#endif

int rtsa_crypto_strerror(int ret, char *buf, size_t len)
{
#if defined(RTSA_CRYPTO_WRAPPER_ENABLE_TLS)
  ret = rtsa_crypto_tls_error_to_mbedtls(ret);
#endif
  mbedtls_strerror(ret, buf, len);
  return 0;
}
