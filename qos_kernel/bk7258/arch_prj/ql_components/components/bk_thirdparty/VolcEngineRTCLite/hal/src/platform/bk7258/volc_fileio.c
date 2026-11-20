#include <common/bk_include.h>
#include "volc_fileio.h"
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "bk_posix.h"
#include "volc_type.h"

uint32_t volc_file_read(const char* path, bool bin_mode, uint8_t* buffer, uint64_t* size) {
    uint64_t file_len = 0;
    uint32_t ret = VOLC_STATUS_SUCCESS;
    int fd = -1;
    struct stat statbuf = {0};

    VOLC_CHK(path != NULL && size != NULL, VOLC_STATUS_NULL_ARG);

    // Get the size of the file
    ret = stat(path, &statbuf);
    VOLC_CHK(ret == 0, VOLC_STATUS_NOT_FOUND);
    
    file_len = statbuf.st_size; 

    if (buffer == NULL) {
        // requested the length - set and early return
        *size = file_len;
        VOLC_CHK(0, VOLC_STATUS_SUCCESS);
    }

    fd = open(path, O_RDONLY);

    VOLC_CHK(fd != -1, VOLC_STATUS_OPEN_FILE_FAILED);

    // Validate the buffer size
    VOLC_CHK(file_len <= *size, VOLC_STATUS_BUFFER_TOO_SMALL);

    // Read the file into memory buffer
    VOLC_CHK((read(fd, buffer, (size_t) file_len) == file_len), VOLC_STATUS_READ_FILE_FAILED);

err_out_label:
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    return ret;
}

uint32_t volc_file_read_segment(const char* path, bool bin_mode, uint8_t* buffer, uint64_t offset, uint64_t size) {
    uint64_t file_len = 0;
    uint32_t ret = VOLC_STATUS_SUCCESS;
    int fd = -1;
    struct stat statbuf = {0};
    int32_t result = 0;

    VOLC_CHK(path != NULL && buffer != NULL && size != 0, VOLC_STATUS_NULL_ARG);

    // Get the size of the file
    ret = stat(path, &statbuf);
    VOLC_CHK(ret == 0, VOLC_STATUS_NOT_FOUND);
    
    file_len = statbuf.st_size; 

    fd = open(path, O_RDONLY);

    VOLC_CHK(fd != -1, VOLC_STATUS_OPEN_FILE_FAILED);

    // Check if we are trying to read past the end of the file
    VOLC_CHK(offset + size <= file_len, VOLC_STATUS_READ_FILE_FAILED);

    // Set the offset and read the file content
    result = lseek(fd, (uint32_t) offset, SEEK_SET);
    VOLC_CHK(result == offset && (read(fd, buffer, (size_t) size) == size), VOLC_STATUS_READ_FILE_FAILED);

err_out_label:

    if (fd != -1) {
        close(fd);
        fd = -1;
    }

    return ret;
}

uint32_t volc_file_write(const char* path, bool bin_mode, bool append, uint8_t* buffer, uint64_t size) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    int fd = -1;

    VOLC_CHK(path != NULL && buffer != NULL, VOLC_STATUS_NULL_ARG);

    fd = open(path, append ? O_APPEND : O_WRONLY);

    VOLC_CHK(fd != -1, VOLC_STATUS_OPEN_FILE_FAILED);

    // Write the buffer to the file
    VOLC_CHK(write(fd, buffer, (size_t) size) == size, VOLC_STATUS_WRITE_TO_FILE_FAILED);

err_out_label:
    if (fd != -1) {
        close(fd);
        fd = -1;
    }

    return ret;
}

uint32_t volc_file_update(const char* path, bool bin_mode, uint8_t* buffer, uint64_t offset, uint64_t size) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    int fd = -1;

    VOLC_CHK(path != NULL && buffer != NULL, VOLC_STATUS_NULL_ARG);

    fd = open(path, O_RDWR);

    VOLC_CHK(fd != -1, VOLC_STATUS_OPEN_FILE_FAILED);

    VOLC_CHK(offset == lseek(fd, (uint32_t) offset, SEEK_SET), VOLC_STATUS_INVALID_OPERATION);

    // Write the buffer to the file
    VOLC_CHK(write(fd, buffer + offset, (size_t) size) == size, VOLC_STATUS_WRITE_TO_FILE_FAILED);

err_out_label:

    if (fd != -1) {
        close(fd);
        fd = -1;
    }

    return ret;
}

uint32_t volc_file_get_length(const char* path, uint64_t* p_length) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    VOLC_CHK_STATUS(volc_file_read(path, 1, NULL, p_length));
err_out_label:

    return ret;
}

uint32_t volc_file_set_length(const char* path, uint64_t length) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    int fd = -1;
    int32_t ret_val = -1, err_code = 0;

    VOLC_CHK(path != NULL, VOLC_STATUS_NULL_ARG);

    fd = open(path, O_RDWR);
    VOLC_CHK(fd != -1, VOLC_STATUS_OPEN_FILE_FAILED);

    ret_val == ftruncate(fd, (uint32_t)(length));

    err_code = errno;

    if (ret_val == -1) {
        switch (err_code) {
            case EACCES:
                ret = VOLC_STATUS_DIRECTORY_ACCESS_DENIED;
                break;

            case ENOENT:
                ret = VOLC_STATUS_DIRECTORY_MISSING_PATH;
                break;

            case EINVAL:
                ret = VOLC_STATUS_INVALID_ARG_LEN;
                break;

            case EISDIR:
            case EBADF:
                ret = VOLC_STATUS_INVALID_ARG;
                break;

            case ENOSPC:
                ret = VOLC_STATUS_NOT_ENOUGH_MEMORY;
                break;

            default:
                ret = VOLC_STATUS_INVALID_OPERATION;
        }
    }

err_out_label:

    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    return ret;
}

uint32_t volc_file_exists(const char* path, bool* p_exists) {
    if (path == NULL || p_exists == NULL) {
        return VOLC_STATUS_NULL_ARG;
    }

    struct stat st;
    int32_t result = stat(path, &st);
    *p_exists = (result == 0);

    return VOLC_STATUS_SUCCESS;
}

uint32_t volc_file_create(const char* path, uint64_t size) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    int fd = -1;

    VOLC_CHK(path != NULL, VOLC_STATUS_NULL_ARG);

    fd = open(path, O_RDWR | O_CREAT);
    VOLC_CHK(fd != -1, VOLC_STATUS_OPEN_FILE_FAILED);

    if (size != 0) {
        VOLC_CHK(size == ftruncate(fd, (uint32_t)(size)), VOLC_STATUS_INVALID_OPERATION);
    }

err_out_label:

    if (fd != -1) {
        close(fd);
        fd = -1;
    }

    return ret;
}

uint32_t volc_file_delete(const char* path) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    ret = unlink(path);

// err_out_label:
    return ret;
}