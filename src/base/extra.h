#ifndef POMELO_API_EXTRA_SRC_H
#define POMELO_API_EXTRA_SRC_H
#ifdef POMELO_MULTI_THREAD
#include "utils/atomic.h"
#endif // POMELO_MULTI_THREAD
#ifdef __cplusplus
extern "C" {
#endif

#ifdef POMELO_MULTI_THREAD

/// @brief Extra pointer
typedef pomelo_atomic_uint64_t pomelo_extra_t;

/// Set extra data
#define pomelo_extra_set(object, value)                                        \
    pomelo_atomic_uint64_store(&(object), (uint64_t) (value))

/// Get extra data
#define pomelo_extra_get(object) ((void *) pomelo_atomic_uint64_load(&(object)))

#else

/// @brief Extra pointer
typedef void * pomelo_extra_t;

/// Set extra data
#define pomelo_extra_set(object, value) (object) = value

/// Get extra data
#define pomelo_extra_get(object) (object)

#endif // POMELO_MULTI_THREAD


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_EXTRA_SRC_H
