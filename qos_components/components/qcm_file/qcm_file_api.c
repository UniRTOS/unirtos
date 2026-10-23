/*****************************************************************/ /**
* @file file_api.c
* @brief
* @author larson.li@quectel.com
* @date 2023-06-20
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2023-06-20 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#include "qcm_proj_config.h"
#include "qcm_file_api.h"
#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_dev.h"
#endif
#include "qosa_sys.h"
#include "qosa_log.h"
#include "qosa_errno.h"
#include "qcm_utf8.h"
#include "components_nv_cfg.h"

#ifdef CONFIG_QCM_FILE_SFS_FUNC
#include "qcm_qsfs_api.h"
#endif

#define QOS_LOG_TAG LOG_TAG_FILE_API
#if !defined(require_action)
#define require_action(x, action, str)                                                                                                                         \
    do {                                                                                                                                                       \
        if ((x) != 0)                                                                                                                                          \
        {                                                                                                                                                      \
            QLOGE(str);                                                                                                                                        \
            {                                                                                                                                                  \
                action;                                                                                                                                        \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    } while (1 == 0)
#endif

/** The value of this part lies in the fact that we only need to set these modes in the device within the font module.*/
/** File read-only attribute*/
#define QCM_FILE_OPEN_FLAGS_R       (QOSA_VFS_O_RDONLY)
/** File read-only attribute*/
#define QCM_FILE_OPEN_FLAGS_W       (QOSA_VFS_O_WRONLY | QOSA_VFS_O_CREAT | QOSA_VFS_O_TRUNC)
/** Open the write-only file in append mode*/
#define QCM_FILE_OPEN_FLAGS_A       (QOSA_VFS_O_WRONLY | QOSA_VFS_O_CREAT | QOSA_VFS_O_APPEND)
/** Open a readable and writable file, which must exist.*/
#define QCM_FILE_OPEN_FLAGS_R_PLUSE (QOSA_VFS_O_RDWR)
/** Open a readable and writable file; if the file exists, its length is cleared to zero, meaning the file content will disappear. If the file does not exist, create the file.*/
#define QCM_FILE_OPEN_FLAGS_W_PLUSE (QOSA_VFS_O_RDWR | QOSA_VFS_O_CREAT | QOSA_VFS_O_TRUNC)
/** Open a file for reading and writing in append mode. If the file does not exist, it will be created. If the file exists, data written will be appended to the end of the file, meaning the original content of the file will be preserved.*/
#define QCM_FILE_OPEN_FLAGS_A_PLUSE (QOSA_VFS_O_RDWR | QOSA_VFS_O_CREAT | QOSA_VFS_O_APPEND)

//for judge the type of the input file
static qcm_file_type_list_t g_file_type_list[] = qcm_file_type_list_default;
//Used to store the list of information for opened files
static qosa_q_type_t g_save_open_list;

static int file_get_file_info(qcm_file_open_list_t *info, qosa_bool_t by_name, int fd, char *name);

/**
 * @brief Determine whether it is an SFS operation based on the converted real path or the corresponding opened file descriptor.
 *
 * @param[in] char * real_path
 *          - Pass in the converted real path information
 *
 * @param[in] int fd
 *          - Corresponding file descriptor for the opened file
 *
 * @return qosa_bool_t
 *       - If the sfs file operation returns OSA_TRUE
 *       - If not, return OSA_FALSE
 */
qosa_bool_t qcm_file_check_support_sfs(char *real_path, int fd)
{
    if (QOSA_NULL != real_path)
    {
        if (qosa_strncmp(real_path, g_file_type_list[QCM_FILE_SFS].path_name, g_file_type_list[QCM_FILE_SFS].path_name_len) == 0)
        {
            return QOSA_TRUE;
        }
    }
    else
    {
        qcm_file_open_list_t *open_file_ptr = QOSA_NULL;
        open_file_ptr = qcm_file_get_first_file_info();
        while (open_file_ptr != QOSA_NULL)
        {
            if (fd == open_file_ptr->fd)
            {
                return open_file_ptr->is_sfs;
            }
            open_file_ptr = qcm_file_get_next_file_info(open_file_ptr);
        }
    }

    return QOSA_FALSE;
}

/**
 * @brief Get the type_name corresponding to the specified type
 *
 * @param[in] qcm_file_type_e type_e
 *          - The type to be queried
 *
 * @return char *
 *        - Returns the corresponding type name; note that the free function must not be used for deallocation.
 */
char *qcm_file_get_type_name(qcm_file_type_e type_e)
{
    if (type_e >= QCM_FILE_MAX)
    {
        return QOSA_NULL;
    }
    return g_file_type_list[type_e].type_name;
}

/**
 * @brief Get the length of the type_name for the corresponding type
 *
 * @param[in] qcm_file_type_e type_e
 *          - The type to be queried
 *
 * @return qosa_size_t
 *        - Returns the length of the corresponding type name
 */
qosa_size_t qcm_file_get_type_name_len(qcm_file_type_e type_e)
{
    if (type_e >= QCM_FILE_MAX)
    {
        return 0;
    }
    return g_file_type_list[type_e].type_name_len;
}

/**
 * @brief Get the folder directory level, default is only level 1.
 *
 * @param[in] const char * path
 *          - Files to be checked
 *
 * @return int
 *       - Returns the actual directory level on success
 *       - Returns -1 on failure
 */
int qcm_file_get_dir_level(const char *path)
{
    int         ilevel = 1;  //default 1 level
    qosa_size_t i = 0;
    if (path == QOSA_NULL)
    {
        return -1;
    }
    char *temp_path = (char *)path;
    char *path_turn = qcm_file_dedotdotify(temp_path);
    if (path_turn == QOSA_NULL)
        return -1;

    if (path_turn[0] == '/' || QOSA_NULL != qosa_strstr(path_turn, ":/"))
    {
        ilevel -= 1;
    }

    for (i = 0; i < qosa_strlen(path_turn); i++)
    {
        if (path_turn[i] == '/')
        {
            if (i != qosa_strlen(path_turn) - 1)
                ilevel++;
        }
    }
    qosa_free(path_turn);
    return ilevel;
}

/**
 * @brief The function processes the input file path by removing special characters and redundant parts, and returns the processed path.
 *
 * @param[in] char * input
 *           - Full path name of the file to be checked
 *
 * @return char *
 *        - Success: Pointer to the processed path string. The returned pointer needs to be freed using free.
 *        - Failure: returns OSA_NULL
 *
 * @note
 *     If the returned pointer is not OSA_NULL, it must be freed using qcm_free.
 */
char *qcm_file_dedotdotify(char *input)
{
    /**
 * The function `qcm_file_dedotdotify` processes the input file path by removing special characters and redundant parts, returning the processed path. The specific processing logic is as follows:
 * 1. First, check if the input is OSA_NULL or an empty string; if so, directly return OSA_NULL.
 * 2. Create an output buffer `out` with a size equal to the length of the input buffer `input` plus 1. Simultaneously, create an output pointer `outptr` pointing to the starting position of the output buffer.
 * 3. Copy the input buffer `input` to a temporary buffer `clone`, and record the length of the input buffer as `clen`.
 * 4. If the clone is an empty string, return the clone directly.
 * 5. Enter a loop to continuously process the clone until the entire path is processed.
 * 6. Process the clone prefix according to the following rules:
 *     a. If clone starts with "./", remove this prefix by moving the clone pointer forward by 2 positions and subtracting 2 from clen.
 *     b. If the clone starts with "//", remove one slash, move the clone pointer back by 1 position, and subtract 1 from clen.
 *     c. If clone starts with "../", remove this prefix by moving the clone pointer forward by 3 positions and subtracting 3 from clen.
 *     d. If the clone starts with "///", remove this prefix by moving the clone pointer forward by 3 positions and subtracting 3 from clen.
 * 7. Process the prefix of clone according to the following rules:
 *     a. If the clone starts with "/./", replace this prefix with "/", move the clone pointer forward by 2 positions, and subtract 2 from clen.
 *     b. If clone equals "/.", replace "/." with "/", move the clone pointer back by 1 position, and subtract 1 from clen.
 * 8. Process the prefix of clone according to the following rules:
 *     a. If clone starts with "/../", replace this prefix with "/", move the clone pointer forward by 3 positions, and subtract 3 from clen.
 *     b. If clone equals "/..", replace "/.." with "/", move the clone pointer forward by 2 positions, and subtract 2 from clen.
 *     c. Meanwhile, starting from the end of the output buffer and searching backward, locate the last path separator "/", and delete it along with the portion that follows.
 * 9. Process the clone according to the following rules:
 *     a. If clone equals "." or "..", set clone to an empty string and set the output buffer out to an empty string.
 *     b. Otherwise, copy the first path segment from the clone to the output buffer out.
 * 10. After processing the input path `../path/./to/./file.txt` through the `file_dedotdotify` function, the output result will be `path/to/file.txt`.
 */
    if (input == QOSA_NULL || qosa_strlen(input) == 0)
    {
        return QOSA_NULL;
    }
    QLOGV("input=%s", input);
    qosa_size_t inlen = qosa_strlen(input);
    char       *clone;
    qosa_size_t clen = inlen; /* the length of the cloned input */
    char       *out = (char *)qosa_malloc(inlen + 1);
    char       *outptr;
    char       *orgclone;
    if (!out)
        return QOSA_NULL; /* out of memory */

    *out = 0;             /* null-terminates, for inputs like "./" */

    /* get a cloned copy of the input */
    //clone = qosa_strdup(input);
    clone = (char *)qosa_malloc(inlen + 1);
    if (!clone)
    {
        qosa_free(out);
        return QOSA_NULL;
    }
    qosa_memcpy(clone, input, inlen);
    clone[inlen] = '\0';
    orgclone = clone;
    outptr = out;

    if (!*clone)
    {
        /* zero length string, return that */
        qosa_free(out);
        QLOGV("clone=%s", clone);
        return clone;
    }

    do {
        /*  A.  If the input buffer begins with a prefix of "../" or "./", then
		remove that prefix from the input buffer; otherwise, */

        if (!qosa_strncmp("./", clone, 2))
        {
            clone += 2;
            clen -= 2;
        }
        else if (!qosa_strncmp("//", clone, 2))
        {
            clone += 1;
            clen -= 1;
        }
        else if (!qosa_strncmp("../", clone, 3))
        {
            clone += 3;
            clen -= 3;
        }
        else if (!qosa_strncmp("///", clone, 3))
        {
            clone += 3;
            clen -= 3;
        }

        /*  B.  if the input buffer begins with a prefix of "/./" or "/.", where
		"."  is a complete path segment, then replace that prefix with "/" in
		the input buffer; otherwise, */
        else if (!qosa_strncmp("/./", clone, 3))
        {
            clone += 2;
            clen -= 2;
        }
        else if (!qosa_strcmp("/.", clone))
        {
            clone[1] = '/';
            clone++;
            clen -= 1;
        }

        /*  C.  if the input buffer begins with a prefix of "/../" or "/..", where
		".." is a complete path segment, then replace that prefix with "/" in
		the input buffer and remove the last segment and its preceding "/" (if
		any) from the output buffer; otherwise, */

        else if (!qosa_strncmp("/../", clone, 4))
        {
            clone += 3;
            clen -= 3;
            /* remove the last segment from the output buffer */
            while (outptr > out)
            {
                outptr--;
                if (*outptr == '/')
                    break;
            }
            *outptr = 0; /* null-terminate where it stops */
        }
        else if (!qosa_strcmp("/..", clone))
        {
            clone[2] = '/';
            clone += 2;
            clen -= 2;
            /* remove the last segment from the output buffer */
            while (outptr > out)
            {
                outptr--;
                if (*outptr == '/')
                    break;
            }
            *outptr = 0; /* null-terminate where it stops */
        }

        /*  D.  if the input buffer consists only of "." or "..", then remove
		that from the input buffer; otherwise, */

        else if (!qosa_strcmp(".", clone) || !qosa_strcmp("..", clone))
        {
            *clone = 0;
            *out = 0;
        }

        else
        {
            /*  E.  move the first path segment in the input buffer to the end of
		the output buffer, including the initial "/" character (if any) and
		any subsequent characters up to, but not including, the next "/"
		character or the end of the input buffer. */

            do {
                *outptr++ = *clone++;
                clen--;
            } while (*clone && (*clone != '/'));
            *outptr = 0;
        }

    } while (*clone);

    qosa_free(orgclone);
    QLOGV("out=%s clen=%d", out, clen);
    return out;
}
#if 0
/**
 * @brief Get the level of the path name, but do not verify whether the path exists, only evaluate the path name.
 *
 *  Example: -----
 *  sOutPath="test/goodad" returns 2, indicating a second-level directory.
 *  sOutPath="/test/goodad" returns 3, indicating three levels of directories.
 *  sOutPath="test/goodad/" returns 2, indicating a second-level directory.
 *
 * @param[in] const char * path
 *          - Full path name of the file to be checked
 *
 * @return qosa_uint8_t
 *        - Returns the number of levels in the path upon success
 *        - Returns -1 on failure
 */
static qosa_uint8_t unir_file_get_dir_level(const char *path)
{
    int   ilevel = 1;  //default 1 level
    int   path_len = 0;
    int   i = 0;
    char *temp_path = QOSA_NULL;
    char *path_turn = QOSA_NULL;
    if (path == QOSA_NULL)
    {
        return -1;
    }
    temp_path = (char *)path;
    path_turn = qcm_file_dedotdotify(temp_path);
    if (path_turn == QOSA_NULL)
        return -1;

    if (path_turn[0] == '/' || QOSA_NULL != qosa_strstr(path_turn, ":/"))
    {
        ilevel -= 1;
    }
    path_len = qosa_strlen(path_turn);
    for (i = 0; i < path_len; i++)
    {
        if (path_turn[i] == '/')
        {
            if (i != path_len - 1)
                ilevel++;
        }
    }
    qosa_free(path_turn);
    return ilevel;
}
#endif
// for get file type, if not support the file, return FILE_MAX

/**
 * @brief Obtain the corresponding file type to operate on by splitting with a colon.
 *
 * If UFS:1.txt corresponds to the type, it is of the UFS type.
 *
 * @param[in] char * param_str
 *           - To obtain the file type
 *
 * @return qcm_file_type_e
 *        - Returns the type information to be obtained
 */
qcm_file_type_e qcm_file_get_type(char *param_str)
{
    int i = 0;
    if (QOSA_NULL == param_str)
    {
        QLOGE("unir_file_type_get param invalid");
        return QCM_FILE_MAX;
    }
    if (qosa_strchr(param_str, ':'))  //outside path, like "UFS:a.txt"
    {
        while (g_file_type_list[i].file_type < QCM_FILE_MAX)
        {
            if (g_file_type_list[i].type_name == QOSA_NULL || g_file_type_list[i].path_name == QOSA_NULL)
            {
                i++;
                continue;
            }
            if (0 == qosa_strncmp(param_str, g_file_type_list[i].type_name, g_file_type_list[i].type_name_len))
            {
                return (qcm_file_type_e)i;
            }
            else
            {
                i++;
            }
        }
        return QCM_FILE_MAX;
    }
    else
    {
        return QCM_FILE_UFS;
    }
}

/**
 * @brief Address conversion, transforming the original path into a formatted path address
 *
 * @param[in] const char * path_name
 *          - Original path address, such as /fota/1.zip
 *
 * @param[in/out] char * type_name
 *          - Convert to a typed address, such as FOTA:1.zip
 *
 * @param[out] qosa_size_t * type_name_length
 *          - Corresponding converted file name length
 *
 * @return int
 *       - Returns 0 if successful
 *       - Returns -1 on failure
 */
int qcm_file_path_type_covert(const char *path_name, char *type_name, qosa_size_t *type_name_length)
{
    int i = 0;
    if (QOSA_NULL == path_name || QOSA_NULL == type_name || QOSA_NULL == type_name_length || *type_name_length <= 0)
    {
        QLOGE("param invalid");
        return -1;
    }
    while (g_file_type_list[i].file_type < QCM_FILE_MAX)
    {
        if (g_file_type_list[i].type_name == QOSA_NULL || g_file_type_list[i].path_name == QOSA_NULL)
        {
            i++;
            continue;
        }
        if (0 == qosa_strncmp(path_name, g_file_type_list[i].path_name, qosa_strlen(g_file_type_list[i].path_name)))
        {
            if (qosa_strlen(g_file_type_list[i].type_name) + qosa_strlen(path_name + qosa_strlen(g_file_type_list[i].path_name)) >= *type_name_length)
            {
                QLOGE("length failed");
                return -1;
            }
            qosa_sprintf(type_name, "%s%s", g_file_type_list[i].type_name, path_name + qosa_strlen(g_file_type_list[i].path_name));
            *type_name_length = qosa_strlen(type_name) + 1;
            return 0;
        }
        else
        {
            i++;
        }
    }
    QLOGE("error:%s,%s,%d", path_name, type_name, *type_name_length);
    return -1;
}

/**
 * @brief Check if the passed filename is valid
 *
 * 1. Only check if the file characters meet the requirements, without performing systematic interactive validation on the file name.
 *
 * @param[in] char * file_name
 *          - File name to be checked
 *            The file_name content is of type UFS:xxx or directly a path.
 *
 * @return int
 *         - If valid, returns OSA_OK,
 *           Otherwise, return other values.
 */
int qcm_file_is_name_valid(char *file_name)
{
    int ret = QOSA_OK;
    qcm_file_type_e file_type = QCM_FILE_MAX;
    char *tmp_ptr = NULL;
    char *name_valid = NULL;
    if (file_name == QOSA_NULL || qosa_strlen(file_name) == 0)
    {
        ret = -1;
        goto exit;
    }

    file_type = qcm_file_get_type(file_name);
    if (file_type == QCM_FILE_MAX)
    {
        ret = -2;
        goto exit;
    }

    //After successfully obtaining the file type, retrieve the directory information corresponding to that type, and simultaneously verify for any errors.
    tmp_ptr = qosa_strrchr(file_name, ':');  // ':' can only be followed by DEV(ex:UFS, SD...).
    if (tmp_ptr && tmp_ptr != file_name + g_file_type_list[file_type].type_name_len - 1)
    {
        ret = -3;
        goto exit;
    }

    tmp_ptr = qosa_strchr(file_name, '*');  // '*' can only at end and its previous character must be ':' or '/'
    if ((tmp_ptr && tmp_ptr != file_name + qosa_strlen(file_name) - 1) || (tmp_ptr > file_name && *(tmp_ptr - 1) != ':' && *(tmp_ptr - 1) != '/')
        /* || file_name[0] == '/' || qosa_strstr(file_name,":/") != QOSA_NULL */)
    {
        ret = -4;
        goto exit;
    }

    name_valid = file_name;
    int   i, len;
    if (QOSA_NULL != (tmp_ptr = qosa_strstr(name_valid, ":/")))  // "UFS:/123.txt" ==> "UFS:123.txt"
    {
        tmp_ptr++;
        len = qosa_strlen(tmp_ptr);
        for (i = 0; i < len; i++)
        {
            tmp_ptr[i] = tmp_ptr[i + 1];
        }
    }

    len = qosa_strlen(name_valid);
    for (i = 0; i < len; i++)  //Check if special characters are present
    {
        if ( /* || name_valid[i] == '?' || name_valid[i] == '<' || name_valid[i] == '>' || name_valid[i] == '|' */
			name_valid[i] == '\\' || name_valid[i] == '"' )
        {
            ret = -5;
            goto exit;
        }
    }
    for (i = 0; i < len; i++)  //Determine if it is a valid string
    {
        //Knowledge point tip: The difference between '\x01' and 0x01
        if ((name_valid[i] >= '\x01' && name_valid[i] <= '\x1F') || (name_valid[i] >= '\x7F' && name_valid[i] <= '\x9F'))
        {
            if (qcm_utf8_validate_string(name_valid))
            {
                break;
            }
            ret = -7;
            goto exit;
        }
    }

    if ((!qosa_strchr(name_valid, ':') && len >= QCM_FILE_MAX_PATH_LEN) || (qosa_strchr(name_valid, ':') && len >= QCM_FILE_FILENAME_MAX_LEN))
    {
        ret = -6;
        goto exit;
    }

exit:

    return ((QOSA_OK == ret) ? QOSA_OK : QOSA_ERROR_GENERAL);
}

/**
 * @brief Check if the input path information is correct, and attempt to convert the path information to a real path.
 *        If the conversion fails or the converted length exceeds the limit, it fails.
 *
 * @param[in] char * input_path
 *            - The path information to be checked is UFS: type file
 *
 * @param[out] char * real_path
 *           - The verified and converted real path, corresponding to the actual /user directory
 *
 * @param[in/out] int * path_size
 *           - Input path name buffer size, output actual path name length
 *
 * @param[out] int * file_type
 *           - Check the file type information corresponding to the current path
 *
 * @return Q_STATE
 *         - Returns QCM_FILE_OK to indicate successful data verification
 *         - Returns a negative number if the path is invalid
 */
Q_STATE qcm_file_check_path(char *input_path, char *real_path, int *path_size, int *file_type)
{
    qcm_file_type_e file_real_type;
    char           *name_turn;
    char           *name_valid;
    char           *tmp_ptr;
    int             type_name_len;
    const char     *type_name;
    const char     *path_name;
    int             path_name_len;
    char           *path_end_p;
    int             written_len;
    int             real_path_size;

    if (real_path == QOSA_NULL || input_path == QOSA_NULL || path_size == QOSA_NULL)
    {
        return -1;
    }

    real_path_size = *path_size;
    if (real_path_size <= 0)
    {
        return -1;
    }
    if (qcm_file_is_name_valid(input_path) != QOSA_OK)
    {
        return -2;
    }

    file_real_type = qcm_file_get_type(input_path);
    if (file_real_type == QCM_FILE_MAX)
    {
        return -1;
    }

    name_turn = qcm_file_dedotdotify(input_path);
    if (name_turn == QOSA_NULL)
    {
        return -1;
    }

    // Skip leading '/' if exists
    name_valid = (name_turn[0] == '/') ? name_turn + 1 : name_turn;
    // Remove ":/" from paths like "UFS:/123.txt"
    tmp_ptr = qosa_strstr(name_valid, ":/");
    if (tmp_ptr != QOSA_NULL)
    {
        qosa_memmove(tmp_ptr, tmp_ptr + 1, qosa_strlen(tmp_ptr));
    }

    type_name_len = g_file_type_list[file_real_type].type_name_len;
    type_name = g_file_type_list[file_real_type].type_name;
    path_name = g_file_type_list[file_real_type].path_name;
    path_name_len = g_file_type_list[file_real_type].path_name_len;

    if (qosa_strncmp(name_valid, type_name, type_name_len) == 0)
    {
        written_len = path_name_len + qosa_strlen(name_valid + type_name_len);
        if (written_len >= real_path_size)
        {
            qosa_free(name_turn);
            return -1;
        }
        qosa_snprintf(real_path, real_path_size, "%s%s", path_name, name_valid + type_name_len);
    }
    else
    {
        written_len = path_name_len + qosa_strlen(name_valid);
        if (written_len >= real_path_size)
        {
            qosa_free(name_turn);
            return -1;
        }
        qosa_snprintf(real_path, real_path_size, "%s%s", path_name, name_valid);
    }

    path_end_p = qosa_strrchr(real_path, '/');
    if (path_end_p == QOSA_NULL || path_end_p - real_path + 1 > CONFIG_QOSA_FILE_MAX_FOLDER_LEN || qosa_strlen(path_end_p + 1) > CONFIG_QOSA_FILE_MAX_NAME_LEN)
    {
        qosa_free(name_turn);
        return -1;
    }

    if (file_type != QOSA_NULL)
    {
        *file_type = file_real_type;
    }

    //Returns the actual number of characters written (excluding the terminating '\0')
    *path_size = written_len;

    qosa_free(name_turn);
    return QCM_FILE_OK;
}

#if 0
/**
 * @brief Convert UFS and other types to real path information used by VFS, such as /user, etc.
 *
 * @param[in] const char * path_name
 *            - Full path of the file to be converted
 *
 * @param[out] char * type_name
 *            - Converted path content
 *
 * @param[in/out] int * type_name_length
 *           - [in] Corresponding type_name path space length
 *           - [out] If processing is successful, returns the processed address length
 *
 * @return int
 *        - If 0 is returned, it indicates successful execution.
 *        - Returns a negative number to indicate execution failure
 */
static int file_path_type_convert(const char *path_name, char *type_name, int *type_name_length)
{
    int i = 0;
    int name_len = 0;
    if (QOSA_NULL == path_name || QOSA_NULL == type_name || *type_name_length < 0)
    {
        QLOGV("param invalid");
        return -1;
    }
    while (g_file_type_list[i].file_type < QCM_FILE_MAX)
    {
        if (g_file_type_list[i].type_name == QOSA_NULL || g_file_type_list[i].path_name == QOSA_NULL)
        {
            i++;
            continue;
        }
        if (0 == qosa_strncmp(path_name, g_file_type_list[i].path_name, g_file_type_list[i].path_name_len))
        {
            name_len = g_file_type_list[i].type_name_len + qosa_strlen(path_name + g_file_type_list[i].path_name_len);
            if (name_len >= *type_name_length)
            {
                QLOGV("length failed");
                return -1;
            }
            qosa_sprintf(type_name, "%s%s", g_file_type_list[i].type_name, path_name + g_file_type_list[i].path_name_len);
            *type_name_length = name_len + 1;
            return 0;
        }
        else
        {
            i++;
        }
    }
    QLOGV("error:%s,%s,%d", path_name, type_name, *type_name_length);
    return -1;
}
#endif
/**
 * @brief Obtain the corresponding file type through the already opened file descriptor fd
 *
 * @param[in] int fd
 *          - File descriptor information to query
 *
 * @return qcm_file_type_e
 *        - Returns the actual file type found in the query
 *        - If QCM_FILE_MAX is returned, it indicates that no match was found.
 */
qcm_file_type_e qcm_file_type_get_by_fd(int fd)
{
    int                  err;
    qcm_file_open_list_t file_info = {0};

    err = file_get_file_info(&file_info, QOSA_FALSE, fd, QOSA_NULL);
    if (err)
    {
        return QCM_FILE_MAX;
    }

    return qcm_file_get_type(file_info.file_name);
}

/**
 * @brief Convert the open types such as rwb in the FOPEN parameters to the read/write types of the open interface.
 *
 * @param[in] char * mode
 *          - Mode types for the FOPEN interface input
 *
 * @return int
 *        - Returns the converted mode type
 */
int qcm_file_get_mode(char *mode)
{
    if (mode == QOSA_NULL)
    {
        QLOGE("invalid param");
        return -1;
    }

    int mode_len = qosa_strlen(mode);

    if (mode_len < 1 || mode_len > 3)
    {
        QLOGE("invalid mode length");
        return -1;
    }

    char *r_chr = qosa_strchr(mode, 'r');
    char *w_chr = qosa_strchr(mode, 'w');
    char *a_chr = qosa_strchr(mode, 'a');
    char *b_chr = qosa_strchr(mode, 'b');
    char *t_chr = qosa_strchr(mode, 't');
    char *plus_chr = qosa_strchr(mode, '+');

    int rwa_cnt = (r_chr ? 1 : 0) + (w_chr ? 1 : 0) + (a_chr ? 1 : 0);
    int bt_cnt = (b_chr ? 1 : 0) + (t_chr ? 1 : 0);
    int plus_cnt = plus_chr ? 1 : 0;

    //Check the combination of 'r', 'w', 'a'
    if (rwa_cnt != 1)
    {
        QLOGE("invalid read/write/append mode");
        return -1;
    }

    //Check the combination of 'b' and 't', and whether '+' conforms to logic
    if ((bt_cnt > 1) || (mode_len == 2 && bt_cnt + plus_cnt != 1) || (mode_len == 3 && bt_cnt != 1))
    {
        QLOGE("invalid mode combination");
        return -1;
    }

    int flags = 0;

    //Set basic flags based on 'r', 'w', 'a'
    if (r_chr)
    {
        flags = QOSA_VFS_O_RDONLY;
        if (plus_cnt)
            flags = QOSA_VFS_O_RDWR;
    }
    else if (w_chr)
    {
        flags = QOSA_VFS_O_WRONLY | QOSA_VFS_O_CREAT | QOSA_VFS_O_TRUNC;
        if (plus_cnt)
            flags = QOSA_VFS_O_RDWR | QOSA_VFS_O_CREAT | QOSA_VFS_O_TRUNC;
    }
    else if (a_chr)
    {
        flags = QOSA_VFS_O_WRONLY | QOSA_VFS_O_CREAT | QOSA_VFS_O_APPEND;
        if (plus_cnt)
            flags = QOSA_VFS_O_RDWR | QOSA_VFS_O_CREAT | QOSA_VFS_O_APPEND;
    }

    return flags;
}

/**
 * @brief Through the already opened file descriptor fd, to obtain the corresponding already opened file mode information
 *
 * @param[in] int fd
 *          - The file descriptor handle to query
 *
 * @return int
 *        - Returns the actual mode information
 *        - Returns -1 indicates query failure
 */
int qcm_file_get_mode_by_fd(int fd)
{
    int                  err;
    qcm_file_open_list_t file_info;

    err = file_get_file_info(&file_info, QOSA_FALSE, fd, QOSA_NULL);
    if (err)
    {
        return -1;
    }

    return file_info.file_open_mode;
}

/**
 * @brief Query the file information status of the corresponding file
 *
 * @param[out] qcm_file_stat_vfs_t * state
 *            - Returns the corresponding file status information
 *
 * @param[in] char * path
 *           - The folder information to be queried, the path should correspond to the real path information of vfs rather than the UFS converted path
 *
 * @return int
 *        - Returns 0 to indicate successful execution
 *        - Returns -1 indicates execution failure
 */
static int file_get_state_into(qcm_file_stat_vfs_t *state, char *path)
{
    int err = 0;
    if (state == QOSA_NULL)
    {
        QLOGE("invalid param");
        return -1;
    }
    err = qosa_vfs_statvfs((const char *)path, (struct qosa_vfs_statvfs_t *)state);
    if (err < 0)
    {
        QLOGE("get file system failed");
        return -1;
    }
    else
        return 0;
}

/**
 * @brief Save the already opened file information into the linked list.
 *
 * @param[in] qcm_file_open_list_t * file_info
 *           - Pointer to the file information to be saved
 *
 * @return int
 *       - If QCM_FILE_OK is returned, it indicates success,
 *         Otherwise, it indicates execution failure.
 */
static int file_open_info_push(qcm_file_open_list_t *file_info)
{
    qcm_file_open_list_t *save_info = QOSA_NULL;
    save_info = (qcm_file_open_list_t *)qosa_malloc(sizeof(qcm_file_open_list_t));
    if (save_info == QOSA_NULL)
    {
        QLOGE("qosa_malloc failed");
        return -1;
    }
    qosa_memcpy(save_info, file_info, sizeof(qcm_file_open_list_t));

    qosa_q_link((void *)&save_info, &save_info->list);
    qosa_q_put(&g_save_open_list, &save_info->list);
    return QCM_FILE_OK;
}

/**
 * @brief Compare based on the file names stored in the file_open_list_t type,
 *        If the file names are the same, it indicates a successful search.
 *
 * @param[in] void * node1
 *           - Corresponds to the file_open_list_t structure pointer
 *
 * @param[in] void * node2
 *           - Corresponding file name to be queried and compared
 *
 * @return qosa_bool_t
 *        - QOSA_TRUE indicates a successful search,
 *          QOSA_FALSE indicates that the search failed
 */
static qosa_bool_t compare_with_file_name(void *node1, void *node2)
{
    char                 *name = (char *)node2;
    qcm_file_open_list_t *open_list = (qcm_file_open_list_t *)node1;
    QLOGV("name=%s  open_list-name=%s", name, open_list->file_name);
    if (qosa_strlen(open_list->file_name) != qosa_strlen(name))
    {
        return QOSA_FALSE;
    }

    if (qosa_strncmp(open_list->file_name, name, qosa_strlen(name)) == 0)
    {
        return QOSA_TRUE;
    }
    else
    {
        return QOSA_FALSE;
    }
}

/**
 * @brief Compare based on the file handles stored in the file_open_list_t type,
 *        If the file handles are the same, it indicates a successful lookup.
 *
 * @param[in] void * node1
 *           - Corresponding to the file_open_list_t structure pointer
 *
 * @param[in] void * node2
 *           - Corresponding file handle to query and compare
 *
 * @return qosa_bool_t
 *        - QOSA_TRUE indicates a successful search,
 *          QOSA_FALSE indicates that the search failed
 */
static qosa_bool_t compare_with_file_fd(void *node1, void *node2)
{
    int                  *fd = (int *)node2;
    qcm_file_open_list_t *open_list = (qcm_file_open_list_t *)node1;
    QLOGV("fd=%d  open_list-fd=%d", fd, open_list->fd);
    if (*fd == open_list->fd)
    {
        return QOSA_TRUE;
    }
    else
    {
        return QOSA_FALSE;
    }
}

/**
 * @brief Find the corresponding saved file structure type by file name
 *
 * @param[in] qcm_file_open_list_t * info
 *          - Pointer to the file information handle
 *
 * @param[in] qosa_bool_t by_name
 *          - QOSA_FALSE indicates obtaining file information through an already opened file handle
 *          - QOSA_TRUE indicates obtaining file information through the file name
 *
 * @param[in] int fd
 *          - When by_name is OSA_FALSE, the file handle fd must be valid
 *
 * @param[in] char * name
 *          - When by_name is OSA_TRUE, the file name must actually exist
 *
 * @return int
 *        - If OSA_OK is returned, it indicates successful execution,
 *          Otherwise, execution fails.
 */
static int file_get_file_info(qcm_file_open_list_t *info, qosa_bool_t by_name, int fd, char *name)
{
    int                   info_cnt = 0;
    qcm_file_open_list_t *file_info = QOSA_NULL;
    int                   compare_fd = fd;
    if ((fd <= 0 && !by_name) || (!name && by_name) || !info)
    {
        QLOGE("invalid param");
        return -1;
    }

    info_cnt = qosa_q_cnt(&g_save_open_list);
    if (info_cnt <= 0)
    {
        QLOGE("get node count failed, or no file in list");
        goto exit;
    }

    if (by_name == QOSA_TRUE)
    {
        file_info = qosa_q_linear_search(&g_save_open_list, compare_with_file_name, (void *)name);
    }
    else
    {
        file_info = qosa_q_linear_search(&g_save_open_list, compare_with_file_fd, (void *)&compare_fd);
    }

    if (file_info != QOSA_NULL)
    {
        qosa_memcpy(info, file_info, sizeof(qcm_file_open_list_t));
        return 0;
    }

exit:
    return -1;
}

/**
 * @brief Find the specified file FD, dequeue it from the queue, and release it.
 *
 * @param[in] int fd
 *          - The file handle fd to be searched for
 *
 * @return int
 *      - If OSA_OK is returned, it indicates success,
 *        Otherwise, it indicates that the search execution failed.
 */
static int file_open_info_pop(int fd)
{
    int                   info_cnt;
    qcm_file_open_list_t *file_info = QOSA_NULL;
    int                   compare_fd = fd;
    info_cnt = qosa_q_cnt(&g_save_open_list);
    if (info_cnt <= 0)
    {
        QLOGE("get node count failed, or no file in list");
        goto exit;
    }

    file_info = qosa_q_linear_search(&g_save_open_list, compare_with_file_fd, (void *)&compare_fd);
    if (file_info != QOSA_NULL)
    {
        qosa_q_delete(&g_save_open_list, &file_info->list);
        qosa_free(file_info);
        return 0;
    }
    else
    {
        QLOGE("cannot find fd %d in list", fd);
    }

exit:
    return -1;
}

/**
 * @brief Retrieve the information of the first node in the currently opened file linked list
 *
 * @return qcm_file_open_list_t *
 *        - Returns the corresponding queried node information
 */
qcm_file_open_list_t *qcm_file_get_first_file_info(void)
{
    void *client_buf_ptr = QOSA_NULL;
    client_buf_ptr = qosa_q_check(&g_save_open_list);
    return (qcm_file_open_list_t *)client_buf_ptr;
}

/**
 * @brief Get the next node information in the current node
 *
 * @param[in] qcm_file_open_list_t * file_info
 *         - Pointer to the node to be queried
 *
 * @return qcm_file_open_list_t *
 *        - Returns the corresponding queried node information
 */
qcm_file_open_list_t *qcm_file_get_next_file_info(qcm_file_open_list_t *file_info)
{
    void *client_buf_ptr = QOSA_NULL;
    client_buf_ptr = qosa_q_next(&g_save_open_list, &file_info->list);
    return (qcm_file_open_list_t *)client_buf_ptr;
}

/**
 * @brief Description of obtaining an already opened file handle
 *
 * @return int
 *        - Returns the actual number of file handles that have been opened
 */
int qcm_file_open_get_cnt(void)
{
    return qosa_q_cnt(&g_save_open_list);
}

/**
 * @brief Determine if the current file is already open based on the file name
 *
 * @param[in] char * file_name
 *          - File name to be checked
 *
 * @return int
 *       - If it has already been opened, return the corresponding file socket handle,
 *         If any other error occurs, return -1
 */
int file_check_is_opened(char *file_name)
{
    int                  err;
    qcm_file_open_list_t file_info = {0};

    err = file_get_file_info(&file_info, QOSA_TRUE, 0, file_name);
    if (err)
    {
        return -1;
    }

    return file_info.fd;
}

//return -1 for not been opened, 0 for opened, or error
/**
 * @brief Determine whether the current file is open by checking the file handle.
 *
 * @param[in] int fd
 *         - File handle information to be searched
 *
 * @return int
 *        - Returns QCM_FILE_OK to indicate that the file has been opened,
 *         Returns other indicating the file is not open
 */
int qcm_file_is_opened_by_fd(int fd)
{
    int                  err;
    qcm_file_open_list_t file_info = {0};

    err = file_get_file_info(&file_info, QOSA_FALSE, fd, QOSA_NULL);
    if (err)
    {
        return -1;
    }

    return QCM_FILE_OK;
}

/**
 * @brief Get corresponding file name through already opened file handle
 *
 * @param[in] int fd
 *          - File handle information to query
 *
 * @param[out] char * name
 *          - Returns corresponding file name
 *          - Caller must provide a buffer of at least QCM_FILE_FILENAME_MAX_LEN bytes
 *
 * @return int
 *        - Returns QCM_FILE_OK indicates execution successful,
 *          returns other values indicates execution failed
 */
int qcm_file_get_file_name_by_fd(int fd, char *name)
{
    int                  err;
    qcm_file_open_list_t file_info = {0};

    if (name == QOSA_NULL)
    {
        return -1;
    }

    err = file_get_file_info(&file_info, QOSA_FALSE, fd, QOSA_NULL);
    if (err)
    {
        return -1;
    }

    qosa_strcpy(name, file_info.file_name);
    return QCM_FILE_OK;
}

/**
 * @brief Get the corresponding space size by file name
 *
 * @param[in] qcm_file_get_size_type_e type
 *          - Corresponding type for obtaining size
 *
 * @param[in] const char * path
 *         - Corresponding file name to be obtained, e.g., UFS:
 *
 * @return qosa_int64_t
 *        - Returns the actual queried size
 */
qosa_int64_t qcm_file_get_size(qcm_file_get_size_type_e type, const char *path)
{
    qosa_int64_t        size = 0;
    int                 ret;
    qcm_file_type_e     sys_type = QCM_FILE_MAX;
    qcm_file_stat_vfs_t vfs_state = {0};

    require_action((!path), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    //ret = qcm_file_check_path((char *)path, real_path, &sys_type);
    //require_action(ret, return QCM_FILE_INVALID_INPUT_VALUE, "INVALID old path");

    //ret = file_get_state_into(&vfs_state, real_path);
    sys_type = qcm_file_get_type((char *)path);
    QLOGV("path_name=%s", g_file_type_list[sys_type].path_name);
    ret = file_get_state_into(&vfs_state, g_file_type_list[sys_type].path_name);

    require_action(ret, return QCM_FILE_FAILED_TO_GET_STAT, "get system state failed");
    size = (type == QCM_FS_GET_SIZE_TOTAL) ? ((qosa_int64_t)(vfs_state.f_blocks) * (vfs_state.f_bsize))
                                           : ((qosa_int64_t)(vfs_state.f_bavail) * (vfs_state.f_bsize));
    require_action((size < 0), return QCM_FILE_ERROR_GENERAL, "get system state failed");

    if (sys_type == QCM_FILE_UFS || sys_type == QCM_FILE_FOTA || sys_type == QCM_FILE_DATA || sys_type == QCM_FILE_SFS)
    {
        if (size >= CONFIG_QOSA_FILE_CONFIG_RESERVE_SIZE + QCM_FILE_ALIGEN_RESERVE_SIZE)
        {
            size = size - CONFIG_QOSA_FILE_CONFIG_RESERVE_SIZE - QCM_FILE_ALIGEN_RESERVE_SIZE;
            return size;
        }
        else if (size + QCM_FILE_ALIGEN_RESERVE_SIZE > CONFIG_QOSA_FILE_CONFIG_RESERVE_SIZE)
        {
            return 0;
        }
        else
        {
            //It is more reasonable to report 0 here than to report an error.
            return 0;
            //return QCM_FILE_NO_SPACE;
        }
    }
#if !(defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    else if (sys_type == QCM_FILE_SD_1ST)
    {
        qosa_int64_t trace_size = 0;
        if (qosa_dev_get_sdmmc_is_mount(1, &trace_size) == QOSA_DEV_ERRID_SUCCESS)
        {
            QLOGV("harry trace_size=%ld", trace_size);
            if (size >= trace_size)
            {
                return size - trace_size;
            }
            else
            {
                return QCM_FILE_NO_SPACE;
            }
        }
        else
        {
            return QCM_FILE_GET_DISK_INFO_FAIL;
        }
    }
#endif
#if 0
    else if (sys_type == QCM_FILE_EXNSFFS)
    {
        return QCM_FILE_GET_DISK_INFO_FAIL;
    }
    else if (sys_type == QCM_FILE_EFS)
    {
        return QCM_FILE_GET_DISK_INFO_FAIL;
    }
    else if (sys_type == QCM_FILE_EMMC_1ST)
    {
        qosa_int64_t trace_size = 0;
        if(qosa_dev_get_sdmmc_is_mount(1, &trace_size) == QOSA_DEV_ERRID_SUCCESS)
        {
            QLOGV("harry trace_size=%ld", trace_size);
            if (size >= trace_size)
            {
                return size - trace_size;
            }
            else
            {
                return QCM_FILE_NO_SPACE;
            }
        }
        else
        {
            return QCM_FILE_GET_DISK_INFO_FAIL;
        }
    }
    else if (sys_type == QCM_FILE_SD_2ND || sys_type == QCM_FILE_EMMC_2ND)
    {
        return QCM_FILE_GET_DISK_INFO_FAIL;
    }
#endif

    return size;
}

/**
 * @brief Get the remaining space size of the file path
 *
 * @param[in] const char * path
 *           - Path name to be obtained, e.g., UFS:
 *
 * @return qosa_int64_t
 *        - Get actual remaining space size
 */
qosa_int64_t qcm_file_free_size(const char *path)
{
    return qcm_file_get_size(QCM_FS_GET_SIZE_FREE, path);
}
/**
 * @brief Get the remaining space size of the file path based on the file descriptor handle
 *
 * @param[in] int fd
 *           - The file descriptor handle to be obtained
 *
 * @return qosa_int64_t
 *        - Get actual remaining space size
 */
qosa_int64_t qcm_file_free_size_by_fd(int fd)
{
    int          err;
    qosa_int64_t free_size;
    char         name[QCM_FILE_FILENAME_MAX_LEN] = {0};

    err = qcm_file_get_file_name_by_fd(fd, name);
    require_action(err, return QCM_FILE_NOT_OPEN, "file not open");

    free_size = qcm_file_get_size(QCM_FS_GET_SIZE_FREE, name);
    require_action((free_size < 0), return free_size, "get qosa_free size failed");
    require_action((free_size == 0), return QCM_FILE_NO_SPACE, "qosa_free space is not enough");

    return free_size;
}
/**
 * @brief Get the space size of the file path
 *
 * @param[in] const char * path
 *           - Path name to be obtained, e.g., UFS:
 *
 * @return qosa_int64_t
 *        - Get actual space size
 */
qosa_int64_t qcm_file_total_size(const char *path)
{
    return qcm_file_get_size(QCM_FS_GET_SIZE_TOTAL, path);
}

/**
 * @brief Convert the errno error value from system file operations to custom ERROR codes
 *
 * @return qcm_file_error_e
 *        - Corresponding converted error code
 */
static qcm_file_error_e file_errocode_get(void)
{
    qcm_file_error_e errcode = QCM_FILE_OK;

#ifdef CONFIG_QOSA_LINUX_PLATFORM_FUNC
    switch (errno)
#else
    switch (qosa_get_errno())
#endif
    {
        case ENOENT:
            errcode = QCM_FILE_NOT_FOUND;
            break;

        case ENOMEM:
        case ENOSPC:
        case EOVERFLOW:
            errcode = QCM_FILE_NO_SPACE;
            break;

        case EROFS:
            errcode = QCM_FILE_DEVICE_READONLY;
            break;
        case ENOTDIR:
            errcode = QCM_FILE_DIR_NOT_EXIST;
            break;

        case ENAMETOOLONG:
            errcode = QCM_FILE_INVALID_FILE_NAME;
            break;

        case EBUSY:
            errcode = QCM_FILE_ALREADY_OPERATION;
            break;
        case EMLINK:
            errcode = QCM_FILE_CREATE_EXCEED;
            break;
        default:
            break;
    }
    return errcode;
}

/**
 * @brief Corresponding to the fopen function in POSIX
 *
 * @param[in] const char * file_name
 *           - The name of the file to open
 *
 * @param[in] const char * mode
 *           - Corresponding attribute information of the opened file
 *
 * @return Q_FILE
 *        - Returns the file open handle
 */
Q_FILE qcm_file_fopen(const char *file_name, const char *mode)
{
    int                  path_size = QCM_FILE_MAX_PATH_LEN;
    char                 file_real_path[QCM_FILE_MAX_PATH_LEN] = {0};
    int                  open_mode = QOSA_VFS_O_RDONLY;
    qcm_file_open_list_t file_info = {0};
    Q_FILE               file_id = {0};
    int                  err = QCM_FILE_OK;

    err = qcm_file_check_path((char *)file_name, file_real_path, &path_size, QOSA_NULL);
    require_action(err, return QCM_FILE_INVALID_INPUT_VALUE, "INVALID file path");

    if (QOSA_NULL != mode && qosa_strlen(mode) > 1 && 'e' == mode[0])
    {
        file_info.is_sfs = QOSA_TRUE;
        open_mode = qcm_file_get_mode((char *)(mode + 1));
    }
    else
    {
        open_mode = qcm_file_get_mode((char *)mode);
    }

    require_action(!(open_mode >= 0), return QCM_FILE_INVALID_INPUT_VALUE, "INVALID open mode");

    err = file_check_is_opened((char *)file_name);
    require_action((err > 0), return QCM_FILE_ALREADY_OPERATION, "file has been opend");

    if (open_mode & QOSA_VFS_O_CREAT)
    {
        qosa_int64_t free_size = qcm_file_free_size(file_name);
        require_action((free_size < 0), return free_size, "get qosa_free size failed");
        require_action((free_size == 0), return QCM_FILE_NO_SPACE, "qosa_free space is not enough");
    }

#ifdef CONFIG_QCM_FILE_SFS_FUNC
    if (file_info.is_sfs == QOSA_TRUE || qcm_file_check_support_sfs(file_real_path, 0))
    {
        file_info.is_sfs = QOSA_TRUE;
        if (QCM_FILE_OPEN_FLAGS_R != open_mode && QCM_FILE_OPEN_FLAGS_W != open_mode)
        {
            QLOGE("sfs err mode:%d", open_mode);
            return QCM_FILE_INVALID_INPUT_VALUE;
        }
    }
#endif /* CONFIG_QCM_FILE_SFS_FUNC */

    file_id = qosa_vfs_open(file_real_path, open_mode);
    if (file_id <= 0)
    {
        QLOGV("file_id = %d", file_id);
        err = file_errocode_get();
        if (err)
        {
            QLOGE("file open failed, errcode %x", err);
            return err;
        }
/** Value specified by the platform interface */
#if defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
        err = (file_id == OSA_LFS_FILE_IS_OPENED_TOO_MUCH ? QCM_FILE_REACH_MAX_OPENFILE_NUM : QCM_FILE_FAILED_TO_OPEN_FILE);
#else /* CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC */
        err = QCM_FILE_FAILED_TO_OPEN_FILE;
#endif /* CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC */
        return err;
    }

    file_info.fd = file_id;
    file_info.file_open_mode = open_mode;
    qosa_strcpy(file_info.file_name, file_name);

    err = file_open_info_push(&file_info);
    if (err != QCM_FILE_OK)
    {
        QLOGE("push file info to list failed");
        qosa_vfs_close(file_id);
        return QCM_FILE_ERROR_GENERAL;
    }
#ifdef CONFIG_QCM_FILE_SFS_FUNC
    if (file_info.is_sfs)
    {
        if (qcm_sfs_file_proc_info_init(file_id, open_mode) != 0)
        {
            QLOGE("sfs proc fail");
            file_open_info_pop(file_id);
            qosa_vfs_close(file_id);
            return QCM_FILE_FAILED_TO_OPEN_FILE;
        }
    }
#endif /* CONFIG_QCM_FILE_SFS_FUNC */

    QLOGV("open file %s, fd %d", file_name, file_id);
    return file_id;
}

/**
 * @brief Open file, corresponding to the open function for file operations
 *
 * @param[in] const char * file_name
 *           - The name of the file to open
 *
 * @param[in] int mode
 *           - To open the file attribute information
 *
 * @param[in] qosa_bool_t open_platform
 *           - Reserved interface, whether it is a file path for OS operations
 *
 * @return int
 *        - Returns the corresponding file handle
 */
int qcm_file_open_ex(const char *file_name, int mode, qosa_bool_t open_platform)
{
    int                  path_size = QCM_FILE_MAX_PATH_LEN;
    char                 file_real_path[QCM_FILE_MAX_PATH_LEN] = {0};
    qcm_file_open_list_t file_info = {0};
    Q_FILE               file_id = 0;
    int                  err = QCM_FILE_OK;

    //The path we opened is ours.
    if (!open_platform)
    {
        err = qcm_file_check_path((char *)file_name, file_real_path, &path_size, QOSA_NULL);
        require_action(err, return QCM_FILE_INVALID_INPUT_VALUE, "INVALID file path");
    }
    else
    {
        qosa_strncpy(file_real_path, file_name, sizeof(file_real_path)-1);
    }

    err = file_check_is_opened((char *)file_name);
    require_action((err > 0), return QCM_FILE_ALREADY_OPERATION, "file has been opend");

    if (mode & QOSA_VFS_O_CREAT)
    {
        qosa_int64_t free_size = qcm_file_free_size(file_name);
        require_action((free_size < 0), return free_size, "get qosa_free size failed");
        require_action((free_size == 0), return QCM_FILE_NO_SPACE, "qosa_free space is not enough");
    }

#ifdef CONFIG_QCM_FILE_SFS_FUNC
    if (qcm_file_check_support_sfs(file_real_path, 0))
    {
        file_info.is_sfs = QOSA_TRUE;
        if (QCM_FILE_OPEN_FLAGS_R != mode && QCM_FILE_OPEN_FLAGS_W != mode)
        {
            QLOGE("sfs err mode:%d", mode);
            return QCM_FILE_INVALID_INPUT_VALUE;
        }
    }
#endif /* CONFIG_QCM_FILE_SFS_FUNC */

    file_id = qosa_vfs_open(file_real_path, mode);
    if (file_id <= 0)
    {
        err = file_errocode_get();
        if (err)
        {
            QLOGE("file open failed, errcode %x", err);
            return err;
        }
        return QCM_FILE_FAILED_TO_OPEN_FILE;
    }

    file_info.fd = file_id;
    file_info.file_open_mode = mode;
    qosa_strncpy(file_info.file_name, file_name, sizeof(file_info.file_name)-1);

    err = file_open_info_push(&file_info);
    if (err != QCM_FILE_OK)
    {
        QLOGE("push file info to list failed");
        qosa_vfs_close(file_id);
        return QCM_FILE_ERROR_GENERAL;
    }

#ifdef CONFIG_QCM_FILE_SFS_FUNC
    if (file_info.is_sfs)
    {
        if (qcm_sfs_file_proc_info_init(file_id, mode) != 0)
        {
            QLOGE("sfs proc fail");
            file_open_info_pop(file_id);
            qosa_vfs_close(file_id);
            return QCM_FILE_FAILED_TO_OPEN_FILE;
        }
    }
#endif /* CONFIG_QCM_FILE_SFS_FUNC */

    QLOGV("open file %s, fd %d", file_name, file_id);
    return file_id;
}

/**
 * @brief Close the opened file handle
 *
 * @param[in] Q_FILE fd
 *          - The handle to be closed
 *
 * @return int
 *        - Returns the execution result of the corresponding file closure operation
 */
int qcm_file_fclose(Q_FILE fd)
{
    qcm_file_error_e err = QCM_FILE_OK;

    require_action((fd <= 0), return QCM_FILE_INVALID_INPUT_VALUE, "INVALID input param");

    err = qcm_file_is_opened_by_fd(fd);
    require_action(err, return QCM_FILE_NOT_OPEN, "file not open");

#ifdef CONFIG_QCM_FILE_SFS_FUNC
    qcm_sfs_file_proc_info_delete(fd);
#endif /* CONFIG_QCM_FILE_SFS_FUNC */

    err = file_open_info_pop(fd);
    require_action(err, return QCM_FILE_CLOSE_FAIL, "close file failed");

    err = qosa_vfs_close(fd);
    require_action(err, return QCM_FILE_CLOSE_FAIL, "close file failed");

    QLOGV("close file fd %d", fd);
    return QCM_FILE_OK;
}

/**
 * @brief Delete the file with the specified file name
 *
 * @param[in] const char * file_name
 *          - File name to be deleted
 *
 * @return int
 *        - Returns QCM_FILE_OK to indicate successful execution
 *        - Returns other values to indicate execution failure
 */
int qcm_file_remove(const char *file_name)
{
    int  ret;
    int  path_size = QCM_FILE_MAX_PATH_LEN;
    char file_real_path[QCM_FILE_MAX_PATH_LEN];

    require_action(!(file_name), return QCM_FILE_INVALID_INPUT_VALUE, "INVALID input param");

    ret = qcm_file_check_path((char *)file_name, file_real_path, &path_size, QOSA_NULL);
    require_action(ret, return QCM_FILE_DELETE_FILE_FAIL, "cannot find the file");

    ret = file_check_is_opened((char *)file_name);
    require_action((ret > 0), return QCM_FILE_ALREADY_OPERATION, "file has been opend");

    ret = qosa_vfs_unlink(file_real_path);
    if (ret != 0)
    {
        int errcode = file_errocode_get();
        if (errcode)
        {
            QLOGE("delete file failed, errcode %x,%s", errcode, file_name);
            return errcode;
        }
        return QCM_FILE_DELETE_FILE_FAIL;
    }

    return ret;
}

/**
 * @brief Read the corresponding file through the opened file descriptor fd.
 *
 * @param[in] void * buffer
 *          - Storage space for read data
 *
 * @param[in] qosa_size_t size
 *          - The size of the file block to read
 *
 * @param[in] qosa_size_t num
 *          - Number of files to read
 *
 * @param[in] Q_FILE fd
 *          - The file descriptor fd to operate on
 *
 * @return int
 *        - Greater than 0 indicates the size of the file that has already been read
 *        - Returns a value less than 0 indicates execution failure
 */
int qcm_file_fread(void *buffer, qosa_size_t size, qosa_size_t num, Q_FILE fd)
{
    int ret;

    require_action(!(buffer && size && num && fd), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");
#ifdef CONFIG_QCM_FILE_SFS_FUNC
    if (qcm_file_check_support_sfs(QOSA_NULL, fd))
    {
        ret = qcm_file_get_mode_by_fd(fd);
        require_action((QCM_FILE_OPEN_FLAGS_R != ret), return QCM_FILE_FAILED_TO_READ_FILE, "sfs not allow");

        ret = qcm_sfs_read(fd, (void *)buffer, size * num);
    }
    else
#endif /* CONFIG_QCM_FILE_SFS_FUNC */
    {
        ret = qosa_vfs_read(fd, (void *)buffer, size * num);
    }

    require_action((ret < 0), return QCM_FILE_FAILED_TO_READ_FILE, "read file failed");
    require_action((ret == 0), return QCM_FILE_READ_ZERO, "read zero");

    return ret;
}

/**
 * @brief Write file content of specified size through an already opened FD handle
 *
 * @param[in] void * buffer
 *          - The storage space to write data to
 *
 * @param[in] qosa_size_t size
 *          - Block size for writing data
 *
 * @param[in] qosa_size_t num
 *          - Number of files to write
 *
 * @param[in] Q_FILE fd
 *          - The file descriptor fd to operate on
 *
 * @return int
 *        - Greater than 0 indicates the size of the file that has already been written
 *        - Returns a value less than 0 indicates execution failure
 */
int qcm_file_fwrite(void *buffer, qosa_size_t size, qosa_size_t num, Q_FILE fd)
{
    int ret;
    //qosa_int64_t free_size;
    //qosa_int64_t temp_size = 0;

    require_action(!(buffer && size && num && fd), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    //free_size = qcm_file_free_size_by_fd(fd);
    //require_action((free_size < 0), return free_size, "get qosa_free size by fd failed");
    //require_action((free_size == 0), return QCM_FILE_NO_SPACE, "qosa_free space is not enough");

    //temp_size = (size * num);
    //require_action((free_size + QCM_FILE_ALIGEN_RESERVE_SIZE < temp_size), return QCM_FILE_TOO_LARGE, "qosa_free space is not enough");
#ifdef CONFIG_QCM_FILE_SFS_FUNC
    if (qcm_file_check_support_sfs(QOSA_NULL, fd))
    {
        ret = qcm_file_get_mode_by_fd(fd);
        require_action((QCM_FILE_OPEN_FLAGS_W != ret), return QCM_FILE_FAILED_TO_WRITE_FILE, "sfs not allow");

        ret = qcm_sfs_write(fd, (const void *)buffer, size * num);
    }
    else
#endif /* CONFIG_QCM_FILE_SFS_FUNC */
    {
        ret = qosa_vfs_write(fd, (const void *)buffer, size * num);
    }
    require_action((ret <= 0), return QCM_FILE_FAILED_TO_WRITE_FILE, "failed to write file");

    return ret;
}

/**
 * @brief Used to set the file offset
 *
 * @param[in] Q_FILE fd
 *           - File descriptor.
 *
 * @param[in] long offset
 *           - Offset, indicating the offset relative to the position pointed to by whence
 *
 * @param[in] int whence
 *           - Starting offset position, which can be one of the following three values:
 *             FILE_SEEK_SET: Offset relative to the beginning of the file.
 *             QCM_FILE_SEEK_CUR: Offset relative to the current position.
 *             QCM_FILE_SEEK_END: Offset relative to the end of the file.
 *
 * @return int
 *        - Success: Returns the new file offset.
 *        - Failure: -1
 */
int qcm_file_fseek(Q_FILE fd, long offset, int whence)
{
    int ret;

    require_action((fd <= 0), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

#ifdef CONFIG_QCM_FILE_SFS_FUNC
    require_action((QOSA_TRUE == qcm_file_check_support_sfs(QOSA_NULL, fd)), return QCM_FILE_INVALID_INPUT_VALUE, "sfs not allow");
#endif /* CONFIG_QCM_FILE_SFS_FUNC */

    ret = qosa_vfs_lseek(fd, offset, whence);
    QLOGV("ret:%d", ret);
    require_action((ret < 0), return QCM_FILE_SEEK_FAIL, "failed to seek file");

    return ret;
}

/**
 * @brief Used to return the current file pointer position
 *
 * @param[in] Q_FILE fd
 *           - File descriptor.
 *
 * @return int
 *        - Greater than 0 indicates returning the current file pointer position
 *        - Less than 0 indicates execution failure
 */
int qcm_file_ftell(Q_FILE fd)
{
    long ret;

    require_action((fd <= 0), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_fseek(fd, 0, QCM_FILE_SEEK_CUR);
    require_action((ret < 0), return ret, "fseek failed");

    return ret;
}

/**
 * @brief Used to obtain file status information
 *
 * The function can retrieve basic file information, such as file type, file permissions, owner, file size, etc.
 * This information is stored in the structure `stat`, and the specific details of the file can be obtained by accessing the members of the structure.
 *
 * @param[in] Q_FILE fd
 *          - File descriptor, used to specify the file for which to obtain status information.
 *
 * @param[in] struct qosa_vfs_stat_t * st
 *          - Pointer to a stat structure, used to store file status information.
 *
 * @return int
 *        - 0: Successfully obtained file status information.
 *        - -1: Failed to retrieve file status information.
 */
int qcm_file_fstat(Q_FILE fd, struct qosa_vfs_stat_t *st)
{
    int ret;

    require_action((fd <= 0 || st == QOSA_NULL), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");
#ifdef CONFIG_QCM_FILE_SFS_FUNC
    if (qcm_file_check_support_sfs(QOSA_NULL, fd))
    {
        ret = qcm_file_get_mode_by_fd(fd);
        require_action((QCM_FILE_OPEN_FLAGS_R != ret), return QCM_FILE_INVALID_INPUT_VALUE, "sfs not allow");

        ret = qcm_sfs_fstat(fd, QOSA_NULL, QOSA_NULL, st);
    }
    else
#endif /* CONFIG_QCM_FILE_SFS_FUNC */
    {
        ret = qosa_vfs_fstat(fd, st);
    }

    require_action(ret, return QCM_FILE_FAILED_TO_GET_STAT, "get state failed");

    return ret;
}

/**
 * @brief Used in operating systems or file systems. It is typically used to retrieve file metadata,
 *   such as file size, creation time, modification time, etc.
 *
 * @param[in] const char * file_name
 *          - File path, used to specify the file for which to retrieve metadata.
 *
 * @param[in] struct qosa_vfs_stat_t * st
 *          - Pointer to a struct stat, used to store file metadata information.
 *
 * @return int
 *       - 0: Successfully obtained file metadata.
 *        -1: Failed to retrieve file metadata.
 */
int qcm_file_stat_func(const char *file_name, struct qosa_vfs_stat_t *st)
{
    int  ret;
    int  path_size = QCM_FILE_MAX_PATH_LEN;
    char file_real_path[QCM_FILE_MAX_PATH_LEN];

    require_action(!(file_name && st), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_check_path((char *)file_name, file_real_path, &path_size, QOSA_NULL);
    require_action(ret, return QCM_FILE_INVALID_INPUT_VALUE, "INVALID file path");
#ifdef CONFIG_QCM_FILE_SFS_FUNC
    if (qcm_file_check_support_sfs(file_real_path, 0))
    {
        ret = qcm_sfs_fstat(-1, (char *)file_name, file_real_path, st);
    }
    else
#endif /* CONFIG_QCM_FILE_SFS_FUNC */
    {
        QLOGV("file_name=%s file_real_path=%s", file_name, file_real_path);
        QOSA_VFS_DIR *dir = qosa_vfs_opendir(file_real_path);
        if (dir != QOSA_NULL)
        {  //is a dir
            if (qosa_vfs_stat(file_real_path, st) == 0)
            {
                //get fat fs path qosa_vfs_stat OK
                ret = 0;
            }
            else
            {
                //is a sffs fs dir,fill the qosa_vfs_stat struct
                st->st_mode = QCM_FILE_DT_DIR | QCM_FILE_O_ACCESSPERMS;
                ret = 0;
            }
        }
        else
        {  //not a dir,maybe a file,try get file qosa_vfs_stat
            int fd = file_check_is_opened((char *)file_name);
            if (fd == -1)
            {
                if (qosa_vfs_stat(file_real_path, st) == 0)
                {
                    ret = 0;
                }
                else
                {
                    ret = QCM_FILE_NOT_EXIST;
                }
            }
            else
            {
                if (qosa_vfs_fstat(fd, st) == 0)
                {
                    ret = 0;
                }
                else
                {
                    ret = QCM_FILE_NOT_EXIST;
                }
            }
        }
        qosa_vfs_closedir(dir);
    }
    QLOGV("ret=%d", ret);
    return ret;
}

/**
 * @brief Get file size information
 *
 * @param[in] int fd
 *          - File descriptor, the file handle to be operated on
 *
 * @return int
 *        - Returns the current size
 *        - Returns -1 on error
 */
int qcm_file_fsize(int fd)
{
    int                    ret;
    struct qosa_vfs_stat_t st;

    require_action((fd <= 0), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_fstat(fd, &st);
    require_action(ret, return ret, "get qosa_vfs_stat failed");

    return st.st_size;
}

/**
 * @brief Determine if the current file exists
 *
 * @param[in] const char * file_name
 *          - File name to check eg: UFS:1.txt
 *
 * @return int
 *         - QCM_FILE_OK indicates that the file already exists
 *         - QCM_FILE_NOT_EXIST indicates that the file does not exist
 */
int qcm_file_is_exist(const char *file_name)
{
    int                    ret;
    int                    path_size = QCM_FILE_MAX_PATH_LEN;
    char                   file_real_path[QCM_FILE_MAX_PATH_LEN];
    struct qosa_vfs_stat_t file_stat;

    require_action((file_name == QOSA_NULL), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_check_path((char *)file_name, file_real_path, &path_size, QOSA_NULL);
    require_action(ret, return QCM_FILE_INVALID_INPUT_VALUE, "INVALID file path");

    ret = qcm_file_stat_func(file_name, &file_stat);
    if (ret != 0)
    {
        return QCM_FILE_NOT_EXIST;  //file is not exit
    }
    else
    {
        return QCM_FILE_OK;  //file is exit
    }
}

/**
 * @brief Move the file position pointer to the beginning of the file.
 *
 * @param[in] Q_FILE fd
 *          - File descriptor, the file handle to be operated on
 *
 * @return int
 *         - Returns 0 to indicate successful execution
 *         - Returns other values indicate execution failure
 */
int qcm_file_frewind(Q_FILE fd)
{
    int ret;

    require_action((fd <= 0), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_fseek(fd, 0, QCM_FILE_SEEK_SET);
    require_action(ret, return ret, "frewind failed");

    return ret;
}

/**
 * @brief The function truncates the file to the specified length.
 *  If the current file length is less than the specified length, the file will be extended.
 *  If the length exceeds the specified length, the file will be truncated.
 *
 * @param[in] Q_FILE fd
 *          - File descriptor, the file handle to be operated on
 *
 * @param[in] long length
 *          - Specify the length of the file to be adjusted
 *
 * @return int
 *         - 0 indicates successful execution
 *         - -1 indicates execution failure
 */
int qcm_file_ftruncate(Q_FILE fd, long length)
{
    int ret;

    require_action((fd <= 0 || length < 0), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");
#ifdef CONFIG_QCM_FILE_SFS_FUNC
    require_action((QOSA_TRUE == qcm_file_check_support_sfs(QOSA_NULL, fd)), return QCM_FILE_INVALID_INPUT_VALUE, "sfs not allow");
#endif /* CONFIG_QCM_FILE_SFS_FUNC */
    ret = qosa_vfs_ftruncate(fd, length);
    if (ret > 0)
    {
        qosa_vfs_fsync(fd);
    }
    require_action(ret, return QCM_FILE_TRUNCATE_FAIL, "truncate failed");

    return ret;
}

/**
 * @brief For creating folder directories
 *
 * @param[in] const char * path
 *            - Path information for creating the folder
 *
 * @param[in] qosa_uint32_t mode
 *            - Permission information for creating folders, default is 0
 *
 * @return int
 *        - 0 indicates successful execution
 *        - Negative numbers indicate execution failure
 */
int qcm_file_mkdir(const char *path, qosa_uint32_t mode)
{
    int  ret;
    int  path_size = QCM_FILE_MAX_PATH_LEN;
    char dir_real_path[QCM_FILE_MAX_PATH_LEN];

    require_action((path == QOSA_NULL), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_check_path((char *)path, dir_real_path, &path_size, QOSA_NULL);
    require_action(ret, return QCM_FILE_INVALID_INPUT_VALUE, "INVALID QOSA_VFS_DIR path");
    ret = qosa_vfs_mkdir(dir_real_path, mode);
    if (ret != QCM_FILE_OK)
    {
#ifdef CONFIG_QOSA_LINUX_PLATFORM_FUNC
        if (errno == EEXIST)  //dir exist
        {
            require_action(ret, return QCM_FILE_DIR_ALREADY_EXIST, "Dir is already exist");
        }
        else
#else
        if (qosa_get_errno() == EEXIST)  //dir exist
        {
            require_action(ret, return QCM_FILE_DIR_ALREADY_EXIST, "Dir is already exist");
        }
        else
#endif
        {
            require_action(ret, return QCM_FILE_DIR_MAKE_FAIL, "make dir failed");
        }
    }

    return ret;
}

/**
 * @brief Open the corresponding folder
 *
 * @param[in] const char * path
 *           - The folder path to operate on
 *
 * @return QCM_FILE_DIR_T *
 *        - Returns the corresponding file directory handle upon success
 *        - Returns OSA_NULL on failure
 */
QCM_FILE_DIR_T *qcm_file_opendir(const char *path)
{
    int             ret;
    QCM_FILE_DIR_T *dir = QOSA_NULL;
    int             path_size = QCM_FILE_MAX_PATH_LEN;
    char            dir_real_path[QCM_FILE_MAX_PATH_LEN];

    require_action((path == QOSA_NULL), return QOSA_NULL, "invalid input");

    ret = qcm_file_check_path((char *)path, dir_real_path, &path_size, QOSA_NULL);
    require_action(ret, return QOSA_NULL, "INVALID QOSA_VFS_DIR path");
    QLOGV("dir_real_path=%s path=%s", dir_real_path, path);

    dir = (QCM_FILE_DIR_T *)qosa_vfs_opendir(dir_real_path);
    require_action((dir == QOSA_NULL), return QOSA_NULL, "open dir failed");

    return dir;
}

/**
 * @brief Close folder
 *
 * @param[in] QCM_FILE_DIR_T * pdir
 *          - Corresponds to the handle opened by opendir
 *
 * @return int
 *        - Returns FILE_OK to indicate successful execution
 */
int qcm_file_closedir(QCM_FILE_DIR_T *pdir)
{
    int ret;

    require_action((pdir == QOSA_NULL), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qosa_vfs_closedir((QOSA_VFS_DIR *)pdir);
    require_action((pdir == QOSA_NULL), return QCM_FILE_DIR_CLOSE_FAIL, "close dir failed");

    return ret;
}

/**
 * @brief Read folder
 *
 * @param[in] QCM_FILE_DIR_T * pdir
 *          - Corresponds to the handle opened by opendir
 *
 * @return qcm_file_dirent_t *
 *       - If the read is successful, return directory information
 *       - If the read operation fails, return OSA_NULL
 */
qcm_file_dirent_t *qcm_file_readdir(QCM_FILE_DIR_T *pdir)
{
    qcm_file_dirent_t *info = QOSA_NULL;

    require_action((!pdir), return QOSA_NULL, "invalid input");

    info = (qcm_file_dirent_t *)qosa_vfs_readdir((QOSA_VFS_DIR *)pdir);
    require_action((info == QOSA_NULL), return QOSA_NULL, "read dir failed");

    return info;
}

/**
 * @brief Get the read position of the directory stream
 *
 * @param[in] QCM_FILE_DIR_T * pdir
 *          - Corresponds to the handle opened by opendir
 *
 * @return int
 *        - This return value represents the offset from the beginning of the directory file.
 *        - Returns -1 when an error occurs
 */
int qcm_file_telldir(QCM_FILE_DIR_T *pdir)
{
    int offset = -1;

    require_action(!pdir, return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    offset = qosa_vfs_telldir((QOSA_VFS_DIR *)pdir);
    require_action((offset < 0), return QCM_FILE_DIR_TELL_FAIL, "tell dir failed");

    return offset;
}

/**
 * @brief Set the position for the next read directory
 *
 * @param[in] QCM_FILE_DIR_T * pdir
 *          - Corresponds to the handle opened by opendir
 *
 * @param[in] int offset
 *          - Represents the offset from the beginning of the directory file
 *
 * @return int
 *        - This return value represents the offset from the beginning of the directory file.
 *        - Returns -1 when an error occurs
 */
int qcm_file_seekdir(QCM_FILE_DIR_T *pdir, int offset)
{
    int ret;
    require_action((!pdir || offset < 0), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    qosa_vfs_seekdir((QOSA_VFS_DIR *)pdir, offset);

    ret = qcm_file_telldir(pdir);
    require_action((!(offset == ret)), return QCM_FILE_DIR_SEEK_FAIL, "seek dir failed");

    return ret;
}

/**
 * @brief Reset the read directory position to the beginning.
 *
 * @param[in] QCM_FILE_DIR_T * pdir
 *          - Corresponds to the handle opened by opendir
 *
 * @return int
 *        - This return value represents the offset from the beginning of the directory file.
 *        - Returns -1 when an error occurs
 */
int qcm_file_rewinddir(QCM_FILE_DIR_T *pdir)
{
    int ret;

    require_action((!pdir), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_seekdir(pdir, 0);
    require_action(ret, return ret, "seek dir failed");

    return ret;
}

/**
 * @brief Delete folder
 *
 * @param[in] const char * path
 *          - Directory for folder deletion operations
 *
 * @return int
 *        - Returns FILE_OK to indicate successful execution
 *        - Returns other values to indicate failure
 */
int qcm_file_rmdir(const char *path)
{
    int  ret;
    int  path_size = QCM_FILE_MAX_PATH_LEN;
    char dir_real_path[QCM_FILE_MAX_PATH_LEN];

    require_action((path == QOSA_NULL), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_check_path((char *)path, dir_real_path, &path_size, QOSA_NULL);
    require_action(ret, return QCM_FILE_INVALID_INPUT_VALUE, "INVALID QOSA_VFS_DIR path");

    ret = qosa_vfs_rmdir(dir_real_path);
    require_action(ret, return QCM_FILE_DIR_REMOVE_FAIL, "remove dir failed");

    return ret;
}

/**
 * @brief Recursively delete a folder, including all subfiles and subfolders within it.
 *
 * @param[in] char * path
 *          - The folder path to operate on
 *
 * @return int
 *        - Returns FILE_OK to indicate successful execution
 *        - Returns other values to indicate failure
 */
int qcm_file_rmdir_ex(char *path)
{
    qosa_uint32_t path_max = 256 + QCM_FILE_FILENAME_MAX_LEN;
    int           ret;
    require_action((path == QOSA_NULL || (qosa_strlen(path) > path_max)), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");
    if (qosa_strncmp(path, QCM_FILE_PRXFIX_FOTA, QCM_FILE_PRXFIX_FOTA_LEN) == 0
#ifdef CONFIG_QCM_FILE_SFS_FUNC
        || qosa_strncmp(path, QCM_FILE_PRXFIX_SFS, QCM_FILE_PRXFIX_SFS_LEN) == 0
#endif /* CONFIG_QCM_FILE_SFS_FUNC */
    )
    {
        return QCM_FILE_INVALID_INPUT_VALUE;
    }
    QCM_FILE_DIR_T *dir = qcm_file_opendir(path);
    if (dir != QOSA_NULL)
    {
        qcm_file_dirent_t *ent;
        char              *tpath = (char *)qosa_malloc(path_max);
        if (tpath == QOSA_NULL)
            return QCM_FILE_ERROR_GENERAL;
        while ((ent = qcm_file_readdir(dir)) != QOSA_NULL)
        {
            qosa_sprintf(tpath, "%s%s%s", path, (path[qosa_strlen(path) - 1] == ':' || path[qosa_strlen(path) - 1] == '/') ? "" : "/", ent->d_name);
            if (ent->d_type == QOSA_VFS_DT_REG)  //current index is file
            {
                QLOGV("delete file : %s", tpath);
                ret = qcm_file_remove(tpath);
                if (ret < 0)
                {
                    QLOGE("file %s delete failed", tpath);
                    qcm_file_closedir(dir);
                    qosa_free(tpath);
                    return QCM_FILE_DELETE_FILE_FAIL;
                }
            }
            else if (ent->d_type == QOSA_VFS_DT_DIR)  //current index is dir
            {
                qcm_file_rmdir_ex(tpath);
            }
            qosa_memset(tpath, 0, path_max);
        }
        ret = qcm_file_closedir(dir);
        qosa_free(tpath);
        if (ret < 0)
        {
            return QCM_FILE_DIR_CLOSE_FAIL;
        }
    }
    if (!(path[qosa_strlen(path) - 1] == ':' || (path[0] == '/' && qosa_strlen(path) == 1)))  //If not the root directory, delete empty directories
    {
        QLOGV("delete dir : %s", path);
        ret = qcm_file_rmdir(path);
        if (ret < 0)
        {
            return QCM_FILE_DIR_REMOVE_FAIL;
        }
    }
    return QCM_FILE_OK;
}

/**
 * @brief Change file name
 *
 * @param[in] const char * old_name
 *          - Name of the historical file to be modified
 *
 * @param[in] const char * new_name
 *          - Change to the new file name
 *
 * @return int
 *       - Returns FILE_OK to indicate successful execution
 *       - Returns other values to indicate execution failure
 */
int qcm_file_rename(const char *old_name, const char *new_name)
{
    int  ret;
    int  path_size = QCM_FILE_MAX_PATH_LEN;
    char old_real_path[QCM_FILE_MAX_PATH_LEN];
    char new_real_path[QCM_FILE_MAX_PATH_LEN];

    require_action((!(old_name && new_name)), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_check_path((char *)old_name, old_real_path, &path_size, QOSA_NULL);
    require_action(ret, return QCM_FILE_INVALID_INPUT_VALUE, "INVALID old path");

    path_size = QCM_FILE_MAX_PATH_LEN;
    ret = qcm_file_check_path((char *)new_name, new_real_path, &path_size, QOSA_NULL);
    require_action(ret, return QCM_FILE_INVALID_INPUT_VALUE, "INVALID new path");

    ret = qosa_vfs_rename(old_real_path, new_real_path);
    require_action(ret, return QCM_FILE_RENAME_FAIL, "rename failed");

    return ret;
}

/**
 * @brief Write a specified character to the file stream
 *
 * @param[in] int chr
 *          - Character to operate on
 *
 * @param[in] int fd
 *          - The file descriptor handle to write to, which is the handle returned by file_open
 *
 * @return int
 *        - Returns FILE_OK to indicate successful execution
 *        - Returns other values to indicate execution failure
 */
int qcm_file_fputc(int chr, int fd)
{
    int ret = 0;

    require_action((fd <= 0), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_fwrite((void *)&chr, 1, 1, fd);
    if (ret == 1)
    {
        ret = 0;
        ret |= (char)(chr & 0xff);
    }
    else
    {
        ret = -1;
    }

    return ret;
}

/**
 * @brief Write a specified string to a file stream
 *
 * @param[in] const char *chr
 *          - The string to be operated on
 *
 * @param[in] int fd
 *          - The file descriptor handle to write to, which is the handle returned by file_open
 *
 * @return int
 *        - Returns FILE_OK to indicate successful execution
 *        - Returns other values to indicate execution failure
 */
int qcm_file_fputs(const char *chr, int fd)
{
    int ret = 0;

    require_action((fd <= 0 || chr == QOSA_NULL), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_fwrite((void *)chr, qosa_strlen(chr), 1, fd);
    return ret;
}

/**
 * @brief Get a character from the file stream
 *
 * @param[in] int fd
 *          - The file handle to operate on, must be a handle opened by file_open
 *
 * @return int
 *        - Returns a value greater than or equal to 0 indicates successful acquisition
 *        - Returns a negative number to indicate execution failure
 */
int qcm_file_fgetc(int fd)
{
    int  ret = 0;
    char c = 0;

    require_action((fd <= 0), return QCM_FILE_INVALID_INPUT_VALUE, "invalid input");

    ret = qcm_file_fread(&c, 1, 1, fd);
    if (ret == 1)
    {
        ret = 0;
        ret |= c;
    }
    else
    {
        ret = -1;
    }
    return ret;
}

/**
 * @brief Get a string of specified character length from the file stream
 *
 * @param[in] char * str
 *          - To obtain the storage pointer address of a string
 *
 * @param[in] int n
 *          - To obtain the length information of a string
 *
 * @param[in] int fd
 *          - File handle to operate on
 *
 * @return char *
 *         - Returns OSA_NULL to indicate execution failure
 *         - Returns other values to indicate successful execution
 */
char *qcm_file_fgets(char *str, int n, int fd)
{
    int   c;
    char *cs = str;

    if (!(str != QOSA_NULL && n > 0 && fd > 0))
    {
        return QOSA_NULL;
    }

    if (n == 1)
    {
        str[0] = '\0';
        return str;
    }

    qosa_memset(str, 0, n);
    while (--n > 0 && (c = qcm_file_fgetc(fd)) != -1)
    {
        if ((*cs++ = c) == '\n')
        {
            break;
        }
    }

    return (c == -1 && cs == str) ? QOSA_NULL : str;
}

int file_ferror(int fd)
{
    int err = 0;

    require_action((fd <= 0), return QCM_FILE_INVALID_INPUT_VALUE, "invalid fd");

    err = qcm_file_is_opened_by_fd(fd);
    require_action(err, return QCM_FILE_INVALID_INPUT_VALUE, "invalid fd");

    return QCM_FILE_OK;
}

/**
 * @brief Write buffer data back to disk, serving the same function as the fsync function in POSIX.
 */
int qcm_file_fsync(int fd)
{
    int err = 0;

    require_action((fd <= 0), return QCM_FILE_INVALID_INPUT_VALUE, "invalid fd");

    err = qosa_vfs_fsync(fd);
    require_action(err, return QCM_FILE_ERROR_GENERAL, "sync file failed");

    return QCM_FILE_OK;
}

/**
 * @brief Formatted string input, same as the posix fscanf function, see man fscanf
 */
int qcm_file_fscanf(int fd, const char *format, ...)
{
    char *read_buff = QOSA_NULL;

    int ret = 0;

    va_list arg_ptr;

    int buf_len = 1024;

    if (!(fd && format))
    {
        return -1;
    }

    read_buff = qosa_malloc(buf_len);
    if (!read_buff)
    {
        return -1;
    }

    ret = qcm_file_fread(read_buff, buf_len, 1, fd);

    if (ret == 0)
    {
        qosa_free(read_buff);
        return -1;
    }
    va_start(arg_ptr, format);
    //ret = vsscanf(read_buff, format, arg_ptr);
    va_end(arg_ptr);

    qosa_free(read_buff);
    return ret;
}

/**
 * @brief Format output data to a file, similar to the posix fprintf function
 */
int qcm_file_fprintf(int fd, const char *format, ...)
{
    int     ret;
    va_list arg_ptr;
    char   *buffer = QOSA_NULL;
    int     buf_len = 2048;

    if (!(fd && format))
        return -1;

    buffer = qosa_malloc(buf_len);
    if (!buffer)
        return -1;

    qosa_memset(buffer, 0, buf_len);

    va_start(arg_ptr, format);
    ret = qosa_vsnprintf(buffer, buf_len, format, arg_ptr);
    va_end(arg_ptr);

    if (ret < 0)
    {
        qosa_free(buffer);
        return -1;
    }

    ret = qcm_file_fwrite(buffer, qosa_strlen(buffer), 1, fd);

    qosa_free(buffer);

    return ret < 0 ? -1 : ret;
}

void qcm_file_init(void)
{
    qosa_q_init(&g_save_open_list);
#ifdef CONFIG_QCM_FILE_SFS_FUNC
    qcm_sfs_file_init();
    qosa_vfs_mkdir(QCM_SFS_FILE_ROOT_DIR, 0);
#endif /* CONFIG_QCM_FILE_SFS_FUNC */
#ifndef CONFIG_QOSA_QT2131_PLATFORM_FUNC
    qosa_vfs_mkdir(QCM_USER_DIR_ROOT, 0);
    qosa_vfs_mkdir(QCM_OPEN_FOTA_ROOT_DIR, 0);
    qosa_vfs_mkdir(QCM_DATA_FILE_ROOT_DIR, 0);
#endif
}
