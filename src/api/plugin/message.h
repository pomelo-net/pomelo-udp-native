#ifndef POMELO_API_PLUGIN_MESSAGE_SRC_H
#define POMELO_API_PLUGIN_MESSAGE_SRC_H
#include "plugin.h"

#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Write data to message
int POMELO_PLUGIN_CALL pomelo_plugin_message_write(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message,
    size_t length,
    const uint8_t * buffer
);


/// @brief Read data from message
int POMELO_PLUGIN_CALL pomelo_plugin_message_read(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message,
    size_t length,
    uint8_t * buffer
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
