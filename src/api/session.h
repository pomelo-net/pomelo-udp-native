#ifndef POMELO_API_SESSION_SRC_H
#define POMELO_API_SESSION_SRC_H
#include "pomelo/api.h"
#include "delivery/delivery.h"
#include "protocol/protocol.h"
#include "base/extra.h"
#include "utils/list.h"
#include "utils/atomic.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief Session methods table
typedef struct pomelo_session_methods_s pomelo_session_methods_t;


/// @brief Session info
typedef struct pomelo_session_info_s pomelo_session_info_t;


/// @brief Disconnecting function of session
typedef int (*pomelo_session_disconnect_fn)(pomelo_session_t * session);


/// @brief Getting RTT function of session
typedef int (*pomelo_session_get_rtt_fn)(
    pomelo_session_t * session,
    uint64_t * mean,
    uint64_t * variance
);


/// @brief Getting channel function of session
typedef pomelo_channel_t * (*pomelo_session_get_channel_fn)(
    pomelo_session_t * session,
    size_t channel_index
);


/// @brief The state of session
typedef enum pomelo_session_state {
    /// @brief The session is connected
    POMELO_SESSION_STATE_CONNECTED,

    /// @brief The session is connecting
    POMELO_SESSION_STATE_CONNECTING,

    /// @brief The session is disconnected
    POMELO_SESSION_STATE_DISCONNECTED
} pomelo_session_state;


typedef enum pomelo_session_type {
    /// @brief The session is a builtin session
    POMELO_SESSION_TYPE_BUILTIN,

    /// @brief The session is a plugin session
    POMELO_SESSION_TYPE_PLUGIN
} pomelo_session_type;


struct pomelo_session_methods_s {
    /// @brief Disconnecting method
    pomelo_session_disconnect_fn disconnect;

    /// @brief Getting RTT method
    pomelo_session_get_rtt_fn get_rtt;

    /// @brief Getting channel method
    pomelo_session_get_channel_fn get_channel;
};


struct pomelo_session_info_s {
    /// @brief The type of session
    pomelo_session_type type;

    /// @brief The socket
    pomelo_socket_t * socket;

    /// @brief The methods
    pomelo_session_methods_t * methods;
};


struct pomelo_session_s {
    /// @brief The context
    pomelo_context_t * context;

    /// @brief The type of session
    pomelo_session_type type;

    /// @brief The extra data of session
    pomelo_extra_t extra;

    /// @brief The socket
    pomelo_socket_t * socket;

    /// @brief The ID of session
    int64_t client_id;

    /// @brief The address of session
    pomelo_address_t address;

    /// @brief The signature of session.
    pomelo_atomic_uint64_t signature;

    /// @brief Methods table of session
    pomelo_session_methods_t * methods;

    /// @brief Entry of session in list
    pomelo_list_entry_t * entry;

    /// @brief The state of session
    pomelo_session_state state;

    /// @brief The disconnecting task
    pomelo_sequencer_task_t disconnect_task;

    /* Some temporary data */

    /// @brief The original index of session (For sending batch messages)
    size_t tmp_original_index;
};


struct pomelo_session_disconnect_request_s {
    /// @brief The context
    pomelo_context_t * context;

    /// @brief The session
    pomelo_session_t * session;
};


/// @brief On allocate callback
int pomelo_session_on_alloc(
    pomelo_session_t * session,
    pomelo_context_t * context
);


/// @brief On free callback
void pomelo_session_on_free(pomelo_session_t * session);


/// @brief Initialize base session
int pomelo_session_init(
    pomelo_session_t * session,
    pomelo_session_info_t * info
);


/// @brief Cleanup base session
void pomelo_session_cleanup(pomelo_session_t * session);


/// @brief The deferred disconnect function
void pomelo_session_disconnect_deferred(pomelo_session_t * session);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_SESSION_SRC_H
