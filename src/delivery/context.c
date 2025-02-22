#include <assert.h>
#include <string.h>
#include "context.h"
#include "delivery.h"
#include "parcel.h"


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

void pomelo_delivery_context_root_options_init(
    pomelo_delivery_context_root_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_delivery_context_root_options_t));
    options->max_fragments = POMELO_DELIVERY_PARCEL_MAX_FRAGMENTS_DEFAULT;
}


pomelo_delivery_context_root_t * pomelo_delivery_context_root_create(
    pomelo_delivery_context_root_options_t * options
) {
    assert(options != NULL);
    if (options->fragment_capacity <= POMELO_MAX_FRAGMENT_META_DATA_BYTES) {
        // Not enough capacity for fragment
        return NULL;
    }

    size_t max_fragments = options->max_fragments;
    if (max_fragments == 0 ||
        max_fragments > POMELO_DELIVERY_PARCEL_MAX_FRAGMENTS) {
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

    context->allocator = allocator;
    context->buffer_context = options->buffer_context;

    pomelo_pool_options_t pool_options;

    // Create pool of fragments
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_fragment_t);
    pool_options.zero_initialized = true;
#ifdef POMELO_MULTI_THREAD
    pool_options.synchronized = true;
#endif // !POMELO_MULTI_THREAD
    
    context->fragment_pool = pomelo_pool_create(&pool_options);
    if (!context->fragment_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    // Create pool of parcels (synchronized)
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_parcel_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_delivery_parcel_alloc;
    pool_options.deallocate_callback = (pomelo_pool_callback)
        pomelo_delivery_parcel_dealloc;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_delivery_parcel_init;
    pool_options.release_callback = (pomelo_pool_callback)
        pomelo_delivery_parcel_finalize;
    pool_options.callback_context = context;

#ifdef POMELO_MULTI_THREAD
    pool_options.synchronized = true;
#endif // !POMELO_MULTI_THREAD

    context->parcel_pool = pomelo_pool_create(&pool_options);
    if (!context->parcel_pool) {
        pomelo_delivery_context_root_destroy(context);
        return NULL;
    }

    // Initialize the interface
    pomelo_delivery_context_t * base = &context->base;
    base->buffer_context = &context->buffer_context->base;
    base->acquire_fragment = (pomelo_delivery_context_acquire_fragment_fn)
        pomelo_delivery_context_root_acquire_fragment;
    base->release_fragment = (pomelo_delivery_context_release_fragment_fn)
        pomelo_delivery_context_root_release_fragment;
    base->acquire_parcel = (pomelo_delivery_context_acquire_parcel_fn)
        pomelo_delivery_context_root_acquire_parcel;
    base->release_parcel = (pomelo_delivery_context_release_parcel_fn)
        pomelo_delivery_context_root_release_parcel;
    base->statistic = (pomelo_delivery_context_statistic_fn)
        pomelo_delivery_context_root_statistic;

    // Update the payload capacity (exclude the meta)
    base->fragment_payload_capacity =
        options->fragment_capacity - POMELO_MAX_FRAGMENT_META_DATA_BYTES;

    base->max_fragments = max_fragments;

    return context;
}


/// @brief Destroy a delivery context
void pomelo_delivery_context_root_destroy(pomelo_delivery_context_root_t * context) {
    assert(context != NULL);

    if (context->fragment_pool) {
        pomelo_pool_destroy(context->fragment_pool);
        context->fragment_pool = NULL;
    }

    if (context->parcel_pool) {
        pomelo_pool_destroy(context->parcel_pool);
        context->parcel_pool = NULL;
    }

    pomelo_allocator_free(context->allocator, context);
}


pomelo_delivery_parcel_t * pomelo_delivery_context_root_acquire_parcel(
    pomelo_delivery_context_root_t * context
) {
    assert(context != NULL);

    pomelo_delivery_parcel_t * parcel =
        pomelo_pool_acquire(context->parcel_pool);
    if (!parcel) {
        return NULL;
    }

    parcel->context = &context->base;
    return parcel;
}


void pomelo_delivery_context_root_release_parcel(
    pomelo_delivery_context_root_t * context,
    pomelo_delivery_parcel_t * parcel
) {
    assert(context != NULL);
    assert(parcel != NULL);

    pomelo_unrolled_list_t * fragments = parcel->fragments;
    assert(fragments != NULL);

    pomelo_unrolled_list_iterator_t it;
    pomelo_delivery_fragment_t * fragment;
    
    pomelo_unrolled_list_begin(fragments, &it);
    while (pomelo_unrolled_list_iterator_next(&it, &fragment)) {
        if (!fragment) {
            // The fragment has not been arrived
            continue;
        }

        pomelo_delivery_context_root_release_fragment(context, fragment);
    }

    // Clear the list and return it to pool
    pomelo_unrolled_list_clear(fragments);
    pomelo_pool_release(context->parcel_pool, parcel);
}


void pomelo_delivery_context_root_statistic(
    pomelo_delivery_context_root_t * context,
    pomelo_statistic_delivery_t * statistic
) {
    assert(context != NULL);
    assert(statistic != NULL);

    statistic->fragments = pomelo_pool_in_use(context->fragment_pool);
    statistic->parcels = pomelo_pool_in_use(context->parcel_pool);
}



pomelo_delivery_fragment_t * pomelo_delivery_context_root_acquire_fragment(
    pomelo_delivery_context_root_t * context,
    pomelo_buffer_t * buffer
) {
    assert(context != NULL);

    if (buffer) {
        int ret = pomelo_buffer_ref(buffer);
        if (ret < 0) {
            return NULL;
        }
    } else {
        buffer = pomelo_buffer_context_root_acquire(context->buffer_context);
        if (!buffer) {
            return NULL; // Failed to acquire new buffer
        }
    }

    pomelo_delivery_fragment_t * fragment =
        pomelo_pool_acquire(context->fragment_pool);
    if (!fragment) {
        pomelo_buffer_unref(buffer);
        return NULL;
    }

    // Use current buffer
    fragment->buffer = buffer;
    fragment->payload.data = buffer->data;
    fragment->payload.position = 0;
    fragment->payload.capacity = context->base.fragment_payload_capacity;

    return fragment;
}


void pomelo_delivery_context_root_release_fragment(
    pomelo_delivery_context_root_t * context,
    pomelo_delivery_fragment_t * fragment
) {
    assert(context != NULL);
    assert(fragment != NULL);

    if (fragment->buffer) {
        pomelo_buffer_unref(fragment->buffer);
        fragment->buffer = NULL;
    }

    pomelo_pool_release(context->fragment_pool, fragment);
}



pomelo_delivery_fragment_t * pomelo_delivery_context_clone_writing_fragment(
    pomelo_delivery_context_t * context,
    pomelo_delivery_fragment_t * fragment
) {
    assert(context != NULL);
    assert(fragment != NULL);

    // New buffer will be acquired as well
    pomelo_delivery_fragment_t * cloned_fragment =
        context->acquire_fragment(context, NULL);

    if (!cloned_fragment) {
        return NULL;
    }

    // Update the fragment information but buffer
    cloned_fragment->index = fragment->index;
    cloned_fragment->payload.capacity = fragment->payload.capacity;
    cloned_fragment->payload.position = fragment->payload.position;
    memcpy(
        cloned_fragment->payload.data,
        fragment->payload.data,
        fragment->payload.position
    );
    cloned_fragment->acked = fragment->acked;

    return cloned_fragment;
}

/* -------------------------------------------------------------------------- */
/*                         Local-thread context APIs                          */
/* -------------------------------------------------------------------------- */

#ifdef POMELO_MULTI_THREAD

void pomelo_delivery_context_shared_options_init(
    pomelo_delivery_context_shared_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_delivery_context_shared_options_t));
}


pomelo_delivery_context_shared_t *
pomelo_delivery_context_shared_create(
    pomelo_delivery_context_shared_options_t * options
) {
    assert(options != NULL);
    if (!options->context) {
        // No global context is provided
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    pomelo_delivery_context_shared_t * context =
        pomelo_allocator_malloc_t(
            allocator,
            pomelo_delivery_context_shared_t
        );
    if (!context) {
        // Failed to allocate new context
        return NULL;
    }

    memset(context, 0, sizeof(pomelo_delivery_context_shared_t));
    context->allocator = allocator;
    context->root_context = options->context;
    
    // Create local-thread buffer context
    pomelo_buffer_context_shared_options_t buffer_options;
    pomelo_buffer_context_shared_options_init(&buffer_options);
    buffer_options.allocator = allocator;
    buffer_options.context = options->context->buffer_context;
    context->buffer_context =
        pomelo_buffer_context_shared_create(&buffer_options);
    if (!context->buffer_context) {
        // Failed to create local-thread buffer context
        pomelo_delivery_context_shared_destroy(context);
        return NULL;
    }
    
    // Create shared pool of fragments
    pomelo_shared_pool_options_t pool_options;
    pomelo_shared_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.buffers =
        POMELO_DELIVERY_FRAGMENTS_POOL_BUFFER_SHARED_BUFFER_SIZE;
    pool_options.master_pool = options->context->fragment_pool;

    context->fragment_pool = pomelo_shared_pool_create(&pool_options);
    if (!context->fragment_pool) {
        // Failed to create shared fragments pool
        pomelo_delivery_context_shared_destroy(context);
        return NULL;
    }

    // Create shared pool of parcels
    pomelo_shared_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.buffers =
        POMELO_DELIVERY_PARCELS_POOL_BUFFER_SHARED_BUFFER_SIZE;
    pool_options.master_pool = options->context->parcel_pool;

    context->parcel_pool = pomelo_shared_pool_create(&pool_options);
    if (!context->parcel_pool) {
        // Failed to create shared parcels pool
        pomelo_delivery_context_shared_destroy(context);
        return NULL;
    }

    // Initialize the interface
    pomelo_delivery_context_t * base = &context->base;
    base->buffer_context = &context->buffer_context->base;
    base->acquire_fragment = (pomelo_delivery_context_acquire_fragment_fn)
        pomelo_delivery_context_shared_acquire_fragment;
    base->release_fragment = (pomelo_delivery_context_release_fragment_fn)
        pomelo_delivery_context_shared_release_fragment;
    base->acquire_parcel = (pomelo_delivery_context_acquire_parcel_fn)
        pomelo_delivery_context_shared_acquire_parcel;
    base->release_parcel = (pomelo_delivery_context_release_parcel_fn)
        pomelo_delivery_context_shared_release_parcel;
    base->statistic = (pomelo_delivery_context_statistic_fn)
        pomelo_delivery_context_shared_statistic;
    base->fragment_payload_capacity =
        context->root_context->base.fragment_payload_capacity;
    base->max_fragments = context->root_context->base.max_fragments;

    return context;
}


void pomelo_delivery_context_shared_destroy(
    pomelo_delivery_context_shared_t * context
) {
    assert(context != NULL);

    if (context->parcel_pool) {
        pomelo_shared_pool_destroy(context->parcel_pool);
        context->parcel_pool = NULL;
    }

    if (context->fragment_pool) {
        pomelo_shared_pool_destroy(context->fragment_pool);
        context->fragment_pool = NULL;
    }

    if (context->buffer_context) {
        pomelo_buffer_context_shared_destroy(context->buffer_context);
        context->buffer_context = NULL;
    }

    pomelo_allocator_free(context->allocator, context);
}


pomelo_delivery_fragment_t * pomelo_delivery_context_shared_acquire_fragment(
    pomelo_delivery_context_shared_t * context,
    pomelo_buffer_t * buffer
) {
    assert(context != NULL);

    if (buffer) {
        int ret = pomelo_buffer_ref(buffer);
        if (ret < 0) {
            return NULL;
        }
    } else {
        buffer = pomelo_buffer_context_shared_acquire(context->buffer_context);
        if (!buffer) {
            return NULL; // Failed to acquire new buffer
        }
    }

    pomelo_delivery_fragment_t * fragment =
        pomelo_shared_pool_acquire(context->fragment_pool);
    if (!fragment) {
        pomelo_buffer_unref(buffer);
        return NULL;
    }

    fragment->buffer = buffer;
    fragment->payload.data = buffer->data;
    fragment->payload.position = 0;
    fragment->payload.capacity = context->base.fragment_payload_capacity;

    return fragment;
}


void pomelo_delivery_context_shared_release_fragment(
    pomelo_delivery_context_shared_t * context,
    pomelo_delivery_fragment_t * fragment
) {
    assert(context != NULL);
    assert(fragment != NULL);

    if (fragment->buffer) {
        pomelo_buffer_unref(fragment->buffer);
        fragment->buffer = NULL;
    }

    pomelo_shared_pool_release(context->fragment_pool, fragment);
}


pomelo_delivery_parcel_t * pomelo_delivery_context_shared_acquire_parcel(
    pomelo_delivery_context_shared_t * context
) {
    assert(context != NULL);

    pomelo_delivery_parcel_t * parcel =
        pomelo_shared_pool_acquire(context->parcel_pool);
    if (!parcel) {
        return NULL;
    }

    parcel->context = &context->base;
    return parcel;
}


void pomelo_delivery_context_shared_release_parcel(
    pomelo_delivery_context_shared_t * context,
    pomelo_delivery_parcel_t * parcel
) {
    assert(context != NULL);
    assert(parcel != NULL);

    pomelo_unrolled_list_t * fragments = parcel->fragments;
    if (fragments) {
        pomelo_unrolled_list_iterator_t it;
        pomelo_delivery_fragment_t * fragment;
        
        pomelo_unrolled_list_begin(fragments, &it);

        while (pomelo_unrolled_list_iterator_next(&it, &fragment)) {
            if (!fragment) {
                // The fragment has not been arrived
                continue;
            }

            pomelo_delivery_context_shared_release_fragment(context, fragment);
        }

        // Clear the list and return it to pool
        pomelo_unrolled_list_clear(fragments);
    }

    pomelo_shared_pool_release(context->parcel_pool, parcel);
}


/// @brief Get the statistic of delivery context
void pomelo_delivery_context_shared_statistic(
    pomelo_delivery_context_shared_t * context,
    pomelo_statistic_delivery_t * statistic
) {
    assert(context != NULL);
    pomelo_delivery_context_root_statistic(context->root_context, statistic);
}

#endif // ~POMELO_MULTI_THREAD
