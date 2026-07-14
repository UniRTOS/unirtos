#include "qosa_sdk_version.h"

#ifdef QL_UNIRTOS_SUPPORT
const char *qosa_sdk_get_version(void)
{
    return QOSA_SDK_VERSION_TAG;
}

const char *qosa_sdk_get_version_detail(void)
{
    return QOSA_SDK_VERSION_DETAIL;
}
#endif
