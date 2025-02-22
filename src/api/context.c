#include <assert.h>
#include <string.h>
#include "context.h"
#include "message.h"
#include "base/ref.h"
#include "delivery/parcel.h"
#include "delivery/context.h"
#include "plugin/manager.h"
#include "utils/macro.h"
#include "codec/codec.h"


/// The capacity of fragment body
#define POMELO_FRAGMENT_BODY_CAPACITY                                          \
    (POMELO_PACKET_PAYLOAD_BODY_CAPACITY_DEFAULT - POMELO_MAX_FRAGMENT_META_DATA_BYTES)

/* -------------------------------------------------------------------------- */
/*                             Root context APIs                              */
/* -------------------------------------------------------------------------- */


void pomelo_context_root_options_init(pomelo_context_root_options_t * options) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_context_root_options_t));
    options->message_capacity = POMELO_MESSAGE_DEFAULT_CAPACITY;
}


pomelo_context_t * pomelo_context_root_create(
    pomelo_context_root_options_t * options
) {
    assert(options != NULL);
    if (pomelo_codec_init() < 0) {
        return NULL; // Failed to initialize codec
    }

    pomelo_allocator_t * allocator = options->allocator;

    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    if (options->message_capacity == 0) {
        // Invalid message capacity
        return NULL;
    }

    pomelo_context_root_t * context =
        pomelo_allocator_malloc_t(allocator, pomelo_context_root_t);
    if (!context) {
        return NULL;
    }
    memset(context, 0, sizeof(pomelo_context_root_t));

    context->allocator = allocator;

    // Create buffer context
    pomelo_buffer_context_root_options_t buffer_context_options;
    pomelo_buffer_context_root_options_init(&buffer_context_options);
    buffer_context_options.allocator = allocator;
    buffer_context_options.buffer_capacity = POMELO_PACKET_BUFFER_CAPACITY_DEFAULT;

    context->buffer_context =
        pomelo_buffer_context_root_create(&buffer_context_options);
    if (!context->buffer_context) {
        pomelo_context_root_destroy(context);
        return NULL;
    }

    // Create delivery context
    pomelo_delivery_context_root_options_t delivery_context_options;
    pomelo_delivery_context_root_options_init(&delivery_context_options);
    delivery_context_options.allocator = allocator;
    delivery_context_options.buffer_context = context->buffer_context;
    delivery_context_options.fragment_capacity =
        POMELO_PACKET_PAYLOAD_BODY_CAPACITY_DEFAULT;
    delivery_context_options.max_fragments = POMELO_CEIL_DIV(
        options->message_capacity,
        POMELO_FRAGMENT_BODY_CAPACITY
    );

    context->delivery_context =
        pomelo_delivery_context_root_create(&delivery_context_options);
    if (!context->delivery_context) {
        pomelo_context_root_destroy(context);
        return NULL;
    }

    // Create protocol context
    pomelo_protocol_context_root_options_t protocol_context_options;
    pomelo_protocol_context_root_options_init(&protocol_context_options);
    protocol_context_options.allocator = allocator;
    protocol_context_options.buffer_context = context->buffer_context;
    protocol_context_options.packet_capacity = POMELO_PACKET_BUFFER_CAPACITY_DEFAULT;

    context->protocol_context =
        pomelo_protocol_context_root_create(&protocol_context_options);
    if (!context->protocol_context) {
        pomelo_context_root_destroy(context);
        return NULL;
    }

    // Create message pool
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);

    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_message_t);
    pool_options.zero_initialized = true;

#ifdef POMELO_MULTI_THREAD
    pool_options.synchronized = true;
#endif // !POMELO_MULTI_THREAD

    context->message_pool = pomelo_pool_create(&pool_options);
    if (!context->message_pool) {
        pomelo_context_root_destroy(context);
        return NULL;
    }

    // Setup the plugin manager
    pomelo_plugin_manager_options_t plugin_manager_options;
    pomelo_plugin_manager_options_init(&plugin_manager_options);
    plugin_manager_options.allocator = allocator;

    pomelo_plugin_manager_t * plugin_manager =
        pomelo_plugin_manager_create(&plugin_manager_options);
    if (!plugin_manager) {
        pomelo_context_root_destroy(context);
        return NULL;
    }
    context->plugin_manager = plugin_manager;

    // Setup the interface
    pomelo_context_t * base = &context->base;
#ifdef POMELO_MULTI_THREAD
    base->root = context; // Root is itself
#endif
    pomelo_extra_set(base->extra, NULL);
    base->acquire_message = (pomelo_context_acquire_message_fn)
        pomelo_context_acquire_message;
    base->release_message = (pomelo_context_release_message_fn)
        pomelo_context_release_message;
    base->statistic = (pomelo_context_statistic_fn)
        pomelo_context_statistic;
    base->buffer_context = (pomelo_buffer_context_t *)
        context->buffer_context;
    base->protocol_context = (pomelo_protocol_context_t *)
        context->protocol_context;
    base->delivery_context = (pomelo_delivery_context_t *)
        context->delivery_context;
    base->plugin_manager = plugin_manager;
    base->message_capacity = options->message_capacity;

    return base;
}


void pomelo_context_root_destroy(pomelo_context_root_t * context) {
    assert(context != NULL);

    if (context->message_pool) {
        pomelo_pool_destroy(context->message_pool);
        context->message_pool = NULL;
    }

    if (context->protocol_context) {
        pomelo_protocol_context_root_destroy(context->protocol_context);
        context->protocol_context = NULL;
    }

    if (context->delivery_context) {
        pomelo_delivery_context_root_destroy(context->delivery_context);
        context->delivery_context = NULL;
    }

    if (context->buffer_context) {
        pomelo_buffer_context_root_destroy(context->buffer_context);
        context->buffer_context = NULL;
    }

    if (context->plugin_manager) {
        pomelo_plugin_manager_destroy(context->plugin_manager);
        context->plugin_manager = NULL;
    }

    pomelo_allocator_free(context->allocator, context);
}


pomelo_message_t * pomelo_context_acquire_message(pomelo_context_root_t * context) {
    assert(context != NULL);

    pomelo_message_t * message = pomelo_pool_acquire(context->message_pool);
    if (!message) {
        // Failed to allocate message
        return NULL;
    }

    // Set the message context, initialize reference
    message->context = (pomelo_context_t *) context;
    pomelo_reference_init(&message->ref, pomelo_message_reference_finalize);
    return message;
}


void pomelo_context_release_message(
    pomelo_context_root_t * context,
    pomelo_message_t * message
) {
    assert(context != NULL);
    assert(message != NULL);

    // Check the context
    if (((pomelo_context_t *) context) != message->context) {
        // Update the context of message first
        pomelo_message_set_context(
            message,
            (pomelo_context_t *) context
        );
    }

    pomelo_delivery_context_t * delivery_context =
        (pomelo_delivery_context_t *) context->delivery_context;

    if (message->parcel) {
        // Release the delivery message
        delivery_context->release_parcel(delivery_context, message->parcel);
        message->parcel = NULL;
    }

    // Reset the ref counter and release the message
    pomelo_pool_release(context->message_pool, message);
}


void pomelo_context_statistic(
    pomelo_context_root_t * context,
    pomelo_statistic_api_t * statistic
) {
    assert(context != NULL);
    assert(statistic != NULL);

    statistic->messages = pomelo_pool_in_use(context->message_pool);
}


/* -------------------------------------------------------------------------- */
/*                            Shared context APIs                             */
/* -------------------------------------------------------------------------- */

#ifdef POMELO_MULTI_THREAD

void pomelo_context_shared_options_init(
    pomelo_context_shared_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_context_shared_options_t));
}


pomelo_context_t * pomelo_context_shared_create(
    pomelo_context_shared_options_t * options
) {
    assert(options != NULL);
    if (!options->context) {
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_context_shared_t * context =
        pomelo_allocator_malloc_t(allocator, pomelo_context_shared_t);
    if (!context) {
        // Failed to allocate new context
        return NULL;
    }

    pomelo_context_root_t * root = options->context->root;
    if (!root) {
        // Provided context is root
        root = (pomelo_context_root_t *) options->context;
    }

    memset(context, 0, sizeof(pomelo_context_shared_t));
    context->allocator = allocator;

    // Create buffer context
    pomelo_buffer_context_shared_options_t buffer_context_options;
    pomelo_buffer_context_shared_options_init(&buffer_context_options);
    buffer_context_options.allocator = allocator;
    buffer_context_options.context = root->buffer_context;
    
    context->buffer_context =
        pomelo_buffer_context_shared_create(&buffer_context_options);
    if (!context->buffer_context) {
        // Failed to create new buffer context
        pomelo_context_shared_destroy(context);
        return NULL;
    }

    // Create protocol context
    pomelo_protocol_context_shared_options_t protocol_context_options;
    pomelo_protocol_context_shared_options_init(
        &protocol_context_options
    );
    protocol_context_options.allocator = allocator;
    protocol_context_options.context = root->protocol_context;
    
    context->protocol_context =
        pomelo_protocol_context_shared_create(&protocol_context_options);
    if (!context->protocol_context) {
        // Failed to create new buffer context
        pomelo_context_shared_destroy(context);
        return NULL;
    }

    // Create local-thread delivery context
    pomelo_delivery_context_shared_options_t delivery_context_options;
    pomelo_delivery_context_shared_options_init(
        &delivery_context_options
    );
    delivery_context_options.allocator = allocator;
    delivery_context_options.context = root->delivery_context;
    
    context->delivery_context = pomelo_delivery_context_shared_create(
        &delivery_context_options
    );
    if (!context->delivery_context) {
        // Failed to create new delivery context
        pomelo_context_shared_destroy(context);
        return NULL;
    }

    // Create shared messages pool
    pomelo_shared_pool_options_t pool_options;
    pomelo_shared_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.buffers =
        POMELO_API_MESSAGES_POOL_BUFFER_SHARED_BUFFER_SIZE;
    pool_options.master_pool = root->message_pool;
    
    context->message_pool = pomelo_shared_pool_create(&pool_options);
    if (!context->message_pool) {
        // Failed to create message pool
        pomelo_context_shared_destroy(context);
        return NULL;
    }

    // Setup the interface
    pomelo_context_t * base = &context->base;
    base->root = root;
    pomelo_extra_set(base->extra, NULL);
    base->acquire_message = (pomelo_context_acquire_message_fn)
        pomelo_context_shared_acquire_message;
    base->release_message = (pomelo_context_release_message_fn)
        pomelo_context_shared_release_message;
    base->statistic = (pomelo_context_statistic_fn)
        pomelo_context_shared_statistic;
    base->buffer_context = (pomelo_buffer_context_t *)
        context->buffer_context;
    base->protocol_context = (pomelo_protocol_context_t *)
        context->protocol_context;
    base->delivery_context = (pomelo_delivery_context_t *)
        context->delivery_context;
    base->plugin_manager = root->plugin_manager;

    return base;
}


void pomelo_context_shared_destroy(pomelo_context_shared_t * context) {
    assert(context != NULL);

    if (context->delivery_context) {
        pomelo_delivery_context_shared_destroy(
            context->delivery_context
        );
        context->delivery_context = NULL;
    }

    if (context->protocol_context) {
        pomelo_protocol_context_shared_destroy(context->protocol_context);
        context->protocol_context = NULL;
    }

    if (context->message_pool) {
        pomelo_shared_pool_destroy(context->message_pool);
        context->message_pool = NULL;
    }

    if (context->buffer_context) {
        pomelo_buffer_context_shared_destroy(context->buffer_context);
        context->buffer_context = NULL;
    }

    pomelo_allocator_free(context->allocator, context);
}


pomelo_message_t * pomelo_context_shared_acquire_message(
    pomelo_context_shared_t * context
) {
    assert(context != NULL);

    pomelo_message_t * message =
        pomelo_shared_pool_acquire(context->message_pool);
    if (!message) {
        // Failed to allocate message
        return NULL;
    }

    message->context = (pomelo_context_t *) context;

    // Set the ref counter as 1
    pomelo_reference_init(&message->ref, pomelo_message_reference_finalize);
    return message;
}


void pomelo_context_shared_release_message(
    pomelo_context_shared_t * context,
    pomelo_message_t * message
) {
    assert(context != NULL);
    assert(message != NULL);

    // Check the context
    if (&context->base != message->context) {
        // Update the context of message first
        pomelo_message_set_context(message, &context->base);
    }

    pomelo_delivery_context_t * delivery_context =
        &context->delivery_context->base;
    if (message->parcel) {
        // Release the delivery message
        delivery_context->release_parcel(delivery_context, message->parcel);
        message->parcel = NULL;
    }

    // Reset the ref counter and release the message
    pomelo_shared_pool_release(context->message_pool, message);
}


void pomelo_context_shared_statistic(
    pomelo_context_shared_t * context,
    pomelo_statistic_api_t * statistic
) {
    assert(context != NULL);
    pomelo_context_statistic(context->base.root, statistic);
}

#endif // !POMELO_MULTI_THREAD


void pomelo_context_destroy(pomelo_context_t * context) {
    assert(context != NULL);
#ifdef POMELO_MULTI_THREAD
    if ((pomelo_context_t *) context->root == context) {
        pomelo_context_root_destroy((pomelo_context_root_t *) context);
    } else {
        pomelo_context_shared_destroy((pomelo_context_shared_t *) context);
    }
#else // !POMELO_MULTI_THREAD
    pomelo_context_root_destroy((pomelo_context_root_t *) context);
#endif // POMELO_MULTI_THREAD
}


void pomelo_context_set_extra(pomelo_context_t * context, void * data) {
    assert(context != NULL);
    pomelo_extra_set(context->extra, data);
}


void * pomelo_context_get_extra(pomelo_context_t * context) {
    assert(context != NULL);
    return pomelo_extra_get(context->extra);
}
