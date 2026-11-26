#ifndef __RTSA_CRYPTO_MD_H__
#define __RTSA_CRYPTO_MD_H__

#include <stddef.h>
#include <stdint.h>

/** Opaque digest context handle. */
typedef struct rtsa_crypto_md_ctx rtsa_crypto_md_ctx_t;
/** Opaque digest metadata handle. */
typedef struct rtsa_crypto_md_info rtsa_crypto_md_info_t;

/** Digest algorithm identifiers used by the RTSA crypto wrapper, not mbedtls numeric values. */
typedef enum {
  RTSA_CRYPTO_MD_NONE = 0,
  RTSA_CRYPTO_MD_MD5,
  RTSA_CRYPTO_MD_SHA1,
  RTSA_CRYPTO_MD_SHA224,
  RTSA_CRYPTO_MD_SHA256,
  RTSA_CRYPTO_MD_SHA384,
  RTSA_CRYPTO_MD_SHA512,
  RTSA_CRYPTO_MD_RIPEMD160,
} rtsa_crypto_md_type_t;

/**
 * @brief Resolve digest metadata by algorithm type.
 * @param[in] type Digest algorithm identifier.
 * @return:
 * - != NULL: Digest metadata handle.
 * - = NULL: Unsupported digest or resolve failed.
 */
const rtsa_crypto_md_info_t *rtsa_crypto_md_info_from_type(rtsa_crypto_md_type_t type);

/**
 * @brief Query digest output size from metadata.
 * @param[in] info Digest metadata handle.
 * @return:
 * - > 0: Digest size in bytes.
 * - = 0: Invalid metadata.
 */
size_t rtsa_crypto_md_get_size(const rtsa_crypto_md_info_t *info);

/**
 * @brief Return the backend-native digest metadata pointer.
 * @param[in] info Digest metadata handle.
 * @return:
 * - != NULL: Backend-native metadata pointer.
 * - = NULL: Invalid metadata.
 */
const void *rtsa_crypto_md_info_native(const rtsa_crypto_md_info_t *info);

/**
 * @brief Return the backend-native digest context pointer.
 * @param[in] ctx Digest context handle.
 * @return:
 * - != NULL: Backend-native digest context pointer.
 * - = NULL: Invalid context.
 */
void *rtsa_crypto_md_ctx_native(rtsa_crypto_md_ctx_t *ctx);

/**
 * @brief Create a digest context.
 * @return:
 * - != NULL: Digest context handle.
 * - = NULL: Allocation failed.
 */
rtsa_crypto_md_ctx_t *rtsa_crypto_md_ctx_create(void);

/**
 * @brief Destroy a digest context.
 * @param[in] ctx Digest context handle.
 */
void rtsa_crypto_md_ctx_destroy(rtsa_crypto_md_ctx_t *ctx);

/**
 * @brief Configure the digest context.
 * @param[in] ctx Digest context handle.
 * @param[in] info Digest metadata handle.
 * @param[in] hmac Whether to initialize HMAC mode.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_md_ctx_setup(rtsa_crypto_md_ctx_t *ctx, const rtsa_crypto_md_info_t *info, int hmac);

/**
 * @brief Query the active digest output size from the configured context.
 * @param[in] ctx Digest context handle.
 * @return:
 * - > 0: Digest size in bytes.
 * - = 0: Context not ready.
 */
size_t rtsa_crypto_md_ctx_get_size(const rtsa_crypto_md_ctx_t *ctx);

/**
 * @brief Start a digest calculation.
 * @param[in] ctx Digest context handle.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_md_ctx_starts(rtsa_crypto_md_ctx_t *ctx);

/**
 * @brief Feed message bytes into the active digest calculation.
 * @param[in] ctx Digest context handle.
 * @param[in] input Input buffer.
 * @param[in] ilen Input buffer length.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_md_ctx_update(rtsa_crypto_md_ctx_t *ctx, const unsigned char *input, size_t ilen);

/**
 * @brief Finish the digest calculation and write the digest bytes.
 * @param[in] ctx Digest context handle.
 * @param[out] output Destination buffer for the digest value.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_md_ctx_finish(rtsa_crypto_md_ctx_t *ctx, unsigned char *output);

#endif
