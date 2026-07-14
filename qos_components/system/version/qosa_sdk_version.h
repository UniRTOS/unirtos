#ifndef __QOSA_SDK_VERSION_H__
#define __QOSA_SDK_VERSION_H__

#include "qosa_def.h"
#include "qosa_sdk_version_gen.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Manual fallback version when neither project version.txt nor git metadata is available. */
#define QOSA_SDK_DEFAULT_VERSION    1
#define QOSA_SDK_DEFAULT_SUBVERSION 0
#define QOSA_SDK_DEFAULT_REVISION   3

/* Resolved compile-time version macros generated during build. */
#define QOSA_SDK_VERSION    QOSA_SDK_RESOLVED_VERSION_MAJOR
#define QOSA_SDK_SUBVERSION QOSA_SDK_RESOLVED_VERSION_MINOR
#define QOSA_SDK_REVISION   QOSA_SDK_RESOLVED_VERSION_PATCH

#define QOSA_SDK_VERSION_MAJOR QOSA_SDK_VERSION
#define QOSA_SDK_VERSION_MINOR QOSA_SDK_SUBVERSION
#define QOSA_SDK_VERSION_PATCH QOSA_SDK_REVISION

#ifdef QL_UNIRTOS_SUPPORT
QOSA_EXTERN const char *qosa_sdk_get_version(void);
QOSA_EXTERN const char *qosa_sdk_get_version_detail(void);
#else
QOSA_INLINE const char *qosa_sdk_get_version(void)
{
    return QOSA_SDK_VERSION_TAG;
}

QOSA_INLINE const char *qosa_sdk_get_version_detail(void)
{
    return QOSA_SDK_VERSION_DETAIL;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __QOSA_SDK_VERSION_H__ */
