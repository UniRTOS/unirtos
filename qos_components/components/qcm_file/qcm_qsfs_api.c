/*****************************************************************/ /**
* @file qcm_qsfs_api.c
* @brief
* @author larson.li@quectel.com
* @date 2024-04-21
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-04-21 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#include "qcm_qsfs_api.h"
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"

#include "mbedtls/aes.h"
#include "qosa_queue_list.h"
#include "qcm_file_api.h"
#include "qcm_file_cfg.h"
#include "qosa_dev.h"
#include "qosa_virtual_file.h"
#include "qcm_utils.h"

#define QOS_LOG_TAG LOG_TAG_FILE_API

typedef struct
{
    qosa_q_link_type_t  list;
    int                 fd;
    mbedtls_aes_context aes_dec;
    mbedtls_aes_context aes_enc;
    qosa_uint8_t        end_add_bytes;
    qosa_bool_t         write_mul_times;  //QOSA_FALSE
    qosa_bool_t         read_mul_times;   //QOSA_FALSE
    ///*qfupl/qfdwl upload and download end sign Encryption and decryption file operation completion flag, 1 - indicates encryption/decryption can be completed in one go, 0 - requires further encryption/decryption operations to continue*/
    qosa_bool_t  done_flag;        //QOSA_TRUE
    qosa_int64_t write_remainder;  //0
    qosa_int64_t read_remainder;   //0
    qosa_uint8_t write_cache[16];  /*Cache input data*/
    qosa_uint8_t write_cache_cnt;  /*Cached input data count*/
    qosa_uint8_t aes_cache[16];    /*The cache is not the intermediate data of the encryption and decryption multiples*/
    qosa_uint8_t aes_iv[16];
} qcm_sfs_proc_info_t;

static qosa_q_type_t g_sfs_process_list;
static qosa_mutex_t  g_sfs_process_list_lock;

#define QSFS_AESCBC_KEYBITS_128 128
#define QSFS_FILE_END_ADD_BYTES 2 /*byte0: number of padding bytes, byte1: encryption level (compatible with fixed password)*/

static const qosa_uint8_t aes_key_bak[16]
    = {0x01, 0x02, 0x03, 0x04, 0x0a, 0x0b, 0x0c, 0x0d, 0x51, 0x55, 0x45, 0x43, 0x54, 0x45, 0x4C, 0xff /*some 16 byte key*/};
static qosa_uint8_t       aes_key[16] = {0x01, 0x02, 0x03, 0x04, 0x0a, 0x0b, 0x0c, 0x0d, 0x51, 0x55, 0x45, 0x43, 0x54, 0x45, 0x4C, 0xff /*some 16 byte key*/};
static const qosa_uint8_t aes_iv[16]
    = {0x51, 0x55, 0x45, 0x43, 0x54, 0x45, 0x4C, 0x32, 0x30, 0x31, 0x39, 0x61, 0x65, 0x73, 0xb, 0xb /*some 16 byte iv*/}; /*It must not be modified later*/

static void qcm_sfs_file_cpuid_key(void)
{
    qosa_uint64_t cupid = 0;
    qosa_uint8_t  i = 0;

    if (QOSA_DEV_ERRID_SUCCESS == qosa_dev_get_cpu_uid(&cupid))
    {
        for (i = 0; i < 16; i++)
        {
            aes_key[i] = (qosa_uint8_t)((i << 4) | ((cupid >> (i * 4)) & 0xF)); /* CUP ID reverse order */
            // aes_key_bak[i] = (qosa_uint8_t)((i << 4) | ((cupid >> (i * 4)) & 0xF));			/* CUP ID reverse order */
#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL1)
            QLOGV("key[%d]:%x", i, aes_key[i]);
#endif
        }
    }
}

/*mbedtls aescbc encrypt, aes iv  must be global and cyclic*/
static int qcm_sfs_file_encrypt(qcm_sfs_proc_info_t *proc_ptr, unsigned char *output, const unsigned char *input, unsigned int len)
{
    int ret = 0;

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len=%d", len);
#endif
    ret = mbedtls_aes_crypt_cbc(&proc_ptr->aes_enc, MBEDTLS_AES_ENCRYPT, len, proc_ptr->aes_iv, input, output);

    return ret;
}

/*mbedtls aescbc decrypt, aes iv  must be global and cyclic*/
static int qcm_sfs_file_decrypt(qcm_sfs_proc_info_t *proc_ptr, unsigned char *output, const unsigned char *input, unsigned int len)
{
    int ret = 0;

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len=%d", len);
#endif
    ret = mbedtls_aes_crypt_cbc(&proc_ptr->aes_dec, MBEDTLS_AES_DECRYPT, len, proc_ptr->aes_iv, input, output);

    return ret;
}

static qcm_sfs_proc_info_t *qcm_sfs_file_proc_info_get(int fd)
{
    int                  cnt = 0;
    qcm_sfs_proc_info_t *proc_ptr = QOSA_NULL;

    qosa_mutex_lock(g_sfs_process_list_lock, QOSA_WAIT_FOREVER);
    cnt = qosa_q_cnt(&g_sfs_process_list);
    if (cnt == 0)
    {
        qosa_mutex_unlock(g_sfs_process_list_lock);
        return QOSA_NULL;
    }

    proc_ptr = (qcm_sfs_proc_info_t *)qosa_q_check(&g_sfs_process_list);
    while (proc_ptr != QOSA_NULL)
    {
        if (proc_ptr->fd == fd)
        {
            qosa_mutex_unlock(g_sfs_process_list_lock);
            return proc_ptr;
        }
        else
        {
            proc_ptr = qosa_q_next(&g_sfs_process_list, &proc_ptr->list);
        }
    }
    qosa_mutex_unlock(g_sfs_process_list_lock);
    return QOSA_NULL;
}

static void qcm_sfs_file_proc_info_deinit(int fd, qcm_sfs_proc_info_t *proc_ptr)
{
    if (QOSA_NULL == proc_ptr)
    {
        proc_ptr = qcm_sfs_file_proc_info_get(fd);
        if (proc_ptr == QOSA_NULL)
        {
            return;
        }
    }
#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("proc=%p", proc_ptr);
#endif
    mbedtls_aes_free(&proc_ptr->aes_enc);
    mbedtls_aes_free(&proc_ptr->aes_dec);
}

/*Write the file all at once*/
static int qcm_sfs_file_write_onetime(int fd, const void *write_buf, qosa_uint32_t write_size)
{
    int                  len = 0;
    int                  tmp_write_size = 0;
    int                  tmp_write_remainder = 0;
    unsigned char       *cipher_out = QOSA_NULL;
    unsigned char       *write_buf_in = QOSA_NULL;
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    if (fd < 0)
    {
        QLOGE("fd=%d", fd);
        return -1;
    }

    tmp_write_remainder = (write_size + proc_ptr->write_cache_cnt) % 16; /*Calculate the number of extra data points in multiples*/
    if ((write_size + proc_ptr->write_cache_cnt) < 16)
        tmp_write_size = 16 + proc_ptr->end_add_bytes;
    else if (0 == tmp_write_remainder)
        tmp_write_size = write_size + proc_ptr->write_cache_cnt + proc_ptr->end_add_bytes; /*If it is exactly a multiple of 16, then write an additional 1-byte length data at the end.*/
    else
        tmp_write_size = write_size + proc_ptr->write_cache_cnt - tmp_write_remainder + 16 + proc_ptr->end_add_bytes;

    write_buf_in = qosa_malloc(tmp_write_size + 1);
    if (write_buf_in == QOSA_NULL)
    {
        QLOGE("memIn");
        goto exit;
    }
    qosa_memset(write_buf_in, 0, tmp_write_size + 1);
    cipher_out = qosa_malloc(tmp_write_size + 1);
    if (cipher_out == QOSA_NULL)
    {
        QLOGE("memOut");
        goto exit;
    }
    qosa_memset(cipher_out, 0, tmp_write_size + 1);
    qosa_memcpy(write_buf_in, &proc_ptr->write_cache[0], proc_ptr->write_cache_cnt); /*Copy the remaining data to be written from the last time*/
    qosa_memcpy(write_buf_in + proc_ptr->write_cache_cnt, write_buf, write_size);    /*Copy the data to be written*/
    if (0 != tmp_write_remainder)
    {
        qosa_memset(proc_ptr->aes_cache, 0x75, sizeof(proc_ptr->aes_cache));
        qosa_memcpy(write_buf_in + write_size + proc_ptr->write_cache_cnt, proc_ptr->aes_cache, 16 - tmp_write_remainder); /*Copy the data to be filled*/
    }

    write_buf_in[tmp_write_size] = '\0';
    qcm_sfs_file_encrypt(proc_ptr, cipher_out, write_buf_in, tmp_write_size - 1); /*Encrypted data*/
    /*The length of the tail byte is not encrypted; write the number of padding data.*/
    if (0 == tmp_write_remainder)
        cipher_out[tmp_write_size - proc_ptr->end_add_bytes] = '0';
    else
        qosa_sprintf((char *)&cipher_out[tmp_write_size - proc_ptr->end_add_bytes], "%x", 16 - tmp_write_remainder);
    //MSG_SPRINTF_2(MSG_SSID_DS_ATCOP, MSG_LEGACY_HIGH, "%s, rlen=%d",__FUNCTION__, qcm_utils_hex_to_num(cipher_out[tmp_write_size-1]));
    cipher_out[tmp_write_size] = '\0';
    len = qosa_vfs_write(fd, (void *)cipher_out, (qosa_size_t)tmp_write_size); /*while also writing the length of the tail byte*/
    if (len < 0)
    {
        QLOGV("len:%d", len);
    }
    else
        len = write_size; /*Modify to the length of data to be written*/

exit:
    if (cipher_out != QOSA_NULL)
    {
        qosa_free(cipher_out);
        cipher_out = QOSA_NULL;
    }

    if (write_buf_in != QOSA_NULL)
    {
        qosa_free(write_buf_in);
        write_buf_in = QOSA_NULL;
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len:%d, size:%d, remd=%d", len, tmp_write_size, tmp_write_remainder);
#endif
    return len;
}

/*Write to the file several times, this is ending the last write*/
static int qcm_sfs_file_write_lasttimes(int fd, const void *write_buf, qosa_uint32_t write_size)
{
    int            len = 0;
    int            tmp_write_size = 0;
    int            tmp_write_remainder = 0;
    unsigned char *cipher_out = QOSA_NULL;
    unsigned char *write_buf_in = QOSA_NULL;
    unsigned int   write_remainder = (write_size >> 24) & 0xFF;
    write_size = write_size & 0xFFFFFF;
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    if (fd < 0)
    {
        QLOGE("fd=%d", fd);
        return -1;
    }
    tmp_write_remainder = (write_size + write_remainder + proc_ptr->write_cache_cnt) % 16; /*Remaining number of unencrypted data entries*/
    if (0 == tmp_write_remainder)
        tmp_write_size = write_size + write_remainder + proc_ptr->write_cache_cnt + proc_ptr->end_add_bytes;
    else if ((write_size + write_remainder + proc_ptr->write_cache_cnt) < 16)
        tmp_write_size = 16 + proc_ptr->end_add_bytes;
    else
        tmp_write_size = write_size + write_remainder + proc_ptr->write_cache_cnt + 16 + proc_ptr->end_add_bytes - tmp_write_remainder; /*Total length to be written*/
    write_buf_in = qosa_malloc(tmp_write_size + 1);
    if (write_buf_in == QOSA_NULL)
    {
        QLOGE("memIn");
        goto exit;
    }
    qosa_memset(write_buf_in, 0, tmp_write_size + 1);
    cipher_out = qosa_malloc(tmp_write_size + 1);
    if (cipher_out == QOSA_NULL)
    {
        QLOGE("memOut");
        goto exit;
    }

    qosa_memset(cipher_out, 0, tmp_write_size + 1);
    qosa_memcpy(write_buf_in, proc_ptr->aes_cache, write_remainder);                                    /*Copy the remaining data to be written from the last time*/
    qosa_memcpy(write_buf_in + write_remainder, &proc_ptr->write_cache[0], proc_ptr->write_cache_cnt);  /*Copy the remaining data to be written from the last time*/
    if (write_buf != QOSA_NULL)
        qosa_memcpy(write_buf_in + write_remainder + proc_ptr->write_cache_cnt, write_buf, write_size); /*Copy the current multiple write data*/
    if (0 != tmp_write_remainder) /*No padding for multiples, Beyond qosa_malloc causes dump, Not fully PKCS7Padding*/
    {
        qosa_memset(write_buf_in + write_remainder + proc_ptr->write_cache_cnt + write_size, 0x75, 16 - tmp_write_remainder); /*Copy the data to be filled*/
    }
    write_buf_in[tmp_write_size] = '\0';

    qcm_sfs_file_encrypt(proc_ptr, cipher_out, write_buf_in, tmp_write_size - proc_ptr->end_add_bytes); /*Encrypted data*/
    if (0 == tmp_write_remainder)
        cipher_out[tmp_write_size - proc_ptr->end_add_bytes] = '0';
    else
        qosa_sprintf((char *)&cipher_out[tmp_write_size - proc_ptr->end_add_bytes], "%x", 16 - tmp_write_remainder);

    cipher_out[tmp_write_size] = '\0';
    len = qosa_vfs_write((int)fd, (void *)cipher_out, (qosa_size_t)tmp_write_size); /*Write 1 byte of length data at the end*/
    if (len < 0)
    {
        QLOGV("len:%d", len);
    }
    else
    {
        len = write_size; /*Modify to the length of data to be written*/
        proc_ptr->write_cache_cnt = 0;
        qosa_memset(&proc_ptr->write_cache[0], 0, sizeof(proc_ptr->write_cache));
    }

exit:
    if (cipher_out != QOSA_NULL)
    {
        qosa_free(cipher_out);
        cipher_out = QOSA_NULL;
    }

    if (write_buf_in != QOSA_NULL)
    {
        qosa_free(write_buf_in);
        write_buf_in = QOSA_NULL;
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len:%d, size:%d, remd=%d", len, tmp_write_size, tmp_write_remainder);
#endif
    return len;
}

/*Write to the file several times, this is continue to write*/
static int qcm_sfs_file_write_middletimes(int fd, const void *write_buf, qosa_uint32_t write_size)
{
    int                  len = 0;
    int                  tmp_write_size = 0;
    int                  tmp_write_remainder = 0;
    unsigned char       *cipher_out = QOSA_NULL;
    unsigned char       *write_buf_in = QOSA_NULL;
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    if (fd < 0)
    {
        QLOGE("fd=%d", fd);
        return -1;
    }
    unsigned int write_remainder = (write_size >> 24) & 0xFF;
    write_size = write_size & 0xFFFFFF;
    tmp_write_remainder = (write_size + write_remainder + proc_ptr->write_cache_cnt) % 16; /*The number of additional unencrypted data entries*/
    if (0 == tmp_write_remainder)
        tmp_write_size = (write_size + write_remainder + proc_ptr->write_cache_cnt);
    else
        tmp_write_size = write_size + write_remainder + proc_ptr->write_cache_cnt - tmp_write_remainder; /*This time, write the total length first; any excess data will not be written for now.*/

    write_buf_in = qosa_malloc(tmp_write_size + tmp_write_remainder + 1);
    if (write_buf_in == QOSA_NULL)
    {
        QLOGE("memIn");
        goto exit;
    }
    qosa_memset(write_buf_in, 0, tmp_write_size + 1);
    cipher_out = qosa_malloc(tmp_write_size + 1);
    if (cipher_out == QOSA_NULL)
    {
        QLOGE("memOut");
        goto exit;
    }

    qosa_memset(cipher_out, 0, tmp_write_size + 1);
    qosa_memcpy(write_buf_in, proc_ptr->aes_cache, write_remainder);                                   /*Copy the remaining data to be written from the last time*/
    qosa_memcpy(write_buf_in + write_remainder, &proc_ptr->write_cache[0], proc_ptr->write_cache_cnt); /*Copy the remaining data to be written from the last time*/
    qosa_memcpy(write_buf_in + write_remainder + proc_ptr->write_cache_cnt, write_buf, write_size);    /*Copy the current write data*/

    qosa_memset(proc_ptr->aes_cache, 0, sizeof(proc_ptr->aes_cache));
    proc_ptr->write_cache_cnt = 0;
    qosa_memset(&proc_ptr->write_cache[0], 0, sizeof(proc_ptr->write_cache));
    if (tmp_write_remainder)
    {
        qosa_memcpy(proc_ptr->aes_cache, write_buf_in + tmp_write_size, tmp_write_remainder); /*Cache excess data*/
    }
    qosa_memset(write_buf_in + tmp_write_size, 0, tmp_write_remainder + 1);

    qcm_sfs_file_encrypt(proc_ptr, cipher_out, write_buf_in, tmp_write_size); /*Encrypted data*/
    cipher_out[tmp_write_size] = '\0';

    len = qosa_vfs_write(fd, (void *)cipher_out, (qosa_size_t)tmp_write_size); /*, without considering failure retry*/
    if (len < 0)
    {
        tmp_write_remainder = 0;
        QLOGV("len:%d", len);
    }
    else
    {
        len = write_size; /*Modify to the length of data to be written*/
    }

exit:
    if (cipher_out != QOSA_NULL)
    {
        qosa_free(cipher_out);
        cipher_out = QOSA_NULL;
    }

    if (write_buf_in != QOSA_NULL)
    {
        qosa_free(write_buf_in);
        write_buf_in = QOSA_NULL;
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len:%d, size:%d, remd=%d", len, tmp_write_size, tmp_write_remainder);
#endif
    return (tmp_write_remainder << 24) + len;
}

/*Write to the file several times, this is the first write*/
static int qcm_sfs_file_write_firsttimes(int fd, const void *write_buf, qosa_uint32_t write_size)
{
    int                  len = 0;
    int                  tmp_write_size = 0;
    int                  tmp_write_remainder = 0;
    unsigned char       *cipher_out = QOSA_NULL;
    unsigned char       *write_buf_in = QOSA_NULL;
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    if (fd < 0)
    {
        QLOGE("fd=%d", fd);
        return -1;
    }

    tmp_write_remainder = (write_size + proc_ptr->write_cache_cnt) % 16; /*Remaining number of unencrypted data entries*/
    if (0 == tmp_write_remainder)                                        /*Excess byte count*/
        tmp_write_size = write_size + proc_ptr->write_cache_cnt;
    else
        tmp_write_size = write_size + proc_ptr->write_cache_cnt - tmp_write_remainder;
    /*Extra bytes will be written next time*/
    write_buf_in = qosa_malloc(tmp_write_size + 1);
    if (write_buf_in == QOSA_NULL)
    {
        QLOGE("memIn");
        goto exit;
    }
    qosa_memset(write_buf_in, 0, tmp_write_size + 1);
    cipher_out = qosa_malloc(tmp_write_size + 1);
    if (cipher_out == QOSA_NULL)
    {
        QLOGE("memOut");
        goto exit;
    }

    qosa_memset(cipher_out, 0, tmp_write_size + 1);
    qosa_memcpy(write_buf_in, &proc_ptr->write_cache[0], proc_ptr->write_cache_cnt);                              /*Copy the remaining data to be written from the last time*/
    qosa_memcpy(write_buf_in + proc_ptr->write_cache_cnt, write_buf, tmp_write_size - proc_ptr->write_cache_cnt); /*Copy the current multiple write data*/
    write_buf_in[tmp_write_size] = '\0';

    qcm_sfs_file_encrypt(proc_ptr, cipher_out, write_buf_in, tmp_write_size); /*Encrypted data*/
    cipher_out[tmp_write_size] = '\0';
    len = qosa_vfs_write(fd, (void *)cipher_out, (qosa_size_t)tmp_write_size);
    if (len < 0)
    {
        tmp_write_remainder = 0;
        QLOGV("len:%d", len);
    }
    else
    {
        qosa_memset(proc_ptr->aes_cache, 0, sizeof(proc_ptr->aes_cache));
        if (tmp_write_size == proc_ptr->write_cache_cnt)
            qosa_memcpy(proc_ptr->aes_cache, write_buf, tmp_write_remainder);                                                        /*Cache the excess data*/
        else
            qosa_memcpy(proc_ptr->aes_cache, (char *)write_buf + (tmp_write_size - proc_ptr->write_cache_cnt), tmp_write_remainder); /*Cache the excess data*/

        proc_ptr->write_cache_cnt = 0;
        qosa_memset(&proc_ptr->write_cache[0], 0, sizeof(proc_ptr->write_cache));
        len = write_size; /*Modified to the length of data to be written, without considering write failures*/
    }

exit:
    if (cipher_out != QOSA_NULL)
    {
        qosa_free(cipher_out);
        cipher_out = QOSA_NULL;
    }

    if (write_buf_in != QOSA_NULL)
    {
        qosa_free(write_buf_in);
        write_buf_in = QOSA_NULL;
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len:%d, size:%d, remd=%d", len, tmp_write_size, tmp_write_remainder);
#endif
    return (tmp_write_remainder << 24) + len;
}

/*read the file all at once*/
static int qcm_sfs_file_read_onetime(int fd, void *read_buf, qosa_uint32_t read_size)
{
    int                  len = 0;
    int                  tmp_read_size = 0;
    int                  tmp_read_remainder = 0;
    unsigned char       *read_buf_out = QOSA_NULL;
    unsigned char       *decrypted_plain = QOSA_NULL;
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    tmp_read_remainder = read_size % 16;
    if (read_size < 16)
        tmp_read_size = 16;
    else if (0 == tmp_read_remainder)
        tmp_read_size = read_size;
    else
        tmp_read_size = read_size - tmp_read_remainder + 16;

    read_buf_out = qosa_malloc(tmp_read_size + 1);
    if (read_buf_out == QOSA_NULL)
    {
        QLOGE("memIn");
        goto exit;
    }
    qosa_memset(read_buf_out, 0, tmp_read_size + 1);
    decrypted_plain = qosa_malloc(tmp_read_size + 1);
    if (decrypted_plain == QOSA_NULL)
    {
        QLOGE("memOut");
        goto exit;
    }

    qosa_memset(decrypted_plain, 0, tmp_read_size + 1);
    len = qosa_vfs_read(fd, (void *)decrypted_plain, (qosa_size_t)tmp_read_size);
#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len=%d, size=%d", len, tmp_read_size);
#endif
    if (len > 0)
    {
        len = read_size;
        decrypted_plain[tmp_read_size] = '\0';
        qcm_sfs_file_decrypt(proc_ptr, read_buf_out, decrypted_plain, tmp_read_size); /*Decrypted data*/
        qosa_memcpy(read_buf, read_buf_out, read_size);
    }
exit:
    if (decrypted_plain != QOSA_NULL)
    {
        qosa_free(decrypted_plain);
        decrypted_plain = QOSA_NULL;
    }

    if (read_buf_out != QOSA_NULL)
    {
        qosa_free(read_buf_out);
        read_buf_out = QOSA_NULL;
    }

    return len;
}

/*read to the file several times, this is ending the last read*/
static int qcm_sfs_file_read_lasttimes(int fd, void *read_buf, qosa_uint32_t read_size)
{
    int                  len = 0;
    int                  tmp_read_size = 0;
    int                  tmp_read_remainder = 0;
    unsigned char       *read_buf_out = QOSA_NULL;
    unsigned char       *decrypted_plain = QOSA_NULL;
    unsigned int         read_remainder = (read_size >> 24) & 0xFF;
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    read_size = read_size & 0xFFFFFF;
    tmp_read_remainder = (read_size - read_remainder) % 16; /*The additional multiple byte count*/
    if ((read_size - read_remainder) < 16)
        tmp_read_size = 16;
    else if (0 == tmp_read_remainder)
        tmp_read_size = read_size - read_remainder;
    else
        tmp_read_size = read_size - read_remainder - tmp_read_remainder + 16; /*The length of the multiple bytes to be read this time*/

    read_buf_out = qosa_malloc(tmp_read_size + 1);
    if (read_buf_out == QOSA_NULL)
    {
        QLOGE("memIn");
        goto exit;
    }
    qosa_memset(read_buf_out, 0, tmp_read_size + 1);
    decrypted_plain = qosa_malloc(tmp_read_size + 1);
    if (decrypted_plain == QOSA_NULL)
    {
        QLOGE("memOut");
        goto exit;
    }
    qosa_memset(decrypted_plain, 0, tmp_read_size + 1);
    len = qosa_vfs_read(fd, (void *)decrypted_plain, (qosa_size_t)tmp_read_size);
#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len=%d, size=%d", len, tmp_read_size);
#endif
    if (len > 0)
    {
        qcm_sfs_file_decrypt(proc_ptr, read_buf_out, decrypted_plain, tmp_read_size);             /*Decrypted data*/
        qosa_memcpy(read_buf, proc_ptr->aes_cache, read_remainder);                               /*Copy the previously over-read cached data*/
        qosa_memcpy((char *)read_buf + read_remainder, read_buf_out, read_size - read_remainder); /*Copy the other part of the data you want to read*/
        len = read_size;                                                                          /*Modify to the desired read length*/
        read_remainder = 0;
    }

exit:
    if (decrypted_plain != QOSA_NULL)
    {
        qosa_free(decrypted_plain);
        decrypted_plain = QOSA_NULL;
    }

    if (read_buf_out != QOSA_NULL)
    {
        qosa_free(read_buf_out);
        read_buf_out = QOSA_NULL;
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len=%d, size=%d, remd=%d", len, tmp_read_size, read_remainder);
#endif
    return len;
}

/*read to the file several times, this is continue to read*/
static int qcm_sfs_file_read_middletimes(int fd, void *read_buf, qosa_uint32_t read_size)
{
    int                  len = 0;
    int                  tmp_read_size = 0;
    int                  tmp_read_remainder = 0;
    unsigned char       *read_buf_out = QOSA_NULL;
    unsigned char       *decrypted_plain = QOSA_NULL;
    unsigned int         read_remainder = (read_size >> 24) & 0xFF;
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    read_size = read_size & 0xFFFFFF;
    tmp_read_remainder = (read_size - read_remainder) % 16; /*The additional multiple byte count*/
    if (0 == tmp_read_remainder)
        tmp_read_size = read_size - read_remainder;
    else
        tmp_read_size = read_size - read_remainder - tmp_read_remainder + 16; /*The length of the multiple bytes to be read this time*/
    read_buf_out = qosa_malloc(tmp_read_size + 1);
    if (read_buf_out == QOSA_NULL)
    {
        QLOGE("memIn");
        goto exit;
    }
    qosa_memset(read_buf_out, 0, tmp_read_size + 1);
    decrypted_plain = qosa_malloc(tmp_read_size + 1);
    if (decrypted_plain == QOSA_NULL)
    {
        QLOGE("memOut");
        goto exit;
    }
    qosa_memset(decrypted_plain, 0, tmp_read_size + 1);
    len = qosa_vfs_read(fd, (void *)decrypted_plain, (qosa_size_t)tmp_read_size);
    if (len < 0)
    {
        read_remainder = 0;
        QLOGE("len:%d", len);
        goto exit;
    }
    qcm_sfs_file_decrypt(proc_ptr, read_buf_out, decrypted_plain, tmp_read_size);             /*Decrypted data*/
    qosa_memcpy(read_buf, proc_ptr->aes_cache, read_remainder);                               /*Copy the previously over-read cached data*/
    qosa_memcpy((char *)read_buf + read_remainder, read_buf_out, read_size - read_remainder); /*Copy the other part of data you want to read*/
    if (0 == tmp_read_remainder)
    {
        read_remainder = 0;
    }
    else
    {
        read_remainder = 16 - tmp_read_remainder;                                                        /*Calculate the length of the cached data for this multi-degree*/
        qosa_memcpy(proc_ptr->aes_cache, read_buf_out + tmp_read_size - read_remainder, read_remainder); /*Cache the extra read data*/
    }
    len = read_size;                                                                                     /*Modify to the desired read length*/

exit:
    if (decrypted_plain != QOSA_NULL)
    {
        qosa_free(decrypted_plain);
        decrypted_plain = QOSA_NULL;
    }

    if (read_buf_out != QOSA_NULL)
    {
        qosa_free(read_buf_out);
        read_buf_out = QOSA_NULL;
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len=%d, size=%d, remd=%d", len, tmp_read_size, read_remainder);
#endif
    return (read_remainder << 24) + len;
}

/*read to the file several times, this is the first read*/
static int qcm_sfs_file_read_firsttimes(int fd, void *read_buf, qosa_uint32_t read_size)
{
    int                  len = 0;
    int                  tmp_read_size = 0;
    int                  tmp_read_remainder = 0;
    int                  read_remainder = 0;
    unsigned char       *read_buf_out = QOSA_NULL;
    unsigned char       *decrypted_plain = QOSA_NULL;
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    tmp_read_remainder = read_size % 16;
    if (0 == tmp_read_remainder)
        tmp_read_size = read_size;
    else
        tmp_read_size = read_size - tmp_read_remainder + 16;
    read_buf_out = qosa_malloc(tmp_read_size + 1);
    if (read_buf_out == QOSA_NULL)
    {
        QLOGE("memIn");
        goto exit;
    }
    qosa_memset(read_buf_out, 0, tmp_read_size + 1);
    decrypted_plain = qosa_malloc(tmp_read_size + 1);
    if (decrypted_plain == QOSA_NULL)
    {
        QLOGE("memOut");
        goto exit;
    }

    qosa_memset(decrypted_plain, 0, tmp_read_size + 1);
    len = qosa_vfs_read(fd, (void *)decrypted_plain, (qosa_size_t)tmp_read_size);
    if (len < 0)
    {
        read_remainder = 0;
        QLOGE("len:%d", len);
        goto exit;
    }
    qcm_sfs_file_decrypt(proc_ptr, read_buf_out, decrypted_plain, tmp_read_size); /*Decrypted data*/
    qosa_memcpy(read_buf, read_buf_out, read_size);
    len = read_size;
    if (0 == tmp_read_remainder)
        read_remainder = 0;
    else
    {
        qosa_memcpy(proc_ptr->aes_cache, read_buf_out + read_size, 16 - tmp_read_remainder); /*Cache the extra read data*/
        read_remainder = 16 - tmp_read_remainder;
    }

exit:
    if (decrypted_plain != QOSA_NULL)
    {
        qosa_free(decrypted_plain);
        decrypted_plain = QOSA_NULL;
    }

    if (read_buf_out != QOSA_NULL)
    {
        qosa_free(read_buf_out);
        read_buf_out = QOSA_NULL;
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len=%d, size=%d, remd=%d", len, tmp_read_size, read_remainder);
#endif
    return (read_remainder << 24) + len;
}

void qcm_sfs_file_proc_info_delete(int fd)
{
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    if (QOSA_NULL == proc_ptr)
    {
        return;
    }

    if ((QOSA_VFS_O_WRONLY | QOSA_VFS_O_CREAT | QOSA_VFS_O_TRUNC) == qcm_file_get_mode_by_fd(fd))
    {
        qcm_sfs_file_write_lasttimes(fd, QOSA_NULL, proc_ptr->write_remainder << 24);
    }
    qcm_sfs_file_proc_info_deinit(fd, proc_ptr);

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("proc=%p", proc_ptr);
#endif
    qosa_q_delete(&g_sfs_process_list, &(proc_ptr->list));
    qosa_free(proc_ptr);
}

/* return 0 for successfully, -1 for failed */
int qcm_sfs_file_proc_info_init(int fd, int mode)
{
    struct qosa_vfs_stat_t st = {0};
    qcm_sfs_proc_info_t   *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    if (QOSA_NULL == g_sfs_process_list_lock)
    {
        QLOGE("unInit");
        return -1;
    }

    qosa_mutex_lock(g_sfs_process_list_lock, QOSA_WAIT_FOREVER);
    if (proc_ptr == QOSA_NULL)
    {
        proc_ptr = (qcm_sfs_proc_info_t *)qosa_malloc(sizeof(qcm_sfs_proc_info_t));
        if (proc_ptr == QOSA_NULL)
        {
            qosa_mutex_unlock(g_sfs_process_list_lock);
            return -1;
        }
        qosa_memset(proc_ptr, 0, sizeof(qcm_sfs_proc_info_t));
        proc_ptr->fd = fd;

        qosa_q_link(proc_ptr, &proc_ptr->list);
        qosa_q_put(&g_sfs_process_list, &proc_ptr->list);
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("proc=%p, mode:%x", proc_ptr, mode);
#endif

    proc_ptr->end_add_bytes = 1;
    if (QOSA_VFS_O_RDONLY == mode)
    {
        if (qosa_vfs_fstat(fd, &st) < 0)
        {
            QLOGE("fstat");
            qosa_q_delete(&g_sfs_process_list, &proc_ptr->list);
            qosa_mutex_unlock(g_sfs_process_list_lock);
            qosa_free(proc_ptr);
            return -1;
        }

        if (0 == (st.st_size & 0xF) || st.st_size < 0xF || (st.st_size & 0xF) > QSFS_FILE_END_ADD_BYTES)
        {
            QLOGE("st_size:%d", st.st_size);
            qosa_q_delete(&g_sfs_process_list, &proc_ptr->list);
            qosa_mutex_unlock(g_sfs_process_list_lock);
            qosa_free(proc_ptr);
            return -1;
        }

        if (0 == qosa_memcmp(aes_key, aes_key_bak, 16))
        {
            QLOGV("cpuid_key");
            qcm_sfs_file_cpuid_key();
            proc_ptr->end_add_bytes = 2;
        }
    }
    else if (0 == qosa_memcmp(aes_key, aes_key_bak, 16))
    {
        QLOGV("cpuid_key");
        qcm_sfs_file_cpuid_key(); /* SDK should not set! */
        proc_ptr->end_add_bytes = 2;
    }

    mbedtls_aes_init(&proc_ptr->aes_enc);
    mbedtls_aes_setkey_enc(&proc_ptr->aes_enc, aes_key, QSFS_AESCBC_KEYBITS_128);
    mbedtls_aes_init(&proc_ptr->aes_dec);
    mbedtls_aes_setkey_dec(&proc_ptr->aes_dec, aes_key, QSFS_AESCBC_KEYBITS_128);
    qosa_memcpy(proc_ptr->aes_iv, aes_iv, 16);

    qosa_mutex_unlock(g_sfs_process_list_lock);
    return 0;
}

/* set sfs file done flag */
void qcm_sfs_file_set_done_flag(int fd, qosa_bool_t flag)
{
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);
    if (QOSA_NULL != proc_ptr)
    {
        proc_ptr->done_flag = flag; /*Encryption/decryption file operation completion flag, QOSA_TRUE - indicates that encryption/decryption can be completed in one go, QOSA_FALSE - indicates that encryption/decryption operations need to continue*/
#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
        QLOGV("flag=%d", flag);
#endif
    }
}

/* set sfs key */
int qcm_sfs_file_set_aes_key(void *key, qosa_uint8_t len)
{
    if (QOSA_NULL == g_sfs_process_list_lock)
    {
        QLOGE("unInit");
        return -1;
    }

    qosa_mutex_lock(g_sfs_process_list_lock, QOSA_WAIT_FOREVER);
    if (QOSA_NULL == key || len > 16 || len == 0)
    {
        qosa_mutex_unlock(g_sfs_process_list_lock);
        return -1;
    }

    qosa_memcpy(aes_key, aes_key_bak, 16);
    qosa_memcpy(aes_key, key, len);

    qosa_mutex_unlock(g_sfs_process_list_lock);
    return 0;
}

/*Gets the actual size of the encrypted file*/
int qcm_sfs_fstat(int fd, char *fname, char *path, struct qosa_vfs_stat_t *st)
{
    int          ret = -1;
    char         read_buf[4] = {0};
    qosa_int64_t curt_offset = -1; /*-1 indicates the current file is not open*/
    qosa_int64_t num = 0;

    if ((fd < 0 && (QOSA_NULL == fname || QOSA_NULL == path)) || QOSA_NULL == st)
    {
        QLOGE("invalid");
        return -1;
    }

    if (fd > 0 || (fd = file_check_is_opened(fname)) > 0)  // opened
    {
        if (QOSA_VFS_O_RDONLY != qcm_file_get_mode_by_fd(fd))
        {
            QLOGE("unSport");
            return -1;
        }
        if ((curt_offset = qosa_vfs_lseek(fd, (qosa_int64_t)0, SEEK_CUR)) < 0) /*Record the current position of the opened file*/
        {
            QLOGE("lseek");
            return -1;
        }
    }
    else
    {
        fd = qosa_vfs_open((const char *)path, QOSA_VFS_O_RDONLY);
        if (fd < 0)
        {
            QLOGE("open");
            return -1;
        }
    }

    if (qosa_vfs_fstat(fd, st) < 0)
    {
        QLOGE("fstat");
        goto exit;
    }
    QLOGV("st_size=%d", st->st_size);

    /*A multiple of 16 bytes, less than 16 bytes, with a remainder greater than the number of bytes of the final appended flag when divided by 16, is considered abnormal/non-SFS file.*/
    if (-1 == curt_offset && (0 == (st->st_size & 0xF) || st->st_size < 0xF || (st->st_size & 0xF) > QSFS_FILE_END_ADD_BYTES))
    {
        goto exit;
    }

    /*Offset the position to read the appended flag*/
    if (qosa_vfs_lseek(fd, st->st_size - (st->st_size & 0xF), SEEK_SET) < 0)
    {
        QLOGE("lseek");
        goto exit;
    }

    /*Read append flag*/
    if (qosa_vfs_read(fd, (void *)read_buf, (qosa_size_t)1) < 0)
    {
        QLOGE("read");
        goto exit;
    }

    num = st->st_size - (st->st_size & 0xF) - qcm_utils_hex_to_num(read_buf[0]);
    if (num < 0)
    {
        st->st_size = 0;
    }
    else
    {
        st->st_size = num; /*Actual file length*/
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("buf=%s, st_size=%d, remainder=%c", read_buf, st->st_size, read_buf[0]);
#endif
    ret = 0;

exit:
    if (curt_offset >= 0)
    {
        if (qosa_vfs_lseek(fd, curt_offset, SEEK_SET) < 0) /*Restore the current position of an opened file*/
        {
            QLOGE("lseek");
            return -1;
        }
    }
    else if (fd > 0) /*Close the file if it was not originally opened.*/
    {
        qosa_vfs_close(fd);
    }

    return ret;
}

/* write file */
qosa_ssize_t qcm_sfs_write(int fd, const void *data, qosa_size_t size)
{
    int                  len = 0;
    qcm_sfs_proc_info_t *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    if (fd < 0 || QOSA_NULL == data || size <= 0 || size > 0xFFFFFF)
    {
        QLOGE("invalid");
        return -1;
    }

    if (QOSA_NULL == proc_ptr)
    {
        QLOGE("QOSA_NULL");
        return -1;
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("fd=%d, size=%d, cache_cnt:%d, remainder=%d", fd, size, proc_ptr->write_cache_cnt, proc_ptr->write_remainder);
#endif
    if (QOSA_FALSE == proc_ptr->write_mul_times && (QOSA_TRUE == proc_ptr->done_flag)) /*Write the file in one go*/
    {
        len = qcm_sfs_file_write_onetime(fd, data, size);
        qcm_sfs_file_proc_info_deinit(fd, proc_ptr);
    }
    else if (QOSA_FALSE == proc_ptr->write_mul_times) /*First time writing data to a file*/
    {
        if ((size > 16) || ((proc_ptr->write_cache_cnt + size) > 16))
        {
            len = qcm_sfs_file_write_firsttimes(fd, data, size);
            if (len > 0)
            {
                proc_ptr->write_remainder = (len >> 24) & 0xFF;
                len &= 0xFFFFFF;
                proc_ptr->write_mul_times = QOSA_TRUE;
            }
        }
        else
        {
            qosa_memcpy(&proc_ptr->write_cache[proc_ptr->write_cache_cnt], data, size); /*Copy the remaining data to be written from the last time*/
            proc_ptr->write_cache_cnt += size;

            len = size; /*Returns the length of data written, but no actual write occurred.*/
        }
    }
    else if (QOSA_TRUE == proc_ptr->done_flag) /*The last time data writing to the file is completed.*/
    {
        size = size + (proc_ptr->write_remainder << 24);
        len = qcm_sfs_file_write_lasttimes(fd, data, size);
        if (len > 0)
        {
            qcm_sfs_file_proc_info_deinit(fd, proc_ptr);
        }
    }
    else
    {
        if ((size > 16) || ((proc_ptr->write_remainder + proc_ptr->write_cache_cnt + size) > 16))
        {
            size = size + (proc_ptr->write_remainder << 24);
            len = qcm_sfs_file_write_middletimes(fd, data, size);
            if (len > 0)
            {
                proc_ptr->write_remainder = (len >> 24) & 0xFF;
                len &= 0xFFFFFF;
            }
        }
        else
        {
            qosa_memcpy(&proc_ptr->write_cache[proc_ptr->write_cache_cnt], data, size); /*Copy the remaining data to be written from the last time*/
            proc_ptr->write_cache_cnt += size;

            len = size; /*Returns the length of data written, but no actual write occurred.*/
        }
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len:%d, cache_cnt:%d, remainder=%d", len, proc_ptr->write_cache_cnt, proc_ptr->write_remainder);
#endif
    return len;
}

/* read file */
qosa_ssize_t qcm_sfs_read(int fd, void *dst, qosa_size_t size)
{
    int                    len = 0;
    struct qosa_vfs_stat_t st = {0};
    qosa_int64_t           curt_offset = 0;
    qosa_int64_t           fill_zore_size = 0;
    qcm_sfs_proc_info_t   *proc_ptr = qcm_sfs_file_proc_info_get(fd);

    if (fd < 0 || QOSA_NULL == dst || size <= 0)
    {
        QLOGE("invalid");
        return -1;
    }

    if (QOSA_NULL == proc_ptr)
    {
        QLOGE("QOSA_NULL");
        return -1;
    }

    if (QOSA_FALSE == proc_ptr->read_mul_times)
    {
        if (qosa_vfs_lseek(fd, 0, SEEK_SET) < 0)
        {
            QLOGE("lseek");
            return -1;
        }
    }

    if (qcm_sfs_fstat(fd, QOSA_NULL, QOSA_NULL, &st) < 0) /*The start of valid data*/
    {
        QLOGE("fstat");
        return -1;
    }

    curt_offset = qosa_vfs_lseek(fd, 0, SEEK_CUR);
    if (curt_offset < 0)
    {
        QLOGE("lseek");
        return -1;
    }
#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("offset=%d, remainder=%d", curt_offset, proc_ptr->read_remainder);
#endif

    /*Reading a file does not read the appended flag at the end.*/
    if (curt_offset > (qosa_int64_t)st.st_size)      /*The file has been fully read, and this file is padded with 0x00.*/
    {
        fill_zore_size = (curt_offset - st.st_size); /*Number of 0x00 bytes to fill*/
    }

    curt_offset -= proc_ptr->read_remainder; /*The actual read position*/
    if (curt_offset >= (qosa_int64_t)st.st_size)
    {
        QLOGE("read end!");
        return -1;
    }

    if (size > st.st_size - curt_offset)
    {
        size = (st.st_size - curt_offset);
        proc_ptr->done_flag = QOSA_TRUE;
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("fd=%d, size=%d, remainder=%d", fd, size, proc_ptr->read_remainder);
#endif

    if ((qosa_size_t)(proc_ptr->read_remainder - fill_zore_size) >= size)
    {
        qosa_uint8_t i = 0;
        proc_ptr->read_remainder -= size;
        qosa_memcpy(dst, proc_ptr->aes_cache, size);
        for (i = 0; i < proc_ptr->read_remainder; i++)
        {
            proc_ptr->aes_cache[i] = proc_ptr->aes_cache[i + size];
        }
        proc_ptr->aes_cache[i] = 0;
        return size;
    }

    if ((QOSA_FALSE == proc_ptr->read_mul_times) && (QOSA_TRUE == proc_ptr->done_flag)) /*Read the file in one go*/
    {
        len = qcm_sfs_file_read_onetime(fd, dst, size);
        proc_ptr->read_mul_times = QOSA_TRUE;
    }
    else if (QOSA_FALSE == proc_ptr->read_mul_times) /*First time reading data from the file*/
    {
        len = qcm_sfs_file_read_firsttimes(fd, dst, size);
        if (len > 0)
        {
            proc_ptr->read_remainder = (len >> 24) & 0xFF;
            len &= 0xFFFFFF;
            proc_ptr->read_mul_times = QOSA_TRUE;
        }
    }
    else if (QOSA_TRUE == proc_ptr->done_flag) /*Last read from file*/
    {
        size = size + (proc_ptr->read_remainder << 24);
        len = qcm_sfs_file_read_lasttimes(fd, dst, size);
        proc_ptr->read_remainder = 0;
    }
    else
    {
        size = size + (proc_ptr->read_remainder << 24);
        len = qcm_sfs_file_read_middletimes(fd, dst, size);
        if (len > 0)
        {
            proc_ptr->read_remainder = (len >> 24) & 0xFF; /*Save the count of extra data this time*/
            len &= 0xFFFFFF;
        }
    }

#if (QCM_SFS_DEBUG_LEVEL > QCM_SFS_DEBUG_LEVEL0)
    QLOGV("len:%d, remainder=%d", len, proc_ptr->read_remainder);
#endif
    return len;
}

void qcm_sfs_file_init(void)
{
    qosa_q_init(&g_sfs_process_list);
    qosa_mutex_create(&g_sfs_process_list_lock);
}
