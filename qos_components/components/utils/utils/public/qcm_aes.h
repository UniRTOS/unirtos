/**
 *@file qcm_aes.h
 * @author lawrence.liu (lawrence.liu@quectel.com)
 *  @brief
 * @version 0.1
 * @date 2023-11-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef _QOSA_AES_H
#define _QOSA_AES_H

#include "qosa_sys.h"
#include "qosa_def.h"

/**
 * @brief Encrypts input data using the AES algorithm in CBC (Cipher Block Chaining) mode,
 *
 * @param key         Pointer to the AES encryption key.
 * @param keybits     The length of the key in bits. Valid values are typically 128, 192, or 256.
 * @param iv          Pointer to the 16-byte Initialization Vector (IV).
 * @param inputdata   Pointer to the raw input data to be encrypted.
 * @param outputdata  Pointer to the output buffer. Buffer size must be at least ((inlen + 15) / 16) * 16 bytes
 *                    to accommodate PKCS#7 padding requirements (16-byte block alignment).
 * @param inlen       Length of the input data in bytes.
 * @param outlen      Pointer to an integer that will receive the actual length of the encrypted output data.
 *                    Output length will be a multiple of 16 bytes due to block cipher requirements.
 *                    If NULL, the actual output length will not be returned.
 *
 * @return            0 success
 *                   -1 error
 *
 * @note              The output buffer must have sufficient space to hold the padded encrypted data.
 */
int qcm_aes_cbc_encryption(
    qosa_uint8_t *key,
    qosa_uint32_t keybits,
    qosa_uint8_t  iv[16],
    qosa_uint8_t *inputdata,
    qosa_uint8_t *outputdata,
    int           inlen,
    int          *outlen
);

/**
 * @brief Decrypts input data using the AES algorithm in CBC (Cipher Block Chaining) mode,
 *
 * @param key         Pointer to the AES decryption key (same as encryption key).
 * @param keybits     The length of the key in bits. Must match the key used for encryption.
 * @param iv          Pointer to the 16-byte Initialization Vector (IV). Must match the IV used for encryption.
 * @param inputdata   Pointer to the encrypted input data to be decrypted. Data must be a multiple of 16 bytes.
 * @param outputdata  Pointer to the output buffer. Buffer size must be at least equal to input length
 *                    to accommodate the decrypted output data after padding removal.
 * @param inlen       Length of the input data in bytes. Must be a multiple of 16 (block size).
 * @param outlen      Pointer to an integer that will receive the actual length of the decrypted output data
 *                    after PKCS#7 padding removal. This will typically be less than or equal to inlen.
 *                    If NULL, the actual output length will not be returned.
 *
 * @return            0 success
 *                   -1 error
 *
 * @note              The input data length must be a multiple of 16 bytes (AES block size).
 */
int qcm_aes_cbc_decryption(
    qosa_uint8_t *key,
    qosa_uint32_t keybits,
    qosa_uint8_t  iv[16],
    qosa_uint8_t *inputdata,
    qosa_uint8_t *outputdata,
    int           inlen,
    int          *outlen
);


/*****************************************************************
* Function: qcm_aes_string_encryption
*
* Description:
* AES encryption algorithm
* This function is to hash and encrypt the user input data to get an encrypted result.
*
* Parameters:
* key            [in]  secret key
* key_length     [in]  secret key length
* inputdata      [in]  use inut data
* outputdata     [in]  hash result

* Return:
* void
*
*****************************************************************/
void qcm_aes_string_encryption(qosa_uint8_t key[16], int key_length, qosa_uint8_t inputdata[16], qosa_uint8_t outputdata[16]);

/*****************************************************************
* Function: qcm_aes_string_decryption
*
* Description:
* AES decryption algorithm
* This function decrypts the hash encrypted data to get the original data.
*
* Parameters:
* key            [in]  secret key
* key_length     [in]  secret key length
* inputdata      [in]  use inut data
* outputdata     [in]  hash result

* Return:
* void
*
*****************************************************************/
void qcm_aes_string_decryption(qosa_uint8_t key[16], int key_length, qosa_uint8_t inputdata[16], qosa_uint8_t outputdata[16]);

/**
 * @brief Encrypts input data using the AES algorithm in CBC (Cipher Block Chaining) mode,
 *        and automatically applies PKCS#7 padding.
 *
 * @param key         Pointer to the AES encryption key.
 * @param keybits     The length of the key in bits. Valid values are typically 128, 192, or 256.
 * @param iv          Pointer to the 16-byte Initialization Vector (IV).
 * @param inputdata   Pointer to the raw input data to be encrypted.
 * @param outputdata  Pointer to the output buffer.
 * @param inlen       Length of the input data.
 * @param outlen      Length of the output data.
 *
 */
void qcm_aes_string_cbc_encryption(
    qosa_uint8_t *key,
    qosa_uint32_t keybits,
    qosa_uint8_t  iv[16],
    qosa_uint8_t *inputdata,
    qosa_uint8_t *outputdata,
    int           inlen,
    int          *outlen
);

#endif
