#include <string.h>
#include "pomelo-test.h"
#include "pomelo/api.h"
#include "pomelo/platform.h"
#include "pomelo/token.h"
#include "platform-test/platform-test.h"


#define API_TEST_PROTOCOL_ID 50
#define API_TEST_CHANNELS 10
#define API_TEST_MAX_CLIENTS 32
#define API_TEST_ADDRESS "127.0.0.1:8888"
#define API_TEST_TOKEN_EXPIRE (3600 * 1000) // 1 hour
#define API_TEST_TOKEN_TIMEOUT 1 // 1 second
#define API_TEST_CLIENT_ID 125
#define API_TEST_CHANNEL 5


// Environment
static pomelo_allocator_t * allocator;
static pomelo_context_t * context;
static pomelo_platform_t * platform;

// Keys
static uint8_t private_key[POMELO_KEY_BYTES];
static uint8_t connect_token[POMELO_CONNECT_TOKEN_BYTES];
static pomelo_connect_token_t token;

// Sockets
static pomelo_socket_t * server;
static pomelo_socket_t * client;

// Sessions
static pomelo_session_t * server_session;
static pomelo_session_t * client_session;

// Global variables
static uint8_t client_connected;
static uint8_t server_connected;

// Temp variables
static pomelo_context_root_options_t context_options;
static pomelo_socket_options_t socket_options;


static int pomelo_test_on_both_connected(void);


/// @brief Initialize connect token & keys
static int init_connect_token(void) {
    pomelo_codec_buffer_random(private_key, sizeof(private_key));

    token.protocol_id = API_TEST_PROTOCOL_ID;
    token.create_timestamp = pomelo_platform_now(platform);
    token.expire_timestamp = token.create_timestamp + API_TEST_TOKEN_EXPIRE;
    pomelo_codec_buffer_random(
        token.connect_token_nonce,
        sizeof(token.connect_token_nonce)
    );

    token.timeout = API_TEST_TOKEN_TIMEOUT;
    token.naddresses = 1;
    pomelo_address_from_string(&token.addresses[0], API_TEST_ADDRESS);

    pomelo_codec_buffer_random(
        token.client_to_server_key,
        sizeof(token.client_to_server_key)
    );
    pomelo_codec_buffer_random(
        token.server_to_client_key,
        sizeof(token.server_to_client_key)
    );
    token.client_id = API_TEST_CLIENT_ID;
    // Let the user_data empty

    int ret = pomelo_connect_token_encode(connect_token, &token, private_key);
    pomelo_check(ret == 0);

    return 0;
}


int main(void) {
    int ret = 0;

    printf("API test\n");
    allocator = pomelo_allocator_default();
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);

    // Create platform
    platform = pomelo_test_platform_create(allocator);
    pomelo_check(platform != NULL);

    // Start the platform
    pomelo_platform_startup(platform);

    // Create token
    ret = init_connect_token();
    // pomelo_check(ret == 0);

    // Create api context
    pomelo_context_root_options_init(&context_options);

    context_options.allocator = allocator;
    context = pomelo_context_root_create(&context_options);
    pomelo_check(context != NULL);

    // Test messages
    pomelo_message_t * message = pomelo_message_new(context);
    
    pomelo_check(message != NULL);
    pomelo_check(pomelo_message_size(message) == 0);
    pomelo_check(pomelo_message_write_int32(message, 1234) == 0);
    pomelo_check(pomelo_message_size(message) == 4);

    pomelo_message_t * cloned_message = pomelo_message_clone(message);
    pomelo_check(cloned_message != NULL);
    pomelo_check(pomelo_message_size(cloned_message) == 4);

    pomelo_message_unref(message);
    pomelo_message_unref(cloned_message);

    // Create server
    pomelo_socket_options_init(&socket_options);
    socket_options.allocator = allocator;
    socket_options.nchannels = API_TEST_CHANNELS;
    socket_options.platform = platform;
    socket_options.context = context;

    server = pomelo_socket_create(&socket_options);
    pomelo_check(server != NULL);

    pomelo_address_t address;
    pomelo_address_from_string(&address, API_TEST_ADDRESS);

    ret = pomelo_socket_listen(
        server,
        private_key,
        API_TEST_PROTOCOL_ID,
        API_TEST_MAX_CLIENTS,
        &address
    );
    pomelo_check(ret == 0);

    // Create client
    pomelo_socket_options_init(&socket_options);
    socket_options.allocator = allocator;
    socket_options.nchannels = API_TEST_CHANNELS;
    socket_options.platform = platform;
    socket_options.context = (pomelo_context_t *) context;

    client = pomelo_socket_create(&socket_options);
    pomelo_check(client != NULL);

    ret = pomelo_socket_connect(client, connect_token);
    pomelo_check(ret == 0);

    pomelo_test_platform_run(platform);

    // Hmm, there's a bug here.
    pomelo_socket_destroy(server);
    pomelo_socket_destroy(client);

    pomelo_context_destroy(context);
    pomelo_test_platform_destroy(platform);

    // Check memleak
    pomelo_check(alloc_bytes == pomelo_allocator_allocated_bytes(allocator));
    return 0;
}


/* Implementation of events */

void pomelo_client_on_connected(
    pomelo_socket_t * client,
    pomelo_session_t * session
) {
    (void) client;
    pomelo_track_function();
    client_session = session;

    client_connected = 1;
    if (server_connected) {
        pomelo_test_on_both_connected();
    }
}


void pomelo_client_on_disconnected(
    pomelo_socket_t * client,
    pomelo_session_t * session
) {
    (void) client;
    (void) session;

    pomelo_track_function();
}


void pomelo_client_on_received(
    pomelo_socket_t * client,
    pomelo_session_t * session,
    pomelo_message_t * message
) {
    (void) session;
    
    pomelo_track_function();

    // Check received message
    uint8_t value;
    int ret;

    ret = pomelo_message_read_buffer(message, 1, &value);
    pomelo_check(ret == 0);
    pomelo_check(value == 24);

    printf("[i] {Client} Received message is OK.\n");

    printf("[i] Stopping client & server\n");
    pomelo_socket_stop(server);
    pomelo_socket_stop(client);

    printf("[i] Shutting down the platform\n");
    pomelo_platform_shutdown(platform);
}


void pomelo_server_on_connected(
    pomelo_socket_t * server,
    pomelo_session_t * session
) {
    (void) server;

    pomelo_track_function();
    server_session = session;

    server_connected = 1;
    if (client_connected) {
        pomelo_test_on_both_connected();
    }
}


void pomelo_server_on_disconnected(
    pomelo_socket_t * server,
    pomelo_session_t * session
) {
    (void) server;
    (void) session;
    pomelo_track_function();
}


void pomelo_server_on_received(
    pomelo_socket_t * server,
    pomelo_session_t * session,
    pomelo_message_t * message
) {
    (void) server;
    (void) session;
    pomelo_track_function();

    // Check received message
    uint8_t value;
    int ret;

    ret = pomelo_message_read_buffer(message, 1, &value);
    pomelo_check(ret == 0);
    pomelo_check(value == 12);

    printf("[i] {Server} Received message is OK.\n");

    pomelo_message_t * reply =
        pomelo_message_new((pomelo_context_t *) context);
    pomelo_check(reply != NULL);

    value = 24;
    ret = pomelo_message_write_buffer(reply, 1, &value);
    pomelo_check(ret == 0);

    pomelo_channel_t * channel =
        pomelo_session_get_channel(server_session, API_TEST_CHANNEL);
    pomelo_check(channel != NULL);

    // Change the channel to reliable
    ret = pomelo_channel_set_mode(channel, POMELO_CHANNEL_MODE_RELIABLE);
    pomelo_check(ret == 0);

    ret = pomelo_session_send(
        server_session,
        API_TEST_CHANNEL,
        reply
    );
    pomelo_check(ret == 0);
}


/// Process when both client and server have connected
static int pomelo_test_on_both_connected(void) {
    pomelo_track_function();

    // Prepare a message to send from client to server
    pomelo_message_t * message =
        pomelo_message_new((pomelo_context_t *) context);
    pomelo_check(message != NULL);

    int ret;
    uint8_t value = 12;
    ret = pomelo_message_write_buffer(message, 1, &value);
    pomelo_check(ret == 0);

    pomelo_channel_t * channel =
        pomelo_session_get_channel(server_session, API_TEST_CHANNEL);
    pomelo_check(channel != NULL);

    // Change the channel to reliable
    ret = pomelo_channel_set_mode(channel, POMELO_CHANNEL_MODE_RELIABLE);
    pomelo_check(ret == 0);

    ret = pomelo_session_send(
        client_session,
        API_TEST_CHANNEL,
        message
    );
    pomelo_check(ret == 0);

    return 0;
}


void pomelo_socket_on_connected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    pomelo_check(socket != NULL);
    pomelo_check(session != NULL);

    if (socket == server) {
        pomelo_server_on_connected(socket, session);
    } else {
        pomelo_client_on_connected(socket, session);
    }
}


void pomelo_socket_on_disconnected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    pomelo_check(socket != NULL);
    pomelo_check(session != NULL);

    if (socket == server) {
        pomelo_server_on_disconnected(socket, session);
    } else {
        pomelo_client_on_disconnected(socket, session);
    }
}


void pomelo_socket_on_received(
    pomelo_socket_t * socket,
    pomelo_session_t * session,
    pomelo_message_t * message
) {
    pomelo_check(socket != NULL);
    pomelo_check(session != NULL);
    pomelo_check(message != NULL);

    if (socket == server) {
        pomelo_server_on_received(socket, session, message);
    } else {
        pomelo_client_on_received(socket, session, message);
    }
}


static void pomelo_socket_client_on_stopped(void) {
    pomelo_track_function();
}

static void pomelo_socket_server_on_stopped(void) {
    pomelo_track_function();
}

void pomelo_socket_on_stopped(pomelo_socket_t * socket) {
    if (socket == server) {
        pomelo_socket_server_on_stopped();
    } else {
        pomelo_socket_client_on_stopped();
    }
}


void pomelo_socket_on_connect_result(
    pomelo_socket_t * socket,
    pomelo_socket_connect_result result
) {
    (void) socket;
    (void) result;

    pomelo_track_function();
}

void pomelo_message_on_released(pomelo_message_t * message) {
    (void) message;
}
