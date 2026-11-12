#if defined(RTSA_CRYPTO_WRAPPER_ENABLE_TLS)
#include <string.h>

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "rtsa_crypto_base.h"
#include "rtsa_crypto_compat.h"
#include "rtsa_crypto_rng.h"
#include "rtsa_crypto_tls.h"

struct rtsa_crypto_tls_ctx {
  rtsa_crypto_rng_adapter_t *rng;
  mbedtls_ssl_config conf;
  mbedtls_ssl_context ssl;
  mbedtls_x509_crt cacert;
  void *io_ctx;
  rtsa_crypto_tls_send_cb send_cb;
  rtsa_crypto_tls_recv_cb recv_cb;
};

int rtsa_crypto_tls_error_from_mbedtls(int ret)
{
  switch (ret) {
  case MBEDTLS_ERR_SSL_WANT_READ:
    return RTSA_CRYPTO_TLS_ERR_WANT_READ;
  case MBEDTLS_ERR_SSL_WANT_WRITE:
    return RTSA_CRYPTO_TLS_ERR_WANT_WRITE;
  case MBEDTLS_ERR_SSL_TIMEOUT:
    return RTSA_CRYPTO_TLS_ERR_TIMEOUT;
  case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
    return RTSA_CRYPTO_TLS_ERR_PEER_CLOSE_NOTIFY;
  case MBEDTLS_ERR_NET_CONN_RESET:
    return RTSA_CRYPTO_TLS_ERR_NET_CONN_RESET;
  case MBEDTLS_ERR_NET_SEND_FAILED:
    return RTSA_CRYPTO_TLS_ERR_NET_SEND_FAILED;
  default:
    return ret;
  }
}

int rtsa_crypto_tls_error_to_mbedtls(int ret)
{
  switch (ret) {
  case RTSA_CRYPTO_TLS_ERR_WANT_READ:
    return MBEDTLS_ERR_SSL_WANT_READ;
  case RTSA_CRYPTO_TLS_ERR_WANT_WRITE:
    return MBEDTLS_ERR_SSL_WANT_WRITE;
  case RTSA_CRYPTO_TLS_ERR_TIMEOUT:
    return MBEDTLS_ERR_SSL_TIMEOUT;
  case RTSA_CRYPTO_TLS_ERR_PEER_CLOSE_NOTIFY:
    return MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY;
  case RTSA_CRYPTO_TLS_ERR_NET_CONN_RESET:
    return MBEDTLS_ERR_NET_CONN_RESET;
  case RTSA_CRYPTO_TLS_ERR_NET_SEND_FAILED:
    return MBEDTLS_ERR_NET_SEND_FAILED;
  default:
    return ret;
  }
}

static int rtsa_crypto_tls_send_adapter(void *ctx, const unsigned char *buf, size_t len)
{
  rtsa_crypto_tls_ctx_t *tls = (rtsa_crypto_tls_ctx_t *)ctx;
  return rtsa_crypto_tls_error_to_mbedtls(tls->send_cb(tls->io_ctx, buf, len));
}

static int rtsa_crypto_tls_recv_adapter(void *ctx, unsigned char *buf, size_t len)
{
  rtsa_crypto_tls_ctx_t *tls = (rtsa_crypto_tls_ctx_t *)ctx;
  return rtsa_crypto_tls_error_to_mbedtls(tls->recv_cb(tls->io_ctx, buf, len));
}

rtsa_crypto_tls_ctx_t *rtsa_crypto_tls_ctx_create(void)
{
  rtsa_crypto_tls_ctx_t *ctx = RTSA_CRYPTO_WRAPPER_MALLOC(sizeof(*ctx));

  if (ctx == NULL) {
    return NULL;
  }
  memset(ctx, 0, sizeof(*ctx));
  ctx->rng = rtsa_crypto_rng_adapter_create();
  if (ctx->rng == NULL) {
    RTSA_CRYPTO_WRAPPER_FREE(ctx);
    return NULL;
  }
  mbedtls_ssl_config_init(&ctx->conf);
  mbedtls_ssl_init(&ctx->ssl);
  mbedtls_x509_crt_init(&ctx->cacert);
  if (rtsa_crypto_rng_adapter_seed(ctx->rng) != 0) {
    rtsa_crypto_tls_ctx_destroy(ctx);
    return NULL;
  }
  return ctx;
}

void rtsa_crypto_tls_ctx_destroy(rtsa_crypto_tls_ctx_t *ctx)
{
  if (ctx != NULL) {
    mbedtls_ssl_close_notify(&ctx->ssl);
    mbedtls_x509_crt_free(&ctx->cacert);
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    rtsa_crypto_rng_adapter_destroy(ctx->rng);
    RTSA_CRYPTO_WRAPPER_FREE(ctx);
  }
}

int rtsa_crypto_tls_ctx_set_hostname(rtsa_crypto_tls_ctx_t *ctx, const char *hostname)
{
  return rtsa_crypto_tls_error_from_mbedtls(mbedtls_ssl_set_hostname(&ctx->ssl, hostname));
}

int rtsa_crypto_tls_ctx_set_bio(rtsa_crypto_tls_ctx_t *ctx, void *io_ctx, rtsa_crypto_tls_send_cb send_cb, rtsa_crypto_tls_recv_cb recv_cb)
{
  ctx->io_ctx = io_ctx;
  ctx->send_cb = send_cb;
  ctx->recv_cb = recv_cb;
  mbedtls_ssl_set_bio(&ctx->ssl, ctx, rtsa_crypto_tls_send_adapter, rtsa_crypto_tls_recv_adapter, NULL);
  return 0;
}

int rtsa_crypto_tls_ctx_handshake(rtsa_crypto_tls_ctx_t *ctx)
{
  return rtsa_crypto_tls_error_from_mbedtls(mbedtls_ssl_handshake(&ctx->ssl));
}

int rtsa_crypto_tls_ctx_read(rtsa_crypto_tls_ctx_t *ctx, unsigned char *buf, size_t len)
{
  return rtsa_crypto_tls_error_from_mbedtls(mbedtls_ssl_read(&ctx->ssl, buf, len));
}

int rtsa_crypto_tls_ctx_write(rtsa_crypto_tls_ctx_t *ctx, const unsigned char *buf, size_t len)
{
  return rtsa_crypto_tls_error_from_mbedtls(mbedtls_ssl_write(&ctx->ssl, buf, len));
}

int rtsa_crypto_tls_ctx_session_reset(rtsa_crypto_tls_ctx_t *ctx)
{
  return rtsa_crypto_tls_error_from_mbedtls(mbedtls_ssl_session_reset(&ctx->ssl));
}

int rtsa_crypto_tls_ctx_close_notify(rtsa_crypto_tls_ctx_t *ctx)
{
  return rtsa_crypto_tls_error_from_mbedtls(mbedtls_ssl_close_notify(&ctx->ssl));
}

int rtsa_crypto_tls_ctx_is_handshake_over(const rtsa_crypto_tls_ctx_t *ctx)
{
  return mbedtls_ssl_is_handshake_over((mbedtls_ssl_context *)&ctx->ssl);
}

int rtsa_crypto_tls_ctx_has_pending_output(const rtsa_crypto_tls_ctx_t *ctx)
{
#ifndef MBEDTLS_PRIVATE
#define MBEDTLS_PRIVATE(name) name
#endif
  return ctx->ssl.MBEDTLS_PRIVATE(out_left) != 0;
}

int rtsa_crypto_tls_ctx_get_verify_result(const rtsa_crypto_tls_ctx_t *ctx)
{
  return (int)mbedtls_ssl_get_verify_result(&ctx->ssl);
}

int rtsa_crypto_tls_ctx_load_cert(rtsa_crypto_tls_ctx_t *ctx, const unsigned char *cert, size_t cert_len)
{
  return rtsa_crypto_tls_error_from_mbedtls(mbedtls_x509_crt_parse(&ctx->cacert, cert, cert_len));
}

int rtsa_crypto_tls_ctx_configure_defaults(rtsa_crypto_tls_ctx_t *ctx, int authmode, rtsa_crypto_tls_debug_cb debug_cb, void *debug_ctx)
{
  int ret = mbedtls_ssl_config_defaults(&ctx->conf, MBEDTLS_SSL_IS_CLIENT,
                                        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0) {
    return rtsa_crypto_tls_error_from_mbedtls(ret);
  }
  mbedtls_ssl_conf_authmode(&ctx->conf,
                            authmode == RTSA_CRYPTO_TLS_AUTHMODE_OPTIONAL ? MBEDTLS_SSL_VERIFY_OPTIONAL : MBEDTLS_SSL_VERIFY_NONE);
  mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->cacert, NULL);
  mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, rtsa_crypto_rng_adapter_ctr_drbg(ctx->rng));
  mbedtls_ssl_conf_dbg(&ctx->conf, debug_cb, debug_ctx);
  return rtsa_crypto_tls_error_from_mbedtls(mbedtls_ssl_setup(&ctx->ssl, &ctx->conf));
}

void rtsa_crypto_tls_debug_set_threshold(int threshold)
{
#if defined(MBEDTLS_DEBUG_C)
  mbedtls_debug_set_threshold(threshold);
#else
  (void)threshold;
#endif
}

void rtsa_crypto_tls_x509_verify_info(char *buf, size_t size, const char *prefix, uint32_t flags)
{
  mbedtls_x509_crt_verify_info(buf, size, prefix, flags);
}
#endif /* RTSA_CRYPTO_WRAPPER_ENABLE_TLS */
