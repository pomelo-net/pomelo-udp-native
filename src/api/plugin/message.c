#include <assert.h>
#include "pomelo/errno.h"
#include "message.h"
#include "api/context.h"


pomelo_message_t * POMELO_PLUGIN_CALL pomelo_plugin_message_acquire(
    pomelo_plugin_t * plugin
) {
    assert(plugin != NULL);
    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    // Prepare the message
    pomelo_message_t * message = pomelo_context_acquire_message(impl->context);
    if (!message) return NULL;

    // Add message to the list
    if (!pomelo_list_push_back(impl->acquired_messages, message)) {
        pomelo_message_unref(message);
        return NULL;
    }

    return message;
}


int POMELO_PLUGIN_CALL pomelo_plugin_message_write(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message,
    const uint8_t * buffer,
    size_t length
) {
    assert(plugin != NULL);
    assert(message != NULL);
    assert(buffer != NULL);
    if (!plugin || !message || !buffer) return POMELO_ERR_PLUGIN_INVALID_ARG;

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    return pomelo_message_write_buffer(message, buffer, length);
}


int POMELO_PLUGIN_CALL pomelo_plugin_message_read(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message,
    uint8_t * buffer,
    size_t length
) {
    assert(plugin != NULL);
    assert(message != NULL);
    assert(buffer != NULL);

    if (!plugin || !message || !buffer) return POMELO_ERR_PLUGIN_INVALID_ARG;

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    return pomelo_message_read_buffer(message, buffer, length);
}


size_t POMELO_PLUGIN_CALL pomelo_plugin_message_length(
    pomelo_plugin_t * plugin,
    pomelo_message_t * message
) {
    assert(plugin != NULL);
    assert(message != NULL);
    if (!plugin || !message) return 0;

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    return pomelo_message_size(message);
}
