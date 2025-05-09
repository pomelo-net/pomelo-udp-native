#ifndef POMELO_PLUGIN_H
#define POMELO_PLUGIN_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "pomelo/common.h"
#include "pomelo/address.h"


/**
 * APIs for a plugin
 */


#ifdef __cplusplus
extern "C" {
#endif


/// @brief The version of the plugin
#define POMELO_PLUGIN_VERSION_HEX 0x000100000000ULL


/// Entry register
#define POMELO_PLUGIN_ENTRY_REGISTER(entry)                                    \
POMELO_PLUGIN_EXPORT void POMELO_PLUGIN_CALL pomelo_plugin_initializer_entry(  \
    pomelo_plugin_t * plugin,                                                  \
    uint64_t version                                                           \
) {                                                                            \
    if ((version >> 16) != (POMELO_PLUGIN_VERSION_HEX >> 16)) {                \
        fprintf(                                                               \
            stderr,                                                            \
            "Incompatible plugin version: 0x%012llx != 0x%012llx",             \
            version,                                                           \
            POMELO_PLUGIN_VERSION_HEX                                          \
        );                                                                     \
        abort();                                                               \
        return;                                                                \
    }                                                                          \
    entry(plugin);                                                             \
}


struct pomelo_plugin_token_info_s {
    uint8_t * connect_token;
    uint64_t * protocol_id;
    uint64_t * create_timestamp;
    uint64_t * expire_timestamp;
    uint8_t * connect_token_nonce;
    int32_t * timeout;
    int * naddresses;
    pomelo_address_t * addresses;
    uint8_t * client_to_server_key;
    uint8_t * server_to_client_key;
    int64_t * client_id;
    uint8_t * user_data;
};

/// @brief Token information
typedef struct pomelo_plugin_token_info_s pomelo_plugin_token_info_t;


/// @brief Callback when plugin is going to be unloaded
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_on_unload_callback)(
    pomelo_plugin_t * plugin
);


/// @brief The common callback with socket as an argument
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_socket_common_callback)(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
);


/// @brief The common callback with socket as an argument
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_socket_listening_callback)(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    pomelo_address_t * address
);


/// @brief The common callback with socket as an argument
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_socket_connecting_callback)(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    uint8_t * connect_token
);


/// @brief The callback when session is going to send messages
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_session_send_callback)(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    pomelo_message_t * message
);


/// @brief Disconnect a session
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_session_disconnect_callback)(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
);


/// @brief Get session RTT information
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_session_get_rtt_callback)(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    uint64_t * mean,
    uint64_t * variance
);


/// @brief Set the channel mode callback
typedef int (POMELO_PLUGIN_CALL * pomelo_plugin_session_set_mode_callback)(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    pomelo_channel_mode channel_mode
);


/// @brief The task callback
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_task_callback)(
    pomelo_plugin_t * plugin,
    void * data
);


struct pomelo_plugin_s {
    /* ------------------------------------------------------------------ */
    /*                            Plugin APIs                             */
    /* ------------------------------------------------------------------ */
    
    /// @brief Configure all the callbacks
    void (POMELO_PLUGIN_CALL * configure_callbacks)(
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
    );


    /// @brief (Thread-safe) Set the associated data
    void (POMELO_PLUGIN_CALL * set_data)(pomelo_plugin_t * plugin, void * data);


    /// @brief (Thread-safe) Get associated data
    void * (POMELO_PLUGIN_CALL * get_data)(pomelo_plugin_t * plugin);

    /* ------------------------------------------------------------------ */
    /*                            Socket APIs                             */
    /* ------------------------------------------------------------------ */

    /// @brief Get number of channels of socket configuration
    size_t (POMELO_PLUGIN_CALL * socket_get_nchannels)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket
    );


    /// @brief Get the channel mode of socket configuration
    pomelo_channel_mode (POMELO_PLUGIN_CALL * socket_get_channel_mode)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket,
        size_t channel_index
    );


    /// @brief Get the current socket time
    uint64_t (POMELO_PLUGIN_CALL * socket_time)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket
    );

    /* ------------------------------------------------------------------ */
    /*                           Session APIs                             */
    /* ------------------------------------------------------------------ */

    /// @brief Create new session
    pomelo_session_t * (POMELO_PLUGIN_CALL * session_create)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket,
        int64_t client_id,
        pomelo_address_t * address
    );


    /// @brief Destroy a session
    void (POMELO_PLUGIN_CALL * session_destroy)(
        pomelo_plugin_t * plugin,
        pomelo_session_t * session
    );


    /// @brief (Thread-safe) Set private data of session
    void (POMELO_PLUGIN_CALL * session_set_private)(
        pomelo_plugin_t * plugin,
        pomelo_session_t * session,
        void * data
    );


    /// @brief (Thread-safe) Get private data of session
    void * (POMELO_PLUGIN_CALL * session_get_private)(
        pomelo_plugin_t * plugin,
        pomelo_session_t * session
    );


    /// @brief Dispatch a message to session
    void (POMELO_PLUGIN_CALL * session_receive)(
        pomelo_plugin_t * plugin,
        pomelo_session_t * session,
        size_t channel_index,
        pomelo_message_t * message
    );


    /* ------------------------------------------------------------------ */
    /*                           Message APIs                             */
    /* ------------------------------------------------------------------ */

    /// @brief Acquire a message. Messages will be released automatically
    /// when the callbacks return.
    pomelo_message_t * (POMELO_PLUGIN_CALL * message_acquire)(
        pomelo_plugin_t * plugin
    );


    /// @brief Write data to message
    /// @return 0 on success or -1 on failure
    int (POMELO_PLUGIN_CALL * message_write)(
        pomelo_plugin_t * plugin,
        pomelo_message_t * message,
        const uint8_t * buffer,
        size_t length
    );


    /// @brief Read data from message
    /// @return 0 on success or -1 on failure
    int (POMELO_PLUGIN_CALL * message_read)(
        pomelo_plugin_t * plugin,
        pomelo_message_t * message,
        uint8_t * buffer,
        size_t length
    );

    
    /// @brief Get the message length
    size_t (POMELO_PLUGIN_CALL * message_length)(
        pomelo_plugin_t * plugin,
        pomelo_message_t * message
    );

    /* ------------------------------------------------------------------ */
    /*                             Token APIs                             */
    /* ------------------------------------------------------------------ */

    /// @brief (Thread-safe) Decode the public part of connect token
    int (POMELO_PLUGIN_CALL * connect_token_decode)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket,
        uint8_t * connect_token,
        pomelo_plugin_token_info_t * token_info
    );

    /* ------------------------------------------------------------------ */
    /*                    Thread-safe executor APIs                       */
    /* ------------------------------------------------------------------ */

    /// @brief Startup the thread-safe executor. By startup, the thread-safe
    /// executor will be initialized and ready to use and the platform will be
    /// prevented to shutdown until the executor is shutdown.
    int (POMELO_PLUGIN_CALL * executor_startup)(pomelo_plugin_t * plugin);


    /// @brief Shutdown the thread-safe executor. By shutdown, the thread-safe
    /// executor will be destroyed and the platform will be allowed to shutdown.
    void (POMELO_PLUGIN_CALL * executor_shutdown)(pomelo_plugin_t * plugin);


    /// @brief (Thread-safe) Submit a task to run in the platform thread.
    /// This function is threadsafe and can be called from any thread.
    /// @return 0 on success or -1 on failure
    int (POMELO_PLUGIN_CALL * executor_submit)(
        pomelo_plugin_t * plugin,
        pomelo_plugin_task_callback callback,
        void * data
    );
};


#ifdef __cplusplus
}
#endif
#endif // ~POMELO_PLUGIN_H
