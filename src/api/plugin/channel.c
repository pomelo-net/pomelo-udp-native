#include <assert.h>
#include "pomelo/errno.h"
#include "api/message.h"
#include "api/socket.h"
#include "channel.h"
#include "session.h"


pomelo_channel_methods_t * pomelo_channel_plugin_methods(void) {
    static bool initialized = false;
    static pomelo_channel_methods_t methods;

    if (initialized) {
        return &methods;
    }

    methods.get_mode = (pomelo_channel_get_mode_fn)
        pomelo_channel_plugin_get_mode;
    methods.set_mode = (pomelo_channel_set_mode_fn)
        pomelo_channel_plugin_set_mode;
    methods.send = (pomelo_channel_send_fn) pomelo_channel_plugin_send;

    initialized = true;
    return &methods;
}


int pomelo_channel_plugin_init(
    pomelo_channel_plugin_t * channel,
    pomelo_channel_plugin_info_t * info
) {
    assert(channel != NULL);
    assert(info != NULL);

    channel->index = info->index;
    channel->mode = info->mode;

    pomelo_channel_t * channel_base = &channel->base;
    pomelo_channel_info_t info_base = {
        .session = &info->session->base,
        .methods = pomelo_channel_plugin_methods(),
    };
    int ret = pomelo_channel_init(channel_base, &info_base);
    if (ret < 0) return ret; // Failed to initialize channel

    return 0;
}


void pomelo_channel_plugin_cleanup(pomelo_channel_plugin_t * channel) {
    assert(channel != NULL);
    pomelo_channel_cleanup(&channel->base);
}


int pomelo_channel_plugin_set_mode(
    pomelo_channel_plugin_t * channel,
    pomelo_channel_mode mode
) {
    assert(channel != NULL);
    channel->mode = mode;

    pomelo_session_plugin_t * session = (pomelo_session_plugin_t *)
        channel->base.session;
    if (!session) {
        return POMELO_ERR_CHANNEL_INVALID;
    }

    pomelo_plugin_impl_t * plugin = session->plugin;
    if (!plugin) {
        return POMELO_ERR_CHANNEL_INVALID;
    }

    if (plugin->session_set_channel_mode_callback) {
        int ret = plugin->session_set_channel_mode_callback(
            &plugin->base,
            &session->base,
            channel->index,
            mode
        );
        pomelo_plugin_post_callback_cleanup(plugin);
        return ret;
    }

    return 0;
}


pomelo_channel_mode pomelo_channel_plugin_get_mode(
    pomelo_channel_plugin_t * channel
) {
    assert(channel != NULL);
    return channel->mode;
}


void pomelo_channel_plugin_send(
    pomelo_channel_plugin_t * channel,
    pomelo_message_t * message
) {
    assert(channel != NULL);
    assert(message != NULL);

    pomelo_session_plugin_t * session = (pomelo_session_plugin_t *)
        channel->base.session;
    assert(session != NULL);
    
    pomelo_socket_t * socket = session->base.socket;
    assert(socket != NULL);
    
    pomelo_plugin_impl_t * plugin = session->plugin;
    assert(plugin != NULL);


    if (plugin->session_on_send_callback) {
        // Pack the message to make it ready to be read
        pomelo_message_pack(message);
        plugin->session_on_send_callback(
            &plugin->base,
            &session->base,
            channel->index,
            message
        );
        pomelo_message_unpack(message);
        pomelo_plugin_post_callback_cleanup(plugin);
    }

    message->nsent += 1;
    pomelo_socket_dispatch_send_result(socket, message);
}
