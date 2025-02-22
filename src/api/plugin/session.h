#ifndef POMELO_API_PLUGIN_SESSION_SRC_H
#define POMELO_API_PLUGIN_SESSION_SRC_H
#include "api/session.h"
#include "utils/atomic.h"
#include "utils/list.h"
#include "plugin.h"


#ifdef __cplusplus
extern "C" {
#endif


/// @brief Creating command of sessions
typedef struct pomelo_session_plugin_create_command_s
    pomelo_session_plugin_create_command_t;

/// @brief Destroying command of sessions
typedef struct pomelo_session_plugin_destroy_command_s
    pomelo_session_plugin_destroy_command_t;

/// @brief Receiving command of sessions
typedef struct pomelo_session_plugin_receive_command_s
    pomelo_session_plugin_receive_command_t;


struct pomelo_session_plugin_s {
    /// @brief The base session
    pomelo_session_t base;

    /// @brief Created plugin
    pomelo_plugin_impl_t * plugin;

    /// @brief The plugin private data
    void * private_data;

    /// @brief List of channels
    pomelo_unrolled_list_t * channels;
};


struct pomelo_session_plugin_create_command_s {
    pomelo_plugin_impl_t * plugin;
    pomelo_socket_t * socket;
    int64_t client_id;
    pomelo_address_t address;
    void * private_data;
    void * callback_data;
};


struct pomelo_session_plugin_destroy_command_s {
    pomelo_plugin_impl_t * plugin;
    pomelo_session_plugin_t * session;
};


struct pomelo_session_plugin_receive_command_s {
    pomelo_plugin_impl_t * plugin;
    pomelo_session_plugin_t * session;
    size_t channel_index;
    void * callback_data;
};


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Set private data for session
void * POMELO_PLUGIN_CALL pomelo_plugin_process_session_get_private(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
);


/// @brief Handle creating new session
void POMELO_PLUGIN_CALL pomelo_plugin_handle_session_create(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    int64_t client_id,
    pomelo_address_t * address,
    void * private_data,
    void * callback_data
);


/// @brief Handle destroying a session
void POMELO_PLUGIN_CALL pomelo_plugin_handle_session_destroy(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
);


/// @brief Handle receiving new message
void POMELO_PLUGIN_CALL pomelo_plugin_handle_session_receive(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    void * callback_data
);

/// @brief Handle getting signature of session
uint64_t POMELO_PLUGIN_CALL pomelo_plugin_handle_session_signature(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Get the plugin session methods table
pomelo_session_methods_t * pomelo_session_plugin_methods(void);


/// @brief Release the plugin session
void pomelo_session_plugin_release(pomelo_session_plugin_t * session);


/// @brief Allocating callback for session pool
int pomelo_session_plugin_alloc(
    pomelo_session_plugin_t * session,
    pomelo_plugin_impl_t * plugin
);


/// @brief Deallocating callback for session pool
int pomelo_session_plugin_dealloc(
    pomelo_session_plugin_t * session,
    pomelo_plugin_impl_t * plugin
);


/// @brief Initialize the plugin session
int pomelo_session_plugin_init(
    pomelo_session_plugin_t * session,
    pomelo_socket_t * socket
);


/// @brief Finalize the plugin session
int pomelo_session_plugin_finalize(
    pomelo_session_plugin_t * session,
    pomelo_socket_t * socket
);


/// @brief Disconnect the plugin session
int pomelo_session_plugin_disconnect(pomelo_session_plugin_t * session);


/// @brief Get RTT of plugin
int pomelo_session_plugin_get_rtt(
    pomelo_session_plugin_t * session,
    uint64_t * mean,
    uint64_t * variance
);


/// @brief Get the channel by index of session
pomelo_channel_plugin_t * pomelo_session_plugin_get_channel(
    pomelo_session_plugin_t * session,
    size_t channel_index
);


/// @brief Process creating session in main thread
void pomelo_plugin_process_session_create(
    pomelo_session_plugin_create_command_t * command
);


/// @brief Process creating session in main thread
void pomelo_plugin_process_session_create_ex(
    pomelo_plugin_impl_t * plugin,
    pomelo_socket_t * socket,
    int64_t client_id,
    pomelo_address_t * address,
    void * private_data,
    void * callback_data
);


/// @brief Process when creating session failed
void pomelo_plugin_process_session_create_result(
    pomelo_plugin_impl_t * plugin,
    pomelo_socket_t * socket,
    pomelo_session_plugin_t * session,
    void * callback_data,
    pomelo_plugin_error_t error
);


/// @brief Process destroying session in main thread
void pomelo_plugin_process_session_destroy(
    pomelo_session_plugin_destroy_command_t * command
);


/// @brief Process destroying session in main thread
void pomelo_plugin_process_session_destroy_ex(
    pomelo_plugin_impl_t * plugin,
    pomelo_session_plugin_t * session
);


/// @brief Process receiving result
void pomelo_plugin_process_session_receive_result(
    pomelo_plugin_impl_t * plugin,
    pomelo_session_plugin_t * session,
    size_t channel_index,
    void * callback_data,
    pomelo_message_t * message,
    pomelo_plugin_error_t error
);


/// @brief Process destroying session in main thread
void pomelo_plugin_process_session_receive(
    pomelo_session_plugin_receive_command_t * command
);


/// @brief Process destroying session in main thread
void pomelo_plugin_process_session_receive_ex(
    pomelo_plugin_impl_t * plugin,
    pomelo_session_plugin_t * session,
    size_t channel_index,
    void * callback_data
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_PLUGIN_SESSION_SRC_H
