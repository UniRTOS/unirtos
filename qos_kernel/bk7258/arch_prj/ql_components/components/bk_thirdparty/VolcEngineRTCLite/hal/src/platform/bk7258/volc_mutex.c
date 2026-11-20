#include <common/bk_include.h>
#include "FreeRTOS_POSIX.h"
#include "posix/pthread.h"

#include "volc_mutex.h"

// #include <pthread.h>

#include "volc_errno.h"
#include "volc_memory.h"

/* 最好不要依赖可重入锁 */
volc_mutex_t volc_mutex_create(bool reentrant) {
    pthread_mutex_t* p_mutex = NULL;
    pthread_mutexattr_t attr;

    p_mutex = (pthread_mutex_t *)volc_calloc(1, sizeof(pthread_mutex_t));
    if (NULL == p_mutex) {
        return NULL;
    }

    if (0 != pthread_mutexattr_init(&attr) ||
        0 != pthread_mutexattr_settype(&attr, reentrant ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL) ||
        0 != pthread_mutex_init(p_mutex, &attr)) {
        volc_free(p_mutex);
        return NULL;
    }
    
    return (volc_mutex_t)p_mutex;
}

void volc_mutex_lock(volc_mutex_t mutex) {
    pthread_mutex_lock((pthread_mutex_t *)mutex);
}

bool volc_mutex_trylock(volc_mutex_t mutex) {
    return (0 == pthread_mutex_lock((pthread_mutex_t *)mutex));
}

void volc_mutex_unlock(volc_mutex_t mutex) {
    pthread_mutex_unlock((pthread_mutex_t *)mutex);
}

void volc_mutex_destroy(volc_mutex_t mutex) {
    pthread_mutex_t* p_mutex = (pthread_mutex_t *)mutex;
    if (NULL == p_mutex) {
        return;
    }
    pthread_mutex_destroy(p_mutex);
    volc_free(p_mutex);
}
