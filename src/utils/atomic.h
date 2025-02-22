/*
    Atomic compatible data types for internal usage.
    These types use relaxed memory order.
*/
#ifndef POMELO_UTILS_ATOMIC_SRC_H
#define POMELO_UTILS_ATOMIC_SRC_H
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


#ifdef _MSC_VER

/// @brief Atomic unsigned integer 64
typedef uint64_t volatile pomelo_atomic_uint64_t;

/// @brief Atomic signed integer 64
typedef int64_t volatile pomelo_atomic_int64_t;

#else

/// @brief Atomic unsigned integer 64
typedef _Atomic uint64_t pomelo_atomic_uint64_t;

/// @brief Atomic signedinteger 64
typedef _Atomic int64_t pomelo_atomic_int64_t;

#endif // !_MSC_VER


/// @brief Fetch & add uint64 atomic value
/// @return The previous value of atomic object
uint64_t pomelo_atomic_uint64_fetch_add(
    pomelo_atomic_uint64_t * object,
    uint64_t value
);


/// @brief Fetch & subtract uint64 atomic value
/// @return The previous value of atomic object
uint64_t pomelo_atomic_uint64_fetch_sub(
    pomelo_atomic_uint64_t * object,
    uint64_t value
);


/// @brief Load the atomic uint64 value
uint64_t pomelo_atomic_uint64_load(pomelo_atomic_uint64_t * object);


/// @brief Store the value to uint64 atomic
void pomelo_atomic_uint64_store(
    pomelo_atomic_uint64_t * object,
    uint64_t value
);


/// @brief Compare and set atomic value
/// @return true if they are equal and atomic object is set.
bool pomelo_atomic_uint64_compare_exchange(
    pomelo_atomic_uint64_t * object,
    uint64_t expected_value,
    uint64_t new_value
);


/// @brief Load the previous value and store new value
/// @return The previous value
uint64_t pomelo_atomic_uint64_exchange(
    pomelo_atomic_uint64_t * object,
    uint64_t new_value
);


/// @brief Fetch & add int64 atomic value
/// @return The previous value of atomic object
int64_t pomelo_atomic_int64_fetch_add(
    pomelo_atomic_int64_t * object,
    int64_t value
);


/// @brief Fetch & subtract int64 atomic value
/// @return The previous value of atomic object
int64_t pomelo_atomic_int64_fetch_sub(
    pomelo_atomic_int64_t * object,
    int64_t value
);


/// @brief Load the atomic int64 value
int64_t pomelo_atomic_int64_load(pomelo_atomic_int64_t * object);


/// @brief Store the value to uint64 atomic
void pomelo_atomic_int64_store(pomelo_atomic_int64_t * object, int64_t value);


/// @brief Compare and set atomic value
/// @return true if they are equal and atomic object is set.
bool pomelo_atomic_int64_compare_exchange(
    pomelo_atomic_int64_t * object,
    int64_t expected_value,
    int64_t new_value
);


/// @brief Load the previous value and store new value
/// @return The previous value
int64_t pomelo_atomic_int64_exchange(
    pomelo_atomic_int64_t * object,
    int64_t new_value
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_REF_COUNTER_SRC_H
