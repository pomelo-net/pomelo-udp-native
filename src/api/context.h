#ifndef POMELO_API_CONTEXT_SRC_H
#define POMELO_API_CONTEXT_SRC_H
#include "pomelo/api.h"
#include "utils/pool.h"
#include "base/buffer.h"
#include "base/extra.h"
#include "delivery/delivery.h"
#include "protocol/protocol.h"
#include "plugin/plugin.h"
#include "api/message.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The buffer size of fragments shared pool
#define POMELO_API_MESSAGES_POOL_BUFFER_SHARED_BUFFER_SIZE 128

/// @brief Default capacity of a message
#define POMELO_MESSAGE_DEFAULT_CAPACITY 65536

/// @brief Root context
typedef struct pomelo_context_root_s pomelo_context_root_t;

/// @brief Shared context
typedef struct pomelo_context_shared_s pomelo_context_shared_t;


/// @brief The message acquiring function
typedef pomelo_message_t * (*pomelo_context_acquire_message_fn)(
    pomelo_context_t * context,
    pomelo_message_info_t * info
);

/// @brief The message releasing function
typedef void (*pomelo_context_release_message_fn)(
    pomelo_context_t * context,
    pomelo_message_t * message
);

/// @brief The context statistic function
typedef void (*pomelo_context_statistic_fn)(
    pomelo_context_t * context,
    pomelo_statistic_api_t * statistic
);


/// @brief The context interface
struct pomelo_context_s {
    /// @brief Extra data of context
    pomelo_extra_t extra;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief Root context. If this context is root, this will be NULL.
    pomelo_context_root_t * root;

    /// @brief Acquire new message
    pomelo_context_acquire_message_fn acquire_message;

    /// @brief Release a message.
    /// By using this function, the context of message will be updated if the
    /// current context of message is different from this interface.
    pomelo_context_release_message_fn release_message;

    /// @brief Get the statistic of context
    pomelo_context_statistic_fn statistic;

    /// @brief The buffer context
    pomelo_buffer_context_t * buffer_context;

    /// @brief The protocol context
    pomelo_protocol_context_t * protocol_context;

    /// @brief The delivery context
    pomelo_delivery_context_t * delivery_context;

    /// @brief The plugin manager
    pomelo_plugin_manager_t * plugin_manager;

    /// @brief Capacity of a message
    size_t message_capacity;

    /// @brief The socket pool
    pomelo_pool_t * socket_pool;

    /// @brief The builtin session pool
    pomelo_pool_t * builtin_session_pool;

    /// @brief The builtin channel pool
    pomelo_pool_t * builtin_channel_pool;
    
    /// @brief The plugin session pool
    pomelo_pool_t * plugin_session_pool;

    /// @brief The plugin channel pool
    pomelo_pool_t * plugin_channel_pool;
};


struct pomelo_context_root_s {
    /// @brief The interface of context
    pomelo_context_t base;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The data context
    pomelo_buffer_context_t * buffer_context;

    /// @brief The delivery context
    pomelo_delivery_context_t * delivery_context;

    /// @brief [Synchronized] The API messages pool.
    pomelo_pool_t * message_pool;

    /// @brief The plugin manager
    pomelo_plugin_manager_t * plugin_manager;
};


struct pomelo_context_shared_s {
    /// @brief The interface of context
    pomelo_context_t base;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The buffer context
    pomelo_buffer_context_t * buffer_context;

    /// @brief The delivery context
    pomelo_delivery_context_t * delivery_context;

    /// @brief The message pool
    pomelo_pool_t * message_pool;
};


// The following APIs are internal usage.
// For public usage, use the interface instead.

/// @brief Prepare a message for sending
pomelo_message_t * pomelo_context_root_acquire_message(
    pomelo_context_root_t * context,
    pomelo_message_info_t * info
);


/// @brief Release a message
void pomelo_context_root_release_message(
    pomelo_context_root_t * context,
    pomelo_message_t * message
);


/// @brief Get the context statistic
void pomelo_context_root_statistic(
    pomelo_context_root_t * context,
    pomelo_statistic_api_t * statistic
);


/// @brief Destroy root context
void pomelo_context_root_destroy(pomelo_context_root_t * context);


/// @brief Destroy shared context
void pomelo_context_shared_destroy(pomelo_context_shared_t * context);


/// @brief Prepare a message for sending
pomelo_message_t * pomelo_context_shared_acquire_message(
    pomelo_context_shared_t * context,
    pomelo_message_info_t * info
);


/// @brief Release message
void pomelo_context_shared_release_message(
    pomelo_context_shared_t * context,
    pomelo_message_t * message
);


/// @brief Get the context statistic
void pomelo_context_shared_statistic(
    pomelo_context_shared_t * context,
    pomelo_statistic_api_t * statistic
);


/// @brief Acquire a message
pomelo_message_t * pomelo_context_acquire_message_ex(
    pomelo_context_t * context,
    pomelo_message_info_t * info
);


/// @brief Release a message to context
void pomelo_context_release_message(
    pomelo_context_t * context,
    pomelo_message_t * message
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_CONTEXT_SRC_H
