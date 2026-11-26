#include "esim_hal_fs.h"
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "qosa_virtual_file.h"
#include "errno.h"

#define QOS_LOG_TAG LOG_TAG_ESIM

char g_directory[] = "/esim_ipa_file";  //The directory address is specified by the platform.

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
char *ipa_ap_fs_directory_get(void)
{
    return g_directory;
}

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
int ipa_ap_fs_mkdir(int mode)
{
    int  ret = 0;
    char directory[32] = {0};

    qosa_snprintf(directory, 32, "%s/", g_directory);
    ret = qosa_vfs_mkdir(g_directory, mode);
    if (0 != ret)
    {
        QLOGI("mkdir [%s] dir failed, ret=%d \n", directory, ret);
        ret = -1;
    }

    return ret;
}

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
int ipa_ap_fs_open(const char *file_path, esim_ap_fs_flag_e flag)
{
    int my_fd = -1;
    int mode = 0;

    QLOGI("open file:%s .flag:0x%x \n", file_path, flag);
    if (flag & ESIM_AP_FS_O_RDONLY)
    {
        mode |= QOSA_VFS_O_RDONLY;
    }
    if (flag & ESIM_AP_FS_O_WRONLY)
    {
        mode |= QOSA_VFS_O_WRONLY;
    }
    if (flag & ESIM_AP_FS_O_APPEND)
    {
        mode |= QOSA_VFS_O_APPEND;
    }
    if (flag & ESIM_AP_FS_O_RDWR)
    {
        mode |= QOSA_VFS_O_RDWR;
    }
    if (flag & ESIM_AP_FS_O_CREAT)
    {
        mode |= QOSA_VFS_O_CREAT;
    }
    if (flag & ESIM_AP_FS_O_EXCL)
    {
        mode |= QOSA_VFS_O_EXCL;
    }
    if (flag & ESIM_AP_FS_O_TRUNC)
    {
        mode |= QOSA_VFS_O_TRUNC;
    }

    my_fd = qosa_vfs_open(file_path, mode);
    QLOGI("open file fd:0x%x .mode:0x%x \n", my_fd, mode);
    if (my_fd == -1)
    {
        return -1;
    }

    return my_fd;
}

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
int ipa_ap_fs_close(int fd)
{
    int status = 0;

    status = qosa_vfs_close(fd);
    if (status < 0)
    {
        QLOGI("close file err:%d \n", status);
        return -1;
    }

    return 0;
}

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
 *          - Data length to write
 *
 * @return int
 *          - Number of bytes successfully written, >0
 *          - Failure, other
 */
int ipa_ap_fs_write(int fd, const void *buf, int buf_len)
{
    int wr_bytes = 0;

    wr_bytes = qosa_vfs_write(fd, (void *)buf, buf_len);
    QLOGI("write file:0x%x , wr_bytes:%d\n", fd, wr_bytes);
    return wr_bytes;
}

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
int ipa_ap_fs_read(int fd, void *buf, int buf_len)
{
    int rd_bytes = 0;

    QLOGI("read file:%d \n", fd);
    rd_bytes = qosa_vfs_read(fd, buf, buf_len);
    QLOGI("rd_bytes bytes:%d\n", rd_bytes);
    return rd_bytes;
}

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
int ipa_ap_fs_lseek(int fd, int offset, esim_ap_fs_seek_flag_e whence)
{
    qosa_int64_t ret = 0;
    int          my_whence = 0;

    switch (whence)
    {
        case ESIM_AP_FS_SEEK_SET:
            my_whence = QOSA_VFS_SEEK_SET;
            break;
        case ESIM_AP_FS_SEEK_CUR:
            my_whence = QOSA_VFS_SEEK_CUR;
            break;
        case ESIM_AP_FS_SEEK_END:
            my_whence = QOSA_VFS_SEEK_END;
            break;
        default:
            return -1;
    }

    ret = qosa_vfs_lseek(fd, offset, my_whence);
    QLOGI("qosa_vfs_lseek ret:0x%x \n", fd);
    if (ret == -1)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief Delete file
 *
 * @param[in] const char *file_path
 *          - File path
 *
 * @return int
 *          - Success, 0
 *          - Failed, -1
 */
int ipa_ap_fs_unlink(const char *file_path)
{
    int status;
    status = qosa_vfs_unlink(file_path);
    if (status != 0)
    {
        QLOGI("vsim_ap_fs_unlink=0x%x \r\n", status);
        return -1;
    }

    return 0;
}

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
int ipa_ap_fs_exist(const char *file_path)
{
    int                    status;
    struct qosa_vfs_stat_t st = {0};

    status = qosa_vfs_stat(file_path, &st);
    if (status != 0)
    {
        QLOGI("file[%s] not exist\r\n", file_path);
        return -1;
    }

    return 0;
}

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
int ipa_ap_fs_ftell(int fd)
{
    int                    ret = 0;
    struct qosa_vfs_stat_t st = {0};

    ret = (int)qosa_vfs_fstat(fd, &st);
    QLOGI("qosa_vfs_fstat ret:0x%x\r\n", ret);
    if (ret == 0)
    {
        /*Print file size*/
        return st.st_size;
    }
    return -1;
}
