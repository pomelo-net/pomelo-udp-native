#include <assert.h>
#include <stddef.h>
#include "atomic.h"

#ifdef _MSC_VER // For windows
#include <Windows.h>


uint64_t pomelo_atomic_uint64_fetch_add(
    pomelo_atomic_uint64_t * object,
    uint64_t value
) {
    assert(object != NULL);
    return InterlockedExchangeAddNoFence64(
        (LONG64 volatile *) object,
        ((LONG64) value)
    );
}


uint64_t pomelo_atomic_uint64_fetch_sub(
    pomelo_atomic_uint64_t * object,
    uint64_t value
) {
    assert(object != NULL);
    return InterlockedExchangeAddNoFence64(
        (LONG64 volatile *) object,
        -((LONG64) value)
    );
}


void pomelo_atomic_uint64_store(
    pomelo_atomic_uint64_t * object,
    uint64_t value
) {
    assert(object != NULL);
    InterlockedExchangeNoFence64((LONG64 volatile *) object, value);
}


bool pomelo_atomic_uint64_compare_exchange(
    pomelo_atomic_uint64_t * object,
    uint64_t expected_value,
    uint64_t new_value
) {
    assert(object != NULL);
    LONG64 result = InterlockedCompareExchange64(
        (LONG64 volatile *) object,
        (LONG64) new_value,
        (LONG64) expected_value
    );

    return ((uint64_t) result) == expected_value;
}


uint64_t pomelo_atomic_uint64_exchange(
    pomelo_atomic_uint64_t * object,
    uint64_t new_value
) {
    assert(object != NULL);
    return (uint64_t) InterlockedExchangeNoFence64(
        (LONG64 volatile *) object,
        new_value
    );
}


uint64_t pomelo_atomic_uint64_load(pomelo_atomic_uint64_t * object) {
    assert(object != NULL);
    return InterlockedExchangeAddNoFence64((LONG64 volatile *) object, 0);
}


int64_t pomelo_atomic_int64_fetch_add(
    pomelo_atomic_int64_t * object,
    int64_t value
) {
    assert(object != NULL);
    return InterlockedExchangeAddNoFence64(
        (LONG64 volatile *) object,
        ((LONG64) value)
    );
}


int64_t pomelo_atomic_int64_fetch_sub(
    pomelo_atomic_int64_t * object,
    int64_t value
) {
    assert(object != NULL);
    return InterlockedExchangeAddNoFence64(
        (LONG64 volatile *) object,
        -((LONG64) value)
    );
}


void pomelo_atomic_int64_store(
    pomelo_atomic_int64_t * object,
    int64_t value
) {
    assert(object != NULL);
    InterlockedExchangeNoFence64((LONG64 volatile *) object, value);
}


bool pomelo_atomic_int64_compare_exchange(
    pomelo_atomic_int64_t * object,
    int64_t expected_value,
    int64_t new_value
) {
    assert(object != NULL);
    LONG64 result = InterlockedCompareExchange64(
        (LONG64 volatile *) object,
        (LONG64) new_value,
        (LONG64) expected_value
    );

    return ((int64_t) result) == expected_value;
}


int64_t pomelo_atomic_int64_exchange(
    pomelo_atomic_int64_t * object,
    int64_t new_value
) {
    assert(object != NULL);
    return (int64_t) InterlockedExchangeNoFence64(
        (LONG64 volatile *) object,
        new_value
    );
}


int64_t pomelo_atomic_int64_load(pomelo_atomic_int64_t * object) {
    assert(object != NULL);
    return InterlockedExchangeAddNoFence64((LONG64 volatile *) object, 0);
}

#else

#include <stdatomic.h>

uint64_t pomelo_atomic_uint64_fetch_add(
    pomelo_atomic_uint64_t * object,
    uint64_t value
) {
    assert(object != NULL);
    return atomic_fetch_add_explicit(object, value, memory_order_relaxed);
}


uint64_t pomelo_atomic_uint64_fetch_sub(
    pomelo_atomic_uint64_t * object,
    uint64_t value
) {
    assert(object != NULL);
    return atomic_fetch_sub_explicit(object, value, memory_order_relaxed);
}


uint64_t pomelo_atomic_uint64_load(pomelo_atomic_uint64_t * object) {
    assert(object != NULL);
    return atomic_load_explicit(object, memory_order_relaxed);
}


void pomelo_atomic_uint64_store(
    pomelo_atomic_uint64_t * object,
    uint64_t value
) {
    assert(object != NULL);
    atomic_store_explicit(object, value, memory_order_relaxed);
}


bool pomelo_atomic_uint64_compare_exchange(
    pomelo_atomic_uint64_t * object,
    uint64_t expected_value,
    uint64_t new_value
) {
    assert(object != NULL);
    return atomic_compare_exchange_weak(object, &expected_value, new_value);
}


uint64_t pomelo_atomic_uint64_exchange(
    pomelo_atomic_uint64_t * object,
    uint64_t new_value
) {
    assert(object != NULL);
    return atomic_exchange_explicit(object, new_value, memory_order_relaxed);
}


int64_t pomelo_atomic_int64_fetch_add(
    pomelo_atomic_int64_t * object,
    int64_t value
) {
    assert(object != NULL);
    return atomic_fetch_add_explicit(object, value, memory_order_relaxed);
}


int64_t pomelo_atomic_int64_fetch_sub(
    pomelo_atomic_int64_t * object,
    int64_t value
) {
    assert(object != NULL);
    return atomic_fetch_sub_explicit(object, value, memory_order_relaxed);
}


int64_t pomelo_atomic_int64_load(pomelo_atomic_int64_t * object) {
    assert(object != NULL);
    return atomic_load_explicit(object, memory_order_relaxed);
}


void pomelo_atomic_int64_store(pomelo_atomic_int64_t * object, int64_t value) {
    assert(object != NULL);
    atomic_store_explicit(object, value, memory_order_relaxed);
}


bool pomelo_atomic_int64_compare_exchange(
    pomelo_atomic_int64_t * object,
    int64_t expected_value,
    int64_t new_value
) {
    assert(object != NULL);
    return atomic_compare_exchange_weak(object, &expected_value, new_value);
}


int64_t pomelo_atomic_int64_exchange(
    pomelo_atomic_int64_t * object,
    int64_t new_value
) {
    assert(object != NULL);
    return atomic_exchange_explicit(object, new_value, memory_order_relaxed);
}


#endif // For Unix
