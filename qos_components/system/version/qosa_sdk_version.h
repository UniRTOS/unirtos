#ifndef __QOSA_SDK_VERSION_H__
#define __QOSA_SDK_VERSION_H__

#include "qosa_def.h"
#include "qosa_sdk_version_gen.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Manual fallback version when neither project version.txt nor git metadata is available. */
#define QOSA_SDK_DEFAULT_VERSION_MAJOR 1
#define QOSA_SDK_DEFAULT_VERSION_MINOR 0
#define QOSA_SDK_DEFAULT_VERSION_PATCH 3

/* Resolved compile-time version macros generated during build. */
#define QOSA_SDK_VERSION_MAJOR QOSA_SDK_RESOLVED_VERSION_MAJOR
#define QOSA_SDK_VERSION_MINOR QOSA_SDK_RESOLVED_VERSION_MINOR
#define QOSA_SDK_VERSION_PATCH QOSA_SDK_RESOLVED_VERSION_PATCH

QOSA_INLINE const char *qosa_sdk_get_version(void)
{
    return QOSA_SDK_VERSION_TAG;
}

QOSA_INLINE const char *qosa_sdk_get_version_detail(void)
{
    return QOSA_SDK_VERSION_DETAIL;
}

#ifdef __cplusplus
}
#endif

#endif /* __QOSA_SDK_VERSION_H__ */
