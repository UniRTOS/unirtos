#ifndef __RTSA_CRYPTO_CIPHER_H__
#define __RTSA_CRYPTO_CIPHER_H__

#include <stddef.h>
#include <stdint.h>

/** Opaque cipher metadata handle. */
typedef struct rtsa_crypto_cipher_info rtsa_crypto_cipher_info_t;

/** Opaque cipher operation context. */
typedef struct rtsa_crypto_cipher_ctx rtsa_crypto_cipher_ctx_t;

/** Cipher mode identifiers exposed to RTSA code. */
typedef enum {
  RTSA_CRYPTO_CIPHER_MODE_NONE = 0,
  RTSA_CRYPTO_CIPHER_MODE_ECB = 1,
  RTSA_CRYPTO_CIPHER_MODE_CBC = 2,
  RTSA_CRYPTO_CIPHER_MODE_CFB = 3,
  RTSA_CRYPTO_CIPHER_MODE_OFB = 4,
  RTSA_CRYPTO_CIPHER_MODE_CTR = 5,
  RTSA_CRYPTO_CIPHER_MODE_GCM = 6,
  RTSA_CRYPTO_CIPHER_MODE_CCM = 7,
  RTSA_CRYPTO_CIPHER_MODE_XTS = 8,
} rtsa_crypto_cipher_mode_t;

/** Cipher direction used when installing a key. */
typedef enum {
  RTSA_CRYPTO_CIPHER_OP_ENCRYPT = 0,
  RTSA_CRYPTO_CIPHER_OP_DECRYPT = 1,
} rtsa_crypto_cipher_operation_t;

/**
 * @brief Resolve cipher metadata by name.
 * @param[in] cipher_name Cipher algorithm name string.
 * @return:
 * - != NULL: Cipher metadata handle.
 * - = NULL: Unsupported cipher.
 */
const rtsa_crypto_cipher_info_t *rtsa_crypto_cipher_info_from_string(const char *cipher_name);

/**
 * @brief Query key length from cipher metadata.
 * @param[in] info Cipher metadata handle.
 * @return:
 * - > 0: Key length in bits.
 * - = 0: Invalid metadata.
 */
size_t rtsa_crypto_cipher_info_get_key_bitlen(const rtsa_crypto_cipher_info_t *info);

/**
 * @brief Query IV size from cipher metadata.
 * @param[in] info Cipher metadata handle.
 * @return:
 * - > 0: IV size in bytes.
 * - = 0: Invalid metadata or no IV required.
 */
size_t rtsa_crypto_cipher_info_get_iv_size(const rtsa_crypto_cipher_info_t *info);

/**
 * @brief Query block size from cipher metadata.
 * @param[in] info Cipher metadata handle.
 * @return:
 * - > 0: Block size in bytes.
 * - = 0: Invalid metadata.
 */
size_t rtsa_crypto_cipher_info_get_block_size(const rtsa_crypto_cipher_info_t *info);

/**
 * @brief Query cipher mode from metadata.
 * @param[in] info Cipher metadata handle.
 * @return:
 * - != RTSA_CRYPTO_CIPHER_MODE_NONE: Cipher mode.
 * - = RTSA_CRYPTO_CIPHER_MODE_NONE: Invalid metadata.
 */
rtsa_crypto_cipher_mode_t rtsa_crypto_cipher_info_get_mode(const rtsa_crypto_cipher_info_t *info);

/**
 * @brief Create a cipher context.
 * @return:
 * - != NULL: Cipher context handle.
 * - = NULL: Allocation failed.
 */
rtsa_crypto_cipher_ctx_t *rtsa_crypto_cipher_ctx_create(void);

/**
 * @brief Destroy a cipher context.
 * @param[in] ctx Cipher context handle.
 */
void rtsa_crypto_cipher_ctx_destroy(rtsa_crypto_cipher_ctx_t *ctx);

/**
 * @brief Bind cipher metadata to the context.
 * @param[in] ctx Cipher context handle.
 * @param[in] info Cipher metadata handle.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_cipher_ctx_setup(rtsa_crypto_cipher_ctx_t *ctx, const rtsa_crypto_cipher_info_t *info);

/**
 * @brief Install the working key.
 * @param[in] ctx Cipher context handle.
 * @param[in] key Key buffer.
 * @param[in] key_bitlen Key length in bits.
 * @param[in] operation Encrypt or decrypt direction.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_cipher_ctx_setkey(rtsa_crypto_cipher_ctx_t *ctx, const unsigned char *key, unsigned int key_bitlen,
                                  rtsa_crypto_cipher_operation_t operation);

/**
 * @brief Set the IV/nonce for the current operation.
 * @param[in] ctx Cipher context handle.
 * @param[in] iv IV buffer.
 * @param[in] iv_len IV length in bytes.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_cipher_ctx_set_iv(rtsa_crypto_cipher_ctx_t *ctx, const unsigned char *iv, size_t iv_len);

/**
 * @brief Reset per-message state before a new encrypt/decrypt operation.
 * @param[in] ctx Cipher context handle.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_cipher_ctx_reset(rtsa_crypto_cipher_ctx_t *ctx);

/**
 * @brief Provide additional authenticated data for AEAD modes.
 * @param[in] ctx Cipher context handle.
 * @param[in] ad AAD buffer.
 * @param[in] ad_len AAD length in bytes.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_cipher_ctx_update_ad(rtsa_crypto_cipher_ctx_t *ctx, const unsigned char *ad, size_t ad_len);

/**
 * @brief Process cipher payload bytes.
 * @param[in] ctx Cipher context handle.
 * @param[in] input Input buffer.
 * @param[in] ilen Input buffer length in bytes.
 * @param[out] output Output buffer.
 * @param[out] olen Number of bytes written.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_cipher_ctx_update(rtsa_crypto_cipher_ctx_t *ctx, const unsigned char *input, size_t ilen,
                           unsigned char *output, size_t *olen);

/**
 * @brief Finalize the current cipher operation.
 * @param[in] ctx Cipher context handle.
 * @param[out] output Output buffer for remaining bytes.
 * @param[out] olen Number of bytes written.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_cipher_ctx_finish(rtsa_crypto_cipher_ctx_t *ctx, unsigned char *output, size_t *olen);

/**
 * @brief Write the authentication tag after encryption (AEAD modes).
 * @param[in] ctx Cipher context handle.
 * @param[out] tag Buffer to receive the tag.
 * @param[in] tag_len Requested tag length in bytes.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_cipher_ctx_write_tag(rtsa_crypto_cipher_ctx_t *ctx, unsigned char *tag, size_t tag_len);

/**
 * @brief Verify the authentication tag after decryption (AEAD modes).
 * @param[in] ctx Cipher context handle.
 * @param[in] tag Expected tag buffer.
 * @param[in] tag_len Tag length in bytes.
 * @return:
 * - = 0: Tag valid.
 * - != 0: Tag mismatch or backend error code.
 */
int rtsa_crypto_cipher_ctx_check_tag(rtsa_crypto_cipher_ctx_t *ctx, const unsigned char *tag, size_t tag_len);

#endif
