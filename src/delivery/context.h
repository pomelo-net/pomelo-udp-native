#ifndef POMELO_DELIVERY_CONTEXT_SRC_H
#define POMELO_DELIVERY_CONTEXT_SRC_H
#include "utils/pool.h"
#include "platform/platform.h"
#include "base/buffer.h"
#include "delivery.h"


/// @brief The buffer size of parcels shared pool
#define POMELO_DELIVERY_PARCELS_POOL_BUFFER_SHARED_BUFFER_SIZE 128


#ifdef __cplusplus
extern "C" {
#endif


/// @brief The root delivery context
typedef struct pomelo_delivery_context_root_s pomelo_delivery_context_root_t;


/// @brief The non-threadsafe shared delivery context
typedef struct pomelo_delivery_context_shared_s
    pomelo_delivery_context_shared_t;


/// @brief The statistic function of delivery context
typedef void (*pomelo_delivery_context_statistic_fn)(
    pomelo_delivery_context_t * context,
    pomelo_statistic_delivery_t * statistic
);


/// @brief Parcel acquiring function
typedef pomelo_delivery_parcel_t *
(*pomelo_delivery_context_acquire_parcel_fn)(
    pomelo_delivery_context_t * context
);


/// @brief Parcel releasing function
typedef void (*pomelo_delivery_context_release_parcel_fn)(
    pomelo_delivery_context_t * context,
    pomelo_delivery_parcel_t * parcel
);


struct pomelo_delivery_context_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The root context of this context. This might be itself.
    pomelo_delivery_context_root_t * root;

    /// @brief The buffer context
    pomelo_buffer_context_t * buffer_context;

    /// @brief Acquire new parcel
    pomelo_delivery_context_acquire_parcel_fn acquire_parcel;

    /// @brief Release a parcel
    pomelo_delivery_context_release_parcel_fn release_parcel;

    /// @brief Get the statistic of delivery context
    pomelo_delivery_context_statistic_fn statistic;

    /// @brief The capacity of fragment payload
    size_t fragment_content_capacity;

    /// @brief The maximum number of fragments in a parcel
    size_t max_fragments;

    /// @brief The pool of dispatchers
    pomelo_pool_t * dispatcher_pool;

    /// @brief The pool of senders
    pomelo_pool_t * sender_pool;

    /// @brief The pool of receivers
    pomelo_pool_t * receiver_pool;

    /// @brief The pool of endpoints
    pomelo_pool_t * endpoint_pool;

    /// @brief The pool of buses
    pomelo_pool_t * bus_pool;

    /// @brief The pool of receptions
    pomelo_pool_t * reception_pool;

    /// @brief The pool of transmissions
    pomelo_pool_t * transmission_pool;

    /// @brief The pool of heartbeat objects
    pomelo_pool_t * heartbeat_pool;
};


struct pomelo_delivery_context_root_s {
    /// @brief The interface of delivery context
    pomelo_delivery_context_t base;

    /// @brief The buffer context
    pomelo_buffer_context_t * buffer_context;

    /// @brief [Synchronized] The pool of parcels.
    pomelo_pool_t * parcel_pool;
};


struct pomelo_delivery_context_shared_s {
    /// @brief The interface of delivery context
    pomelo_delivery_context_t base;

    /// @brief The buffer context
    pomelo_buffer_context_t * buffer_context;

    /// @brief The pool of parcels.
    pomelo_pool_t * parcel_pool;
};

/* -------------------------------------------------------------------------- */
/*                             Root context APIs                              */
/* -------------------------------------------------------------------------- */

/// @brief Destroy a delivery context
void pomelo_delivery_context_root_destroy(
    pomelo_delivery_context_root_t * context
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


/// @brief Destroy a local-thread delivery context
void pomelo_delivery_context_shared_destroy(
    pomelo_delivery_context_shared_t * context
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


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_CONTEXT_SRC_H
