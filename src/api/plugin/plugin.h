#ifndef POMELO_API_PLUGIN_SRC_H
#define POMELO_API_PLUGIN_SRC_H
#include "pomelo/api.h"
#include "pomelo/plugin.h"
#include "api/session.h"
#include "utils/map.h"
#include "utils/atomic.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief The plugin manager
typedef struct pomelo_plugin_manager_s pomelo_plugin_manager_t;

/// @brief The session for plugins
typedef struct pomelo_session_plugin_s pomelo_session_plugin_t;

/// @brief The associated channel with plugin sessions
typedef struct pomelo_channel_plugin_s pomelo_channel_plugin_t;

/// @brief The implementation of plugin enviroment
typedef struct pomelo_plugin_impl_s pomelo_plugin_impl_t;


struct pomelo_plugin_impl_s {
    /// @brief Base plugin
    pomelo_plugin_t base;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The context of plugin
    pomelo_context_t * context;

    /// @brief The platform of plugin
    pomelo_platform_t * platform;

    /// @brief Associated data
    pomelo_atomic_uint64_t data;

    /// @brief [Synchronized] Central pool of session creating commands
    pomelo_pool_t * session_create_command_pool;

    /// @brief Releasing pool for main thread. This reduces the number of lock
    /// acquiring times.
    pomelo_shared_pool_t * session_create_command_pool_release;

    /// @brief [Synchronized] Central pool of session destroying commands
    pomelo_pool_t * session_destroy_command_pool;

    /// @brief Releasing pool for main thread. This reduces the number of lock
    /// acquiring times.
    pomelo_shared_pool_t * session_destroy_command_pool_release;

    /// @brief [Synchronized] Central pool of session receiving commands
    pomelo_pool_t * session_receive_command_pool;

    /// @brief Releasing pool for main thread. This reduces the number of lock
    /// acquiring times.
    pomelo_shared_pool_t * session_receive_command_pool_release;

    /// @brief Pool of sessions
    pomelo_pool_t * session_plugin_pool;

    /// @brief Pool of channels
    pomelo_pool_t * channel_plugin_pool;

    /// @brief Node of this plugin in attached list
    pomelo_list_node_t * attached_list_node;

    /* Callbacks */
    pomelo_plugin_on_unload_callback on_unload_callback;
    pomelo_plugin_socket_common_callback socket_on_created_callback;
    pomelo_plugin_socket_common_callback socket_on_destroyed_callback;
    pomelo_plugin_socket_listening_callback socket_on_listening_callback;
    pomelo_plugin_socket_connecting_callback socket_on_connecting_callback;
    pomelo_plugin_socket_common_callback socket_on_stopped_callback;
    pomelo_plugin_session_create_callback session_create_callback;
    pomelo_plugin_session_receive_callback session_receive_callback;
    pomelo_plugin_session_disconnect_callback session_disconnect_callback;
    pomelo_plugin_session_get_rtt_callback session_get_rtt_callback;
    pomelo_plugin_session_set_mode_callback session_set_channel_mode_callback;
    pomelo_plugin_session_send_callback session_send_callback;
};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Stop the socket for all plugins
void pomelo_plugin_stop_socket(pomelo_socket_t * socket);

/// @brief Process when all attached plugins detached
void pomelo_plugin_on_all_plugins_detached_socket(pomelo_socket_t * socket);

/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Create new plugin
pomelo_plugin_impl_t * pomelo_plugin_create(
    pomelo_allocator_t * allocator,
    pomelo_context_t * context,
    pomelo_platform_t * platform
);


/// @brief Destroy the plugin
void pomelo_plugin_destroy(pomelo_plugin_impl_t * plugin);


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Configure the plugin
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
);

/// @brief Set associated data for plugin
void POMELO_PLUGIN_CALL pomelo_plugin_set_data(
    pomelo_plugin_t * plugin,
    void * data
);

/// @brief Get associated data from plugin
void * POMELO_PLUGIN_CALL pomelo_plugin_get_data(pomelo_plugin_t * plugin);

/* -------------------------------------------------------------------------- */
/*                        Internal dispatching APIs                           */
/* -------------------------------------------------------------------------- */

/// @brief Dispatch created event of socket
void pomelo_plugin_dispatch_socket_on_created(pomelo_socket_t * socket);

/// @brief Dispatch listening event of socket
void pomelo_plugin_dispatch_socket_on_listening(
    pomelo_socket_t * socket,
    pomelo_address_t * address
);

/// @brief Dispatch connecting event of socket
void pomelo_plugin_dispatch_socket_on_connecting(
    pomelo_socket_t * socket,
    uint8_t * connect_token
);

/// @brief Dispatch stopped event of socket
void pomelo_plugin_dispatch_socket_on_stopped(pomelo_socket_t * socket);

/// @brief Dispatch destroyed event of socket
void pomelo_plugin_dispatch_socket_on_destroyed(pomelo_socket_t * socket);


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Initialize the plugin template
void pomelo_plugin_init_template(pomelo_plugin_t * tpl);


/// @brief Parse address
void pomelo_plugin_parse_address(
    pomelo_address_t * address,
    pomelo_address_type address_type,
    uint8_t * address_host,
    uint16_t address_port
);

#ifdef __cplusplus
}
#endif
#endif // ~POMELO_API_PLUGIN_SRC_H
