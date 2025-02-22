#include <assert.h>
#include <string.h>
#include "pomelo/allocator.h"
#include "base/packet.h"
#include "utils/macro.h"
#include "peer.h"
#include "socket.h"
#include "client.h"
#include "server.h"


/* -------------------------------------------------------------------------- */
/*                               Public API                                   */
/* -------------------------------------------------------------------------- */


void pomelo_protocol_socket_destroy(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);

    // Call the specific destroy function
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_destroy((pomelo_protocol_server_t *) socket);
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_destroy((pomelo_protocol_client_t *) socket);
            break;
        
        default:
            break;
    }
}


int pomelo_protocol_socket_start(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);

    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_STOPPED) {
        return -1; // The socket's still running or stopping
    }

    int ret = 0;

    // Start the socket
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            ret = pomelo_protocol_server_start(
                (pomelo_protocol_server_t *) socket
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            ret = pomelo_protocol_client_start(
                (pomelo_protocol_client_t *) socket
            );
            break;

        default:
            return -1;
    }

    if (ret == 0) {
        socket->state = POMELO_PROTOCOL_SOCKET_STATE_RUNNING;
    }

    return ret;
}


void pomelo_protocol_socket_stop(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);

    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) {
        return;
    }

    socket->state = POMELO_PROTOCOL_SOCKET_STATE_STOPPING;

    // Process stopping inherited sockets first
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_stop((pomelo_protocol_server_t *) socket);
            break;
        
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_stop((pomelo_protocol_client_t *) socket);
            break;
    }

    // Stop receiving first
    pomelo_adapter_stop(socket->adapter);

    // Cancel all works in the work group first
    pomelo_platform_cancel_task_group(
        socket->platform,
        socket->task_group,
        (pomelo_platform_task_cb) pomelo_protocol_socket_stop_deferred,
        socket
    );

    // => pomelo_protocol_socket_stop_deferred
    //   => pomelo_protocol_server_stop_deferred()
    //   => pomelo_protocol_client_stop_deferred()
}


void pomelo_protocol_socket_statistic(
    pomelo_protocol_socket_t * socket,
    pomelo_statistic_protocol_t * statistic
) {
    assert(socket != NULL);
    assert(statistic != NULL);

    size_t packets = 0;
    for (int i = 0; i < POMELO_PACKET_TYPE_COUNT; ++i) {
        packets += pomelo_pool_in_use(socket->pools.packets[i]);
    }

    statistic->packets = packets;
    statistic->recv_passes = pomelo_pool_in_use(socket->pools.recv_pass);
    statistic->send_passes = pomelo_pool_in_use(socket->pools.send_pass);

    statistic->recv_valid_bytes = socket->statistic.valid_recv_bytes;
    statistic->recv_invalid_bytes = socket->statistic.invalid_recv_bytes;

    pomelo_protocol_server_t * server = NULL;
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            // Server mode
            server = (pomelo_protocol_server_t *) socket;
            statistic->peers = pomelo_pool_in_use(server->peer_pool);
            break;
        
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            // Client mode
            statistic->peers = 
                (socket->state == POMELO_PROTOCOL_SOCKET_STATE_RUNNING);
            break;
        
        default:
            statistic->peers = 0;
    }
}


uint64_t pomelo_protocol_socket_time(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);
    int64_t offset = pomelo_atomic_int64_load(&socket->clock.offset);
    return pomelo_platform_hrtime(socket->platform) + offset;
}


/* -------------------------------------------------------------------------- */
/*                               Init & Cleanup                               */
/* -------------------------------------------------------------------------- */

int pomelo_protocol_socket_init(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_socket_options_t * options
) {
    assert(socket != NULL);
    assert(options != NULL);

    // Set everything with NULL
    memset(socket, 0, sizeof(pomelo_protocol_socket_t));

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    if (!options->platform) {
        return -1;
    }

    if (!options->context) {
        return -1;
    }

    socket->allocator = allocator;
    socket->platform = options->platform;
    socket->context = options->context;

    // Initialize adapter
    pomelo_adapter_options_t adapter_options;
    pomelo_adapter_options_init(&adapter_options);
    adapter_options.allocator = allocator;
    adapter_options.platform = options->platform;

    socket->adapter = pomelo_adapter_create(&adapter_options);
    if (!socket->adapter) {
        pomelo_protocol_socket_cleanup(socket);
        return -1; // Failed to create adapter
    }
    pomelo_adapter_set_extra(socket->adapter, socket);
    uint8_t adapter_capability = pomelo_adapter_get_capability(socket->adapter);
    if (!(adapter_capability & POMELO_ADAPTER_CAPABILITY_ENCRYPTED)) {
        // This adapter does not support encrypted packetes
        socket->no_encrypt = true;
    }

    pomelo_protocol_socket_pools_t * pools = &socket->pools;

    // Initialize pools
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);

    // The common allocator for all packet pools
    pool_options.allocator = allocator;
    pool_options.callback_context = socket;

    // Packet connection request
    pool_options.element_size = sizeof(pomelo_packet_request_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_socket_packet_request_init;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_socket_packet_request_reset;
    pools->packets[POMELO_PACKET_REQUEST] = pomelo_pool_create(&pool_options);

    // Packet connection denied
    pool_options.element_size = sizeof(pomelo_packet_denied_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_socket_packet_denied_init;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_socket_packet_denied_reset;
    pools->packets[POMELO_PACKET_DENIED] = pomelo_pool_create(&pool_options);

    // Packet connection challenge
    pool_options.element_size = sizeof(pomelo_packet_challenge_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_socket_packet_challenge_init;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_socket_packet_challenge_reset;
    pools->packets[POMELO_PACKET_CHALLENGE] = pomelo_pool_create(&pool_options);

    // Packet connection response
    pool_options.element_size = sizeof(pomelo_packet_response_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_socket_packet_response_init;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_socket_packet_response_reset;
    pools->packets[POMELO_PACKET_RESPONSE] = pomelo_pool_create(&pool_options);

    // Packet connection ping
    pool_options.element_size = sizeof(pomelo_packet_ping_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_socket_packet_ping_init;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_socket_packet_ping_reset;
    pools->packets[POMELO_PACKET_PING] = pomelo_pool_create(&pool_options);

    // Packet connection payload
    pool_options.element_size = sizeof(pomelo_packet_payload_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_socket_packet_payload_init;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_socket_packet_payload_reset;
    pools->packets[POMELO_PACKET_PAYLOAD] = pomelo_pool_create(&pool_options);

    // Packet connection disconnect
    pool_options.element_size = sizeof(pomelo_packet_disconnect_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_socket_packet_disconnect_init;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_socket_packet_disconnect_reset;
    pools->packets[POMELO_PACKET_DISCONNECT] =
        pomelo_pool_create(&pool_options);

    // Packet pong
    pool_options.element_size = sizeof(pomelo_packet_pong_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_socket_packet_pong_init;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_socket_packet_pong_reset;
    pools->packets[POMELO_PACKET_PONG] = pomelo_pool_create(&pool_options);

    for (int i = 0; i < POMELO_PACKET_TYPE_COUNT; ++i) {
        if (!pools->packets[i]) {
            pomelo_protocol_socket_cleanup(socket);
            return -1;
        }
    }

    // Receiving passes pool
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_protocol_recv_pass_t);
    pool_options.zero_initialized = true;
    pool_options.callback_context = socket;

    pools->recv_pass = pomelo_pool_create(&pool_options);

    if (!pools->recv_pass) {
        pomelo_protocol_socket_cleanup(socket);
        return -1;
    }

    // Sending passes pool
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_protocol_send_pass_t);
    pool_options.zero_initialized = true;
    pool_options.callback_context = socket;

    pools->send_pass = pomelo_pool_create(&pool_options);
    if (!pools->send_pass) {
        pomelo_protocol_socket_cleanup(socket);
        return -1;
    }

    // Create work group
    socket->task_group = pomelo_platform_acquire_task_group(socket->platform);
    if (!socket->task_group) {
        pomelo_protocol_socket_cleanup(socket);
        return -1;
    }

    // Initialize time
    pomelo_protocol_clock_init(&socket->clock, socket->platform);
    return 0;
}


void pomelo_protocol_socket_cleanup(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);

    if (socket->adapter) {
        pomelo_adapter_destroy(socket->adapter);
        socket->adapter = NULL;
    }

    pomelo_protocol_socket_pools_t * pools = &socket->pools;

    for (int i = 0; i < POMELO_PACKET_TYPE_COUNT; ++i) {
        if (pools->packets[i]) {
            pomelo_pool_destroy(pools->packets[i]);
            pools->packets[i] = NULL;
        }
    }

    if (pools->recv_pass) {
        pomelo_pool_destroy(pools->recv_pass);
        pools->recv_pass = NULL;
    }

    if (pools->send_pass) {
        pomelo_pool_destroy(pools->send_pass);
        pools->send_pass = NULL;
    }

    if (socket->task_group) {
        pomelo_platform_release_task_group(
            socket->platform,
            socket->task_group
        );
        socket->task_group = NULL;
    }

    // Do not free the socket in this cleanup function
}


/* -------------------------------------------------------------------------- */
/*                            Platform callbacks                              */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_socket_on_alloc(
    pomelo_protocol_socket_t * socket,
    pomelo_buffer_vector_t * buffer_vector
) {
    assert(socket != NULL);

    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) {
        // Socket is not running now
        return;
    }

    pomelo_protocol_context_t * context = socket->context;
    pomelo_buffer_context_t * buffer_context =
        context->buffer_context;

    // Acquire new buffer.
    pomelo_buffer_t * buffer = buffer_context->acquire(buffer_context);
    if (!buffer) {
        // Failed to acquire new buffer
        return;
    }

    buffer_vector->data = buffer->data;
    buffer_vector->length = buffer->capacity;
}


void pomelo_protocol_socket_on_recv(
    pomelo_protocol_socket_t * socket,
    pomelo_address_t * address,
    pomelo_buffer_vector_t * buffer_vector,
    int status,
    bool encrypted
) {
    // Address may be NULL
    assert(socket != NULL);
    assert(buffer_vector != NULL);
    uint64_t recv_time = pomelo_platform_hrtime(socket->platform);

    uint8_t * data = buffer_vector->data;
    if (!data) {
        return;
    }
    pomelo_buffer_t * buffer = pomelo_buffer_from_data(data);

    if (status < 0 || socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) {
        // Free allocated packet and discard
        pomelo_buffer_unref(buffer);
        return;
    }

    // The ownership of buffer will be transfered to the receiving pass in case
    // of success. Otherwise, it will be released on failure.

    int ret;
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            // The server is running
            ret = pomelo_protocol_server_on_recv(
                (pomelo_protocol_server_t *) socket,
                address,
                buffer,
                buffer_vector->length,
                recv_time,
                encrypted
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            // The client running
            ret = pomelo_protocol_client_on_recv(
                (pomelo_protocol_client_t *) socket,
                address,
                buffer,
                buffer_vector->length,
                recv_time,
                encrypted
            );
            break;
            
        default:
            ret = -1;
    }

    if (ret < 0) {
        // The packet is invalid
        socket->statistic.invalid_recv_bytes += buffer_vector->length;
        pomelo_buffer_unref(buffer);
        return;
    }

    // The packet is valid
    socket->statistic.valid_recv_bytes += buffer_vector->length;
}


void pomelo_protocol_socket_on_sent(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_send_pass_t * pass,
    int status
) {
    assert(socket != NULL);
    assert(pass != NULL);

    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) {
        pass->result = -1;
    } else {
        pass->result = status;
    }

    // Forward to sending pass callback
    pomelo_protocol_send_pass_done(pass);
}


/* -------------------------------------------------------------------------- */
/*                             Incoming packets                               */
/* -------------------------------------------------------------------------- */


void pomelo_protocol_socket_recv_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_t * packet,
    uint64_t recv_time,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    if (result == 0) {
        // Update the last receive time
        peer->last_recv_time = recv_time;
    }

    switch (packet->type) {
        case POMELO_PACKET_REQUEST:
            pomelo_protocol_socket_recv_request_packet(
                socket, peer, (pomelo_packet_request_t *) packet, result
            );
            break;
        
        case POMELO_PACKET_DENIED:
            pomelo_protocol_socket_recv_denied_packet(
                socket, peer, (pomelo_packet_denied_t *) packet, result
            );
            break;
        
        case POMELO_PACKET_CHALLENGE:
            pomelo_protocol_socket_recv_challenge_packet(
                socket, peer, (pomelo_packet_challenge_t *) packet, result
            );
            break;
        
        case POMELO_PACKET_RESPONSE:
            pomelo_protocol_socket_recv_response_packet(
                socket, peer, (pomelo_packet_response_t *) packet, result
            );
            break;
        
        case POMELO_PACKET_PING:
            pomelo_protocol_socket_recv_ping_packet(
                socket, peer, (pomelo_packet_ping_t *) packet, recv_time, result
            );
            break;
        
        case POMELO_PACKET_PAYLOAD:
            pomelo_protocol_socket_recv_payload_packet(
                socket, peer, (pomelo_packet_payload_t *) packet, result
            );
            break;
        
        case POMELO_PACKET_DISCONNECT:
            pomelo_protocol_socket_recv_disconnect_packet(
                socket, peer, (pomelo_packet_disconnect_t *) packet, result
            );
            break;
        
        case POMELO_PACKET_PONG:
            pomelo_protocol_socket_recv_pong_packet(
                socket, peer, (pomelo_packet_pong_t *) packet, recv_time, result
            );
            break;
        
        default:
            // Discard the packet
            pomelo_protocol_socket_release_packet(socket, packet);
    }
}


void pomelo_protocol_socket_recv_request_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Only server handles this packet
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_recv_request_packet(
                (pomelo_protocol_server_t *) socket, peer, packet, result
            );
            break;
        
        default:
            // Discard the packet
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_recv_denied_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_denied_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Oly client handles this packet
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_recv_denied_packet(
                (pomelo_protocol_client_t *) socket, peer, packet, result
            );
            break;

        default:
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_recv_challenge_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_challenge_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Only client handles this packet
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_recv_challenge_packet(
                (pomelo_protocol_client_t *) socket, peer, packet, result
            );
            break;

        default:
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_recv_response_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_response_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);
    
    // Only server handles this packet
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_recv_response_packet(
                (pomelo_protocol_server_t *) socket, peer, packet, result
            );
            break;

        default:
            // Discard packet
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_recv_ping_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    uint64_t recv_time,
    int result
) {
    assert(socket != NULL);

    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_recv_ping_packet(
                (pomelo_protocol_server_t *) socket,
                peer,
                packet,
                recv_time,
                result
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_recv_ping_packet(
                (pomelo_protocol_client_t *) socket,
                peer,
                packet,
                recv_time,
                result
            );
            break;

        default:
            // Unhandled state, release the packet
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_recv_payload_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_payload_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    if (result < 0) {
        // Failed to decode, release packet
        pomelo_protocol_socket_release_packet(
            socket, (pomelo_packet_t *) packet
        );
        return;
    }

    pomelo_packet_t * base = &packet->base;
    pomelo_buffer_t * buffer = base->buffer;
    pomelo_payload_t * header = &base->header;
    pomelo_payload_t * body = &base->body;

    // Calculate offset and length from header & body
    size_t offset = header->capacity + body->position;
    size_t length = body->capacity - body->position;

    // Ref the buffer to make sure the buffer will not be release in the
    // packet release function.
    pomelo_buffer_ref(buffer);

    // Release packet before calling the callback.
    pomelo_protocol_socket_release_packet(socket, base);

    // Call the callback. Buffer will be passed to transporter
    pomelo_protocol_socket_on_received(
        socket,
        peer,
        buffer,
        offset,
        length
    );

    // Decrease the buffer reference
    pomelo_buffer_unref(buffer);
}


void pomelo_protocol_socket_recv_disconnect_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_recv_disconnect_packet(
                (pomelo_protocol_server_t *) socket, peer, packet, result
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_recv_disconnect_packet(
                (pomelo_protocol_client_t *) socket, peer, packet, result
            );
            break;

        default:
            // Discard the packet
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_recv_pong_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_pong_t * packet,
    uint64_t recv_time,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Server & client won't handle this packet
    if (result < 0) {
        // Failed to decode, release packet
        pomelo_protocol_socket_release_packet(
            socket, (pomelo_packet_t *) packet
        );
        return;
    }

    // Update the last receive time
    peer->last_recv_time = pomelo_platform_hrtime(socket->platform);

    // Submit pong reply
    pomelo_protocol_peer_receive_pong(peer, packet, recv_time);

    // Finally, release the packet
    pomelo_protocol_socket_release_packet(socket, (pomelo_packet_t *) packet);
}


/* -------------------------------------------------------------------------- */
/*                            Outgoing packets                                */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_socket_sent_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) {
        // Socket is not running
        return;
    }

    switch (packet->type) {
        case POMELO_PACKET_REQUEST:
            pomelo_protocol_socket_sent_request_packet(
                socket, peer, (pomelo_packet_request_t *) packet, result
            );
            break;

        case POMELO_PACKET_DENIED:
            pomelo_protocol_socket_sent_denied_packet(
                socket, peer, (pomelo_packet_denied_t *) packet, result
            );
            break;

        case POMELO_PACKET_CHALLENGE:
            pomelo_protocol_socket_sent_challenge_packet(
                socket, peer, (pomelo_packet_challenge_t *) packet, result
            );
            break;

        case POMELO_PACKET_RESPONSE:
            pomelo_protocol_socket_sent_response_packet(
                socket, peer, (pomelo_packet_response_t *) packet, result
            );
            break;
        
        case POMELO_PACKET_PING:
            pomelo_protocol_socket_sent_ping_packet(
                socket, peer, (pomelo_packet_ping_t *) packet, result
            );
            break;

        case POMELO_PACKET_PAYLOAD:
            pomelo_protocol_socket_sent_payload_packet(
                socket, peer, (pomelo_packet_payload_t *) packet, result
            );
            break;

        case POMELO_PACKET_DISCONNECT:
            pomelo_protocol_socket_sent_disconnect_packet(
                socket, peer, (pomelo_packet_disconnect_t *) packet, result
            );
            break;

        case POMELO_PACKET_PONG:
            pomelo_protocol_socket_sent_pong_packet(
                socket, peer, (pomelo_packet_pong_t *) packet, result
            );
            break;
        
        default:
            // Release the packet in other cases
            pomelo_protocol_socket_release_packet(socket, packet);
    }
}


void pomelo_protocol_socket_sent_request_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Only client handles this packet
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_sent_request_packet(
                (pomelo_protocol_client_t *) socket, peer, packet, result
            );
            break;

        default:
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_sent_denied_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_denied_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Only server handles this callback
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_sent_denied_packet(
                (pomelo_protocol_server_t *) socket, peer, packet, result
            );
            break;

        default:
            // Release the packet
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_sent_challenge_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_challenge_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Only server handles this packet
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_sent_challenge_packet(
                (pomelo_protocol_server_t *) socket, peer, packet, result
            );
            break;
        
        default:
            // Release the packet
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_sent_response_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_response_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Only client handles this packet
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_sent_response_packet(
                (pomelo_protocol_client_t *) socket, peer, packet, result
            );
            break;
        
        default:
            // Release the packet
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_sent_ping_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Both client & server handle this packet
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_sent_ping_packet(
                (pomelo_protocol_server_t *) socket, peer, packet, result
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_sent_ping_packet(
                (pomelo_protocol_client_t *) socket, peer, packet, result
            );
            break;
        
        default:
            // Release the packet
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_sent_payload_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_payload_t * packet,
    int result
) {
    assert(socket != NULL);
    (void) peer;
    assert(packet != NULL);
    (void) result;

    // Unref the source buffer
    pomelo_buffer_unref(packet->source);

    // Release the packet
    pomelo_protocol_socket_release_packet(socket, (pomelo_packet_t *) packet);
}


void pomelo_protocol_socket_sent_disconnect_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Both client & server handle this packet
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_sent_disconnect_packet(
                (pomelo_protocol_server_t *) socket, peer, packet, result
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_sent_disconnect_packet(
                (pomelo_protocol_client_t *) socket, peer, packet, result
            );
            break;
        
        default:
            // Release the packet
            pomelo_protocol_socket_release_packet(
                socket, (pomelo_packet_t *) packet
            );
    }
}


void pomelo_protocol_socket_sent_pong_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_pong_t * packet,
    int result
) {
    assert(socket != NULL);
    (void) peer;
    assert(packet != NULL);
    (void) result;

    pomelo_protocol_socket_release_packet(socket, (pomelo_packet_t *) packet);
}


/* -------------------------------------------------------------------------- */
/*                              Common functions                              */
/* -------------------------------------------------------------------------- */


pomelo_protocol_send_pass_t * pomelo_protocol_socket_prepare_send_pass(
    pomelo_protocol_socket_t * socket,
    pomelo_packet_type type
) {
    assert(socket != NULL);
    pomelo_protocol_socket_pools_t * pools = &socket->pools;

    // Acquire a sending pass
    pomelo_protocol_send_pass_t * pass = pomelo_pool_acquire(pools->send_pass);
    if (!pass) {
        // Cannot allocate pass
        return NULL;
    }
    pass->socket = socket;

    // Acquire a packet
    pomelo_packet_t * packet = pomelo_pool_acquire(pools->packets[type]);
    if (!packet) {
        // Cannot allocate packet
        pomelo_protocol_socket_cancel_send_pass(socket, pass);
        return NULL;
    }
    pass->packet = packet;

    pomelo_protocol_context_t * context = socket->context;
    pomelo_buffer_context_t * buffer_context = context->buffer_context;

    // Acquire buffer
    pomelo_buffer_t * buffer = buffer_context->acquire(buffer_context);
    if (!buffer) {
        pomelo_protocol_socket_cancel_send_pass(socket, pass);
        return NULL;
    }

    // Attach the buffer to packet
    pomelo_packet_attach_buffer(packet, buffer);
    if (socket->no_encrypt) {
        pass->flags = POMELO_PROTOCOL_PASS_FLAG_NO_ENCRYPT;
    }

    return pass;
}


void pomelo_protocol_socket_cancel_send_pass(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_send_pass_t * pass
) {
    assert(socket != NULL);
    assert(pass != NULL);

    // Retrieve the packet and detach it from sending pass
    pomelo_packet_t * packet = pass->packet;

    // Release packet and its associated buffer
    if (packet) {
        if (packet->buffer) {
            pomelo_buffer_unref(packet->buffer);
            packet->buffer = NULL;
        }
        pomelo_pool_release(socket->pools.packets[packet->type], packet);
    }
    pass->packet = NULL;

    // Release sending pass
    pomelo_pool_release(socket->pools.send_pass, pass);
}


void pomelo_protocol_socket_release_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_packet_t * packet
) {
    assert(socket != NULL);
    assert(packet != NULL);

    // Unref buffer
    if (packet->buffer) {
        pomelo_buffer_unref(packet->buffer);
        packet->buffer = NULL;
    }
    
    // Release the packet
    pomelo_pool_release(socket->pools.packets[packet->type], packet);
}


void pomelo_protocol_socket_stop_deferred(
    pomelo_protocol_socket_t * socket
) {
    // Call from stopping socket
    assert(socket != NULL);
    // The state is stopped, resources can be cleaned up in this state

    // Change the state
    socket->state = POMELO_PROTOCOL_SOCKET_STATE_STOPPED;

    // The callback `pomelo_protocol_socket_on_stopped` will be called in
    // deferred function of each mode
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_stop_deferred(
                (pomelo_protocol_client_t *) socket
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_stop_deferred(
                (pomelo_protocol_server_t *) socket
            );
            break;
    }
}


void pomelo_protocol_socket_reply_pong(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    uint64_t ping_sequence,
    uint64_t ping_recv_time
) {
    assert(socket != NULL);
    assert(peer != NULL);

    uint64_t delta_time = peer->last_recv_ping_time - ping_recv_time;
    if (delta_time < POMELO_FREQ_TO_NS(POMELO_PONG_REPLY_FREQUENCY_HZ)) {
        return; // Too short, ignore.
    }

    pomelo_protocol_send_pass_t * pass =
        pomelo_protocol_socket_prepare_send_pass(socket, POMELO_PACKET_PONG);
    if (!pass) {
        return; // Failed to allocate the pong sending pass
    }

    peer->last_recv_ping_time = ping_recv_time;

    uint64_t now = pomelo_platform_hrtime(socket->platform);
    pomelo_packet_pong_t * packet = (pomelo_packet_pong_t *) pass->packet;
    packet->base.sequence = peer->sequence_number++;
    packet->ping_sequence = ping_sequence;
    packet->ping_recv_time = ping_recv_time;
    packet->pong_delta_time = now - ping_recv_time;

    pass->peer = peer;
    pomelo_protocol_send_pass_submit(pass);
}


int pomelo_protocol_socket_send(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(buffer != NULL);

    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) {
        return -1;
    }

    // Check length
    if (length > (socket->context->packet_payload_body_capacity)) {
        return -1; // Buffer is too large
    }

    if (length > buffer->capacity) {
        return -1; // Out of range
    }

    if (length == 0) {
        return 0; // Nothing to do
    }

    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            switch (peer->state.client) {
                case POMELO_CLIENT_CONNECTED:
                case POMELO_CLIENT_DISCONNECTING:
                    break;

                default:
                    return -1;
            }
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            switch (peer->state.server) {
                case POMELO_SERVER_CONNECTED:
                case POMELO_SERVER_DISCONNECTING:
                    break;

                case POMELO_SERVER_UNCONFIRMED:
                    // With unconfirmed session, we need to send ping
                    // packet before sending payload
                    pomelo_protocol_server_ping(
                        (pomelo_protocol_server_t *) socket,
                        peer
                    );
                    break;

                default:
                    return -1;
            }

            break;
        
        default:
            return -1;
    }

    // Acquire a sending pass
    pomelo_protocol_send_pass_t * pass =
        pomelo_protocol_socket_prepare_send_pass(socket, POMELO_PACKET_PAYLOAD);
    if (!pass) {
        // Cannot allocate pass
        return -1;
    }

    // Acquire a packet
    pomelo_packet_payload_t * packet = (pomelo_packet_payload_t *) pass->packet;

    // Update the sequence and source of packet payload
    packet->base.sequence = peer->sequence_number++;
    packet->source = buffer;

    // Set body with buffer
    pomelo_payload_t * body = &packet->base.body;
    body->data = buffer->data + offset;
    body->position = length;
    body->capacity = length;

    // Increase the reference of source buffer
    pomelo_buffer_ref(buffer);

    // Set receipient and send
    pass->peer = peer;
    pomelo_protocol_send_pass_submit(pass);

    // => pomelo_protocol_socket_sent_payload
    return 0;
}


void pomelo_protocol_socket_sync_time(
    pomelo_protocol_socket_t * socket,
    uint64_t req_send_time,  // t0
    uint64_t req_recv_time,  // t1
    uint64_t res_delta_time, // t2 - t1
    uint64_t res_recv_time   // t3
) {
    assert(socket != NULL);
    if (socket->mode == POMELO_PROTOCOL_SOCKET_MODE_SERVER) {
        return; // Server will not synchronize socket time
    }
    pomelo_protocol_client_t * client = (pomelo_protocol_client_t *) socket;
    pomelo_protocol_clock_sync(
        &socket->clock,
        &client->peer->rtt,
        req_send_time,
        req_recv_time,
        req_recv_time + res_delta_time,
        res_recv_time
    );
}


/* -------------------------------------------------------------------------- */
/*                         Packet callback functions                          */
/* -------------------------------------------------------------------------- */

int pomelo_socket_packet_request_init(
    pomelo_packet_request_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_request_init(packet);
    return 0;
}


int pomelo_socket_packet_request_reset(
    pomelo_packet_request_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_reset((pomelo_packet_t *) packet);
    return 0;
}


int pomelo_socket_packet_response_init(
    pomelo_packet_response_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_response_init(packet);
    return 0;
}


int pomelo_socket_packet_response_reset(
    pomelo_packet_response_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_reset((pomelo_packet_t *) packet);
    return 0;
}


int pomelo_socket_packet_denied_init(
    pomelo_packet_denied_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_denied_init(packet);
    return 0;
}


int pomelo_socket_packet_denied_reset(
    pomelo_packet_denied_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_reset((pomelo_packet_t *) packet);
    return 0;
}


int pomelo_socket_packet_challenge_init(
    pomelo_packet_challenge_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_challenge_init(packet);
    return 0;
}


int pomelo_socket_packet_challenge_reset(
    pomelo_packet_challenge_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_reset((pomelo_packet_t *) packet);
    return 0;
}


int pomelo_socket_packet_payload_init(
    pomelo_packet_payload_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_payload_init(packet);
    return 0;
}


int pomelo_socket_packet_payload_reset(
    pomelo_packet_payload_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_reset((pomelo_packet_t *) packet);
    return 0;
}


int pomelo_socket_packet_ping_init(
    pomelo_packet_ping_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_ping_init(packet);
    return 0;
}


int pomelo_socket_packet_ping_reset(
    pomelo_packet_ping_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_reset((pomelo_packet_t *) packet);
    return 0;
}


int pomelo_socket_packet_disconnect_init(
    pomelo_packet_disconnect_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_disconnect_init(packet);
    return 0;
}


int pomelo_socket_packet_disconnect_reset(
    pomelo_packet_disconnect_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_reset((pomelo_packet_t *) packet);
    return 0;
}


int pomelo_socket_packet_pong_init(
    pomelo_packet_pong_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_pong_init(packet);
    return 0;
}


int pomelo_socket_packet_pong_reset(
    pomelo_packet_pong_t * packet,
    pomelo_protocol_socket_t * socket
) {
    (void) socket;
    pomelo_packet_reset((pomelo_packet_t *) packet);
    return 0;
}
