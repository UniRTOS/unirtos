#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    beken_timer_t timer;        /**< freertos timer */
    uint32_t timer_interval;    /**< timer interval(ms) */
    uint32_t data_size;         /**< total data size(bytes) */
    char tag[20];               /**< printf tag */
} count_util_t;

/**
 * @brief      Multi-parameter item structure for count util
 */
typedef struct
{
    char tag[20];               /**< parameter tag */
    uint32_t data_size;         /**< total data size(bytes) */
} count_util_param_t;

/**
 * @brief      Multi-parameter count util structure
 */
typedef struct
{
    beken_timer_t timer;        /**< freertos timer */
    uint32_t timer_interval;    /**< timer interval(ms) */
    count_util_param_t *params; /**< parameters array */
    uint32_t param_count;       /**< number of parameters */
} count_util_multi_t;

/**
 * @brief      Destroy a count util
 *
 * @param[in]      count_util  The count util handle
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t count_util_destroy(count_util_t *count_util);

/**
 * @brief      Create a count util
 *
 * @param[in]      count_util  The count util handle
 * @param[in]      interval    The timer interval(ms)
 * @param[in]      tag         The printf tag
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t count_util_create(count_util_t *count_util, uint32_t interval, char *tag);

/**
 * @brief      Add data size to count util
 *
 * @param[in]      count_util  The count util handle
 * @param[in]      size        The data size(bytes)
 *
 * @return         None
 */
void count_util_add_size(count_util_t *count_util, int32_t size);

/**
 * @brief      Destroy a multi-parameter count util
 *
 * @param[in]      count_util  The multi-parameter count util handle
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t count_util_multi_destroy(count_util_multi_t *count_util);

/**
 * @brief      Create a multi-parameter count util
 *
 * @param[in]      count_util   The multi-parameter count util handle
 * @param[in]      interval     The timer interval(ms)
 * @param[in]      tags         The array of parameter tags
 * @param[in]      param_count  The number of parameters
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 *
 * @note           The tags array should contain param_count strings
 */
bk_err_t count_util_multi_create(count_util_multi_t *count_util, uint32_t interval, char **tags, uint32_t param_count);

/**
 * @brief      Add data size to specific parameter in multi-parameter count util
 *
 * @param[in]      count_util   The multi-parameter count util handle
 * @param[in]      param_index  The parameter index (0 to param_count-1)
 * @param[in]      size         The data size(bytes)
 *
 * @return         None
 */
void count_util_multi_add_size(count_util_multi_t *count_util, uint32_t param_index, int32_t size);

#ifdef __cplusplus
}
#endif

