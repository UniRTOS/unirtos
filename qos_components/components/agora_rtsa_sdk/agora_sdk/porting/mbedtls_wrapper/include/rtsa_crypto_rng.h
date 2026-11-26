#ifndef __RTSA_CRYPTO_RNG_H__
#define __RTSA_CRYPTO_RNG_H__

/** Opaque RNG adapter context (CTR-DRBG + entropy). */
typedef struct rtsa_crypto_rng_adapter rtsa_crypto_rng_adapter_t;

/**
 * @brief Create an RNG adapter instance.
 * @return:
 * - != NULL: RNG adapter handle.
 * - = NULL: Allocation failed.
 */
rtsa_crypto_rng_adapter_t *rtsa_crypto_rng_adapter_create(void);

/**
 * @brief Destroy an RNG adapter and free associated resources.
 * @param[in] ctx RNG adapter handle.
 */
void rtsa_crypto_rng_adapter_destroy(rtsa_crypto_rng_adapter_t *ctx);

/**
 * @brief Seed the CTR-DRBG with entropy.
 * @param[in] ctx RNG adapter handle.
 * @return:
 * - = 0: Success.
 * - != 0: Backend error code.
 */
int rtsa_crypto_rng_adapter_seed(rtsa_crypto_rng_adapter_t *ctx);

/**
 * @brief Return the underlying CTR-DRBG context pointer.
 * @param[in] ctx RNG adapter handle.
 * @return:
 * - != NULL: Backend-native CTR-DRBG context pointer.
 * - = NULL: Invalid context.
 */
void *rtsa_crypto_rng_adapter_ctr_drbg(rtsa_crypto_rng_adapter_t *ctx);

#endif
