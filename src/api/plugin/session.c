#include <assert.h>
#include <string.h>
#include "pomelo/errno.h"
#include "api/socket.h"
#include "api/message.h"
#include "api/context.h"
#include "session.h"
#include "channel.h"


/// @brief The initial size of channels array
#define POMELO_PLUGIN_SESSION_CHANNELS_INITIAL_SIZE 128


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */


void POMELO_PLUGIN_CALL pomelo_plugin_session_set_private(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    void * data
) {
    assert(plugin != NULL);
    assert(session != NULL);
    if (!plugin || !session) return; // Invalid arguments
    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    pomelo_extra_set(((pomelo_session_plugin_t *) session)->private_data, data);
}


void * POMELO_PLUGIN_CALL pomelo_plugin_session_get_private(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
) {
    assert(plugin != NULL);
    assert(session != NULL);
    if (!plugin || !session) return NULL; // Invalid arguments
    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    return pomelo_extra_get(
        ((pomelo_session_plugin_t *) session)->private_data
    );
}


pomelo_session_t * POMELO_PLUGIN_CALL pomelo_plugin_session_create(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    int64_t client_id,
    pomelo_address_t * address
) {
    assert(plugin != NULL);
    assert(socket != NULL);
    assert(address != NULL);
    if (!plugin || !socket || !address) return NULL; // Invalid arguments

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    // Acquire new session
    pomelo_session_plugin_info_t info = {
        .socket = socket,
        .plugin = impl
    };
    pomelo_session_plugin_t * session =
        pomelo_pool_acquire(impl->context->plugin_session_pool, &info);
    if (!session) return NULL; // Failed to acquire new session

    // Update client ID & address
    pomelo_session_t * base = &session->base;
    base->client_id = client_id;
    base->address = *address;
    base->state = POMELO_SESSION_STATE_CONNECTED;

    // Add session to socket
    pomelo_socket_add_session(socket, base);

    // Then call the API callback
    pomelo_socket_on_connected(socket, base);
    return base;
}


void POMELO_PLUGIN_CALL pomelo_plugin_session_destroy(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
) {
    assert(plugin != NULL);
    assert(session != NULL);
    if (!plugin || !session) return; // Invalid arguments

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    // Call the API callback
    pomelo_socket_t * socket = session->socket;

    session->state = POMELO_SESSION_STATE_DISCONNECTED;
    pomelo_sequencer_submit(
        &socket->sequencer,
        &((pomelo_session_plugin_t *) session)->destroy_task
    );
    // => pomelo_session_plugin_destroy_deferred()
}


void POMELO_PLUGIN_CALL pomelo_plugin_session_receive(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    pomelo_message_t * message
) {
    assert(plugin != NULL);
    assert(session != NULL);
    (void) channel_index;

    if (!plugin || !session) return; // Invalid arguments

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    // Pack the message to make it ready to be read
    pomelo_message_pack(message);

    // Dispatch the event
    pomelo_socket_on_received(session->socket, session, message);
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


pomelo_session_methods_t * pomelo_session_plugin_methods(void) {
    static bool initialized = false;
    static pomelo_session_methods_t methods;

    if (initialized) {
        return &methods;
    }

    // Update methods
    methods.disconnect = (pomelo_session_disconnect_fn)
        pomelo_session_plugin_disconnect;
    methods.get_rtt = (pomelo_session_get_rtt_fn)
        pomelo_session_plugin_get_rtt;
    methods.get_channel = (pomelo_session_get_channel_fn)
        pomelo_session_plugin_get_channel;

    initialized = true;
    return &methods;
}


int pomelo_session_plugin_on_alloc(
    pomelo_session_plugin_t * session,
    pomelo_context_t * context
) {
    assert(session != NULL);
    assert(context != NULL);

    int ret = pomelo_session_on_alloc(&session->base, context);
    if (ret < 0) return ret;

    // Create new list of channels
    pomelo_array_options_t array_options = {
        .allocator = context->allocator,
        .element_size = sizeof(pomelo_channel_plugin_t *),
        .initial_capacity = POMELO_PLUGIN_SESSION_CHANNELS_INITIAL_SIZE
    };
    session->channels = pomelo_array_create(&array_options);
    if (!session->channels) return -1; // Failed to create new array of channels

    return 0;
}


void pomelo_session_plugin_on_free(pomelo_session_plugin_t * session) {
    assert(session != NULL);

    if (session->channels) {
        pomelo_array_destroy(session->channels);
        session->channels = NULL;
    }

    // Free the session
    pomelo_session_on_free(&session->base);
}


int pomelo_session_plugin_init(
    pomelo_session_plugin_t * session,
    pomelo_session_plugin_info_t * info
) {
    assert(session != NULL);
    assert(info != NULL);

    // Initialize base session
    pomelo_socket_t * socket = info->socket;
    pomelo_session_info_t base_info = {
        .type = POMELO_SESSION_TYPE_PLUGIN,
        .socket = socket,
        .methods = pomelo_session_plugin_methods()
    };
    int ret = pomelo_session_init(&session->base, &base_info);
    if (ret < 0) return ret; // Failed to initialize base session

    // Update the methods table
    session->plugin = info->plugin;
    pomelo_extra_set(session->private_data, NULL);

    // Initialize the destroy task
    pomelo_sequencer_task_init(
        &session->destroy_task,
        (pomelo_sequencer_callback) pomelo_session_plugin_destroy_deferred,
        session
    );

    // Initialize channels
    pomelo_pool_t * channel_pool = socket->context->plugin_channel_pool;
    pomelo_array_t * channels = session->channels;
    pomelo_array_t * channel_modes = socket->channel_modes;
    size_t nchannels = channel_modes->size;
    pomelo_channel_mode mode = POMELO_CHANNEL_MODE_UNRELIABLE;

    // Resize the channels array
    ret = pomelo_array_resize(channels, nchannels);
    if (ret < 0) return ret; // Failed to resize channels array
    pomelo_array_fill_zero(channels); // Fill the array with zeros

    // Initialize all channels
    for (size_t i = 0; i < nchannels; i++) {
        pomelo_array_get(channel_modes, i, &mode);
        pomelo_channel_plugin_info_t info = {
            .session = session,
            .index = i,
            .mode = mode
        };
        pomelo_channel_plugin_t * channel =
            pomelo_pool_acquire(channel_pool, &info);
        if (!channel) return -1; // Failed to acquire new channel

        // Set the channel to array
        pomelo_array_set(channels, i, channel);
    }

    return 0;
}


void pomelo_session_plugin_cleanup(pomelo_session_plugin_t * session) {
    assert(session != NULL);

    pomelo_pool_t * channel_pool = session->base.context->plugin_channel_pool;
    pomelo_array_t * channels = session->channels;

    // Release all channels
    for (size_t i = 0; i < channels->size; i++) {
        pomelo_channel_plugin_t * channel = NULL;
        pomelo_array_get(channels, i, &channel);
        if (channel) {
            pomelo_pool_release(channel_pool, channel);
        }
    }
    pomelo_array_clear(channels);
    
    // Cleanup the base session
    pomelo_session_cleanup(&session->base);
}


int pomelo_session_plugin_disconnect(pomelo_session_plugin_t * session) {
    assert(session != NULL);
    pomelo_plugin_impl_t * plugin = session->plugin;
    if (!plugin) {
        return POMELO_ERR_SESSION_INVALID;
    }

    if (plugin->session_disconnect_callback) {
        plugin->session_disconnect_callback(&plugin->base, &session->base);
        pomelo_plugin_post_callback_cleanup(plugin);
    }

    return 0;
}


int pomelo_session_plugin_get_rtt(
    pomelo_session_plugin_t * session,
    uint64_t * mean,
    uint64_t * variance
) {
    assert(session != NULL);

    pomelo_plugin_impl_t * plugin = session->plugin;
    if (!plugin) {
        return POMELO_ERR_SESSION_INVALID;
    }

    if (plugin->session_get_rtt_callback) {
        // No RTT is provided
        return POMELO_ERR_SESSION_INVALID;
    }

    plugin->session_get_rtt_callback(
        &plugin->base,
        &session->base,
        mean,
        variance
    );
    pomelo_plugin_post_callback_cleanup(plugin);

    return 0;
}


/// @brief Get the channel by index of session
pomelo_channel_plugin_t * pomelo_session_plugin_get_channel(
    pomelo_session_plugin_t * session,
    size_t channel_index
) {
    assert(session != NULL);
    pomelo_channel_plugin_t * channel = NULL;
    pomelo_array_get(session->channels, channel_index, &channel);
    return channel;
}


void pomelo_session_plugin_destroy_deferred(pomelo_session_plugin_t * session) {
    assert(session != NULL);
    pomelo_socket_t * socket = session->base.socket;
    pomelo_context_t * context = socket->context;

    // Emit the disconnected event
    pomelo_socket_on_disconnected(socket, &session->base);

    // Remove session from list
    pomelo_socket_remove_session(socket, &session->base);
    pomelo_pool_release(context->plugin_session_pool, session);
}
