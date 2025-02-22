#ifndef POMELO_COMMON_H
#define POMELO_COMMON_H

/*
    This defines some core entities, enums and constants for both core & plugin
    modules
*/

#ifdef __cplusplus
extern "C" {
#endif


/// @brief The socket
typedef struct pomelo_socket_s pomelo_socket_t;


/// @brief A session describes the connection between 2 peers.
///
/// Each connected session has an unique signature.
/// It is generated uniquely when the session is acquired from pool
/// and set zero when it is released.
/// 
/// This is useful in multi-threaded enviroment to check signature before
/// sending. Check the following example.
/// 
/// Let say, in multi-threaded enviroment, we have 2 threads: one for the 
/// logic and one for the network. They share a synchronized sending queue.
/// The logic thread puts the sending commands to the queue and the
/// network thread takes out the commands to send messages. Then, the
/// session is disconnected and it is released to the pool for later use.
/// Unfortunatly, the sending queue is not empty at this moment and new
/// session is connected, it reuses the previous session object.
/// So, by holding the pointer to the session, the following command will
/// send messages to the incorrect target - the new one.
///
/// In this case, it is necessary to double check the sending command
/// signature to avoid sending to incorrect targets.
typedef struct pomelo_session_s pomelo_session_t;

/// @brief The message for sending/receiving.
/// It contains multiple fragments.
typedef struct pomelo_message_s pomelo_message_t;

/// @brief A channel of session
typedef struct pomelo_channel_s pomelo_channel_t;


/// @brief Message delivery mode
typedef enum pomelo_channel_mode_e {
    /// @brief The unreliable mode.
    /// The packet might be out of order or it might be lost.
    POMELO_CHANNEL_MODE_UNRELIABLE,

    /// @brief The sequenced mode.
    /// The packet might be lost but the order will be maintained.
    POMELO_CHANNEL_MODE_SEQUENCED,

    /// @brief The reliable mode.
    /// The packet will be received by the target.
    POMELO_CHANNEL_MODE_RELIABLE,

    /// @brief Channel mode count
    POMELO_CHANNEL_MODE_COUNT
} pomelo_channel_mode;


/// @brief The API context interface
typedef struct pomelo_context_s pomelo_context_t;

/// @brief Plugin enviroment structure
typedef struct pomelo_plugin_s pomelo_plugin_t;


#ifdef _MSC_VER
    #define POMELO_PLUGIN_EXPORT __declspec(dllexport)
    #define POMELO_PLUGIN_CALL __stdcall
#elif defined(__GNUC__)
    #if ((__GNUC__ > 4) || (__GNUC__ == 4) && (__GNUC_MINOR__ > 2)) || __has_attribute(visibility)
        #ifdef ARM
            #define POMELO_PLUGIN_EXPORT __attribute__((externally_visible,visibility("default")))
        #else
            #define POMELO_PLUGIN_EXPORT __attribute__((visibility("default")))
        #endif
    #else
        #define POMELO_PLUGIN_EXPORT
    #endif
    #define POMELO_PLUGIN_CALL
#else
    // Unsupported
    #define POMELO_PLUGIN_EXPORT
    #define POMELO_PLUGIN_CALL
#endif


/// @brief Plugin initializing function
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_initializer)(
    pomelo_plugin_t * plugin
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_ENTITIES_H
