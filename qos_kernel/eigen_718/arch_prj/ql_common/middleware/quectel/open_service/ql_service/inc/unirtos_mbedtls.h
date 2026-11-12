/**  @file
  func.h

  @brief Automatically generated function declarations header file.

*/

/*================================================================
  Copyright (c) 2022 Quectel Wireless Solution, Co., Ltd.  All Rights Reserved.
  Quectel Wireless Solution Proprietary and Confidential.
=================================================================*/
/*=================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN              WHO         WHAT, WHERE, WHY
------------     -------     -------------------------------------------------------------------------------

=================================================================*/

#ifndef __UNIRTOS_MBEDTLS_H__
#define __UNIRTOS_MBEDTLS_H__

#include <mbedtls/version.h>
#if MBEDTLS_VERSION_NUMBER >= 0x02040000
#include <mbedtls/net_sockets.h>
#else
#include <mbedtls/net.h>
#endif
#include <mbedtls/ssl.h>
#include <mbedtls/x509.h>

#if MBEDTLS_VERSION_NUMBER >= 0x03000000
#include "mbedtls/build_info.h"
#endif /* MBEDTLS_VERSION_NUMBER */

#include <mbedtls/error.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/sha256.h>
#include "mbedtls/debug.h"
#include "mbedtls/sha1.h"
#include "mbedtls/aes.h"
#if defined(MBEDTLS_TIMING_C)
#include "mbedtls/timing.h"
#endif

#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/cipher.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"
#include "psa/crypto.h"

extern const char *mbedtls_ssl_get_alpn_protocol(const mbedtls_ssl_context *ssl);
typedef const char *(* _api_mbedtls_ssl_get_alpn_protocol_t)(const mbedtls_ssl_context *ssl);

extern int mbedtls_ssl_conf_alpn_protocols(mbedtls_ssl_config *conf, const char **protos);
typedef int (* _api_mbedtls_ssl_conf_alpn_protocols_t)(mbedtls_ssl_config *conf, const char **protos);

extern int mbedtls_pk_parse_public_key(mbedtls_pk_context *ctx, const unsigned char *key, size_t keylen);
typedef int (*_api_mbedtls_pk_parse_public_key_t)(mbedtls_pk_context *ctx, const unsigned char *key, size_t keylen);

extern mbedtls_pk_type_t mbedtls_pk_get_type(const mbedtls_pk_context *ctx);
typedef mbedtls_pk_type_t (*_api_mbedtls_pk_get_type_t)(const mbedtls_pk_context *ctx);

extern int mbedtls_rsa_pkcs1_encrypt(mbedtls_rsa_context *ctx,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng,
                               size_t ilen,
                               const unsigned char *input,
                               unsigned char *output);
typedef int (*_api_mbedtls_rsa_pkcs1_encrypt_t)(mbedtls_rsa_context *ctx,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng,
                               size_t ilen,
                               const unsigned char *input,
                               unsigned char *output);

//size_t mbedtls_rsa_get_len(const mbedtls_rsa_context *ctx);
extern size_t mbedtls_rsa_get_len(const mbedtls_rsa_context *ctx);
typedef size_t (*_api_mbedtls_rsa_get_len_t)(const mbedtls_rsa_context *ctx);

extern int mbedtls_pk_encrypt(mbedtls_pk_context *ctx,
                       const unsigned char *input, size_t ilen,
                       unsigned char *output, size_t *olen, size_t osize,
                       int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);
typedef int (* _api_mbedtls_pk_encrypt_t)(mbedtls_pk_context *ctx,
                       const unsigned char *input, size_t ilen,
                       unsigned char *output, size_t *olen, size_t osize,
                       int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);

extern void mbedtls_ssl_conf_dbg( mbedtls_ssl_config *conf,
    void (*f_dbg)(void *, int, const char *, int, const char *),
    void  *p_dbg );
typedef void (* _api_mbedtls_ssl_conf_dbg_t)( mbedtls_ssl_config *conf,
    void (*f_dbg)(void *, int, const char *, int, const char *),
    void  *p_dbg );

extern void  mbedtls_debug_set_threshold(int threshold );
typedef void (* _api_mbedtls_debug_set_threshold_t)(int threshold );

extern void mbedtls_ssl_conf_min_version(mbedtls_ssl_config *conf, int major, int minor);
typedef void (* _api_mbedtls_ssl_conf_min_version_t)(mbedtls_ssl_config *conf, int major, int minor);

extern void mbedtls_ssl_conf_max_version(mbedtls_ssl_config *conf, int major, int minor);
typedef void (* _api_mbedtls_ssl_conf_max_version_t)(mbedtls_ssl_config *conf, int major, int minor);

extern int mbedtls_x509_crt_info(char *buf, size_t size, const char *prefix,const mbedtls_x509_crt *crt);
typedef int (* _api_mbedtls_x509_crt_info_t)(char *buf, size_t size, const char *prefix,const mbedtls_x509_crt *crt);

extern int mbedtls_x509_crt_verify_info(char *buf, size_t size, const char *prefix,uint32_t flags);
typedef int (* _api_mbedtls_x509_crt_verify_info_t)(char *buf, size_t size, const char *prefix,uint32_t flags);

extern void mbedtls_entropy_init(mbedtls_entropy_context *ctx);
typedef void (* _api_mbedtls_entropy_init_t)(mbedtls_entropy_context *ctx);

extern void mbedtls_ctr_drbg_init( mbedtls_ctr_drbg_context *ctx );
typedef void (* _api_mbedtls_ctr_drbg_init_t)( mbedtls_ctr_drbg_context *ctx );

extern int mbedtls_entropy_func(void *data, unsigned char *output, size_t len);
typedef int (* _api_mbedtls_entropy_func_t)(void *data, unsigned char *output, size_t len);

extern void mbedtls_ssl_config_init(mbedtls_ssl_config *conf);
typedef void (* _api_mbedtls_ssl_config_init_t)(mbedtls_ssl_config *conf);

extern int mbedtls_ssl_config_defaults(mbedtls_ssl_config *conf,int endpoint, int transport, int preset);
typedef int (* _api_mbedtls_ssl_config_defaults_t)(mbedtls_ssl_config *conf,int endpoint, int transport, int preset);

extern void mbedtls_x509_crt_init(mbedtls_x509_crt *crt);
typedef void (* _api_mbedtls_x509_crt_init_t)(mbedtls_x509_crt *crt);

extern void mbedtls_pk_init(mbedtls_pk_context *ctx);
typedef void (* _api_mbedtls_pk_init_t)(mbedtls_pk_context *ctx);

extern int mbedtls_ssl_conf_max_frag_len(mbedtls_ssl_config *conf, unsigned char mfl_code);
typedef int (* _api_mbedtls_ssl_conf_max_frag_len_t)(mbedtls_ssl_config *conf, unsigned char mfl_code);

extern void mbedtls_ssl_conf_psk_cb(mbedtls_ssl_config *conf,int (*f_psk)(void *, mbedtls_ssl_context *, const unsigned char *,size_t),void *p_psk);
typedef void (* _api_mbedtls_ssl_conf_psk_cb_t)(mbedtls_ssl_config *conf,int (*f_psk)(void *, mbedtls_ssl_context *, const unsigned char *,size_t),void *p_psk);

extern void mbedtls_ssl_conf_authmode(mbedtls_ssl_config *conf, int authmode);
typedef void (* _api_mbedtls_ssl_conf_authmode_t)(mbedtls_ssl_config *conf, int authmode);

extern int mbedtls_x509_crt_parse(mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen);
typedef int (* _api_mbedtls_x509_crt_parse_t)(mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen);

extern void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config *conf,mbedtls_x509_crt *ca_chain,mbedtls_x509_crl *ca_crl);
typedef void (* _api_mbedtls_ssl_conf_ca_chain_t)(mbedtls_ssl_config *conf,mbedtls_x509_crt *ca_chain,mbedtls_x509_crl *ca_crl);

extern int mbedtls_pk_parse_key(mbedtls_pk_context *ctx,const unsigned char *key, size_t keylen,const unsigned char *pwd, size_t pwdlen,mbedtls_f_rng_t *f_rng, void *p_rng);
typedef int (* _api_mbedtls_pk_parse_key_t)(mbedtls_pk_context *ctx,const unsigned char *key, size_t keylen,const unsigned char *pwd, size_t pwdlen,mbedtls_f_rng_t *f_rng, void *p_rng);

extern int mbedtls_ssl_conf_own_cert(mbedtls_ssl_config *conf,mbedtls_x509_crt *own_cert,mbedtls_pk_context *pk_key);
typedef int (* _api_mbedtls_ssl_conf_own_cert_t)(mbedtls_ssl_config *conf,mbedtls_x509_crt *own_cert,mbedtls_pk_context *pk_key);


extern void mbedtls_ssl_init(mbedtls_ssl_context *ssl);
typedef void (* _api_mbedtls_ssl_init_t)(mbedtls_ssl_context *ssl);

extern void mbedtls_ssl_conf_cert_profile(mbedtls_ssl_config *conf,const mbedtls_x509_crt_profile *profile);
typedef void (* _api_mbedtls_ssl_conf_cert_profile_t)(mbedtls_ssl_config *conf,const mbedtls_x509_crt_profile *profile);

extern void mbedtls_ssl_conf_rng(mbedtls_ssl_config *conf,int (*f_rng)(void *, unsigned char *, size_t),void *p_rng);
typedef void (* _api_mbedtls_ssl_conf_rng_t)(mbedtls_ssl_config *conf,int (*f_rng)(void *, unsigned char *, size_t),void *p_rng);

extern int mbedtls_ctr_drbg_random( void *p_rng,unsigned char *output, size_t output_len );
typedef int (* _api_mbedtls_ctr_drbg_random_t)( void *p_rng,unsigned char *output, size_t output_len );

extern void mbedtls_ssl_conf_verify(mbedtls_ssl_config *conf,int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *),void *p_vrfy);
typedef void (* _api_mbedtls_ssl_conf_verify_t)(mbedtls_ssl_config *conf,int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *),void *p_vrfy);

extern void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl,void *p_bio,mbedtls_ssl_send_t *f_send,mbedtls_ssl_recv_t *f_recv,mbedtls_ssl_recv_timeout_t *f_recv_timeout);
typedef void (* _api_mbedtls_ssl_set_bio_t)(mbedtls_ssl_context *ssl,void *p_bio,mbedtls_ssl_send_t *f_send,mbedtls_ssl_recv_t *f_recv,mbedtls_ssl_recv_timeout_t *f_recv_timeout);

extern void mbedtls_ssl_conf_ciphersuites(mbedtls_ssl_config *conf,const int *ciphersuites);
typedef void (* _api_mbedtls_ssl_conf_ciphersuites_t)(mbedtls_ssl_config *conf,const int *ciphersuites);

const int *mbedtls_ssl_list_ciphersuites(void);
extern void mbedtls_ssl_set_mtu(mbedtls_ssl_context *ssl, uint16_t mtu);
typedef void (* _api_mbedtls_ssl_set_mtu_t)(mbedtls_ssl_context *ssl, uint16_t mtu);

extern void mbedtls_ssl_set_datagram_packing(mbedtls_ssl_context *ssl,unsigned allow_packing);
typedef void (* _api_mbedtls_ssl_set_datagram_packing_t)(mbedtls_ssl_context *ssl,unsigned allow_packing);

extern void mbedtls_ssl_set_timer_cb(mbedtls_ssl_context *ssl,void *p_timer,mbedtls_ssl_set_timer_t *f_set_timer,mbedtls_ssl_get_timer_t *f_get_timer);
typedef void (* _api_mbedtls_ssl_set_timer_cb_t)(mbedtls_ssl_context *ssl,void *p_timer,mbedtls_ssl_set_timer_t *f_set_timer,mbedtls_ssl_get_timer_t *f_get_timer);

extern void mbedtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms);
typedef void (* _api_mbedtls_timing_set_delay_t)(void *data, uint32_t int_ms, uint32_t fin_ms);

extern int mbedtls_timing_get_delay(void *data);
typedef int (* _api_mbedtls_timing_get_delay_t)(void *data);

extern void mbedtls_ssl_conf_handshake_timeout(mbedtls_ssl_config *conf, uint32_t min, uint32_t max);
typedef void (* _api_mbedtls_ssl_conf_handshake_timeout_t)(mbedtls_ssl_config *conf, uint32_t min, uint32_t max);

extern void mbedtls_ssl_conf_session_tickets(mbedtls_ssl_config *conf, int use_tickets);
typedef void (* _api_mbedtls_ssl_conf_session_tickets_t)(mbedtls_ssl_config *conf, int use_tickets);

extern int mbedtls_ssl_set_hostname(mbedtls_ssl_context *ssl, const char *hostname);
typedef int (* _api_mbedtls_ssl_set_hostname_t)(mbedtls_ssl_context *ssl, const char *hostname);

extern int mbedtls_ssl_setup(mbedtls_ssl_context *ssl,const mbedtls_ssl_config *conf);
typedef int (* _api_mbedtls_ssl_setup_t)(mbedtls_ssl_context *ssl,const mbedtls_ssl_config *conf);

extern int mbedtls_ssl_set_session(mbedtls_ssl_context *ssl, const mbedtls_ssl_session *session);
typedef int (* _api_mbedtls_ssl_set_session_t)(mbedtls_ssl_context *ssl, const mbedtls_ssl_session *session);

extern int mbedtls_ssl_handshake(mbedtls_ssl_context *ssl);
typedef int (* _api_mbedtls_ssl_handshake_t)(mbedtls_ssl_context *ssl);

const char *mbedtls_ssl_get_ciphersuite(const mbedtls_ssl_context *ssl);
typedef char *(* _api_mbedtls_ssl_get_ciphersuite_t)(const mbedtls_ssl_context *ssl);

extern void mbedtls_ssl_session_init(mbedtls_ssl_session *session);
typedef void (* _api_mbedtls_ssl_session_init_t)(mbedtls_ssl_session *session);

extern int mbedtls_ssl_get_session(const mbedtls_ssl_context *ssl, mbedtls_ssl_session *session);
typedef int (* _api_mbedtls_ssl_get_session_t)(const mbedtls_ssl_context *ssl, mbedtls_ssl_session *session);

extern size_t mbedtls_ssl_get_bytes_avail(const mbedtls_ssl_context *ssl);
typedef size_t (* _api_mbedtls_ssl_get_bytes_avail_t)(const mbedtls_ssl_context *ssl);

extern void mbedtls_pk_free(mbedtls_pk_context *ctx);
typedef void (* _api_mbedtls_pk_free_t)(mbedtls_pk_context *ctx);

extern void mbedtls_x509_crt_free(mbedtls_x509_crt *crt);
typedef void (* _api_mbedtls_x509_crt_free_t)(mbedtls_x509_crt *crt);

extern void mbedtls_ssl_config_free(mbedtls_ssl_config *conf);
typedef void (* _api_mbedtls_ssl_config_free_t)(mbedtls_ssl_config *conf);

extern void mbedtls_ssl_free(mbedtls_ssl_context *ssl);
typedef void (* _api_mbedtls_ssl_free_t)(mbedtls_ssl_context *ssl);

extern void mbedtls_ctr_drbg_free( mbedtls_ctr_drbg_context *ctx );
typedef void (* _api_mbedtls_ctr_drbg_free_t)( mbedtls_ctr_drbg_context *ctx );

extern void mbedtls_entropy_free(mbedtls_entropy_context *ctx);
typedef void (* _api_mbedtls_entropy_free_t)(mbedtls_entropy_context *ctx);

extern int mbedtls_ssl_read( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len );
typedef int (* _api_mbedtls_ssl_read_t)( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len );

extern int mbedtls_ssl_write( mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len );
typedef int (* _api_mbedtls_ssl_write_t)( mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len );

extern void mbedtls_ssl_session_free(mbedtls_ssl_session *session);
typedef void (* _api_mbedtls_ssl_session_free_t)(mbedtls_ssl_session *session);

extern int mbedtls_ctr_drbg_seed( mbedtls_ctr_drbg_context *ctx,
  int (*f_entropy)(void *, unsigned char *, size_t),
  void *p_entropy,
  const unsigned char *custom,
  size_t len );
typedef int (* _api_mbedtls_ctr_drbg_seed_t)(mbedtls_ctr_drbg_context *ctx,
  int (*f_entropy)(void *, unsigned char *, size_t),
  void *p_entropy,
  const unsigned char *custom,
  size_t len );

extern void mbedtls_ssl_conf_arc4_support(mbedtls_ssl_config *conf, char arc4);
typedef void (* _api_mbedtls_ssl_conf_arc4_support_t)(mbedtls_ssl_config *conf, char arc4);

extern const int *mbedtls_ssl_list_ciphersuites(void);
typedef const int *(* _api_mbedtls_ssl_list_ciphersuites_t)(void);

extern void mbedtls_ssl_conf_renegotiation(mbedtls_ssl_config *conf, int renegotiation);
typedef void (* _api_mbedtls_ssl_conf_renegotiation_t)(mbedtls_ssl_config *conf, int renegotiation);

extern const mbedtls_ssl_ciphersuite_t *mbedtls_ssl_ciphersuite_from_id(int ciphersuite_id);
typedef const mbedtls_ssl_ciphersuite_t *(* _api_mbedtls_ssl_ciphersuite_from_id_t)(int ciphersuite_id);

extern int mbedtls_ssl_conf_psk(mbedtls_ssl_config *conf,
  const unsigned char *psk, size_t psk_len,
  const unsigned char *psk_identity, size_t psk_identity_len);
typedef int (* _api_mbedtls_ssl_conf_psk_t)(mbedtls_ssl_config *conf,
  const unsigned char *psk, size_t psk_len,
  const unsigned char *psk_identity, size_t psk_identity_len);

extern void mbedtls_sha1_init( mbedtls_sha1_context *ctx );
typedef void (* _api_mbedtls_sha1_init_t)( mbedtls_sha1_context *ctx );

extern int mbedtls_sha1_starts( mbedtls_sha1_context *ctx );
typedef int (* _api_mbedtls_sha1_starts_t)( mbedtls_sha1_context *ctx );

extern int mbedtls_sha1_update( mbedtls_sha1_context *ctx ,const unsigned char *input, size_t ilen );
typedef int (* _api_mbedtls_sha1_update_t)( mbedtls_sha1_context *ctx ,const unsigned char *input, size_t ilen );

extern int mbedtls_sha1_finish( mbedtls_sha1_context *ctx , unsigned char output[20] );
typedef int (* _api_mbedtls_sha1_finish_t)( mbedtls_sha1_context *ctx , unsigned char output[20] );

extern void mbedtls_aes_free( mbedtls_aes_context *ctx );
typedef void (* _api_mbedtls_aes_free_t)( mbedtls_aes_context *ctx);

extern int mbedtls_aes_crypt_cbc( mbedtls_aes_context *ctx,
    int mode,
    size_t length,
    unsigned char iv[16],
    const unsigned char *input,
    unsigned char *output );
typedef int (* _api_mbedtls_aes_crypt_cbc_t)( mbedtls_aes_context *ctx,
    int mode,
    size_t length,
    unsigned char iv[16],
    const unsigned char *input,
    unsigned char *output );

extern void mbedtls_aes_init( mbedtls_aes_context *ctx );
typedef void (* _api_mbedtls_aes_init_t)( mbedtls_aes_context *ctx);

extern int mbedtls_aes_setkey_enc( mbedtls_aes_context *ctx, const unsigned char *key,
    unsigned int keybits );
typedef int (* _api_mbedtls_aes_setkey_enc_t)( mbedtls_aes_context *ctx, const unsigned char *key,
    unsigned int keybits );

extern int mbedtls_aes_setkey_dec( mbedtls_aes_context *ctx, const unsigned char *key,
    unsigned int keybits );
typedef int (* _api_mbedtls_aes_setkey_dec_t)( mbedtls_aes_context *ctx, const unsigned char *key,
    unsigned int keybits );

extern void mbedtls_psa_crypto_free(void);
typedef void (*_api_mbedtls_psa_crypto_free_t)(void);

extern psa_status_t psa_crypto_init(void);
typedef psa_status_t (*_api_psa_crypto_init_t)(void);

extern void mbedtls_ssl_conf_tls13_key_exchange_modes(mbedtls_ssl_config *conf, const int kex_modes);
typedef void (*_api_mbedtls_ssl_conf_tls13_key_exchange_modes_t)(mbedtls_ssl_config *conf, const int kex_modes);

void mbedtls_ssl_set_export_keys_cb(mbedtls_ssl_context *ssl, mbedtls_ssl_export_keys_t *f_export_keys, void *p_export_keys);
typedef void (*_api_mbedtls_ssl_set_export_keys_cb_t)(mbedtls_ssl_context *ssl, mbedtls_ssl_export_keys_t *f_export_keys, void *p_export_keys);

extern int mbedtls_platform_set_calloc_free(void *(*calloc_func)(size_t, size_t),
                                            void (*free_func)(void *));
typedef int (*_api_mbedtls_platform_set_calloc_free_t)(void *(*calloc_func)(size_t, size_t),
                                                       void (*free_func)(void *));

extern int mbedtls_ssl_close_notify(mbedtls_ssl_context *ssl);
typedef int (*_api_mbedtls_ssl_close_notify_t)(mbedtls_ssl_context *ssl);

extern void mbedtls_ssl_conf_read_timeout(mbedtls_ssl_config *conf, uint32_t timeout);
typedef void (*_api_mbedtls_ssl_conf_read_timeout_t)(mbedtls_ssl_config *conf, uint32_t timeout);

extern uint32_t mbedtls_ssl_get_verify_result(const mbedtls_ssl_context *ssl);
typedef uint32_t (*_api_mbedtls_ssl_get_verify_result_t)(const mbedtls_ssl_context *ssl);

#if defined(MBEDTLS_CIPHER_C)
extern const mbedtls_cipher_info_t *mbedtls_cipher_info_from_string(const char *cipher_name);
typedef const mbedtls_cipher_info_t *(*_api_mbedtls_cipher_info_from_string_t)(const char *cipher_name);

extern void mbedtls_cipher_init(mbedtls_cipher_context_t *ctx);
typedef void (*_api_mbedtls_cipher_init_t)(mbedtls_cipher_context_t *ctx);

extern void mbedtls_cipher_free(mbedtls_cipher_context_t *ctx);
typedef void (*_api_mbedtls_cipher_free_t)(mbedtls_cipher_context_t *ctx);

extern int mbedtls_cipher_setup(mbedtls_cipher_context_t *ctx, const mbedtls_cipher_info_t *cipher_info);
typedef int (*_api_mbedtls_cipher_setup_t)(mbedtls_cipher_context_t *ctx, const mbedtls_cipher_info_t *cipher_info);

extern int mbedtls_cipher_setkey(mbedtls_cipher_context_t *ctx, const unsigned char *key,
                                 int key_bitlen, const mbedtls_operation_t operation);
typedef int (*_api_mbedtls_cipher_setkey_t)(mbedtls_cipher_context_t *ctx, const unsigned char *key,
                                            int key_bitlen, const mbedtls_operation_t operation);

extern int mbedtls_cipher_set_iv(mbedtls_cipher_context_t *ctx, const unsigned char *iv, size_t iv_len);
typedef int (*_api_mbedtls_cipher_set_iv_t)(mbedtls_cipher_context_t *ctx, const unsigned char *iv, size_t iv_len);

extern int mbedtls_cipher_reset(mbedtls_cipher_context_t *ctx);
typedef int (*_api_mbedtls_cipher_reset_t)(mbedtls_cipher_context_t *ctx);

extern int mbedtls_cipher_update(mbedtls_cipher_context_t *ctx, const unsigned char *input,
                                 size_t ilen, unsigned char *output, size_t *olen);
typedef int (*_api_mbedtls_cipher_update_t)(mbedtls_cipher_context_t *ctx, const unsigned char *input,
                                            size_t ilen, unsigned char *output, size_t *olen);

extern int mbedtls_cipher_finish(mbedtls_cipher_context_t *ctx, unsigned char *output, size_t *olen);
typedef int (*_api_mbedtls_cipher_finish_t)(mbedtls_cipher_context_t *ctx, unsigned char *output, size_t *olen);

#if defined(MBEDTLS_GCM_C) || defined(MBEDTLS_CHACHAPOLY_C)
extern int mbedtls_cipher_update_ad(mbedtls_cipher_context_t *ctx, const unsigned char *ad, size_t ad_len);
typedef int (*_api_mbedtls_cipher_update_ad_t)(mbedtls_cipher_context_t *ctx, const unsigned char *ad, size_t ad_len);

extern int mbedtls_cipher_write_tag(mbedtls_cipher_context_t *ctx, unsigned char *tag, size_t tag_len);
typedef int (*_api_mbedtls_cipher_write_tag_t)(mbedtls_cipher_context_t *ctx, unsigned char *tag, size_t tag_len);

extern int mbedtls_cipher_check_tag(mbedtls_cipher_context_t *ctx, const unsigned char *tag, size_t tag_len);
typedef int (*_api_mbedtls_cipher_check_tag_t)(mbedtls_cipher_context_t *ctx, const unsigned char *tag, size_t tag_len);
#endif /* MBEDTLS_GCM_C || MBEDTLS_CHACHAPOLY_C */
#endif /* MBEDTLS_CIPHER_C */

#if defined(MBEDTLS_MD_C)
extern const mbedtls_md_info_t *mbedtls_md_info_from_type(mbedtls_md_type_t md_type);
typedef const mbedtls_md_info_t *(*_api_mbedtls_md_info_from_type_t)(mbedtls_md_type_t md_type);

extern const mbedtls_md_info_t *mbedtls_md_info_from_ctx(const mbedtls_md_context_t *ctx);
typedef const mbedtls_md_info_t *(*_api_mbedtls_md_info_from_ctx_t)(const mbedtls_md_context_t *ctx);

extern void mbedtls_md_init(mbedtls_md_context_t *ctx);
typedef void (*_api_mbedtls_md_init_t)(mbedtls_md_context_t *ctx);

extern void mbedtls_md_free(mbedtls_md_context_t *ctx);
typedef void (*_api_mbedtls_md_free_t)(mbedtls_md_context_t *ctx);

extern int mbedtls_md_setup(mbedtls_md_context_t *ctx, const mbedtls_md_info_t *md_info, int hmac);
typedef int (*_api_mbedtls_md_setup_t)(mbedtls_md_context_t *ctx, const mbedtls_md_info_t *md_info, int hmac);

extern unsigned char mbedtls_md_get_size(const mbedtls_md_info_t *md_info);
typedef unsigned char (*_api_mbedtls_md_get_size_t)(const mbedtls_md_info_t *md_info);

extern int mbedtls_md_starts(mbedtls_md_context_t *ctx);
typedef int (*_api_mbedtls_md_starts_t)(mbedtls_md_context_t *ctx);

extern int mbedtls_md_update(mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen);
typedef int (*_api_mbedtls_md_update_t)(mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen);

extern int mbedtls_md_finish(mbedtls_md_context_t *ctx, unsigned char *output);
typedef int (*_api_mbedtls_md_finish_t)(mbedtls_md_context_t *ctx, unsigned char *output);

#if defined(MBEDTLS_PKCS5_C) && !defined(MBEDTLS_DEPRECATED_REMOVED)
typedef int (*_api_mbedtls_pkcs5_pbkdf2_hmac_t)(mbedtls_md_context_t *ctx,
                                                const unsigned char *password, size_t plen,
                                                const unsigned char *salt, size_t slen,
                                                unsigned int iteration_count,
                                                uint32_t key_length, unsigned char *output);
#endif /* MBEDTLS_PKCS5_C && !MBEDTLS_DEPRECATED_REMOVED */
#endif /* MBEDTLS_MD_C */

#if defined(MBEDTLS_PKCS5_C) && !defined(MBEDTLS_DEPRECATED_REMOVED)
extern void mbedtls_strerror(int errnum, char *buffer, size_t buflen);
typedef void (*_api_mbedtls_strerror_t)(int errnum, char *buffer, size_t buflen);
#endif /* MBEDTLS_PKCS5_C && !MBEDTLS_DEPRECATED_REMOVED */

#endif /* __UNIRTOS_MBEDTLS_H__ */
