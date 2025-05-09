#include <assert.h>
#include <string.h>
#include "api/context.h"
#include "api/socket.h"
#include "plugin.h"
#include "manager.h"
#include "socket.h"
#include "session.h"
#include "message.h"
#include "channel.h"
#include "token.h"
#include "executor.h"


/* -------------------------------------------------------------------------- */
/*                                 Public APIs                                */
/* -------------------------------------------------------------------------- */

pomelo_plugin_t * pomelo_plugin_register(
    pomelo_allocator_t * allocator,
    pomelo_context_t * context,
    pomelo_platform_t * platform,
    pomelo_plugin_initializer initializer
) {
    assert(context != NULL);
    assert(initializer != NULL);

    if (!allocator) {
        allocator = pomelo_allocator_default();
        if (!allocator) return NULL; // Failed to get default allocator
    }

    pomelo_plugin_impl_t * plugin =
        pomelo_plugin_create(allocator, context, platform);
    if (!plugin) return NULL; // Failed to create plugin
    pomelo_plugin_t * base = &plugin->base;

    pomelo_plugin_manager_t * manager = context->plugin_manager;
    int ret = pomelo_plugin_manager_add_plugin(manager, base);
    if (ret < 0) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    initializer(base, POMELO_PLUGIN_VERSION_HEX);
    pomelo_plugin_post_callback_cleanup(plugin);

    return base;
}


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_plugin_stop_socket(pomelo_socket_t * socket) {
    assert(socket != NULL);
    pomelo_plugin_dispatch_socket_on_stopped(socket);
}


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */


pomelo_plugin_impl_t * pomelo_plugin_create(
    pomelo_allocator_t * allocator,
    pomelo_context_t * context,
    pomelo_platform_t * platform
) {
    assert(allocator != NULL);
    assert(context != NULL);
    assert(platform != NULL);

    pomelo_plugin_impl_t * plugin =
        pomelo_allocator_malloc_t(allocator, pomelo_plugin_impl_t);
    if (!plugin) return NULL; // Failed to allocate memory
    memset(plugin, 0, sizeof(pomelo_plugin_impl_t));

#ifndef NDEBUG
    plugin->signature = POMELO_PLUGIN_SIGNATURE;
#endif

    plugin->allocator = allocator;
    plugin->context = context;
    plugin->platform = platform;
    pomelo_atomic_uint64_store(&plugin->data, 0);

    pomelo_list_options_t list_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_message_t *)
    };
    plugin->acquired_messages = pomelo_list_create(&list_options);
    if (!plugin->acquired_messages) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    pomelo_pool_root_options_t pool_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_plugin_executor_command_t),
        .synchronized = true
    };
    plugin->command_pool = pomelo_pool_root_create(&pool_options);
    if (!plugin->command_pool) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    return plugin;
}

void pomelo_plugin_destroy(pomelo_plugin_impl_t * plugin) {
    assert(plugin != NULL);

    // Call the unload callback
    if (plugin->on_unload_callback) {
        plugin->on_unload_callback((pomelo_plugin_t *) plugin);
        pomelo_plugin_post_callback_cleanup(plugin);
    }

    if (plugin->acquired_messages) {
        pomelo_list_destroy(plugin->acquired_messages);
        plugin->acquired_messages = NULL;
    }

    if (plugin->command_pool) {
        pomelo_pool_destroy(plugin->command_pool);
        plugin->command_pool = NULL;
    }

    pomelo_allocator_free(plugin->allocator, plugin);
}


/* -------------------------------------------------------------------------- */
/*                                 Export APIs                                */
/* -------------------------------------------------------------------------- */

void POMELO_PLUGIN_CALL pomelo_plugin_configure(
    pomelo_plugin_t * plugin,
    pomelo_plugin_on_unload_callback on_unload_callback,
    pomelo_plugin_socket_common_callback socket_on_created_callback,
    pomelo_plugin_socket_common_callback socket_on_destroyed_callback,
    pomelo_plugin_socket_listening_callback socket_on_listening_callback,
    pomelo_plugin_socket_connecting_callback socket_on_connecting_callback,
    pomelo_plugin_socket_common_callback socket_on_stopped_callback,
    pomelo_plugin_session_send_callback session_on_send_callback,
    pomelo_plugin_session_disconnect_callback session_disconnect_callback,
    pomelo_plugin_session_get_rtt_callback session_get_rtt_callback,
    pomelo_plugin_session_set_mode_callback session_set_mode_callback
) {
    assert(plugin != NULL);
    if (!plugin) return; // Invalid arguments

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    impl->on_unload_callback = on_unload_callback;
    impl->socket_on_created_callback = socket_on_created_callback;
    impl->socket_on_destroyed_callback = socket_on_destroyed_callback;
    impl->socket_on_listening_callback = socket_on_listening_callback;
    impl->socket_on_connecting_callback = socket_on_connecting_callback;
    impl->socket_on_stopped_callback = socket_on_stopped_callback;
    impl->session_on_send_callback = session_on_send_callback;
    impl->session_disconnect_callback = session_disconnect_callback;
    impl->session_get_rtt_callback = session_get_rtt_callback;
    impl->session_set_channel_mode_callback = session_set_mode_callback;
}


void POMELO_PLUGIN_CALL pomelo_plugin_set_data(
    pomelo_plugin_t * plugin,
    void * data
) {
    assert(plugin != NULL);
    if (!plugin) return; // Invalid arguments

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    pomelo_atomic_uint64_store(&impl->data, (uint64_t) data);
}


void * POMELO_PLUGIN_CALL pomelo_plugin_get_data(pomelo_plugin_t * plugin) {
    assert(plugin != NULL);
    if (!plugin) return NULL; // Invalid arguments

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    return (void *) pomelo_atomic_uint64_load(&impl->data);
}


void pomelo_plugin_init_template(pomelo_plugin_t * tpl) {
    assert(tpl != NULL);

    // Bind the exported functions
    tpl->configure_callbacks = pomelo_plugin_configure;
    tpl->set_data = pomelo_plugin_set_data;
    tpl->get_data = pomelo_plugin_get_data;

    /* Socket */
    tpl->socket_get_nchannels = pomelo_plugin_socket_get_nchannels;
    tpl->socket_get_channel_mode = pomelo_plugin_socket_get_channel_mode;
    tpl->socket_time = pomelo_plugin_socket_time;

    /* Session */
    tpl->session_set_private = pomelo_plugin_session_set_private;
    tpl->session_get_private = pomelo_plugin_session_get_private;
    tpl->session_create = pomelo_plugin_session_create;
    tpl->session_destroy = pomelo_plugin_session_destroy;
    tpl->session_receive = pomelo_plugin_session_receive;

    /* Message */
    tpl->message_acquire = pomelo_plugin_message_acquire;
    tpl->message_write = pomelo_plugin_message_write;
    tpl->message_read = pomelo_plugin_message_read;
    tpl->message_length = pomelo_plugin_message_length;

    /* Token */
    tpl->connect_token_decode = pomelo_plugin_token_connect_token_decode;

    /* Executor */
    tpl->executor_startup = pomelo_plugin_executor_startup;
    tpl->executor_shutdown = pomelo_plugin_executor_shutdown;
    tpl->executor_submit = pomelo_plugin_executor_submit;
}


void pomelo_plugin_parse_address(
    pomelo_address_t * address,
    pomelo_address_type address_type,
    uint8_t * address_host,
    uint16_t address_port
) {
    assert(address != NULL);
    assert(address_host != NULL);
    if (!address || !address_host) return; // Invalid arguments

    address->type = address_type;
    if (address_type == POMELO_ADDRESS_IPV4) {
        memcpy(address->ip.v4, address_host, sizeof(address->ip.v4));
    } else {
        memcpy(address->ip.v6, address_host, sizeof(address->ip.v6));
    }
    address->port = address_port;
}


/* -------------------------------------------------------------------------- */
/*                        Internal dispatching APIs                           */
/* -------------------------------------------------------------------------- */

void pomelo_plugin_dispatch_socket_on_created(pomelo_socket_t * socket) {
    assert(socket != NULL);
    pomelo_plugin_manager_t * plugin_manager = socket->context->plugin_manager;
    pomelo_list_t * plugins = plugin_manager->plugins;

    pomelo_plugin_impl_t * plugin = NULL;
    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, plugins);
    while (pomelo_list_iterator_next(&it, &plugin) == 0) {
        if (plugin->socket_on_created_callback) {
            plugin->socket_on_created_callback(&plugin->base, socket);
            pomelo_plugin_post_callback_cleanup(plugin);
        }
    }
}


void pomelo_plugin_dispatch_socket_on_listening(
    pomelo_socket_t * socket,
    pomelo_address_t * address
) {
    assert(socket != NULL);
    pomelo_plugin_manager_t * plugin_manager = socket->context->plugin_manager;
    pomelo_list_t * plugins = plugin_manager->plugins;

    pomelo_plugin_impl_t * plugin = NULL;
    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, plugins);
    while (pomelo_list_iterator_next(&it, &plugin) == 0) {
        if (plugin->socket_on_listening_callback) {
            plugin->socket_on_listening_callback(
                &plugin->base,
                socket,
                address
            );
            pomelo_plugin_post_callback_cleanup(plugin);
        }
    }
}


void pomelo_plugin_dispatch_socket_on_connecting(
    pomelo_socket_t * socket,
    uint8_t * connect_token
) {
    assert(socket != NULL);
    pomelo_plugin_manager_t * plugin_manager = socket->context->plugin_manager;
    pomelo_list_t * plugins = plugin_manager->plugins;

    pomelo_plugin_impl_t * plugin = NULL;
    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, plugins);
    while (pomelo_list_iterator_next(&it, &plugin) == 0) {
        if (plugin->socket_on_connecting_callback) {
            // Clone the address host here to make it cannot modified
            plugin->socket_on_connecting_callback(
                &plugin->base,
                socket,
                connect_token
            );
            pomelo_plugin_post_callback_cleanup(plugin);
        }
    };
}


void pomelo_plugin_dispatch_socket_on_stopped(pomelo_socket_t * socket) {
    assert(socket != NULL);
    pomelo_plugin_manager_t * plugin_manager = socket->context->plugin_manager;
    pomelo_list_t * plugins = plugin_manager->plugins;

    pomelo_plugin_impl_t * plugin = NULL;
    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, plugins);
    while (pomelo_list_iterator_next(&it, &plugin) == 0) {
        if (plugin->socket_on_stopped_callback) {
            plugin->socket_on_stopped_callback(&plugin->base, socket);
            pomelo_plugin_post_callback_cleanup(plugin);
        }
    };
}


void pomelo_plugin_dispatch_socket_on_destroyed(pomelo_socket_t * socket) {
    assert(socket != NULL);
    pomelo_plugin_manager_t * plugin_manager = socket->context->plugin_manager;
    pomelo_list_t * plugins = plugin_manager->plugins;

    pomelo_plugin_impl_t * plugin = NULL;
    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, plugins);
    while (pomelo_list_iterator_next(&it, &plugin) == 0) {
        if (plugin->socket_on_destroyed_callback) {
            plugin->socket_on_destroyed_callback(&plugin->base, socket);
            pomelo_plugin_post_callback_cleanup(plugin);
        }
    }
}


void pomelo_plugin_post_callback_cleanup(pomelo_plugin_impl_t * impl) {
    assert(impl != NULL);

    // Release all acquired messages
    pomelo_message_t * message;
    while (pomelo_list_pop_front(impl->acquired_messages, &message) == 0) {
        pomelo_message_unref(message);
    }
}
