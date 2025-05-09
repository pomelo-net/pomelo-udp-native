#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "socket.h"
#include "peer.h"
#include "client.h"
#include "server.h"
#include "context.h"


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
    pomelo_buffer_view_t * views,
    size_t nviews
) {
    assert(peer != NULL);
    return pomelo_protocol_socket_send(peer->socket, peer, views, nviews);
}


void * pomelo_protocol_peer_get_extra(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);
    return peer->extra;
}


void pomelo_protocol_peer_set_extra(
    pomelo_protocol_peer_t * peer,
    void * data
) {
    assert(peer != NULL);
    peer->extra = data;
}


/* -------------------------------------------------------------------------- */
/*                               Private API                                  */
/* -------------------------------------------------------------------------- */

int pomelo_protocol_peer_on_alloc(
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_context_t * context
) {
    assert(peer != NULL);
    assert(context != NULL);
    peer->context = context;

    // Reset the replay protector
    memset(
        peer->replay_protector.received_sequence,
        0xff, // Maximum value
        sizeof(peer->replay_protector.received_sequence)
    );

    pomelo_allocator_t * allocator = context->allocator;

    // Create sending sender list
    pomelo_list_options_t list_options;
    memset(&list_options, 0, sizeof(pomelo_list_options_t));
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_protocol_sender_t *);
    peer->senders = pomelo_list_create(&list_options);
    if (!peer->senders) return -1; // Failed to create new list

    // Create receiving receiver list
    memset(&list_options, 0, sizeof(pomelo_list_options_t));
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_protocol_receiver_t *);
    peer->receivers = pomelo_list_create(&list_options);
    if (!peer->receivers) return -1; // Failed to create new list

    return 0;
}


int pomelo_protocol_peer_init(
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_peer_info_t * info
) {
    assert(peer != NULL);
    assert(info != NULL);
    pomelo_protocol_context_t * context = peer->context;

    peer->socket = info->socket;
    peer->created_time_ns = info->created_time_ns;

    // Acquire new codec context
    peer->crypto_ctx = pomelo_protocol_context_acquire_crypto_context(context);
    if (!peer->crypto_ctx) return -1; // Failed to acquire new crypto context

    // Initialize the disconnect task
    pomelo_sequencer_task_init(
        &peer->disconnect_task,
        (pomelo_sequencer_callback) pomelo_protocol_peer_disconnect_deferred,
        peer
    );

    return 0;
}


void pomelo_protocol_peer_cleanup(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);

    peer->extra = NULL;
    peer->client_id = 0;
    memset(&peer->address, 0, sizeof(peer->address));
    peer->socket = NULL;
    peer->state = POMELO_PROTOCOL_PEER_DISCONNECTED;
    peer->last_recv_time = 0;
    peer->last_recv_time_keep_alive = 0;
    peer->timeout_ns = 0;
    peer->sequence_number = 0;
    memset(
        peer->replay_protector.received_sequence,
        0xff,
        sizeof(peer->replay_protector.received_sequence)
    );

    // Unref the codec context
    if (peer->crypto_ctx) {
        pomelo_reference_unref(&peer->crypto_ctx->ref);
        peer->crypto_ctx = NULL;
    }

    peer->created_time_ns = 0;

    pomelo_list_clear(peer->senders);
    pomelo_list_clear(peer->receivers);

    peer->entry = NULL;
    peer->flags = 0;
    peer->remain_redundant_disconnect = 0;
    memset(peer->user_data, 0, sizeof(peer->user_data));
}


void pomelo_protocol_peer_on_free(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);

    if (peer->senders) {
        pomelo_list_destroy(peer->senders);
        peer->senders = NULL;
    }

    if (peer->receivers) {
        pomelo_list_destroy(peer->receivers);
        peer->receivers = NULL;
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
        if (delta > POMELO_REPLAY_PROTECTED_BUFFER_SIZE) return -1;
    }

    uint64_t index = sequence_number % POMELO_REPLAY_PROTECTED_BUFFER_SIZE;
    uint64_t received = protector->received_sequence[index];

    if (received == UINT64_MAX || received < sequence_number) {
        protector->received_sequence[index] = sequence_number;

        if (sequence_number > protector->most_recent_sequence) {
            protector->most_recent_sequence = sequence_number;
        }

        return 0;
    }

    return -1;
}


void pomelo_protocol_peer_cancel_senders_and_receivers(
    pomelo_protocol_peer_t * peer
) {
    assert(peer != NULL);

    // Cancel all senders
    pomelo_list_t * senders = peer->senders;
    pomelo_protocol_sender_t * sender = NULL;
    while (pomelo_list_pop_front(senders, &sender) == 0) {
        sender->entry = NULL; // The entry is already removed
        pomelo_protocol_sender_cancel(sender);
    }

    // Cancel all receivers
    pomelo_list_t * receivers = peer->receivers;
    pomelo_protocol_receiver_t * receiver = NULL;
    while (pomelo_list_pop_front(receivers, &receiver) == 0) {
        receiver->entry = NULL; // The entry is already removed
        pomelo_protocol_receiver_cancel(receiver);
    }
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
    pomelo_sequencer_submit(peer->socket->sequencer, &peer->disconnect_task);
    // => pomelo_protocol_peer_disconnect_deferred()
    return 0;
}


void pomelo_protocol_peer_disconnect_deferred(pomelo_protocol_peer_t * peer) {
    assert(peer != NULL);
    pomelo_protocol_socket_disconnect_peer(peer->socket, peer);
}
