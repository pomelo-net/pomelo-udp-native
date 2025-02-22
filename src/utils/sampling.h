#ifndef POMELO_UTILS_SAMPLING_SRC_H
#define POMELO_UTILS_SAMPLING_SRC_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Sample set of uint64 values
typedef struct pomelo_sample_set_u64_s pomelo_sample_set_u64_t;

/// @brief Sample set of int64 values
typedef struct pomelo_sample_set_i64_s pomelo_sample_set_i64_t;


struct pomelo_sample_set_u64_s {
    /// @brief Initialized flag of sample
    bool initialized;

    /// @brief Current active sample in set
    size_t index;

    /// @brief Sample set
    uint64_t * values;

    /// @brief Sum of all elements in sample set
    uint64_t sum;

    /// @brief Sum of all squared elements in sample set
    uint64_t sum_squared;

    /// @brief Size of sample set
    size_t size;
};


struct pomelo_sample_set_i64_s {
    /// @brief Initialized flag of sample
    bool initialized;

    /// @brief Current active sample in set
    size_t index;

    /// @brief Sample set
    int64_t * values;

    /// @brief Sum of all elements in sample set
    int64_t sum;

    /// @brief Sum of all squared elements in sample set
    int64_t sum_squared;

    /// @brief Size of sample set
    size_t size;
};


/// @brief Initialize new u64 sample set
void pomelo_sample_set_u64_init(
    pomelo_sample_set_u64_t * sample_set,
    uint64_t * values,
    size_t size
);


/// @brief Submit a value to sample set
void pomelo_sample_set_u64_submit(
    pomelo_sample_set_u64_t * sample_set,
    uint64_t value
);


/// @brief Calculate mean and variance of sample set
void pomelo_sample_set_u64_calc(
    pomelo_sample_set_u64_t * sample_set,
    uint64_t * mean,
    uint64_t * variance
);



/// @brief Initialize i64 sample set
void pomelo_sample_set_i64_init(
    pomelo_sample_set_i64_t * sample_set,
    int64_t * values,
    size_t size
);


/// @brief Submit a value to sample set
void pomelo_sample_set_i64_submit(
    pomelo_sample_set_i64_t * sample_set,
    int64_t value
);


/// @brief Calculate mean and variance of sample set
void pomelo_sample_set_i64_calc(
    pomelo_sample_set_i64_t * sample_set,
    int64_t * mean,
    int64_t * variance
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_SAMPLING_SRC_H
