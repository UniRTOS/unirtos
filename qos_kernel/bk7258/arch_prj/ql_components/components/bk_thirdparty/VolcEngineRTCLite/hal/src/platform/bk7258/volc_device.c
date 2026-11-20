#include <common/bk_include.h>

#include "volc_device.h"

#include <stdio.h>

#include "volc_errno.h"

uint32_t volc_get_platform_name(char* platform, uint32_t len) {
    int32_t res = 0;
    if (NULL == platform) {
        return VOLC_FAILED;
    }

    res = snprintf(platform, len , "%s", CONFIG_SOC_STR);
    if (res >= len) {
        return VOLC_FAILED;
    }
    return VOLC_SUCCESS;
}

uint32_t volc_get_os_version(char* os, uint32_t len) {
    int32_t res = 0;

    if (NULL == os) {
        return VOLC_FAILED;
    }

    res = snprintf(os, len, "%s", "FreeRTOS/V10.5.1");
    if (res >= len) {
        return VOLC_FAILED;
    }
    return VOLC_SUCCESS;
}

uint32_t volc_get_compiler_info(char* info, uint32_t len) {
    int32_t res = VOLC_SUCCESS;

    if (NULL == info) {
        return VOLC_FAILED;
    }

#if defined(__clang__)
    snprintf(info, len, "%s/%d.%d.%d", "Clang", (int)__clang_major__, (int)__clang_minor__, (int)__clang_patchlevel__);
#elif defined(__GNUC__)
    snprintf(info, len, "%s/%d.%d.%d", "GCC", (int)__GNUC__, (int)__GNUC_MINOR__, (int)__GNUC_PATCHLEVEL__);
#endif
    return res;
}