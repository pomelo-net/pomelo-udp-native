#ifndef POMELO_DELIVERY_CONTEXT_SRC_H
#define POMELO_DELIVERY_CONTEXT_SRC_H
#include "utils/pool.h"
#include "platform/platform.h"
#include "base/buffer.h"
#include "fragment.h"
#include "delivery.h"

// The following APIs must be accessed through the interface of context.

/// @brief The buffer size of fragments shared pool
#define POMELO_DELIVERY_FRAGMENTS_POOL_BUFFER_SHARED_BUFFER_SIZE 128

/// @brief The buffer size of parcels shared pool
#define POMELO_DELIVERY_PARCELS_POOL_BUFFER_SHARED_BUFFER_SIZE 128


#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_delivery_context_root_s {
    /// @brief The interface of delivery context
    pomelo_delivery_context_t base;

    /// @brief The allocator of context
    pomelo_allocator_t * allocator;

    /// @brief [Shared] The buffer context
    pomelo_buffer_context_root_t * buffer_context;

    /// @brief [Synchronized] The pool of fragments
    pomelo_pool_t * fragment_pool;

    /// @brief [Synchronized] The pool of parcels.
    pomelo_pool_t * parcel_pool;
};

#ifdef POMELO_MULTI_THREAD

struct pomelo_delivery_context_shared_s {
    /// @brief The interface of delivery context
    pomelo_delivery_context_t base;

    /// @brief The allocator of context
    pomelo_allocator_t * allocator;

    /// @brief The root context of this local context
    pomelo_delivery_context_root_t * root_context;

    /// @brief The buffer context
    pomelo_buffer_context_shared_t * buffer_context;

    /// @brief The pool of fragments
    pomelo_shared_pool_t * fragment_pool;

    /// @brief The pool of parcels.
    pomelo_shared_pool_t * parcel_pool;
};

#endif // ~POMELO_MULTI_THREAD

/* -------------------------------------------------------------------------- */
/*                            Global context APIs                             */
/* -------------------------------------------------------------------------- */


/// @brief Acquire new fragment.
/// If buffer is passed NULL, new buffer will be acquired and attached to new
/// fragment. Otherwise, the buffer will be attached and referenced.
pomelo_delivery_fragment_t * pomelo_delivery_context_root_acquire_fragment(
    pomelo_delivery_context_root_t * context,
    pomelo_buffer_t * buffer
);


/// @brief Release fragment to context
void pomelo_delivery_context_root_release_fragment(
    pomelo_delivery_context_root_t * context,
    pomelo_delivery_fragment_t * fragment
);


/// @brief Acquire a parcel for sending.
pomelo_delivery_parcel_t * pomelo_delivery_context_root_acquire_parcel(
    pomelo_delivery_context_root_t * context
);


/// @brief Release a parcel.
/// This will forcely set the ref counter of parcel to 0.
void pomelo_delivery_context_root_release_parcel(
    pomelo_delivery_context_root_t * context,
    pomelo_delivery_parcel_t * parcel
);


/// @brief Get the statistic of delivery context
void pomelo_delivery_context_root_statistic(
    pomelo_delivery_context_root_t * context,
    pomelo_statistic_delivery_t * statistic
);


/* -------------------------------------------------------------------------- */
/*                           Shared context APIs                              */
/* -------------------------------------------------------------------------- */

#ifdef POMELO_MULTI_THREAD

/// @brief Acquire new fragment.
/// If buffer is passed NULL, new buffer will be acquired and attached to new
/// fragment. Otherwise, the buffer will be attached and referenced.
pomelo_delivery_fragment_t * pomelo_delivery_context_shared_acquire_fragment(
    pomelo_delivery_context_shared_t * context,
    pomelo_buffer_t * buffer
);


/// @brief Release fragment to context
void pomelo_delivery_context_shared_release_fragment(
    pomelo_delivery_context_shared_t * context,
    pomelo_delivery_fragment_t * fragment
);


/// @brief Acquire a parcel for sending.
pomelo_delivery_parcel_t * pomelo_delivery_context_shared_acquire_parcel(
    pomelo_delivery_context_shared_t * context
);

/// @brief Release a parcel.
/// This will forcely set the ref counter of parcel to 0.
void pomelo_delivery_context_shared_release_parcel(
    pomelo_delivery_context_shared_t * context,
    pomelo_delivery_parcel_t * parcel
);


/// @brief Get the statistic of delivery context
void pomelo_delivery_context_shared_statistic(
    pomelo_delivery_context_shared_t * context,
    pomelo_statistic_delivery_t * statistic
);

#endif // !POMELO_MULTI_THREAD

/* -------------------------------------------------------------------------- */
/*                             Common context APIs                            */
/* -------------------------------------------------------------------------- */


/// @brief Clone the fragment
pomelo_delivery_fragment_t * pomelo_delivery_context_clone_writing_fragment(
    pomelo_delivery_context_t * context,
    pomelo_delivery_fragment_t * fragment
);



#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_CONTEXT_SRC_H
