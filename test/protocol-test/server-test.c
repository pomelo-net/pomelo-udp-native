#include "pomelo-test.h"
#include "pomelo/platforms/platform-uv.h"
#include "protocol/client.h"
#include "codec/codec.h"
#include "codec/packet.h"
#include "adapter-simulator.h"

/*
    Protocol server test:
        - Simulator sends request packet
        - Server responses challenge packet
        - Simulator check challenge packet and replies response packet
        - Server replies a ping packet
        - Simulator replies a ping packet as well
        - Server dispatches connected event
        - Server prepares and sends a payload to client
        - Simulator received a payload, check the content of payload
*/


// Constants
#define SERVER_ADDRESS "127.0.0.1:8888"
#define CLIENT_ADDRESS "127.0.0.1:8889"
#define MAX_CLIENTS 10
#define CONNECT_TIMEOUT 1 // seconds
#define TOKEN_EXPIRE 3600 // seconds


// Allocator, platform & context
static pomelo_allocator_t * allocator;
static pomelo_platform_t * platform;
static pomelo_buffer_context_root_t * buffer_ctx;
static pomelo_protocol_context_root_t * protocol_ctx;
static uv_loop_t uv_loop;

// Socket
static pomelo_protocol_socket_t * server;

// Keys
static uint8_t private_key[POMELO_KEY_BYTES];
static uint8_t challenge_key[POMELO_KEY_BYTES];
static uint8_t connect_token[POMELO_CONNECT_TOKEN_BYTES];

// Codec
static pomelo_codec_packet_context_t codec_ctx;

// Temp variables
static pomelo_connect_token_t token;
static pomelo_platform_uv_options_t platform_options;
static pomelo_protocol_context_root_options_t protocol_ctx_opts;
static pomelo_buffer_context_root_options_t buffer_ctx_opts;
static pomelo_protocol_server_options_t server_options;

// Packets
static pomelo_packet_request_t packet_request;
static pomelo_packet_response_t packet_response;
static pomelo_packet_ping_t packet_ping;
static pomelo_packet_payload_t packet_payload;

// Global variables
static uint64_t sequence;
static uint64_t ping_sequence;

static pomelo_address_t address;

static uint64_t protocol_id;
static int64_t client_id;
static int32_t sample_v1;
static uint64_t sample_v2;


/// @brief Encode, encrypt and dispatch the packet to client
static void delivery_outgoing_packet(pomelo_packet_t * packet) {
    int ret = 0;
    // Encode & encrypt packet
    ret = pomelo_codec_encode_packet_header(packet);
    pomelo_check(ret == 0);
    ret = pomelo_codec_encode_packet_body(packet);
    pomelo_check(ret == 0);
    ret = pomelo_codec_encrypt_packet(&codec_ctx, packet);
    pomelo_check(ret == 0);

    // Dispatch message
    pomelo_adapter_recv(server->adapter, &address, packet);
}


/// @brief Decrypt and decode incoming packet
static void process_incoming_packet(pomelo_packet_t * packet) {
    int ret = 0;

    // Rewind payloads for reading
    packet->header.capacity = packet->header.position;
    packet->header.position = 0;
    packet->body.capacity = packet->body.position;
    packet->body.position = 0;

    pomelo_codec_packet_header_t header = { 0 };
    ret = pomelo_codec_decode_packet_header(&header, &packet->header);
    pomelo_check(ret == 0);

    // Update header value
    packet->sequence = header.sequence;

    // Decrypt packet first
    ret = pomelo_codec_decrypt_packet(&codec_ctx, packet);
    pomelo_check(ret == 0);

    // Decode body
    ret = pomelo_codec_decode_packet_body(packet);
    pomelo_check(ret == 0);
}


static void process_payload_packet(pomelo_packet_payload_t * packet) {
    pomelo_track_function();
    process_incoming_packet(&packet->base);

    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    pomelo_packet_payload_init(&packet_payload);
    pomelo_packet_attach_buffer(&packet_payload.base, buffer);

    packet_payload.base.sequence = ++sequence;
    packet_payload.source = buffer;

    // Clone data from packet to new packet
    memcpy(
        packet_payload.base.body.data,
        packet->base.body.data,
        packet->base.body.capacity
    );
    packet_payload.base.body.position = packet->base.body.capacity;

    delivery_outgoing_packet(&packet_payload.base);
    pomelo_buffer_unref(buffer);
}


static void process_challenge_packet(pomelo_packet_challenge_t * packet) {
    pomelo_track_function();
    process_incoming_packet(&packet->base);

    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    pomelo_packet_response_init(&packet_response);
    pomelo_packet_attach_buffer(&packet_response.base, buffer);

    packet_response.base.sequence = ++sequence;
    packet_response.token_sequence = packet->token_sequence;
    memcpy(
        packet_response.encrypted_challenge_token,
        packet->encrypted_challenge_token,
        sizeof(packet->encrypted_challenge_token)
    );

    delivery_outgoing_packet(&packet_response.base);
    pomelo_buffer_unref(buffer);
}


static void process_ping_packet(pomelo_packet_ping_t * packet) {
    pomelo_track_function();
    process_incoming_packet(&packet->base);

    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);
    pomelo_packet_ping_init(&packet_ping);
    pomelo_packet_attach_buffer(&packet_ping.base, buffer);

    packet_ping.base.sequence = ++sequence;
    packet_ping.client_id = token.client_id;
    packet_ping.ping_sequence = ++ping_sequence;
    packet_ping.attach_time = false;
    packet_ping.time = 0;

    delivery_outgoing_packet(&packet_ping.base);
    pomelo_buffer_unref(buffer);
}


/// Handle send requests from client 
static void send_handler(
    pomelo_address_t * address,
    pomelo_packet_t * packet
) {
    (void) address; // Address is always null when send from client

    switch (packet->type) {
        case POMELO_PACKET_CHALLENGE:
            process_challenge_packet((pomelo_packet_challenge_t *) packet);
            break;

        case POMELO_PACKET_PING:
            process_ping_packet((pomelo_packet_ping_t *) packet);
            break;

        case POMELO_PACKET_PAYLOAD:
            process_payload_packet((pomelo_packet_payload_t *) packet);
            break;

        default:
            break;
    }
}


/// @brief Send request packet
static void send_request_packet(void * unused) {
    printf("[i] Start sending request packet...\n");
    (void) unused;

    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    pomelo_packet_request_init(&packet_request);
    pomelo_packet_attach_buffer(&packet_request.base, buffer);

    packet_request.base.sequence = 0; // Sequence of packet request is always 0
    packet_request.protocol_id = token.protocol_id;
    packet_request.expire_timestamp = token.expire_timestamp;
    memcpy(
        packet_request.connect_token_nonce,
        token.connect_token_nonce,
        sizeof(token.connect_token_nonce)
    );
    memcpy(
        packet_request.encrypted_token,
        connect_token + POMELO_CONNECT_TOKEN_PRIVATE_OFFSET,
        POMELO_CONNECT_TOKEN_PRIVATE_BYTES
    );

    delivery_outgoing_packet(&packet_request.base);
    pomelo_buffer_unref(buffer);
}


void pomelo_protocol_socket_on_connected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
) {
    (void) socket;
    pomelo_track_function();

    int ret = 0;

    // Prepare a buffer to send
    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    pomelo_payload_t payload;
    payload.data = buffer->data;
    payload.capacity = buffer->capacity;
    payload.position = 0;

    ret = pomelo_payload_write_int32(&payload, sample_v1);
    pomelo_check(ret == 0);
    ret = pomelo_payload_write_uint64(&payload, sample_v2);
    pomelo_check(ret == 0);
    ret = pomelo_protocol_peer_send(peer, buffer, 0, payload.position);
    pomelo_check(ret == 0);

    // Unref buffer after sending
    pomelo_buffer_unref(buffer);
}


void pomelo_protocol_socket_on_disconnected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
) {
    (void) peer;
    pomelo_track_function();
    pomelo_protocol_socket_stop(socket);
}


void pomelo_protocol_socket_on_received(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
) {
    (void) socket;
    pomelo_track_function();

    int ret = 0;

    pomelo_payload_t payload;
    payload.data = buffer->data + offset;
    payload.capacity = length;
    payload.position = 0;

    int32_t v1 = 0;
    uint64_t v2 = 0;

    ret = pomelo_payload_read_int32(&payload, &v1);
    pomelo_check(ret == 0);
    pomelo_check(v1 == sample_v1);

    ret = pomelo_payload_read_uint64(&payload, &v2);
    pomelo_check(ret == 0);
    pomelo_check(v2 == sample_v2);

    // Disconnect peer
    printf("[i] Disconnecting peer, it may take upto 1 second...\n");
    pomelo_protocol_peer_disconnect(peer);
}


void pomelo_protocol_socket_on_stopped(pomelo_protocol_socket_t * socket) {
    (void) socket;
    pomelo_track_function();
    pomelo_platform_shutdown(platform);
}


void pomelo_protocol_socket_on_connect_result(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_connect_result result
) {
    (void) socket;
    pomelo_track_function();
    printf("[i] Connect result %d\n", result);
}


int main(void) {
    printf("Test protocol server.\n");
    if (pomelo_codec_init() < 0) {
        printf("Failed to initialize codec\n");
        return -1;
    }

    int ret = 0;
    allocator = pomelo_allocator_default();
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);

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

    // Generate keys
    ret = pomelo_codec_buffer_random(private_key, sizeof(private_key));
    pomelo_check(ret == 0);
    ret = pomelo_codec_buffer_random(challenge_key, sizeof(challenge_key));
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
    ret = pomelo_address_from_string(&token.addresses[0], SERVER_ADDRESS);
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
    ret = pomelo_codec_buffer_random(
        token.user_data,
        sizeof(token.user_data)
    );
    pomelo_check(ret == 0);

    // Encode connect token
    ret = pomelo_connect_token_encode(connect_token, &token, private_key);
    pomelo_check(ret == 0);

    codec_ctx.packet_encrypt_key = token.client_to_server_key;
    codec_ctx.packet_decrypt_key = token.server_to_client_key;
    codec_ctx.protocol_id = token.protocol_id;

    // Create client address
    ret = pomelo_address_from_string(&address, CLIENT_ADDRESS);
    pomelo_check(ret == 0);

    // Create server
    pomelo_protocol_server_options_init(&server_options);
    server_options.allocator = allocator;
    server_options.context = (pomelo_protocol_context_t *) protocol_ctx;
    server_options.max_clients = MAX_CLIENTS;
    server_options.platform = platform;
    server_options.private_key = private_key;
    server_options.protocol_id = protocol_id;
    ret = pomelo_address_from_string(&server_options.address, SERVER_ADDRESS);
    pomelo_check(ret == 0);
    server = pomelo_protocol_server_create(&server_options);
    pomelo_check(server != NULL);

    // Set handler for client
    server->adapter->send_handler = send_handler;

    ret = pomelo_protocol_socket_start(server);
    pomelo_check(ret == 0);

    // After starting server, send request from client
    pomelo_platform_submit_deferred_task(
        platform,
        NULL, // group
        send_request_packet,
        NULL  // data
    );

    // Run the loop
    uv_run(&uv_loop, UV_RUN_DEFAULT);
    uv_loop_close(&uv_loop);

    // Cleanup
    pomelo_protocol_socket_destroy(server);
    pomelo_platform_uv_destroy(platform);
    pomelo_protocol_context_root_destroy(protocol_ctx);
    pomelo_buffer_context_root_destroy(buffer_ctx);

    // Check memleak
    pomelo_check(alloc_bytes == pomelo_allocator_allocated_bytes(allocator));
    printf("Test passed!\n");
    return 0;
}
