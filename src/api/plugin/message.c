#include <assert.h>
#include "pomelo/errno.h"
#include "message.h"


int POMELO_PLUGIN_CALL pomelo_plugin_message_write(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message,
    size_t length,
    const uint8_t * buffer
) {
    assert(plugin != NULL);
    assert(message != NULL);
    assert(buffer != NULL);

    if (!plugin || !message || !buffer) {
        return POMELO_ERR_PLUGIN_INVALID_ARG;
    }

    return pomelo_message_write_buffer(message, length, buffer);
}


int POMELO_PLUGIN_CALL pomelo_plugin_message_read(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message,
    size_t length,
    uint8_t * buffer
) {
    assert(plugin != NULL);
    assert(message != NULL);
    assert(buffer != NULL);

    if (!plugin || !message || !buffer) {
        return POMELO_ERR_PLUGIN_INVALID_ARG;
    }

    return pomelo_message_read_buffer(message, length, buffer);
}


size_t POMELO_PLUGIN_CALL pomelo_plugin_message_length(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message
) {
    assert(plugin != NULL);
    assert(message != NULL);

    if (!plugin || !message) {
        return 0;
    }

    return pomelo_message_size(message);
}
