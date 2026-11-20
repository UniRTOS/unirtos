#include <common/bk_include.h>
#include "volc_directory.h"
#include <sys/stat.h>
// #include <dirent.h> // #error "<dirent.h> not supported"
#include <sys/errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bk_posix.h"
#include "volc_errno.h"
#include "volc_type.h"
#include "volc_memory.h"
#include "volc_fileio.h"

static uint32_t _volc_remove_file_dir(uint64_t data, volc_dir_entry_type_e type, char* path, char* name) {
    VOLC_UNUSED_PARAM(data);
    VOLC_UNUSED_PARAM(name);
    uint32_t ret = VOLC_STATUS_SUCCESS;

    switch (type) {
        case VOLC_DIR_ENTRY_TYPE_DIRECTORY:
            VOLC_CHK(0 == rmdir(path), VOLC_STATUS_REMOVE_DIRECTORY_FAILED);
            break;
        case VOLC_DIR_ENTRY_TYPE_FILE:
            VOLC_CHK(0 == remove(path), VOLC_STATUS_REMOVE_FILE_FAILED);
            break;
        case VOLC_DIR_ENTRY_TYPE_LINK:
            VOLC_CHK(0 == unlink(path), VOLC_STATUS_REMOVE_LINK_FAILED);
            break;
        default:
            VOLC_CHK(0, VOLC_STATUS_UNKNOWN_DIR_ENTRY_TYPE);
    }

err_out_label:
    return ret;
}

static uint32_t _volc_get_file_dir_size(uint64_t data, volc_dir_entry_type_e type, char* path, char* name) {
    VOLC_UNUSED_PARAM(name);
    uint64_t size = 0;
    uint64_t* p_size = (uint64_t*)(uint32_t)data;
    uint32_t ret = VOLC_STATUS_SUCCESS;

    switch (type) {
        case VOLC_DIR_ENTRY_TYPE_DIRECTORY:
            break;
        case VOLC_DIR_ENTRY_TYPE_FILE:
            VOLC_CHK_STATUS(volc_file_get_length(path, &size));
            break;
        case VOLC_DIR_ENTRY_TYPE_LINK:
            break;
        default:
            VOLC_CHK(0, VOLC_STATUS_UNKNOWN_DIR_ENTRY_TYPE);
    }

    *p_size += size;

err_out_label:
    return ret;
}

uint32_t volc_traverse_directory(const char* path, uint64_t data, bool iterate, volc_directory_entry_callback entry) {
    uint32_t ret = VOLC_SUCCESS;
    char temp_file_name[VOLC_MAX_PATH_LEN];
    uint32_t path_len = 0;

    uint32_t dir_path_len;
    DIR* dir = NULL;
    struct dirent* dir_ent = NULL;
    struct stat entry_stat;

    VOLC_CHK(path != NULL && entry != NULL && path[0] != '\0', VOLC_STATUS_INVALID_ARG);

    // Ensure we don't get a very long paths. Need at least a separator and a null terminator.
    path_len = (uint32_t) strlen(path);
    VOLC_CHK(path_len + 2 < VOLC_MAX_PATH_LEN, VOLC_STATUS_PATH_TOO_LONG);

    // Ensure the path is appended with the separator
    strcpy(temp_file_name, path);

    if (temp_file_name[path_len - 1] != VOLC_FPATHSEPARATOR) {
        temp_file_name[path_len] = VOLC_FPATHSEPARATOR;
        path_len++;
        temp_file_name[path_len] = '\0';
    }

    dir = opendir(temp_file_name);

    // Need to make a distinction between various types of failures
    if (dir == NULL) {
        switch (errno) {
            case EACCES:
                VOLC_CHK(0, VOLC_STATUS_DIRECTORY_ACCESS_DENIED);
            case ENOENT:
                VOLC_CHK(0, VOLC_STATUS_DIRECTORY_MISSING_PATH);
            default:
                VOLC_CHK(0, VOLC_STATUS_DIRECTORY_OPEN_FAILED);
        }
    }

    while (NULL != (dir_ent = readdir(dir))) {
        if ((0 == strcmp(dir_ent->d_name, ".")) || (0 == strcmp(dir_ent->d_name, ".."))) {
            continue;
        }

        // Prepare the path
        temp_file_name[path_len] = '\0';

        // Check if it's a directory, link, file or unknown
        strncat(temp_file_name, dir_ent->d_name, VOLC_MAX_PATH_LEN - path_len);
        VOLC_CHK(0 == stat(temp_file_name, &entry_stat), VOLC_STATUS_DIRECTORY_ENTRY_STAT_ERROR);

        if (S_ISREG(entry_stat.st_mode)) {
            VOLC_CHK_STATUS(entry(data, VOLC_DIR_ENTRY_TYPE_FILE, temp_file_name, dir_ent->d_name));
        } else if (S_ISLNK(entry_stat.st_mode)) {
            VOLC_CHK_STATUS(entry(data, VOLC_DIR_ENTRY_TYPE_LINK, temp_file_name, dir_ent->d_name));
        } else if (S_ISDIR(entry_stat.st_mode)) {
            // Append the path separator and null terminate
            dir_path_len = strlen(temp_file_name);
            VOLC_CHK(dir_path_len + 2 < VOLC_MAX_PATH_LEN, VOLC_STATUS_PATH_TOO_LONG);

            // Iterate into sub-directories if specified
            if (iterate) {
                temp_file_name[dir_path_len] = VOLC_FPATHSEPARATOR;
                temp_file_name[dir_path_len + 1] = '\0';

                // Recurse into the directory
                VOLC_CHK_STATUS(volc_traverse_directory(temp_file_name, data, iterate, entry));
            }

            // Remove the path separator
            temp_file_name[dir_path_len] = '\0';
            VOLC_CHK_STATUS(entry(data, VOLC_DIR_ENTRY_TYPE_DIRECTORY, temp_file_name, dir_ent->d_name));
        } else {
            // We treat this as unknown
            VOLC_CHK_STATUS(entry(data, VOLC_DIR_ENTRY_TYPE_UNKNOWN, temp_file_name, dir_ent->d_name));
        }
    }

err_out_label:

    if (dir != NULL) {
        closedir(dir);
        dir = NULL;
    }
    return ret;
}

uint32_t volc_remove_directory(const char* path) {
    uint32_t ret = VOLC_STATUS_SUCCESS;

    VOLC_CHK(path != NULL && path[0] != '\0', VOLC_STATUS_INVALID_ARG);

    VOLC_CHK_STATUS(volc_traverse_directory(path, 0, 1, _volc_remove_file_dir));

    // Finally, remove the directory when it's empty
    VOLC_CHK(0 == rmdir(path), VOLC_STATUS_REMOVE_DIRECTORY_FAILED);

err_out_label:
    return ret;
}

uint32_t volc_create_directory(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
        {
            return 0;
        } 
        else
        {
            return -1;
        }
    }
    else
    {
        char* p = strrchr(path,'/');
        if(p == NULL) {
            return -1;
        }

        size_t len = p - path;
        char* parant_path = volc_calloc(1,len + 1);
        strncpy(parant_path,path,len);
        volc_create_directory(parant_path);
        VOLC_SAFE_MEMFREE(parant_path);
        if (mkdir(path, 0777) == -1) {
            return -1;
        }
        return 0;
    }
}

uint32_t volc_get_directory_size(const char* path, uint64_t* p_size) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    uint64_t size = 0;

    VOLC_CHK(p_size != NULL && path != NULL && path[0] != '\0', VOLC_STATUS_INVALID_ARG);

    VOLC_CHK_STATUS(volc_traverse_directory(path, (uint64_t)(uint32_t) &size, 1, _volc_get_file_dir_size));

    *p_size = size;

err_out_label:
    return ret;
}