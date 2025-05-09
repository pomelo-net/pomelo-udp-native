#include <assert.h>
#include "protocol/protocol.h"
#include "session.h"
#include "api/socket.h"
#include "api/context.h"
#include "api/plugin/plugin.h"


void pomelo_protocol_socket_on_connected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
) {
    assert(socket != NULL);
    assert(peer != NULL);

    pomelo_socket_t * api_socket = pomelo_protocol_socket_get_extra(socket);
    if (!api_socket) return; // No associated socket

    // Acquire new session
    pomelo_session_builtin_info_t info = {
        .socket = api_socket,
        .peer = peer
    };
    pomelo_session_builtin_t * session = pomelo_pool_acquire(
        api_socket->context->builtin_session_pool,
        &info
    );
    if (!session) return; // Failed to allocate session

    // Update the state of session
    pomelo_session_t * base = &session->base;
    base->state = POMELO_SESSION_STATE_CONNECTING;

    // Add session to list (head of list)
    pomelo_socket_add_session(api_socket, base);
}


void pomelo_protocol_socket_on_disconnected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
) {
    (void) socket;
    assert(peer != NULL);

    pomelo_session_builtin_t * session = pomelo_protocol_peer_get_extra(peer);
    if (!session) return; // No associated session, discard

    pomelo_session_builtin_on_disconnected(session);
}


void pomelo_protocol_socket_on_received(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_view_t * view
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(view != NULL);

    pomelo_socket_t * api_socket = pomelo_protocol_socket_get_extra(socket);
    if (!api_socket) return; // No associated socket

    pomelo_session_builtin_t * session = pomelo_protocol_peer_get_extra(peer);
    if (!session) return; // No associated session, discard

    // Forward payload from protocol layer to delivery layer
    pomelo_delivery_endpoint_recv(session->endpoint, view);
}


void pomelo_protocol_socket_on_connect_result(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_connect_result result
) {
    assert(socket != NULL);

    pomelo_socket_t * api_socket = pomelo_protocol_socket_get_extra(socket);
    if (!api_socket) return; // No associated socket

    // Finally, call the callback
    pomelo_socket_on_connect_result(
        api_socket,
        (pomelo_socket_connect_result) result
    );
}
