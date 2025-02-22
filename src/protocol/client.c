// Specific functions for socket client
#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "pomelo/allocator.h"
#include "codec/packed.h"
#include "codec/packet.h"
#include "socket.h"
#include "peer.h"
#include "client.h"



/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


int pomelo_protocol_client_options_init(
    pomelo_protocol_client_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_protocol_client_options_t));
    return 0;
}


pomelo_protocol_socket_t * pomelo_protocol_client_create(
    pomelo_protocol_client_options_t * options
) {
    assert(options != NULL);
    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    // Allocate new client structure
    pomelo_protocol_client_t * client = 
        pomelo_allocator_malloc_t(allocator, pomelo_protocol_client_t);
    if (!client) {
        return NULL;
    }
    memset(client, 0, sizeof(pomelo_protocol_client_t));

    // The socket has the same address as the client
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;

    // Initialize the socket base
    int ret = pomelo_protocol_socket_init(
        socket,
        (pomelo_protocol_socket_options_t *) options
    );
    if (ret < 0) {
        // Initializing base socket is failed
        pomelo_allocator_free(allocator, client);
        return NULL;
    }

    // Initialize the socket client
    socket->mode = POMELO_PROTOCOL_SOCKET_MODE_CLIENT;
    memcpy(
        client->connect_token_raw,
        options->connect_token,
        POMELO_CONNECT_TOKEN_BYTES
    );

    // Initialize the client specific functions
    client->address_index = 0; // No choosen address
    pomelo_protocol_peer_t * peer =
        pomelo_allocator_malloc_t(allocator, pomelo_protocol_peer_t);
    if (!peer) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }

    // Initialize the peer
    client->peer = peer;
    pomelo_protocol_peer_init(client->peer, socket);

    pomelo_protocol_socket_pools_t * pools = &socket->pools;
    pomelo_protocol_client_emitters_t * emitters = &client->emitters;

    pomelo_protocol_emitter_t * request = &emitters->request;
    pomelo_protocol_emitter_t * response = &emitters->response;
    pomelo_protocol_emitter_t * ping = &emitters->ping;
    pomelo_protocol_emitter_t * disconnect = &emitters->disconnect;

    pomelo_protocol_emitter_init(request);
    pomelo_protocol_emitter_init(response);
    pomelo_protocol_emitter_init(ping);
    pomelo_protocol_emitter_init(disconnect);

    request->frequency = POMELO_CONNECTION_REQUEST_RESPONSE_FREQUENCY_HZ;
    response->frequency = POMELO_CONNECTION_REQUEST_RESPONSE_FREQUENCY_HZ;
    ping->frequency = POMELO_PING_FREQUENCY_HZ;
    disconnect->frequency = POMELO_DISCONNECT_FREQUENCY_HZ;
    disconnect->limit = POMELO_DISCONNECT_REDUNDANT_LIMIT;

    // Update socket & peer
    request->socket = socket;
    response->socket = socket;
    ping->socket = socket;
    disconnect->socket = socket;
    request->peer = peer;
    response->peer = peer;
    ping->peer = peer;
    disconnect->peer = peer;

    // Request, response packet only need to be encoded once
    pomelo_protocol_emitter_set_encode_once(request);
    pomelo_protocol_emitter_set_encode_once(response);
    
    // Set the triggering callbacks
    request->timeout_cb = (pomelo_protocol_emitter_cb)
        pomelo_protocol_client_request_timed_out_callback;
    response->trigger_cb = (pomelo_protocol_emitter_cb)
        pomelo_protocol_client_response_callback;
    ping->trigger_cb = (pomelo_protocol_emitter_cb)
        pomelo_protocol_client_ping_callback;
    disconnect->trigger_cb = (pomelo_protocol_emitter_cb)
        pomelo_protocol_client_disconnect_trigger;
    
    // Set limit callbacks
    disconnect->limit_reached_cb = (pomelo_protocol_emitter_cb)
        pomelo_protocol_client_disconnect_callback;

    // Acquire the packets & payloads from the begining for client
    request->packet =
        pomelo_pool_acquire(pools->packets[POMELO_PACKET_REQUEST]);
    if (!request->packet) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }
    
    response->packet =
        pomelo_pool_acquire(pools->packets[POMELO_PACKET_RESPONSE]);
    if (!response->packet) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }
    
    ping->packet =
        pomelo_pool_acquire(pools->packets[POMELO_PACKET_PING]);
    if (!ping->packet) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }
    
    disconnect->packet =
        pomelo_pool_acquire(pools->packets[POMELO_PACKET_DISCONNECT]);
    if (!disconnect->packet) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }
    
    // Allocate buffers for emitters.
    pomelo_buffer_context_t * buffer_context = socket->context->buffer_context;
    pomelo_buffer_t * buffer = buffer_context->acquire(buffer_context);
    if (!buffer) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }
    pomelo_packet_attach_buffer(request->packet, buffer);

    buffer = buffer_context->acquire(buffer_context);
    if (!buffer) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }
    pomelo_packet_attach_buffer(response->packet, buffer);

    buffer = buffer_context->acquire(buffer_context);
    if (!buffer) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }
    pomelo_packet_attach_buffer(ping->packet, buffer);

    buffer = buffer_context->acquire(buffer_context);
    if (!buffer) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }
    pomelo_packet_attach_buffer(disconnect->packet, buffer);

    return socket;
}


void pomelo_protocol_client_destroy(pomelo_protocol_client_t * client) {
    assert(client != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;

    pomelo_protocol_socket_pools_t * pools = &socket->pools;
    pomelo_protocol_client_emitters_t * emitters = &client->emitters;

    // Release all associated resources of emitters

    pomelo_protocol_emitter_t * request = &emitters->request;
    pomelo_protocol_emitter_t * response = &emitters->response;
    pomelo_protocol_emitter_t * ping = &emitters->ping;
    pomelo_protocol_emitter_t * disconnect = &emitters->disconnect;

    // Request emitter
    if (request->packet) {
        if (request->packet->buffer) {
            pomelo_buffer_unref(request->packet->buffer);
            request->packet->buffer = NULL;
        }

        pomelo_pool_release(
            pools->packets[POMELO_PACKET_REQUEST],
            request->packet
        );
        request->packet = NULL;
    }

    // Response emitter
    if (response->packet) {
        if (response->packet->buffer) {
            pomelo_buffer_unref(response->packet->buffer);
            response->packet->buffer = NULL;
        }

        pomelo_pool_release(
            pools->packets[POMELO_PACKET_RESPONSE],
            response->packet
        );
        response->packet = NULL;
    }

    // Ping emitter
    if (ping->packet) {
        if (ping->packet->buffer) {
            pomelo_buffer_unref(ping->packet->buffer);
            ping->packet->buffer = NULL;
        }

        pomelo_pool_release(
            pools->packets[POMELO_PACKET_PING],
            ping->packet
        );
        ping->packet = NULL;
    }

    // Disconnect emitter
    if (disconnect->packet) {
        if (disconnect->packet->buffer) {
            pomelo_buffer_unref(disconnect->packet->buffer);
            disconnect->packet->buffer = NULL;
        }

        pomelo_pool_release(
            pools->packets[POMELO_PACKET_DISCONNECT],
            disconnect->packet
        );
        disconnect->packet = NULL;
    }

    // Cleanup the peer
    if (client->peer) {
        pomelo_protocol_peer_cleanup(client->peer);
        pomelo_allocator_free(socket->allocator, client->peer);
        client->peer = NULL;
    }

    // Cleanup the socket after cleaning up the client
    pomelo_protocol_socket_cleanup(socket);

    // Free itself
    pomelo_allocator_free(socket->allocator, client);
}



/* -------------------------------------------------------------------------- */
/*                             Receive callbacks                              */
/* -------------------------------------------------------------------------- */

int pomelo_protocol_client_on_recv(
    pomelo_protocol_client_t * client,
    pomelo_address_t * address,
    pomelo_buffer_t * buffer,
    size_t length,
    uint64_t recv_time,
    bool encrypted
) {
    assert(client != NULL);
    assert(buffer != NULL);

    // Wrap the buffer into a payload for reading
    pomelo_payload_t payload;
    payload.data = buffer->data;
    payload.position = 0;
    payload.capacity = length;
    
    // Validate the capacity of incoming packet
    if (encrypted) {
        if (length < POMELO_PACKET_ENCRYPTED_MIN_CAPACITY) {
            return -1;
        }
    } else {
        if (length < POMELO_PACKET_UNENCRYPTED_MIN_CAPACITY) {
            return -1;
        }
    }

    pomelo_codec_packet_header_t header;
    int ret = pomelo_codec_decode_packet_header(&header, &payload);
    if (ret < 0) {
        return -1; // Failed to decode packet header
    }

    // Client ignores request & response packets
    pomelo_packet_type type = header.type;
    if (type == POMELO_PACKET_REQUEST || type == POMELO_PACKET_RESPONSE) {
        return -1;
    }
    size_t header_size = payload.position;

    // The real size of body (decrypted)
    size_t body_size = length - header_size;
    if (encrypted) {
        body_size -= POMELO_HMAC_BYTES;
    }

    // Check the packet source address
    // Only accept packet from connected server address
    if (!pomelo_address_compare(address, &client->peer->address)) {
        // Discard, invalid packet source
        return -1;
    }

    // Validate the packet size
    if (!pomelo_packet_validate_packet_body_size(type, body_size)) {
        return -1;
    }

    pomelo_protocol_peer_t * peer = client->peer;
    pomelo_protocol_peer_client_state state = peer->state.client;
    uint64_t sequence = header.sequence;

    // Check the state of client and protect client from replay
    switch (type) {
        // Only accept denied packet when sending connection request or
        // response
        case POMELO_PACKET_DENIED:
            if (state != POMELO_CLIENT_SENDING_CONNECTION_REQUEST &&
                state != POMELO_CLIENT_SENDING_CONNECTION_RESPONSE
            ) {
                return -1;
            }
            break;

        // Only accept challenge packet when sending connection request
        case POMELO_PACKET_CHALLENGE:
            if (state != POMELO_CLIENT_SENDING_CONNECTION_REQUEST) {
                return -1;
            }
            break;

        // Only accept ping when sending connection response or connected
        case POMELO_PACKET_PING:
            if (state != POMELO_CLIENT_CONNECTED &&
                state != POMELO_CLIENT_SENDING_CONNECTION_RESPONSE
            ) {
                return -1;
            }

            // Replay protection
            if (!pomelo_protocol_peer_protect_replay(peer, sequence)) {
                return -1;
            }
            break;

        // Only accept when connected
        case POMELO_PACKET_PAYLOAD:
        case POMELO_PACKET_DISCONNECT:
        case POMELO_PACKET_PONG:
            if (state != POMELO_CLIENT_CONNECTED) {
                return -1;
            }

            // Replay protection
            if (!pomelo_protocol_peer_protect_replay(peer, sequence)) {
                return -1;
            }
            break;

        default: // Discard other packet types
            return -1;
    }

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    pomelo_protocol_socket_pools_t * pools = &socket->pools;
    pomelo_pool_t * packet_pool = pools->packets[type];
    pomelo_packet_t * packet = pomelo_pool_acquire(packet_pool);
    if (!packet) {
        // Failed acquire the packet
        return -1;
    }

    // Allocate & update the packet information
    pomelo_protocol_recv_pass_t * pass = pomelo_pool_acquire(pools->recv_pass);
    if (!pass) {
        // Failed to allocate decoding information object
        pomelo_pool_release(packet_pool, packet);
        return -1;
    }

    if (!encrypted) {
        pass->flags |= POMELO_PROTOCOL_PASS_FLAG_NO_DECRYPT;
    }

    // Update the header & body payload
    packet->buffer = buffer;
    packet->header.data = buffer->data;
    packet->header.capacity = header_size;
    packet->header.position = 0;

    packet->body.data = buffer->data + header_size;
    packet->body.capacity = payload.capacity - header_size;
    packet->body.position = 0;

    // Update the packet sequence number
    packet->sequence = sequence;

    pass->socket = &client->socket;
    pass->packet = packet;
    pass->peer = client->peer;
    pass->recv_time = recv_time;

    // Post the data to work
    pomelo_protocol_recv_pass_submit(pass);
    return 0;
}


/* -------------------------------------------------------------------------- */
/*                          Client incoming packets                           */
/* -------------------------------------------------------------------------- */


void pomelo_protocol_client_recv_denied_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_denied_t * packet,
    int result
) {
    assert(client != NULL);
    (void) peer;
    assert(packet != NULL);

    // Release packet in any case
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    pomelo_protocol_socket_release_packet(socket, (pomelo_packet_t *) packet);
    if (result < 0) {
        return; // Failed to decode packet, ignore
    }

    // Stop the socket, then call the deinied callback
    client->stopped_reason = POMELO_SOCKET_CLIENT_STOPPED_DENIED;
    pomelo_protocol_socket_stop(socket);
}


void pomelo_protocol_client_recv_challenge_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_challenge_t * packet,
    int result
) {
    assert(client != NULL);
    (void) peer;
    assert(packet != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    if (result < 0) {
        // Failed to decode packet, release it
        pomelo_protocol_socket_release_packet(
            socket, (pomelo_packet_t *) packet
        );
        return;
    }
    
    // Change state to sending connection response
    client->peer->state.client = POMELO_CLIENT_SENDING_CONNECTION_RESPONSE;

    // Stop sending request packets
    pomelo_protocol_client_request_stop(client);

    pomelo_protocol_emitter_t * response = &client->emitters.response;
    pomelo_packet_response_t * packet_response = (pomelo_packet_response_t *)
        response->packet;
    packet_response->token_sequence = packet->token_sequence;

    memcpy(
        packet_response->encrypted_challenge_token,
        packet->encrypted_challenge_token,
        POMELO_CHALLENGE_TOKEN_BYTES
    );
    response->trigger_cb = NULL;

    // Start sending response packet
    pomelo_protocol_client_response_start(client);

    // Release the packet
    pomelo_protocol_socket_release_packet(socket, (pomelo_packet_t *) packet);
}


void pomelo_protocol_client_recv_disconnect_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
) {
    assert(client != NULL);
    (void) peer;

    // Release packet in any case
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    pomelo_protocol_socket_release_packet(socket, (pomelo_packet_t *) packet);
    if (result < 0) {
        return; // Failed to decode packet
    }

    if (client->peer->state.client != POMELO_CLIENT_CONNECTED) {
        return; // Invalid state
    }

    // Stop the socket, then call the deinied callback
    client->stopped_reason = POMELO_SOCKET_CLIENT_STOPPED_DISCONNECTED;
    pomelo_protocol_socket_stop(socket);
}


void pomelo_protocol_client_recv_ping_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    uint64_t recv_time,
    int result
) {
    assert(client != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    if (result < 0) {
        pomelo_protocol_socket_release_packet(
            socket, (pomelo_packet_t *) packet
        );
        return; // Failed to decode, release packet
    }

    // Reply pong packet after receiving ping
    pomelo_protocol_socket_reply_pong(
        socket,
        peer,
        packet->ping_sequence,
        recv_time
    );

    // Capture the client ID and release packet
    int64_t client_id = packet->client_id;
    pomelo_protocol_socket_release_packet(socket, (pomelo_packet_t *) packet);

    if (peer->state.client == POMELO_CLIENT_SENDING_CONNECTION_RESPONSE) {
        if (packet->attach_time) {
            // Set the initial time = Server time
            pomelo_protocol_clock_set(&socket->clock, packet->time);
        }

        pomelo_protocol_client_handle_ping_on_response(client, peer, client_id);
    }
}


void pomelo_protocol_client_handle_ping_on_response(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    int64_t client_id
) {
    assert(client != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    int ret = pomelo_protocol_client_update_state_connected(
        client, peer, client_id
    );
    if (ret < 0) {
        // Call error case
        client->stopped_reason = POMELO_SOCKET_CLIENT_STOPPED_ERROR;
        pomelo_protocol_socket_stop(socket);
        return;
    }

    // Call the connect result callback first
    pomelo_protocol_socket_on_connect_result(
        socket,
        POMELO_PROTOCOL_SOCKET_CONNECT_SUCCESS
    );

    // Call the callback
    pomelo_protocol_socket_on_connected(socket, peer);
}


int pomelo_protocol_client_update_state_connected(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    int64_t client_id
) {
    assert(client != NULL);
    
    // Change state to connected & update the client ID
    peer->state.client = POMELO_CLIENT_CONNECTED;
    peer->client_id = client_id;

    // Stop sending response
    int ret = pomelo_protocol_client_response_stop(client);
    if (ret < 0) return -1;

    // Start sending ping
    ret = pomelo_protocol_client_ping_start(client);
    if (ret < 0) return -1;

    return 0;
}


/* -------------------------------------------------------------------------- */
/*                         Client specific functions                          */
/* -------------------------------------------------------------------------- */

int pomelo_protocol_client_start(pomelo_protocol_client_t * client) {
    assert(client != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    pomelo_connect_token_t * connect_token = &client->connect_token;
    uint8_t * connect_token_raw = client->connect_token_raw;
    int ret;

    // No decryption in decoding public part process. So we don't have to move
    // the decoding process to other worker.

    // Decode the public portion of connect token to get token information
    ret = pomelo_connect_token_decode_public(connect_token_raw, connect_token);
    if (ret < 0) return ret;

    uint64_t time = pomelo_platform_now(socket->platform);
    if (connect_token->expire_timestamp < time) {
        return -1;
    }

    // Update the request information
    pomelo_packet_request_t * request =
        (pomelo_packet_request_t *) client->emitters.request.packet;

    request->protocol_id = client->connect_token.protocol_id;
    request->expire_timestamp = connect_token->expire_timestamp;
    memcpy(
        request->connect_token_nonce,
        connect_token->connect_token_nonce,
        POMELO_CONNECT_TOKEN_NONCE_BYTES
    );
    memcpy(
        request->encrypted_token,
        connect_token_raw + POMELO_CONNECT_TOKEN_PRIVATE_OFFSET,
        POMELO_CONNECT_TOKEN_PRIVATE_BYTES
    );

    if (connect_token->naddresses <= 0) {
        return -1;
    }

    // Update the codec keys
    pomelo_protocol_peer_t * peer = client->peer;
    pomelo_protocol_peer_reset(peer);

    memcpy(
        peer->decode_key,
        connect_token->server_to_client_key,
        POMELO_KEY_BYTES
    );
    memcpy(
        peer->encode_key,
        connect_token->client_to_server_key,
        POMELO_KEY_BYTES
    );
    peer->created_time_ns = pomelo_platform_hrtime(socket->platform);
    peer->timeout_ns = connect_token->timeout * 1000000000ULL;

    pomelo_codec_packet_context_t * codec_ctx = &peer->codec_ctx;
    codec_ctx->packet_encrypt_key = peer->encode_key;
    codec_ctx->packet_decrypt_key = peer->decode_key;
    codec_ctx->protocol_id = connect_token->protocol_id;

    client->address_index = 0;
    pomelo_address_t * address = connect_token->addresses;

    return pomelo_protocol_client_connect_address(client, address);
}


int pomelo_protocol_client_connect_address(
    pomelo_protocol_client_t * client,
    pomelo_address_t * address
) {
    assert(client != NULL);
    assert(address != NULL);

    // Reset the stopped reason
    client->stopped_reason = POMELO_SOCKET_CLIENT_STOPPED_UNSET;

    pomelo_protocol_peer_t * peer = client->peer;
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;

    // Set the peer target address
    peer->address = *address;

    // Initialize adapter
    int ret = pomelo_adapter_connect(socket->adapter, address);
    if (ret < 0) {
        return ret; // Failed to start connecting
    }

    // Change state to sending connection request
    peer->state.client = POMELO_CLIENT_SENDING_CONNECTION_REQUEST;
    return pomelo_protocol_client_request_start(client);
}


int pomelo_protocol_client_request_start(pomelo_protocol_client_t * client) {
    assert(client != NULL);

    // Update the request timeout
    int32_t timeout = client->connect_token.timeout;
    client->emitters.request.timeout_ms = POMELO_SECONDS_TO_MS(timeout);

    return pomelo_protocol_emitter_start(&client->emitters.request);
}


int pomelo_protocol_client_request_stop(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    return pomelo_protocol_emitter_stop(&client->emitters.request);
}


int pomelo_protocol_client_response_start(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    return pomelo_protocol_emitter_start(&client->emitters.response);
}


int pomelo_protocol_client_response_stop(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    return pomelo_protocol_emitter_stop(&client->emitters.response);
}


int pomelo_protocol_client_ping_start(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    return pomelo_protocol_emitter_start(&client->emitters.ping);
}


void pomelo_protocol_client_ping_callback(
    pomelo_protocol_client_t * client,
    pomelo_protocol_emitter_t * emitter
) {
    assert(client != NULL);
    (void) emitter;

    pomelo_protocol_peer_t * peer = client->peer;
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;

    // Update the sequence number
    pomelo_packet_ping_t * packet = 
        (pomelo_packet_ping_t *) client->emitters.ping.packet;

    packet->base.sequence = peer->sequence_number++;
    packet->client_id = peer->client_id;
    packet->ping_sequence = pomelo_protocol_peer_next_ping_sequence(peer);
    packet->attach_time = false; // No time attaching

    // Get the current time
    uint64_t time_ns = pomelo_platform_hrtime(socket->platform);
    
    uint64_t elapsed_ns = time_ns - peer->last_recv_time;
    if (elapsed_ns > peer->timeout_ns) {
        pomelo_protocol_client_process_disconnected_peer(client, peer);
        client->stopped_reason = POMELO_SOCKET_CLIENT_STOPPED_LOST_CONNECTION;
        pomelo_protocol_socket_stop(socket);
        return;
    }
}


void pomelo_protocol_client_response_callback(
    pomelo_protocol_client_t * client,
    pomelo_protocol_emitter_t * emitter
) {
    assert(client != NULL);
    assert(emitter != NULL);

    // Update sequence number
    emitter->packet->sequence = client->peer->sequence_number++;
}


int pomelo_protocol_client_ping_stop(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    return pomelo_protocol_emitter_stop(&client->emitters.ping);
}


int pomelo_protocol_client_disconnect_start(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    return pomelo_protocol_emitter_start(&client->emitters.disconnect);
}


void pomelo_protocol_client_disconnect_trigger(
    pomelo_protocol_client_t * client,
    pomelo_protocol_emitter_t * emitter
) {
    assert(client != NULL);
    assert(emitter != NULL);

    // Update sequence number
    emitter->packet->sequence = client->peer->sequence_number++;
}


void pomelo_protocol_client_disconnect_callback(
    pomelo_protocol_client_t * client,
    pomelo_protocol_emitter_t * emitter
) {
    (void) client;
    assert(emitter != NULL);
    // Send enough disconnect packets.

    // Call the callback
    pomelo_protocol_socket_on_disconnected(emitter->socket, emitter->peer);

    // And stop the socket
    pomelo_protocol_socket_stop(emitter->socket);
}


int pomelo_protocol_client_disconnect_stop(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    return pomelo_protocol_emitter_stop(&client->emitters.disconnect);
}


void pomelo_protocol_client_process_disconnected_peer(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer
) {
    assert(client != NULL);
    (void) peer;
    pomelo_protocol_client_ping_stop(client);
}


pomelo_address_t * pomelo_protocol_client_next_connect_address(
    pomelo_protocol_client_t * client
) {
    assert(client != NULL);

    if (client->address_index >= client->connect_token.naddresses) {
        // No more addresses
        return NULL;
    }

    return &client->connect_token.addresses[++client->address_index];
}


void pomelo_protocol_client_request_timed_out_callback(
    pomelo_protocol_client_t * client,
    pomelo_protocol_emitter_t * emitter
) {
    assert(client != NULL);
    (void) emitter;

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    client->stopped_reason = POMELO_SOCKET_CLIENT_STOPPED_REQUEST_TIMED_OUT;

    pomelo_protocol_socket_stop(socket);
}


void pomelo_protocol_client_stop(pomelo_protocol_client_t * client) {
    assert(client != NULL);

    // Stop all the emitters
    pomelo_protocol_client_ping_stop(client);
    pomelo_protocol_client_request_stop(client);
    pomelo_protocol_client_response_stop(client);
    pomelo_protocol_client_disconnect_stop(client);

    client->peer->state.client = POMELO_CLIENT_DISCONNECTED;
}


void pomelo_protocol_client_stop_deferred(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    pomelo_address_t * address;
    int ret;
    bool connecting = false; // Connecting flag for retrying

    pomelo_protocol_client_stopped_reason reason = client->stopped_reason;
    switch (reason) {
        // Retry next address cases:
        case POMELO_SOCKET_CLIENT_STOPPED_DENIED:
        case POMELO_SOCKET_CLIENT_STOPPED_REQUEST_TIMED_OUT:
            // Try the next address
            address = pomelo_protocol_client_next_connect_address(client);
            if (address) {
                // Found next address, try next one
                ret = pomelo_protocol_client_connect_address(client, address);
                connecting = (ret == 0);
            }

            if (!connecting) {
                // Stopped
                pomelo_protocol_socket_on_connect_result(
                    socket,
                    reason == POMELO_SOCKET_CLIENT_STOPPED_DENIED
                        ? POMELO_PROTOCOL_SOCKET_CONNECT_DENIED
                        : POMELO_PROTOCOL_SOCKET_CONNECT_TIMED_OUT
                );
            }
            break;

        case POMELO_SOCKET_CLIENT_STOPPED_DISCONNECTED:
        case POMELO_SOCKET_CLIENT_STOPPED_LOST_CONNECTION:
            // Close connection by server or lost the connection
            pomelo_protocol_socket_on_disconnected(socket, client->peer);
            break;
        
        case POMELO_SOCKET_CLIENT_STOPPED_ERROR:
            // This case is out of specs
            pomelo_protocol_socket_on_connect_result(
                socket,
                POMELO_PROTOCOL_SOCKET_CONNECT_DENIED
            );
            break;

        default: // Just avoid warning
            break;
    }

    if (!connecting) {
        // Call the final callback
        pomelo_protocol_socket_on_stopped(socket);
    }
}


int pomelo_protocol_client_disconnect_peer(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer
) {
    assert(client != NULL);
    assert(peer != NULL);

    if (peer->state.client != POMELO_CLIENT_CONNECTED) {
        return -1; // Peer has not connected to server.
    }

    if (peer != client->peer) {
        return -1; // Wrong peer to disconnect
    }

    // Stop sending ping packet
    pomelo_protocol_client_ping_stop(client);

    // Update the state
    peer->state.client = POMELO_CLIENT_DISCONNECTING;

    // Start sending redundant disconnect packets
    return pomelo_protocol_client_disconnect_start(client);
    // => pomelo_protocol_client_disconnect_callback
}


/* -------------------------------------------------------------------------- */
/*                          Client outgoing packets                           */
/* -------------------------------------------------------------------------- */


void pomelo_protocol_client_sent_request_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * packet,
    int result
) {
    assert(client != NULL);
    (void) peer;
    (void) packet;
    (void) result;

    pomelo_protocol_emitter_on_sent(&client->emitters.request);
}


void pomelo_protocol_client_sent_response_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_response_t * packet,
    int result
) {
    assert(client != NULL);
    (void) peer;
    (void) packet;
    (void) result;

    pomelo_protocol_emitter_on_sent(&client->emitters.response);
}


void pomelo_protocol_client_sent_ping_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    int result
) {
    assert(client != NULL);
    (void) peer;
    (void) packet;
    (void) result;

    pomelo_protocol_emitter_on_sent(&client->emitters.ping);
}


void pomelo_protocol_client_sent_disconnect_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
) {
    assert(client != NULL);
    (void) peer;
    (void) packet;
    (void) result;

    pomelo_protocol_emitter_on_sent(&client->emitters.disconnect);
}
