// Specific functions for socket servers
#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "pomelo/allocator.h"
#include "codec/packed.h"
#include "codec/packet.h"
#include "socket.h"
#include "peer.h"
#include "server.h"


/* -------------------------------------------------------------------------- */
/*                              Server internal                               */
/* -------------------------------------------------------------------------- */

static int64_t pomelo_map_address_hash(
    pomelo_map_t * map,
    void * callback_context,
    pomelo_address_t * address
) {
    (void) map;
    (void) callback_context;
    return pomelo_address_hash(address);
}

static bool pomelo_map_address_compare(
    pomelo_map_t * map,
    void * callback_context,
    pomelo_address_t * first,
    pomelo_address_t * second
) {
    (void) map;
    (void) callback_context;
    return pomelo_address_compare(first, second);
}

static int pomelo_socket_server_peer_init(
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_server_t * server
) {
    assert(peer != NULL);
    assert(server != NULL);

    return pomelo_protocol_peer_init(peer, (pomelo_protocol_socket_t *) server);
}


static int pomelo_socket_server_peer_reset(
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_server_t * server
) {
    assert(peer != NULL);
    (void) server;
    pomelo_protocol_peer_reset(peer);
    return 0;
}


static int pomelo_socket_server_peer_cleanup(
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_server_t * server
) {
    assert(peer != NULL);
    (void) server;
    pomelo_protocol_peer_cleanup(peer);
    return 0;
}


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


int pomelo_protocol_server_options_init(
    pomelo_protocol_server_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_protocol_server_options_t));
    return 0;
}


pomelo_protocol_socket_t * pomelo_protocol_server_create(
    pomelo_protocol_server_options_t * options
) {
    assert(options != NULL);
    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_protocol_server_t * server =
        pomelo_allocator_malloc_t(allocator, pomelo_protocol_server_t);
    if (!server) {
        return NULL;
    }
    memset(server, 0, sizeof(pomelo_protocol_server_t));

    // The socket has the same address as the server
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    int ret = pomelo_protocol_socket_init(
        socket,
        (pomelo_protocol_socket_options_t *) options
    );

    if (ret < 0) {
        pomelo_allocator_free(allocator, server);
        return NULL;
    }

    // Initialize the server
    socket->mode = POMELO_PROTOCOL_SOCKET_MODE_SERVER;
    server->max_clients = options->max_clients;
    server->address = options->address;
    server->protocol_id = options->protocol_id;
    memcpy(server->private_key, options->private_key, POMELO_KEY_BYTES);

    pomelo_pool_options_t pool_options;
    pomelo_map_options_t map_options;

    // Create pool of peers
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_protocol_peer_t);
    pool_options.callback_context = socket;

    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_socket_server_peer_init;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_socket_server_peer_reset;
    pool_options.deallocate_callback = (pomelo_pool_callback)
        pomelo_socket_server_peer_cleanup;

    server->peer_pool = pomelo_pool_create(&pool_options);
    if (!server->peer_pool) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }

    // Create client ID to peer map
    pomelo_map_options_init(&map_options);
    map_options.allocator = allocator;
    map_options.key_size = sizeof(int64_t);
    map_options.value_size = sizeof(pomelo_protocol_peer_t *);
    server->client_id_map = pomelo_map_create(&map_options);
    if (!server->client_id_map) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }

    // Create address to peer maps
    pomelo_map_options_init(&map_options);
    map_options.allocator = allocator;
    map_options.key_size = sizeof(pomelo_address_t);
    map_options.value_size = sizeof(pomelo_protocol_peer_t *);
    map_options.hash_fn = (pomelo_map_hash_fn) pomelo_map_address_hash;
    map_options.compare_fn = (pomelo_map_compare_fn) pomelo_map_address_compare;

    server->address_connected_map = pomelo_map_create(&map_options);
    if (!server->address_connected_map) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }

    server->address_anonymous_map = pomelo_map_create(&map_options);
    if (!server->address_anonymous_map) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }

    pomelo_list_options_t list_options;
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_protocol_peer_t *);

    // The following lists share the same scheme

    // Create connected peers list
    server->connected_peers = pomelo_list_create(&list_options);
    if (!server->connected_peers) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }

    // Create disconnecting peers list
    server->disconnecting_peers = pomelo_list_create(&list_options);
    if (!server->disconnecting_peers) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }

    // Create anonymous peers list
    server->anonymous_peers = pomelo_list_create(&list_options);
    if (!server->anonymous_peers) {
        pomelo_protocol_socket_destroy(socket);
        return NULL;
    }

    server->anonymous_sequence_number = 0;
    server->challenge_sequence_number = 0;
    server->ping_timer = NULL;

    return socket;
}



/* -------------------------------------------------------------------------- */
/*                                  Cleanup                                   */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_server_destroy(pomelo_protocol_server_t * server) {
    assert(server != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    
    if (server->peer_pool) {
        pomelo_pool_destroy(server->peer_pool);
        server->peer_pool = NULL;
    }
    
    if (server->client_id_map) {
        pomelo_map_destroy(server->client_id_map);
        server->client_id_map = NULL;
    }

    if (server->address_connected_map) {
        pomelo_map_destroy(server->address_connected_map);
        server->address_connected_map = NULL;
    }

    if (server->address_anonymous_map) {
        pomelo_map_destroy(server->address_anonymous_map);
        server->address_anonymous_map = NULL;
    }

    if (server->connected_peers) {
        pomelo_list_destroy(server->connected_peers);
        server->connected_peers = NULL;
    }

    if (server->disconnecting_peers) {
        pomelo_list_destroy(server->disconnecting_peers);
        server->disconnecting_peers = NULL;
    }

    if (server->anonymous_peers) {
        pomelo_list_destroy(server->anonymous_peers);
        server->anonymous_peers = NULL;
    }

    if (server->ping_timer) {
        pomelo_platform_timer_stop(socket->platform, server->ping_timer);
        server->ping_timer = NULL;
    }

    // Cleanup the socket after cleaning up the server
    pomelo_protocol_socket_cleanup(socket);

    // Free itself
    pomelo_allocator_free(socket->allocator, server);
}



/* -------------------------------------------------------------------------- */
/*                             Receive callbacks                              */
/* -------------------------------------------------------------------------- */

int pomelo_protocol_server_on_recv(
    pomelo_protocol_server_t * server,
    pomelo_address_t * address,
    pomelo_buffer_t * buffer,
    size_t length,
    uint64_t recv_time,
    bool encrypted
) {
    assert(server != NULL);
    assert(address != NULL);
    assert(buffer != NULL);

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

    // Wrap the buffer into a payload for reading
    pomelo_payload_t payload;
    payload.data = buffer->data;
    payload.position = 0;
    payload.capacity = length;

    // Read the prefix
    pomelo_codec_packet_header_t header;
    int ret = pomelo_codec_decode_packet_header(&header, &payload);
    if (ret < 0) {
        return -1; // Failed to decode packet header
    }

    // Server ignores challenge & denied packets
    pomelo_packet_type type = header.type;
    if (type == POMELO_PACKET_CHALLENGE || type == POMELO_PACKET_DENIED) {
        // Invalid packet type
        return -1;
    }

    uint64_t sequence = header.sequence;
    size_t header_size = payload.position;
    size_t body_size = length - header_size;

    if (type != POMELO_PACKET_REQUEST && encrypted) {
        // With encrypted packet, we need to substract the length of hash
        body_size -= POMELO_HMAC_BYTES;
    }

    // Validate the packet size
    if (!pomelo_packet_validate_packet_body_size(type, body_size)) {
        return -1; // Invalid packet
    }

    // Get the peer
    pomelo_protocol_peer_t * peer =
        pomelo_protocol_server_find_connected_peer(server, address);
    pomelo_protocol_peer_server_state state;

    // Check the peer out and protect server from packet replay
    switch (type) {
        // Only server packet
        case POMELO_PACKET_REQUEST:
        case POMELO_PACKET_RESPONSE:
            if (peer) {
                return -1; // Not available for connected socket
            }
            break;

        // Replay protection
        case POMELO_PACKET_PAYLOAD:
        case POMELO_PACKET_PING:
        case POMELO_PACKET_DISCONNECT:
        case POMELO_PACKET_PONG:
            if (!peer) { 
                return -1; // Not available for unconnected peer
            }
            state = peer->state.server;

            // Check the state
            if (state != POMELO_SERVER_CONNECTED &&
                state != POMELO_SERVER_UNCONFIRMED) {
                return -1;
            }

            // Replay protection
            if (!pomelo_protocol_peer_protect_replay(peer, sequence)) {
                return -1;
            }
            break;

        default:
            return -1; // Other cases are discarded
    }

    // Customized cases for request & response packet
    if (type == POMELO_PACKET_REQUEST) {
        // Quick checking the protocol ID of requesting token.
        uint64_t * p_protocol_id = (uint64_t *)
            (payload.data + POMELO_PACKET_REQUEST_PROTOCOL_ID_OFFSET);

        if (*p_protocol_id != server->protocol_id) {
            return -1; // Mismatch protocol ID
        }

        peer = pomelo_protocol_server_find_or_create_anonymous_peer(
            server, address
        );
        if (!peer) {
            return -1; // Cannot allocate new anonymous peer
        }

        // Update the codec context for anonymous peer
        pomelo_codec_packet_context_t * codec_ctx = &peer->codec_ctx;
        codec_ctx->protocol_id = 0; // Protocol ID will be set later
        codec_ctx->packet_decrypt_key = peer->decode_key;
        codec_ctx->packet_encrypt_key = peer->encode_key;

    } else if (type == POMELO_PACKET_RESPONSE) {
        peer = pomelo_protocol_server_find_anonymous_peer(server, address);
        if (!peer) {
            return -1; // No anonymous peer
        }
    }

    pomelo_protocol_socket_pools_t * pools = &server->socket.pools;
    pomelo_pool_t * packet_pool = pools->packets[type];
    pomelo_packet_t * packet = pomelo_pool_acquire(packet_pool);
    if (!packet) {
        return -1; // Failed acquire the packet
    }

    // Attach encryption key for packet
    switch (type) {
        case POMELO_PACKET_REQUEST:
            ((pomelo_packet_request_t *) packet)->private_key =
                server->private_key;
            break;
        
        case POMELO_PACKET_RESPONSE:
            ((pomelo_packet_response_t *) packet)->challenge_key =
                server->challenge_key;
            break;
        
        default:
            break;
    }

    // Update the packet sequence for encrypted packet
    packet->sequence = sequence;

    // Allocate & update the packet information
    pomelo_protocol_recv_pass_t * pass = pomelo_pool_acquire(pools->recv_pass);
    if (!pass) {
        pomelo_pool_release(packet_pool, packet);
        return -1; // // Failed to allocate info object
    }

    if (!encrypted) {
        pass->flags |= POMELO_PROTOCOL_PASS_FLAG_NO_DECRYPT;
    }

    // Attach the buffer
    packet->buffer = buffer;

    // Attach header
    packet->header.data = buffer->data;
    packet->header.capacity = header_size;
    packet->header.position = 0;

    // Attach body. Body payload includes HMAC (if encrypted)
    packet->body.data = buffer->data + header_size;
    packet->body.capacity = payload.capacity - header_size;
    packet->body.position = 0;

    pass->socket = &server->socket;
    pass->peer = peer;
    pass->packet = packet;
    pass->recv_time = recv_time;

    // Post the data to work
    pomelo_protocol_recv_pass_submit(pass);

    // => pomelo_protocol_socket_recv_packet
    return 0;
}


/* -------------------------------------------------------------------------- */
/*                              Server packets                                */
/* -------------------------------------------------------------------------- */


void pomelo_protocol_server_recv_request_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * packet,
    int result
) {
    assert(server != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    if (result < 0) {
        // Failed to decode, remove the anonymous peer and release packet
        pomelo_protocol_server_remove_anonymous_peer(server, peer);
        pomelo_protocol_socket_release_packet(
            socket, (pomelo_packet_t *) packet
        );
        return;
    }

    pomelo_connect_token_t * token = &packet->token;

    if (server->connected_peers->size >= server->max_clients) {
        // Full of slots, send denied packet.
        // The anonymous peer will be removed after sending completely the
        // denied packet. So we don't need to remove it here.
        pomelo_protocol_server_send_denied_packet(server, peer);
        pomelo_protocol_socket_release_packet(
            socket,
            (pomelo_packet_t *) packet
        );
        return;
    }

    // We get the token as raw private connect token
    int64_t client_id = token->client_id;

    // Get peer by ID
    if (pomelo_map_has(server->client_id_map, client_id)) {
        // Not NULL, connected, discard
        pomelo_protocol_server_remove_anonymous_peer(server, peer);
        pomelo_protocol_socket_release_packet(
            socket,
            (pomelo_packet_t *) packet
        );
        return;
    }

    peer->state.server = POMELO_SERVER_ANONYMOUS;
    peer->client_id = client_id; // Set the client ID
    peer->client_index = (int) client_id; // This is the same as client ID
    peer->last_recv_time = pomelo_platform_hrtime(socket->platform);
    peer->timeout_ns = token->timeout * 1000000000ULL;

    // Update codec protocol id
    peer->codec_ctx.protocol_id = token->protocol_id;

    // Update the codec keys
    memcpy(
        peer->decode_key,
        token->client_to_server_key,
        POMELO_KEY_BYTES
    );
    memcpy(
        peer->encode_key,
        token->server_to_client_key,
        POMELO_KEY_BYTES
    );

    // Response with challenge packet
    pomelo_protocol_server_send_challenge_packet(server, peer, packet);

    // Finally, release the packet
    pomelo_protocol_socket_release_packet(socket, (pomelo_packet_t *) packet);
}


void pomelo_protocol_server_recv_response_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_response_t * packet,
    int result
) {
    assert(server != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;    
    if (result < 0) {
        // Failed to decode packet
        pomelo_protocol_socket_release_packet(
            socket,
            (pomelo_packet_t *) packet
        );
        return;
    }


    pomelo_map_t * client_id_map = server->client_id_map;
    int64_t client_id = packet->challenge_token.client_id;
    if (pomelo_map_has(client_id_map, client_id)) {
        // There's a peer in client id map, connected, discard
        pomelo_protocol_socket_release_packet(
            socket,
            (pomelo_packet_t *) packet
        );
        return;
    }

    pomelo_map_t * address_connected_map = server->address_connected_map;
    pomelo_map_t * address_anonymous_map = server->address_anonymous_map;

    pomelo_address_t * address = &peer->address;

    // This peer is an anonymous peer
    peer->state.server = POMELO_SERVER_UNCONFIRMED;

    // Remove peer from list of anonymous peers and then add it to list of
    // connected peers
    pomelo_map_del(address_anonymous_map, *address);
    pomelo_list_remove(server->anonymous_peers, peer->connected_node);
    
    // Add to connected peers, and put the peer to connected map
    peer->connected_node = pomelo_list_push_back(server->connected_peers, peer);
    pomelo_map_set(address_connected_map, *address, peer);
    pomelo_map_set(client_id_map, client_id, peer);

    // Send ping packet
    pomelo_protocol_server_ping(server, peer);
    
    // Release the packet
    pomelo_protocol_socket_release_packet(
        socket,
        (pomelo_packet_t *) packet
    );

    // Finally, call the callback
    pomelo_protocol_socket_on_connected(socket, peer);
}


void pomelo_protocol_server_recv_disconnect_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
) {
    assert(server != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Release packet first
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    pomelo_protocol_socket_release_packet(socket, (pomelo_packet_t *) packet);

    if (result < 0) {
        return; // Failed to decode packet
    }

    // The process the disconnected peer (The callback will be called here)
    pomelo_protocol_server_process_disconnected_peer(server, peer);
}


void pomelo_protocol_server_recv_ping_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    uint64_t recv_time,
    int result
) {
    assert(server != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    if (result < 0) {
        // Failed to decode, release packet
        pomelo_protocol_socket_release_packet(
            socket,
            (pomelo_packet_t *) packet
        );
        return;
    }

    if (peer->client_id != packet->client_id) {
        // Mismatch client ID
        pomelo_protocol_socket_release_packet(
            socket,
            (pomelo_packet_t *) packet
        );
        return;
    }

    if (peer->state.server == POMELO_SERVER_UNCONFIRMED) {
        // Switch the state from unconfirmed to connected
        peer->state.server = POMELO_SERVER_CONNECTED;
    }

    // Reply a pong packet after receiving ping
    pomelo_protocol_socket_reply_pong(
        socket,
        peer,
        packet->ping_sequence,
        recv_time
    );

    // Finally, release the packet
    pomelo_protocol_socket_release_packet(socket, (pomelo_packet_t *) packet);
}


/* -------------------------------------------------------------------------- */
/*                          Server outgoing packets                           */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_server_sent_denied_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_denied_t * packet,
    int result
) {
    assert(server != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    (void) result;

    if (peer->state.server != POMELO_SERVER_ANONYMOUS) {
        // Only available for anonymous peer
        pomelo_protocol_socket_release_packet(
            (pomelo_protocol_socket_t *) server,
            (pomelo_packet_t *) packet
        );
        return;
    }

    // Remove the anonymous peer
    pomelo_protocol_server_remove_anonymous_peer(server, peer);

    // Release the packet
    pomelo_protocol_socket_release_packet(
        (pomelo_protocol_socket_t *) server,
        (pomelo_packet_t *) packet
    );
}


void pomelo_protocol_server_sent_challenge_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_challenge_t * packet,
    int result
) {
    assert(server != NULL);
    (void) peer;
    assert(packet != NULL);
    (void) result;

    // Release the packet
    pomelo_protocol_socket_release_packet(
        (pomelo_protocol_socket_t *) server,
        (pomelo_packet_t *) packet
    );
}


void pomelo_protocol_server_sent_ping_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    int result
) {
    assert(server != NULL);
    (void) peer;
    assert(packet != NULL);
    (void) result;

    // Release the packet
    pomelo_protocol_socket_release_packet(
        (pomelo_protocol_socket_t *) server,
        (pomelo_packet_t *) packet
    );
}


void pomelo_protocol_server_sent_disconnect_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
) {
    assert(server != NULL);
    (void) peer;
    assert(packet != NULL);
    (void) result;

    // Release the packet
    pomelo_protocol_socket_release_packet(
        (pomelo_protocol_socket_t *) server,
        (pomelo_packet_t *) packet
    );
}


/* -------------------------------------------------------------------------- */
/*                         Server specific functions                          */
/* -------------------------------------------------------------------------- */

int pomelo_protocol_server_start(pomelo_protocol_server_t * server) {
    assert(server != NULL);

    int ret;
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;

    // Initialize randomly challenge key
    ret = pomelo_codec_buffer_random(server->challenge_key, POMELO_KEY_BYTES);
    if (ret < 0) {
        return ret;
    }

    // Start adapter as server
    ret = pomelo_adapter_listen(socket->adapter, &server->address);
    if (ret < 0) {
        return ret;
    }

    // Start the ping interval
    server->ping_timer = pomelo_platform_timer_start(
        socket->platform,
        (pomelo_platform_timer_cb) pomelo_protocol_server_ping_callback,
        0, // No timeout
        POMELO_FREQ_TO_MS(POMELO_PING_FREQUENCY_HZ),
        socket
    );

    if (!server->ping_timer) {
        // Cannot start new timer, stop the socket.
        pomelo_adapter_stop(socket->adapter);
        return -1;
    }

    return 0;
}


void pomelo_protocol_server_ping_callback(
    pomelo_protocol_server_t * server
) {
    assert(server != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    pomelo_list_t * peers = server->connected_peers;
    if (peers->size == 0) {
        // No connected clients, nothing to do
        return;
    }

    // Get the current time
    uint64_t time_ns = pomelo_platform_hrtime(socket->platform);
    uint64_t elapsed_ns; // Elapsed time from previous received packet

    pomelo_protocol_peer_t * peer;
    pomelo_list_ptr_for(peers, peer, {
        elapsed_ns = time_ns - peer->last_recv_time;

        if (elapsed_ns > peer->timeout_ns) {
            // Timed out, call the disconnect callback
            pomelo_protocol_server_process_disconnected_peer(server, peer);
        } else {
            // Send the ping to peer
            pomelo_protocol_server_ping(server, peer);
        }
    });
}


void pomelo_protocol_server_process_disconnected_peer(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
) {
    assert(server != NULL);
    assert(peer != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;

    peer->state.server = POMELO_SERVER_DISCONNECTED;

    // Call the callback
    pomelo_protocol_socket_on_disconnected(socket, peer);

    // Then, remove it from list and return it to pool
    pomelo_protocol_server_remove_connected_peer(server, peer);
}


void pomelo_protocol_server_send_challenge_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * request_packet
) {
    assert(server != NULL);
    assert(peer != NULL);
    assert(request_packet != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    pomelo_connect_token_t * token = &request_packet->token;
    pomelo_protocol_send_pass_t * pass =
        pomelo_protocol_socket_prepare_send_pass(
            socket, POMELO_PACKET_CHALLENGE
        );
    if (!pass) {
        return; // Cannot allocate pass
    }

    pomelo_packet_challenge_t * packet = 
        (pomelo_packet_challenge_t *) pass->packet;

    packet->challenge_key = server->challenge_key;
    packet->base.sequence = server->anonymous_sequence_number++;
    packet->token_sequence = server->challenge_sequence_number++;
    packet->challenge_token.client_id = token->client_id;

    // Copy user data
    memcpy(
        packet->challenge_token.user_data,
        token->user_data,
        POMELO_USER_DATA_BYTES
    );

    pass->peer = peer;
    pomelo_protocol_send_pass_submit(pass);
}


void pomelo_protocol_server_send_denied_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
) {
    assert(server != NULL);
    assert(peer != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    pomelo_protocol_send_pass_t * pass =
        pomelo_protocol_socket_prepare_send_pass(socket, POMELO_PACKET_DENIED);
    if (!pass) {
        // Cannot allocate pass
        return;
    }

    // Update sequence number
    pomelo_packet_denied_t * packet = (pomelo_packet_denied_t *) pass->packet;
    packet->sequence = server->anonymous_sequence_number++;

    pass->peer = peer;
    pomelo_protocol_send_pass_submit(pass);

    // The anonymous peer will be removed in the following callback
    // => pomelo_protocol_server_send_denied_packet_callback
}



void pomelo_protocol_server_ping(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
) {
    assert(server != NULL);
    assert(peer != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    pomelo_protocol_send_pass_t * pass =
        pomelo_protocol_socket_prepare_send_pass(socket, POMELO_PACKET_PING);
    if (!pass) {
        // Cannot allocate pass
        return;
    }

    pomelo_packet_ping_t * packet = (pomelo_packet_ping_t *) pass->packet;

    // Update the packet data
    packet->base.sequence = peer->sequence_number++;
    packet->client_id = peer->client_id;
    packet->ping_sequence = pomelo_protocol_peer_next_ping_sequence(peer);

    if (peer->state.server == POMELO_SERVER_UNCONFIRMED) {
        packet->attach_time = true;
        packet->time = pomelo_protocol_socket_time(socket);
    } else {
        packet->attach_time = false;
    }

    // Update pass information
    pass->peer = peer;

    // Finally, submit the pass
    pomelo_protocol_send_pass_submit(pass);
}


pomelo_protocol_peer_t * pomelo_protocol_server_find_connected_peer(
    pomelo_protocol_server_t * server,
    pomelo_address_t * address
) {
    assert(server != NULL);
    assert(address != NULL);

    pomelo_protocol_peer_t * peer = NULL;

    // Get the peer
    pomelo_map_get(server->address_connected_map, *address, &peer);
    return peer;
}


pomelo_protocol_peer_t * pomelo_protocol_server_find_anonymous_peer(
    pomelo_protocol_server_t * server,
    pomelo_address_t * address
) {
    assert(server != NULL);
    assert(address != NULL);

    pomelo_protocol_peer_t * peer = NULL;
    pomelo_map_get(server->address_anonymous_map, *address, &peer);
    return peer;
}


pomelo_protocol_peer_t * pomelo_protocol_server_find_or_create_anonymous_peer(
    pomelo_protocol_server_t * server,
    pomelo_address_t * address
) {
    assert(server != NULL);
    assert(address != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    pomelo_list_t * anonymous_peers = server->anonymous_peers;
    pomelo_map_t * address_anonymous_map = server->address_anonymous_map;
    pomelo_protocol_peer_t * peer;

    // Find the peer first
    peer = pomelo_protocol_server_find_anonymous_peer(server, address);
    if (peer) {
        // Found the peer, return it
        return peer;
    }

    uint64_t time = pomelo_platform_hrtime(socket->platform);

    if (anonymous_peers->size > 0) {
        // Try to find expired peer.
        // The most long-living-time peer is at the front of anonymous list
        peer = pomelo_list_element(
            anonymous_peers->front,
            pomelo_protocol_peer_t *
        );
        
        uint64_t delta = time - peer->created_time_ns;
        if (delta > POMELO_ANONYMOUS_PEER_EXPIRE_TIME_NS) {
            // This peer is expired
            // Move it to the last of list
            pomelo_list_remove(anonymous_peers, peer->connected_node);
            peer->connected_node =
                pomelo_list_push_back(anonymous_peers, peer);

            // Update the key of peer address map
            pomelo_map_del(address_anonymous_map, peer->address);
            pomelo_map_set(address_anonymous_map, *address, peer);

            // Update the values
            peer->created_time_ns = time;
            peer->address = *address;

            // And return it
            return peer;
        }
    }

    // Or create new one
    peer = pomelo_pool_acquire(server->peer_pool);
    if (!peer) {
        // Cannot allocate new peer
        return NULL;
    }

    // Push the peer to anonymous list & map
    peer->connected_node =
        pomelo_list_push_back(server->anonymous_peers, peer);
    pomelo_map_set(address_anonymous_map, *address, peer);

    peer->created_time_ns = time;
    peer->address = *address;

    return peer;
}


void pomelo_protocol_server_remove_connected_peer(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
) {
    assert(server != NULL);

    pomelo_protocol_peer_terminate_passes(peer);
    pomelo_list_remove(server->connected_peers, peer->connected_node);
    pomelo_map_del(server->client_id_map, peer->client_id);
    pomelo_map_del(server->address_connected_map, peer->address);
    pomelo_pool_release(server->peer_pool, peer);
}


void pomelo_protocol_server_remove_anonymous_peer(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
) {
    assert(server != NULL);
    assert(peer != NULL);

    pomelo_protocol_peer_terminate_passes(peer);
    pomelo_map_del(server->address_anonymous_map, peer->address);
    pomelo_pool_release(server->peer_pool, peer);
}


void pomelo_protocol_server_stop(pomelo_protocol_server_t * server) {
    assert(server != NULL);
    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;

    if (server->ping_timer) {
        pomelo_platform_timer_stop(socket->platform, server->ping_timer);
        server->ping_timer = NULL;
    }

    if (server->disconnecting_timer) {
        pomelo_platform_timer_stop(
            socket->platform,
            server->disconnecting_timer
        );
        server->disconnecting_timer = NULL;
    }
}


void pomelo_protocol_server_stop_deferred(pomelo_protocol_server_t * server) {
    assert(server != NULL);
    // Stop all timers, reset all values and remove all sessions

    // Reset all values
    server->anonymous_sequence_number = 0;
    server->challenge_sequence_number = 0;
    memset(server->private_key, 0, POMELO_KEY_BYTES);
    memset(server->challenge_key, 0, POMELO_KEY_BYTES);

    // Remove all connected peers
    pomelo_list_iterator_t it;
    pomelo_protocol_peer_t * peer;
    pomelo_list_iterate(server->connected_peers, &it);

    while (pomelo_list_iterator_next(&it, &peer)) {
        peer->state.server = POMELO_SERVER_DISCONNECTED;
        pomelo_pool_release(server->peer_pool, peer);        
    }

    pomelo_list_clear(server->connected_peers);

    // Release all anonymous peers
    pomelo_list_iterate(server->anonymous_peers, &it);
    while (pomelo_list_iterator_next(&it, &peer)) {
        peer->state.server = POMELO_SERVER_DISCONNECTED;
        pomelo_pool_release(server->peer_pool, peer);        
    }
    pomelo_list_clear(server->anonymous_peers);

    // Remove all disconnecting peers
    pomelo_list_iterate(server->disconnecting_peers, &it);
    while (pomelo_list_iterator_next(&it, &peer)) {
        peer->state.server = POMELO_SERVER_DISCONNECTED;
        pomelo_pool_release(server->peer_pool, peer);        
    }
    pomelo_list_clear(server->disconnecting_peers);


    // Remove all mapping
    pomelo_map_clear(server->client_id_map);
    pomelo_map_clear(server->address_connected_map);
    pomelo_map_clear(server->address_anonymous_map);

    // Finally, call the stopped callback
    pomelo_protocol_socket_on_stopped((pomelo_protocol_socket_t *) server);
}


int pomelo_protocol_server_disconnect_peer(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
) {
    assert(server != NULL);
    assert(peer != NULL);

    if (peer->state.server != POMELO_SERVER_CONNECTED) {
        return -1;
    }

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    pomelo_list_t * peers = server->disconnecting_peers;

    peer->disconnecting_node = pomelo_list_push_back(peers, peer);
    if (!peer->disconnecting_node) {
        return -1;
    }

    peer->state.server = POMELO_SERVER_DISCONNECTING;
    peer->remain_redundant_disconnect = POMELO_DISCONNECT_REDUNDANT_LIMIT;

    if (peers->size == 1) {
        // Start looping disconnect scanning
        server->disconnecting_timer = pomelo_platform_timer_start(
            socket->platform,
            (pomelo_platform_timer_cb) pomelo_protocol_server_send_disconnect,
            0, // No timeout
            POMELO_FREQ_TO_MS(POMELO_DISCONNECT_FREQUENCY_HZ),
            socket
        );

        if (!server->disconnecting_timer) {
            return -1;
        }
    }

    return 0;
}


void pomelo_protocol_server_send_disconnect(pomelo_protocol_server_t * server) {
    assert(server != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    pomelo_protocol_peer_t * peer;
    pomelo_list_t * disconnecting_peers = server->disconnecting_peers;
    pomelo_list_iterator_t it;

    pomelo_list_iterate(disconnecting_peers, &it);
    while (pomelo_list_iterator_next(&it, &peer)) {
        pomelo_protocol_server_send_disconnect_peer(server, peer);

        if (--peer->remain_redundant_disconnect == 0) {
            pomelo_protocol_server_process_disconnected_peer(server, peer);
            pomelo_list_iterator_remove(&it);
        }
    }

    if (disconnecting_peers->size == 0 && server->disconnecting_timer) {
        pomelo_platform_timer_stop(
            socket->platform,
            server->disconnecting_timer
        );

        server->disconnecting_timer = NULL;
    }
}


void pomelo_protocol_server_send_disconnect_peer(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
) {
    assert(server != NULL);
    assert(peer != NULL);

    pomelo_protocol_socket_t * socket = (pomelo_protocol_socket_t *) server;
    pomelo_protocol_send_pass_t * pass =
        pomelo_protocol_socket_prepare_send_pass(
            socket, POMELO_PACKET_DISCONNECT
        );
    if (!pass) {
        // Cannot allocate pass
        return;
    }

    // Update the packet data
    pass->packet->sequence = peer->sequence_number++;

    // Update pass information & submit
    pass->peer = peer;
    pomelo_protocol_send_pass_submit(pass);
}
