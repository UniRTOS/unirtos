#include <common/bk_include.h>
#include "FreeRTOS_POSIX.h"
#include "posix/pthread.h"
#include "posix/time.h"

#include "volc_cond.h"

#include "volc_errno.h"
#include "volc_memory.h"
#include "volc_time.h"

volc_cond_t volc_cond_create(void) {
    pthread_cond_t* p_cond = (pthread_cond_t *)volc_calloc(1, sizeof(pthread_cond_t));
    if (NULL == p_cond) {
        goto err_out_label;
    }

    if (0 != pthread_cond_init(p_cond, NULL)) {
        volc_free(p_cond);
        goto err_out_label;
    }

    return (volc_cond_t)p_cond;
err_out_label:
    return NULL;
}

uint32_t volc_cond_signal(volc_cond_t cond) {
    pthread_cond_t* p_cond = (pthread_cond_t *)cond;
    if (NULL == p_cond) {
        goto err_out_label;
    }

    if (0 != pthread_cond_signal(p_cond)) {
        goto err_out_label;
    }

    return VOLC_SUCCESS;
err_out_label:
    return VOLC_FAILED;
}

uint32_t volc_cond_broadcast(volc_cond_t cond) {
    pthread_cond_t* p_cond = (pthread_cond_t *)cond;
    if (NULL == p_cond) {
        goto err_out_label;
    }

    if (0 != pthread_cond_broadcast(p_cond)) {
        goto err_out_label;
    }

    return VOLC_SUCCESS;
err_out_label:
    return VOLC_FAILED;
}

uint32_t volc_cond_wait(volc_cond_t cond, volc_mutex_t mutex, uint64_t timeout) {
    uint32_t ret_status = VOLC_SUCCESS;
    int32_t ret_val = 0;
    struct timespec time_spec = {0};
    uint64_t cur_time = volc_get_time();
    uint64_t time = timeout + cur_time;
    pthread_cond_t* p_cond = (pthread_cond_t *)cond;
    pthread_mutex_t* p_mutex = (pthread_mutex_t *)mutex;

    // If we overflow or have specific infinite timeout then wait unconditionally
    if (time < timeout || VOLC_INFINITE_TIME_VALUE == time) {
        ret_val = pthread_cond_wait(p_cond, p_mutex);
        if (0 != ret_val) {
            return VOLC_FAILED;
        }
    } else {
        time_spec.tv_sec = time / VOLC_HUNDREDS_OF_NANOS_IN_A_SECOND;
        time_spec.tv_nsec = (time % VOLC_HUNDREDS_OF_NANOS_IN_A_SECOND) * VOLC_DEFAULT_TIME_UNIT_IN_NANOS;
        ret_val = pthread_cond_timedwait(p_cond, p_mutex, &time_spec);
        if (0 != ret_val) {
            return VOLC_FAILED;
        }
    }
    return ret_status;
}

void volc_cond_destroy(volc_cond_t cond) {
    pthread_cond_t* p_cond = (pthread_cond_t *)cond;
    if (NULL == p_cond) {
        return;
    }

    pthread_cond_destroy(p_cond);
    volc_free(p_cond);
}

