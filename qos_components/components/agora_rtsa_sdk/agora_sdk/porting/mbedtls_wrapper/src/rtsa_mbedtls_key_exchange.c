#if defined(RTSA_CRYPTO_WRAPPER_ENABLE_KEY_EXCHANGE)
#include <string.h>

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdh.h"
#include "rtsa_crypto_base.h"
#include "rtsa_crypto_compat.h"
#include "rtsa_crypto_key_exchange.h"
#include "rtsa_crypto_rng.h"

struct rtsa_crypto_ecdh_ctx {
  rtsa_crypto_rng_adapter_t *rng;
  mbedtls_ecdh_context ecdh;
};

rtsa_crypto_ecdh_ctx_t *rtsa_crypto_ecdh_ctx_create(void)
{
  rtsa_crypto_ecdh_ctx_t *ctx = RTSA_CRYPTO_WRAPPER_MALLOC(sizeof(*ctx));

  if (ctx == NULL) {
    return NULL;
  }
  memset(ctx, 0, sizeof(*ctx));
  ctx->rng = rtsa_crypto_rng_adapter_create();
  if (ctx->rng == NULL) {
    RTSA_CRYPTO_WRAPPER_FREE(ctx);
    return NULL;
  }
  mbedtls_ecdh_init(&ctx->ecdh);
  if (rtsa_crypto_rng_adapter_seed(ctx->rng) != 0) {
    rtsa_crypto_ecdh_ctx_destroy(ctx);
    return NULL;
  }
  return ctx;
}

void rtsa_crypto_ecdh_ctx_destroy(rtsa_crypto_ecdh_ctx_t *ctx)
{
  if (ctx != NULL) {
    mbedtls_ecdh_free(&ctx->ecdh);
    rtsa_crypto_rng_adapter_destroy(ctx->rng);
    RTSA_CRYPTO_WRAPPER_FREE(ctx);
  }
}

int rtsa_crypto_ecdh_ctx_setup_p256(rtsa_crypto_ecdh_ctx_t *ctx)
{
  return mbedtls_ecdh_setup(&ctx->ecdh, MBEDTLS_ECP_DP_SECP256R1);
}

int rtsa_crypto_ecdh_ctx_make_public_params(rtsa_crypto_ecdh_ctx_t *ctx, unsigned char *output, size_t output_len, size_t *olen)
{
  return mbedtls_ecdh_make_params(&ctx->ecdh, olen, output, output_len, mbedtls_ctr_drbg_random,
                                  rtsa_crypto_rng_adapter_ctr_drbg(ctx->rng));
}

int rtsa_crypto_ecdh_ctx_read_public(rtsa_crypto_ecdh_ctx_t *ctx, const unsigned char *peer_key, size_t peer_key_len)
{
  return mbedtls_ecdh_read_public(&ctx->ecdh, peer_key, peer_key_len);
}

int rtsa_crypto_ecdh_ctx_calc_secret(rtsa_crypto_ecdh_ctx_t *ctx, unsigned char *output, size_t output_len, size_t *olen)
{
  return mbedtls_ecdh_calc_secret(&ctx->ecdh, olen, output, output_len, mbedtls_ctr_drbg_random,
                                  rtsa_crypto_rng_adapter_ctr_drbg(ctx->rng));
}
#endif /* RTSA_CRYPTO_WRAPPER_ENABLE_KEY_EXCHANGE */
