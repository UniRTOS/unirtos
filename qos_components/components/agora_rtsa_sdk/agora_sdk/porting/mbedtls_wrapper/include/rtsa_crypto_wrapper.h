#ifndef __RTSA_CRYPTO_WRAPPER_H__
#define __RTSA_CRYPTO_WRAPPER_H__

/*
 * Aggregate include for all RTSA crypto abstraction interfaces.
 * RTSA business code should prefer including this header when multiple
 * wrapper capabilities are needed.
 */

#include "rtsa_crypto_base.h"
#include "rtsa_crypto_rng.h"
#include "rtsa_crypto_cipher.h"
#include "rtsa_crypto_md.h"
#include "rtsa_crypto_kdf.h"
#include "rtsa_crypto_key_exchange.h"
#include "rtsa_crypto_tls.h"

#endif
