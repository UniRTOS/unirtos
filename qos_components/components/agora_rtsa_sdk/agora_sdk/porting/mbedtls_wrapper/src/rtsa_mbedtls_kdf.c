#if defined(RTSA_CRYPTO_WRAPPER_ENABLE_PBKDF2) || defined(RTSA_CRYPTO_WRAPPER_ENABLE_HKDF)
#include "mbedtls/hkdf.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"
#include "rtsa_crypto_kdf.h"
#include "rtsa_crypto_md.h"

#if defined(RTSA_CRYPTO_WRAPPER_ENABLE_PBKDF2)
int rtsa_crypto_pbkdf2_hmac(rtsa_crypto_md_ctx_t *ctx, const unsigned char *password, size_t pwd_len,
                            const unsigned char *salt, size_t salt_len, unsigned int iteration_count,
                            uint32_t key_length, unsigned char *output)
{
  mbedtls_md_context_t *native_ctx = (mbedtls_md_context_t *)rtsa_crypto_md_ctx_native(ctx);

  if (native_ctx == NULL) {
    return -1;
  }

  return mbedtls_pkcs5_pbkdf2_hmac(native_ctx, password, pwd_len, salt, salt_len,
                                   iteration_count, key_length, output);
}
#endif

#if defined(RTSA_CRYPTO_WRAPPER_ENABLE_HKDF)
int rtsa_crypto_hkdf(const rtsa_crypto_md_info_t *md_info, const unsigned char *secret, size_t secret_len,
                     const unsigned char *salt, size_t salt_len, const unsigned char *info, size_t info_len,
                     unsigned char *output, size_t output_len)
{
  const mbedtls_md_info_t *native_md_info =
      (const mbedtls_md_info_t *)rtsa_crypto_md_info_native(md_info);

  if (native_md_info == NULL) {
    return -1;
  }

  return mbedtls_hkdf(native_md_info, secret, secret_len, salt, salt_len, info, info_len, output, output_len);
}
#endif
#endif /* RTSA_CRYPTO_WRAPPER_ENABLE_PBKDF2 || RTSA_CRYPTO_WRAPPER_ENABLE_HKDF */
