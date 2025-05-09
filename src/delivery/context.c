#include <assert.h>
#include <string.h>
#include "context.h"
#include "delivery.h"
#include "parcel.h"
#include "sender.h"
#include "receiver.h"
#include "endpoint.h"
#include "bus.h"
#include "dispatcher.h"
#include "heartbeat.h"


/// @brief Initialize the parcel
static int parcel_init(pomelo_delivery_parcel_t * parcel, void * unused) {
    (void) unused;
    return pomelo_delivery_parcel_init(parcel);
}


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


void pomelo_delivery_context_destroy(pomelo_delivery_context_t * context) {
    assert(context != NULL);
    if ((pomelo_delivery_context_t *) (context->root) == context) {
        pomelo_delivery_context_root_destroy(
            (pomelo_delivery_context_root_t *) context
        );
    } else {
        pomelo_delivery_context_shared_destroy(
            (pomelo_delivery_context_shared_t *) context
        );
    }
}


void pomelo_delivery_context_statistic(
    pomelo_delivery_context_t * context,
    pomelo_statistic_delivery_t * statistic
) {
    assert(context != NULL);
    context->statistic(context, statistic);
}


pomelo_delivery_parcel_t * pomelo_delivery_context_acquire_parcel(
    pomelo_delivery_context_t * context
) {
    assert(context != NULL);
    return context->acquire_parcel(context);
}


/* -------------------------------------------------------------------------- */
/*                               Module APIs                                  */
/* -------------------------------------------------------------------------- */

pomelo_delivery_context_t * pomelo_delivery_context_root_create(
    pomelo_delivery_context_root_options_t * options
) {
    assert(options != NULL);
    if (options->fragment_capacity <= POMELO_MAX_FRAGMENT_META_DATA_BYTES) {
        // Not enough capacity for fragment
        return NULL;
    }

    size_t max_fragments = options->max_fragments;
    if (max_fragments == 0) {
        max_fragments = POMELO_DELIVERY_PARCEL_MAX_FRAGMENTS_DEFAULT;
    } else if (max_fragments > POMELO_DELIVERY_PARCEL_MAX_FRAGMENTS) {
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_delivery_context_root_t * context =
        pomelo_allocator_malloc_t(allocator, pomelo_delivery_context_root_t);
    if (!context) {
        return NULL;
    }
    memset(context, 0, sizeof(pomelo_delivery_context_root_t));

    context->buffer_context = (pomelo_buffer_context_t *)
        options->buffer_context->root;
    pomelo_pool_root_options_t pool_options;


    // Create pool of parcels
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_parcel_t);
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_delivery_parcel_on_alloc;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_delivery_parcel_on_free;
    pool_options.on_init = (pomelo_pool_init_cb) parcel_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_delivery_parcel_cleanup;
    pool_options.alloc_data = context;
    pool_options.synchronized = options->synchronized;

    context->parcel_pool = pomelo_pool_root_create(&pool_options);
    if (!context->parcel_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    // Initialize the interface
    pomelo_delivery_context_t * base = &context->base;
    base->allocator = allocator;
    base->root = context;
    base->buffer_context = context->buffer_context;
    base->acquire_parcel = (pomelo_delivery_context_acquire_parcel_fn)
        pomelo_delivery_context_root_acquire_parcel;
    base->release_parcel = (pomelo_delivery_context_release_parcel_fn)
        pomelo_delivery_context_root_release_parcel;
    base->statistic = (pomelo_delivery_context_statistic_fn)
        pomelo_delivery_context_root_statistic;

    // Update the payload capacity (exclude the meta)
    base->fragment_content_capacity =
        options->fragment_capacity - POMELO_MAX_FRAGMENT_META_DATA_BYTES;

    base->max_fragments = max_fragments;

    // Create pool of dispatchers
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_dispatcher_t);
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_delivery_dispatcher_on_alloc;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_delivery_dispatcher_on_free;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_delivery_dispatcher_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_delivery_dispatcher_cleanup;
    pool_options.alloc_data = context;
    base->dispatcher_pool = pomelo_pool_root_create(&pool_options);
    if (!base->dispatcher_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    // Create pool of senders
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_sender_t);
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_delivery_sender_on_alloc;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_delivery_sender_on_free;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_delivery_sender_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_delivery_sender_cleanup;
    pool_options.alloc_data = context;

    base->sender_pool = pomelo_pool_root_create(&pool_options);
    if (!base->sender_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    // Create pool of receivers
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_receiver_t);
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_delivery_receiver_on_alloc;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_delivery_receiver_on_free;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_delivery_receiver_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_delivery_receiver_cleanup;
    pool_options.alloc_data = context;
    base->receiver_pool = pomelo_pool_root_create(&pool_options);
    if (!base->receiver_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    // Create pool of endpoints
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_endpoint_t);
    pool_options.alloc_data = context;
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_delivery_endpoint_on_alloc;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_delivery_endpoint_on_free;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_delivery_endpoint_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_delivery_endpoint_cleanup;
    base->endpoint_pool = pomelo_pool_root_create(&pool_options);
    if (!base->endpoint_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    // Create pool of buses
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_bus_t);
    pool_options.alloc_data = context;
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_delivery_bus_on_alloc;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_delivery_bus_on_free;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_delivery_bus_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_delivery_bus_cleanup;
    base->bus_pool = pomelo_pool_root_create(&pool_options);
    if (!base->bus_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    // Create pool of bus receives
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_reception_t);
    pool_options.alloc_data = context;
    base->reception_pool = pomelo_pool_root_create(&pool_options);
    if (!base->reception_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    // Create pool of recipients
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_transmission_t);
    pool_options.zero_init = true;
    base->transmission_pool = pomelo_pool_root_create(&pool_options);
    if (!base->transmission_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    // Create pool of heartbeat objects
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_heartbeat_t);
    pool_options.alloc_data = base;
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_delivery_heartbeat_on_alloc;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_delivery_heartbeat_on_free;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_delivery_heartbeat_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_delivery_heartbeat_cleanup;
    base->heartbeat_pool = pomelo_pool_root_create(&pool_options);
    if (!base->heartbeat_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    return base;
}


/// @brief Destroy a delivery context
void pomelo_delivery_context_root_destroy(
    pomelo_delivery_context_root_t * context
) {
    assert(context != NULL);

    pomelo_delivery_context_t * base = &context->base;
    if (base->dispatcher_pool) {
        pomelo_pool_destroy(base->dispatcher_pool);
        base->dispatcher_pool = NULL;
    }

    if (base->sender_pool) {
        pomelo_pool_destroy(base->sender_pool);
        base->sender_pool = NULL;
    }

    if (base->receiver_pool) {
        pomelo_pool_destroy(base->receiver_pool);
        base->receiver_pool = NULL;
    }

    if (base->reception_pool) {
        pomelo_pool_destroy(base->reception_pool);
        base->reception_pool = NULL;
    }

    if (base->endpoint_pool) {
        pomelo_pool_destroy(base->endpoint_pool);
        base->endpoint_pool = NULL;
    }

    if (base->bus_pool) {
        pomelo_pool_destroy(base->bus_pool);
        base->bus_pool = NULL;
    }

    if (context->parcel_pool) {
        pomelo_pool_destroy(context->parcel_pool);
        context->parcel_pool = NULL;
    }

    if (base->transmission_pool) {
        pomelo_pool_destroy(base->transmission_pool);
        base->transmission_pool = NULL;
    }

    if (base->heartbeat_pool) {
        pomelo_pool_destroy(base->heartbeat_pool);
        base->heartbeat_pool = NULL;
    }

    pomelo_allocator_free(base->allocator, context);
}


pomelo_delivery_parcel_t * pomelo_delivery_context_root_acquire_parcel(
    pomelo_delivery_context_root_t * context
) {
    assert(context != NULL);

    pomelo_delivery_parcel_t * parcel =
        pomelo_pool_acquire(context->parcel_pool, NULL);
    if (!parcel) return NULL;

    parcel->context = &context->base;
    return parcel;
}


void pomelo_delivery_context_root_release_parcel(
    pomelo_delivery_context_root_t * context,
    pomelo_delivery_parcel_t * parcel
) {
    assert(context != NULL);
    pomelo_pool_release(context->parcel_pool, parcel);
}


void pomelo_delivery_context_root_statistic(
    pomelo_delivery_context_root_t * context,
    pomelo_statistic_delivery_t * statistic
) {
    assert(context != NULL);
    assert(statistic != NULL);

    pomelo_delivery_context_t * base = &context->base;
    statistic->dispatchers = pomelo_pool_in_use(base->dispatcher_pool);
    statistic->senders = pomelo_pool_in_use(base->sender_pool);
    statistic->receivers = pomelo_pool_in_use(base->receiver_pool);
    statistic->endpoints = pomelo_pool_in_use(base->endpoint_pool);
    statistic->buses = pomelo_pool_in_use(base->bus_pool);
    statistic->receptions = pomelo_pool_in_use(base->reception_pool);
    statistic->transmissions = pomelo_pool_in_use(base->transmission_pool);
    statistic->parcels = pomelo_pool_in_use(context->parcel_pool);
    statistic->heartbeats = pomelo_pool_in_use(base->heartbeat_pool);
}


/* -------------------------------------------------------------------------- */
/*                            Shared context APIs                             */
/* -------------------------------------------------------------------------- */

pomelo_delivery_context_t * pomelo_delivery_context_shared_create(
    pomelo_delivery_context_shared_options_t * options
) {
    assert(options != NULL);
    if (!options->origin_context) {
        return NULL; // No global context is provided
    }

    pomelo_allocator_t * allocator = options->allocator;
    pomelo_delivery_context_shared_t * context = pomelo_allocator_malloc_t(
        allocator,
        pomelo_delivery_context_shared_t
    );
    if (!context) return NULL; // Failed to allocate new context

    memset(context, 0, sizeof(pomelo_delivery_context_shared_t));
    pomelo_delivery_context_root_t * root = options->origin_context->root;
    
    // Create local-thread buffer context
    pomelo_buffer_context_shared_options_t buffer_options = {
        .allocator = allocator,
        .context = options->origin_context->buffer_context
    };
    context->buffer_context =
        pomelo_buffer_context_shared_create(&buffer_options);
    if (!context->buffer_context) {
        // Failed to create local-thread buffer context
        pomelo_delivery_context_shared_destroy(context);
        return NULL;
    }
    
    // Create shared pool of fragments
    pomelo_pool_shared_options_t pool_options;

    // Create shared pool of parcels
    memset(&pool_options, 0, sizeof(pomelo_pool_shared_options_t));
    pool_options.allocator = allocator;
    pool_options.buffers =
        POMELO_DELIVERY_PARCELS_POOL_BUFFER_SHARED_BUFFER_SIZE;
    pool_options.origin_pool = root->parcel_pool;

    context->parcel_pool = pomelo_pool_shared_create(&pool_options);
    if (!context->parcel_pool) {
        // Failed to create shared parcels pool
        pomelo_delivery_context_shared_destroy(context);
        return NULL;
    }

    // Initialize the interface
    pomelo_delivery_context_t * base = &context->base;
    base->allocator = allocator;
    base->buffer_context = context->buffer_context;
    base->acquire_parcel = (pomelo_delivery_context_acquire_parcel_fn)
        pomelo_delivery_context_shared_acquire_parcel;
    base->release_parcel = (pomelo_delivery_context_release_parcel_fn)
        pomelo_delivery_context_shared_release_parcel;
    base->statistic = (pomelo_delivery_context_statistic_fn)
        pomelo_delivery_context_shared_statistic;
    base->root = root;
    base->fragment_content_capacity = root->base.fragment_content_capacity;
    base->max_fragments = root->base.max_fragments;
    base->dispatcher_pool = root->base.dispatcher_pool;
    base->sender_pool = root->base.sender_pool;
    base->receiver_pool = root->base.receiver_pool;
    base->endpoint_pool = root->base.endpoint_pool;
    base->bus_pool = root->base.bus_pool;
    base->transmission_pool = root->base.transmission_pool;
    base->reception_pool = root->base.reception_pool;
    base->heartbeat_pool = root->base.heartbeat_pool;

    return base;
}


void pomelo_delivery_context_shared_destroy(
    pomelo_delivery_context_shared_t * context
) {
    assert(context != NULL);

    if (context->buffer_context) {
        pomelo_buffer_context_destroy(context->buffer_context);
        context->buffer_context = NULL;
    }

    if (context->parcel_pool) {
        pomelo_pool_destroy(context->parcel_pool);
        context->parcel_pool = NULL;
    }

    pomelo_allocator_free(context->base.allocator, context);
}


pomelo_delivery_parcel_t * pomelo_delivery_context_shared_acquire_parcel(
    pomelo_delivery_context_shared_t * context
) {
    assert(context != NULL);

    pomelo_delivery_parcel_t * parcel =
        pomelo_pool_acquire(context->parcel_pool, NULL);
    if (!parcel) return NULL; // Failed to acquire 

    parcel->context = &context->base;
    return parcel;
}


void pomelo_delivery_context_shared_release_parcel(
    pomelo_delivery_context_shared_t * context,
    pomelo_delivery_parcel_t * parcel
) {
    assert(context != NULL);
    pomelo_pool_release(context->parcel_pool, parcel);
}


/// @brief Get the statistic of delivery context
void pomelo_delivery_context_shared_statistic(
    pomelo_delivery_context_shared_t * context,
    pomelo_statistic_delivery_t * statistic
) {
    assert(context != NULL);
    pomelo_delivery_context_root_statistic(context->base.root, statistic);
}
