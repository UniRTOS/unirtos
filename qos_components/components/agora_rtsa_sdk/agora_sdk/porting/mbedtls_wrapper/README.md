# RTSA Crypto Wrapper

## Overview

This is the mbedTLS adapter layer for RTSA SDK. It bridges the SDK's internal
crypto operations to the mbedTLS library through a set of generic
`rtsa_crypto_*` interfaces.

When the SDK is built with external mbedTLS mode (`CONFIG_EXTERNAL_MBEDTLS`),
this wrapper is packaged alongside `libagora-rtc-sdk.a`. Customers need to
compile these sources together with their own mbedTLS to resolve the crypto
symbols that the SDK depends on.

## File Structure

```text
mbedtls_wrapper/
├── CMakeLists.txt          # CMake config — defines sources, macros, and include path
├── README.md                # This file
├── include/                 # Public wrapper headers (generic rtsa_crypto_* API)
│   ├── rtsa_crypto_base.h          # Error string conversion
│   ├── rtsa_crypto_cipher.h        # Symmetric cipher (AES-GCM, AES-CTR, etc)
│   ├── rtsa_crypto_kdf.h           # Key derivation (PBKDF2, HKDF)
│   ├── rtsa_crypto_key_exchange.h  # ECDH P-256 key agreement
│   ├── rtsa_crypto_md.h            # Message digest (SHA-256, HMAC)
│   ├── rtsa_crypto_rng.h           # Cryptographic RNG (CTR-DRBG)
│   └── rtsa_crypto_tls.h           # TLS 1.2 client session
└── src/                     # mbedTLS backend implementation
    ├── rtsa_mbedtls_base.c
    ├── rtsa_mbedtls_cipher.c
    ├── rtsa_mbedtls_kdf.c
    ├── rtsa_mbedtls_key_exchange.c
    ├── rtsa_mbedtls_md.c
    ├── rtsa_mbedtls_rng.c
    └── rtsa_mbedtls_tls.c
```

## Capability Macros

Each source file is guarded by capability macros. The `CMakeLists.txt`
shipped with the SDK package already specifies the exact set of macros
required by this build — customers should **not** modify these values.
They must match the SDK binary; mismatched macros will cause link errors.

Available macros (for reference only):

| Macro | Controls |
|-------|----------|
| `RTSA_CRYPTO_WRAPPER_ENABLE_CIPHER` | Symmetric cipher |
| `RTSA_CRYPTO_WRAPPER_ENABLE_MD` | Message digest |
| `RTSA_CRYPTO_WRAPPER_ENABLE_PBKDF2` | PBKDF2 key derivation |
| `RTSA_CRYPTO_WRAPPER_ENABLE_HKDF` | HKDF key derivation |
| `RTSA_CRYPTO_WRAPPER_ENABLE_RNG` | Random number generation |
| `RTSA_CRYPTO_WRAPPER_ENABLE_TLS` | TLS client |
| `RTSA_CRYPTO_WRAPPER_ENABLE_KEY_EXCHANGE` | ECDH key agreement |

## Integration (CMake)

The generated `CMakeLists.txt` exports three variables ready for use:

- `RTSA_CRYPTO_WRAPPER_INCLUDES` — header include path
- `RTSA_CRYPTO_WRAPPER_SOURCES` — source files to compile
- `RTSA_CRYPTO_WRAPPER_DEFINITIONS` — required compile definitions

Example CMake integration:

```cmake
include(${SDK_PATH}/porting/mbedtls_wrapper/CMakeLists.txt)

target_include_directories(your_app PRIVATE
  ${RTSA_CRYPTO_WRAPPER_INCLUDES}
  ${YOUR_MBEDTLS_INCLUDE_PATH})

target_sources(your_app PRIVATE
  ${RTSA_CRYPTO_WRAPPER_SOURCES})

target_compile_definitions(your_app PRIVATE
  ${RTSA_CRYPTO_WRAPPER_DEFINITIONS})

target_link_libraries(your_app PRIVATE
  agora-rtc-sdk
  mbedtls mbedcrypto mbedx509)
```

## mbedTLS Compatibility

Supports mbedTLS 2.x and 3.x. Version differences are handled internally.
