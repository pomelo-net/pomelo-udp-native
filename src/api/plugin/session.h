#ifndef POMELO_API_PLUGIN_SESSION_SRC_H
#define POMELO_API_PLUGIN_SESSION_SRC_H
#include "api/session.h"
#include "utils/atomic.h"
#include "utils/array.h"
#include "plugin.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The session plugin info
typedef struct pomelo_session_plugin_info_s pomelo_session_plugin_info_t;


struct pomelo_session_plugin_info_s {
    /// @brief The socket
    pomelo_socket_t * socket;

    /// @brief The plugin implementation
    pomelo_plugin_impl_t * plugin;
};

struct pomelo_session_plugin_s {
    /// @brief The base session
    pomelo_session_t base;

    /// @brief Created plugin
    pomelo_plugin_impl_t * plugin;

    /// @brief The plugin private data
    pomelo_extra_t private_data;

    /// @brief List of channels
    pomelo_array_t * channels;

    /// @brief The destroy task
    pomelo_sequencer_task_t destroy_task;
};


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Set private data for session
void POMELO_PLUGIN_CALL pomelo_plugin_session_set_private(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    void * data
);


/// @brief Get private data for session
void * POMELO_PLUGIN_CALL pomelo_plugin_session_get_private(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
);


/// @brief Handle creating new session
pomelo_session_t * POMELO_PLUGIN_CALL pomelo_plugin_session_create(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    int64_t client_id,
    pomelo_address_t * address
);


/// @brief Handle destroying a session
void POMELO_PLUGIN_CALL pomelo_plugin_session_destroy(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
);


/// @brief Handle receiving new message
void POMELO_PLUGIN_CALL pomelo_plugin_session_receive(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    pomelo_message_t * message
);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Get the plugin session methods table
pomelo_session_methods_t * pomelo_session_plugin_methods(void);



/// @brief Allocating callback for session pool
int pomelo_session_plugin_on_alloc(
    pomelo_session_plugin_t * session,
    pomelo_context_t * context
);


/// @brief Deallocating callback for session pool
void pomelo_session_plugin_on_free(pomelo_session_plugin_t * session);


/// @brief Initialize the plugin session
int pomelo_session_plugin_init(
    pomelo_session_plugin_t * session,
    pomelo_session_plugin_info_t * info
);


/// @brief Cleanup the plugin session
void pomelo_session_plugin_cleanup(pomelo_session_plugin_t * session);


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


/// @brief Destroy the plugin session
void pomelo_session_plugin_destroy_deferred(pomelo_session_plugin_t * session);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_PLUGIN_SESSION_SRC_H
