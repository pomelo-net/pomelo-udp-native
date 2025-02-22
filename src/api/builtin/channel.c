#include <assert.h>
#include <string.h>
#include "pomelo/errno.h"
#include "api/message.h"
#include "api/session.h"
#include "api/socket.h"
#include "api/channel.h"
#include "channel.h"
#include "session.h"


pomelo_channel_methods_t * pomelo_channel_builtin_methods(void) {
    static bool initialized = false;
    static pomelo_channel_methods_t methods;
    if (initialized) {
        return &methods;
    }

    // Update methods
    methods.send = (pomelo_channel_send_fn) pomelo_channel_builtin_send;
    methods.set_mode = (pomelo_channel_set_mode_fn)
        pomelo_channel_builtin_set_mode;
    methods.get_mode = (pomelo_channel_get_mode_fn)
        pomelo_channel_builtin_get_mode;

    initialized = true;
    return &methods;
}


int pomelo_channel_builtin_init(
    pomelo_channel_builtin_t * channel,
    pomelo_session_builtin_t * session
) {
    assert(channel != NULL);
    assert(session != NULL);

    int ret = pomelo_channel_init(&channel->base, &session->base);
    if (ret < 0) {
        return ret;
    }

    // Update methods table
    channel->base.methods = pomelo_channel_builtin_methods();
    
    channel->bus = NULL;
    channel->mode = POMELO_CHANNEL_MODE_UNRELIABLE;

    return 0;
}


int pomelo_channel_builtin_wrap(
    pomelo_channel_builtin_t * channel,
    pomelo_delivery_bus_t * bus,
    pomelo_channel_mode mode
) {
    assert(channel != NULL);
    assert(bus != NULL);

    channel->bus = bus;
    channel->mode = mode;

    return 0;
}



int pomelo_channel_builtin_finalize(
    pomelo_channel_builtin_t * channel,
    pomelo_session_builtin_t * session
) {
    assert(channel != NULL);

    channel->bus = NULL;
    channel->mode = POMELO_CHANNEL_MODE_UNRELIABLE;

    return pomelo_channel_finalize(&channel->base, &session->base);
}


/// @brief Change the delivery mode of a channel
int pomelo_channel_builtin_set_mode(
    pomelo_channel_builtin_t * channel,
    pomelo_channel_mode mode
) {
    assert(channel != NULL);
    if (mode < 0 || mode >= POMELO_CHANNEL_MODE_COUNT) {
        return POMELO_ERR_CHANNEL_INVALID_ARG;
    }

    channel->mode = mode;
    return 0;
}


pomelo_channel_mode pomelo_channel_builtin_get_mode(
    pomelo_channel_builtin_t * channel
) {
    assert(channel != NULL);
    return channel->mode;
}



int pomelo_channel_builtin_send(
    pomelo_channel_builtin_t * channel,
    pomelo_message_t * message
) {
    assert(channel != NULL);
    assert(message != NULL);

    pomelo_delivery_bus_t * bus = channel->bus;
    if (!bus) {
        // Invalid channel
        return POMELO_ERR_CHANNEL_INVALID;
    }

    // Update the message context
    pomelo_message_set_context(message, channel->base.session->socket->context);

    pomelo_delivery_mode mode = (pomelo_delivery_mode) channel->mode;
    int ret = pomelo_delivery_bus_send(bus, message->parcel, mode);

    // Unreference the message after sending
    if (ret == 0) {
        // Only unref the message when sending is successful
        pomelo_message_unref(message);
    }
    return ret;
}
