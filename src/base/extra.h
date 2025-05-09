#ifndef POMELO_API_EXTRA_SRC_H
#define POMELO_API_EXTRA_SRC_H
#include "utils/atomic.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Extra pointer
typedef pomelo_atomic_uint64_t pomelo_extra_t;


/// @brief Set extra data
#define pomelo_extra_set(object, value)                                        \
    pomelo_atomic_uint64_store(&(object), (uint64_t) (value))


/// @brief Get extra data
#define pomelo_extra_get(object) ((void *) pomelo_atomic_uint64_load(&(object)))


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_EXTRA_SRC_H
