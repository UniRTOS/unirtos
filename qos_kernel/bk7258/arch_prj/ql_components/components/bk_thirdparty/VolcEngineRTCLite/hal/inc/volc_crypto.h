/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_CRYPTO_H__
#define __HAL_VOLC_CRYPTO_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#if defined(__BUILDING_BYTE_RTC_SDK__)
#define __byte_rtc_api__ __declspec(dllexport)
#else
#define __byte_rtc_api__ __declspec(dllimport)
#endif
#else
#define __byte_rtc_api__  __attribute__((visibility("default")))
#endif

/**
 * @brief 定义生成证书的最大大小，单位为字节。
 * 
 * 该宏定义了生成的证书的最大允许大小，这里设置为 4096 字节。
 */
#define VOLC_GENERATED_CERTIFICATE_MAX_SIZE 4096

/**
 * @brief 定义 DTLS 证书序列号的最小长度，单位为字节。
 * 
 * 该宏定义了 DTLS 证书序列号的最小长度，这里设置为 8 字节。
 */
#define VOLC_DTLS_CERT_MIN_SERIAL_NUM_SIZE  8

/**
 * @brief 定义 DTLS 证书序列号的最大长度，单位为字节。
 * 
 * 该宏定义了 DTLS 证书序列号的最大长度，这里设置为 20 字节。
 */
#define VOLC_DTLS_CERT_MAX_SERIAL_NUM_SIZE  20

/**
 * @brief 定义生成证书的有效期，单位为天。
 * 
 * 该宏定义了生成的证书的有效期，这里设置为 365 天。
 */
#define VOLC_GENERATED_CERTIFICATE_DAYS     365

/**
 * @brief 定义生成证书的名称。
 * 
 * 该宏定义了生成的证书的名称，这里设置为 "LITE-WebRTC-Client"。
 */
#define VOLC_GENERATED_CERTIFICATE_NAME     "LITE-WebRTC-Client"

/**
 * @brief 定义密钥提取器的标签。
 * 
 * 该宏定义了密钥提取器的标签，用于 DTLS-SRTP 协议，这里设置为 "EXTRACTOR-dtls_srtp"。
 */
#define VOLC_KEYING_EXTRACTOR_LABEL         "EXTRACTOR-dtls_srtp"

/**
 * @brief 定义 RSA 算法的公共指数。
 * 
 * 该宏定义了 RSA 算法的公共指数，这里设置为 0x10001L。
 */
#define VOLC_RTC_RSA_F4             0x10001L

/**
 * @brief 定义 MD5 哈希算法的摘要长度，单位为字节。
 * 
 * 该宏定义了 MD5 哈希算法生成的摘要的长度，这里设置为 16 字节。
 */
#define VOLC_RTC_MD5_DIGEST_LENGTH  16

/**
 * @brief 定义 SHA1 哈希算法的摘要长度，单位为字节。
 * 
 * 该宏定义了 SHA1 哈希算法生成的摘要的长度，这里设置为 20 字节。
 */
#define VOLC_RTC_SHA1_DIGEST_LENGTH 20

/**
 * @brief 定义 AES256 加密算法的密钥长度，单位为字节。
 * 
 * 该宏定义了 AES256 加密算法使用的密钥的长度，这里设置为 32 字节。
 */
#define VOLC_AES256_KEY_LENGTH     32

/**
 * @brief 定义 AES128 加密算法的密钥长度，单位为字节。
 * 
 * 该宏定义了 AES128 加密算法使用的密钥的长度，这里设置为 16 字节。
 */
#define VOLC_AES128_KEY_LENGTH     16

/**
 * @brief 证书句柄。
 * 
 * 该 typedef 定义了一个名为 volc_cert_t 的类型别名，它实际上是一个 void 指针，用于表示证书对象。
 */
typedef void* volc_cert_t;

/**
 * @brief 秘钥句柄。
 * 
 * 该 typedef 定义了一个名为 volc_pkey_t 的类型别名，它实际上是一个 void 指针，用于表示私钥对象。
 */
typedef void* volc_pkey_t;

/**
 * @brief 定义 SRTP 配置文件的枚举类型。
 * 
 * 该枚举类型定义了不同的 SRTP 配置文件，包括 AES128_CM_HMAC_SHA1_80 和 AES128_CM_HMAC_SHA1_32。
 */
typedef enum {
    /**
     * @brief 使用 AES128-CM 和 HMAC-SHA1-80 的 SRTP 配置文件。
     */
    VOLC_SRTP_PROFILE_AES128_CM_HMAC_SHA1_80 = 0,
    /**
     * @brief 使用 AES128-CM 和 HMAC-SHA1-32 的 SRTP 配置文件。
     */
    VOLC_SRTP_PROFILE_AES128_CM_HMAC_SHA1_32 = 1,
} volc_srtp_profile_t;

/**
 * @brief 创建证书和密钥对。
 *
 * 该函数用于生成一个证书和对应的私钥。可以选择生成 RSA 证书。
 *
 * @param certificate_bits 证书的位数，例如 2048 或 4096。
 * @param generate_rsa_certificate 是否生成 RSA 证书。
 * @param cert 指向存储生成证书的指针。
 * @param pkey 指向存储生成私钥的指针。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_certificate_and_key_create(int32_t certificate_bits, bool generate_rsa_certificate, volc_cert_t* cert, volc_pkey_t* pkey);

/**
 * @brief 销毁证书和密钥对。
 *
 * 该函数用于释放之前生成的证书和私钥所占用的资源。
 *
 * @param cert 指向要销毁的证书的指针。
 * @param pkey 指向要销毁的私钥的指针。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_certificate_and_key_destroy(volc_cert_t* cert, volc_pkey_t* pkey);

/**
 * @brief 计算输入数据的 MD5 哈希值。
 *
 * 该函数使用 MD5 算法计算输入数据的哈希值。
 *
 * @param input 输入数据的指针。
 * @param ilen 输入数据的长度。
 * @param output 存储 MD5 哈希值的数组，长度必须为 VOLC_RTC_MD5_DIGEST_LENGTH。
 */
__byte_rtc_api__ void volc_md5(const unsigned char* input, size_t ilen, unsigned char output[VOLC_RTC_MD5_DIGEST_LENGTH]);

/**
 * @brief 使用 SHA1 算法计算 HMAC 值。
 *
 * 该函数使用 SHA1 算法和给定的密钥计算输入数据的 HMAC 值。
 *
 * @param key 用于 HMAC 计算的密钥。
 * @param keylen 密钥的长度。
 * @param input 输入数据的指针。
 * @param ilen 输入数据的长度。
 * @param output 存储 HMAC 值的数组。
 * @param plen 指向存储 HMAC 值长度的指针。
 */
__byte_rtc_api__ void volc_sha1_hmac(const unsigned char* key, size_t keylen, const unsigned char* input, size_t ilen, unsigned char* output, uint32_t* plen);

/**
 * @brief 对输入数据进行加密或解密操作。
 *
 * 该函数可以对输入数据进行加密或解密，具体操作取决于 encrypt 参数。
 *
 * @param encrypt 如果为 true，则进行加密操作；如果为 false，则进行解密操作。
 * @param key 用于加密或解密的密钥。
 * @param iv 初始化向量。
 * @param input 输入数据的指针。
 * @param ilen 输入数据的长度。
 * @param output 存储加密或解密结果的数组。
 * @param olen 指向存储输出数据长度的指针。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_encrypt_or_decrypt(bool encrypt, const char* key, uint64_t iv, const unsigned char* input, uint32_t ilen, unsigned char* output, __byte_rtc_api__ uint32_t* olen);

/**
 * @brief 使用公钥验证签名。
 *
 * 该函数使用给定的公钥验证凭证的签名是否有效。
 *
 * @param pub_key 用于验证的公钥。
 * @param pub_key_len 公钥的长度。
 * @param credential 要验证的凭证。
 * @param credential_len 凭证的长度。
 * @param signature 凭证的签名。
 * @param signature_len 签名的长度。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_pk_verify(const char* pub_key, size_t pub_key_len, const char* credential, size_t credential_len, const char* signature, size_t signature_len);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_CRYPTO_H__ */
