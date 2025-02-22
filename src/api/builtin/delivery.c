#include <assert.h>
#include "pomelo/errno.h"
#include "delivery/delivery.h"
#include "session.h"
#include "api/socket.h"
#include "api/message.h"


int pomelo_delivery_endpoint_send(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
) {
    assert(endpoint != NULL);
    assert(buffer != NULL);

    pomelo_session_builtin_t * session =
        pomelo_delivery_endpoint_get_extra(endpoint);
    if (!session) {
        // No associated session
        return POMELO_ERR_SESSION_INVALID;
    }

    return pomelo_protocol_peer_send(session->peer, buffer, offset, length);
}


int pomelo_delivery_endpoint_rtt(
    pomelo_delivery_endpoint_t * endpoint,
    uint64_t * mean,
    uint64_t * variance
) {
    pomelo_session_builtin_t * session =
        pomelo_delivery_endpoint_get_extra(endpoint);
    if (!session) {
        // No associated session
        return POMELO_ERR_SESSION_INVALID;
    }

    return pomelo_protocol_peer_rtt(session->peer, mean, variance);
}


void pomelo_delivery_bus_on_received(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel
) {
    assert(bus != NULL);
    assert(parcel != NULL);

    pomelo_delivery_endpoint_t * endpoint =
        pomelo_delivery_bus_get_endpoint(bus);

    pomelo_session_t * session = pomelo_delivery_endpoint_get_extra(endpoint);
    if (!session) {
        // No associated session
        return;
    }

    pomelo_context_t * context = session->socket->context;
    pomelo_message_t * api_message = pomelo_message_wrap(context, parcel);
    
    if (!api_message) {
        // Failed to acquire api message
        return;
    }

    // Call the callback
    pomelo_socket_on_received(session->socket, session, api_message);

    // Release message
    pomelo_message_unref(api_message);
}


void pomelo_delivery_transporter_on_stopped(
    pomelo_delivery_transporter_t * transporter
) {
    assert(transporter != NULL);
    pomelo_socket_t * socket =
        pomelo_delivery_transporter_get_extra(transporter);

    if (socket->state == POMELO_SOCKET_STATE_STOPPING) {
        // Process transporter stopped component
        pomelo_socket_process_stopped_component(
            socket,
            POMELO_SOCKET_COMPONENT_DELIVERY_TRANSPORTER
        );
    } else {
        pomelo_socket_process_stopped_component_unexpectedly(
            socket,
            POMELO_SOCKET_COMPONENT_DELIVERY_TRANSPORTER
        );
    }
}
