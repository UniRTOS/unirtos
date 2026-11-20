#include "volc_atomic.h"

void volc_atomic_store(volatile size_t* p_atomic, size_t val) {
    __atomic_store_n(p_atomic, val, __ATOMIC_SEQ_CST);
}

bool volc_atomic_compare_exchange(volatile size_t* p_atomic, size_t* p_expected, size_t desired) {
    return __atomic_compare_exchange_n(p_atomic, p_expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_load(volatile size_t* p_atomic) {
    return __atomic_load_n(p_atomic, __ATOMIC_SEQ_CST);
}
size_t volc_atomic_exchange(volatile size_t* p_atomic, size_t val) {
    return __atomic_exchange_n(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_increment(volatile size_t* p_atomic) {
    return __atomic_fetch_add(p_atomic, 1, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_decrement(volatile size_t* p_atomic) {
    return __atomic_fetch_sub(p_atomic, 1, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_add(volatile size_t* p_atomic, size_t val) {
    return __atomic_fetch_add(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_sub(volatile size_t* p_atomic, size_t val) {
    return __atomic_fetch_sub(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_and(volatile size_t* p_atomic, size_t val) {
    return __atomic_fetch_and(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_or(volatile size_t* p_atomic, size_t val) {
    return __atomic_fetch_or(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_xor(volatile size_t* p_atomic, size_t val) {
    return __atomic_fetch_xor(p_atomic, val, __ATOMIC_SEQ_CST);
}