#include <assert.h>
#include "pomelo/errno.h"
#include "session.h"
#include "channel.h"
#include "api/socket.h"



/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

pomelo_session_methods_t * pomelo_session_builtin_methods(void) {
    static bool initialized = false;
    static pomelo_session_methods_t methods;
    if (initialized) {
        return &methods;
    }

    // Update methods
    methods.disconnect = (pomelo_session_disconnect_fn)
        pomelo_session_builtin_disconnect;
    methods.get_rtt = (pomelo_session_get_rtt_fn)
        pomelo_session_builtin_get_rtt;
    methods.get_channel = (pomelo_session_get_channel_fn)
        pomelo_session_builtin_get_channel;
    methods.release = (pomelo_session_release_fn)
        pomelo_session_builtin_release;

    initialized = true;
    return &methods;
}


int pomelo_session_builtin_disconnect(pomelo_session_builtin_t * session) {
    assert(session != NULL);
    pomelo_protocol_peer_t * peer = session->peer;
    if (!peer) {
        return POMELO_ERR_SESSION_INVALID;
    }

    int ret = pomelo_protocol_peer_disconnect(peer);
    if (ret < 0) {
        return ret;
    }

    // The protocol will emit `disconnected` event after a while (abount one
    // second after). So that we dispatch here in API level to make sure higher
    // implementation will not have to wait.

    // Clear the association with protocol to make sure disconnected callback
    // will not be called anymore
    pomelo_protocol_peer_set_extra(session->peer, NULL);
    session->peer = NULL;

    pomelo_socket_t * socket = session->base.socket;
    return pomelo_platform_submit_deferred_task(
        socket->platform,
        socket->task_group,
        (pomelo_platform_task_cb) pomelo_session_builtin_on_disconnected,
        session
    );
}


int pomelo_session_builtin_get_rtt(
    pomelo_session_builtin_t * session,
    uint64_t * mean,
    uint64_t * variance
) {
    assert(session != NULL);
    pomelo_protocol_peer_t * peer = session->peer;
    if (!peer) {
        return POMELO_ERR_SESSION_INVALID;
    }

    return pomelo_protocol_peer_rtt(peer, mean, variance);
}


pomelo_channel_builtin_t * pomelo_session_builtin_get_channel(
    pomelo_session_builtin_t * session,
    size_t channel_index
) {
    assert(session != NULL);
    if (!session->channels) {
        // Invalid session
        return NULL;
    }

    pomelo_socket_t * socket = session->base.socket;
    if (channel_index >= socket->nchannels) {
        // Index is out of range
        return NULL;
    }

    return session->channels + channel_index;
}


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */


int pomelo_session_builtin_wrap(
    pomelo_session_builtin_t * session,
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_protocol_peer_t * peer
) {
    assert(session != NULL);
    assert(endpoint != NULL);
    assert(peer != NULL);

    pomelo_delivery_endpoint_set_extra(endpoint, session);
    pomelo_protocol_peer_set_extra(peer, session);

    pomelo_session_t * base = &session->base;

    session->endpoint = endpoint;
    session->peer = peer;
    base->client_id = pomelo_protocol_peer_get_client_id(peer);
    base->address = *pomelo_protocol_peer_get_address(peer);

    // Initialize all channels
    pomelo_socket_t * socket = base->socket;
    size_t nchannels = socket->nchannels;
    pomelo_channel_builtin_t * channels = session->channels;
    for (size_t i = 0; i < nchannels; ++i) {
        int ret = pomelo_channel_builtin_wrap(
            channels + i,
            pomelo_delivery_endpoint_get_bus(endpoint, i), // bus
            socket->channel_modes[i]                       // mode
        );
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}


int pomelo_session_builtin_init(
    pomelo_session_builtin_t * session,
    pomelo_socket_t * socket
) {
    assert(session != NULL);
    assert(socket != NULL);

    // Initialize base first
    pomelo_session_t * session_base = &session->base;
    int ret = pomelo_session_init(session_base, socket);
    if (ret < 0) {
        // Initialize base session failed
        return ret;
    }

    session_base->methods = pomelo_session_builtin_methods();

    // Initialize channels. Address of channels array is at the end of channel.
    pomelo_channel_builtin_t * channels =
        (pomelo_channel_builtin_t *) (session + 1);
    session->channels = channels;

    size_t nchannels = socket->nchannels;
    for (size_t i = 0; i < nchannels; i++) {
        ret = pomelo_channel_builtin_init(channels + i, session);
        if (ret < 0) {
            // Failed to initialize channel
            return ret;
        }
    }

    return 0;
}


int pomelo_session_builtin_finalize(
    pomelo_session_builtin_t * session,
    pomelo_socket_t * socket
) {
    assert(session != NULL);
    assert(socket != NULL);
    pomelo_session_t * base = &session->base;

    // Release channels
    if (session->channels) {
        // Finalize all channels
        size_t nchannels = base->socket->nchannels;
        for (size_t i = 0; i < nchannels; ++i) {
            pomelo_channel_builtin_finalize(session->channels + i, session);
        }
    }

    // Finally, finalize base
    return pomelo_session_finalize(base, socket);
}


void pomelo_session_builtin_release(pomelo_session_builtin_t * session) {
    assert(session != NULL);

    pomelo_socket_t * socket = session->base.socket;
    pomelo_pool_release(socket->builtin_session_pool, session);
}


void pomelo_session_builtin_on_disconnected(
    pomelo_session_builtin_t * session
) {
    assert(session != NULL);

    pomelo_session_t * session_base = &session->base;
    pomelo_socket_t * socket = session_base->socket;

    if (session->peer) {
        // Peer may be unset before
        pomelo_protocol_peer_set_extra(session->peer, NULL);
        session->peer = NULL;
    }
    
    // Remove session from list
    pomelo_socket_remove_session(socket, session_base);

    // Release the endpoint & session
    pomelo_delivery_transporter_release_endpoint(
        socket->transporter,
        session->endpoint
    );

    // Clear association with delivery
    pomelo_delivery_endpoint_set_extra(session->endpoint, NULL);
    session->endpoint = NULL;

    // Call the callback
    pomelo_socket_on_disconnected(socket, &session->base);

    // Finally, release the session
    pomelo_session_builtin_release(session);
}
