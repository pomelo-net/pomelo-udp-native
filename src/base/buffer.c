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


/* -------------------------------------------------------------------------- */
/*                             Global buffer APIs                             */
/* -------------------------------------------------------------------------- */

void pomelo_buffer_context_root_options_init(
    pomelo_buffer_context_root_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_buffer_context_root_options_t));
}


pomelo_buffer_context_root_t * pomelo_buffer_context_root_create(
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
    pomelo_buffer_context_t * base =
        (pomelo_buffer_context_t *) context;
    base->acquire = (pomelo_buffer_context_acquire_fn)
        pomelo_buffer_context_root_acquire;
    base->release = (pomelo_buffer_context_release_fn)
        pomelo_buffer_context_root_release;
    base->statistic = (pomelo_buffer_context_statistic_fn)
        pomelo_buffer_context_root_statistic;

    // Create data pool (Synchronized)
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.callback_context = context;
    pool_options.element_size =
        sizeof(pomelo_buffer_t) + context->buffer_capacity;
    pool_options.allocate_callback = (pomelo_pool_callback) pomelo_buffer_alloc;
    pool_options.acquire_callback = (pomelo_pool_callback) pomelo_buffer_init;
#ifdef POMELO_MULTI_THREAD
    pool_options.synchronized = true;
#endif // !POMELO_MULTI_THREAD

    context->buffer_pool = pomelo_pool_create(&pool_options);
    if (!context->buffer_pool) {
        // Failed to create buffer pool
        pomelo_buffer_context_root_destroy(context);
        return NULL;
    }

    return context;
}


void pomelo_buffer_context_root_destroy(pomelo_buffer_context_root_t * context) {
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

    pomelo_buffer_t * buffer = pomelo_pool_acquire(context->buffer_pool);
    if (!buffer) {
        // Failed to allocate new buffer
        return NULL;
    }

    // Set the context for buffer
    buffer->context = (pomelo_buffer_context_t *) context;
    return buffer;
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
    pomelo_statistic_buffer_context_t * statistic
) {
    assert(context != NULL);
    assert(statistic != NULL);

    statistic->buffers = pomelo_pool_in_use(context->buffer_pool);
}

/* -------------------------------------------------------------------------- */
/*                        Shared buffer context APIs                          */
/* -------------------------------------------------------------------------- */

#ifdef POMELO_MULTI_THREAD

void pomelo_buffer_context_shared_options_init(
    pomelo_buffer_context_shared_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_buffer_context_shared_options_t));
    options->buffer_size = POMELO_BUFFER_CONTEXT_SHARED_BUFFER_DEFAULT_SIZE;
}


pomelo_buffer_context_shared_t * pomelo_buffer_context_shared_create(
    pomelo_buffer_context_shared_options_t * options
) {
    assert(options != NULL);
    if (!options->context || options->buffer_size <= 0) {
        return NULL;
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
    context->root_context = options->context;

    // Setup interface
    pomelo_buffer_context_t * base = &context->base;
    base->acquire = (pomelo_buffer_context_acquire_fn)
        pomelo_buffer_context_shared_acquire;
    base->release = (pomelo_buffer_context_release_fn)
        pomelo_buffer_context_shared_release;
    base->statistic = (pomelo_buffer_context_statistic_fn)
        pomelo_buffer_context_shared_statistic;

    // Create shared buffer pool from master context
    pomelo_shared_pool_options_t pool_options;
    pomelo_shared_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.buffers = options->buffer_size;
    pool_options.master_pool = options->context->buffer_pool;

    context->buffer_pool = pomelo_shared_pool_create(&pool_options);
    if (!context->buffer_pool) {
        pomelo_buffer_context_shared_destroy(context);
        return NULL;
    }

    return context;
}


void pomelo_buffer_context_shared_destroy(
    pomelo_buffer_context_shared_t * context
) {
    assert(context != NULL);

    if (context->buffer_pool) {
        pomelo_shared_pool_destroy(context->buffer_pool);
        context->buffer_pool = NULL;
    }

    pomelo_allocator_free(context->allocator, context);
}


pomelo_buffer_t * pomelo_buffer_context_shared_acquire(
    pomelo_buffer_context_shared_t * context
) {
    assert(context != NULL);

    pomelo_buffer_t * buffer = pomelo_shared_pool_acquire(context->buffer_pool);
    if (!buffer) {
        // Failed to allocate buffer
        return NULL;
    }

    // Set the context for buffer
    buffer->context = (pomelo_buffer_context_t *) context;
    return buffer;
}


/// @brief Release a buffer
void pomelo_buffer_context_shared_release(
    pomelo_buffer_context_shared_t * context,
    pomelo_buffer_t * buffer
) {
    assert(context != NULL);
    assert(buffer != NULL);

    // Release the buffer to pool
    pomelo_shared_pool_release(context->buffer_pool, buffer);
}


/// @brief Get the statistic of buffer context
void pomelo_buffer_context_shared_statistic(
    pomelo_buffer_context_shared_t * context,
    pomelo_statistic_buffer_context_t * statistic
) {
    assert(context != NULL);
    pomelo_buffer_context_root_statistic(context->root_context, statistic);
}

#endif // POMELO_MULTI_THREAD

/* -------------------------------------------------------------------------- */
/*                               Buffer APIs                                  */
/* -------------------------------------------------------------------------- */

int pomelo_buffer_ref(pomelo_buffer_t * buffer) {
    assert(buffer != NULL);
    bool result = pomelo_reference_ref(&buffer->ref);
    return result ? 0 : -1;
}


int pomelo_buffer_unref(pomelo_buffer_t * buffer) {
    assert(buffer != NULL);
    pomelo_reference_unref(&buffer->ref);
    return 0;
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


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

int pomelo_buffer_alloc(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_root_t * context
) {
    assert(buffer != NULL);
    assert(context != NULL);

    buffer->data = (uint8_t *) (buffer + 1); // Data is at the end of buffer
    buffer->capacity = context->buffer_capacity;

    return 0;
}


int pomelo_buffer_init(
    pomelo_buffer_t * buffer,
    pomelo_buffer_context_root_t * context
) {
    assert(buffer != NULL);
    (void) context;

    // Initialize reference
    pomelo_reference_init(
        &buffer->ref,
        (pomelo_ref_finalize_cb) pomelo_buffer_finalize
    );
    return 0;
}


void pomelo_buffer_finalize(pomelo_buffer_t * buffer) {
    assert(buffer != NULL);
    pomelo_buffer_context_t * context = buffer->context;
    context->release(context, buffer);
}
