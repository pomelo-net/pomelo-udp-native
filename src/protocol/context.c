#include <assert.h>
#include <string.h>
#include "context.h"
#include "socket.h"


/// The minimum capacity of a packet
#define POMELO_PACKET_MIN_CAPACITY                                             \
    (POMELO_PACKET_RESPONSE_BODY_SIZE + POMELO_PACKET_HEADER_CAPACITY)


int pomelo_protocol_context_root_options_init(
    pomelo_protocol_context_root_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_protocol_context_root_options_t));

    return 0;
}


pomelo_protocol_context_root_t * pomelo_protocol_context_root_create(
    pomelo_protocol_context_root_options_t * options
) {
    assert(options != NULL);
    if (!options->buffer_context) {
        // No buffer context is provided
        return NULL;
    }

    if (options->packet_capacity < POMELO_PACKET_MIN_CAPACITY) {
        // Not enough capacity for packets
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_protocol_context_root_t * context =
        pomelo_allocator_malloc_t(allocator, pomelo_protocol_context_root_t);
    if (!context) {
        // Failed to allocate new protocol context
        return NULL;
    }

    memset(context, 0, sizeof(pomelo_protocol_context_root_t));
    context->allocator = allocator;
    context->buffer_context = options->buffer_context;

    // Initialize the interface
    pomelo_protocol_context_t * base = &context->base;
    base->buffer_context = (pomelo_buffer_context_t *)
        context->buffer_context;
    
    base->packet_capacity = options->packet_capacity;
    base->packet_payload_body_capacity =
        options->packet_capacity - POMELO_PACKET_HEADER_CAPACITY;

    return context;
}


void pomelo_protocol_context_root_destroy(
    pomelo_protocol_context_root_t * context
) {
    assert(context != NULL);

    // Just free the context
    pomelo_allocator_free(context->allocator, context);
}


/* -------------------------------------------------------------------------- */
/*                             Shared context APIs                            */
/* -------------------------------------------------------------------------- */

#ifdef POMELO_MULTI_THREAD

int pomelo_protocol_context_shared_options_init(
    pomelo_protocol_context_shared_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_protocol_context_shared_options_t));
    return 0;
}


pomelo_protocol_context_shared_t *
pomelo_protocol_context_shared_create(
    pomelo_protocol_context_shared_options_t * options
) {
    assert(options != NULL);
    if (!options->context) {
        // No context is provided
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_protocol_context_shared_t * context =
        pomelo_allocator_malloc_t(
            allocator,
            pomelo_protocol_context_shared_t
        );

    if (!context) {
        return NULL;
    }

    memset(context, 0, sizeof(pomelo_protocol_context_shared_t));
    context->allocator = allocator;
    
    pomelo_buffer_context_shared_options_t buffer_context_options;
    pomelo_buffer_context_shared_options_init(&buffer_context_options);
    buffer_context_options.allocator = allocator;
    buffer_context_options.context = options->context->buffer_context;

    pomelo_buffer_context_shared_t * buffer_context =
        pomelo_buffer_context_shared_create(&buffer_context_options);
    if (!buffer_context) {
        // Failed to create buffer context
        pomelo_allocator_free(allocator, context);
        return NULL;
    }

    context->buffer_context = buffer_context;
    context->root_context = options->context;

    // Initialize the interface
    pomelo_protocol_context_t * base = &context->base;
    base->buffer_context = &buffer_context->base;
    base->packet_capacity = options->context->base.packet_capacity;
    base->packet_payload_body_capacity =
        options->context->base.packet_payload_body_capacity;

    return context;
}


void pomelo_protocol_context_shared_destroy(
    pomelo_protocol_context_shared_t * context
) {
    assert(context != NULL);

    // Free the buffer context
    if (context->buffer_context) {
        pomelo_buffer_context_shared_destroy(context->buffer_context);
        context->buffer_context = NULL;
    }

    // Free itself
    pomelo_allocator_free(context->allocator, context);
}

#endif // !POMELO_MULTI_THREAD
