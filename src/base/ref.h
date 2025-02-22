#ifndef POMELO_BASE_REF_COUNTER_SRC_H
#define POMELO_BASE_REF_COUNTER_SRC_H
#include <stdbool.h>
#ifdef POMELO_MULTI_THREAD
#include "utils/atomic.h"
#else // !POMELO_MULTI_THREAD
#include <stdint.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef POMELO_MULTI_THREAD

typedef pomelo_atomic_int64_t pomelo_ref_counter_t;
#define pomelo_ref_counter_get(ref_counter)                                    \
    pomelo_atomic_int64_load(&(ref_counter))

#else

typedef int64_t pomelo_ref_counter_t;
#define pomelo_ref_counter_get(ref_counter) (ref_counter)

#endif // POMELO_MULTI_THREAD


/// @brief Reference
typedef struct pomelo_reference_s pomelo_reference_t;

/// @brief Finalize callback of reference
typedef void (*pomelo_ref_finalize_cb)(pomelo_reference_t * ref);

struct pomelo_reference_s {
    /// @brief Ref counter
    pomelo_ref_counter_t ref_counter;

    /// @brief Finalize callback
    pomelo_ref_finalize_cb finalize_cb;
};


/// @brief Initialize reference. This will set ref counter as 1
void pomelo_reference_init(
    pomelo_reference_t * object,
    pomelo_ref_finalize_cb finalize_cb
);

/// @brief Increase ref counter of a reference.
/// @return True if success, or false if object is finalized
bool pomelo_reference_ref(pomelo_reference_t * object);

/// @brief Decrease ref counter of reference. If this value reaches 0, finalize
/// callback will be called.
void pomelo_reference_unref(pomelo_reference_t * object);

/// @brief Get ref count of reference
#define pomelo_reference_ref_count(object) \
    pomelo_ref_counter_get((object)->ref_counter)


#ifdef __cplusplus
}
#endif
#endif // POMELO_BASE_REF_COUNTER_SRC_H
