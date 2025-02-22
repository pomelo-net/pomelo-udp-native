#include <assert.h>
#include <string.h>
#include "pomelo/errno.h"
#include "api/socket.h"
#include "api/message.h"
#include "session.h"
#include "channel.h"


/// Bucket size for channels list of plugin session
#define POMELO_SESSION_CHANNELS_BUCKET_SIZE 64


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */

void * POMELO_PLUGIN_CALL pomelo_plugin_process_session_get_private(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
) {
    assert(plugin != NULL);
    assert(session != NULL);
    if (!plugin || !session) {
        return NULL;
    }

    return ((pomelo_session_plugin_t *) session)->private_data;
}


void POMELO_PLUGIN_CALL pomelo_plugin_handle_session_create(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    int64_t client_id,
    pomelo_address_t * address,
    void * private_data,
    void * callback_data
) {
    assert(plugin != NULL);
    assert(socket != NULL);
    assert(address != NULL);

    if (!plugin) {
        // No plugin is provided, nothing to do
        return;
    }

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    if (!socket || !address) {
        // Invalid arguments
        pomelo_plugin_process_session_create_result(
            impl,
            socket,
            /* session = */ NULL,
            callback_data,
            POMELO_PLUGIN_ERROR_INVALID_ARGS
        );
        return;
    }

    pomelo_session_plugin_create_command_t * command =
        pomelo_pool_acquire(impl->session_create_command_pool);

    if (!command) {
        // Failed to acquire new command
        pomelo_plugin_process_session_create_result(
            impl,
            socket,
            /* session = */ NULL,
            callback_data,
            POMELO_PLUGIN_ERROR_ACQUIRE_COMMAND
        );
        return;
    }

    command->plugin = impl;
    command->socket = socket;
    command->client_id = client_id;
    command->address = *address;
    command->private_data = private_data;
    command->callback_data = callback_data;

    int ret = pomelo_platform_submit_main_task(
        impl->platform,
        (pomelo_platform_task_cb) pomelo_plugin_process_session_create,
        command
    );
    if (ret < 0) {
        // Failed to submit, release the command
        pomelo_pool_release(impl->session_create_command_pool, command);
        pomelo_plugin_process_session_create_result(
            impl,
            socket,
            /* session = */ NULL,
            callback_data,
            POMELO_PLUGIN_ERROR_SUBMIT_COMMAND
        );
    }
}


void POMELO_PLUGIN_CALL pomelo_plugin_handle_session_destroy(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
) {
    assert(plugin != NULL);
    assert(session != NULL);
    if (!plugin || !session) {
        // Invalid arguments
        return;
    }

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_session_plugin_destroy_command_t * command =
        pomelo_pool_acquire(impl->session_destroy_command_pool);
    if (!command) {
        // Failed to acquire new command
        return;
    }

    command->plugin = impl;
    command->session = (pomelo_session_plugin_t *) session;
    int ret = pomelo_platform_submit_main_task(
        impl->platform,
        (pomelo_platform_task_cb) pomelo_plugin_process_session_destroy,
        command
    );
    if (ret < 0) {
        // Failed to submit, release the command
        pomelo_pool_release(impl->session_destroy_command_pool, command);
    }
}


void POMELO_PLUGIN_CALL pomelo_plugin_handle_session_receive(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    void * callback_data
) {
    assert(plugin != NULL);
    assert(session != NULL);
    if (!plugin) {
        return; // No plugin is provided, nothing to do
    }

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_session_plugin_t * plugin_session =
        (pomelo_session_plugin_t *) session;
    if (!session) {
        pomelo_plugin_process_session_receive_result(
            impl,
            plugin_session,
            channel_index,
            callback_data,
            /* message = */ NULL,
            POMELO_PLUGIN_ERROR_INVALID_ARGS
        );
        return;
    }

    pomelo_session_plugin_receive_command_t * command =
        pomelo_pool_acquire(impl->session_receive_command_pool);
    if (!command) {
        pomelo_plugin_process_session_receive_result(
            impl,
            plugin_session,
            channel_index,
            callback_data,
            /* message = */ NULL,
            POMELO_PLUGIN_ERROR_ACQUIRE_COMMAND
        );
        return;
    }

    // Update command properties
    command->plugin = impl;
    command->session = plugin_session;
    command->channel_index = channel_index;
    command->callback_data = callback_data;

    // Submit the command
    int ret = pomelo_platform_submit_main_task(
        impl->platform,
        (pomelo_platform_task_cb) pomelo_plugin_process_session_receive,
        command
    );
    if (ret < 0) {
        // Failed to submit, release the command
        pomelo_pool_release(impl->session_receive_command_pool, command);
        pomelo_plugin_process_session_receive_result(
            impl,
            plugin_session,
            channel_index,
            callback_data,
            /* message = */ NULL,
            POMELO_PLUGIN_ERROR_SUBMIT_COMMAND
        );
        return;
    }
}


uint64_t POMELO_PLUGIN_CALL pomelo_plugin_handle_session_signature(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
) {
    (void) plugin;
    assert(session != NULL);
    return pomelo_session_get_signature(session);
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
    methods.release = (pomelo_session_release_fn)
        pomelo_session_plugin_release;

    initialized = true;
    return &methods;
}


void pomelo_session_plugin_release(pomelo_session_plugin_t * session) {
    assert(session != NULL);
    pomelo_pool_release(session->plugin->session_plugin_pool, session);
}


int pomelo_session_plugin_alloc(
    pomelo_session_plugin_t * session,
    pomelo_plugin_impl_t * plugin
) {
    assert(session != NULL);
    assert(plugin != NULL);

    // Create new list of channels
    pomelo_unrolled_list_options_t options;
    pomelo_unrolled_list_options_init(&options);

    options.allocator = plugin->allocator;
    options.element_size = sizeof(pomelo_channel_plugin_t *);
    options.bucket_elements = POMELO_SESSION_CHANNELS_BUCKET_SIZE;

    session->channels = pomelo_unrolled_list_create(&options);
    if (!session->channels) {
        return -1;
    }

    session->plugin = plugin;
    return 0;
}


int pomelo_session_plugin_dealloc(
    pomelo_session_plugin_t * session,
    pomelo_plugin_impl_t * plugin
) {
    assert(session != NULL);
    (void) plugin;

    if (session->channels) {
        pomelo_unrolled_list_destroy(session->channels);
        session->channels = NULL;
    }

    session->plugin = NULL;
    return 0;
}


int pomelo_session_plugin_init(
    pomelo_session_plugin_t * session,
    pomelo_socket_t * socket
) {
    assert(session != NULL);
    assert(socket != NULL);

    int ret = pomelo_session_init(&session->base, socket);
    if (ret < 0) {
        return ret;
    }

    // Update the methods table
    session->base.methods = pomelo_session_plugin_methods();

    // Initialize channels
    pomelo_pool_t * channel_pool = session->plugin->channel_plugin_pool;
    pomelo_unrolled_list_t * channels = session->channels;
    pomelo_channel_mode * channel_modes = socket->channel_modes;
    size_t nchannels = socket->nchannels;

    for (size_t i = 0; i < nchannels; i++) {
        pomelo_channel_plugin_t * channel = pomelo_pool_acquire(channel_pool);
        if (!channel) {
            // Failed to acquire new channel
            pomelo_session_plugin_finalize(session, socket);
            return POMELO_ERR_SESSION_PLUGIN_INIT;
        }

        // Push new channel to list
        int ret = pomelo_unrolled_list_push_back(channels, channel);
        if (ret < 0) {
            pomelo_pool_release(channel_pool, channel);
            pomelo_session_plugin_finalize(session, socket);
            return POMELO_ERR_SESSION_PLUGIN_INIT;
        }

        // Initialize new channel
        ret = pomelo_channel_plugin_init(channel, session);
        if (ret < 0) {
            pomelo_session_plugin_finalize(session, socket);
            return ret;
        }

        // Update the channel information
        channel->index = i;
        channel->mode = channel_modes[i];
    }

    return 0;
}


int pomelo_session_plugin_finalize(
    pomelo_session_plugin_t * session,
    pomelo_socket_t * socket
) {
    assert(session != NULL);
    assert(socket != NULL);

    if (session->channels) {
        pomelo_pool_t * channel_pool = session->plugin->channel_plugin_pool;

        pomelo_unrolled_list_iterator_t it;
        pomelo_channel_plugin_t * channel;

        pomelo_unrolled_list_begin(session->channels, &it);
        while (pomelo_unrolled_list_iterator_next(&it, &channel)) {
            pomelo_channel_plugin_finalize(channel, session);
            pomelo_pool_acquire(channel_pool);
        }

        // Clear the list
        pomelo_unrolled_list_clear(session->channels);
    }
    
    return pomelo_session_finalize(&session->base, socket);
}


int pomelo_session_plugin_disconnect(pomelo_session_plugin_t * session) {
    assert(session != NULL);
    pomelo_plugin_impl_t * plugin = session->plugin;
    if (!plugin) {
        return POMELO_ERR_SESSION_INVALID;
    }

    if (!plugin->session_disconnect_callback) {
        // It means that nothing to do
        return 0;
    }

    plugin->session_disconnect_callback(&plugin->base, &session->base);
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

    if (!plugin->session_get_rtt_callback) {
        // No RTT is provided
        return POMELO_ERR_SESSION_INVALID;
    }

    plugin->session_get_rtt_callback(
        &plugin->base,
        &session->base,
        mean,
        variance
    );
    return 0;
}


/// @brief Get the channel by index of session
pomelo_channel_plugin_t * pomelo_session_plugin_get_channel(
    pomelo_session_plugin_t * session,
    size_t channel_index
) {
    assert(session != NULL);
    pomelo_channel_plugin_t * channel = NULL;
    pomelo_unrolled_list_get(session->channels, channel_index, &channel);
    return channel;
}


void pomelo_plugin_process_session_create(
    pomelo_session_plugin_create_command_t * command
) {
    assert(command != NULL);
    pomelo_plugin_process_session_create_ex(
        command->plugin,
        command->socket,
        command->client_id,
        &command->address,
        command->private_data,
        command->callback_data
    );

    pomelo_shared_pool_release(
        command->plugin->session_create_command_pool_release,
        command
    );
}


void pomelo_plugin_process_session_create_ex(
    pomelo_plugin_impl_t * plugin,
    pomelo_socket_t * socket,
    int64_t client_id,
    pomelo_address_t * address,
    void * private_data,
    void * callback_data
) {
    assert(plugin != NULL);
    assert(socket != NULL);
    assert(address != NULL);

    pomelo_pool_t * session_pool = plugin->session_plugin_pool;
    pomelo_session_plugin_t * session = pomelo_pool_acquire(session_pool);
    if (!session) {
        // Failed to acquire new session
        pomelo_plugin_process_session_create_result(
            plugin,
            socket,
            /* session = */ NULL,
            callback_data,
            POMELO_PLUGIN_ERROR_ACQUIRE_SESSION
        );
        return;
    }

    // Initialize base session properties
    int ret = pomelo_session_plugin_init(session, socket);
    if (ret < 0) {
        // Failed to initialize session
        pomelo_session_plugin_finalize(session, socket);
        pomelo_pool_release(session_pool, session);

        pomelo_plugin_process_session_create_result(
            plugin,
            socket,
            /* session = */ NULL,
            callback_data,
            POMELO_PLUGIN_ERROR_ACQUIRE_SESSION
        );
        return;
    }

    // Update client ID & address
    pomelo_session_t * base_session = &session->base;
    base_session->client_id = client_id;
    base_session->address = *address;

    // Set the plugin session properties
    session->private_data = private_data;
    session->plugin = plugin;

    pomelo_plugin_process_session_create_result(
        plugin,
        socket,
        session,
        callback_data,
        POMELO_PLUGIN_ERROR_OK
    );

    pomelo_session_t * session_base = &session->base;

    // Add session to socket
    pomelo_socket_add_session(socket, session_base);

    // Then call the API callback
    pomelo_socket_on_connected(socket, session_base);
}


void pomelo_plugin_process_session_create_result(
    pomelo_plugin_impl_t * plugin,
    pomelo_socket_t * socket,
    pomelo_session_plugin_t * session,
    void * callback_data,
    pomelo_plugin_error_t error
) {
    assert(plugin != NULL);

    // Call the plugin session creating callback
    if (plugin->session_create_callback) {
        plugin->session_create_callback(
            &plugin->base,
            socket,
            &session->base,
            callback_data,
            error
        );
    }
}


/// @brief Process creating session in main thread
void pomelo_plugin_process_session_destroy(
    pomelo_session_plugin_destroy_command_t * command
) {
    assert(command != NULL);
    pomelo_plugin_process_session_destroy_ex(command->plugin, command->session);
    pomelo_shared_pool_release(
        command->plugin->session_destroy_command_pool_release,
        command
    );
}


void pomelo_plugin_process_session_destroy_ex(
    pomelo_plugin_impl_t * plugin,
    pomelo_session_plugin_t * session
) {
    (void) plugin;
    assert(session != NULL);

    // Call the API callback
    pomelo_session_t * session_base = &session->base;
    pomelo_socket_t * socket = session_base->socket;

    // Remove the session from list
    pomelo_socket_remove_session(socket, session_base);

    // Dispatch on disconnected event
    pomelo_socket_on_disconnected(session_base->socket, session_base);

    // Finalize base
    pomelo_session_finalize(session_base, session_base->socket);

    // Then release
    pomelo_session_plugin_release(session);
}


void pomelo_plugin_process_session_receive_result(
    pomelo_plugin_impl_t * plugin,
    pomelo_session_plugin_t * session,
    size_t channel_index,
    void * callback_data,
    pomelo_message_t * message,
    pomelo_plugin_error_t error
) {
    assert(plugin != NULL);

    if (plugin->session_receive_callback) {
        plugin->session_receive_callback(
            &plugin->base,
            &session->base,
            channel_index,
            callback_data,
            message,
            error
        );
    }
}


void pomelo_plugin_process_session_receive(
    pomelo_session_plugin_receive_command_t * command
) {
    assert(command != NULL);
    pomelo_plugin_process_session_receive_ex(
        command->plugin,
        command->session,
        command->channel_index,
        command->callback_data
    );

    pomelo_shared_pool_release(
        command->plugin->session_receive_command_pool_release,
        command
    );
}


void pomelo_plugin_process_session_receive_ex(
    pomelo_plugin_impl_t * plugin,
    pomelo_session_plugin_t * session,
    size_t channel_index,
    void * callback_data
) {
    assert(plugin != NULL);
    assert(session != NULL);

    pomelo_message_t * message = pomelo_message_new(plugin->context);
    if (!message) {
        // Failed to acquire new message
        pomelo_plugin_process_session_receive_result(
            plugin,
            session,
            channel_index,
            callback_data,
            /* message = */ NULL,
            POMELO_PLUGIN_ERROR_ACQUIRE_MESSAGE
        );
        return;
    }

    pomelo_plugin_process_session_receive_result(
        plugin,
        session,
        channel_index,
        callback_data,
        message,
        POMELO_PLUGIN_ERROR_OK
    );

    // Pack the message to make it ready to be read
    pomelo_message_pack(message);

    // Dispatch the event
    pomelo_socket_on_received(session->base.socket, &session->base, message);

    // Then unref the message
    pomelo_message_unref(message);
}
