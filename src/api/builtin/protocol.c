#include <assert.h>
#include "protocol/protocol.h"
#include "session.h"
#include "api/socket.h"
#include "api/plugin/plugin.h"


void pomelo_protocol_socket_on_connected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
) {
    assert(socket != NULL);
    assert(peer != NULL);

    pomelo_socket_t * api_socket = pomelo_protocol_socket_get_extra(socket);
    if (!api_socket) {
        // No associated socket
        return;
    }

    // Built-in sessions pool
    pomelo_pool_t * session_pool = api_socket->builtin_session_pool;

    // Acquire new session
    pomelo_session_builtin_t * session = pomelo_pool_acquire(session_pool);
    if (!session) {
        // Failed to allocate session
        return;
    }

    // Acquire new delivery endpoint
    pomelo_delivery_endpoint_t * endpoint =
        pomelo_delivery_transporter_acquire_endpoint(api_socket->transporter);
    if (!endpoint) {
        // Failed to acquire new endpoint
        pomelo_pool_release(session_pool, session);
        return;
    }

    // Wrap the session
    int ret = pomelo_session_builtin_wrap(session, endpoint, peer);
    if (ret < 0) {
        // Failed to wrap new session
        pomelo_pool_release(session_pool, session);
        pomelo_delivery_transporter_release_endpoint(
            api_socket->transporter,
            endpoint
        );
        return;
    }

    pomelo_session_t * session_base = &session->base;

    // Add session to list (head of list)
    pomelo_socket_add_session(api_socket, session_base);

    // Call the callback
    pomelo_socket_on_connected(api_socket, session_base);
}


void pomelo_protocol_socket_on_disconnected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
) {
    (void) socket;
    assert(peer != NULL);

    pomelo_session_builtin_t * session = pomelo_protocol_peer_get_extra(peer);
    if (!session) {
        // No associated session, discard
        return;
    }

    pomelo_session_builtin_on_disconnected(session);
}


void pomelo_protocol_socket_on_received(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(buffer != NULL);

    pomelo_socket_t * api_socket = pomelo_protocol_socket_get_extra(socket);
    if (!api_socket) {
        // No associated socket
        return;
    }

    pomelo_session_builtin_t * session = pomelo_protocol_peer_get_extra(peer);
    if (!session) {
        // No associated session, discard
        return;
    }

    // Forward payload from protocol layer to delivery layer
    pomelo_delivery_endpoint_recv(session->endpoint, buffer, offset, length);
}


void pomelo_protocol_socket_on_stopped(pomelo_protocol_socket_t * socket) {
    assert(socket != NULL);

    pomelo_socket_t * api_socket = pomelo_protocol_socket_get_extra(socket);
    if (api_socket->state == POMELO_SOCKET_STATE_STOPPING) {
        // The socket has request stopping
        pomelo_socket_process_stopped_component(
            api_socket,
            POMELO_SOCKET_COMPONENT_PROTOCOL_SOCKET
        );
    } else {
        pomelo_socket_process_stopped_component_unexpectedly(
            api_socket,
            POMELO_SOCKET_COMPONENT_PROTOCOL_SOCKET
        );
    }
}


void pomelo_protocol_socket_on_connect_result(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_connect_result result
) {
    assert(socket != NULL);

    pomelo_socket_t * api_socket = pomelo_protocol_socket_get_extra(socket);

    // Finally, call the callback
    pomelo_socket_on_connect_result(
        api_socket,
        (pomelo_socket_connect_result) result
    );
}
