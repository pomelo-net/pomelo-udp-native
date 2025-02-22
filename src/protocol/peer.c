#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "socket.h"
#include "peer.h"
#include "client.h"
#include "server.h"


/* -------------------------------------------------------------------------- */
/*                               Public API                                   */
/* -------------------------------------------------------------------------- */


int64_t pomelo_peer_client_id(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);

    return peer->client_id;
}


pomelo_address_t * pomelo_peer_address(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);

    return &peer->address;
}


int pomelo_protocol_peer_send(
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
) {
    assert(peer != NULL);
    return pomelo_protocol_socket_send(
        peer->socket,
        peer,
        buffer,
        offset,
        length
    );
}


/* -------------------------------------------------------------------------- */
/*                               Private API                                  */
/* -------------------------------------------------------------------------- */

int pomelo_protocol_peer_init(
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_socket_t * socket
) {
    assert(peer != NULL);
    assert(socket != NULL);

    memset(peer, 0, sizeof(pomelo_protocol_peer_t));

    // Reset the replay protector
    memset(
        peer->replay_protector.received_sequence,
        0xff, // Maximum value
        sizeof(peer->replay_protector.received_sequence)
    );

    peer->allocator = socket->allocator;
    peer->socket = socket;
    pomelo_rtt_calculator_init(&peer->rtt);

    // Create sending passes list
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = socket->allocator;
    list_options.element_size = sizeof(pomelo_protocol_send_pass_t *);

    peer->send_passes = pomelo_list_create(&list_options);
    if (!peer->send_passes) {
        pomelo_protocol_peer_cleanup(peer);
        return -1;
    }

    // Create receiving passes list
    pomelo_list_options_init(&list_options);
    list_options.allocator = socket->allocator;
    list_options.element_size = sizeof(pomelo_protocol_recv_pass_t *);
    
    peer->recv_passes = pomelo_list_create(&list_options);
    if (!peer->recv_passes) {
        pomelo_protocol_peer_cleanup(peer);
        return -1;
    }

    return 0;
}


void pomelo_protocol_peer_reset(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);

    pomelo_allocator_t * allocator = peer->allocator;
    pomelo_protocol_socket_t * socket = peer->socket;
    pomelo_list_t * send_passes = peer->send_passes;
    pomelo_list_t * recv_passes = peer->recv_passes;

    memset(peer, 0, sizeof(pomelo_protocol_peer_t));
    memset(
        peer->replay_protector.received_sequence,
        0xff,
        sizeof(peer->replay_protector.received_sequence)
    );

    peer->allocator = allocator;
    peer->socket = socket;
    peer->send_passes = send_passes;
    peer->recv_passes = recv_passes;
    pomelo_rtt_calculator_init(&peer->rtt);

    pomelo_list_clear(peer->send_passes);
    pomelo_list_clear(peer->recv_passes);
}


void pomelo_protocol_peer_cleanup(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);

    if (peer->send_passes) {
        pomelo_list_destroy(peer->send_passes);
        peer->send_passes = NULL;
    }

    if (peer->recv_passes) {
        pomelo_list_destroy(peer->recv_passes);
        peer->recv_passes = NULL;
    }
}


int pomelo_protocol_peer_protect_replay(
    pomelo_protocol_peer_t * peer,
    uint64_t sequence_number
) {
    assert(peer != NULL);

    pomelo_protocol_replay_protector_t * protector = &peer->replay_protector;
    if (sequence_number < protector->most_recent_sequence) {
        uint64_t delta = protector->most_recent_sequence - sequence_number;
        if (delta > POMELO_REPlAY_PROTECTED_BUFFER_SIZE) {
            return 0;
        }
    }

    uint64_t index = sequence_number % POMELO_REPlAY_PROTECTED_BUFFER_SIZE;
    uint64_t received = protector->received_sequence[index];

    if (received == UINT64_MAX || received < sequence_number) {
        protector->received_sequence[index] = sequence_number;

        if (sequence_number > protector->most_recent_sequence) {
            protector->most_recent_sequence = sequence_number;
        }

        return 1;
    }

    return 0;
}


uint64_t pomelo_protocol_peer_next_ping_sequence(
    pomelo_protocol_peer_t * peer
) {
    assert(peer != NULL);

    uint64_t now = pomelo_platform_hrtime(peer->socket->platform);
    pomelo_rtt_entry_t * entry =
        pomelo_rtt_calculator_next_entry(&peer->rtt, now);

    return entry->sequence;
}


void pomelo_protocol_peer_receive_pong(
    pomelo_protocol_peer_t * peer,
    pomelo_packet_pong_t * packet,
    uint64_t recv_time
) {
    assert(peer != NULL);
    assert(packet != NULL);

    pomelo_rtt_entry_t * entry =
        pomelo_rtt_calculator_entry(&peer->rtt, packet->ping_sequence);
    if (!entry) {
        return;
    }
    uint64_t send_time = entry->time;

    // Update RTT 
    pomelo_rtt_calculator_submit_entry(
        &peer->rtt,
        entry,
        recv_time,              // t3
        packet->pong_delta_time // t2 - t1
    );

    // Then sync time
    pomelo_protocol_socket_sync_time(
        peer->socket,
        send_time,                // t0
        packet->ping_recv_time,   // t1
        packet->pong_delta_time,  // t2 - t1
        recv_time                 // t3
    );
}


void pomelo_protocol_peer_terminate_passes(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);
    pomelo_protocol_send_pass_t * send_pass = NULL;
    pomelo_protocol_recv_pass_t * recv_pass = NULL;

    pomelo_list_for(peer->send_passes, send_pass, pomelo_protocol_send_pass_t *,
        { send_pass->flags |= POMELO_PROTOCOL_PASS_FLAG_TERMINATED; }
    );

    pomelo_list_for(peer->recv_passes, recv_pass, pomelo_protocol_recv_pass_t *,
        { recv_pass->flags |= POMELO_PROTOCOL_PASS_FLAG_TERMINATED; }
    );
}


int64_t pomelo_protocol_peer_get_client_id(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);
    return peer->client_id;
}


pomelo_address_t * pomelo_protocol_peer_get_address(
    pomelo_protocol_peer_t * peer
) {
    assert(peer != NULL);
    return &peer->address;
}


int pomelo_protocol_peer_disconnect(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);
    pomelo_protocol_socket_t * socket = peer->socket;

    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            return pomelo_protocol_server_disconnect_peer(
                (pomelo_protocol_server_t *) socket,
                peer
            );
        
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            return pomelo_protocol_client_disconnect_peer(
                (pomelo_protocol_client_t *) socket,
                peer
            );
    }

    return -1;
}


int pomelo_protocol_peer_rtt(
    pomelo_protocol_peer_t * peer,
    uint64_t * mean,
    uint64_t * variance
) {
    assert(peer != NULL);
    pomelo_rtt_calculator_get(&peer->rtt, mean, variance);
    return 0;
}
