#ifndef __RTSA_CRYPTO_TLS_H__
#define __RTSA_CRYPTO_TLS_H__

#include <stddef.h>
#include <stdint.h>

/** Opaque TLS session context. */
typedef struct rtsa_crypto_tls_ctx rtsa_crypto_tls_ctx_t;
/** Placeholder opaque certificate type for future expansion. */
typedef struct rtsa_crypto_x509_crt rtsa_crypto_x509_crt_t;

/** Transport send callback supplied by the RTSA caller. */
typedef int (*rtsa_crypto_tls_send_cb)(void *ctx, const unsigned char *buf, size_t len);
/** Transport receive callback supplied by the RTSA caller. */
typedef int (*rtsa_crypto_tls_recv_cb)(void *ctx, unsigned char *buf, size_t len);
/** Debug callback supplied by the RTSA caller. */
typedef void (*rtsa_crypto_tls_debug_cb)(void *ctx, int level, const char *file, int line, const char *str);

/**
 * @brief Create a TLS client session context.
 * @return:
 * - != NULL: TLS context handle.
 * - = NULL: Allocation failed.
 */
rtsa_crypto_tls_ctx_t *rtsa_crypto_tls_ctx_create(void);

/**
 * @brief Destroy a TLS client session context.
 * @param[in] ctx TLS context handle.
 */
void rtsa_crypto_tls_ctx_destroy(rtsa_crypto_tls_ctx_t *ctx);

/**
 * @brief Set the hostname for server certificate verification.
 * @param[in] ctx TLS context handle.
 * @param[in] hostname Server hostname string.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_tls_ctx_set_hostname(rtsa_crypto_tls_ctx_t *ctx, const char *hostname);

/**
 * @brief Configure transport I/O callbacks for the TLS session.
 * @param[in] ctx TLS context handle.
 * @param[in] io_ctx User-supplied I/O context passed to callbacks.
 * @param[in] send_cb Send callback function.
 * @param[in] recv_cb Receive callback function.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_tls_ctx_set_bio(rtsa_crypto_tls_ctx_t *ctx, void *io_ctx,
                                rtsa_crypto_tls_send_cb send_cb, rtsa_crypto_tls_recv_cb recv_cb);

/**
 * @brief Run the TLS handshake.
 * @param[in] ctx TLS context handle.
 * @return:
 * - = 0: Handshake complete.
 * - != 0: Backend error code or WANT_READ/WANT_WRITE.
 */
int rtsa_crypto_tls_ctx_handshake(rtsa_crypto_tls_ctx_t *ctx);

/**
 * @brief Read decrypted application data from the TLS session.
 * @param[in] ctx TLS context handle.
 * @param[out] buf Destination buffer.
 * @param[in] len Maximum number of bytes to read.
 * @return:
 * - > 0: Number of bytes read.
 * - = 0: Connection closed.
 * - < 0: Backend error code or WANT_READ/WANT_WRITE.
 */
int rtsa_crypto_tls_ctx_read(rtsa_crypto_tls_ctx_t *ctx, unsigned char *buf, size_t len);

/**
 * @brief Write application data to the TLS session.
 * @param[in] ctx TLS context handle.
 * @param[in] buf Data buffer to send.
 * @param[in] len Number of bytes to write.
 * @return:
 * - > 0: Number of bytes written.
 * - < 0: Backend error code or WANT_READ/WANT_WRITE.
 */
int rtsa_crypto_tls_ctx_write(rtsa_crypto_tls_ctx_t *ctx, const unsigned char *buf, size_t len);

/**
 * @brief Reset the TLS session for reuse.
 * @param[in] ctx TLS context handle.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_tls_ctx_session_reset(rtsa_crypto_tls_ctx_t *ctx);

/**
 * @brief Send a close-notify alert to the peer.
 * @param[in] ctx TLS context handle.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code or WANT_WRITE.
 */
int rtsa_crypto_tls_ctx_close_notify(rtsa_crypto_tls_ctx_t *ctx);

/**
 * @brief Check whether the TLS handshake is complete.
 * @param[in] ctx TLS context handle.
 * @return:
 * - = 1: Handshake is over.
 * - = 0: Handshake still in progress.
 */
int rtsa_crypto_tls_ctx_is_handshake_over(const rtsa_crypto_tls_ctx_t *ctx);

/**
 * @brief Check whether there is pending output data to flush.
 * @param[in] ctx TLS context handle.
 * @return:
 * - = 1: Pending output exists.
 * - = 0: No pending output.
 */
int rtsa_crypto_tls_ctx_has_pending_output(const rtsa_crypto_tls_ctx_t *ctx);

/**
 * @brief Get the certificate verification result after handshake.
 * @param[in] ctx TLS context handle.
 * @return:
 * - = 0: Verification passed.
 * - != 0: Verification flags indicating failure reasons.
 */
int rtsa_crypto_tls_ctx_get_verify_result(const rtsa_crypto_tls_ctx_t *ctx);

/**
 * @brief Load a CA certificate chain blob into the session context.
 * @param[in] ctx TLS context handle.
 * @param[in] cert Certificate data buffer (PEM or DER).
 * @param[in] cert_len Certificate data length in bytes.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_tls_ctx_load_cert(rtsa_crypto_tls_ctx_t *ctx, const unsigned char *cert, size_t cert_len);

/**
 * @brief Apply default client-side TLS configuration to the session.
 * @param[in] ctx TLS context handle.
 * @param[in] authmode Authentication mode (NONE or OPTIONAL).
 * @param[in] debug_cb Debug callback function or NULL.
 * @param[in] debug_ctx User context passed to debug callback.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_tls_ctx_configure_defaults(rtsa_crypto_tls_ctx_t *ctx, int authmode,
                                           rtsa_crypto_tls_debug_cb debug_cb, void *debug_ctx);

/**
 * @brief Set the TLS debug output threshold level.
 * @param[in] threshold Debug verbosity level.
 */
void rtsa_crypto_tls_debug_set_threshold(int threshold);

/**
 * @brief Format X.509 verification flags into a human-readable string.
 * @param[in] buf Destination buffer.
 * @param[in] size Destination buffer size.
 * @param[in] prefix Prefix string for each line.
 * @param[in] flags Verification flags to format.
 */
void rtsa_crypto_tls_x509_verify_info(char *buf, size_t size, const char *prefix, uint32_t flags);

/**
 * @brief Convert a backend-native TLS error code into the wrapper-defined value.
 * @param[in] ret Backend-native TLS error code.
 * @return Wrapper-defined TLS error code or the original value if no mapping exists.
 */
int rtsa_crypto_tls_error_from_mbedtls(int ret);

/**
 * @brief Convert a wrapper-defined TLS error code into the backend-native value.
 * @param[in] ret Wrapper-defined TLS error code.
 * @return Backend-native TLS error code or the original value if no mapping exists.
 */
int rtsa_crypto_tls_error_to_mbedtls(int ret);

/* Auth mode values understood by rtsa_crypto_tls_ctx_configure_defaults(). */
#define RTSA_CRYPTO_TLS_AUTHMODE_NONE 0
#define RTSA_CRYPTO_TLS_AUTHMODE_OPTIONAL 1

/*
 * Wrapper-defined TLS status codes.
 * These values intentionally do not match backend-native error numbers and
 * use a small decimal-only range instead of the mbedtls hex error pattern.
 */
#define RTSA_CRYPTO_TLS_ERR_BASE (-200000)
#define RTSA_CRYPTO_TLS_ERR_WANT_READ (RTSA_CRYPTO_TLS_ERR_BASE - 1)
#define RTSA_CRYPTO_TLS_ERR_WANT_WRITE (RTSA_CRYPTO_TLS_ERR_BASE - 2)
#define RTSA_CRYPTO_TLS_ERR_TIMEOUT (RTSA_CRYPTO_TLS_ERR_BASE - 3)
#define RTSA_CRYPTO_TLS_ERR_PEER_CLOSE_NOTIFY (RTSA_CRYPTO_TLS_ERR_BASE - 4)
#define RTSA_CRYPTO_TLS_ERR_NET_CONN_RESET (RTSA_CRYPTO_TLS_ERR_BASE - 5)
#define RTSA_CRYPTO_TLS_ERR_NET_SEND_FAILED (RTSA_CRYPTO_TLS_ERR_BASE - 6)

#endif
