#ifndef POMELO_API_BUILTIN_CHANNEL_SRC_H
#define POMELO_API_BUILTIN_CHANNEL_SRC_H
#include "pomelo/api.h"
#include "api/channel.h"
#include "delivery/delivery.h"
#include "builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/// @brief The channel builtin info
typedef struct pomelo_channel_builtin_info_s pomelo_channel_builtin_info_t;


struct pomelo_channel_builtin_info_s {
    /// @brief The session
    pomelo_session_builtin_t * session;

    /// @brief The channel mode
    pomelo_channel_mode mode;

    /// @brief The delivery bus
    pomelo_delivery_bus_t * bus;
};


struct pomelo_channel_builtin_s {
    pomelo_channel_t base;

    /// @brief The delivery mode
    pomelo_channel_mode mode;

    /// @brief The delivery bus
    pomelo_delivery_bus_t * bus;
};


/// @brief Get the channel builtin methods table
pomelo_channel_methods_t * pomelo_channel_builtin_methods(void);


/// @brief Initialize the channel
int pomelo_channel_builtin_init(
    pomelo_channel_builtin_t * channel,
    pomelo_channel_builtin_info_t * info
);


/// @brief Cleanup the channel
void pomelo_channel_builtin_cleanup(pomelo_channel_builtin_t * channel);


/// @brief Set mode for builtin channel
int pomelo_channel_builtin_set_mode(
    pomelo_channel_builtin_t * channel,
    pomelo_channel_mode mode
);


/// @brief Get mode of builtin channel
pomelo_channel_mode pomelo_channel_builtin_get_mode(
    pomelo_channel_builtin_t * channel
);


/// @brief Send message through builtin channel
void pomelo_channel_builtin_send(
    pomelo_channel_builtin_t * channel,
    pomelo_message_t * message
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_BUILTIN_CHANNEL_SRC_H
