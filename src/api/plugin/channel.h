#ifndef POMELO_API_PLUGIN_CHANNEL_SRC_H
#define POMELO_API_PLUGIN_CHANNEL_SRC_H
#include "api/channel.h"
#include "plugin.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The channel plugin info
typedef struct pomelo_channel_plugin_info_s pomelo_channel_plugin_info_t;


struct pomelo_channel_plugin_info_s {
    /// @brief The session
    pomelo_session_plugin_t * session;

    /// @brief The index of channel
    size_t index;

    /// @brief The mode of channel
    pomelo_channel_mode mode;
};


struct pomelo_channel_plugin_s {
    /// @brief The base channel
    pomelo_channel_t base;

    /// @brief Channel mode
    pomelo_channel_mode mode;

    /// @brief Index of channel
    size_t index;
};


/// @brief Get the channel builtin methods table
pomelo_channel_methods_t * pomelo_channel_plugin_methods(void);


/// @brief Initialize the channel
int pomelo_channel_plugin_init(
    pomelo_channel_plugin_t * channel,
    pomelo_channel_plugin_info_t * info
);


/// @brief Cleanup the channel
void pomelo_channel_plugin_cleanup(pomelo_channel_plugin_t * channel);


/// @brief Set mode for builtin channel
int pomelo_channel_plugin_set_mode(
    pomelo_channel_plugin_t * channel,
    pomelo_channel_mode mode
);


/// @brief Get mode of builtin channel
pomelo_channel_mode pomelo_channel_plugin_get_mode(
    pomelo_channel_plugin_t * channel
);


/// @brief Send message through builtin channel
void pomelo_channel_plugin_send(
    pomelo_channel_plugin_t * channel,
    pomelo_message_t * message
);


#ifdef __cplusplus
}
#endif
#endif // ~POMELO_API_PLUGIN_CHANNEL_SRC_H
