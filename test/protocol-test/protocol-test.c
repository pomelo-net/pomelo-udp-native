#include <assert.h>
#include "pomelo-test.h"
#include "pomelo/token.h"
#include "protocol/protocol.h"
#include "adapter/adapter.h"
#include "pomelo/platforms/platform-uv.h"
#include "codec/codec.h"
#include "protocol/socket.h"


/*
    With unencrypted test: We have to write a customized adapter since
    default adapter does not support unencrypted packets.
    
    Test script:
        - Client connects to server
        - After connected, client send a payload to server
        - Then, server echos the same message to client
        - Finish test
*/

// Constants
#define SOCKET_ADDRESS "127.0.0.1:8888"
#define MAX_CLIENTS 10
#define CONNECT_TIMEOUT 1 // seconds
#define TOKEN_EXPIRE 3600 // seconds


// Libraries
static uv_loop_t uv_loop;
static pomelo_allocator_t * allocator;
static pomelo_platform_t * platform;

// Contexts
static pomelo_buffer_context_root_t * buffer_ctx;
static pomelo_protocol_context_root_t * protocol_ctx;

// Sockets
static pomelo_protocol_socket_t * client;
static pomelo_protocol_socket_t * server;

// Key & token
static uint8_t private_key[POMELO_KEY_BYTES];
static uint8_t connect_token[POMELO_CONNECT_TOKEN_BYTES];

// Global variables
static int stopped_count;

static uint64_t protocol_id;
static int64_t client_id;
static int32_t sample_v1;
static uint64_t sample_v2;

// Temp variables
static pomelo_connect_token_t token;
static pomelo_platform_uv_options_t platform_options;
static pomelo_protocol_context_root_options_t protocol_ctx_opts;
static pomelo_buffer_context_root_options_t buffer_ctx_opts;
static pomelo_protocol_client_options_t client_options;
static pomelo_protocol_server_options_t server_options;


int main(void) {
    printf("Test protocol.\n");

    if (pomelo_codec_init() < 0) {
        printf("Failed to initialize codec.\n");
        return -1;
    }

    int ret = 0;
    allocator = pomelo_allocator_default();
    size_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);

    // Initialize UV loop
    uv_loop_init(&uv_loop);

    // Create platform
    pomelo_platform_uv_options_init(&platform_options);
    platform_options.allocator = allocator;
    platform_options.uv_loop = &uv_loop;
    platform = pomelo_platform_uv_create(&platform_options);
    pomelo_check(platform != NULL);
    pomelo_platform_startup(platform);

    // Create buffer context
    pomelo_buffer_context_root_options_init(&buffer_ctx_opts);
    buffer_ctx_opts.allocator = allocator;
    buffer_ctx_opts.buffer_capacity = POMELO_PACKET_BUFFER_CAPACITY_DEFAULT;
    buffer_ctx = pomelo_buffer_context_root_create(&buffer_ctx_opts);
    pomelo_check(buffer_ctx != NULL);

    // Create protocol context
    pomelo_protocol_context_root_options_init(&protocol_ctx_opts);
    protocol_ctx_opts.allocator = allocator;
    protocol_ctx_opts.buffer_context = buffer_ctx;
    protocol_ctx_opts.packet_capacity = POMELO_PACKET_BUFFER_CAPACITY_DEFAULT;
    protocol_ctx = pomelo_protocol_context_root_create(&protocol_ctx_opts);
    pomelo_check(protocol_ctx != NULL);

    // Generate private key
    ret = pomelo_codec_buffer_random(private_key, sizeof(private_key));
    pomelo_check(ret == 0);

    // Random values
    ret = pomelo_codec_buffer_random(
        (uint8_t *) &protocol_id,
        sizeof(protocol_id)
    );
    pomelo_check(ret == 0);
    ret = pomelo_codec_buffer_random(
        (uint8_t *) &client_id,
        sizeof(client_id)
    );
    pomelo_check(ret == 0);
    ret = pomelo_codec_buffer_random(
        (uint8_t *) &sample_v1,
        sizeof(sample_v1)
    );
    pomelo_check(ret == 0);
    ret = pomelo_codec_buffer_random(
        (uint8_t *) &sample_v2,
        sizeof(sample_v2)
    );
    pomelo_check(ret == 0);

    // Create connect token
    token.protocol_id = protocol_id;
    token.create_timestamp = pomelo_platform_now(platform);
    token.expire_timestamp = token.create_timestamp + TOKEN_EXPIRE * 1000ULL;
    ret = pomelo_codec_buffer_random(
        token.connect_token_nonce,
        sizeof(token.connect_token_nonce)
    );
    pomelo_check(ret == 0);
    token.timeout = CONNECT_TIMEOUT;
    token.naddresses = 1;
    ret = pomelo_address_from_string(&token.addresses[0], SOCKET_ADDRESS);
    pomelo_check(ret == 0);

    ret = pomelo_codec_buffer_random(
        token.client_to_server_key,
        sizeof(token.client_to_server_key)
    );
    pomelo_check(ret == 0);

    ret = pomelo_codec_buffer_random(
        token.server_to_client_key,
        sizeof(token.server_to_client_key)
    );
    pomelo_check(ret == 0);
    token.client_id = client_id;

    // Encode connect token
    ret = pomelo_connect_token_encode(connect_token, &token, private_key);
    pomelo_check(ret == 0);

    // Create client
    pomelo_protocol_client_options_init(&client_options);
    client_options.allocator = allocator;
    client_options.context = (pomelo_protocol_context_t *) protocol_ctx;
    client_options.platform = platform;
    client_options.connect_token = connect_token;
    client = pomelo_protocol_client_create(&client_options);
    pomelo_check(client != NULL);

    // Create server
    pomelo_protocol_server_options_init(&server_options);
    server_options.allocator = allocator;
    server_options.context = (pomelo_protocol_context_t *) protocol_ctx;
    server_options.platform = platform;
    server_options.private_key = private_key;
    server_options.protocol_id = protocol_id;
    server_options.max_clients = MAX_CLIENTS;
    ret = pomelo_address_from_string(&server_options.address, SOCKET_ADDRESS);
    pomelo_check(ret == 0);
    server = pomelo_protocol_server_create(&server_options);
    pomelo_check(server != NULL);

    // Check the capacity of server
    pomelo_adapter_t * adapter = server->adapter;
    uint8_t capacity = pomelo_adapter_get_capability(adapter);
    if (capacity & POMELO_ADAPTER_CAPABILITY_ENCRYPTED) {
        printf("Adapter capacity: Encrypted\n");
    }
    if (capacity & POMELO_ADAPTER_CAPABILITY_UNENCRYPTED) {
        printf("Adapter capacity: Unencrypted\n");
    }

    // Start server
    ret = pomelo_protocol_socket_start(server);
    pomelo_check(ret == 0);

    // Start client
    ret = pomelo_protocol_socket_start(client);
    pomelo_check(ret == 0);

    // Run
    uv_run(&uv_loop, UV_RUN_DEFAULT);

    // Cleanup
    uv_loop_close(&uv_loop);
    pomelo_protocol_socket_destroy(server);
    pomelo_protocol_socket_destroy(client);
    pomelo_platform_uv_destroy(platform);
    pomelo_protocol_context_root_destroy(protocol_ctx);
    pomelo_buffer_context_root_destroy(buffer_ctx);

    // Check memleak
    pomelo_check(alloc_bytes == pomelo_allocator_allocated_bytes(allocator));
    printf("Test passed!\n");
    return 0;
}

/* -------------------------------------------------------------------------- */
/*                         Socket callbacks implementation                    */
/* -------------------------------------------------------------------------- */

void pomelo_protocol_socket_on_connected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
) {
    assert(socket != NULL);
    assert(peer != NULL);

    if (socket == client) {
        printf("Client connected.\n");
        // After connected, client will send a payload to server
        pomelo_buffer_t * buffer =
            pomelo_buffer_context_root_acquire(buffer_ctx);
        pomelo_check(buffer != NULL);
        memset(buffer->data, 0, buffer->capacity);

        pomelo_payload_t payload;
        payload.data = buffer->data;
        payload.capacity = POMELO_PACKET_PAYLOAD_BODY_CAPACITY_DEFAULT;
        payload.position = 0;

        pomelo_payload_write_int32(&payload, sample_v1);
        pomelo_payload_write_uint64(&payload, sample_v2);
        
        int ret = pomelo_protocol_peer_send(peer, buffer, 0, payload.position);
        pomelo_check(ret == 0);
        pomelo_buffer_unref(buffer);
    } else {
        printf("Server connected.\n");
    }
}


void pomelo_protocol_socket_on_disconnected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
) {
    assert(socket != NULL);
    (void) peer;

    // Stop server socket when client disconnects
    if (socket == server) {
        pomelo_protocol_socket_stop(socket);
    }
}


void pomelo_protocol_socket_on_received(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
) {
    assert(socket != NULL);
    assert(peer != NULL);
    assert(buffer != NULL);

    if (socket == server) {
        printf("Server got %zu bytes from client\n", length);
        // Echo the payload
        pomelo_protocol_peer_send(peer, buffer, offset, length);
    } else {
        printf("Client got %zu bytes from server\n", length);
        int32_t v1 = 0;
        uint64_t v2 = 0;

        pomelo_payload_t payload;
        payload.data = buffer->data + offset;
        payload.position = 0;
        payload.capacity = length;
        
        int ret = pomelo_payload_read_int32(&payload, &v1);
        pomelo_check(ret == 0);
        pomelo_check(v1 == sample_v1);

        ret = pomelo_payload_read_uint64(&payload, &v2);
        pomelo_check(ret == 0);
        pomelo_check(v2 == sample_v2);
        pomelo_protocol_peer_disconnect(peer);
        printf("Disconnecting... It may take upto 1 second...\n");
    }
}


void pomelo_protocol_socket_on_stopped(pomelo_protocol_socket_t * socket) {
    (void) socket;
    if (++stopped_count == 2) {
        printf("Shutting down...\n");
        pomelo_platform_shutdown(platform);
    }
}


void pomelo_protocol_socket_on_connect_result(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_connect_result result
) {
    (void) socket;
    if (result != POMELO_PROTOCOL_SOCKET_CONNECT_SUCCESS) {
        // Failed to connect to server, stop server
        printf("Failed to connect to server\n");
        pomelo_protocol_socket_stop(server);
    }
}
