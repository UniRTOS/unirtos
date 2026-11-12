/**
 * \file config-mini-tls1_1.h
 *
 * \brief Minimal configuration for TLS 1.1 (RFC 4346)
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
 * Minimal configuration for TLS 1.1 (RFC 4346), implementing only the
 * required ciphersuite: MBEDTLS_TLS_RSA_WITH_3DES_EDE_CBC_SHA
 *
 * See README.txt for usage instructions.
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H
#if 1 // Quectel add
/* OS */
#include "qosa_platform_cfg.h"
#define MBEDTLS_OS_FREERTOS

/*TCPIP STACK*/
#define MBEDTLS_TCPIP_LWIP


#if defined(MBEDTLS_OS_FREERTOS)
#define MBEDTLS_PLATFORM_CALLOC_MACRO calloc //mbedtls_calloc //
#define MBEDTLS_PLATFORM_FREE_MACRO	free //mbedtls_free //
#endif

//other part need
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_AES_C
#endif

/* System support */
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_HAVE_TIME

/* mbed TLS feature support */
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#if 1 // Quectel add
#define MBEDTLS_SSL_PROTO_TLS1
#endif
#define MBEDTLS_SSL_PROTO_TLS1_1
#if 1 // Quectel add
#define MBEDTLS_SSL_PROTO_TLS1_2
#endif

/* mbed TLS modules */
#define MBEDTLS_AES_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_DES_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_MD_C
#define MBEDTLS_MD5_C
#if 0 // Quectel add
#define MBEDTLS_NET_C
#endif
#define MBEDTLS_OID_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_RSA_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SSL_CLI_C
#if 0 // Quectel add
#define MBEDTLS_SSL_SRV_C
#endif
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C

/* For test certificates */
#define MBEDTLS_BASE64_C
#if 0 // Quectel add
#define MBEDTLS_CERTS_C
#endif
#define MBEDTLS_PEM_PARSE_C

/* For testing with compat.sh */
//#define MBEDTLS_FS_IO

#if 1 // Quectel add
//#define MBEDTLS_SSL_SESSION_TICKETS
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_HAVE_TIME_DATE

#define MBEDTLS_TIMING_C
#define MBEDTLS_SSL_SERVER_NAME_INDICATION

#define MBEDTLS_USE_RAND_API_ENTROPY
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED //dtls

#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
#define MBEDTLS_KEY_EXCHANGE_SOME_PSK_ENABLED
#define MBEDTLS_SSL_PROTO_DTLS
#define MBEDTLS_TIMING_C
#define MBEDTLS_SSL_MAX_CONTENT_LEN             (16*1024)   /**< Size of the input / output buffer */
//#define MBEDTLS_SSL_MAX_OUT_CONTENT_LEN         (4*1024)   /**< Size of the input / output buffer */

// Use for security fota
#define MBEDTLS_SHA512_C

#define MBEDTLS_ECP_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED

#define MBEDTLS_SSL_RENEGOTIATION
#define MBEDTLS_ARC4_C
#define MBEDTLS_DEBUG_C
//#define MBEDTLS_ERROR_STRERROR_DUMMY
#define MBEDTLS_SSL_SESSION_TICKETS

#define MBEDTLS_ECDH_C
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED

//some server need
#define MBEDTLS_GCM_C
/* for 220.180.239.212 (9611/9612) */
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP192R1_ENABLED

#ifdef CONFIG_QOSA_MBEDTLS_MEMORY_OPTIMIZTAION_SUPPORT
#define MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH
#endif /* CONFIG_QOSA_MBEDTLS_MEMORY_OPTIMIZTAION_SUPPORT */

#endif // Quectel add end


#include "mbedtls/check_config.h"
#endif /* MBEDTLS_CONFIG_H */
