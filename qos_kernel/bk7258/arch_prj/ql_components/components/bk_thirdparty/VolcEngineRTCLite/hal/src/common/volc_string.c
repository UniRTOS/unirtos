#include "volc_string.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "volc_errno.h"
#include "volc_type.h"
#include "volc_memory.h"

char* volc_string_get(volc_string_t* str) {
    if (NULL == str) {
        return NULL;
    }
    return str->buffer;
}

char* volc_string_strstr(volc_string_t* str, char* src) {
    if (NULL == str || NULL == str->buffer || NULL == src) {
        return NULL;
    }
    return strstr(str->buffer, src);
}

uint32_t volc_string_init(volc_string_t* str) {
    if (NULL == str) {
        return VOLC_FAILED;
    }
    memset(str, 0, sizeof(volc_string_t));
    return VOLC_SUCCESS;
}

uint32_t volc_string_deinit(volc_string_t* str) {
    if (NULL == str) {
        return VOLC_FAILED;
    }
    str->length = 0;
    str->capacity = 0;
    volc_free(str->buffer);
    str->buffer = NULL;
    return VOLC_SUCCESS;
}

uint32_t volc_string_set_with_length(volc_string_t* str,uint8_t * src,int32_t length) {
    if (NULL == str || NULL == src || length <= 0) {
        return VOLC_FAILED;
    }
    if (length >= str->capacity) {
        str->buffer = (char *)volc_realloc(str->buffer, length + 1);
        if (NULL == str->buffer) {
            return VOLC_FAILED;
        }
        str->capacity = length + 1;
    }
    memcpy(str->buffer, src, length);
    str->length = length;
    str->buffer[length] = 0;

    return VOLC_SUCCESS;
}

uint32_t volc_string_set(volc_string_t* str,char* src) {
    if (NULL == str || NULL == src) {
        return VOLC_FAILED;
    }
    volc_string_set_with_length(str, (uint8_t *)src, strlen(src));
    return VOLC_SUCCESS;
}

uint32_t volc_string2set(volc_string_t* str,volc_string_t* str2) {
    if (NULL == str || NULL == str2) {
        return VOLC_FAILED;
    }
    return volc_string_set_with_length(str, (uint8_t *)volc_string_get(str2), volc_string_get_length(str2));
}

uint32_t volc_string_append_with_length(volc_string_t* str, char* src,int32_t length) {
    if (NULL == str || NULL == src) {
        return VOLC_FAILED;
    }
    if (str->length + length >= str->capacity) {
        str->buffer = (char *)volc_realloc(str->buffer, str->length + length + 1);
        if (NULL == str->buffer) {
            return VOLC_FAILED;
        }
        str->capacity = str->length + length + 1;
    }

    strncpy(str->buffer + str->length, src, length);
    str->length += length;
    str->buffer[str->length] = 0;
    return VOLC_SUCCESS;
}

uint32_t volc_string_append(volc_string_t * str,char* src) {
    if (NULL == str || NULL == src) {
        return VOLC_FAILED;
    }
    return volc_string_append_with_length(str, src, strlen(src));
}

uint32_t volc_string_snprintf(volc_string_t * str, int32_t length, const char * format, ...) {
    if (NULL == str) {
        return VOLC_FAILED;
    }
    va_list arg;
    va_start(arg, format);
    int32_t len = vsnprintf(str->buffer, 0, format, arg);
    va_end(arg);

    len = VOLC_MIN(len, length);

    if (len >= str->capacity) {
        str->buffer = (char *)volc_realloc(str->buffer, len + 1);
        if (NULL == str->buffer) {
            return VOLC_FAILED;
        }
        str->capacity = len + 1;
    }
    va_start(arg, format);
    vsnprintf(str->buffer, len + 1, format, arg);
    va_end(arg);

    if (len > length) {
        return VOLC_FAILED;
    }
    str->length = len;
    str->buffer[str->length] = 0;

    return VOLC_SUCCESS;
}

uint32_t volc_string_reset(volc_string_t*str) {
    if (NULL == str) {
        return VOLC_FAILED;
    }
    memset(str->buffer, 0, str->length);
    return VOLC_SUCCESS;
}

uint32_t volc_string_generate_json_safe_string(volc_string_t* str,int len) {
    if (NULL == str) {
        return VOLC_FAILED;
    }
    if (len > str->capacity) {
        str->buffer = (char *)volc_realloc(str->buffer, len + 1);
        if (NULL == str->buffer) {
            return VOLC_FAILED;
        }
        str->capacity = len + 1;
    }

    for (int i = 0; i < len; i++) {
        str->buffer[i] = VOLC_VALID_CHAR_SET_FOR_JSON[rand() % (VOLC_ARRAY_SIZE(VOLC_VALID_CHAR_SET_FOR_JSON) - 1)];
    }
    str->length = len;
    str->buffer[len] = 0;
    return VOLC_SUCCESS;
}

uint32_t volc_string_get_length(volc_string_t * str) {
    if (NULL == str || NULL == str->buffer || str->length <= 0) {
        return 0;
    }
    return (uint32_t)str->length;
}

int32_t volc_string_cmp(volc_string_t * str,const char * dst) {
    return strcmp(volc_string_get(str), dst);
}

int32_t volc_string2cmp(volc_string_t * str,volc_string_t * str2) {
    if (volc_string_get_length(str) != volc_string_get_length(str2)) {
        return -1;
    }
    return memcmp(volc_string_get(str), volc_string_get(str2), volc_string_get_length(str));
}

int32_t volc_string_cmp_with_length(volc_string_t * str,const char * dst,int len) {
    return strncmp(volc_string_get(str), dst, len);
}