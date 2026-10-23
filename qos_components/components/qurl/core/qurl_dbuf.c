/** @file         qurl_dbuf.c
 *  @brief        Brief description: dbuf, which stands for dynamic data packet, dynamic\data
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-10-11 16:08:45
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qurl_config.h"
#include "qurl_default_basic.h"
#include "qurl_dbuf.h"
#include "qurl_log.h"

#define DBUF_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief Create a new dbuf data node
 *
 * @param node_ptr
 * @param size
 * @return qurl_ecode_t
 */
static inline qurl_ecode_t dbuf_node_new(qurl_dbuf_node_t *node_ptr, long size)
{
    unsigned char *ptr = NULL;

    ptr = (unsigned char *)qurl_malloc(size + 1);
    if (ptr == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }
    ptr[size] = 0x00;

    node_ptr->total_size = size;
    node_ptr->valid_size = 0;
    node_ptr->w_index = 0;
    node_ptr->r_index = 0;
    node_ptr->buf = ptr;

    qurl_list_node_init(&(node_ptr->node));

    return QURL_OK;
}

/**
 * @brief Delete dbuf data node
 *
 * @param node_ptr
 */
static inline void dbuf_node_delete(qurl_dbuf_node_t *node_ptr)
{
    if (node_ptr->buf != QOSA_NULL)
    {
        qurl_free(node_ptr->buf);
    }
    qurl_memset(node_ptr, 0x00, sizeof(qurl_dbuf_node_t)); /*Leave no trace*/
}

/**
 * @brief Write data into the dbuf data node
 *
 * @param node_ptr
 * @param data
 * @param len
 */
static inline void dbuf_node_write(qurl_dbuf_node_t *node_ptr, void *data, long len)
{
    long w_index = node_ptr->w_index;
    long total_size = node_ptr->total_size;
    long first_w_size = 0;

    if (node_ptr->valid_size + len > total_size) /*Insufficient space, bro.*/
    {
        /*Log markers can be placed here.*/
        return;
    }

    if (len + w_index <= total_size) /*No need for segmented writing*/
    {
        qurl_memcpy(node_ptr->buf + w_index, data, len);
    }
    else /*It needs to be written in segments because it has wrapped around.*/
    {
        first_w_size = total_size - w_index;
        qurl_memcpy(node_ptr->buf + w_index, data, first_w_size);
        qurl_memcpy(node_ptr->buf, (unsigned char *)data + first_w_size, len - first_w_size);
    }
    /*Update flag*/
    node_ptr->w_index += len;
    node_ptr->w_index %= total_size;
    node_ptr->valid_size += len;
}

/*dbuf_node_write_header is not provided*/

/**
 * @brief Read data from the dbuf data node, please clear the read data.
 *
 * @param node_ptr
 * @param buff
 * @param len
 */
static inline void dbuf_node_read(qurl_dbuf_node_t *node_ptr, void *buff, long len)
{
    long r_index = node_ptr->r_index;
    long total_size = node_ptr->total_size;
    long first_r_size = 0;

    if (len > node_ptr->valid_size) /*Bro, there's not enough data for you to read.*/
    {
        /*Here you can add log markers*/
        return;
    }

    if (total_size - r_index >= len) /*No need for segmented reading*/
    {
        if (buff != QOSA_NULL)
        {
            qurl_memcpy(buff, node_ptr->buf + r_index, len);
        }
    }
    else /*Need to read in segments*/
    {
        first_r_size = total_size - r_index;
        if (buff != QOSA_NULL)
        {
            qurl_memcpy(buff, node_ptr->buf + r_index, first_r_size);
            qurl_memcpy((unsigned char *)buff + first_r_size, node_ptr->buf, len - first_r_size);
        }
    }
    /*Update flag*/
    node_ptr->r_index += len;
    node_ptr->r_index %= total_size;
    node_ptr->valid_size -= len;
}

/*Read-only, do not delete*/
/**
 * @brief Read data from the dbuf data node without deleting the read data
 *
 * @param node_ptr
 * @param buff
 * @param len
 * @param offset
 */
static inline void dbuf_node_only_read(qurl_dbuf_node_t *node_ptr, void *buff, long len, long offset)
{
    long r_index = node_ptr->r_index;
    long total_size = node_ptr->total_size;
    long first_r_size = 0;

    if (len > (node_ptr->valid_size - offset)) /*Bro, there's not enough data for you to read.*/
    {
        /*Log markers can be placed here*/
        return;
    }

    if ((total_size - (r_index + offset)) >= len) /*No need for segmented reading*/
    {
        if (buff != QOSA_NULL)
        {
            qurl_memcpy(buff, node_ptr->buf + r_index + offset, len);
        }
    }
    else /*Need to read in segments*/
    {
        first_r_size = total_size - (r_index + offset);
        if (buff != QOSA_NULL)
        {
            qurl_memcpy(buff, node_ptr->buf + r_index + offset, first_r_size);
            qurl_memcpy((unsigned char *)buff + first_r_size, node_ptr->buf, len - first_r_size);
        }
    }
}

/**
 * @brief Get the pointer to continuous data from the dbuf data node, return 1 indicates there is a second half segment.
 *
 * @param node_ptr
 * @param buff
 * @param len
 * @param offset
 */
static inline qurl_bit_t dbuf_node_only_read_ptr(qurl_dbuf_node_t *node_ptr, void **buff_pptr, long *buff_len_ptr, long offset)
{
    long valid_size = node_ptr->valid_size;
    long r_index = node_ptr->r_index;
    long total_size = node_ptr->total_size;
    long first_r_size = 0;

    if ((total_size - (r_index + offset)) >= valid_size) /*No need for segmented reading*/
    {
        *buff_pptr = (void *)(node_ptr->buf + r_index + offset);
        *buff_len_ptr = valid_size;
        return 0;
    }
    else /*Need to read in segments*/
    {
        first_r_size = total_size - r_index;
        if (first_r_size > offset)
        {
            *buff_pptr = (void *)(node_ptr->buf + r_index + offset);
            *buff_len_ptr = first_r_size - offset;
            return 1;
        }
        else
        {
            *buff_pptr = (void *)(node_ptr->buf + (offset - first_r_size));
            *buff_len_ptr = valid_size - offset;
            return 0;
        }
    }
}

static inline long dbuf_node_valid_size(qurl_dbuf_node_t *node_ptr)
{
    return (node_ptr->valid_size);
}

static inline long dbuf_node_idle_size(qurl_dbuf_node_t *node_ptr)
{
    return (node_ptr->total_size - node_ptr->valid_size);
}

static inline unsigned char dbuf_node_is_empty(qurl_dbuf_node_t *node_ptr)
{
    return (node_ptr->valid_size == 0);
}

#if 0
// UNUSED
static inline unsigned char dbuf_node_is_full(qurl_dbuf_node_t *node_ptr)
{
    return (node_ptr->valid_size == node_ptr->total_size);
}
#endif

static int dbuf_node_delete_list_cmp_cb(void *node_ptr, void *arg)
{
    qurl_dbuf_node_t *dbuf_node_ptr = QOSA_NULL;

    if (node_ptr != QOSA_NULL)
    {
        dbuf_node_ptr = qurl_list_node_entry(node_ptr, qurl_dbuf_node_t, node);
        qurl_list_node_delete(arg, node_ptr);
        dbuf_node_delete(dbuf_node_ptr);
        qurl_free(dbuf_node_ptr);
    }

    return 0; /*Traverse all*/
}

/**
 * @brief Generate the backtracking table for the pattern string
 */
static void dbuf_kmp_nextval_creat(unsigned char *data_ptr, unsigned char *nextval)
{
    unsigned char i = 1;
    unsigned char j = 0;
    unsigned char len = nextval[0];

    nextval[1] = 0;

    while (i < len)
    {
        if (j == 0 || data_ptr[j - 1] == data_ptr[i - 1])
        {
            i++;
            j++;
            if (data_ptr[j - 1] != data_ptr[i - 1])
            {
                nextval[i] = j;
            }
            else
            {
                nextval[i] = nextval[j];
            }
        }
        else
        {
            j = nextval[j];
        }
    }
}

/* API */

/**
 * @brief Create a new dbuf
 *
 * @param dbuf_ptr
 * @param limit_size
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_new(qurl_dbuf_t *dbuf_ptr, long limit_size)
{
    qurl_ecode_t      ret = 0;
    qurl_dbuf_ctrl_t *ctrl_ptr = QOSA_NULL;

    if (limit_size == 0)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    ctrl_ptr = qurl_malloc(sizeof(qurl_dbuf_ctrl_t));
    if (ctrl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }
    qurl_memset(ctrl_ptr, 0x00, sizeof(qurl_dbuf_ctrl_t));

    ctrl_ptr->limit_size = limit_size;
    ctrl_ptr->total_size = 0;
    ctrl_ptr->valid_size = 0;

    ret = qurl_list_init(&(ctrl_ptr->data_list));
    if (ret != QURL_OK)
    {
        qurl_free(ctrl_ptr);
        return ret;
    }

    *dbuf_ptr = (qurl_dbuf_t)ctrl_ptr;

    return QURL_OK;
}

/**
 * @brief Delete dbuf
 *
 * @param dbuf
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_delete(qurl_dbuf_t dbuf)
{
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;

    if (ctrl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    qurl_list_search_each_safe(&(ctrl_ptr->data_list), dbuf_node_delete_list_cmp_cb, &(ctrl_ptr->data_list));
    qurl_list_delete(&(ctrl_ptr->data_list));

    qurl_free(ctrl_ptr);

    return QURL_OK;
}

/**
 * @brief Valid data in the dbuf
 *
 * @param dbuf
 * @param size_ptr
 * @return qurl_ecode_t
 */
long qurl_dbuf_vaild_size(qurl_dbuf_t dbuf)
{
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;

    if (ctrl_ptr == QOSA_NULL)
    {
        return -1;
    }

    return ctrl_ptr->valid_size;
}

/**
 * @brief Concatenate dbuf to the head
 *
 * @param dbuf
 * @param concat_dbuf
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_concat_head(qurl_dbuf_t dbuf, qurl_dbuf_t concat_dbuf)
{
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;
    qurl_dbuf_ctrl_t *concat_ctrl_ptr = (qurl_dbuf_ctrl_t *)concat_dbuf;

    if (ctrl_ptr == QOSA_NULL || concat_ctrl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    if ((ctrl_ptr->limit_size > 0) && (ctrl_ptr->valid_size + concat_ctrl_ptr->valid_size > ctrl_ptr->limit_size))
    {
        return QURL_ECODE_OVER_LIMIT;
    }

    qurl_list_concat_head(&ctrl_ptr->data_list, &concat_ctrl_ptr->data_list);
    ctrl_ptr->total_size += concat_ctrl_ptr->total_size;
    ctrl_ptr->valid_size += concat_ctrl_ptr->valid_size;
    concat_ctrl_ptr->total_size = 0;
    concat_ctrl_ptr->valid_size = 0;

    return QURL_OK;
}

/**
 * @brief Append dbuf to the end
 *
 * @param dbuf
 * @param concat_dbuf
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_concat_tail(qurl_dbuf_t dbuf, qurl_dbuf_t concat_dbuf)
{
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;
    qurl_dbuf_ctrl_t *concat_ctrl_ptr = (qurl_dbuf_ctrl_t *)concat_dbuf;

    if (ctrl_ptr == QOSA_NULL || concat_ctrl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    if ((ctrl_ptr->limit_size > 0) && (ctrl_ptr->valid_size + concat_ctrl_ptr->valid_size > ctrl_ptr->limit_size))
    {
        return QURL_ECODE_OVER_LIMIT;
    }

    /*[lzm][todo]Clear the idle space gaps at the splice points, first try merging, if not possible then try separating, and if still not possible then leave it as is.*/

    qurl_list_concat_tail(&ctrl_ptr->data_list, &concat_ctrl_ptr->data_list);
    ctrl_ptr->total_size += concat_ctrl_ptr->total_size;
    ctrl_ptr->valid_size += concat_ctrl_ptr->valid_size;
    concat_ctrl_ptr->total_size = 0;
    concat_ctrl_ptr->valid_size = 0;

    return QURL_OK;
}

/**
 * @brief Write data to dbuf
 *
 * @param dbuf
 * @param data_ptr
 * @param data_size
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_push_data(qurl_dbuf_t dbuf, void *data_ptr, long data_size)
{
    qurl_ecode_t      ret = 0;
    long              idle_size = 0; /*Current writable space of the dbuf chain*/
    long              new_dbuf_len = 0;
    long              written_len = 0;
    qurl_list_t       debris_list = {0};
    qurl_list_node_t *list_node_ptr = QOSA_NULL;
    qurl_dbuf_node_t *new_node_ptr = QOSA_NULL;
    qurl_dbuf_node_t *debris_new_node_ptr = QOSA_NULL;
    qurl_dbuf_node_t *node_ptr = QOSA_NULL;
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;

    if (ctrl_ptr == QOSA_NULL || data_size < 0)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    if (data_ptr == QOSA_NULL || data_size == 0)
    {
        return QURL_OK;
    }

    if (ctrl_ptr->limit_size > 0 && (ctrl_ptr->valid_size + data_size) > ctrl_ptr->limit_size)
    {
        return QURL_ECODE_OVER_LIMIT;
    }

    /*Two memory allocation schemes:
	 * 1. Apply for sufficient memory at once, then push the data in.
	 * 2. Allocate dynamic memory according to the minimum granularity of dbuf, then push it into the chain.
	 * Both solutions can be implemented, and when a single large allocation fails, try the minimal granularity allocation. This approach is suitable for platforms prone to memory fragmentation.*/
    list_node_ptr = (qurl_list_node_t *)qurl_list_check_tail(&(ctrl_ptr->data_list));
    if (list_node_ptr != QOSA_NULL)
    {
        node_ptr = qurl_list_node_entry(list_node_ptr, qurl_dbuf_node_t, node);
        idle_size = dbuf_node_idle_size(node_ptr);
    }

    if (node_ptr != QOSA_NULL && idle_size >= data_size) /*Currently, there is sufficient free space.*/
    {
        dbuf_node_write(node_ptr, data_ptr, data_size);
        ctrl_ptr->valid_size += data_size;
        idle_size = dbuf_node_idle_size(node_ptr);
    }
    else /*Need to add dbuf*/
    {
        new_dbuf_len = data_size - idle_size;
        if (new_dbuf_len < QURL_DBUF_NODE_MIN_SIZE)
        {
            new_dbuf_len = QURL_DBUF_NODE_MIN_SIZE;
        }

        new_node_ptr = qurl_malloc(sizeof(qurl_dbuf_node_t));
        if (new_node_ptr == QOSA_NULL)
        {
            return QURL_ECODE_NO_MEMORY;
        }

        ret = dbuf_node_new(new_node_ptr, new_dbuf_len);
        if (ret == QURL_OK)
        {
            /*Option 1: One-time Write*/
            if (node_ptr != QOSA_NULL && idle_size > 0)
            {
                dbuf_node_write(node_ptr, data_ptr, idle_size);
            }
            dbuf_node_write(new_node_ptr, (void *)((unsigned char *)data_ptr + idle_size), data_size - idle_size);
            qurl_list_put_tail(&(ctrl_ptr->data_list), &(new_node_ptr->node));
            ctrl_ptr->total_size += new_dbuf_len;
            ctrl_ptr->valid_size += data_size;
        }
        else if ((data_size - idle_size) != new_dbuf_len)
        {
            /*Option 2: Fragmented Write*/
            qurl_free(new_node_ptr);
            new_node_ptr = QOSA_NULL;

            qurl_list_init(&(debris_list));

            new_dbuf_len = 0;
            while (written_len < (data_size - idle_size))
            {
                debris_new_node_ptr = qurl_malloc(sizeof(qurl_dbuf_node_t));
                if (debris_new_node_ptr == QOSA_NULL)
                {
                    break;
                }

                ret = dbuf_node_new(debris_new_node_ptr, QURL_DBUF_NODE_MIN_SIZE);
                if (ret != QURL_OK)
                {
                    qurl_free(debris_new_node_ptr);
                    debris_new_node_ptr = QOSA_NULL;
                    break;
                }

                dbuf_node_write(
                    debris_new_node_ptr,
                    (void *)((unsigned char *)data_ptr + idle_size + written_len),
                    DBUF_MIN((data_size - idle_size - written_len), QURL_DBUF_NODE_MIN_SIZE)
                );
                qurl_list_put_tail(&(debris_list), &(debris_new_node_ptr->node));
                written_len += DBUF_MIN((data_size - idle_size - written_len), QURL_DBUF_NODE_MIN_SIZE);
                new_dbuf_len += QURL_DBUF_NODE_MIN_SIZE;
            };
            if (written_len < (data_size - idle_size))
            {
                /*Insufficient space caused*/
                qurl_list_search_each_safe(&debris_list, dbuf_node_delete_list_cmp_cb, &debris_list);
                qurl_list_delete(&(debris_list));
                return QURL_ECODE_NO_MEMORY;
            }
            /*There is no issue with the memory.*/
            if (node_ptr != QOSA_NULL && idle_size > 0)
            {
                dbuf_node_write(node_ptr, data_ptr, idle_size);
            }
            qurl_list_concat_tail(&(ctrl_ptr->data_list), &(debris_list));
            qurl_list_delete(&(debris_list));
            ctrl_ptr->total_size += new_dbuf_len;
            ctrl_ptr->valid_size += data_size;
        }
        else
        {
            /*There is really no space left.*/
            qurl_free(new_node_ptr);
            new_node_ptr = QOSA_NULL;
            return QURL_ECODE_NO_MEMORY;
        }
    }

    return QURL_OK;
}

/**
 * @brief Write to dbuf in formatted form
 * Encapsulated for use
 *
 * @param dbuf
 * @param fmt
 * @param ...
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_push_data_vfmt(qurl_dbuf_t dbuf, const char *fmt, va_list args)
{
    qurl_ecode_t      ret = QURL_OK;
    long              data_len = 0;
    char             *data_ptr = QOSA_NULL;
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;

    if (ctrl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    /*[lzm][note]The 20231204 solution increases memory peak usage but eliminates memory fragmentation.
Of course, we can use more code to implement other solutions:
For example, without increasing the memory peak, directly connect the currently allocated memory to the new dbuf node. This will create memory gaps.
For example: comprehensively evaluate the memory peak and gaps of the tail node and the current input data, then the tool determines whether to reallocate the memory of the existing tail node to clear the gaps based on the comprehensive evaluation result.*/

    data_len = (long)qurl_vasprintf(&data_ptr, fmt, args);
    if (data_ptr == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }

    ret = qurl_dbuf_push_data(dbuf, (void *)data_ptr, data_len);
    qurl_free(data_ptr);

    return ret;
}

/**
 * @brief Write to dbuf in formatted form
 *
 * @param dbuf
 * @param fmt
 * @param ...
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_push_data_fmt(qurl_dbuf_t dbuf, const char *fmt, ...)
{
    qurl_ecode_t ret = QURL_OK;
    va_list      args;

    va_start(args, fmt);
    ret = qurl_dbuf_push_data_vfmt(dbuf, fmt, args);
    va_end(args);

    return ret;
}

/**
 * @brief Pull specified length data from dbuf
 *
 * @param dbuf
 * @param data_ptr
 * @param pull_size
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_pull_data(qurl_dbuf_t dbuf, void *data_ptr, long pull_size)
{
    long              node_valid_len = 0;
    long              temp_size = pull_size; /*Remaining size to read*/
    qurl_list_node_t *list_node_ptr = QOSA_NULL;
    qurl_list_node_t *prev_list_node_ptr = QOSA_NULL;
    qurl_dbuf_node_t *node_ptr = QOSA_NULL;
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;

    if (ctrl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    if (pull_size == 0)
    {
        return QURL_OK;
    }

    if (pull_size > ctrl_ptr->valid_size)
    {
        return QURL_ECODE_OVER_LIMIT;
    }

    while (temp_size > 0)
    {
        prev_list_node_ptr = list_node_ptr;
        list_node_ptr = (qurl_list_node_t *)qurl_list_check_head(&(ctrl_ptr->data_list));
        if (list_node_ptr == QOSA_NULL || (prev_list_node_ptr == list_node_ptr))
        {
            break;
        }
        node_ptr = qurl_list_node_entry(list_node_ptr, qurl_dbuf_node_t, node);
        node_valid_len = dbuf_node_valid_size(node_ptr);
        if (data_ptr)
        {
            dbuf_node_read(node_ptr, (void *)((unsigned char *)data_ptr + (pull_size - temp_size)), DBUF_MIN(temp_size, node_valid_len));
        }
        else
        {
            dbuf_node_read(node_ptr, QOSA_NULL, DBUF_MIN(temp_size, node_valid_len));
        }
        temp_size -= DBUF_MIN(temp_size, node_valid_len);
        if (dbuf_node_is_empty(node_ptr))
        {
            qurl_list_node_delete(&(ctrl_ptr->data_list), list_node_ptr);
            dbuf_node_delete(node_ptr);
            ctrl_ptr->total_size -= node_ptr->total_size;
            qurl_free(node_ptr);
            node_ptr = QOSA_NULL;
        }
    }

    if (temp_size > 0)
    {
        ctrl_ptr->valid_size -= (pull_size - temp_size);
        /*[lzm][note]Perhaps the data can be rewritten back to the head of dbuf here.*/
        qurl_loge("error!!!\r\n");
        return QURL_ECODE_FAILURE;
    }

    ctrl_ptr->valid_size -= pull_size;

    return QURL_OK;
}

/**
 * @brief Pull limited-length data from dbuf, as much as possible.
 *
 * @param dbuf
 * @param data_ptr
 * @param pull_size_ptr[in/out] [in] Maximum length to pull; [out] Actual length pulled
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_pull_data_try(qurl_dbuf_t dbuf, void *data_ptr, long *pull_size_ptr)
{
    qurl_ecode_t      ret = 0;
    long              read_size = *pull_size_ptr;
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;

    if (ctrl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    if (read_size > ctrl_ptr->valid_size)
    {
        read_size = ctrl_ptr->valid_size;
    }

    ret = qurl_dbuf_pull_data(dbuf, data_ptr, read_size);
    if (ret == QURL_OK)
    {
        *pull_size_ptr = read_size;
    }

    return ret;
}

/**
 * @brief Clear all data from dbuf
 *
 * @param dbuf
 * @param data_ptr
 * @param pull_size
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_clear_data(qurl_dbuf_t dbuf)
{
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;

    if (ctrl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    qurl_list_search_each_safe(&(ctrl_ptr->data_list), dbuf_node_delete_list_cmp_cb, &(ctrl_ptr->data_list));

    ctrl_ptr->total_size = 0;
    ctrl_ptr->valid_size = 0;

    return QURL_OK;
}

/**
 * @brief Read data of specified length from dbuf (compared to pull, the difference with read is that read does not delete data)
 *
 * @param dbuf
 * @param data_ptr
 * @param read_size
 * @param offset[in]  start position
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_read_data(qurl_dbuf_t dbuf, void *data_ptr, long read_size, long offset)
{
    long              node_valid_len = 0;
    long              temp_size = read_size; /*Remaining size to read*/
    long              temp_offset = offset;  /*Remaining distance reading index gap*/
    qurl_list_node_t *list_node_ptr = QOSA_NULL;
    qurl_dbuf_node_t *node_ptr = QOSA_NULL;
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;

    if (ctrl_ptr == QOSA_NULL || (read_size > 0 && data_ptr == QOSA_NULL))
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    if (read_size == 0)
    {
        return QURL_OK;
    }

    if (read_size > (ctrl_ptr->valid_size - offset))
    {
        return QURL_ECODE_OVER_LIMIT;
    }

    list_node_ptr = (qurl_list_node_t *)qurl_list_check_head(&(ctrl_ptr->data_list));

    while (temp_size > 0)
    {
        if (list_node_ptr == QOSA_NULL)
        {
            break;
        }

        node_ptr = qurl_list_node_entry(list_node_ptr, qurl_dbuf_node_t, node);
        node_valid_len = dbuf_node_valid_size(node_ptr);
        if (temp_offset > 0 && temp_offset >= node_valid_len) /*The node does not contain the expected data.*/
        {
            temp_offset -= node_valid_len;
        }
        else /*This node contains the expected data to be read.*/
        {
            dbuf_node_only_read(
                node_ptr,
                (void *)((unsigned char *)data_ptr + (read_size - temp_size)),
                DBUF_MIN(temp_size, (node_valid_len - temp_offset)),
                temp_offset
            );
            temp_size -= DBUF_MIN(temp_size, (node_valid_len - temp_offset));
            temp_offset = 0; /*Reading has already started.*/
        }

        list_node_ptr = (qurl_list_node_t *)qurl_list_check_next(&(ctrl_ptr->data_list), list_node_ptr);
    }

    if (temp_size > 0)
    {
        qurl_loge("error!!![%ld][%ld]\r\n", read_size, temp_size);
        return QURL_ECODE_FAILURE;
    }

    return QURL_OK;
}

/**
 * @brief Read limited length data from dbuf (compared to pull, the difference with read is that read does not delete data)
 *
 * @param dbuf
 * @param data_ptr
 * @param read_size_ptr[in/out] [in] Maximum length to read; [out] Actual length read
 * @param offset[in]  start position
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_read_data_try(qurl_dbuf_t dbuf, void *data_ptr, long *read_size_ptr, long offset)
{
    qurl_ecode_t      ret = 0;
    long              read_size = *read_size_ptr;
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;

    if (ctrl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    if (read_size > (ctrl_ptr->valid_size - offset))
    {
        read_size = ctrl_ptr->valid_size - offset;
    }

    ret = qurl_dbuf_read_data(dbuf, data_ptr, read_size, offset);
    if (ret == QURL_OK)
    {
        *read_size_ptr = read_size;
    }

    return ret;
}

/**
 * @brief Match the string and return its starting position
 * Using the KMP algorithm: [https://www.cnblogs.com/lizhuming/p/15487367.html]
 *
 * @param dbuf
 * @param data_ptr
 * @param data_len [in] must not exceed 255; embedded retrieval does not require this many
 * @param offset
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_dbuf_find(qurl_dbuf_t dbuf, unsigned char *data_ptr, unsigned char data_len, long offset)
{
    qurl_bit_t        nextval_alloc = QOSA_FALSE;
    qurl_bit_t        have_data = QOSA_FALSE;
    long              i = 0;
    long              j = 0;
    long              temp_offset = offset; /*Remaining distance read index gap*/
    long              node_valid_len = 0;
    long              dbuf_valid_len = 0;
    long              cache_index = 0;
    long              cache_len = 0;
    unsigned char    *cache_ptr = QOSA_NULL;
    unsigned char    *nextval_ptr = QOSA_NULL;
    qurl_list_node_t *list_node_ptr = QOSA_NULL;
    qurl_dbuf_node_t *node_ptr = QOSA_NULL;
    qurl_dbuf_ctrl_t *ctrl_ptr = (qurl_dbuf_ctrl_t *)dbuf;
    unsigned char    *nextval[22] = {0}; /*If the length of the matching data does not exceed 20, we use the memory here for the matching table.*/

    if (ctrl_ptr == QOSA_NULL)
    {
        return (int)QURL_ECODE_PARAM_INVALID < 0 ? QURL_ECODE_PARAM_INVALID : -QURL_ECODE_PARAM_INVALID;
    }

    if ((long)data_len > (ctrl_ptr->valid_size - offset))
    {
        return (int)QURL_ECODE_PARAM_INVALID < 0 ? QURL_ECODE_PARAM_INVALID : -QURL_ECODE_PARAM_INVALID;
    }
    dbuf_valid_len = ctrl_ptr->valid_size - offset;

    if (data_len > sizeof(nextval) - 2)
    {
        nextval_ptr = (unsigned char *)qurl_malloc(data_len + 2);
        if (nextval_ptr == NULL)
        {
            return (int)QURL_ECODE_NO_MEMORY < 0 ? QURL_ECODE_NO_MEMORY : -QURL_ECODE_NO_MEMORY;
        }
        nextval_alloc = QOSA_TRUE;
    }
    else
    {
        nextval_ptr = (unsigned char *)(&nextval[0]);
    }
    nextval_ptr[0] = data_len;
    nextval_ptr[data_len + 1] = 0;

    dbuf_kmp_nextval_creat(data_ptr, nextval_ptr);

    list_node_ptr = (qurl_list_node_t *)qurl_list_check_head(&(ctrl_ptr->data_list));
    while (i <= dbuf_valid_len && j <= data_len && list_node_ptr != QOSA_NULL)
    {
        node_ptr = qurl_list_node_entry(list_node_ptr, qurl_dbuf_node_t, node);
        node_valid_len = dbuf_node_valid_size(node_ptr);
        if (temp_offset > 0 && temp_offset >= node_valid_len) /*The node does not contain the expected data.*/
        {
            temp_offset -= node_valid_len;
        }
        else /*This node contains the expected data to be retrieved.*/
        {
            cache_len = 0;
            have_data = QOSA_TRUE;
            while (have_data && i <= dbuf_valid_len && j <= data_len)
            {
                have_data = dbuf_node_only_read_ptr(node_ptr, (void **)&cache_ptr, &cache_len, temp_offset + cache_len);
                temp_offset = 0; /*Reading has already started.*/

                cache_index = 0;

                while (cache_index <= cache_len && i <= dbuf_valid_len && j <= data_len)
                {
                    /*Compare*/
                    if (j == 0 || cache_ptr[cache_index] == data_ptr[j - 1])
                    {
                        cache_index++;
                        i++;
                        j++;
                    }
                    else
                    {
                        /*Backtrack j*/
                        j = nextval_ptr[j];
                    }
                }
            }
        }

        list_node_ptr = (qurl_list_node_t *)qurl_list_check_next(&(ctrl_ptr->data_list), list_node_ptr);
    }
    if (nextval_alloc)
    {
        qurl_free(nextval_ptr);
        nextval_ptr = QOSA_NULL;
    }
    if (j > data_len)
    {
        return (qurl_ecode_t)(i - (int)data_len);
    }

    return (int)QURL_ECODE_NOT_FIND < 0 ? QURL_ECODE_NOT_FIND : -QURL_ECODE_NOT_FIND;
}
