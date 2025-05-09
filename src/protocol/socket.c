#include <assert.h>
#include <string.h>
#include "pomelo/allocator.h"
#include "utils/macro.h"
#include "peer.h"
#include "socket.h"
#include "client.h"
#include "server.h"
#include "context.h"
#include "packet.h"


/* -------------------------------------------------------------------------- */
/*                               Public API                                   */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_socket_destroy(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);
    pomelo_sequencer_submit(socket->sequencer, &socket->destroy_task);
    // => pomelo_protocol_socket_destroy_deferred()
}


int pomelo_protocol_socket_start(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);

    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_STOPPED) {
        return -1; // The socket's still running or stopping
    }

    // Start the socket
    int ret = 0;
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
    }

    if (ret == 0) {
        socket->state = POMELO_PROTOCOL_SOCKET_STATE_RUNNING;
    }

    return ret;
}


void pomelo_protocol_socket_stop(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);
    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) return;
    socket->state = POMELO_PROTOCOL_SOCKET_STATE_STOPPING;

    // Submit the stopping task to the sequencer
    pomelo_sequencer_submit(socket->sequencer, &socket->stop_task);
    // => pomelo_protocol_socket_stop_deferred
}


void * pomelo_protocol_socket_get_extra(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);
    return socket->extra;
}


void pomelo_protocol_socket_set_extra(
    pomelo_protocol_socket_t * socket,
    void * data
) {
    assert(socket != NULL);
    socket->extra = data;
}


pomelo_protocol_socket_statistic_t * pomelo_protocol_socket_statistic(
    pomelo_protocol_socket_t * socket
) {
    assert(socket != NULL);
    return &socket->statistic;
}


/* -------------------------------------------------------------------------- */
/*                               Init & Cleanup                               */
/* -------------------------------------------------------------------------- */

int pomelo_protocol_socket_on_alloc(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_context_t * context
) {
    assert(socket != NULL);
    assert(context != NULL);
    socket->context = context;
    return 0;
}


void pomelo_protocol_socket_on_free(pomelo_protocol_socket_t * socket) {
    (void) socket;
}


int pomelo_protocol_socket_init(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_socket_options_t * options
) {
    assert(socket != NULL);
    assert(options != NULL);

    if (!options->platform || !options->adapter || !options->sequencer) {
        return -1; // Invalid options
    }

    pomelo_platform_t * platform = options->platform;

    socket->extra = NULL;
    socket->platform = platform;
    socket->adapter = options->adapter;
    pomelo_adapter_set_extra(socket->adapter, socket);

    socket->sequencer = options->sequencer;

    socket->mode = options->mode;
    socket->state = POMELO_PROTOCOL_SOCKET_STATE_STOPPED;
    memset(&socket->statistic, 0, sizeof(pomelo_protocol_socket_statistic_t));
    socket->flags = options->flags;

    // Initialize the stop task
    pomelo_sequencer_task_init(
        &socket->stop_task,
        (pomelo_sequencer_callback) pomelo_protocol_socket_stop_deferred,
        socket
    );

    // Initialize the destroy task
    pomelo_sequencer_task_init(
        &socket->destroy_task,
        (pomelo_sequencer_callback) pomelo_protocol_socket_destroy_deferred,
        socket
    );

    return 0;
}


void pomelo_protocol_socket_cleanup(pomelo_protocol_socket_t * socket) {
    (void) socket;
}


void pomelo_protocol_socket_stop_deferred(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);
    socket->state = POMELO_PROTOCOL_SOCKET_STATE_STOPPED;

    // Process stopping specific mode
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_stop((pomelo_protocol_server_t *) socket);
            break;
        
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_stop((pomelo_protocol_client_t *) socket);
            break;
    }
}


void pomelo_protocol_socket_destroy_deferred(
    pomelo_protocol_socket_t * socket
) {
    assert(socket != NULL);

    // Call the specific destroy function
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_destroy((pomelo_protocol_server_t *) socket);
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_destroy((pomelo_protocol_client_t *) socket);
            break;
    }
}



/* -------------------------------------------------------------------------- */
/*                             Incoming packets                               */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_socket_recv_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
) {
    assert(socket != NULL);
    assert(packet != NULL);

    switch (packet->type) {
        case POMELO_PROTOCOL_PACKET_PAYLOAD:
            pomelo_protocol_socket_recv_payload(
                socket, peer, (pomelo_protocol_packet_payload_t *) packet
            );
            break;
            
        default:
            break;
    }

    // Pass to specific mode
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_recv_packet(
                (pomelo_protocol_server_t *) socket, peer, packet
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_recv_packet(
                (pomelo_protocol_client_t *) socket, peer, packet
            );
            break;
    }
}


void pomelo_protocol_socket_recv_failed(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_header_t * header
) {
    assert(socket != NULL);

    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_recv_failed(
                (pomelo_protocol_server_t *) socket, peer, header
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_recv_failed(
                (pomelo_protocol_client_t *) socket, peer, header
            );
            break;
    }
}


void pomelo_protocol_socket_recv_payload(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_payload_t * packet
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    // Ref the buffer to make sure the buffer will not be release in the
    // packet release function.
    pomelo_buffer_view_t * view = &packet->views[0];
    pomelo_buffer_ref(view->buffer);

    // Call the callback. Buffer will be passed to transporter
    pomelo_protocol_socket_on_received(socket, peer, view);

    // Decrease the buffer reference
    pomelo_buffer_unref(view->buffer);
}


void pomelo_protocol_socket_recv_disconnect(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_disconnect_t * packet
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_recv_disconnect(
                (pomelo_protocol_server_t *) socket, peer, packet
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_recv_disconnect(
                (pomelo_protocol_client_t *) socket, peer, packet
            );
            break;
    }
}


/* -------------------------------------------------------------------------- */
/*                             Outgoing packets                               */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_socket_sent_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
) {
    assert(socket != NULL);

    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_sent_packet(
                (pomelo_protocol_server_t *) socket, peer, packet
            );
            break;

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_sent_packet(
                (pomelo_protocol_client_t *) socket, peer, packet
            );            
            break;
    }
}


int pomelo_protocol_socket_send(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_view_t * views,
    size_t nviews
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(views != NULL);
    // Check state of peer and socket

    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) {
        return -1; // Socket is not running
    }

    if (peer->state != POMELO_PROTOCOL_PEER_CONNECTED) {
        return -1; // Invalid state
    }

    // Check views
    if (nviews == 0) return 0; // Nothing to do
    if (nviews > POMELO_PROTOCOL_PAYLOAD_MAX_VIEWS) {
        return -1; // Too many views
    }

    pomelo_protocol_context_t * context = socket->context;

    size_t length = 0;
    for (size_t i = 0; i < nviews; i++) {
        assert(views[i].buffer != NULL);
        length += views[i].length;
    }
    if (length == 0) return 0; // Nothing to do
    if (length > context->payload_capacity) {
        return -1; // Invalid payload
    }

    // Check if we need to send a keep alive packet
    if (socket->mode == POMELO_PROTOCOL_SOCKET_MODE_SERVER) {
        pomelo_protocol_server_presend_packet(
            (pomelo_protocol_server_t *) socket, peer
        );
    }

    // Acquire new payload packet
    pomelo_protocol_packet_payload_info_t info = {
        .sequence = pomelo_protocol_peer_next_sequence(peer),
        .nviews = nviews,
        .views = views
    };
    pomelo_protocol_packet_payload_t * packet = pomelo_pool_acquire(
        context->packet_pools[POMELO_PROTOCOL_PACKET_PAYLOAD],
        &info
    );
    if (!packet) return -1; // Failed to acquire packet

    // Dispatch the packet
    pomelo_protocol_socket_dispatch(socket, peer, &packet->base);
    return 0;
}


void pomelo_protocol_socket_disconnect_peer(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
) {
    assert(socket != NULL);
    assert(peer != NULL);

    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            pomelo_protocol_server_disconnect_peer(
                (pomelo_protocol_server_t *) socket, peer
            );
            break;
        
        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            pomelo_protocol_client_disconnect_peer(
                (pomelo_protocol_client_t *) socket, peer
            );
            break;
    }
}


void pomelo_protocol_socket_handle_sender_complete(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_sender_t * sender
) {
    assert(socket != NULL);
    assert(sender != NULL);

    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) {
        return; // Socket is not running, ignore
    }

    if (sender->flags & POMELO_PROTOCOL_SENDER_FLAG_FAILED) {
        return; // Sender has failed, ignore
    }

    pomelo_protocol_socket_sent_packet(socket, sender->peer, sender->packet);
}


void pomelo_protocol_socket_handle_receiver_complete(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_receiver_t * receiver
) {
    assert(socket != NULL);
    assert(receiver != NULL);

    // Check if socket is stopping
    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) {
        return; // Socket is not running, ignore
    }

    pomelo_protocol_peer_t * peer = receiver->peer;
    assert(peer != NULL);

    // Unset processing response flag
    if (receiver->packet->type == POMELO_PROTOCOL_PACKET_RESPONSE) {
        assert(peer->flags & POMELO_PEER_FLAG_PROCESSING_RESPONSE);
        peer->flags &= ~POMELO_PEER_FLAG_PROCESSING_RESPONSE;
    }

    if (receiver->flags & POMELO_PROTOCOL_RECEIVER_FLAG_FAILED) {
        socket->statistic.invalid_recv_bytes += receiver->body_view.length;
        pomelo_protocol_socket_recv_failed(socket, peer, &receiver->header);
        return; // Failed, ignore
    }

    // Validate the packet after decoding
    pomelo_protocol_packet_t * packet = receiver->packet;
    int ret = pomelo_protocol_socket_validate_packet(socket, peer, packet);
    if (ret < 0) {
        socket->statistic.invalid_recv_bytes += receiver->body_view.length;
        pomelo_protocol_socket_recv_failed(socket, peer, &receiver->header);
        return; // Failed, ignore
    }

    // Update statistic and the last received time of peer
    socket->statistic.valid_recv_bytes += receiver->body_view.length;
    peer->last_recv_time = receiver->recv_time;

    // Handle the incoming packet
    pomelo_protocol_socket_recv_packet(socket, peer, packet);
}


void pomelo_protocol_socket_accept(
    pomelo_protocol_socket_t * socket,
    pomelo_address_t * address,
    pomelo_buffer_view_t * view,
    bool encrypted
) {
    assert(socket != NULL);
    assert(address != NULL);
    assert(view != NULL);

    // Validate the incoming packet
    pomelo_protocol_packet_incoming_t incoming;
    incoming.address = address;
    incoming.view = view;
    incoming.encrypted = encrypted;

    pomelo_protocol_packet_validation_t validation;

    int ret = pomelo_protocol_socket_validate_incoming(socket, &incoming, &validation);
    if (ret < 0) {
        socket->statistic.invalid_recv_bytes += view->length;
        return; // Validate failed, ignore the packet
    }

    pomelo_protocol_peer_t * peer = validation.peer;
    pomelo_protocol_packet_header_t * header = &validation.header;

    // Acquire new receiver
    pomelo_protocol_receiver_info_t info = {
        .peer = peer,
        .header = header,
        .body_view = view,
        .flags = encrypted ? 0 : POMELO_PROTOCOL_RECEIVER_FLAG_NO_DECRYPT
    };
    pomelo_protocol_receiver_t * receiver =
        pomelo_pool_acquire(socket->context->receiver_pool, &info);
    if (!receiver) return; // Failed to acquire receiver

    // Set processing response flag
    if (header->type == POMELO_PROTOCOL_PACKET_RESPONSE) {
        assert(!(peer->flags & POMELO_PEER_FLAG_PROCESSING_RESPONSE));
        peer->flags |= POMELO_PEER_FLAG_PROCESSING_RESPONSE;
    }

    // Submit the receiver
    pomelo_protocol_receiver_submit(receiver);
    // => pomelo_protocol_socket_handle_receiver_complete
}


int pomelo_protocol_socket_validate_incoming(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_packet_incoming_t * incoming,
    pomelo_protocol_packet_validation_t * validation
) {
    assert(socket != NULL);
    assert(incoming != NULL);
    assert(validation != NULL);

    pomelo_buffer_view_t * view = incoming->view;
    bool encrypted = incoming->encrypted;

    // Check if the socket is running
    if (socket->state != POMELO_PROTOCOL_SOCKET_STATE_RUNNING) return -1;

    // Validate buffer length
    if (encrypted) {
        if (view->length < POMELO_PROTOCOL_PACKET_ENCRYPTED_MIN_CAPACITY) return -1;
    } else {
        if (view->length < POMELO_PROTOCOL_PACKET_UNENCRYPTED_MIN_CAPACITY) return -1;
    }

    // Attach buffer into a payload for reading
    pomelo_protocol_packet_header_t * header = &validation->header;

    // Read header of the packet
    int ret = pomelo_protocol_packet_header_decode(header, view);
    if (ret < 0) return -1; // Failed to decode packet header

    // Validate the packet body size
    bool valid = pomelo_protocol_packet_validate_body_length(
        header->type,
        view->length,
        encrypted
    );
    if (!valid) return -1; // Invalid packet

    // Process the incoming packet with specific mode
    switch (socket->mode) {
        case POMELO_PROTOCOL_SOCKET_MODE_SERVER:
            return pomelo_protocol_server_validate(
                (pomelo_protocol_server_t *) socket, incoming, validation
            );

        case POMELO_PROTOCOL_SOCKET_MODE_CLIENT:
            return pomelo_protocol_client_validate(
                (pomelo_protocol_client_t *) socket, incoming, validation
            );
    }

    return 0;
}


int pomelo_protocol_socket_validate_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    switch (packet->type) {
        case POMELO_PROTOCOL_PACKET_KEEP_ALIVE:
            return pomelo_protocol_socket_validate_keep_alive(
                socket, peer, (pomelo_protocol_packet_keep_alive_t *) packet
            );

        default:
            return 0;
    }
    return 0;
}


int pomelo_protocol_socket_validate_keep_alive(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_keep_alive_t * packet
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);

    if (peer->state != POMELO_PROTOCOL_PEER_CONNECTED) {
        return 0; // No keep alive checking for this state
    }

    return (packet->client_id == peer->client_id) ? 0 : -1;
}


void pomelo_protocol_socket_dispatch(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(packet != NULL);
    
    // Acquire new sender
    pomelo_protocol_context_t * context = socket->context;
    pomelo_protocol_sender_info_t info = {
        .peer = peer,
        .packet = packet,
        .flags = (socket->flags & POMELO_PROTOCOL_SOCKET_FLAG_NO_ENCRYPT)
            ? POMELO_PROTOCOL_SENDER_FLAG_NO_ENCRYPT
            : 0
    };
    pomelo_protocol_sender_t * sender =
        pomelo_pool_acquire(context->sender_pool, &info);
    if (!sender) {
        pomelo_protocol_context_release_packet(context, packet);
        return; // Failed to acquire sender
    }

    // Submit the sender
    pomelo_protocol_sender_submit(sender);
    // => pomelo_protocol_socket_handle_sender_complete
}


void pomelo_protocol_socket_dispatch_peer_disconnected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
) {
    assert(socket != NULL);
    assert(peer != NULL);
    pomelo_protocol_socket_on_disconnected(socket, peer);
}
