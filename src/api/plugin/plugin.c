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
#include "time.h"


#define POMELO_PLUGIN_SHARED_POOL_RELEASE_BUFFER 32


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
        if (!allocator) {
            return NULL;
        }
    }

    pomelo_plugin_impl_t * plugin =
        pomelo_plugin_create(allocator, context, platform);
    if (!plugin) {
        return NULL;
    }
    pomelo_plugin_t * base = &plugin->base;

    pomelo_plugin_manager_t * manager = context->plugin_manager;
    int ret = pomelo_plugin_manager_add_plugin(manager, base);
    if (ret < 0) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    initializer(base);
    return base;
}


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_plugin_stop_socket(pomelo_socket_t * socket) {
    assert(socket != NULL);
    pomelo_plugin_dispatch_socket_on_stopped(socket);

    if (pomelo_list_is_empty(socket->attached_plugins)) {
        pomelo_plugin_on_all_plugins_detached_socket(socket);
    }
}


void pomelo_plugin_on_all_plugins_detached_socket(pomelo_socket_t * socket) {
    assert(socket != NULL);
    pomelo_socket_process_stopped_component(
        socket,
        POMELO_SOCKET_COMPONENT_PLUGINS
    );
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
    if (!plugin) {
        return NULL;
    }
    memset(plugin, 0, sizeof(pomelo_plugin_impl_t));

    plugin->allocator = allocator;
    plugin->context = context;
    plugin->platform = platform;
    pomelo_atomic_uint64_store(&plugin->data, 0);

    // Create pool of session creating commands
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_session_plugin_create_command_t);
    pool_options.zero_initialized = true;
    pool_options.synchronized = true;

    plugin->session_create_command_pool = pomelo_pool_create(&pool_options);
    if (!plugin->session_create_command_pool) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    // Create create_command_release pool
    pomelo_shared_pool_options_t shared_pool_options;
    pomelo_shared_pool_options_init(&shared_pool_options);
    shared_pool_options.allocator = allocator;
    shared_pool_options.master_pool = plugin->session_create_command_pool;
    shared_pool_options.buffers = POMELO_PLUGIN_SHARED_POOL_RELEASE_BUFFER;

    plugin->session_create_command_pool_release =
        pomelo_shared_pool_create(&shared_pool_options);
    if (!plugin->session_create_command_pool_release) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    // Create pool of session destroying commands
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_session_plugin_destroy_command_t);
    pool_options.zero_initialized = true;
    pool_options.synchronized = true;

    plugin->session_destroy_command_pool = pomelo_pool_create(&pool_options);
    if (!plugin->session_destroy_command_pool) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    // Create destroy_command_release pool
    pomelo_shared_pool_options_init(&shared_pool_options);
    shared_pool_options.allocator = allocator;
    shared_pool_options.master_pool = plugin->session_destroy_command_pool;
    shared_pool_options.buffers = POMELO_PLUGIN_SHARED_POOL_RELEASE_BUFFER;

    plugin->session_destroy_command_pool_release =
        pomelo_shared_pool_create(&shared_pool_options);
    if (!plugin->session_destroy_command_pool_release) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    // Create receive_command pool
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_session_plugin_receive_command_t);
    pool_options.zero_initialized = true;
    pool_options.synchronized = true;

    plugin->session_receive_command_pool = pomelo_pool_create(&pool_options);
    if (!plugin->session_receive_command_pool) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    // Create receive_command_release pool
    pomelo_shared_pool_options_init(&shared_pool_options);
    shared_pool_options.allocator = allocator;
    shared_pool_options.master_pool = plugin->session_receive_command_pool;
    shared_pool_options.buffers = POMELO_PLUGIN_SHARED_POOL_RELEASE_BUFFER;

    plugin->session_receive_command_pool_release =
        pomelo_shared_pool_create(&shared_pool_options);
    if (!plugin->session_receive_command_pool_release) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    // Create pool of plugin sessions
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_session_plugin_t);
    pool_options.callback_context = plugin;
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_session_plugin_alloc;
    pool_options.deallocate_callback = (pomelo_pool_callback)
        pomelo_session_plugin_dealloc;

    plugin->session_plugin_pool = pomelo_pool_create(&pool_options);
    if (!plugin->session_plugin_pool) {
        pomelo_plugin_destroy(plugin);
        return NULL;
    }

    // Create pool of plugin channels
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_channel_plugin_t);
    pool_options.zero_initialized = true;
    plugin->channel_plugin_pool = pomelo_pool_create(&pool_options);
    if (!plugin->channel_plugin_pool) {
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
    }

    if (plugin->session_create_command_pool_release) {
        pomelo_shared_pool_destroy(plugin->session_create_command_pool_release);
        plugin->session_create_command_pool_release = NULL;
    }

    if (plugin->session_create_command_pool) {
        pomelo_pool_destroy(plugin->session_create_command_pool);
        plugin->session_create_command_pool = NULL;
    }

    if (plugin->session_destroy_command_pool_release) {
        pomelo_shared_pool_destroy(
            plugin->session_destroy_command_pool_release
        );
        plugin->session_destroy_command_pool_release = NULL;
    }

    if (plugin->session_destroy_command_pool) {
        pomelo_pool_destroy(plugin->session_destroy_command_pool);
        plugin->session_destroy_command_pool = NULL;
    }

    if (plugin->session_receive_command_pool_release) {
        pomelo_shared_pool_destroy(
            plugin->session_receive_command_pool_release
        );
        plugin->session_receive_command_pool_release = NULL;
    }

    if (plugin->session_receive_command_pool) {
        pomelo_pool_destroy(plugin->session_receive_command_pool);
        plugin->session_receive_command_pool = NULL;
    }

    if (plugin->session_plugin_pool) {
        pomelo_pool_destroy(plugin->session_plugin_pool);
        plugin->session_plugin_pool = NULL;
    }

    if (plugin->channel_plugin_pool) {
        pomelo_pool_destroy(plugin->channel_plugin_pool);
        plugin->channel_plugin_pool = NULL;
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
    pomelo_plugin_session_create_callback session_create_callback,
    pomelo_plugin_session_receive_callback session_receive_callback,
    pomelo_plugin_session_disconnect_callback session_disconnect_callback,
    pomelo_plugin_session_get_rtt_callback session_get_rtt_callback,
    pomelo_plugin_session_set_mode_callback session_set_channel_mode_callback,
    pomelo_plugin_session_send_callback session_send_callback
) {
    assert(plugin != NULL);
    if (!plugin) {
        return;
    }

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;

    impl->on_unload_callback = on_unload_callback;
    impl->socket_on_created_callback = socket_on_created_callback;
    impl->socket_on_destroyed_callback = socket_on_destroyed_callback;
    impl->socket_on_listening_callback = socket_on_listening_callback;
    impl->socket_on_connecting_callback = socket_on_connecting_callback;
    impl->socket_on_stopped_callback = socket_on_stopped_callback;
    impl->session_create_callback = session_create_callback;
    impl->session_receive_callback = session_receive_callback;
    impl->session_disconnect_callback = session_disconnect_callback;
    impl->session_get_rtt_callback = session_get_rtt_callback;
    impl->session_set_channel_mode_callback = session_set_channel_mode_callback;
    impl->session_send_callback = session_send_callback;
}


void POMELO_PLUGIN_CALL pomelo_plugin_set_data(
    pomelo_plugin_t * plugin,
    void * data
) {
    assert(plugin != NULL);
    if (!plugin) {
        return;
    }

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_atomic_uint64_store(&impl->data, (uint64_t) data);
}


void * POMELO_PLUGIN_CALL pomelo_plugin_get_data(pomelo_plugin_t * plugin) {
    assert(plugin != NULL);
    if (!plugin) {
        return NULL;
    }

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
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
    tpl->socket_attach = pomelo_plugin_socket_attach;
    tpl->socket_detach = pomelo_plugin_socket_detach;
    tpl->socket_time = pomelo_plugin_socket_time;

    /* Session */
    tpl->session_get_private = pomelo_plugin_process_session_get_private;
    tpl->session_create = pomelo_plugin_handle_session_create;
    tpl->session_destroy = pomelo_plugin_handle_session_destroy;
    tpl->session_receive = pomelo_plugin_handle_session_receive;
    tpl->session_signature = pomelo_plugin_handle_session_signature;

    /* Message */
    tpl->message_write = pomelo_plugin_message_write;
    tpl->message_read = pomelo_plugin_message_read;
    tpl->message_length = pomelo_plugin_message_length;

    /* Token */
    tpl->connect_token_decode = pomelo_plugin_token_connect_token_decode;
}


void pomelo_plugin_parse_address(
    pomelo_address_t * address,
    pomelo_address_type address_type,
    uint8_t * address_host,
    uint16_t address_port
) {
    assert(address != NULL);
    assert(address_host != NULL);
    if (!address || !address_host) {
        return;
    }

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

    pomelo_plugin_impl_t * plugin;
    pomelo_list_for(plugins, plugin, pomelo_plugin_impl_t *, {
        if (plugin->socket_on_created_callback) {
            plugin->socket_on_created_callback(&plugin->base, socket);
        }
    });
}


void pomelo_plugin_dispatch_socket_on_listening(
    pomelo_socket_t * socket,
    pomelo_address_t * address
) {
    assert(socket != NULL);
    pomelo_plugin_manager_t * plugin_manager = socket->context->plugin_manager;
    pomelo_list_t * plugins = plugin_manager->plugins;

    pomelo_plugin_impl_t * plugin;
    pomelo_list_for(plugins, plugin, pomelo_plugin_impl_t *, {
        if (plugin->socket_on_listening_callback) {
            plugin->socket_on_listening_callback(
                &plugin->base,
                socket,
                address
            );
        }
    });
}


void pomelo_plugin_dispatch_socket_on_connecting(
    pomelo_socket_t * socket,
    uint8_t * connect_token
) {
    assert(socket != NULL);
    pomelo_plugin_manager_t * plugin_manager = socket->context->plugin_manager;
    pomelo_list_t * plugins = plugin_manager->plugins;

    pomelo_plugin_impl_t * plugin;
    pomelo_list_for(plugins, plugin, pomelo_plugin_impl_t *, {
        if (plugin->socket_on_connecting_callback) {
            // Clone the address host here to make it cannot modified
            plugin->socket_on_connecting_callback(
                &plugin->base,
                socket,
                connect_token
            );
        }
    });
}


void pomelo_plugin_dispatch_socket_on_stopped(pomelo_socket_t * socket) {
    assert(socket != NULL);
    pomelo_plugin_manager_t * plugin_manager = socket->context->plugin_manager;
    pomelo_list_t * plugins = plugin_manager->plugins;

    pomelo_plugin_impl_t * plugin;
    pomelo_list_for(plugins, plugin, pomelo_plugin_impl_t *, {
        if (plugin->socket_on_stopped_callback) {
            plugin->socket_on_stopped_callback(&plugin->base, socket);
        }
    });
}


void pomelo_plugin_dispatch_socket_on_destroyed(pomelo_socket_t * socket) {
    assert(socket != NULL);
    pomelo_plugin_manager_t * plugin_manager = socket->context->plugin_manager;
    pomelo_list_t * plugins = plugin_manager->plugins;

    pomelo_plugin_impl_t * plugin;
    pomelo_list_for(plugins, plugin, pomelo_plugin_impl_t *, {
        if (plugin->socket_on_destroyed_callback) {
            plugin->socket_on_destroyed_callback(&plugin->base, socket);
        }
    });
}
