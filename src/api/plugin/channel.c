#include <assert.h>
#include "pomelo/errno.h"
#include "api/message.h"
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
    pomelo_session_plugin_t * session
) {
    assert(channel != NULL);
    assert(session != NULL);

    // Initialize base first
    pomelo_channel_t * channel_base = &channel->base;
    int ret = pomelo_channel_init(&channel->base, &session->base);
    if (ret < 0) {
        return ret;
    }

    channel_base->methods = pomelo_channel_plugin_methods();
    return 0;
}


int pomelo_channel_plugin_finalize(
    pomelo_channel_plugin_t * channel,
    pomelo_session_plugin_t * session
) {
    assert(channel != NULL);
    assert(session != NULL);

    return pomelo_channel_finalize(&channel->base, &session->base);
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
        return plugin->session_set_channel_mode_callback(
            &plugin->base,
            &session->base,
            channel->index,
            mode
        );
    }

    return 0;
}


pomelo_channel_mode pomelo_channel_plugin_get_mode(
    pomelo_channel_plugin_t * channel
) {
    assert(channel != NULL);
    return channel->mode;
}


int pomelo_channel_plugin_send(
    pomelo_channel_plugin_t * channel,
    pomelo_message_t * message
) {
    assert(channel != NULL);
    assert(message != NULL);

    pomelo_session_plugin_t * session = (pomelo_session_plugin_t *)
        channel->base.session;
    if (!session) {
        return POMELO_ERR_CHANNEL_INVALID;
    }

    pomelo_plugin_impl_t * plugin = session->plugin;
    if (!plugin) {
        return POMELO_ERR_CHANNEL_INVALID;
    }

    // Pack the message to make it ready to be read
    int ret = pomelo_message_pack(message);
    if (ret < 0) return ret;

    if (plugin->session_send_callback) {
        plugin->session_send_callback(
            &plugin->base,
            &session->base,
            channel->index,
            message
        );
    }

    // Finally unref the message
    pomelo_message_unref(message);
    return 0;
}
