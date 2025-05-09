#ifndef POMELO_COMMON_H
#define POMELO_COMMON_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The socket represents a network endpoint that can be used for either
/// client or server connections.
///
/// A socket can be configured with multiple channels for message delivery, each
/// with its own delivery mode. The number of channels and their modes are set
/// when the socket is created and cannot be changed afterwards.
///
/// For server sockets, it listens for incoming client connections. For client
/// sockets, it manages the connection to a remote server. The socket handles
/// connection establishment, session management, and message routing through
/// its configured channels.
typedef struct pomelo_socket_s pomelo_socket_t;


/// @brief A session represents a connection between two network peers.
///
/// Each active session is assigned a unique signature identifier when acquired
/// from the session pool. This signature is reset to zero when the session is
/// released back to the pool.
///
/// The signature mechanism provides important safety checks in multi-threaded
/// environments. Consider the following scenario:
///
/// In a multi-threaded application with separate logic and network threads
/// sharing a synchronized queue, the logic thread enqueues send commands while
/// the network thread processes them. If a session disconnects with pending
/// commands and its object is reused by a new connection, those commands would
/// incorrectly target the new session if only using pointers.
///
/// Therefore, validating session signatures before processing send commands is
/// essential to detect and discard stale commands that reference released
/// sessions, preventing messages from being misdirected.
typedef struct pomelo_session_s pomelo_session_t;


/// @brief The message for sending/receiving data between sessions.
///
/// Messages are used to transmit data between connected sessions through
/// channels.
/// Each message has a length and payload buffer containing the actual data.
/// 
/// When sending, the message payload is written by the sender and transmitted
/// through a channel according to its delivery mode (unreliable, sequenced, or
/// reliable).
///
/// When receiving, the message payload is read by the receiver after being
/// delivered through a channel. The delivery guarantees depend on the channel's
/// mode.
typedef struct pomelo_message_s pomelo_message_t;


/// @brief A channel represents a communication pathway within a session for
/// message delivery.
///
/// Each session can have multiple channels configured, with each channel
/// operating independently with its own delivery mode (unreliable, sequenced,
/// or reliable).
///
/// Channels provide message ordering and reliability guarantees based on their
/// mode:
/// - Unreliable channels may drop or reorder messages
/// - Sequenced channels maintain message order but may drop messages
/// - Reliable channels guarantee both delivery and ordering
///
/// The channel configuration (number and modes) is fixed when the socket is
/// created and cannot be changed during the session lifetime. Messages sent
/// through a channel are delivered according to that channel's delivery
/// guarantees.
typedef struct pomelo_channel_s pomelo_channel_t;


/// @brief Message delivery mode
///
/// Specifies how messages are delivered through a channel:
///
/// - Unreliable: Messages may be dropped or arrive out of order. Best for
///   real-time data where latest values matter more than completeness.
///
/// - Sequenced: Messages maintain ordering but may be dropped. Good for
///   periodic updates where order matters but gaps are acceptable.
///
/// - Reliable: Guarantees both delivery and ordering. Required for critical
///   data that must arrive intact and in sequence.
///
/// The delivery mode is set per-channel when configuring a socket and cannot
/// be changed after the socket is created. All messages sent through a channel
/// will follow that channel's delivery guarantees.
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
///
/// The context manages the core networking functionality and plugin system.
/// It provides:
/// - Plugin registration and management
/// - Socket creation and configuration 
/// - Global settings and state
/// - Resource allocation and cleanup
///
/// A context must be created before using any networking functionality.
/// Multiple contexts can exist independently to support different 
/// networking configurations.
typedef struct pomelo_context_s pomelo_context_t;


/// @brief Plugin enviroment structure
///
/// The plugin environment provides a structured way to extend and customize
/// the core networking functionality.
///
/// It includes:
/// - Plugin registration and management
/// - Session and channel management
/// - Message handling
typedef struct pomelo_plugin_s pomelo_plugin_t;


/// @brief The adapter
///
/// The adapter is a component that handles the low-level details of network
/// communication. It provides a platform-independent interface for sending
/// and receiving messages.
typedef struct pomelo_adapter_s pomelo_adapter_t;


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
///
/// This function is used to initialize the plugin.
/// It is called when the plugin is loaded.
///
/// The plugin can use this function to initialize its own resources.
/// 
typedef void (POMELO_PLUGIN_CALL * pomelo_plugin_initializer)(
    pomelo_plugin_t * plugin,
    uint64_t version
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_COMMON_H
