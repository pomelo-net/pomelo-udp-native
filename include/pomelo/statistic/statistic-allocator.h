#ifndef POMELO_STATISTIC_ALLOCATOR_H
#define POMELO_STATISTIC_ALLOCATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief The statistic of allocator module
typedef struct pomelo_statistic_allocator_s pomelo_statistic_allocator_t;

struct pomelo_statistic_allocator_s {
    /// @brief The number of allocated bytes
    uint64_t allocated_bytes;
};

#ifdef __cplusplus
}
#endif

#endif // POMELO_STATISTIC_ALLOCATOR_H
