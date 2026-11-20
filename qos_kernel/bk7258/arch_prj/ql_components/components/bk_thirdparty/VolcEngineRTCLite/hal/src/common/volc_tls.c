#include "volc_tls.h"

#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>

#include "volc_memory.h"
#include "volc_type.h"

typedef struct {
    mbedtls_ssl_context ssl_ctx;
    mbedtls_ssl_config ssl_ctx_config;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt cacert;
} volc_tls_mbedtls_ctx_t;

uint32_t volc_tls_create(volc_tls_t* tls)
{
    uint32_t ret = VOLC_STATUS_SUCCESS;
    volc_tls_mbedtls_ctx_t* ctx = NULL;

    VOLC_CHK(tls != NULL, VOLC_STATUS_NULL_ARG);
    ctx = (volc_tls_mbedtls_ctx_t*) volc_malloc(sizeof(volc_tls_mbedtls_ctx_t));
    VOLC_CHK(ctx != NULL, VOLC_STATUS_NOT_ENOUGH_MEMORY);
    *tls = (volc_tls_t)ctx;

    mbedtls_ssl_init(&ctx->ssl_ctx);
    mbedtls_ssl_config_init(&ctx->ssl_ctx_config);
    mbedtls_x509_crt_init(&ctx->cacert);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
    mbedtls_entropy_init(&ctx->entropy);
    VOLC_CHK(mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy, NULL, 0) == 0, VOLC_STATUS_CREATE_SSL_FAILED);
    return ret;
err_out_label:
    if (ctx) {
        volc_free(ctx);
    }
    return ret;
}

uint32_t volc_tls_destroy(volc_tls_t tls)
{
    volc_tls_mbedtls_ctx_t* ctx = (volc_tls_mbedtls_ctx_t*) tls;
    if (NULL == tls) {
        return VOLC_STATUS_SUCCESS;
    }
    while (mbedtls_ssl_close_notify(&ctx->ssl_ctx) == MBEDTLS_ERR_SSL_WANT_WRITE) {
        // keep flushing outgoing buffer until nothing left
    }
    mbedtls_ssl_free(&ctx->ssl_ctx);
    mbedtls_ssl_config_free(&ctx->ssl_ctx_config);
    mbedtls_x509_crt_free(&ctx->cacert);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_entropy_free(&ctx->entropy);
    volc_free(ctx);
    return VOLC_STATUS_SUCCESS;
}

int volc_tls_start(volc_tls_t tls, bool is_server, const char* host, volc_tls_send_callback send_callback, volc_tls_recv_callback recv_callback, void* custom_data)
{
    int ret = 0;
    volc_tls_mbedtls_ctx_t* ctx = (volc_tls_mbedtls_ctx_t*)tls;
    if (NULL == tls || NULL == host || NULL == send_callback || NULL == recv_callback) {
        return -1;
    }

    mbedtls_ssl_config_defaults(&(ctx->ssl_ctx_config), is_server ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_ca_chain(&ctx->ssl_ctx_config, &(ctx->cacert), NULL);
    mbedtls_ssl_conf_authmode(&ctx->ssl_ctx_config, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&ctx->ssl_ctx_config, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    mbedtls_ssl_setup(&ctx->ssl_ctx, &ctx->ssl_ctx_config);

    mbedtls_ssl_set_hostname( &ctx->ssl_ctx, host);
    mbedtls_ssl_set_bio(&ctx->ssl_ctx, custom_data,
                    (mbedtls_ssl_send_t *)send_callback,
                    (mbedtls_ssl_recv_t *)recv_callback, NULL);
    /* init and send handshake */
    ret = mbedtls_ssl_handshake(&ctx->ssl_ctx);

// err_out_label:
    return ret;
}

int volc_tls_read(volc_tls_t tls, unsigned char* buf, size_t len) {
    volc_tls_mbedtls_ctx_t* ctx = (volc_tls_mbedtls_ctx_t*)tls;
    if (NULL == tls) {
        return -1;
    }
    return mbedtls_ssl_read(&ctx->ssl_ctx, buf, len);
}

int volc_tls_write(volc_tls_t tls, const unsigned char* buf, size_t len) {
    volc_tls_mbedtls_ctx_t* ctx = (volc_tls_mbedtls_ctx_t*)tls;
    if (NULL == tls) {
        return -1;
    }
    return mbedtls_ssl_write(&ctx->ssl_ctx, buf, len);
}

bool volc_tls_is_handshake_over(volc_tls_t tls) {
    bool ret = false;
    volc_tls_mbedtls_ctx_t* ctx = (volc_tls_mbedtls_ctx_t*)tls;
    if (NULL == tls) {
        return false;
    }
#if MBEDTLS_VERSION_NUMBER >= 0x03050200
    ret = mbedtls_ssl_is_handshake_over(&ctx->ssl_ctx);
#else
    ret = (ctx->ssl_ctx.state == MBEDTLS_SSL_HANDSHAKE_OVER);
#endif
    return ret;
}

size_t volc_tls_get_bytes_avail(volc_tls_t tls) {
    volc_tls_mbedtls_ctx_t* ctx = (volc_tls_mbedtls_ctx_t*)tls;
    if (NULL == tls) {
        return 0;
    }
    return mbedtls_ssl_get_bytes_avail(&ctx->ssl_ctx);
}
