#include <assert.h>
#include <string.h>
#include "pomelo-test.h"
#include "pomelo/api.h"
#include "pomelo/platforms/platform-uv.h"
#include "pomelo/platform.h"
#include "pomelo/token.h"
#include "pomelo/plugin.h"
#include "platform/platform.h"
#include "delivery/delivery.h"


static pomelo_allocator_t * allocator = NULL;
static uv_loop_t * uv_loop = NULL;
static pomelo_context_t * context = NULL;
static pomelo_platform_t * platform = NULL;
static pomelo_socket_t * server = NULL;
static pomelo_plugin_t * plugin = NULL;

static size_t total_channels = 3;
static pomelo_channel_mode channel_modes[] = {
    POMELO_CHANNEL_MODE_UNRELIABLE,
    POMELO_CHANNEL_MODE_RELIABLE,
    POMELO_CHANNEL_MODE_SEQUENCED
};
static int64_t client_id = 1254;
static uint8_t address_host[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};
static uint16_t address_port = 32141;
void * private_data = (void *) 0x12345678;


void plugin_demo_entry(pomelo_plugin_t *plugin);
POMELO_PLUGIN_ENTRY_REGISTER(plugin_demo_entry)


int main(void) {
    printf("Plugin test\n");

    /* Init */
    allocator = pomelo_allocator_default();
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);

    // Create UV loop
    uv_loop = pomelo_allocator_malloc_t(allocator, uv_loop_t);
    uv_loop_init(uv_loop);

    // Create platforms
    pomelo_platform_uv_options_t platform_options;
    pomelo_platform_uv_options_init(&platform_options);

    platform_options.allocator = allocator;
    platform_options.uv_loop = uv_loop;

    platform = pomelo_platform_uv_create(&platform_options);
    pomelo_check(platform != NULL);

    // Start the platform
    pomelo_platform_startup(platform);

    // Create api context
    pomelo_context_root_options_t context_options;
    pomelo_context_root_options_init(&context_options);

    context_options.allocator = allocator;
    context = pomelo_context_root_create(&context_options);
    pomelo_check(context != NULL);
    /* End of init */

    /* ------------------------------------------------------------------ */

    // Load an initializer
    pomelo_plugin_initializer initializer =
        pomelo_plugin_load_by_name("pomelo-test-demo-plugin");

    pomelo_check(initializer != NULL);
    initializer(NULL); // Try to call it

    // Register plugin
    plugin = pomelo_plugin_register(
        allocator,
        (pomelo_context_t *) context,
        platform,
        pomelo_plugin_initializer_entry
    );
    pomelo_check(plugin != NULL);

    // Create new socket
    pomelo_socket_options_t options;
    pomelo_socket_options_init(&options);
    options.allocator = allocator;
    options.context = (pomelo_context_t *) context;
    options.platform = platform;
    options.nchannels = total_channels;
    options.channel_modes = channel_modes;

    server = pomelo_socket_create(&options);
    pomelo_check(server != NULL);

    uv_run(uv_loop, UV_RUN_DEFAULT);
    uv_loop_close(uv_loop);

    /* ------------------------------------------------------------------ */

    /* Cleanup */
    pomelo_socket_destroy(server);
    pomelo_context_destroy(context);
    pomelo_platform_uv_destroy(platform);
    pomelo_allocator_free(allocator, uv_loop);
    /* End of cleanup */

    pomelo_check(alloc_bytes == pomelo_allocator_allocated_bytes(allocator));

    printf("Plugin test passed\n");
    return 0;
}


void pomelo_socket_on_connected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    pomelo_track_function();
    pomelo_check(socket != NULL);
    pomelo_check(session != NULL);

    // Send a message to this session
    pomelo_message_t * message =
        pomelo_message_new((pomelo_context_t *) context);
    pomelo_check(message != NULL);

    int ret = pomelo_message_write_int32(message, 1234);
    pomelo_check(ret == 0);

    ret = pomelo_session_send(session, 0, message);
    pomelo_check(ret == 0);

    // => plugin_demo_session_send
}


void pomelo_socket_on_disconnected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    pomelo_track_function();
    pomelo_check(socket != NULL);
    pomelo_check(session != NULL);
}


void pomelo_socket_on_received(
    pomelo_socket_t * socket,
    pomelo_session_t * session,
    pomelo_message_t * message
) {
    pomelo_track_function();
    pomelo_check(socket != NULL);
    pomelo_check(session != NULL);
    pomelo_check(message != NULL);

    int32_t value = 0;
    int ret = pomelo_message_read_int32(message, &value);
    pomelo_check(ret == 0);
    pomelo_check(value == 4455);

    // Done testing
    pomelo_platform_shutdown(platform);
}


void pomelo_socket_on_stopped(pomelo_socket_t * socket) {
    pomelo_track_function();
    (void) socket;
}


void pomelo_socket_on_connect_result(
    pomelo_socket_t * socket,
    pomelo_socket_connect_result result
) {
    pomelo_track_function();
    (void) socket;
    (void) result;

}

/* -------------------------------------------------------------------------- */
/*                                 Callbacks                                  */
/* -------------------------------------------------------------------------- */


void plugin_demo_socket_on_created_deferred(void * data) {
    pomelo_track_function();
    assert(data == plugin);

    // Create new session and dispatch connected
    pomelo_address_t address;
    address.type = POMELO_ADDRESS_IPV6;
    address.port = address_port;
    memcpy(address.ip.v6, address_host, sizeof(address.ip.v6));

    plugin->session_create(
        plugin,
        server,
        client_id,
        &address,
        private_data,
        data // Use plugin as private data
    );

    // => plugin_demo_session_create
}


/* -------------------------------------------------------------------------- */
/*                           Plugin implementation                            */
/* -------------------------------------------------------------------------- */

void POMELO_PLUGIN_CALL plugin_demo_on_unload(
    pomelo_plugin_t * plugin
) {
    (void) plugin;
    return;
}


void POMELO_PLUGIN_CALL plugin_demo_socket_on_created(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(socket != NULL);

    // Check the channels mode
    size_t nchannels = plugin->socket_get_nchannels(plugin, socket);
    pomelo_check(nchannels == total_channels);

    for (size_t i = 0; i < nchannels; i++) {
        int channel_mode = plugin->socket_get_channel_mode(plugin, socket, i);
        pomelo_check(channel_mode == ((int) channel_modes[i]));
    }

    // Submit next tick
    pomelo_platform_submit_deferred_task(
        platform,
        NULL,
        plugin_demo_socket_on_created_deferred,
        plugin
    );
}


void POMELO_PLUGIN_CALL plugin_demo_socket_on_destroyed(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(socket != NULL);
}


void POMELO_PLUGIN_CALL plugin_demo_socket_on_listening(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    pomelo_address_t * address
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(socket != NULL);
    (void) address;
}


void POMELO_PLUGIN_CALL plugin_demo_socket_on_connecting(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    uint8_t * connect_token
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(socket != NULL);
    (void) connect_token;
}


void POMELO_PLUGIN_CALL plugin_demo_socket_on_stopped(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(socket != NULL);
}


void POMELO_PLUGIN_CALL plugin_demo_session_create(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    pomelo_session_t * session,
    void * callback_data,
    pomelo_plugin_error_t error
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(socket != NULL);
    pomelo_check(session != NULL);
    pomelo_check(callback_data == plugin);
    pomelo_check(error == POMELO_PLUGIN_ERROR_OK);
    pomelo_check(session != NULL);

    void * data = plugin->session_get_private(plugin, session);
    pomelo_check(data == private_data);

    // pomelo_socket_on_connected
}


void POMELO_PLUGIN_CALL plugin_demo_session_receive(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    void * callback_data,
    pomelo_message_t * message,
    pomelo_plugin_error_t error
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(session != NULL);
    pomelo_check(message != NULL);
    pomelo_check(channel_index == 1); // Receiving channel is 1
    pomelo_check(callback_data == NULL);
    pomelo_check(message != NULL);
    pomelo_check(error == POMELO_PLUGIN_ERROR_OK);

    // Write to message
    int ret = pomelo_message_write_int32(message, 4455);
    pomelo_check(ret == 0);

    // pomelo_socket_on_received
}


void POMELO_PLUGIN_CALL plugin_demo_session_disconnect(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(session != NULL);
}


void POMELO_PLUGIN_CALL plugin_demo_session_get_rtt(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    uint64_t * mean,
    uint64_t * variance
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(session != NULL);
    pomelo_check(mean != NULL);
    pomelo_check(variance != NULL);
}


int POMELO_PLUGIN_CALL plugin_demo_session_set_mode(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    pomelo_channel_mode channel_mode
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(session != NULL);
    (void) channel_index;
    (void) channel_mode;

    return 0;
}


void POMELO_PLUGIN_CALL plugin_demo_session_send(
    pomelo_plugin_t * plugin,
    pomelo_session_t * session,
    size_t channel_index,
    pomelo_message_t * message
) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);
    pomelo_check(session != NULL);
    pomelo_check(message != NULL);

    pomelo_check(channel_index == 0);
    int32_t value = 0;
    int ret = pomelo_message_read_int32(message, &value);
    pomelo_check(ret == 0);
    pomelo_check(value == 1234);
    pomelo_check(channel_index == 0);

    // Prepare on receive
    plugin->session_receive(plugin, session, 1, NULL);
}


void plugin_demo_entry(pomelo_plugin_t * plugin) {
    pomelo_track_function();
    pomelo_check(plugin != NULL);

    // Configure the plugin
    plugin->configure_callbacks(
        plugin,
        plugin_demo_on_unload,
        plugin_demo_socket_on_created,
        plugin_demo_socket_on_destroyed,
        plugin_demo_socket_on_listening,
        plugin_demo_socket_on_connecting,
        plugin_demo_socket_on_stopped,
        plugin_demo_session_create,
        plugin_demo_session_receive,
        plugin_demo_session_disconnect,
        plugin_demo_session_get_rtt,
        plugin_demo_session_set_mode,
        plugin_demo_session_send
    );
}


void pomelo_message_on_released(pomelo_message_t * message) {
    (void) message;
}
