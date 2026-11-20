#pragma once

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_error.h"





#define AVDK_RETURN_ON_FALSE(cond, err_code, tag, format, ...)                                                          \
    do {                                                                                                                \
        if (unlikely(!(cond))) {                                                                                        \
            BK_LOGE((char *)tag, "%s error[%d]: " format "\n", __FUNCTION__, __LINE__ __VA_OPT__(,) __VA_ARGS__);       \
            return err_code;                                                                                            \
        }                                                                                                               \
    } while (0)

#define AVDK_RETURN_ON_ERROR(func, tag, format, ...)                                                                    \
    do {                                                                                                                \
        avdk_err_t err_code = (func);                                                                                   \
        if (unlikely(err_code != AVDK_ERR_OK)) {                                                                        \
            BK_LOGE((char *)tag, "%s line: %d, " format "\n", __FUNCTION__, __LINE__ __VA_OPT__(,) __VA_ARGS__);        \
            return err_code;                                                                                            \
        }                                                                                                               \
    } while (0)

#define AVDK_GOTO_ON_FALSE(cond, err_code, goto_err, tag, format, ...) do {                                             \
        if (unlikely(!(cond))) {                                                                                        \
            BK_LOGE((char *)tag, "%s line: %d, " format, __FUNCTION__, __LINE__ __VA_OPT__(,) __VA_ARGS__);             \
            ret = err_code;                                                                                             \
            goto goto_err;                                                                                              \
        }                                                                                                               \
    } while (0)

#define AVDK_GOTO_ON_ERROR(func, goto_err, tag, format, ...) do {                                                       \
        bk_err_t err_rc_ = (func);                                                                                      \
        if (unlikely(err_rc_ != BK_OK)) {                                                                               \
            ret = err_rc_;                                                                                              \
            BK_LOGE((char *)tag, "%s line: %d, " format, __FUNCTION__, __LINE__ __VA_OPT__(,) __VA_ARGS__);             \
            goto goto_err;                                                                                              \
        }                                                                                                               \
    } while(0)

#define AVDK_RETURN_VOID_ON_FALSE(cond, tag, format, ...)                                                               \
    do {                                                                                                                \
        if (unlikely(!(cond))) {                                                                                        \
            BK_LOGE((char *)tag, "%s line: %d, " format "\n", __FUNCTION__, __LINE__ __VA_OPT__(,) __VA_ARGS__);        \
            return;                                                                                                     \
        }                                                                                                               \
    } while (0)

#define AVDK_RETURN_VOID_ON_ERROR(func, tag, format, ...)                                                               \
    do {                                                                                                                \
        avdk_err_t err_code = (func);                                                                                   \
        if (unlikely(err_code != AVDK_ERR_OK)) {                                                                        \
            BK_LOGE((char *)tag, "%s line: %d, " format "\n", __FUNCTION__, __LINE__ __VA_OPT__(,) __VA_ARGS__);        \
            return;                                                                                                     \
        }                                                                                                               \
    } while (0)

#define AVDK_GOTO_VOID_ON_FALSE(cond, goto_err, tag, format, ...) do {                                                 \
        if (unlikely(!(cond))) {                                                                                        \
            BK_LOGE((char *)tag, "%s line: %d, " format, __FUNCTION__, __LINE__ __VA_OPT__(,) __VA_ARGS__);             \
            goto goto_err;                                                                                              \
        }                                                                                                               \
    } while (0)

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */