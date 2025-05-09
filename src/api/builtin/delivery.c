#include <assert.h>
#include "pomelo/errno.h"
#include "delivery/delivery.h"
#include "session.h"
#include "api/socket.h"
#include "api/message.h"
#include "api/channel.h"
#include "api/context.h"


int pomelo_delivery_endpoint_send(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_buffer_view_t * views,
    size_t nviews
) {
    assert(endpoint != NULL);
    assert(views != NULL);
    assert(nviews > 0);

    pomelo_session_builtin_t * session =
        pomelo_delivery_endpoint_get_extra(endpoint);
    if (!session) return -1; // No associated session
    return pomelo_protocol_peer_send(session->peer, views, nviews);
}


void pomelo_delivery_endpoint_on_ready(pomelo_delivery_endpoint_t * endpoint) {
    assert(endpoint != NULL);

    pomelo_session_builtin_t * session =
        pomelo_delivery_endpoint_get_extra(endpoint);
    if (!session) return; // No associated session
    session->ready = true;

    pomelo_session_t * base = &session->base;

    // Update the state of session
    base->state = POMELO_SESSION_STATE_CONNECTED;

    // Call the callback.
    pomelo_socket_on_connected(base->socket, base);
}


void pomelo_delivery_bus_on_received(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode
) {
    assert(bus != NULL);
    assert(parcel != NULL);
    (void) mode; // Not used

    pomelo_delivery_endpoint_t * endpoint =
        pomelo_delivery_bus_get_endpoint(bus);
    assert(endpoint != NULL);

    pomelo_session_builtin_t * session =
        pomelo_delivery_endpoint_get_extra(endpoint);
    if (!session) return; // No associated session
    assert(session->ready); // Session must be ready

    pomelo_socket_t * socket = session->base.socket;
    pomelo_context_t * context = socket->context;

    // Wrap the parcel to message
    pomelo_message_info_t info = {
        .context = context,
        .mode = POMELO_MESSAGE_MODE_READ,
        .parcel = parcel
    };
    pomelo_message_t * message =
        pomelo_context_acquire_message_ex(context, &info);
    if (!message) return; // Failed to acquire api message

    // Call the callback
    pomelo_socket_on_received(socket, &session->base, message);

    // Unref the message
    pomelo_message_unref(message);
}


void pomelo_delivery_sender_on_result(
    pomelo_delivery_sender_t * sender,
    pomelo_delivery_parcel_t * parcel,
    size_t transmission_count
) {
    assert(sender != NULL);
    assert(parcel != NULL);

    pomelo_message_t * message = pomelo_delivery_parcel_get_extra(parcel);
    pomelo_socket_t * socket = pomelo_delivery_sender_get_extra(sender);
    if (!message || !socket) return; // Invalid params

    message->nsent += transmission_count;
    pomelo_socket_dispatch_send_result(socket, message);
}
