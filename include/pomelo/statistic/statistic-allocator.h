#ifndef POMELO_STATISTIC_ALLOCATOR_H
#define POMELO_STATISTIC_ALLOCATOR_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_statistic_allocator_s {
    /// @brief The number of in-use bytes
    uint64_t allocated_bytes;
};


/// @brief The statistic of allocator module
typedef struct pomelo_statistic_allocator_s pomelo_statistic_allocator_t;


#ifdef __cplusplus
}
#endif
#endif // POMELO_STATISTIC_ALLOCATOR_H
