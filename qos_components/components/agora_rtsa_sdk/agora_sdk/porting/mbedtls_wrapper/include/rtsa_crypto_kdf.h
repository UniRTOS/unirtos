#ifndef __RTSA_CRYPTO_KDF_H__
#define __RTSA_CRYPTO_KDF_H__

#include <stddef.h>
#include <stdint.h>
#include "rtsa_crypto_md.h"

/**
 * @brief Derive key material with PBKDF2-HMAC.
 * @param[in] ctx Configured digest context in HMAC mode.
 * @param[in] password Password buffer.
 * @param[in] pwd_len Password length in bytes.
 * @param[in] salt Salt buffer.
 * @param[in] salt_len Salt length in bytes.
 * @param[in] iteration_count PBKDF2 iteration count.
 * @param[in] key_length Requested output length in bytes.
 * @param[out] output Derived key buffer.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_pbkdf2_hmac(rtsa_crypto_md_ctx_t *ctx, const unsigned char *password, size_t pwd_len,
                            const unsigned char *salt, size_t salt_len, unsigned int iteration_count,
                            uint32_t key_length, unsigned char *output);

/**
 * @brief Derive key material with HKDF.
 * @param[in] md_info Digest metadata used by HKDF.
 * @param[in] secret Input keying material.
 * @param[in] secret_len Input keying material length in bytes.
 * @param[in] salt Optional salt buffer.
 * @param[in] salt_len Salt length in bytes.
 * @param[in] info Optional context/application info buffer.
 * @param[in] info_len Context/application info length in bytes.
 * @param[out] output Derived key buffer.
 * @param[in] output_len Requested output length in bytes.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_hkdf(const rtsa_crypto_md_info_t *md_info, const unsigned char *secret, size_t secret_len,
                     const unsigned char *salt, size_t salt_len, const unsigned char *info, size_t info_len,
                     unsigned char *output, size_t output_len);

#endif
