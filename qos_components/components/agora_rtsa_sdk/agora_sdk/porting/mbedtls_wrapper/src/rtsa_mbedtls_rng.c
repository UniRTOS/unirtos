#if defined(RTSA_CRYPTO_WRAPPER_ENABLE_RNG)
#include "api/aosl_mm.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "rtsa_crypto_rng.h"

#if defined(RTSA_CRYPTO_WRAPPER_USE_RTSA_RNG)
#include "utils_crypto_rng.h"
#endif

struct rtsa_crypto_rng_adapter {
#if defined(RTSA_CRYPTO_WRAPPER_USE_RTSA_RNG)
  void *rtsa_rng;
#endif
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
};

rtsa_crypto_rng_adapter_t *rtsa_crypto_rng_adapter_create(void)
{
  rtsa_crypto_rng_adapter_t *ctx = aosl_malloc(sizeof(*ctx));

  if (ctx == NULL) {
    return NULL;
  }

  mbedtls_entropy_init(&ctx->entropy);
  mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
#if defined(RTSA_CRYPTO_WRAPPER_USE_RTSA_RNG)
  ctx->rtsa_rng = NULL;
#endif
  return ctx;
}

void rtsa_crypto_rng_adapter_destroy(rtsa_crypto_rng_adapter_t *ctx)
{
  if (ctx == NULL) {
    return;
  }

  mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
  mbedtls_entropy_free(&ctx->entropy);
#if defined(RTSA_CRYPTO_WRAPPER_USE_RTSA_RNG)
  rtsa_internal_rng_ctx_destroy((rtsa_internal_rng_ctx_t *)ctx->rtsa_rng);
  ctx->rtsa_rng = NULL;
#endif
  aosl_free(ctx);
}

int rtsa_crypto_rng_adapter_seed(rtsa_crypto_rng_adapter_t *ctx)
{
#if defined(RTSA_CRYPTO_WRAPPER_USE_RTSA_RNG)
  ctx->rtsa_rng = rtsa_internal_rng_ctx_create();
  if (ctx->rtsa_rng == NULL) {
    return -1;
  }

  return mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, rtsa_internal_rng_random, ctx->rtsa_rng, NULL, 0);
#else
  return mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy, NULL, 0);
#endif
}

void *rtsa_crypto_rng_adapter_ctr_drbg(rtsa_crypto_rng_adapter_t *ctx)
{
  return &ctx->ctr_drbg;
}
#endif /* RTSA_CRYPTO_WRAPPER_ENABLE_RNG */
