#include <assert.h>
#include <string.h>
#include "buffer.h"


/* Check reference of buffer, only available in debug mode */
#ifndef NDEBUG
#define pomelo_buffer_check_alive(buffer)                                      \
    assert(pomelo_reference_ref_count(&(buffer)->ref) > 0)
#else
#define pomelo_buffer_check_alive(buffer) (void) (buffer)
#endif

#define POMELO_BUFFER_CONTEXT_SHARED_BUFFER_DEFAULT_SIZE 128


/* -------------------------------------------------------------------------- */
/*                                 Common APIs                                */
/* -------------------------------------------------------------------------- */


void pomelo_buffer_context_destroy(pomelo_buffer_context_t * context) {
    assert(context != NULL);

    if ((pomelo_buffer_context_t *) (context->root) == context) {
        pomelo_buffer_context_root_destroy(
            (pomelo_buffer_context_root_t *) context
        );
    } else {
        pomelo_buffer_context_shared_destroy(
            (pomelo_buffer_context_shared_t *) context
        );
    }
}


pomelo_buffer_t * pomelo_buffer_context_acquire(
    pomelo_buffer_context_t * context
) {
    assert(context != NULL);
    return context->acquire(context);
}


void pomelo_buffer_context_statistic(
    pomelo_buffer_context_t * context,
    pomelo_statistic_buffer_t * statistic
) {
    assert(context != NULL);
    assert(statistic != NULL);
    context->statistic(context, statistic);
}


/* -------------------------------------------------------------------------- */
/*                             Root context APIs                              */
/* -------------------------------------------------------------------------- */

pomelo_buffer_context_t * pomelo_buffer_context_root_create(
    pomelo_buffer_context_root_options_t * options
) {
    assert(options != NULL);
    if (options->buffer_capacity == 0) {
        // Buffer capacity is not valid
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_buffer_context_root_t * context =
        pomelo_allocator_malloc_t(allocator, pomelo_buffer_context_root_t);
    if (!context) {
        // Failed to allocate data pool
        return NULL;
    }

    memset(context, 0, sizeof(pomelo_buffer_context_root_t));
    context->allocator = allocator;
    context->buffer_capacity = options->buffer_capacity;

    // Set the interface functions
    pomelo_buffer_context_t * base = &context->base;
    base->root = context;
    base->acquire = (pomelo_buffer_context_acquire_fn)
        pomelo_buffer_context_root_acquire;
    base->release = (pomelo_buffer_context_release_fn)
        pomelo_buffer_context_root_release;
    base->statistic = (pomelo_buffer_context_statistic_fn)
        pomelo_buffer_context_root_statistic;

    // Create buffer pool
    pomelo_pool_root_options_t pool_options = {
        .allocator = allocator,
        .alloc_data = context,
        .element_size = sizeof(pomelo_buffer_t) + context->buffer_capacity,
        .on_alloc = (pomelo_pool_alloc_cb) pomelo_buffer_on_alloc,
        .on_init = (pomelo_pool_init_cb) pomelo_buffer_init,
        .synchronized = options->synchronized
    };
    context->buffer_pool = pomelo_pool_root_create(&pool_options);
    if (!context->buffer_pool) {
        // Failed to create buffer pool
        pomelo_buffer_context_root_destroy(context);
        return NULL;
    }

    return base;
}


void pomelo_buffer_context_root_destroy(
    pomelo_buffer_context_root_t * context
) {
    assert(context != NULL);

    if (context->buffer_pool) {
        pomelo_pool_destroy(context->buffer_pool);
        context->buffer_pool = NULL;
    }

    pomelo_allocator_free(context->allocator, context);
}


pomelo_buffer_t * pomelo_buffer_context_root_acquire(
    pomelo_buffer_context_root_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->buffer_pool, context);
}


void pomelo_buffer_context_root_release(
    pomelo_buffer_context_root_t * context,
    pomelo_buffer_t * buffer
) {
    assert(context != NULL);
    assert(buffer != NULL);

    // Release the buffer to pool
    pomelo_pool_release(context->buffer_pool, buffer);
}


void pomelo_buffer_context_root_statistic(
    pomelo_buffer_context_root_t * context,
    pomelo_statistic_buffer_t * statistic
) {
    assert(context != NULL);
    assert(statistic != NULL);

    statistic->buffers = pomelo_pool_in_use(context->buffer_pool);
}

/* -------------------------------------------------------------------------- */
/*                        Shared buffer context APIs                          */
/* -------------------------------------------------------------------------- */


pomelo_buffer_context_t * pomelo_buffer_context_shared_create(
    pomelo_buffer_context_shared_options_t * options
) {
    assert(options != NULL);
    if (!options->context || options->buffer_size <= 0) {
        return NULL;
    }

    size_t buffer_size = options->buffer_size;
    if (buffer_size == 0) {
        buffer_size = POMELO_BUFFER_CONTEXT_SHARED_BUFFER_DEFAULT_SIZE;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_buffer_context_shared_t * context = pomelo_allocator_malloc_t(
        allocator,
        pomelo_buffer_context_shared_t
    );

    if (!context) {
        // Failed to allocate new context
        return NULL;
    }

    memset(context, 0, sizeof(pomelo_buffer_context_shared_t));
    context->allocator = allocator;

    // Setup interface
    pomelo_buffer_context_t * base = &context->base;
    base->root = options->context->root;
    base->acquire = (pomelo_buffer_context_acquire_fn)
        pomelo_buffer_context_shared_acquire;
    base->release = (pomelo_buffer_context_release_fn)
        pomelo_buffer_context_shared_release;
    base->statistic = (pomelo_buffer_context_statistic_fn)
        pomelo_buffer_context_shared_statistic;

    // Create shared buffer pool from master context
    pomelo_pool_shared_options_t pool_options = {
        .allocator = allocator,
        .buffers = buffer_size,
        .origin_pool = options->context->root->buffer_pool
    };
    context->buffer_pool = pomelo_pool_shared_create(&pool_options);
    if (!context->buffer_pool) {
        pomelo_buffer_context_shared_destroy(context);
        return NULL;
    }

    return base;
}


void pomelo_buffer_context_shared_destroy(
    pomelo_buffer_context_shared_t * context
) {
    assert(context != NULL);

    if (context->buffer_pool) {
        pomelo_pool_destroy(context->buffer_pool);
        context->buffer_pool = NULL;
    }

    pomelo_allocator_free(context->allocator, context);
}


pomelo_buffer_t * pomelo_buffer_context_shared_acquire(
    pomelo_buffer_context_shared_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->buffer_pool, context);
}


void pomelo_buffer_context_shared_release(
    pomelo_buffer_context_shared_t * context,
    pomelo_buffer_t * buffer
) {
    assert(context != NULL);
    assert(buffer != NULL);
    pomelo_pool_release(context->buffer_pool, buffer);
}


void pomelo_buffer_context_shared_statistic(
    pomelo_buffer_context_shared_t * context,
    pomelo_statistic_buffer_t * statistic
) {
    assert(context != NULL);
    pomelo_buffer_context_root_statistic(context->base.root, statistic);
}


/* -------------------------------------------------------------------------- */
/*                               Buffer APIs                                  */
/* -------------------------------------------------------------------------- */

bool pomelo_buffer_ref(pomelo_buffer_t * buffer) {
    assert(buffer != NULL);
    return pomelo_reference_ref(&buffer->ref);
}


void pomelo_buffer_unref(pomelo_buffer_t * buffer) {
    assert(buffer != NULL);
    pomelo_reference_unref(&buffer->ref);
}



/// @brief Change the buffer context
void pomelo_buffer_set_context(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_t * context
) {
    assert(buffer != NULL);
    pomelo_buffer_check_alive(buffer);
    buffer->context = context;
}


pomelo_buffer_t * pomelo_buffer_wrap(
    void * data,
    size_t capacity,
    pomelo_buffer_finalize_fn finalize_fn
) {
    assert(data != NULL);
    if (capacity < sizeof(pomelo_buffer_t)) return NULL; // Not enough memory

    pomelo_buffer_t * buffer = (pomelo_buffer_t *) data;
    buffer->data = (uint8_t *) (buffer + 1);
    buffer->capacity = capacity - sizeof(pomelo_buffer_t);
    buffer->context = NULL;

    pomelo_reference_init(&buffer->ref, (pomelo_ref_finalize_cb) finalize_fn);
    return buffer;
}


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

int pomelo_buffer_on_alloc(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_root_t * context
) {
    assert(buffer != NULL);
    assert(context != NULL);

    // Data is always at the end of buffer header
    buffer->data = (uint8_t *) (buffer + 1);
    buffer->capacity = context->buffer_capacity;

    return 0;
}


int pomelo_buffer_init(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_t * context
) {
    assert(buffer != NULL);
    buffer->context = context;

    // Initialize reference
    pomelo_reference_init(
        &buffer->ref,
        (pomelo_ref_finalize_cb) pomelo_buffer_on_finalize
    );
    return 0;
}


void pomelo_buffer_on_finalize(pomelo_buffer_t * buffer) {
    assert(buffer != NULL);
    pomelo_buffer_context_t * context = buffer->context;
    context->release(context, buffer);
}
