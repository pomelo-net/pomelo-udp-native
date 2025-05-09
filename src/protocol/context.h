#ifndef POMELO_PROTOCOL_CONTEXT_SRC_H
#define POMELO_PROTOCOL_CONTEXT_SRC_H
#include "utils/pool.h"
#include "protocol.h"
#include "packet.h"
#ifdef __cplusplus
extern "C" {
#endif // __plusplus


struct pomelo_protocol_context_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The buffer context
    pomelo_buffer_context_t * buffer_context;

    /// @brief Capacity of payload
    size_t payload_capacity;

    /// @brief Pool of senders
    pomelo_pool_t * sender_pool;

    /// @brief Pool of receivers
    pomelo_pool_t * receiver_pool;

    /// @brief Pool of packets
    pomelo_pool_t * packet_pools[POMELO_PROTOCOL_PACKET_TYPE_COUNT];

    /// @brief Pool of peers
    pomelo_pool_t * peer_pool;

    /// @brief Pool of server sockets
    pomelo_pool_t * server_pool;

    /// @brief Pool of client sockets
    pomelo_pool_t * client_pool;

    /// @brief Pool of crypto contexts
    pomelo_pool_t * crypto_context_pool;

    /// @brief Pool of acceptance
    pomelo_pool_t * acceptance_pool;
};


/// @brief Release the packet
void pomelo_protocol_context_release_packet(
    pomelo_protocol_context_t * context,
    pomelo_protocol_packet_t * packet
);


/// @brief Acquire a crypto context
pomelo_protocol_crypto_context_t *
pomelo_protocol_context_acquire_crypto_context(
    pomelo_protocol_context_t * context
);


/// @brief Release a crypto context
void pomelo_protocol_context_release_crypto_context(
    pomelo_protocol_context_t * context,
    pomelo_protocol_crypto_context_t * crypto_ctx
);


#ifdef __cplusplus
}
#endif // __plusplus
#endif // POMELO_PROTOCOL_CONTEXT_SRC_H
