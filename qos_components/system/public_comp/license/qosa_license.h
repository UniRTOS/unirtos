/*****************************************************************/ /**
* @file qosa_license.h
* @brief Platform license verification interface
* @author Amara.Wang@quectel.com
* @date 2025-12-11
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2025-12-11 <td>1.0 <td>Amara.Wang <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_LICENSE_H__
#define __QOSA_LICENSE_H__

/** Maximum length of device information obtained by license */
#define QUECTEL_LICENSE_DEV_INFO_MAX_LEN   (48)
/** Maximum length of license file name */
#define QUECTEL_LICENSE_FILE_NAME_MAX_LEN  (80)
/** Maximum length of digest information calculation */
#define QUECTEL_LICENSE_DIGEST_MAX_LEN     (128)
/** Maximum length of license content */
#define QUECTEL_LICENSE_INFO_MAX_LEN       (1024)
/** Maximum length of license decode */
#define QUECTEL_LICENSE_DECODE_MAX_LEN     (1024)
/** Maximum length of APP public key */
#define QUECTEL_LICENSE_PUBLIC_KEY_MAX_LEN (1024)

/**
 * @enum qosa_license_error_e
 * @brief License verification error code enumeration definition
 */
typedef enum
{
    QOSA_LIC_OK = 0,                    /*!< Success */
    QOSA_LIC_ERROR_BASE = 0x80000000,   /*!< Error code base value */
    QOSA_LIC_MALLOC_ERROR,              /*!< Memory allocation error */
    QOSA_LIC_GET_LICENSE_INFO_ERROR,    /*!< Get license information error */
    QOSA_LIC_JSON_PARSER_ERROR,         /*!< JSON parsing error */
    QOSA_LIC_DEVICE_INEQUALITY_ERROR,   /*!< Device information mismatch error */
    QOSA_LIC_APP_NO_REGISTER_ERROR,     /*!< APP not registered error */
    QOSA_LIC_CHECK_APP_ID_ERROR,        /*!< Check APP ID error */
    QOSA_LIC_JSON_PARSER_TIME_ERROR,    /*!< JSON time parsing error */
    QOSA_LIC_LICENSE_TIMEOUT_ERROR,     /*!< License timeout error */
    QOSA_LIC_BASE64_DECODE_ERROR,       /*!< Base64 decoding error */
    QOSA_LIC_VERIFY_SIGNATURE_ERROR,    /*!< Signature verification error */
    QOSA_LIC_APP_SAVE_PUBLIC_KEY_ERROR, /*!< Save APP public key error */
    QOSA_LIC_APP_READ_PUBLIC_KEY_ERROR, /*!< Read APP public key error */
    QOSA_LIC_SAVE_LICENSE_ERROR,        /*!< Save license error */

    QOSA_LIC_ERROR_MAX                  /*!< Maximum error code value */
} qosa_license_error_e;

/**
 * @enum qosa_license_app_type_t
 * @brief APP type enumeration definition, each authorized APP TYPE is unique
 */
typedef enum
{
    QOSA_LIC_APP_MIN_TYPE = -1,              /*!< Minimum APP type value */
    QOSA_LIC_APP_TEST1_TYPE = 0,             /*!< Verified APP type 1 */
    QOSA_LIC_APP_TEST2_TYPE,                 /*!< Verified APP type 2 */
    QOSA_LIC_APP_TEST3_TYPE,                 /*!< Verified APP type 3 */
    QOSA_LIC_APP_CAMERA_WATER_METER_AI_TYPE, /*!< Camera water meter AI APP type */
    QOSA_LIC_APP_MAX_TYPE                    /*!< Maximum APP type value */
} qosa_license_app_type_t;

/** Get APP ID callback function prototype */
typedef int (*qosa_lic_get_app_id)(void);
/** Save APP License callback function prototype */
typedef int (*qosa_lic_set_app_license)(unsigned char *a, int b);
/** Get APP License callback function prototype */
typedef int (*qosa_lic_get_app_license)(unsigned char *a, int *b);
/** Save APP public key callback function prototype */
typedef int (*qosa_lic_set_app_save_pubk)(unsigned char *a, int b);
/** Get APP public key callback function prototype */
typedef int (*qosa_lic_get_app_pubk)(unsigned char *a, int *b);

/**
 * @struct _tag_APP_INFO
 * @brief APP information structure definition
 */
typedef struct _tag_APP_INFO
{
    int                        init_reg; /*!< Whether registered */
    int                        app_id;   /*!< APP ID, users need to create and obtain the corresponding APP ID on the authorization server cloud platform */
    qosa_license_app_type_t    app_type; /*!< APP type */
    qosa_lic_set_app_license   save_license_cb; /*!< Current APP type gets the saved license file name */
    qosa_lic_get_app_license   read_license_cb; /*!< Read License callback function */
    qosa_lic_set_app_save_pubk save_pubk_cb;    /*!< Save public key information */
    qosa_lic_get_app_pubk      read_pubk_cb;    /*!< Read public key callback function */
    qosa_lic_get_app_id        get_app_id_cb;   /*!< Get APPID */
} qosa_license_app_info_t;

/**
 * @brief Used to convert appid to apptype
 *
 * @param[in] int appid
 *            The appid to convert
 *
 * @return qosa_license_error_e
 *         Returns QOSA_LIC_OK for success,
 *         Returns others for failure
 */
qosa_license_app_type_t qosa_license_appid_to_apptype(int appid);

/**
 * @brief Apps requiring authorization need to register through this function to access the authorization system
 *
 * @param[in] qosa_license_app_type_t app_type
 *            Corresponding appId type, can convert appid to apptype through qosa_license_appid_to_apptype function
 *
 * @param[in] qosa_license_app_info_t * app_info
 *            Information of the APP to be registered
 *
 * @return qosa_license_error_e
 *         Returns QOSA_LIC_OK for success,
 *         Returns others for failure
 */
qosa_license_error_e qosa_license_register_app(qosa_license_app_type_t app_type, qosa_license_app_info_t *app_info);

/**
 * @brief Used to get appid
 *
 * @param[in] ql_license_app_type_t app_type
 *            The app_type to get appid
   @param[out]  int *appid
                The obtained appid is returned with parameters
 * @return ql_license_error_e
 *         Returns QL_LIC_OK for success,
 *         Returns others for failure
 */
qosa_license_error_e qosa_license_get_apppid(qosa_license_app_type_t app_type, int *appid);

/**
 * @brief Check if the specified APP ID has passed license authorization
 *
 * @param[in] qosa_license_app_type_t app_id
 *            The APP ID to query
 *
 * @return qosa_license_error_e
 *         Execution result
 */
qosa_license_error_e qosa_license_get_active_result(qosa_license_app_type_t app_type);

/**
 * @brief Save License information
 *
 * @param[in] app_type APP type
 * @param[in] data     License data pointer
 * @param[in] data_len License data length
 *
 * @return qosa_license_error_e
 *         Returns QOSA_LIC_OK for success,
 *         Returns others for failure
 */
qosa_license_error_e qosa_license_save_license_info(qosa_license_app_type_t app_type, unsigned char *data, int data_len);

/**
 * @brief Module device information acquisition interface
 *
 * @param[in] unsigned char * dev_info
 *            Starting address of device information address space to obtain
 *
 * @param[in] int * dev_info_len
 *            Size of dev_info space information
 *
 * @return int
 *            Returns 0 for success,
 *            Returns non-zero for failure
 *
 */
int qosa_license_get_dev_info(unsigned char *dev_info, int *dev_info_len);

/**
 * @brief Calculate SHA256 digest of data
 *
 * @param[in] plaintest Input data
 * @param[in] plaintext_len Data length
 * @param[out] digest_txt Digest output buffer
 * @param[out] digest_len Digest length
 * @return int 0 for success
 */
int qosa_license_digest(unsigned char *plaintest, int plaintext_len, unsigned char *digest_txt, int *digest_len);

#endif /* __QOSA_LICENSE_H__ */
