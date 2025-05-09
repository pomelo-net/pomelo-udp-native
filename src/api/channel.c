#include <assert.h>
#include <string.h>
#include "pomelo/errno.h"
#include "channel.h"
#include "message.h"
#include "session.h"
#include "socket.h"

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


void pomelo_channel_send(
    pomelo_channel_t * channel,
    pomelo_message_t * message,
    void * data
) {
    assert(channel != NULL);
    assert(message != NULL);

    // Prepare the message for sending
    pomelo_message_prepare_send(message, data);

    assert(channel->methods != NULL && channel->methods->send != NULL);
    channel->methods->send(channel, message);
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
    pomelo_channel_info_t * info
) {
    assert(channel != NULL);
    assert(info != NULL);

    channel->session = info->session;
    channel->methods = info->methods;
    pomelo_extra_set(channel->extra, NULL);
    return 0;
}


void pomelo_channel_cleanup(pomelo_channel_t * channel) {
    assert(channel != NULL);
    // Call the cleanup callback
    pomelo_channel_on_cleanup(channel);

    channel->session = NULL;
    channel->methods = NULL;
}
