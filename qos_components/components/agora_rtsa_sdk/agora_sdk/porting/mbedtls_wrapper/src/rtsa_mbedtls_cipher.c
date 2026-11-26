#if defined(RTSA_CRYPTO_WRAPPER_ENABLE_CIPHER)
#include <string.h>

#include "mbedtls/cipher.h"
#include "mbedtls/version.h"
#include "rtsa_crypto_cipher.h"
#include "rtsa_crypto_compat.h"

#ifndef MBEDTLS_PRIVATE
#define MBEDTLS_PRIVATE(name) name
#endif

struct rtsa_crypto_cipher_info {
  const mbedtls_cipher_info_t *info;
};

struct rtsa_crypto_cipher_ctx {
  mbedtls_cipher_context_t ctx;
};

static struct rtsa_crypto_cipher_info make_info(const mbedtls_cipher_info_t *info)
{
  struct rtsa_crypto_cipher_info wrapper;
  wrapper.info = info;
  return wrapper;
}

const rtsa_crypto_cipher_info_t *rtsa_crypto_cipher_info_from_string(const char *cipher_name)
{
  static struct rtsa_crypto_cipher_info holder;
  holder = make_info(mbedtls_cipher_info_from_string(cipher_name));
  return holder.info ? &holder : NULL;
}

size_t rtsa_crypto_cipher_info_get_key_bitlen(const rtsa_crypto_cipher_info_t *info)
{
  if (info == NULL || info->info == NULL) {
    return 0;
  }
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03030000)
  return mbedtls_cipher_info_get_key_bitlen(info->info);
#else
  return info->info->MBEDTLS_PRIVATE(key_bitlen);
#endif
}

size_t rtsa_crypto_cipher_info_get_iv_size(const rtsa_crypto_cipher_info_t *info)
{
  if (info == NULL || info->info == NULL) {
    return 0;
  }
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03030000)
  return mbedtls_cipher_info_get_iv_size(info->info);
#else
  return info->info->MBEDTLS_PRIVATE(iv_size);
#endif
}

size_t rtsa_crypto_cipher_info_get_block_size(const rtsa_crypto_cipher_info_t *info)
{
  if (info == NULL || info->info == NULL) {
    return 0;
  }
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03030000)
  return mbedtls_cipher_info_get_block_size(info->info);
#else
  return info->info->MBEDTLS_PRIVATE(block_size);
#endif
}

rtsa_crypto_cipher_mode_t rtsa_crypto_cipher_info_get_mode(const rtsa_crypto_cipher_info_t *info)
{
  mbedtls_cipher_mode_t mode;

  if (info == NULL || info->info == NULL) {
    return RTSA_CRYPTO_CIPHER_MODE_NONE;
  }
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03030000)
  mode = mbedtls_cipher_info_get_mode(info->info);
#else
  mode = info->info->MBEDTLS_PRIVATE(mode);
#endif
  switch (mode) {
  case MBEDTLS_MODE_ECB:
    return RTSA_CRYPTO_CIPHER_MODE_ECB;
  case MBEDTLS_MODE_CBC:
    return RTSA_CRYPTO_CIPHER_MODE_CBC;
  case MBEDTLS_MODE_CFB:
    return RTSA_CRYPTO_CIPHER_MODE_CFB;
  case MBEDTLS_MODE_OFB:
    return RTSA_CRYPTO_CIPHER_MODE_OFB;
  case MBEDTLS_MODE_CTR:
    return RTSA_CRYPTO_CIPHER_MODE_CTR;
  case MBEDTLS_MODE_GCM:
    return RTSA_CRYPTO_CIPHER_MODE_GCM;
  case MBEDTLS_MODE_CCM:
    return RTSA_CRYPTO_CIPHER_MODE_CCM;
  case MBEDTLS_MODE_XTS:
    return RTSA_CRYPTO_CIPHER_MODE_XTS;
  default:
    return RTSA_CRYPTO_CIPHER_MODE_NONE;
  }
}

rtsa_crypto_cipher_ctx_t *rtsa_crypto_cipher_ctx_create(void)
{
  rtsa_crypto_cipher_ctx_t *ctx = RTSA_CRYPTO_WRAPPER_MALLOC(sizeof(*ctx));

  if (ctx == NULL) {
    return NULL;
  }
  memset(ctx, 0, sizeof(*ctx));
  mbedtls_cipher_init(&ctx->ctx);
  return ctx;
}

void rtsa_crypto_cipher_ctx_destroy(rtsa_crypto_cipher_ctx_t *ctx)
{
  if (ctx != NULL) {
    mbedtls_cipher_free(&ctx->ctx);
    RTSA_CRYPTO_WRAPPER_FREE(ctx);
  }
}

int rtsa_crypto_cipher_ctx_setup(rtsa_crypto_cipher_ctx_t *ctx, const rtsa_crypto_cipher_info_t *info)
{
  return mbedtls_cipher_setup(&ctx->ctx, info->info);
}

int rtsa_crypto_cipher_ctx_setkey(rtsa_crypto_cipher_ctx_t *ctx, const unsigned char *key, unsigned int key_bitlen,
                           rtsa_crypto_cipher_operation_t operation)
{
  return mbedtls_cipher_setkey(&ctx->ctx, key, key_bitlen,
                               operation == RTSA_CRYPTO_CIPHER_OP_ENCRYPT ? MBEDTLS_ENCRYPT : MBEDTLS_DECRYPT);
}

int rtsa_crypto_cipher_ctx_set_iv(rtsa_crypto_cipher_ctx_t *ctx, const unsigned char *iv, size_t iv_len)
{
  return mbedtls_cipher_set_iv(&ctx->ctx, iv, iv_len);
}

int rtsa_crypto_cipher_ctx_reset(rtsa_crypto_cipher_ctx_t *ctx)
{
  return mbedtls_cipher_reset(&ctx->ctx);
}

int rtsa_crypto_cipher_ctx_update_ad(rtsa_crypto_cipher_ctx_t *ctx, const unsigned char *ad, size_t ad_len)
{
  return mbedtls_cipher_update_ad(&ctx->ctx, ad, ad_len);
}

int rtsa_crypto_cipher_ctx_update(rtsa_crypto_cipher_ctx_t *ctx, const unsigned char *input, size_t ilen,
                           unsigned char *output, size_t *olen)
{
  return mbedtls_cipher_update(&ctx->ctx, input, ilen, output, olen);
}

int rtsa_crypto_cipher_ctx_finish(rtsa_crypto_cipher_ctx_t *ctx, unsigned char *output, size_t *olen)
{
  return mbedtls_cipher_finish(&ctx->ctx, output, olen);
}

int rtsa_crypto_cipher_ctx_write_tag(rtsa_crypto_cipher_ctx_t *ctx, unsigned char *tag, size_t tag_len)
{
  return mbedtls_cipher_write_tag(&ctx->ctx, tag, tag_len);
}

int rtsa_crypto_cipher_ctx_check_tag(rtsa_crypto_cipher_ctx_t *ctx, const unsigned char *tag, size_t tag_len)
{
  return mbedtls_cipher_check_tag(&ctx->ctx, tag, tag_len);
}
#endif /* RTSA_CRYPTO_WRAPPER_ENABLE_CIPHER */
