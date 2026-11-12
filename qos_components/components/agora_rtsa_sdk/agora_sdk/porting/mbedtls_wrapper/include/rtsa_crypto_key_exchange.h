#ifndef __RTSA_CRYPTO_KEY_EXCHANGE_H__
#define __RTSA_CRYPTO_KEY_EXCHANGE_H__

#include <stddef.h>
#include <stdint.h>

/** Opaque ECDH context used for key agreement. */
typedef struct rtsa_crypto_ecdh_ctx rtsa_crypto_ecdh_ctx_t;

/**
 * @brief Create an ECDH context.
 * @return:
 * - != NULL: ECDH context handle.
 * - = NULL: Allocation failed.
 */
rtsa_crypto_ecdh_ctx_t *rtsa_crypto_ecdh_ctx_create(void);

/**
 * @brief Destroy an ECDH context.
 * @param[in] ctx ECDH context handle.
 */
void rtsa_crypto_ecdh_ctx_destroy(rtsa_crypto_ecdh_ctx_t *ctx);

/**
 * @brief Configure the context for the P-256 (secp256r1) curve.
 * @param[in] ctx ECDH context handle.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_ecdh_ctx_setup_p256(rtsa_crypto_ecdh_ctx_t *ctx);

/**
 * @brief Generate and export local public parameters.
 * @param[in] ctx ECDH context handle.
 * @param[out] output Buffer to receive the public parameters.
 * @param[in] output_len Size of the output buffer in bytes.
 * @param[out] olen Actual number of bytes written.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_ecdh_ctx_make_public_params(rtsa_crypto_ecdh_ctx_t *ctx, unsigned char *output, size_t output_len, size_t *olen);

/**
 * @brief Import the peer's public key.
 * @param[in] ctx ECDH context handle.
 * @param[in] peer_key Peer public key buffer.
 * @param[in] peer_key_len Peer public key length in bytes.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_ecdh_ctx_read_public(rtsa_crypto_ecdh_ctx_t *ctx, const unsigned char *peer_key, size_t peer_key_len);

/**
 * @brief Calculate the shared secret.
 * @param[in] ctx ECDH context handle.
 * @param[out] output Buffer to receive the shared secret.
 * @param[in] output_len Size of the output buffer in bytes.
 * @param[out] olen Actual number of bytes written.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_ecdh_ctx_calc_secret(rtsa_crypto_ecdh_ctx_t *ctx, unsigned char *output, size_t output_len, size_t *olen);

#endif
