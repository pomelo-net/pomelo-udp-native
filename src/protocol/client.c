// Specific functions for socket client
#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "pomelo/allocator.h"
#include "socket.h"
#include "peer.h"
#include "client.h"
#include "context.h"


int pomelo_protocol_client_on_alloc(
    pomelo_protocol_client_t * client,
    pomelo_protocol_context_t * context
) {
    assert(client != NULL);
    assert(context != NULL);

    pomelo_protocol_socket_t * socket = &client->socket;
    int ret = pomelo_protocol_socket_on_alloc(socket, context);
    if (ret < 0) return ret; // Initializing base socket is failed


    // Initialize the request emitter
    pomelo_protocol_emitter_options_t emitter_options;
    memset(&emitter_options, 0, sizeof(pomelo_protocol_emitter_options_t));
    emitter_options.client = client;
    emitter_options.frequency = POMELO_CONNECTION_REQUEST_RESPONSE_FREQUENCY_HZ;
    emitter_options.trigger_cb = pomelo_protocol_client_send_request;
    emitter_options.timeout_cb = pomelo_protocol_client_handle_request_timeout;

    ret = pomelo_protocol_emitter_init(
        &client->emitter_request,
        &emitter_options
    );
    if (ret < 0) return ret;

    // Initialize the response emitter
    memset(&emitter_options, 0, sizeof(pomelo_protocol_emitter_options_t));
    emitter_options.client = client;
    emitter_options.frequency = POMELO_CONNECTION_REQUEST_RESPONSE_FREQUENCY_HZ;
    emitter_options.trigger_cb = pomelo_protocol_client_send_response;
    ret = pomelo_protocol_emitter_init(
        &client->emitter_response,
        &emitter_options
    );
    if (ret < 0) return ret;

    // Initialize the keep_alive emitter
    memset(&emitter_options, 0, sizeof(pomelo_protocol_emitter_options_t));
    emitter_options.client = client;
    emitter_options.frequency = POMELO_KEEP_ALIVE_FREQUENCY_HZ;
    emitter_options.trigger_cb = pomelo_protocol_client_send_keep_alive;
    ret = pomelo_protocol_emitter_init(
        &client->emitter_keep_alive,
        &emitter_options
    );
    if (ret < 0) return ret;

    // Initialize the disconnect emitter
    memset(&emitter_options, 0, sizeof(pomelo_protocol_emitter_options_t));
    emitter_options.client = client;
    emitter_options.frequency = POMELO_DISCONNECT_FREQUENCY_HZ;
    emitter_options.limit = POMELO_DISCONNECT_REDUNDANT_LIMIT;
    emitter_options.trigger_cb = pomelo_protocol_client_send_disconnect;
    emitter_options.limit_cb = pomelo_protocol_client_handle_disconnect_limit;
    ret = pomelo_protocol_emitter_init(
        &client->emitter_disconnect,
        &emitter_options
    );
    if (ret < 0) return ret;

    return 0;
}


void pomelo_protocol_client_on_free(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    pomelo_protocol_socket_on_free(&client->socket);
}


int pomelo_protocol_client_init(
    pomelo_protocol_client_t * client,
    pomelo_protocol_client_options_t * options
) {
    assert(client != NULL);
    assert(options != NULL);

    if (!options->platform || !options->adapter) return -1;

    // Check capability of adapter
    uint32_t flags = 0;
    uint32_t capability = pomelo_adapter_get_capability(options->adapter);
    // Check if the adapter supports client
    if (!(capability & POMELO_ADAPTER_CAPABILITY_CLIENT_ALL)) {
        return -1; // No supported capabilities
    }

    // Check if the adapter supports encryption
    if (!(capability & POMELO_ADAPTER_CAPABILITY_CLIENT_ENCRYPTED)) {
        flags |= POMELO_PROTOCOL_SOCKET_FLAG_NO_ENCRYPT;
    }

    pomelo_protocol_socket_t * socket = &client->socket;
    pomelo_protocol_socket_options_t socket_options = {
        .platform = options->platform,
        .adapter = options->adapter,
        .sequencer = options->sequencer,
        .mode = POMELO_PROTOCOL_SOCKET_MODE_CLIENT,
        .flags = flags
    };
    // Initialize the base socket
    int ret = pomelo_protocol_socket_init(socket, &socket_options);
    if (ret < 0) return ret; // Initializing base socket is failed

    client->address_index = 0; // No choosen address
    memset(&client->connect_token, 0, sizeof(pomelo_connect_token_t));
    memcpy(
        client->connect_token_data,
        options->connect_token,
        POMELO_CONNECT_TOKEN_BYTES
    );
    client->challenge_token_sequence = 0;
    memset(client->challenge_token_data, 0, POMELO_CHALLENGE_TOKEN_BYTES);
    
    return 0;
}


void pomelo_protocol_client_cleanup(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    pomelo_protocol_socket_cleanup(&client->socket);
}


pomelo_protocol_socket_t * pomelo_protocol_client_create(
    pomelo_protocol_client_options_t * options
) {
    assert(options != NULL);
    if (!options->context) return NULL; // No context is provided

    return (pomelo_protocol_socket_t *)
        pomelo_pool_acquire(options->context->client_pool, options);
}


void pomelo_protocol_client_destroy(pomelo_protocol_client_t * client) {
    assert(client != NULL);

    // Release the client to pool
    pomelo_pool_release(client->socket.context->client_pool, client);
}


int pomelo_protocol_client_start(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;

    // Acquire a peer for client
    pomelo_protocol_peer_info_t info = {
        .socket = socket,
        .created_time_ns = pomelo_platform_hrtime(socket->platform),
    };
    pomelo_protocol_peer_t * peer =
        pomelo_pool_acquire(socket->context->peer_pool, &info);
    if (!peer) return -1; // Failed to acquire peer
    client->peer = peer;
    peer->state = POMELO_PROTOCOL_PEER_DISCONNECTED;

    // No decryption in decoding public part process. So we don't have to move
    // the decoding process to other worker.

    // Decode the public portion of connect token to get token information
    pomelo_connect_token_t * connect_token = &client->connect_token;
    int ret = pomelo_connect_token_decode_public(
        client->connect_token_data,
        connect_token
    );
    if (ret < 0) {
        peer->state = POMELO_PROTOCOL_PEER_INVALID_CONNECT_TOKEN;
        return ret;
    }

    uint64_t time = pomelo_platform_now(socket->platform);
    if (connect_token->expire_timestamp < time) {
        peer->state = POMELO_PROTOCOL_PEER_CONNECT_TOKEN_EXPIRE;
        return -1;
    }

    if (connect_token->naddresses <= 0) {
        peer->state = POMELO_PROTOCOL_PEER_INVALID_CONNECT_TOKEN;
        return -1;
    }

    // Update the codec keys
    peer->socket = socket;
    peer->created_time_ns = pomelo_platform_hrtime(socket->platform);
    peer->timeout_ns = connect_token->timeout * 1000000000ULL;

    pomelo_protocol_crypto_context_t * codec_ctx = peer->crypto_ctx;
    codec_ctx->protocol_id = connect_token->protocol_id;
    memcpy(
        codec_ctx->packet_encrypt_key,
        connect_token->client_to_server_key,
        POMELO_KEY_BYTES
    );
    memcpy(
        codec_ctx->packet_decrypt_key,
        connect_token->server_to_client_key,
        POMELO_KEY_BYTES
    );
    memset(codec_ctx->private_key, 0, POMELO_KEY_BYTES);
    memset(codec_ctx->challenge_key, 0, POMELO_KEY_BYTES);

    // Reset the address index
    client->address_index = 0;

    // Connect to the first address
    pomelo_address_t * address = connect_token->addresses;

    return pomelo_protocol_client_connect_address(client, address);
}


void pomelo_protocol_client_stop(pomelo_protocol_client_t * client) {
    assert(client != NULL);
    pomelo_protocol_peer_t * peer = client->peer;
    if (!peer) return;

    peer->state = POMELO_PROTOCOL_PEER_DISCONNECTED;

    // Stop the adapter
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    pomelo_adapter_stop(socket->adapter);

    // Stop all the emitters
    pomelo_protocol_emitter_stop(&client->emitter_request);
    pomelo_protocol_emitter_stop(&client->emitter_response);
    pomelo_protocol_emitter_stop(&client->emitter_keep_alive);
    pomelo_protocol_emitter_stop(&client->emitter_disconnect);
    pomelo_protocol_peer_cancel_senders_and_receivers(peer);

    // Release the peer
    if (client->peer) {
        pomelo_pool_release(socket->context->peer_pool, client->peer);
        client->peer = NULL;
    }
}


int pomelo_protocol_client_validate(
    pomelo_protocol_client_t * client,
    pomelo_protocol_packet_incoming_t * incoming,
    pomelo_protocol_packet_validation_t * validation
) {
    assert(client != NULL);
    assert(incoming != NULL);
    assert(validation != NULL);

    pomelo_protocol_packet_header_t * header = &validation->header;
    pomelo_address_t * address = incoming->address;
    pomelo_protocol_packet_type type = header->type;
    if (type == POMELO_PROTOCOL_PACKET_REQUEST || type == POMELO_PROTOCOL_PACKET_RESPONSE) {
        return -1;
    }

    // Check the packet source address
    // Only accept packet from connected server address
    if (!pomelo_address_compare(address, &client->peer->address)) {
        // Discard, invalid packet source
        return -1;
    }

    pomelo_protocol_peer_t * peer = client->peer;
    pomelo_protocol_peer_state state = peer->state;

    // Check the state of client and protect client from replay
    switch (type) {
        // Only accept denied packet when sending connection request or
        // response
        case POMELO_PROTOCOL_PACKET_DENIED:
            if (state != POMELO_PROTOCOL_PEER_REQUEST &&
                state != POMELO_PROTOCOL_PEER_RESPONSE
            ) return -1;
            break;

        // Only accept challenge packet when sending connection request
        case POMELO_PROTOCOL_PACKET_CHALLENGE:
            if (state != POMELO_PROTOCOL_PEER_REQUEST) return -1;
            break;

        // Only accept keep alive when sending connection response or connected
        case POMELO_PROTOCOL_PACKET_KEEP_ALIVE:
            if (state != POMELO_PROTOCOL_PEER_CONNECTED &&
                state != POMELO_PROTOCOL_PEER_RESPONSE
            ) return -1;

            // Replay protection
            if (
                pomelo_protocol_peer_protect_replay(peer, header->sequence) < 0
            ) return -1;
            break;

        // Only accept when connected
        case POMELO_PROTOCOL_PACKET_PAYLOAD:
        case POMELO_PROTOCOL_PACKET_DISCONNECT:
            if (state != POMELO_PROTOCOL_PEER_CONNECTED) return -1;

            // Replay protection
            if (
                pomelo_protocol_peer_protect_replay(peer, header->sequence) < 0
            ) return -1;
            break;

        default: // Discard other packet types
            return -1;
    }

    validation->peer = peer;
    return 0;
}


/* -------------------------------------------------------------------------- */
/*                          Client incoming packets                           */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_client_recv_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
) {
    assert(client != NULL);
    assert(peer != NULL);
    assert(packet != NULL);
    
    switch (packet->type) {
        case POMELO_PROTOCOL_PACKET_DENIED:
            pomelo_protocol_client_recv_denied(
                client, peer, (pomelo_protocol_packet_denied_t *) packet
            );
            break;
        
        case POMELO_PROTOCOL_PACKET_CHALLENGE:
            pomelo_protocol_client_recv_challenge(
                client, peer, (pomelo_protocol_packet_challenge_t *) packet
            );
            break;
        
        case POMELO_PROTOCOL_PACKET_DISCONNECT:
            pomelo_protocol_client_recv_disconnect(
                client, peer, (pomelo_protocol_packet_disconnect_t *) packet
            );
            break;
            
        case POMELO_PROTOCOL_PACKET_KEEP_ALIVE:
            pomelo_protocol_client_recv_keep_alive(
                client,
                peer,
                (pomelo_protocol_packet_keep_alive_t *) packet
            );
            break;
        
        default:
            break;
    }
}


void pomelo_protocol_client_recv_failed(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_header_t * header
) {
    (void) client;
    (void) peer;
    (void) header;
    // Ignore
}


void pomelo_protocol_client_recv_denied(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_denied_t * packet
) {
    assert(client != NULL);
    assert(peer != NULL);
    assert(
        peer->state == POMELO_PROTOCOL_PEER_REQUEST ||
        peer->state == POMELO_PROTOCOL_PEER_RESPONSE
    );

    (void) peer;
    (void) packet;

    // Change state to connection denied
    peer->state = POMELO_PROTOCOL_PEER_DENIED;

    // Get next address
    pomelo_address_t * address = pomelo_protocol_client_next_address(client);
    if (!address) {
        // No more addresses, call the connect result callback
        pomelo_protocol_socket_on_connect_result(
            (pomelo_protocol_socket_t *) client,
            POMELO_PROTOCOL_SOCKET_CONNECT_DENIED
        );

        // Then stop the socket
        pomelo_protocol_socket_stop((pomelo_protocol_socket_t *) client);
        return;
    }

    // Retry next address
    pomelo_protocol_client_stop(client);
    pomelo_protocol_client_connect_address(client, address);
}


void pomelo_protocol_client_recv_challenge(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_challenge_t * packet
) {
    assert(client != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    if (peer->state != POMELO_PROTOCOL_PEER_REQUEST) {
        return; // Ignore
    }

    // Change state to sending connection response
    peer->state = POMELO_PROTOCOL_PEER_RESPONSE;

    // Stop sending request packets
    pomelo_protocol_emitter_stop(&client->emitter_request);

    // Set the challenge token sequence and encrypted challenge token
    client->challenge_token_sequence = packet->token_sequence;
    memcpy(
        client->challenge_token_data,
        packet->challenge_data.encrypted,
        sizeof(client->challenge_token_data)
    );

    // Start sending response packet
    int ret = pomelo_protocol_emitter_start(&client->emitter_response);
    if (ret < 0) {
        // Failed to start sending response packet
        pomelo_protocol_socket_stop((pomelo_protocol_socket_t *) client);
    }
}


void pomelo_protocol_client_recv_disconnect(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_disconnect_t * packet
) {
    assert(client != NULL);
    (void) peer;
    (void) packet;

    if (peer->state != POMELO_PROTOCOL_PEER_CONNECTED) {
        return; // Ignore
    }

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    peer->state = POMELO_PROTOCOL_PEER_DISCONNECTED;
    pomelo_protocol_socket_dispatch_peer_disconnected(socket, client->peer);
    pomelo_protocol_socket_stop(socket);
}


void pomelo_protocol_client_recv_keep_alive(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_keep_alive_t * packet
) {
    assert(client != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    if (peer->state != POMELO_PROTOCOL_PEER_RESPONSE) {
        return; // Only process when client is sending connection response
    }

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    peer->state = POMELO_PROTOCOL_PEER_CONNECTED;
    peer->client_id = packet->client_id;

    // Stop response and start keep alive
    pomelo_protocol_emitter_stop(&client->emitter_response);
    int ret = pomelo_protocol_emitter_start(&client->emitter_keep_alive);
    if (ret < 0) {
        pomelo_protocol_socket_stop(socket);
        return; // Failed to start response emitter
    }

    // Call the connect result callback first
    pomelo_protocol_socket_on_connect_result(
        socket,
        POMELO_PROTOCOL_SOCKET_CONNECT_SUCCESS
    );

    // Call the callback
    pomelo_protocol_socket_on_connected(socket, peer);
}


/* -------------------------------------------------------------------------- */
/*                             Outgoing packets                               */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_client_sent_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
) {
    (void) client;
    (void) peer;
    (void) packet;
    // Ignore the packet
}


/* -------------------------------------------------------------------------- */
/*                         Client specific functions                          */
/* -------------------------------------------------------------------------- */

int pomelo_protocol_client_connect_address(
    pomelo_protocol_client_t * client,
    pomelo_address_t * address
) {
    assert(client != NULL);
    assert(address != NULL);

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
    peer->state = POMELO_PROTOCOL_PEER_REQUEST;

    // Update timeout of emitter
    if (client->connect_token.timeout > 0) {
        client->emitter_request.timeout_ms =
            POMELO_SECONDS_TO_MS(client->connect_token.timeout);
    } else {
        client->emitter_request.timeout_ms = 0;
    }

    return pomelo_protocol_emitter_start(&client->emitter_request);
}


void pomelo_protocol_client_send_request(pomelo_protocol_client_t * client) {
    assert(client != NULL);

    pomelo_protocol_peer_t * peer = client->peer;
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;

    // Update the request information
    pomelo_connect_token_t * connect_token = &client->connect_token;
    pomelo_protocol_packet_request_info_t info = {
        .protocol_id = connect_token->protocol_id,
        .expire_timestamp = connect_token->expire_timestamp,
        .connect_token_nonce = connect_token->connect_token_nonce,
        .encrypted_connect_token =
            (client->connect_token_data + POMELO_CONNECT_TOKEN_PRIVATE_OFFSET)
    };
    pomelo_protocol_packet_request_t * request = pomelo_pool_acquire(
        socket->context->packet_pools[POMELO_PROTOCOL_PACKET_REQUEST],
        &info
    );
    if (!request) {
        // Failed to allocate packet
        pomelo_protocol_socket_stop(socket);
        return;
    }

    pomelo_protocol_socket_dispatch(socket, peer, &request->base);
}


void pomelo_protocol_client_send_response(pomelo_protocol_client_t * client) {
    assert(client != NULL);

    pomelo_protocol_peer_t * peer = client->peer;
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    pomelo_protocol_packet_response_info_t info = {
        .sequence = pomelo_protocol_peer_next_sequence(peer),
        .token_sequence = client->challenge_token_sequence,
        .encrypted_challenge_token = client->challenge_token_data,
    };
    pomelo_protocol_packet_response_t * packet = pomelo_pool_acquire(
        socket->context->packet_pools[POMELO_PROTOCOL_PACKET_RESPONSE],
        &info
    );
    if (!packet) {
        // Failed to allocate packet
        pomelo_protocol_socket_stop(socket);
        return;
    }

    pomelo_protocol_socket_dispatch(socket, peer, &packet->base);
}


void pomelo_protocol_client_send_keep_alive(pomelo_protocol_client_t * client) {
    assert(client != NULL);

    pomelo_protocol_peer_t * peer = client->peer;
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;

    // Check expiration
    uint64_t time_ns = pomelo_platform_hrtime(socket->platform);
    uint64_t elapsed_ns = time_ns - peer->last_recv_time;
    if (elapsed_ns > peer->timeout_ns) {
        pomelo_protocol_emitter_stop(&client->emitter_keep_alive);
        peer->state = POMELO_PROTOCOL_PEER_TIMED_OUT;
        pomelo_protocol_socket_dispatch_peer_disconnected(socket, client->peer);
        pomelo_protocol_socket_stop(socket);
        return;
    }

    // Update the sequence number
    pomelo_protocol_packet_keep_alive_info_t info = {
        .sequence = pomelo_protocol_peer_next_sequence(peer),
        .client_id = peer->client_id
    };
    pomelo_protocol_packet_keep_alive_t * packet = pomelo_pool_acquire(
        socket->context->packet_pools[POMELO_PROTOCOL_PACKET_KEEP_ALIVE],
        &info
    );
    if (!packet) {
        // Failed to allocate packet
        pomelo_protocol_socket_stop(socket);
        return;
    }

    pomelo_protocol_socket_dispatch(socket, peer, &packet->base);
}


void pomelo_protocol_client_send_disconnect(pomelo_protocol_client_t * client) {
    assert(client != NULL);

    pomelo_protocol_peer_t * peer = client->peer;
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    pomelo_protocol_packet_disconnect_info_t info = {
        .sequence = pomelo_protocol_peer_next_sequence(peer)
    };
    pomelo_protocol_packet_disconnect_t * packet = pomelo_pool_acquire(
        socket->context->packet_pools[POMELO_PROTOCOL_PACKET_DISCONNECT],
        &info
    );
    if (!packet) {
        // Failed to allocate packet
        pomelo_protocol_socket_stop(socket);
        return;
    }

    pomelo_protocol_socket_dispatch(socket, peer, &packet->base);
}


pomelo_address_t * pomelo_protocol_client_next_address(
    pomelo_protocol_client_t * client
) {
    assert(client != NULL);

    int index = ++client->address_index;
    if (index >= client->connect_token.naddresses) {
        // No more addresses
        return NULL;
    }
    return &client->connect_token.addresses[index];
}


int pomelo_protocol_client_disconnect_peer(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer
) {
    assert(client != NULL);
    assert(peer != NULL);

    if (peer->state != POMELO_PROTOCOL_PEER_CONNECTED) {
        return -1; // Peer has not connected to server.
    }

    if (peer != client->peer) {
        return -1; // Wrong peer to disconnect
    }

    // Stop sending keep alive packet
    pomelo_protocol_emitter_stop(&client->emitter_keep_alive);

    // Update the state
    peer->state = POMELO_PROTOCOL_PEER_DISCONNECTING;

    // Call the callback
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;
    pomelo_protocol_socket_dispatch_peer_disconnected(socket, client->peer);

    // Start sending redundant disconnect packets
    int ret = pomelo_protocol_emitter_start(&client->emitter_disconnect);
    if (ret < 0) {
        // Stop the socket because of failure
        pomelo_protocol_socket_stop(socket);
        return ret;
    }

    return 0;
}


void pomelo_protocol_client_handle_request_timeout(
    pomelo_protocol_client_t * client
) {
    assert(client != NULL);

    // Stop the request emitter
    pomelo_protocol_emitter_stop(&client->emitter_request);

    // Change the state
    pomelo_protocol_peer_t * peer = client->peer;
    switch (peer->state) {
        case POMELO_PROTOCOL_PEER_REQUEST:
            peer->state = POMELO_PROTOCOL_PEER_REQUEST_TIMED_OUT;
            break;

        case POMELO_PROTOCOL_PEER_RESPONSE:
            peer->state = POMELO_PROTOCOL_PEER_RESPONSE_TIMED_OUT;
            break;

        default:
            break;
    }

    pomelo_address_t * address = pomelo_protocol_client_next_address(client);
    if (!address) {
        pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) client;

        // No more addresses, call the connect result callback
        pomelo_protocol_socket_on_connect_result(
            socket,
            POMELO_PROTOCOL_SOCKET_CONNECT_TIMED_OUT
        );

        // Then stop the socket
        pomelo_protocol_socket_stop(socket);
        return;
    }

    // Retry next address
    pomelo_protocol_client_stop(client);
    pomelo_protocol_client_connect_address(client, address);
}


void pomelo_protocol_client_handle_disconnect_limit(
    pomelo_protocol_client_t * client
) {
    assert(client != NULL);

    // Stop sending disconnect packets
    pomelo_protocol_emitter_stop(&client->emitter_disconnect);
    pomelo_protocol_socket_stop(&client->socket);
}
