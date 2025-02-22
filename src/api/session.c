#include <assert.h>
#include <string.h>
#include "pomelo/errno.h"
#include "delivery/delivery.h"
#include "session.h"
#include "message.h"
#include "socket.h"
#include "context.h"
#include "channel.h"

#ifdef POMELO_MULTI_THREAD
#define pomelo_session_signature_set(signature, value) \
    pomelo_atomic_uint64_store(&(signature), value)
#define pomelo_session_signature_get(signature)        \
    pomelo_atomic_uint64_load(&(signature))

#else // !POMELO_MULTI_THREAD
#define pomelo_session_signature_set(signature, value) (signature) = value
#define pomelo_session_signature_get(signature) signature

#endif // POMELO_MULTI_THREAD


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


void pomelo_session_set_extra(pomelo_session_t * session, void * data) {
    assert(session != NULL);
    pomelo_extra_set(session->extra, data);
}


void * pomelo_session_get_extra(pomelo_session_t * session) {
    assert(session != NULL);
    return pomelo_extra_get(session->extra);
}


int64_t pomelo_session_get_client_id(pomelo_session_t * session) {
    assert(session != NULL);
    return session->client_id;
}


pomelo_address_t * pomelo_session_get_address(pomelo_session_t * session) {
    assert(session != NULL);
    return &session->address;
}


int pomelo_session_send(
    pomelo_session_t * session,
    size_t channel_index,
    pomelo_message_t * message
) {
    assert(session != NULL);
    assert(message != NULL);

    uint64_t signature = pomelo_session_get_signature(session);
    if (signature == 0) {
        // Session has been released
        return POMELO_ERR_SESSION_INVALID;
    }

    // Get the channel
    pomelo_channel_t * channel =
        pomelo_session_get_channel(session, channel_index);
    if (!channel) {
        return POMELO_ERR_CHANNEL_INVALID;
    }

    // And send through it
    return pomelo_channel_send(channel, message);
}


uint64_t pomelo_session_get_signature(pomelo_session_t * session) {
    assert(session != NULL);
    return pomelo_session_signature_get(session->signature);
}


pomelo_socket_t * pomelo_session_get_socket(pomelo_session_t * session) {
    assert(session != NULL);
    return session->socket;
}


int pomelo_session_disconnect(pomelo_session_t * session) {
    assert(session != NULL);
    pomelo_session_methods_t * methods = session->methods;
    if (!methods) {
        // Invalid session
        return POMELO_ERR_SESSION_INVALID;
    }

    assert(methods->disconnect != NULL);
    return methods->disconnect(session);
}


int pomelo_session_get_rtt(pomelo_session_t * session, pomelo_rtt_t * rtt) {
    assert(session != NULL);
    assert(rtt != NULL);

    pomelo_session_methods_t * methods = session->methods;
    if (!methods) {
        // Invalid session
        return POMELO_ERR_SESSION_INVALID;
    }

    assert(methods->get_rtt != NULL);
    return methods->get_rtt(session, &rtt->mean, &rtt->variance);
}


pomelo_channel_t * pomelo_session_get_channel(
    pomelo_session_t * session,
    size_t channel_index
) {
    assert(session != NULL);
    pomelo_session_methods_t * methods = session->methods;
    if (!methods) {
        // Invalid session
        return NULL;
    }

    assert(methods->get_channel != NULL);
    return methods->get_channel(session, channel_index);
}


int pomelo_session_set_channel_mode(
    pomelo_session_t * session,
    size_t channel_index,
    pomelo_channel_mode mode
) {
    assert(session != NULL);
    pomelo_channel_t * channel =
        pomelo_session_get_channel(session, channel_index);
    if (!channel) {
        return POMELO_ERR_CHANNEL_INVALID;
    }

    return pomelo_channel_set_mode(channel, mode);
}


pomelo_channel_mode pomelo_session_get_channel_mode(
    pomelo_session_t * session,
    size_t channel_index
) {
    assert(session != NULL);
    pomelo_channel_t * channel =
        pomelo_session_get_channel(session, channel_index);
    if (!channel) {
        return POMELO_CHANNEL_MODE_UNRELIABLE;
    }

    return pomelo_channel_get_mode(channel);
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

int pomelo_session_init(pomelo_session_t * session, pomelo_socket_t * socket) {
    assert(session != NULL);
    assert(socket != NULL);
    memset(session, 0, sizeof(pomelo_session_t));

    session->socket = socket;
    pomelo_extra_set(session->extra, NULL);

    // Update the signature
    uint64_t signature = ++socket->session_signature_generator;
    pomelo_session_signature_set(session->signature, signature);
    return 0;
}


/// @brief FInalize base session
int pomelo_session_finalize(
    pomelo_session_t * session,
    pomelo_socket_t * socket
) {
    assert(session != NULL);
    (void) socket;

    session->socket = NULL;
    session->methods = NULL;

    // Reset the session signature to avoid sending
    pomelo_session_signature_set(session->signature, 0);
    return 0;
}
