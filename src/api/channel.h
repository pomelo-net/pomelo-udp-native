#ifndef POMELO_API_CHANNEL_H
#define POMELO_API_CHANNEL_H
#include "pomelo/api.h"
#include "delivery/bus.h"
#include "base/extra.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


/// @brief Sending function of channel
typedef int (*pomelo_channel_send_fn)(
    pomelo_channel_t * channel,
    pomelo_message_t * message
);


/// @brief Setting channel mode
typedef int (*pomelo_channel_set_mode_fn)(
    pomelo_channel_t * channel,
    pomelo_channel_mode mode
);


typedef pomelo_channel_mode (*pomelo_channel_get_mode_fn)(
    pomelo_channel_t * channel
);

/// @brief Channel methods table
typedef struct pomelo_channel_methods_s pomelo_channel_methods_t;


struct pomelo_channel_methods_s {
    /// @brief Sending message method
    pomelo_channel_send_fn send;

    /// @brief Set mode function
    pomelo_channel_set_mode_fn set_mode;

    /// @brief Get mode function
    pomelo_channel_get_mode_fn get_mode;
};


struct pomelo_channel_s {
    /// @brief The extra data of channel
    pomelo_extra_t extra;

    /// @brief The session of this channel
    pomelo_session_t * session;

    /// @brief Table of methods
    pomelo_channel_methods_t * methods;
};


/// @brief Initialize channel
int pomelo_channel_init(
    pomelo_channel_t * channel,
    pomelo_session_t * session
);

/// @brief Finalize channel
int pomelo_channel_finalize(
    pomelo_channel_t * channel,
    pomelo_session_t * session
);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // POMELO_API_CHANNEL_H
