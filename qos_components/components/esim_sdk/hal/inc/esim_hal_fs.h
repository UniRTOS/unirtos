#ifndef __ESIM_HAL_FS_H__
#define __ESIM_HAL_FS_H__

/**
 * @enum esim_ap_fs_seek_flag_e
 * @brief eSIM File System Positioning Operation Flag Enumeration
 * @details Define the reference position for file positioning operations, used for setting the offset of the file read/write pointer.
 */
typedef enum
{
    ESIM_AP_FS_SEEK_SET = 0, /*!< Offset from the beginning of the file (SEEK_SET)*/
    ESIM_AP_FS_SEEK_CUR = 1, /*!< Offset from the current position (SEEK_CUR)*/
    ESIM_AP_FS_SEEK_END = 2, /*!< Offset from the end of the file (SEEK_END)*/

    ESIM_AP_FS_SEEK_MAX      /*!< Enumeration maximum value, used for boundary checking*/
} esim_ap_fs_seek_flag_e;

/**
 * @enum esim_ap_fs_flag_e
 * @brief eSIM File System File Open Mode Flag Enumeration
 * @note Flag values are designed as bit fields and can be combined using bitwise OR (|).
 */
typedef enum
{
    ESIM_AP_FS_O_RDONLY = 1,  /*!< Open file in read-only mode, corresponding to POSIX O_RDONLY*/
    ESIM_AP_FS_O_WRONLY = 2,  /*!< Open file in write-only mode, corresponding to POSIX O_WRONLY*/
    ESIM_AP_FS_O_RDWR = 4,    /*!< Open file in read-write mode, corresponding to POSIX O_RDWR*/
    ESIM_AP_FS_O_CREAT = 8,   /*!< Create the file if it does not exist, corresponding to POSIX O_CREAT*/
    ESIM_AP_FS_O_EXCL = 16,   /*!< Used in conjunction with O_CREAT, returns an error if the file exists, corresponding to POSIX O_EXCL*/
    ESIM_AP_FS_O_TRUNC = 32,  /*!< If the file exists and is writable, clear its contents, corresponding to POSIX O_TRUNC*/
    ESIM_AP_FS_O_APPEND = 64, /*!< Append mode write, corresponding to POSIX O_APPEND*/

    AP_FS_O_MAX               /*!< Enumeration maximum value, used for boundary checking*/
} esim_ap_fs_flag_e;

/**
 * @brief Obtain the ipa file directory, the directory path is specified by the platform.
 *
 * @param[in] const char *path
 *          - Directory name-ESIM_IPA_FILE_DIR_PATH
 *
 * @return char *
 *          - Success, directory data
 *          - Failed, QOSA_NULL
 */
extern char *ipa_ap_fs_directory_get(void);

/**
 * @brief Create ipa file directory
 *
 * @param[in] int mode
 *          - mode - 0
 *
 * @return int
 *          - Success, 0
 *          - Failure, -1
 */
extern int ipa_ap_fs_mkdir(int mode);

/**
 * @brief Open file
 *
 * @param[in] const char *file_path
 *          - File path
 *
 * @param[in] esim_ap_fs_flag_e flag
 *          - File opening modes
 *
 * @return int
 *          - Success, the handle of the successfully opened file
 *          - Failure, -1
 */
extern int ipa_ap_fs_open(const char *file_path, esim_ap_fs_flag_e flag);

/**
 * @brief Close file
 *
 * @param[in] int fd
 *          - File ID
 *
 * @return int
 *          - Success, 0
 *          - Failure, -1
 */
extern int ipa_ap_fs_close(int fd);

/**
 * @brief Write file
 *
 * @param[in] int fd
 *          - File ID
 *
 * @param[in] const void *buf
 *          - Data to be written
 *
 * @param[in] int buf_len
 *          - Length of data to be written
 *
 * @return int
 *          - Number of bytes successfully written, >0
 *          - Failure, other
 */
extern int ipa_ap_fs_write(int fd, const void *buf, int bufLen);

/**
 * @brief Read file
 *
 * @param[in] int fd
 *          - File ID
 *
 * @param[out] const void *buf
 *          - Read data
 *
 * @param[in] int buf_len
 *          - Length of data to read
 *
 * @return int
 *          - The number of bytes successfully read, >0
 *          - Failed, other
 */
extern int ipa_ap_fs_read(int fd, void *buf, int bufLen);

/**
 * @brief Relocate file position pointer
 *
 * @param[in] int fd
 *          - File ID
 *
 * @param[in] int offset
 *          - Offset
 *
 * @param[in] esim_ap_fs_seek_flag_e whence
 *          - Offset starting position
 *
 * @return int
 *          - Success, 0
 *          - Failure, -1
 */
extern int ipa_ap_fs_lseek(int fd, int offset, esim_ap_fs_seek_flag_e whence);

/**
 * @brief Delete file
 *
 * @param[in] const char *file_path
 *          - File path
 *
 * @return int
 *          - Success, 0
 *          - Failure, -1
 */
extern int ipa_ap_fs_unlink(const char *file_path);

/**
 * @brief Check if the file already exists
 *
 * @param[in] const char *file_path
 *          - File path
 *
 * @return int
 *          - Exists, 0
 *          - Does not exist, -1
 */
extern int ipa_ap_fs_exist(const char *file_path);

/**
 * @brief Get file size
 *
 * @param[in] int fd
 *          - File ID

 *
 * @return int
 *          - Success, file size
 *          - Does not exist, -1
 */
extern int ipa_ap_fs_ftell(int fd);

#endif
