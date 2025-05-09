#include <assert.h>
#include <string.h>
#include "pomelo/errno.h"
#include "api/message.h"
#include "api/session.h"
#include "api/socket.h"
#include "api/channel.h"
#include "api/context.h"
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
    pomelo_channel_builtin_info_t * info
) {
    assert(channel != NULL);
    assert(info != NULL);

    // Initialize base channel
    pomelo_channel_info_t base_info = {
        .session = &info->session->base,
        .methods = pomelo_channel_builtin_methods()
    };
    int ret = pomelo_channel_init(&channel->base, &base_info);
    if (ret < 0) return ret; // Failed to initialize base channel

    // Initialize bus & mode
    channel->bus = info->bus;
    pomelo_delivery_bus_set_extra(channel->bus, channel);

    channel->mode = info->mode;

    return 0;
}


void pomelo_channel_builtin_cleanup(pomelo_channel_builtin_t * channel) {
    assert(channel != NULL);

    pomelo_delivery_bus_set_extra(channel->bus, NULL);
    channel->bus = NULL;

    channel->mode = POMELO_CHANNEL_MODE_UNRELIABLE;
    pomelo_channel_cleanup(&channel->base);
}


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


void pomelo_channel_builtin_send(
    pomelo_channel_builtin_t * channel,
    pomelo_message_t * message
) {
    assert(channel != NULL);
    assert(message != NULL);

    pomelo_delivery_bus_t * bus = channel->bus;
    assert(bus != NULL);

    pomelo_socket_t * socket = channel->base.session->socket;
    assert(socket != NULL);

    // Update the message context
    pomelo_message_set_context(message, socket->context);

    // Create sender
    pomelo_delivery_sender_options_t options = {
        .context = socket->context->delivery_context,
        .parcel = message->parcel,
        .platform = socket->platform
    };
    pomelo_delivery_sender_t * sender = pomelo_delivery_sender_create(&options);
    if (!sender) {
        // Failed to create sender
        pomelo_socket_dispatch_send_result(socket, message);
        return;
    }
    pomelo_delivery_sender_set_extra(sender, socket);

    int ret = pomelo_delivery_sender_add_transmission(
        sender, bus, (pomelo_delivery_mode) channel->mode
    );
    if (ret < 0) {
        // Failed to add recipient
        pomelo_socket_dispatch_send_result(socket, message);
        pomelo_delivery_sender_cancel(sender);
        return;
    }
    
    pomelo_delivery_sender_submit(sender);
}
