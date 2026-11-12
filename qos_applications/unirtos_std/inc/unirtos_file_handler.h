/*****************************************************************/ /**
* @file unirtos_file_handler.h
* @brief
* @author larson.li@quectel.com
* @date 2023-06-13
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-06-13 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_FILE_HANDLER_H__
#define __UNIRTOS_FILE_HANDLER_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qcm_file_api.h"
#include "qosa_at_cmd.h"
#include "qosa_watermark.h"
#include "qosa_buffer_block.h"
#include "unirtos_atcmd_cfg.h"

#define UNIR_FILE_TASK_PRIO               QOSA_PRIORITY_NORMAL
#define UNIR_FILE_TASK_EVENT_MAX          30

#define UNIR_FILE_SIZE_DEFAULT            (10 * 1024)

#define UNIR_FILE_FUPLOAD_LENGTH_MIN      1  //File write parameter range start value changed to 1, 0 is an invalid parameter
#define UNIR_FILE_FUPLOAD_LENGTH_MAX      1 * 1024 * 1024
#define UNIR_FILE_FUPLOAD_LENGTH_DEFAULT  10 * 1024

#define UNIR_FILE_FUPLOAD_TIMEOUT_MIN     1
#define UNIR_FILE_FUPLOAD_TIMEOUT_MAX     65535
#define UNIR_FILE_FUPLOAD_TIMEOUT_DEFAULT 5  //write timeout default 5s

#define UNIR_FILE_FUPLOAD_ACKMODE_MIN     0
#define UNIR_FILE_FUPLOAD_ACKMODE_MAX     1
#define UNIR_FILE_FUPLOAD_ACKMODE_DEFAULT 0

#define UNIR_FILE_OPEN_MODE_MIN           0
#define UNIR_FILE_OPEN_MODE_MAX           2
#define UNIR_FILE_OPEN_MODE_DEFAULT       0

#define UNIR_FILE_HANDLER_MIN             0
#define UNIR_FILE_HANDLER_MAX             0x7fffffff
#define UNIR_FILE_HANDLER_DEFAULT         -1  //default file handler

#define UNIR_FILE_FSEEK_POSITION_MIN      0
#define UNIR_FILE_FSEEK_POSITION_MAX      2
#define UNIR_FILE_FSEEK_POSITION_DEFAULT  0

#define UNIR_FILE_HANDLER_TYPE_WRITE      0
#define UNIR_FILE_HANDLER_TYPE_READ       1
#define UNIR_FILE_HANDLER_TYPE_DELETE     2

#define UNIR_FILE_FREAD_LENGTH_MIN        CONFIG_UNIRTOS_QFREAD_MIN_LEN
#define UNIR_FILE_FREAD_LENGTH_MAX        0x7fffffff
#define UNIR_FILE_FREAD_LENGTH_DEFAULT    UNIR_FILE_SIZE_DEFAULT

#define UNIR_FILE_OPENED_NUM              10

#define UNIR_FILE_FSEEK_OFFSET_MIN        0
#define UNIR_FILE_FSEEK_OFFSET_MAX        0x7fffffff
#define UNIR_FILE_FSEEK_OFFSET_DEFAULT    0

#define UNIR_FILE_FUPLOAD_RESPONSE_LEN    (1 << 10)  //recv 1K response "A"

/**
 * @enum qfile_cmd_e_type
 * @brief  Mainly used for command types of the file handler task
 */
typedef enum
{
    QFILE_MIN_CMD = -1,
    QFILE_FWRITE_DATA_CMD, /*!< write file  QFUPL  QFWRITE   */
    QFILE_FREAD_DATA_CMD,  /*!< download file info QFDWL QFREAD */

    QFILE_QFUPL_CMD,
    QFILE_QFDWL_CMD,
    QFILE_QFMKDIR_CMD,
    QFILE_QFRMDIR_CMD,
    QFILE_QFOPEN_CMD,
    QFILE_QFOPEN_GET_CMD,
    QFILE_QFLIST_CMD,
    QFILE_QFDEL_CMD,
    QFILE_QFCLOSE_CMD,
    QFILE_QFREAD_CMD,
    QFILE_QFLDS_CMD,
    QFILE_QFWRITE_CMD,
    QFILE_QFSEEK_CMD,
    QFILE_QFPOSITION_CMD,
    QFILE_QFTUCAT_CMD,
    #ifdef CONFIG_QAPP_UNIRTOS_AT_QFMD5_CMD
    QFILE_QFMD5_CMD,
    #endif /* CONFIG_QAPP_UNIRTOS_AT_QFMD5_CMD */

    QFILE_MAX_CMD
} qfile_cmd_e_type;

/**
 * @enum UNIR_FILE_ERROR_CODES
 * @struct Standard File AT Command Error Codes
 */
typedef enum
{
    UNIR_FILE_OK = 0,
    UNIR_FILE_INVALID_INPUT_VALUE = 400, /*!< invalid input value       */
    UNIR_FILE_LARGER_THAN_FILESIZE,      /*!< large than file size      */
    UNIR_FILE_READ_ZERO,                 /*!< read zero byte            */
    UNIR_FILE_DRIVE_FULL,                /*!< drive full                */
    UNIR_FILE_MOV_ERROR,                 /*!< move error                */
    UNIR_FILE_FILE_NOT_FOUND = 405,      /*!< file not found            */
    UNIR_FILE_INVALID_FILE_NAME,         /*!< invalidate file name      */
    UNIR_FILE_FILE_ALREADY_EXISTED,      /*!< file already exist        */
    UNIR_FILE_FAILED_TO_CREATE_FILE,     /*!< failed to create file     */
    UNIR_FILE_FAILED_TO_WRITE_FILE,      /*!< failed to write file      */
    UNIR_FILE_FAILED_TO_OPEN_FILE = 410, /*!< failed to open file       */
    UNIR_FILE_FAILED_TO_READ_FILE,       /*!< failed to read file       */
    UNIR_FILE_EXCEED_MAX_LENGTH,         /*!< exceed max length         */
    UNIR_FILE_REACH_MAX_OPENFILE_NUM,    /*!< reach max opened file num */
    UNIR_FILE_IS_READONLY,               /*!< file is the read-only     */
    UNIR_FILE_GET_SIZE_FAIL = 415,       /*!< get file size failed      */
    UNIR_FILE_INVALID_FILE_DESCRIPTOR,   /*!< description invalidate    */
    UNIR_FILE_LIST_FILE_FAIL,            /*!< failed to list file       */
    UNIR_FILE_DELETE_FILE_FAIL,          /*!< failed to delete file     */
    UNIR_FILE_GET_DISK_INFO_FAIL,        /*!< failed to get disk info   */
    UNIR_FILE_NO_SPACE = 420,            /*!< disk no space             */
    UNIR_FILE_TIME_OUT,                  /*!< time out                  */
    UNIR_FILE_FILE_NOT_FOUND_2,          /*!< file not found            */
    UNIR_FILE_FILE_TOO_LARGE,            /*!< file too large            */
    UNIR_FILE_FILE_ALREADY_EXIST,        /*!< file already exist        */
    UNIR_FILE_INVALID_PARAMETER = 425,   /*!< invalidate parameter      */
    UNIR_FILE_ALREADY_OPERATION,         /*!< file already open         */
    UNIR_FILE_ERROR_GENERAL,
    UNIR_FILE_RENAME_ERROR,
    UNIR_FILE_QUIT_DATE_MODE,
    UNIR_FILE_ERROR_MAX,
} UNIR_FILE_ERROR_CODES;

typedef enum
{
    QFILE_DELE_SINGLE_FILE = 0, /* qfile delete single file info    */
    QFILE_DELE_ALL_FILE         /* qfile delete all file info       */
} qfile_dele_type;

typedef enum
{
    FILE_NO_OPERATION = 0,
    FILE_UPLOADING,    //qfupl, or qfwrite
    FILE_DOWNLOADING,  //qfread, or qfdwl
    FILE_OPERATION_MAX
} FILE_OPERATION_E;

typedef enum
{
    QFILE_LIST_SINGLE_FILE = 0, /* qflist single file info    */
    QFILE_LIST_ALL_FILE         /* qflist all file info       */
} qfile_list_type;

/**
 * @struct QFUPL_PARAM
 * @brief Used by QFUPL+QFWRITE functions for file upload
 */
typedef struct
{
    qosa_mutex_t            lock;
    char                    file_name[QCM_FILE_FILENAME_MAX_LEN];
    qosa_uint32_t           file_size;     /* file size                 */
    qosa_uint32_t           time_out;      /*!< Exit after no data is fed for a certain period */
    qosa_uint32_t           ack_mode;      /*!< Whether a response is required */
    qosa_uint32_t           size_uploaded; /*!< The length that has been currently obtained */
    qosa_uint32_t           size_write;    /*!< Length to be obtained set by AT*/
    qosa_uint16_t           checksum;      /*!< crc */
    qosa_uint16_t           odd_byte;      /*!< crc */
    qosa_uint8_t            odd_flag;      /*!< crc */
    qosa_uint32_t           bytes_ack;     /*!< Number of ACKs that have been accumulated and need to be responded to */
    qosa_at_dev_type_e      dev_port;      /*!< AT dev channel corresponding to file operations */
    Q_FILE                  fd;            /*!< File handle of the current file to be saved */
    qosa_uint8_t            fupl_flag;     /*!< Whether it is a fupl command, or fwrite */
    qosa_int8_t             flow_flag;     /*!< Corresponding flow control management */
    qosa_uint8_t            upload_finish; /*!< Whether the upload has finished */
    qosa_timer_t            fupl_timer;    /*!< File upload timer */
    qosa_buffer_block_t    *item_data_ptr; /*!< If the data currently taken from the watermark is not fully used, it is all saved here */
    qosa_buffer_watermark_t tx_wm_ptr;     /*!< watermark TX pointer */
    qosa_buffer_watermark_t rx_wm_ptr;     /*!< watermark RX pointer */
    qosa_q_type_t           q_ptr_tx;      /*!< watermark attached queue */
    qosa_q_type_t           q_ptr_rx;      /*!< watermark attached queue */
    qosa_bool_t             timeout_flag;
    qosa_bool_t             force_exit;    /*!< Force exit, user actively executes +++ or dtr */
} file_hd_fupl_param_t;

/**
 * @struct QFDWL_PARAM
 * @brief Used by QFDWL+QFREAD functions for file downloading
 */
typedef struct
{
    char                    file_name[QCM_FILE_FILENAME_MAX_LEN];
    qosa_uint32_t           size_downloaded;  /* Downloaded length */
    qosa_uint32_t           size_read;        /* Expected length for AT command reading */
    qosa_uint16_t           checksum;         /*!< Calculate the CRC value of the file */
    qosa_uint16_t           odd_byte;         /*!< Calculate the CRC value of the file */
    qosa_uint8_t            odd_flag;         /*!< Calculate the CRC value of the file */
    int                     fd;               /*!< File handle to operate on */
    qosa_int8_t             fdwl_flag;        /*!< Whether it is an fdwl command */
    qosa_bool_t             flow_flag;        /*!< Flow control management */
    qosa_at_dev_type_e      dev_port;         /*!< Corresponding AT channel */
    qosa_buffer_watermark_t tx_wm_ptr;        /*!< watermark TX pointer */
    qosa_buffer_watermark_t rx_wm_ptr;        /*!< watermark RX pointer */
    qosa_q_type_t           q_ptr_tx;         /*!< watermark attached queue */
    qosa_q_type_t           q_ptr_rx;         /*!< watermark attached queue */
    qosa_bool_t             force_exit;       /*!< Force exit, user actively executes +++ or dtr */
    qosa_bool_t             non_empty_report; /*!< Whether to report empty notification */
    UNIR_FILE_ERROR_CODES   last_error_code;  /*!< The final error code */
    qosa_timer_t            wait_timer;       /*!< Prevent deadlock due to inconsistent state caused by adjacent state */
    qosa_bool_t             read_finish;      /*!< Whether the data has been read completely */
} file_hd_fdwl_param_t;

typedef union
{
    file_hd_fupl_param_t fupl;
    file_hd_fdwl_param_t fdwl;
} FILE_T_PARAM;

typedef struct
{
    FILE_OPERATION_E op;
    FILE_T_PARAM     fparam;
} DM_PARAM;

/************************************************************************/
/*  Used for the file task to send message body to the AT task                                     */
/************************************************************************/
/**
 * @enum file_resp_cmd_e
 * @brief Message command codes used by file task to send to center task
 */
typedef enum
{
    FILE_RESP_READ_FINISH,     /*!< File read completion AT return notification */
    FILE_RESP_WRITE_FINISH,    /*!< AT notification returned upon file write completion */
    FILE_RESP_PRINT_RESULT,    /*!< General return report for AT operation */
    FILE_RESP_ENTRY_DATA_MODE, /*!< File switch enters data mode AT return notification */
    FILE_RESP_MAX
} file_resp_cmd_e;

/**
 * @struct at_file_resp_info_t
 * @brief  Used for synchronizing file processing information from file task to AT task context
 */
typedef struct
{
    qosa_at_dev_type_e    dev_port;
    file_resp_cmd_e       cmd_id;
    qosa_buffer_block_t  *memory_ptr;
    UNIR_FILE_ERROR_CODES file_error_code;
    qosa_int32_t          result_code;
    qosa_int8_t           fdwl_flag;
    qosa_uint8_t          fupl_flag;
    int                   fd;
} at_file_resp_info_t;

/**
 * @brief Information structure for FILE task switching into data mode
 */
typedef struct
{
    qosa_at_dev_type_e       dev_port;    /*!< Corresponding AT channel port number */
    qosa_buffer_watermark_t *tx_ptr;      /*!< Pointer to the TX watermark for switching into passthrough mode */
    qosa_buffer_watermark_t *rx_ptr;      /*!< Pointer to the RX watermark for switching into passthrough mode */
    qosa_sem_t               release_sem; /*!< Semaphore corresponding to synchronization wait for switch completion */
    FILE_OPERATION_E         opt;         /*!< Corresponding to FILE QFDWL QFUPL switching mode selection */
    qosa_int8_t              fdwl_flag;   /*!< Corresponding to FILE QFDWL QFWRITE mode switching selection */
    qosa_uint32_t            size_read;   /* Expected length for AT command reading */
} at_file_entry_data_mode_info_t;

/**
 * @brief Message body sent from the FILE task to the AT task, used for synchronizing file processing result information
 */
typedef struct
{
    file_resp_cmd_e cmd_id;
    void           *argv;
} at_file_event_cmd_info;

/************************************************************************/
/*  Used for AT thread to send message content to file task               */
/************************************************************************/

/**
 * @struct file_task_del_info_t
 * @brief Used to send a delete file action to the file task
 */
typedef struct
{
    char               file_name[QCM_FILE_FILENAME_MAX_LEN]; /*!< file name, like "UFS:a.txt" */
    char               file_open_mode;                       /*!< Mode for opening the operation file */
    qcm_file_type_e    file_type;
    int                file_index;
    int                file_fd;
    qosa_at_dev_type_e dev_port;      /*!< The device channel number corresponding to the AT command */
    qosa_uint8_t       first_list;
    qfile_list_type    list_type;     /*!< Type of file traversal for qflst */
    qfile_dele_type    dele_type;     /*!< Type of file to be deleted by qfdel */
    qosa_int64_t       file_len;      /*!< File length related information */
    qosa_uint32_t      timeout_value; /*!< Timer time configuration information */
    qosa_uint8_t       ack_mode;      /*!< Whether to enable ack mode for fwrite fupl */
    int                rmdir_mode;    /*!< Folder deletion mode */
    qosa_int64_t       offset;
    qosa_int64_t       position;
} at_to_file_param_info_t;

/**
 * @struct unir_file_msg_t
 * @brief  Used to pass messages from AT commands to the file task
 */
typedef struct
{
    qfile_cmd_e_type cmd;
    void            *argv;
} file_task_msg_t;

/**
 * @brief Notifies the file task of an AT command message event
 *
 * @param[in] qfile_cmd_e_type cmd
 *          - Corresponds to the specific command type
 *
 * @param[in] void * argv
 *          - User parameter to be passed
 *
 * @return int
 *       - Returns 0 on successful execution
 *       - Returns a negative number on execution failure
 */
int unir_notify_file_task(qfile_cmd_e_type cmd, void *argv);

/**
 * @brief Deinitializes file upload parameters for the specified AT device port
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *          - AT device port corresponding to the upload context to be released
 *
 * @return int
 *       - Returns 0 on successful execution
 *       - Other return values indicate failure
 */
int file_hd_upload_param_deinit(qosa_at_dev_type_e dev_port);

#endif /* __UNIRTOS_FILE_HANDLER_H__ */
