#include <assert.h>
#include <string.h>
#include "pomelo/errno.h"
#include "socket.h"
#include "context.h"
#include "session.h"
#include "message.h"
#include "channel.h"
#include "protocol/socket.h"
#include "protocol/client.h"
#include "protocol/server.h"
#include "builtin/session.h"
#include "builtin/channel.h"
#include "plugin/plugin.h"
#include "plugin/socket.h"


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


void pomelo_socket_options_init(pomelo_socket_options_t * options) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_socket_options_t));
}


pomelo_socket_t * pomelo_socket_create(pomelo_socket_options_t * options) {
    assert(options != NULL);

    if (!options->context) {
        // No context is provided
        return NULL;
    }

    if (options->nchannels == 0 || options->nchannels > POMELO_MAX_CHANNELS) {
        // Invalid number of channels
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_socket_t * socket =
        pomelo_allocator_malloc_t(allocator, pomelo_socket_t);
    if (!socket) {
        return NULL;
    }
    memset(socket, 0, sizeof(pomelo_socket_t));

    pomelo_extra_set(socket->extra, NULL);
    socket->allocator = allocator;
    socket->context = options->context;
    socket->platform = options->platform;
    socket->nchannels = options->nchannels;

    socket->task_group = pomelo_platform_acquire_task_group(socket->platform);
    if (!socket->task_group) {
        pomelo_socket_destroy(socket);
        return NULL;
    }

    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_session_t *);
    socket->sessions = pomelo_list_create(&list_options);
    if (!socket->sessions) {
        pomelo_socket_destroy(socket);
        return NULL;
    }

    // Allocate channel template array
    size_t channel_modes_size =
        options->nchannels * sizeof(pomelo_channel_mode);
    socket->channel_modes = pomelo_allocator_malloc(
        allocator,
        channel_modes_size
    );
    if (!socket->channel_modes) {
        pomelo_socket_destroy(socket);
        return NULL;
    }

    // Initialize channel template array
    if (options->channel_modes) {
        memcpy(
            socket->channel_modes,
            options->channel_modes,
            channel_modes_size
        );
    } else {
        memset(socket->channel_modes, 0, channel_modes_size);
    }

    // The protocol socket will be created later
    socket->protocol_socket = NULL;

    // Create transporter
    pomelo_delivery_transporter_options_t transporter_options;
    pomelo_delivery_transporter_options_init(&transporter_options);
    transporter_options.allocator = allocator;
    transporter_options.nbuses = options->nchannels;
    transporter_options.context = options->context->delivery_context;
    transporter_options.platform = options->platform;

    socket->transporter =
        pomelo_delivery_transporter_create(&transporter_options);
    if (!socket->transporter) {
        pomelo_socket_destroy(socket);
        return NULL;
    }

    // Set the socket as the extra data of transporter
    pomelo_delivery_transporter_set_extra(socket->transporter, socket);

    // Create session pool
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);

    pool_options.allocator = allocator;

    // Session itself and its channels
    pool_options.element_size = (
        sizeof(pomelo_session_builtin_t) +
        options->nchannels * sizeof(pomelo_channel_builtin_t)
    );

    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_session_builtin_init;
    pool_options.release_callback = (pomelo_pool_callback)
        pomelo_session_builtin_finalize;
    pool_options.zero_initialized = true;
    pool_options.callback_context = socket;

    socket->builtin_session_pool = pomelo_pool_create(&pool_options);
    if (!socket->builtin_session_pool) {
        pomelo_socket_destroy(socket);
        return NULL;
    }

    // Create attached plugins list
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_plugin_impl_t *);
    socket->attached_plugins = pomelo_list_create(&list_options);
    if (!socket->attached_plugins) {
        pomelo_socket_destroy(socket);
        return NULL;
    }

    // Create detaching commands pool
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.synchronized = true;
    pool_options.element_size = sizeof(pomelo_plugin_socket_detach_command_t);
    socket->detach_commands_pool = pomelo_pool_create(&pool_options);
    if (!socket->detach_commands_pool) {
        pomelo_socket_destroy(socket);
        return NULL;
    }

    // Dispatch to plugins
    pomelo_plugin_dispatch_socket_on_created(socket);

    return socket;
}


void pomelo_socket_destroy(pomelo_socket_t * socket) {
    assert(socket != NULL);

    // Dispatch to plugins
    pomelo_plugin_dispatch_socket_on_destroyed(socket);

    if (socket->protocol_socket) {
        pomelo_protocol_socket_destroy(socket->protocol_socket);
        socket->protocol_socket = NULL;
    }

    if (socket->transporter) {
        pomelo_delivery_transporter_destroy(socket->transporter);
        socket->transporter = NULL;
    }

    if (socket->builtin_session_pool) {
        pomelo_pool_destroy(socket->builtin_session_pool);
        socket->builtin_session_pool = NULL;
    }

    if (socket->channel_modes) {
        pomelo_allocator_free(socket->allocator, socket->channel_modes);
        socket->channel_modes = NULL;
    }

    if (socket->attached_plugins) {
        pomelo_list_destroy(socket->attached_plugins);
        socket->attached_plugins = NULL;
    }

    if (socket->detach_commands_pool) {
        pomelo_pool_destroy(socket->detach_commands_pool);
        socket->detach_commands_pool = NULL;
    }

    if (socket->task_group) {
        pomelo_platform_release_task_group(
            socket->platform,
            socket->task_group
        );
        socket->task_group = NULL;
    }

    if (socket->sessions) {
        pomelo_list_destroy(socket->sessions);
        socket->sessions = NULL;
    }

    pomelo_allocator_free(socket->allocator, socket);
}


void pomelo_socket_set_extra(pomelo_socket_t * socket, void * data) {
    assert(socket != NULL);
    pomelo_extra_set(socket->extra, data);
}


void * pomelo_socket_get_extra(pomelo_socket_t * socket) {
    assert(socket != NULL);
    return pomelo_extra_get(socket->extra);
}


int pomelo_socket_connect(pomelo_socket_t * socket, uint8_t * connect_token) {
    assert(socket != NULL);
    assert(connect_token != NULL);

    if (socket->state != POMELO_SOCKET_STATE_STOPPED) {
        // It is not stopped
        return POMELO_ERR_SOCKET_ILLEGAL_STATE;
    }

    // Create protocol socket first
    pomelo_protocol_client_options_t client_options;
    pomelo_protocol_client_options_init(&client_options);
    
    client_options.allocator = socket->allocator;
    client_options.connect_token = connect_token;
    client_options.context = socket->context->protocol_context;
    client_options.platform = socket->platform;

    socket->protocol_socket = pomelo_protocol_client_create(&client_options);
    if (!socket->protocol_socket) {
        // Failed to create client
        return POMELO_ERR_SOCKET_CONNECT;
    }

    // Set the socket as the extra data of protocol socket
    pomelo_protocol_socket_set_extra(socket->protocol_socket, socket);

    // Finally, start the socket
    int ret = pomelo_protocol_socket_start(socket->protocol_socket);

    if (ret == 0) {
        // Change the state to running
        socket->state = POMELO_SOCKET_STATE_RUNNING_CLIENT;
        socket->stopped_components = 0; // Reset the stopped components flag
    }

    // Dispatch to plugins
    pomelo_plugin_dispatch_socket_on_connecting(socket, connect_token);
    return ret;
}


int pomelo_socket_listen(
    pomelo_socket_t * socket,
    uint8_t * private_key,
    uint64_t protocol_id,
    size_t max_clients,
    pomelo_address_t * address
) {
    assert(socket != NULL);
    assert(private_key != NULL);
    assert(address != NULL);

    if (socket->state != POMELO_SOCKET_STATE_STOPPED) {
        // It is still running
        return POMELO_ERR_SOCKET_ILLEGAL_STATE;
    }

    if (max_clients == 0) {
        // Invalid argument
        return POMELO_ERR_SOCKET_INVALID_ARG;
    }

    pomelo_protocol_server_options_t server_options;
    pomelo_protocol_server_options_init(&server_options);

    server_options.allocator = socket->allocator;
    server_options.context = socket->context->protocol_context;
    server_options.platform = socket->platform;
    server_options.max_clients = max_clients;
    server_options.private_key = private_key;
    server_options.protocol_id = protocol_id;
    server_options.address = *address;

    socket->protocol_socket = pomelo_protocol_server_create(&server_options);
    if (!socket->protocol_socket) {
        return POMELO_ERR_SOCKET_LISTEN;
    }

    // Set the socket as the extra data of protocol socket
    pomelo_protocol_socket_set_extra(socket->protocol_socket, socket);

    // Start the socket
    int ret = pomelo_protocol_socket_start(socket->protocol_socket);
    if (ret < 0) return ret;

    // Change the state
    socket->state = POMELO_SOCKET_STATE_RUNNING_SERVER;
    socket->stopped_components = 0; // Reset the stopped components flag
    memcpy(socket->private_key, private_key, POMELO_KEY_BYTES);
    
    // Dispatch to plugins
    pomelo_plugin_dispatch_socket_on_listening(socket, address);

    return 0;
}


int pomelo_socket_stop(pomelo_socket_t * socket) {
    assert(socket != NULL);

    if (socket->state == POMELO_SOCKET_STATE_STOPPED ||
        socket->state == POMELO_SOCKET_STATE_STOPPING)
    {
        // The socket is not running now
        return POMELO_ERR_SOCKET_ILLEGAL_STATE;
    }

    // Set stopping state
    socket->state = POMELO_SOCKET_STATE_STOPPING;

    // Stop the socket and transporter
    pomelo_protocol_socket_stop(socket->protocol_socket);
    // => pomelo_protocol_socket_on_stopped

    pomelo_delivery_transporter_stop(socket->transporter);
    // => pomelo_delivery_transporter_on_stopped

    // Cancel all pending tasks
    pomelo_platform_cancel_task_group(
        socket->platform,
        socket->task_group,
        NULL,
        NULL
    );
    
    // Dispatch plugin event
    pomelo_plugin_stop_socket(socket);

    // Release all sessions and clear the sessions list
    pomelo_session_t * session = NULL;
    pomelo_list_for(socket->sessions, session, pomelo_session_t *, {
        pomelo_session_methods_t * methods = session->methods;
        if (methods) {
            assert(methods->release != NULL);
            methods->release(session);
        }
    });
    pomelo_list_clear(socket->sessions);
    return 0;
}


pomelo_socket_state pomelo_socket_get_state(pomelo_socket_t * socket) {
    assert(socket != NULL);
    return socket->state;
}


void pomelo_socket_statistic(
    pomelo_socket_t * socket,
    pomelo_statistic_t * statistic
) {
    assert(socket != NULL);
    memset(statistic, 0, sizeof(pomelo_statistic_t));

    // API statistic
    pomelo_context_t * context = socket->context;
    pomelo_statistic_api_t * api = &statistic->api;

    context->statistic(context, api);
    api->sessions = pomelo_pool_in_use(socket->builtin_session_pool);

    // Allocator statistic
    statistic->allocator.allocated_bytes =
        pomelo_allocator_allocated_bytes(socket->allocator);

    // Buffer context statistic
    pomelo_buffer_context_t * buffer_context = 
        socket->context->buffer_context;
    buffer_context->statistic(buffer_context, &statistic->buffer_context);

    // Platform statistic
    pomelo_platform_statistic(socket->platform, &statistic->platform);

    if (socket->state == POMELO_SOCKET_STATE_RUNNING_CLIENT ||
        socket->state == POMELO_SOCKET_STATE_RUNNING_SERVER)
    {
        // Only available when socket is running
        pomelo_protocol_socket_statistic(
            socket->protocol_socket,
            &statistic->protocol
        );
    }

    pomelo_delivery_transporter_statistic(
        socket->transporter,
        &statistic->delivery
    );
}


int pomelo_socket_iterate_sessions(
    pomelo_socket_t * socket,
    pomelo_session_iterator_t * iterator
) {
    assert(socket != NULL);
    assert(iterator != NULL);
    pomelo_list_t * sessions = socket->sessions;

    iterator->signature = sessions->mod_count;
    iterator->state = sessions->front;

    return 0;
}


size_t pomelo_socket_get_nsessions(pomelo_socket_t * socket) {
    assert(socket != NULL);
    return socket->sessions->size;
}


size_t pomelo_socket_send(
    pomelo_socket_t * socket,
    size_t channel_index,
    pomelo_message_t * message,
    pomelo_session_t ** recipients,
    size_t recipients_size
) {
    assert(socket != NULL);
    assert(message != NULL);
    assert(recipients != NULL);

    // Update the context for message
    pomelo_message_set_context(message, socket->context);

    if (recipients_size == 0) {
        // Just nothing to do
        pomelo_message_unref(message);
        return 0;
    }

    if (channel_index >= socket->nchannels) {
        // Channel is invalid
        pomelo_message_unref(message);
        return 0;
    }

    // The original message is for the last recipient
    size_t last_index = recipients_size - 1;
    size_t sent_count = 0;

    pomelo_message_t * cloned_message = NULL;

    // send for the first (n - 1) sessions
    for (size_t i = 0; i < last_index; i++) {
        pomelo_session_t * session = recipients[i];
        if (!cloned_message) {
            cloned_message = pomelo_message_clone(message);
            if (!cloned_message) {
                // Failed to clone the message
                break;
            }
        }

        int ret = pomelo_session_send(session, channel_index, cloned_message);
        if (ret == 0) {
            sent_count++;
            cloned_message = NULL;
            // In the case of failure sending, the message will be re-used
        }
    }

    // Cleanup the cloned message (if it remains)
    if (cloned_message) {
        pomelo_message_unref(cloned_message);
        cloned_message = NULL;
    }

    // Send the message to the last one
    pomelo_session_t * session = recipients[last_index];
    int ret = pomelo_session_send(session, channel_index, message);
    if (ret == 0) {
        sent_count++;
    } else {
        // In the case of failure, unref the message
        pomelo_message_unref(message);
    }

    return sent_count;
}


uint64_t pomelo_socket_time(pomelo_socket_t * socket) {
    assert(socket != NULL);
    return pomelo_protocol_socket_time(socket->protocol_socket);
}


pomelo_context_t * pomelo_socket_get_context(pomelo_socket_t * socket) {
    assert(socket != NULL);
    return socket->context;
}


pomelo_platform_t * pomelo_socket_get_platform(pomelo_socket_t * socket) {
    assert(socket != NULL);
    return socket->platform;
}


size_t pomelo_socket_get_nchannels(pomelo_socket_t * socket) {
    assert(socket != NULL);
    return socket->nchannels;
}


pomelo_channel_mode pomelo_socket_get_channel_mode(
    pomelo_socket_t * socket,
    size_t channel_index
) {
    assert(socket != NULL);
    if (channel_index >= socket->nchannels) {
        return POMELO_CHANNEL_MODE_UNRELIABLE;
    }
    return socket->channel_modes[channel_index];
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_socket_stop_deferred(pomelo_socket_t * socket) {
    assert(socket != NULL);

    // Destroy the protocol socket
    pomelo_protocol_socket_destroy(socket->protocol_socket);
    socket->protocol_socket = NULL;

    // Keep the transporter

    // Change the state
    socket->state = POMELO_SOCKET_STATE_STOPPED;
    
    // Clear the private key
    memset(socket->private_key, 0, POMELO_KEY_BYTES);

    // Finally, call the callback
    pomelo_socket_on_stopped(socket);
}


void pomelo_socket_process_stopped_component(
    pomelo_socket_t * socket,
    uint8_t component_mask
) {
    assert(socket != NULL);
    socket->stopped_components |= component_mask;

    if (socket->stopped_components == POMELO_SOCKET_COMPONENT_ALL) {
        pomelo_socket_stop_deferred(socket);
    }
}


void pomelo_socket_process_stopped_component_unexpectedly(
    pomelo_socket_t * socket,
    uint8_t component_mask
) {
    assert(socket != NULL);

    // In this state, the protocol socket has been stopped.
    // We stop the transporter and shutdown job platform
    socket->state = POMELO_SOCKET_STATE_STOPPING;

    // Unexpected stopping, reset the stopped flag, process stopped component
    pomelo_socket_process_stopped_component(socket, component_mask);

    uint8_t components = socket->stopped_components;
    if ((components & POMELO_SOCKET_COMPONENT_PROTOCOL_SOCKET) == 0) {
        // The socket has not been stopping
        pomelo_protocol_socket_stop(socket->protocol_socket);
        // => pomelo_protocol_socket_on_stopped
    }

    if ((components & POMELO_SOCKET_COMPONENT_DELIVERY_TRANSPORTER) == 0) {
        // Request stop transporter
        pomelo_delivery_transporter_stop(socket->transporter);
        // => pomelo_delivery_transporter_on_stopped
    }

    if ((components & POMELO_SOCKET_COMPONENT_PLUGINS) == 0) {
        // Request stop all plugins
        pomelo_plugin_stop_socket(socket);
        // => pomelo_plugin_on_all_plugins_detached_socket
    }
}


void pomelo_socket_add_session(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    assert(socket != NULL);
    assert(session != NULL);
    session->node = pomelo_list_push_back(socket->sessions, session);
}


void pomelo_socket_remove_session(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    assert(socket != NULL);
    assert(session != NULL);

    if (session->node) {
        pomelo_list_remove(socket->sessions, session->node);
        session->node = NULL;
    }
}


int pomelo_session_iterator_next(
    pomelo_session_iterator_t * iterator,
    pomelo_session_t ** p_session
) {
    assert(iterator != NULL);
    assert(p_session != NULL);

    pomelo_list_node_t * node = iterator->state;
    if (!node) {
        *p_session = NULL;
        return 0; // No more element
    }

    pomelo_session_t * session = pomelo_list_element(node, pomelo_session_t *);
    pomelo_socket_t * socket = session->socket;
    if (!socket) {
        return -1; // Invalid
    }

    pomelo_list_t * sessions = socket->sessions;
    if (!sessions || iterator->signature != sessions->mod_count) {
        return -1; // Invalid
    }

    iterator->state = node->next;
    *p_session = session;
    return 0;
}


size_t pomelo_session_iterator_load(
    pomelo_session_iterator_t * iterator,
    size_t max_elements,
    pomelo_session_t ** array_sessions
) {
    assert(iterator != NULL);
    assert(array_sessions != NULL);

    pomelo_list_node_t * node = iterator->state;
    if (!node) {
        return 0; // No more elements
    }

    pomelo_session_t * session = pomelo_list_element(node, pomelo_session_t *);
    pomelo_socket_t * socket = session->socket;
    if (!socket) {
        return -1; // Invalid
    }

    pomelo_list_t * sessions = socket->sessions;
    if (!sessions || iterator->signature != sessions->mod_count) {
        return -1; // Invalid
    }

    size_t count = 0;
    while (count < max_elements && session) {
        array_sessions[count] = session;
        count++;
        node = node->next;
        session = pomelo_list_element(node, pomelo_session_t *);
    }

    iterator->state = node;
    return count;
}
