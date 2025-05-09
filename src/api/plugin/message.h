#ifndef POMELO_API_PLUGIN_MESSAGE_SRC_H
#define POMELO_API_PLUGIN_MESSAGE_SRC_H
#include "plugin.h"

#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Acquire a message
pomelo_message_t * POMELO_PLUGIN_CALL pomelo_plugin_message_acquire(
    pomelo_plugin_t * plugin
);


/// @brief Write data to message
int POMELO_PLUGIN_CALL pomelo_plugin_message_write(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message,
    const uint8_t * buffer,
    size_t length
);


/// @brief Read data from message
int POMELO_PLUGIN_CALL pomelo_plugin_message_read(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message,
    uint8_t * buffer,
    size_t length
);


/// @brief Get the message length
size_t POMELO_PLUGIN_CALL pomelo_plugin_message_length(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message
);

#ifdef __cplusplus
}
#endif
#endif // ~POMELO_API_PLUGIN_MESSAGE_SRC_H
