#include <assert.h>
#include <string.h>
#include "context.h"
#include "socket.h"
#include "client.h"
#include "server.h"
#include "adapter.h"

/* -------------------------------------------------------------------------- */
/*                            Packet init/cleanup                             */
/* -------------------------------------------------------------------------- */

/// @brief The packet descriptor
typedef struct pomelo_protocol_packet_descriptor_s {
    /// @brief The size of the packet
    size_t size;

    /// @brief The init callback
    pomelo_pool_init_cb init;

    /// @brief The cleanup callback
    pomelo_pool_cleanup_cb cleanup;

} pomelo_protocol_packet_descriptor_t;


/// @brief Packet descriptors
static pomelo_protocol_packet_descriptor_t packet_descriptors[] = {
    [POMELO_PROTOCOL_PACKET_REQUEST] = {
        sizeof(pomelo_protocol_packet_request_t),
        (pomelo_pool_init_cb) pomelo_protocol_packet_request_init,
        (pomelo_pool_cleanup_cb) pomelo_protocol_packet_request_cleanup,
    },

    [POMELO_PROTOCOL_PACKET_DENIED] = {
        sizeof(pomelo_protocol_packet_denied_t),
        (pomelo_pool_init_cb) pomelo_protocol_packet_denied_init,
        (pomelo_pool_cleanup_cb) pomelo_protocol_packet_denied_cleanup,
    },

    [POMELO_PROTOCOL_PACKET_CHALLENGE] = {
        sizeof(pomelo_protocol_packet_challenge_t),
        (pomelo_pool_init_cb) pomelo_protocol_packet_challenge_init,
        (pomelo_pool_cleanup_cb) pomelo_protocol_packet_challenge_cleanup,
    },

    [POMELO_PROTOCOL_PACKET_RESPONSE] = {
        sizeof(pomelo_protocol_packet_response_t),
        (pomelo_pool_init_cb) pomelo_protocol_packet_response_init,
        (pomelo_pool_cleanup_cb) pomelo_protocol_packet_response_cleanup,
    },

    [POMELO_PROTOCOL_PACKET_KEEP_ALIVE] = {
        sizeof(pomelo_protocol_packet_keep_alive_t),
        (pomelo_pool_init_cb) pomelo_protocol_packet_keep_alive_init,
        (pomelo_pool_cleanup_cb) pomelo_protocol_packet_keep_alive_cleanup,
    },

    [POMELO_PROTOCOL_PACKET_PAYLOAD] = {
        sizeof(pomelo_protocol_packet_payload_t),
        (pomelo_pool_init_cb) pomelo_protocol_packet_payload_init,
        (pomelo_pool_cleanup_cb) pomelo_protocol_packet_payload_cleanup,
    },

    [POMELO_PROTOCOL_PACKET_DISCONNECT] = {
        sizeof(pomelo_protocol_packet_disconnect_t),
        (pomelo_pool_init_cb) pomelo_protocol_packet_disconnect_init,
        (pomelo_pool_cleanup_cb) pomelo_protocol_packet_disconnect_cleanup,
    }
};


/* -------------------------------------------------------------------------- */
/*                          Codec context init/cleanup                        */
/* -------------------------------------------------------------------------- */

/// @brief Initialize the codec context
static int crypto_context_init(
    pomelo_protocol_crypto_context_t * crypto_ctx,
    void * unused
) {
    (void) unused;
    return pomelo_protocol_crypto_context_init(crypto_ctx);
}


/* -------------------------------------------------------------------------- */
/*                              Public functions                              */
/* -------------------------------------------------------------------------- */

pomelo_protocol_context_t * pomelo_protocol_context_create(
    pomelo_protocol_context_options_t * options
) {
    assert(options != NULL);
    if (!options->buffer_context) return NULL; // No buffer context is provided

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_protocol_context_t * context =
        pomelo_allocator_malloc_t(allocator, pomelo_protocol_context_t);
    if (!context) return NULL;

    memset(context, 0, sizeof(pomelo_protocol_context_t));
    context->allocator = allocator;
    context->buffer_context = options->buffer_context;
    context->payload_capacity = options->payload_capacity;

    // Initialize pools
    pomelo_pool_root_options_t pool_options;
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    
    // The common allocator for all packet pools
    pool_options.allocator = allocator;
    pool_options.alloc_data = context;

    for (int i = 0; i < POMELO_PROTOCOL_PACKET_TYPE_COUNT; ++i) {
        pomelo_protocol_packet_descriptor_t * descriptor =
            &packet_descriptors[i];
        pool_options.element_size = descriptor->size;
        pool_options.on_init = descriptor->init;
        pool_options.on_cleanup = descriptor->cleanup;
        pool_options.zero_init = true;
        context->packet_pools[i] = pomelo_pool_root_create(&pool_options);
        if (!context->packet_pools[i]) {
            pomelo_protocol_context_destroy(context);
            return NULL;
        }
    }

    // Receiver pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_protocol_receiver_t);
    pool_options.zero_init = true;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_protocol_receiver_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_protocol_receiver_cleanup;
    context->receiver_pool = pomelo_pool_root_create(&pool_options);
    if (!context->receiver_pool) {
        pomelo_protocol_context_destroy(context);
        return NULL;
    }

    // Sender pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_protocol_sender_t);
    pool_options.zero_init = true;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_protocol_sender_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_protocol_sender_cleanup;
    context->sender_pool = pomelo_pool_root_create(&pool_options);
    if (!context->sender_pool) {
        pomelo_protocol_context_destroy(context);
        return NULL;
    }

    // Peers pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_protocol_peer_t);
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_protocol_peer_on_alloc;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_protocol_peer_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_protocol_peer_cleanup;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_protocol_peer_on_free;
    pool_options.alloc_data = context;
    context->peer_pool = pomelo_pool_root_create(&pool_options);
    if (!context->peer_pool) {
        pomelo_protocol_context_destroy(context);
        return NULL;
    }

    // Client pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_protocol_client_t);
    pool_options.alloc_data = context;
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_protocol_client_on_alloc;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_protocol_client_on_free;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_protocol_client_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_protocol_client_cleanup;
    context->client_pool = pomelo_pool_root_create(&pool_options);
    if (!context->client_pool) {
        pomelo_protocol_context_destroy(context);
        return NULL;
    }

    // Server pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_protocol_server_t);
    pool_options.alloc_data = context;
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_protocol_server_on_alloc;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_protocol_server_on_free;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_protocol_server_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_protocol_server_cleanup;
    context->server_pool = pomelo_pool_root_create(&pool_options);
    if (!context->server_pool) {
        pomelo_protocol_context_destroy(context);
        return NULL;
    }

    // Crypto context pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_protocol_crypto_context_t);
    pool_options.alloc_data = context;
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_protocol_crypto_context_on_alloc;
    pool_options.on_init = (pomelo_pool_init_cb) crypto_context_init;
    context->crypto_context_pool = pomelo_pool_root_create(&pool_options);
    if (!context->crypto_context_pool) {
        pomelo_protocol_context_destroy(context);
        return NULL;
    }

    // Acceptance pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_protocol_acceptance_t);
    pool_options.alloc_data = context;
    context->acceptance_pool = pomelo_pool_root_create(&pool_options);
    if (!context->acceptance_pool) {
        pomelo_protocol_context_destroy(context);
        return NULL;
    }

    return context;
}


void pomelo_protocol_context_destroy(pomelo_protocol_context_t * context) {
    assert(context != NULL);

    if (context->client_pool) {
        pomelo_pool_destroy(context->client_pool);
        context->client_pool = NULL;
    }

    if (context->server_pool) {
        pomelo_pool_destroy(context->server_pool);
        context->server_pool = NULL;
    }

    if (context->receiver_pool) {
        pomelo_pool_destroy(context->receiver_pool);
        context->receiver_pool = NULL;
    }

    if (context->sender_pool) {
        pomelo_pool_destroy(context->sender_pool);
        context->sender_pool = NULL;
    }

    if (context->peer_pool) {
        pomelo_pool_destroy(context->peer_pool);
        context->peer_pool = NULL;
    }

    if (context->crypto_context_pool) {
        pomelo_pool_destroy(context->crypto_context_pool);
        context->crypto_context_pool = NULL;
    }

    for (int i = 0; i < POMELO_PROTOCOL_PACKET_TYPE_COUNT; ++i) {
        if (context->packet_pools[i]) {
            pomelo_pool_destroy(context->packet_pools[i]);
            context->packet_pools[i] = NULL;
        }
    }

    if (context->acceptance_pool) {
        pomelo_pool_destroy(context->acceptance_pool);
        context->acceptance_pool = NULL;
    }

    // Free the context
    pomelo_allocator_free(context->allocator, context);
}


void pomelo_protocol_context_statistic(
    pomelo_protocol_context_t * context,
    pomelo_statistic_protocol_t * statistic
) {
    assert(context != NULL);
    assert(statistic != NULL);
    statistic->senders = pomelo_pool_in_use(context->sender_pool);
    statistic->receivers = pomelo_pool_in_use(context->receiver_pool);
    size_t packets = 0;
    for (int i = 0; i < POMELO_PROTOCOL_PACKET_TYPE_COUNT; ++i) {
        packets += pomelo_pool_in_use(context->packet_pools[i]);
    }
    statistic->packets = packets;
    statistic->peers = pomelo_pool_in_use(context->peer_pool);
    statistic->servers = pomelo_pool_in_use(context->server_pool);
    statistic->clients = pomelo_pool_in_use(context->client_pool);
    statistic->crypto_contexts =
        pomelo_pool_in_use(context->crypto_context_pool);
    statistic->acceptances = pomelo_pool_in_use(context->acceptance_pool);
}


void pomelo_protocol_context_release_packet(
    pomelo_protocol_context_t * context,
    pomelo_protocol_packet_t * packet
) {
    assert(context != NULL);
    assert(packet != NULL);

    pomelo_pool_release(context->packet_pools[packet->type], packet);
}


pomelo_protocol_crypto_context_t *
pomelo_protocol_context_acquire_crypto_context(
    pomelo_protocol_context_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->crypto_context_pool, NULL);
}


/// @brief Release a crypto context
void pomelo_protocol_context_release_crypto_context(
    pomelo_protocol_context_t * context,
    pomelo_protocol_crypto_context_t * crypto_ctx
) {
    assert(context != NULL);
    pomelo_pool_release(context->crypto_context_pool, crypto_ctx);
}
