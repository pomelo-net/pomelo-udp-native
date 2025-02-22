#ifndef POMELO_PLUGIN_H
#define POMELO_PLUGIN_H
#include <stdint.h>
#include <stddef.h>
#include "pomelo/common.h"
#include "pomelo/address.h"

/*
    These are the APIs for a plugin.
    With threadsafe functions, they can be called in any thread.
    With non-threadsafe functions, they can only be called in plugin callbacks.
*/


#ifdef __cplusplus
extern "C" {
#endif


/// Entry register
#define POMELO_PLUGIN_ENTRY_REGISTER(entry)                                    \
POMELO_PLUGIN_EXPORT void POMELO_PLUGIN_CALL                                   \
pomelo_plugin_initializer_entry(pomelo_plugin_t * plugin) { entry(plugin); }


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


enum pomelo_plugin_error_e {
    POMELO_PLUGIN_ERROR_ACQUIRE_MESSAGE = -5,
    POMELO_PLUGIN_ERROR_ACQUIRE_SESSION = -4,
    POMELO_PLUGIN_ERROR_SUBMIT_COMMAND = -3,
    POMELO_PLUGIN_ERROR_ACQUIRE_COMMAND = -2,
    POMELO_PLUGIN_ERROR_INVALID_ARGS = -1,
    POMELO_PLUGIN_ERROR_OK = 0,
};


/// @brief Error code for plugin callbacks
typedef enum pomelo_plugin_error_e pomelo_plugin_error_t;


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


/// @brief The common callback with session as an argument
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_session_create_callback)(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    pomelo_session_t * session,
    void * callback_data,
    pomelo_plugin_error_t error
);


/// @brief Prepare receiving new message for session
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_session_receive_callback)(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    void * callback_data,
    pomelo_message_t * message,
    pomelo_plugin_error_t error
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


/// @brief Send message through a channel
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_session_send_callback)(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    pomelo_message_t * message
);


struct pomelo_plugin_s {
    /* ------------------------------------------------------------------ */
    /*                            Plugin APIs                             */
    /* ------------------------------------------------------------------ */
    
    /// @brief (Non-threadsafe) Configure all the callbacks
    void (POMELO_PLUGIN_CALL * configure_callbacks)(
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
        pomelo_plugin_session_set_mode_callback session_set_mode_callback,
        pomelo_plugin_session_send_callback session_send_callback
    );

    /// @brief (Threadsafe) Set the associated data
    void (POMELO_PLUGIN_CALL * set_data)(
        pomelo_plugin_t * plugin,
        void * data
    );

    /// @brief (Threadsafe) Get associated data
    void * (POMELO_PLUGIN_CALL * get_data)(pomelo_plugin_t * plugin);

    /* ------------------------------------------------------------------ */
    /*                            Socket APIs                             */
    /* ------------------------------------------------------------------ */

    /// @brief (Non-threadsafe) Get number of channels of socket configuration
    size_t (POMELO_PLUGIN_CALL * socket_get_nchannels)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket
    );


    /// @brief (Non-threadsafe) Get the channel mode of socket configuration
    pomelo_channel_mode (POMELO_PLUGIN_CALL * socket_get_channel_mode)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket,
        size_t channel_index
    );

    /// @brief (Non-threadsafe) Attach the socket with plugin. This will make
    /// sure the socket will not completely be stopped until it is detached
    /// from socket.
    void (POMELO_PLUGIN_CALL * socket_attach)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket
    );

    /// @brief (Threadsafe) Detach the socket from this plugin. This will allow
    /// main socket to stop completely
    void (POMELO_PLUGIN_CALL * socket_detach)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket
    );

    /// @brief (Threadsafe) Get the current socket time
    uint64_t (POMELO_PLUGIN_CALL * socket_time)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket
    );

    /* ------------------------------------------------------------------ */
    /*                           Session APIs                             */
    /* ------------------------------------------------------------------ */

    /// @brief (Thread-safe) Create new session
    void (POMELO_PLUGIN_CALL * session_create)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket,
        int64_t client_id,
        pomelo_address_t * address,
        void * private_data,
        void * callback_data
    ); // => socket_on_created_callback


    /// @brief (Thread-safe) Destroy the session
    void (POMELO_PLUGIN_CALL * session_destroy)(
        pomelo_plugin_t * plugin,
        pomelo_session_t * session
    );


    /// @brief (Thread-safe) Get private data of session
    void * (POMELO_PLUGIN_CALL * session_get_private)(
        pomelo_plugin_t * plugin,
        pomelo_session_t * session
    );


    /// @brief (Thread-safe) Dispatch on received event of session
    void (POMELO_PLUGIN_CALL * session_receive)(
        pomelo_plugin_t * plugin,
        pomelo_session_t * session,
        size_t channel_index,
        void * callback_data
    ); // => session_receive_callback


    /// @brief Get the signature of session
    uint64_t (POMELO_PLUGIN_CALL * session_signature)(
        pomelo_plugin_t * plugin,
        pomelo_session_t * session
    );

    /* ------------------------------------------------------------------ */
    /*                           Message APIs                             */
    /* ------------------------------------------------------------------ */

    /*
        All following APIs are only available in message callback functions
    */

    /// @brief (Non-threadsafe) Write data to message
    /// Only available inside callback `pomelo_plugin_session_receive_callback`
    /// @return 0 on success or -1 on failure
    int (POMELO_PLUGIN_CALL * message_write)(
        pomelo_plugin_t * plugin,
        pomelo_message_t * message,
        size_t length,
        const uint8_t * buffer
    );


    /// @brief (Non-threadsafe) Read data from message
    /// Only available inside callback `pomelo_plugin_session_send_callback`
    /// @return 0 on success or -1 on failure
    int (POMELO_PLUGIN_CALL * message_read)(
        pomelo_plugin_t * plugin,
        pomelo_message_t * message,
        size_t length,
        uint8_t * buffer
    );

    
    /// @brief Get the message length
    size_t (POMELO_PLUGIN_CALL * message_length)(
        pomelo_plugin_t * plugin,
        pomelo_message_t * message
    );

    /* ------------------------------------------------------------------ */
    /*                             Token APIs                             */
    /* ------------------------------------------------------------------ */

    /// @brief (Threadsafe) Decode the public part of connect token
    int (POMELO_PLUGIN_CALL * connect_token_decode)(
        pomelo_plugin_t * plugin,
        pomelo_socket_t * socket,
        uint8_t * connect_token,
        pomelo_plugin_token_info_t * token_info
    );
};


#ifdef __cplusplus
}
#endif
#endif // ~POMELO_PLUGIN_H
