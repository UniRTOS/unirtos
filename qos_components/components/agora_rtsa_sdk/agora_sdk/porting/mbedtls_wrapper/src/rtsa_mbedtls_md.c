#if defined(RTSA_CRYPTO_WRAPPER_ENABLE_MD)
#include "mbedtls/md.h"
#include "rtsa_crypto_md.h"
#include "rtsa_crypto_compat.h"

struct rtsa_crypto_md_info {
  const mbedtls_md_info_t *info;
};

struct rtsa_crypto_md_ctx {
  mbedtls_md_context_t ctx;
};

static mbedtls_md_type_t rtsa_crypto_md_type_to_mbedtls(rtsa_crypto_md_type_t type)
{
  switch (type) {
  case RTSA_CRYPTO_MD_NONE:
    return MBEDTLS_MD_NONE;
  case RTSA_CRYPTO_MD_MD5:
    return MBEDTLS_MD_MD5;
  case RTSA_CRYPTO_MD_SHA1:
    return MBEDTLS_MD_SHA1;
  case RTSA_CRYPTO_MD_SHA224:
    return MBEDTLS_MD_SHA224;
  case RTSA_CRYPTO_MD_SHA256:
    return MBEDTLS_MD_SHA256;
  case RTSA_CRYPTO_MD_SHA384:
    return MBEDTLS_MD_SHA384;
  case RTSA_CRYPTO_MD_SHA512:
    return MBEDTLS_MD_SHA512;
  case RTSA_CRYPTO_MD_RIPEMD160:
    return MBEDTLS_MD_RIPEMD160;
  default:
    return MBEDTLS_MD_NONE;
  }
}

const rtsa_crypto_md_info_t *rtsa_crypto_md_info_from_type(rtsa_crypto_md_type_t type)
{
  static struct rtsa_crypto_md_info holder;
  mbedtls_md_type_t mbedtls_type = rtsa_crypto_md_type_to_mbedtls(type);

  holder.info = mbedtls_md_info_from_type(mbedtls_type);
  return holder.info ? &holder : NULL;
}

size_t rtsa_crypto_md_get_size(const rtsa_crypto_md_info_t *info)
{
  if (info == NULL || info->info == NULL) {
    return 0;
  }
  return mbedtls_md_get_size(info->info);
}

const void *rtsa_crypto_md_info_native(const rtsa_crypto_md_info_t *info)
{
  if (info == NULL) {
    return NULL;
  }
  return info->info;
}

void *rtsa_crypto_md_ctx_native(rtsa_crypto_md_ctx_t *ctx)
{
  if (ctx == NULL) {
    return NULL;
  }
  return &ctx->ctx;
}

rtsa_crypto_md_ctx_t *rtsa_crypto_md_ctx_create(void)
{
  rtsa_crypto_md_ctx_t *ctx = RTSA_CRYPTO_WRAPPER_MALLOC(sizeof(*ctx));

  if (ctx == NULL) {
    return NULL;
  }
  mbedtls_md_init(&ctx->ctx);
  return ctx;
}

void rtsa_crypto_md_ctx_destroy(rtsa_crypto_md_ctx_t *ctx)
{
  if (ctx != NULL) {
    mbedtls_md_free(&ctx->ctx);
    RTSA_CRYPTO_WRAPPER_FREE(ctx);
  }
}

int rtsa_crypto_md_ctx_setup(rtsa_crypto_md_ctx_t *ctx, const rtsa_crypto_md_info_t *info, int hmac)
{
  return mbedtls_md_setup(&ctx->ctx, info->info, hmac);
}

size_t rtsa_crypto_md_ctx_get_size(const rtsa_crypto_md_ctx_t *ctx)
{
  const mbedtls_md_info_t *info = mbedtls_md_info_from_ctx(&ctx->ctx);

  if (info == NULL) {
    return 0;
  }
  return mbedtls_md_get_size(info);
}

int rtsa_crypto_md_ctx_starts(rtsa_crypto_md_ctx_t *ctx)
{
  return mbedtls_md_starts(&ctx->ctx);
}

int rtsa_crypto_md_ctx_update(rtsa_crypto_md_ctx_t *ctx, const unsigned char *input, size_t ilen)
{
  return mbedtls_md_update(&ctx->ctx, input, ilen);
}

int rtsa_crypto_md_ctx_finish(rtsa_crypto_md_ctx_t *ctx, unsigned char *output)
{
  return mbedtls_md_finish(&ctx->ctx, output);
}
#endif /* RTSA_CRYPTO_WRAPPER_ENABLE_MD */
