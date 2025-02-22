#include <assert.h>
#include "pomelo/errno.h"
#include "channel.h"


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


void pomelo_channel_set_extra(pomelo_channel_t * channel, void * data) {
    assert(channel != NULL);
    pomelo_extra_set(channel->extra, data);
}


void * pomelo_channel_get_extra(pomelo_channel_t * channel) {
    assert(channel != NULL);
    return pomelo_extra_get(channel->extra);
}


int pomelo_channel_send(
    pomelo_channel_t * channel,
    pomelo_message_t * message
) {
    assert(channel != NULL);
    assert(message != NULL);

    pomelo_channel_methods_t * methods = channel->methods;
    if (!methods) {
        // Invalid channel
        return POMELO_ERR_CHANNEL_INVALID;
    }

    assert(methods->send != NULL);
    return methods->send(channel, message);
}


int pomelo_channel_set_mode(
    pomelo_channel_t * channel,
    pomelo_channel_mode mode
) {
    assert(channel != NULL);
    pomelo_channel_methods_t * methods = channel->methods;
    if (!methods) {
        // Invalid channel
        return POMELO_ERR_CHANNEL_INVALID;
    }

    assert(methods->set_mode != NULL);
    return methods->set_mode(channel, mode);
}


pomelo_channel_mode pomelo_channel_get_mode(pomelo_channel_t * channel) {
    assert(channel != NULL);
    pomelo_channel_methods_t * methods = channel->methods;
    if (!methods) {
        // Invalid channel
        return POMELO_CHANNEL_MODE_UNRELIABLE;
    }

    assert(methods->get_mode != NULL);
    return methods->get_mode(channel);
}


pomelo_session_t * pomelo_channel_get_session(pomelo_channel_t * channel) {
    assert(channel != NULL);
    return channel->session;
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

int pomelo_channel_init(
    pomelo_channel_t * channel,
    pomelo_session_t * session
) {
    assert(channel != NULL);
    assert(session != NULL);

    channel->session = session;
    pomelo_extra_set(channel->extra, NULL);
    return 0;
}


int pomelo_channel_finalize(
    pomelo_channel_t * channel,
    pomelo_session_t * session
) {
    assert(channel != NULL);
    (void) session;

    channel->session = NULL;
    channel->methods = NULL;
    return 0;
}
