#include <common/bk_include.h>

#include "volc_crypto.h"

#include <stdlib.h>
#include <string.h>

#include <mbedtls/platform.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include <mbedtls/sha256.h>
#include <mbedtls/md5.h>

#include "volc_memory.h"
#include "volc_time.h"
#include "volc_type.h"

static uint32_t _volc_dtls_fill_pseudo_randwom_bits(uint8_t* p_buf, uint32_t buf_size) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    uint32_t i = 0;

    VOLC_CHK(p_buf != NULL, VOLC_STATUS_NULL_ARG);
    VOLC_CHK(buf_size >= VOLC_DTLS_CERT_MIN_SERIAL_NUM_SIZE && buf_size <= VOLC_DTLS_CERT_MAX_SERIAL_NUM_SIZE, ret);

    for (i = 0; i < buf_size; i++) {
        *p_buf++ = (uint8_t) (rand() & 0xFF);
    }

err_out_label:
    return ret;
}

uint32_t volc_certificate_and_key_create(int32_t certificate_bits, bool generate_rsa_certificate, volc_cert_t* p_cert, volc_pkey_t* p_pkey) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    bool initialized = false;
    char* p_cert_buf = NULL;
    char not_before_buf[MBEDTLS_X509_RFC5280_UTC_TIME_LEN + 1], notAfterBuf[MBEDTLS_X509_RFC5280_UTC_TIME_LEN + 1];
    uint64_t now, not_after;
    uint32_t written;
    int32_t len;
    mbedtls_x509_crt* p_cert_tmp = NULL;
    mbedtls_pk_context* p_key_tmp = NULL;
    mbedtls_entropy_context* p_entropy = NULL;
    mbedtls_ctr_drbg_context* p_ctr_drbg = NULL;
    mbedtls_mpi serial;
    mbedtls_x509write_cert* p_write_cert = NULL;
    uint8_t cert_sn[VOLC_DTLS_CERT_MAX_SERIAL_NUM_SIZE];

    VOLC_CHK(p_cert != NULL && p_pkey != NULL, VOLC_STATUS_NULL_ARG);
    VOLC_CHK((p_cert_tmp = (mbedtls_x509_crt *)volc_calloc(1, sizeof(mbedtls_x509_crt))) != NULL, VOLC_STATUS_NOT_ENOUGH_MEMORY);
    VOLC_CHK((p_key_tmp = (mbedtls_pk_context *)volc_calloc(1, sizeof(mbedtls_pk_context))) != NULL, VOLC_STATUS_NOT_ENOUGH_MEMORY);
    *p_cert = (volc_cert_t)p_cert_tmp;
    *p_pkey = (volc_pkey_t)p_key_tmp;

    VOLC_CHK(NULL != (p_cert_buf = (char*) volc_malloc(VOLC_GENERATED_CERTIFICATE_MAX_SIZE)), VOLC_STATUS_NOT_ENOUGH_MEMORY);
    VOLC_CHK(NULL != (p_entropy = (mbedtls_entropy_context*) volc_malloc(sizeof(mbedtls_entropy_context))), VOLC_STATUS_NOT_ENOUGH_MEMORY);
    VOLC_CHK(NULL != (p_ctr_drbg = (mbedtls_ctr_drbg_context*) volc_malloc(sizeof(mbedtls_ctr_drbg_context))), VOLC_STATUS_NOT_ENOUGH_MEMORY);
    VOLC_CHK(NULL != (p_write_cert = (mbedtls_x509write_cert*) volc_malloc(sizeof(mbedtls_x509write_cert))), VOLC_STATUS_NOT_ENOUGH_MEMORY);
    VOLC_CHK_STATUS(_volc_dtls_fill_pseudo_randwom_bits(cert_sn, sizeof(cert_sn)));

    // initialize to sane values
    mbedtls_entropy_init(p_entropy);
    mbedtls_ctr_drbg_init(p_ctr_drbg);
    mbedtls_mpi_init(&serial);
    mbedtls_x509write_crt_init(p_write_cert);
    mbedtls_x509_crt_init(p_cert_tmp);
    mbedtls_pk_init(p_key_tmp);
    initialized = true;
    VOLC_CHK(mbedtls_ctr_drbg_seed(p_ctr_drbg, mbedtls_entropy_func, p_entropy, NULL, 0) == 0, VOLC_STATUS_CERTIFICATE_GENERATION_FAILED);

    // generate a key
    if (generate_rsa_certificate) {
        VOLC_CHK(mbedtls_pk_setup(p_key_tmp, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) == 0 &&
                mbedtls_rsa_gen_key(mbedtls_pk_rsa(*p_key_tmp), mbedtls_ctr_drbg_random, p_ctr_drbg, certificate_bits, VOLC_RTC_RSA_F4) == 0,
            VOLC_STATUS_CERTIFICATE_GENERATION_FAILED);
    } else {
        VOLC_CHK(mbedtls_pk_setup(p_key_tmp, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) == 0 &&
                mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(*p_key_tmp), mbedtls_ctr_drbg_random, p_ctr_drbg) == 0,
            VOLC_STATUS_CERTIFICATE_GENERATION_FAILED);
    }

    // generate a new certificate
    VOLC_CHK(mbedtls_mpi_read_binary(&serial, cert_sn, sizeof(cert_sn)) == 0, VOLC_STATUS_CERTIFICATE_GENERATION_FAILED);

    now = volc_get_time_ms() * VOLC_HUNDREDS_OF_NANOS_IN_A_MILLISECOND;
    VOLC_CHK(volc_generate_timestamp_str(now, "%Y%m%d%H%M%S", not_before_buf, sizeof(not_before_buf), &written) == VOLC_STATUS_SUCCESS,
        VOLC_STATUS_CERTIFICATE_GENERATION_FAILED);
    not_after = now + VOLC_GENERATED_CERTIFICATE_DAYS * VOLC_HUNDREDS_OF_NANOS_IN_A_DAY;
    VOLC_CHK(volc_generate_timestamp_str(not_after, "%Y%m%d%H%M%S", notAfterBuf, sizeof(notAfterBuf), &written) == VOLC_STATUS_SUCCESS,
        VOLC_STATUS_CERTIFICATE_GENERATION_FAILED);

    VOLC_CHK(mbedtls_x509write_crt_set_serial(p_write_cert, &serial) == 0 &&
            mbedtls_x509write_crt_set_validity(p_write_cert, not_before_buf, notAfterBuf) == 0 &&
            mbedtls_x509write_crt_set_subject_name(p_write_cert, "O=" VOLC_GENERATED_CERTIFICATE_NAME ",CN=" VOLC_GENERATED_CERTIFICATE_NAME) == 0 &&
            mbedtls_x509write_crt_set_issuer_name(p_write_cert, "O=" VOLC_GENERATED_CERTIFICATE_NAME ",CN=" VOLC_GENERATED_CERTIFICATE_NAME) == 0,
        VOLC_STATUS_CERTIFICATE_GENERATION_FAILED);
    // void functions, it must succeed
    mbedtls_x509write_crt_set_version(p_write_cert, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_subject_key(p_write_cert, p_key_tmp);
    mbedtls_x509write_crt_set_issuer_key(p_write_cert, p_key_tmp);
    mbedtls_x509write_crt_set_md_alg(p_write_cert, MBEDTLS_MD_SHA1);

    memset(p_cert_buf, 0, VOLC_GENERATED_CERTIFICATE_MAX_SIZE);
    len = mbedtls_x509write_crt_der(p_write_cert, (void *) p_cert_buf, VOLC_GENERATED_CERTIFICATE_MAX_SIZE, mbedtls_ctr_drbg_random, p_ctr_drbg);
    VOLC_CHK(len >= 0, VOLC_STATUS_CERTIFICATE_GENERATION_FAILED);

    VOLC_CHK(mbedtls_x509_crt_parse_der(p_cert_tmp, (void *) (p_cert_buf + VOLC_GENERATED_CERTIFICATE_MAX_SIZE - len), len) == 0, VOLC_STATUS_CERTIFICATE_GENERATION_FAILED);

err_out_label:
    if (initialized) {
        mbedtls_x509write_crt_free(p_write_cert);
        mbedtls_mpi_free(&serial);
        mbedtls_ctr_drbg_free(p_ctr_drbg);
        mbedtls_entropy_free(p_entropy);

        if (VOLC_STATUS_FAILED(ret)) {
            volc_certificate_and_key_destroy((volc_cert_t)&p_cert_tmp, (volc_pkey_t)&p_key_tmp);
        }
    }
    VOLC_SAFE_MEMFREE(p_cert_buf);
    VOLC_SAFE_MEMFREE(p_entropy);
    VOLC_SAFE_MEMFREE(p_ctr_drbg);
    VOLC_SAFE_MEMFREE(p_write_cert);
    return ret;
}

uint32_t volc_certificate_and_key_destroy(volc_cert_t* p_cert, volc_pkey_t* p_key) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    mbedtls_x509_crt* p_cert_tmp = NULL;
    mbedtls_pk_context* p_key_tmp = NULL;

    VOLC_CHK(p_key != NULL && p_key != NULL, VOLC_STATUS_NULL_ARG);

    p_cert_tmp = (mbedtls_x509_crt *)*p_cert;
    p_key_tmp = (mbedtls_pk_context *)*p_key;

    mbedtls_x509_crt_free(p_cert_tmp);
    mbedtls_pk_free(p_key_tmp);
    VOLC_SAFE_MEMFREE(*p_cert);
    VOLC_SAFE_MEMFREE(*p_key);

err_out_label:
    return ret;
}

void volc_md5(const unsigned char* input, size_t ilen, unsigned char output[VOLC_RTC_MD5_DIGEST_LENGTH]) {
    mbedtls_md5(input, ilen, output);
}

void volc_sha1_hmac(const unsigned char* key, size_t keylen, const unsigned char* input, size_t ilen, unsigned char* output, uint32_t* plen) {
    mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), key, keylen, input, ilen, output);
    *(plen) = mbedtls_md_get_size(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1));
}

uint32_t volc_encrypt_or_decrypt(bool encrypt, const char* key, uint64_t iv_number, const unsigned char* input, uint32_t ilen, unsigned char* output, uint32_t* olen) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    size_t output_size = 0;
    size_t size = 0;
    uint64_t iv[2] = {iv_number, iv_number};
    VOLC_CHK(input != NULL, VOLC_STATUS_NULL_ARG);
    mbedtls_cipher_context_t aes_ctr_128_ctr;
    const mbedtls_cipher_info_t *info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_CTR);

    mbedtls_cipher_init(&aes_ctr_128_ctr);
    mbedtls_cipher_setup(&aes_ctr_128_ctr, info);
    mbedtls_operation_t mode = encrypt ? MBEDTLS_ENCRYPT : MBEDTLS_DECRYPT;
    mbedtls_cipher_setkey(&aes_ctr_128_ctr, (uint8_t*)key, VOLC_AES128_KEY_LENGTH * 8, mode);
    mbedtls_cipher_set_iv(&aes_ctr_128_ctr, (uint8_t*)iv, sizeof(iv));

    mbedtls_cipher_update(&aes_ctr_128_ctr, input, ilen, output, &size);
    output_size += size;

    mbedtls_cipher_finish(&aes_ctr_128_ctr, output + output_size, &size);
    output_size += size;
    output[output_size] = 0;
    mbedtls_cipher_free(&aes_ctr_128_ctr);
    *olen = output_size;
err_out_label:
    return ret;
}

uint32_t volc_pk_verify(const char* pub_key, size_t pub_key_len, const char* credential, size_t credential_len, const char* signature, size_t signature_len) {
    uint32_t ret = 0;
    unsigned char hash[MBEDTLS_MD_MAX_SIZE] = { 0 };
    mbedtls_pk_context pk;
    const mbedtls_md_info_t* md_info = NULL;

    mbedtls_pk_init(&pk);
    mbedtls_pk_parse_public_key(&pk, (uint8_t*)pub_key, pub_key_len);

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        ret = VOLC_STATUS_INTERNAL_ERROR;
        goto err_out_label;
    }
    ret = mbedtls_md(md_info, (uint8_t*)credential, credential_len, hash);
    if (0 != ret) {
        ret = VOLC_STATUS_INTERNAL_ERROR;
        goto err_out_label;
    }

    ret = mbedtls_pk_verify(&pk, mbedtls_md_get_type(md_info), hash, mbedtls_md_get_size(md_info), (uint8_t*)signature, signature_len);
    if (ret != 0) {
        ret = VOLC_STATUS_LICENSE_ILLEGAL;
        goto err_out_label;
    }
err_out_label:
    mbedtls_pk_free(&pk);
    return ret;
}
