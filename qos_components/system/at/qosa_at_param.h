/*****************************************************************/ /**
* @file qosa_at_param.h
* @brief
* @author larson.li@quectel.com
* @date 2023-04-27
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-04-27 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __AT_PARAM_H__
#define __AT_PARAM_H__

#include "qosa_def.h"

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 

#include "qosa_sys.h"
#include "qosa_at_sio.h"
#include "qosa_queue_list.h"
#include "qosa_system_utils.h"

#define QOSA_AT_NAME_LEN_MAX  24    //Maximum length of AT name, such as "cfun"
#define QOSA_AT_PARMA_LEN_MAX 3072  //Maximum total length of AT command
#define QOSA_AT_MAX_LEN       (QOSA_AT_PARMA_LEN_MAX + QOSA_AT_NAME_LEN_MAX)
#define QOSA_AT_MAX_PARAM_CNT 24    //Number of parameters in AT command
#define QOSA_AT_LF_TIMEOUT    10    //If no data after CR, wait QOSA_AT_LF_TIMEOUT ms

typedef struct _qosa_at_cmd qosa_at_cmd_t;

/**
 * AT command execution mode
 */
typedef enum
{
    QOSA_AT_CMD_SET,  /*!< eg: AT+QIACT=1 */
    QOSA_AT_CMD_TEST, /*!< eg: AT+QIACT=? */
    QOSA_AT_CMD_READ, /*!< eg: AT+QIACT? */
    QOSA_AT_CMD_EXE   /*!< eg: ATO */
} qosa_at_cmd_type_e;

/**
 * Parameter type, whether it is string, empty character, or other
 */
typedef enum
{
    QOSA_AT_PARAM_TYPE_EMPTY,
    QOSA_AT_PARAM_TYPE_RAW,
    QOSA_AT_PARAM_TYPE_STRING,
} qosa_at_param_type_e;

/**
 * Special AT character parsing type
 */
typedef enum
{
    QOSA_AT_CONSTRAIN_MIN = 100,
    QOSA_AT_CONSTRAIN_CJSON, /*!< JSON parameter input type */

    QOSA_AT_CONSTRAIN_MAX
} qosa_at_constrain_e;

/**
 * AT command registration information.
 *
 * note: Match the corresponding execution function through the registered name
 */
typedef struct _qosa_at_desc
{
    char *name;                             /*!< AT command name, such as "+IPR" */
    void (*handler)(struct _qosa_at_cmd *); /*!< At command handler */
    qosa_uint32_t constrains;               /*!< AT command constrains */
} qosa_at_desc_t;

/**
 * Parse the parameter list of AT command
 */
typedef struct
{
    qosa_uint8_t  type;   /*!< parameter type, used by AT engine internally */
    qosa_uint16_t length; /*!< value length */
    char         *value;  /*!< value, the real size is variable */
} qosa_at_param_t;

/**
 * AT parser function input parameters
 */
typedef struct _qosa_at_cmd
{
    qosa_at_dev_type_e dev_port;                      /*!< Port number of the current port */
    qosa_uint8_t       param_count;                   /*!< Number of parameters in the current AT */
    qosa_at_cmd_type_e type;                          /*!< Type of AT command */
    qosa_at_desc_t    *desc;                          /*!< AT command information */
    qosa_bool_t        resp_flag;                     /*!< Used to mark whether the current AT command is the first output, used to add \r\n */
    qosa_bool_t        err_flag;                      /*!< Whether the current AT encountered an error during execution */
    qosa_at_param_t   *params[QOSA_AT_MAX_PARAM_CNT]; /*!< Parameter array */
} qosa_at_cmd_t;

typedef struct
{
    qosa_uint32_t minval; /*!< minimal value */
    qosa_uint32_t maxval; /*!< maximum value */
} qosa_uint_range_t;

typedef struct
{
    int minval; /*!< minimal value */
    int maxval; /*!< maximum value */
} qosa_int_range_t;

typedef struct
{
    qosa_uint64_t minval; /*!< minimal value */
    qosa_uint64_t maxval; /*!< maximum value */
} qosa_uint64_range_t;

/**
 * data structure to define a signed 64bits integer range
 */
typedef struct
{
    qosa_int64_t minval; /*!< minimal value */
    qosa_int64_t maxval; /*!< maximum value */
} qosa_int64_range_t;

/*************************************************************************
*  							Function
**************************************************************************/
/**
 * extract uint parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 */
qosa_uint32_t qosa_at_param_uint(qosa_at_param_t *param, qosa_bool_t *paramok);

/**
 * extract optional uint parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUint.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 */

qosa_uint32_t qosa_at_param_default_uint(qosa_at_param_t *param, qosa_uint32_t defval, qosa_bool_t *paramok);

/**
 * extract uint parameter, and check range
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param minval    minimum valid value, inclusive
 * @param maxval    maximum valid value, inclusive
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 *          - \a param is not in range
 */
qosa_uint32_t qosa_at_param_uint_in_range(qosa_at_param_t *param, qosa_uint32_t minval, qosa_uint32_t maxval, qosa_bool_t *paramok);

/**
 * extract optional uint parameter, and check range
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUintInRange. The \a defval is not required in the
 * range.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param minval    minimum valid value, inclusive
 * @param maxval    maximum valid value, inclusive
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 *          - \a param is not in range
 */
qosa_uint32_t
qosa_at_param_default_uint_in_range(qosa_at_param_t *param, qosa_uint32_t defval, qosa_uint32_t minval, qosa_uint32_t maxval, qosa_bool_t *paramok);

/**
 * extract uint parameter, and check in list
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param list      array of valid values
 * @param count     valid value count
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 *          - \a param is not in list
 */
qosa_uint32_t qosa_at_param_uint_in_list(qosa_at_param_t *param, const qosa_uint32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

/**
 * extract optional uint parameter, and check in list
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUintInList. The \a defval is not required in the
 * list.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param list      array of valid values
 * @param count     valid value count
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 *          - \a param is not in list
 */
qosa_uint32_t
qosa_at_param_default_uint_in_list(qosa_at_param_t *param, qosa_uint32_t defval, const qosa_uint32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

/**
 * extract int parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 */
qosa_int32_t qosa_at_param_int(qosa_at_param_t *param, qosa_bool_t *paramok);

/**
 * extract optional int parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUint.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 */
qosa_int32_t qosa_at_param_default_int(qosa_at_param_t *param, qosa_int32_t defval, qosa_bool_t *paramok);

/**
 * extract int parameter, and check range
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param minval    minimum valid value, inclusive
 * @param maxval    maximum valid value, inclusive
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 *          - \a param is not in range
 */
qosa_int32_t qosa_at_param_int_in_range(qosa_at_param_t *param, qosa_int32_t minval, qosa_int32_t maxval, qosa_bool_t *paramok);

/**
 * extract optional int parameter, and check range
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUintInRange. The \a defval is not required in the
 * range.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param minval    minimum valid value, inclusive
 * @param maxval    maximum valid value, inclusive
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 *          - \a param is not in range
 */
qosa_int32_t qosa_at_param_default_int_in_range(qosa_at_param_t *param, qosa_int32_t defval, qosa_int32_t minval, qosa_int32_t maxval, qosa_bool_t *paramok);

/**
 * extract int parameter, and check in list
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param list      array of valid values
 * @param count     valid value count
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 *          - \a param is not in list
 */
qosa_int32_t qosa_at_param_int_in_list(qosa_at_param_t *param, const qosa_int32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

/**
 * extract optional int parameter, and check in list
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUintInList. The \a defval is not required in the
 * list.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param list      array of valid values
 * @param count     valid value count
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 *          - \a param is not in list
 */
qosa_int32_t
qosa_at_param_default_int_in_list(qosa_at_param_t *param, qosa_int32_t defval, const qosa_int32_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

/**
 * extract uint64 parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 */
qosa_uint64_t qosa_at_param_uint64(qosa_at_param_t *param, qosa_bool_t *paramok);

/**
 * extract optional uint64 parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUint.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 */
qosa_uint64_t qosa_at_param_default_uint64(qosa_at_param_t *param, qosa_uint64_t defval, qosa_bool_t *paramok);

/**
 * extract uint64 parameter, and check range
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param minval    minimum valid value, inclusive
 * @param maxval    maximum valid value, inclusive
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 *          - \a param is not in range
 */
qosa_uint64_t qosa_at_param_uint64_in_range(qosa_at_param_t *param, qosa_uint64_t minval, qosa_uint64_t maxval, qosa_bool_t *paramok);

/**
 * extract optional uint64 parameter, and check range
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUintInRange. The \a defval is not required in the
 * range.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param minval    minimum valid value, inclusive
 * @param maxval    maximum valid value, inclusive
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 *          - \a param is not in range
 */
qosa_uint64_t
qosa_at_param_default_uint64_in_range(qosa_at_param_t *param, qosa_uint64_t defval, qosa_uint64_t minval, qosa_uint64_t maxval, qosa_bool_t *paramok);

/**
 * extract uint64 parameter, and check in list
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param list      array of valid values
 * @param count     valid value count
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 *          - \a param is not in list
 */
qosa_uint64_t qosa_at_param_uint64_in_list(qosa_at_param_t *param, const qosa_uint64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

/**
 * extract optional uint64 parameter, and check in list
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUintInList. The \a defval is not required in the
 * list.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param list      array of valid values
 * @param count     valid value count
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 *          - \a param is not in list
 */
qosa_uint64_t
qosa_at_param_default_uint64_in_list(qosa_at_param_t *param, qosa_uint64_t defval, const qosa_uint64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

/**
 * extract int64 parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 */
qosa_int64_t qosa_at_param_int64(qosa_at_param_t *param, qosa_bool_t *paramok);

/**
 * extract optional int64 parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUint.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 */
qosa_int64_t qosa_at_param_default_int64(qosa_at_param_t *param, qosa_int64_t defval, qosa_bool_t *paramok);

/**
 * extract int64 parameter, and check range
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param minval    minimum valid value, inclusive
 * @param maxval    maximum valid value, inclusive
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 *          - \a param is not in range
 */
qosa_int64_t qosa_at_param_int64_in_range(qosa_at_param_t *param, qosa_int64_t minval, qosa_int64_t maxval, qosa_bool_t *paramok);

/**
 * extract optional int64 parameter, and check range
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUintInRange. The \a defval is not required in the
 * range.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param minval    minimum valid value, inclusive
 * @param maxval    maximum valid value, inclusive
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 *          - \a param is not in range
 */
qosa_int64_t qosa_at_param_default_int64_in_range(qosa_at_param_t *param, qosa_int64_t defval, qosa_int64_t minval, qosa_int64_t maxval, qosa_bool_t *paramok);

/**
 * extract int64 parameter, and check in list
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param list      array of valid values
 * @param count     valid value count
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 *          - \a param is not in list
 */
qosa_int64_t qosa_at_param_int64_in_list(qosa_at_param_t *param, const qosa_int64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

/**
 * extract optional int64 parameter, and check in list
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUintInList. The \a defval is not required in the
 * list.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param list      array of valid values
 * @param count     valid value count
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - int parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is not integer number
 *          - \a param is not in list
 */
qosa_int64_t
qosa_at_param_default_int64_in_list(qosa_at_param_t *param, qosa_int64_t defval, const qosa_int64_t *list, qosa_uint32_t count, qosa_bool_t *paramok);

/**
 * extract string parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * String parameter is parameter started and ended with double quotation.
 * The escape defined in V.250 is \\HH. For example, \30 for '0'.
 *
 * It is possible that the internal storage will be changed during this
 * function call. So, don't call other parameter APIs for the same
 * parameter after it is called. However, this function itself can be
 * called again.
 *
 * The output pointer will be valid till the parameter itself is deleted.
 *
 * The output string is ended with null byte for end of string.
 *
 * @param param     parameter pointer
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - parsed string
 *      - NULL on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not string (start and end with double quotation)
 */
const char *qosa_at_param_string(qosa_at_param_t *param, qosa_bool_t *paramok);

/**
 * extract hex or str parameter from raw text
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * Raw text parameter is parameter started and ended without double quotation.
 * The escape defined in V.250 is \\HH. For example, \30 for '0'.
 *
 * It is possible that the internal storage will be changed during this
 * function call. So, don't call other parameter APIs for the same
 * parameter after it is called. However, this function itself can be
 * called again.
 *
 * The output pointer will be valid till the parameter itself is deleted.
 *
 * The output string is ended with null byte for end of string.
 *
 * @param param     parameter pointer
 * @param len     the length of the parameter
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - parsed string
 *      - NULL on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is string (start and end with double quotation)
 */
const char *qosa_at_param_hex_data(qosa_at_param_t *param, qosa_uint32_t len, qosa_bool_t *paramok);

/**
 * extract hex parameter from raw text
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * Raw text parameter is parameter started and ended without double quotation.
 * The escape defined in V.250 is \\HH. For example, \30 for '0'.
 *
 * It is possible that the internal storage will be changed during this
 * function call. So, don't call other parameter APIs for the same
 * parameter after it is called. However, this function itself can be
 * called again.
 *
 * The output pointer will be valid till the parameter itself is deleted.
 *
 * The output string is ended with null byte for end of string.
 *
 * @param param     parameter pointer
 * @param len     the length of the parameter
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - parsed string
 *      - NULL on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is string (start and end with double quotation)
 */
const char *qosa_at_param_hex_raw_text(qosa_at_param_t *param, qosa_uint32_t len, qosa_bool_t *paramok);
/**
 * extract string parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * String parameter is parameter started and ended with double quotation.
 * The escape defined in V.250 is \\HH. For example, \30 for '0'.
 *
 * It is possible that the internal storage will be changed during this
 * function call. So, don't call other parameter APIs for the same
 * parameter after it is called. However, this function itself can be
 * called again.
 *
 * The output pointer will be valid till the parameter itself is deleted.
 *
 * The output string is ended with null byte for end of string.
 *
 * @param param     parameter pointer
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - parsed string
 *      - NULL on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not string (start and end with double quotation)
 */
const char *qosa_at_param_option_string(qosa_at_param_t *param, qosa_bool_t *paramok);
/**
 * extract optional string parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamStr.
 *
 * It is possible that the internal storage will be changed during this
 * function call. So, don't call other parameter APIs for the same
 * parameter after it is called. However, this function itself can be
 * called again.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - parsed string
 *      - NULL on failed
 *          - \a *paramok is false at input
 *          - \a param is not string (start and end with double quotation)
 */
const char *qosa_at_param_default_string(qosa_at_param_t *param, const char *defval, qosa_bool_t *paramok);

/**
 * extract raw parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * The raw parameter is not parsed. For example, the double quotation of
 * string parameter will be kept, and the escape in string parameter is
 * untouched.
 *
 * The output string is ended with null byte as end of string.
 *
 * @param param     parameter pointer
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - parsed string
 *      - NULL on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 */
const char *qosa_at_param_raw_text(qosa_at_param_t *param, qosa_bool_t *paramok);

/**
 * extract floating point parameter
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - floating point parameter
 *      - NULL on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not double
 */
double qosa_at_param_double(qosa_at_param_t *param, qosa_bool_t *paramok);

/**
 * extract string parameter and return mapped uint
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * It is just a wrapper to:
 * - extract string parameter
 * - map to uint
 *
 * @param param     parameter pointer
 * @param vsmap     integer/string map, ended with NULL string value
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - NULL on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not in map
 */
qosa_uint32_t qosa_at_param_uint_by_string_map(qosa_at_param_t *param, const qosa_value_str_map_t *vsmap, qosa_bool_t *paramok);

/**
 * extract optional string parameter and return mapped uint
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * When \a param is empty, \a defval is returned. Otherwise, it is the
 * same as \a unir_atParamUintByStrMap. The \a defval is not required in the
 * map.
 *
 * @param param     parameter pointer
 * @param defval    default value
 * @param vsmap     integer/string map, ended with NULL string value
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - NULL on failed
 *          - \a *paramok is false at input
 *          - \a param is not in map
 */
qosa_uint32_t qosa_at_param_default_uint_by_string_map(qosa_at_param_t *param, qosa_uint32_t defval, const qosa_value_str_map_t *vsmap, qosa_bool_t *paramok);

/**
 * extract uint parameter by hex string
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * The valid parameter is the output of:
 * \code{.cpp}
 * printf("\"%x\"", value)
 * \endcode
 *
 * For example, parameter "12345" will be parsed as 0x12345.
 *
 * @param param     parameter pointer
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - NULL on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not the needed format
 */
qosa_uint32_t qosa_at_param_hex_string_uint(qosa_at_param_t *param, qosa_bool_t *paramok);

/**
 * trim tailing characters at the end of parameter
 *
 * This shall be called before the parameter isn't parsed. \p unir_atParamRawText
 * won't parse the parameter, so it is safe to call it after \p unir_atParamRawText.
 *
 * It will change the internal storage of parameter directly. So, only call
 * it when absolutely needed, and you know what you are doing.
 *
 * @param param     parameter pointer
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - true on success
 *      - false on failed
 *          - \p param type is not raw text
 *          - the length of parameter is shorter than \p len
 */
qosa_bool_t qosa_at_param_trim_tail(qosa_at_param_t *param, qosa_uint32_t len);

/**
 * check whether the parameter is empty
 *
 * Both not specified and skipped parameter are empty. For example:
 * - AT+CMD=123
 * - AT+CMD=123,
 * - AT+CMD=123,,456
 *
 * In all of the above cases, params[1] is empty.
 *
 * @param param     parameter pointer
 * @return
 *      - true if the parameter is empty
 *      - false otherwise
 */
qosa_bool_t qosa_at_param_is_empty(qosa_at_param_t *param);

/**
 * extract hex qosa_int64_t parameter, and check range
 *
 * When \a *paramok is false, it will do nothing. On failure, \a *param
 * will be set to false on return.
 *
 * @param param     parameter pointer
 * @param minval    minimum valid value, inclusive
 * @param maxval    maximum valid value, inclusive
 * @param paramok   in/out parameter parsing ok flag
 * @return
 *      - uint parameter
 *      - 0 on failed
 *          - \a *paramok is false at input
 *          - \a param is empty
 *          - \a param is not integer number
 *          - \a param is not in range
 */
qosa_int64_t qosa_at_param_hex_int64_in_range(qosa_at_param_t *param, qosa_int64_t minval, qosa_int64_t maxval, qosa_bool_t *paramok);

#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __AT_PARAM_H__ */
