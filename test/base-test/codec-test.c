#include <string.h>
#include "pomelo-test.h"
#include "codec/codec.h"
#include "codec/token.h"
#include "codec/packet.h"
#include "codec/packed.h"
#include "protocol/socket.h"
#include "base-test.h"


// Environment
static pomelo_allocator_t * allocator;

// Keys
static uint8_t private_key[POMELO_KEY_BYTES];
static uint8_t challenge_key[POMELO_KEY_BYTES];
static uint8_t codec_key[POMELO_KEY_BYTES];

// Contexts
static pomelo_codec_packet_context_t codec_ctx;
static pomelo_buffer_context_root_t * buffer_ctx;

// Packets
static pomelo_packet_request_t packet_request;
static pomelo_packet_challenge_t packet_challenge;
static pomelo_packet_response_t packet_response;
static pomelo_packet_ping_t packet_ping;
static pomelo_packet_payload_t packet_payload;
static pomelo_packet_denied_t packet_denied;
static pomelo_packet_disconnect_t packet_disconnect;
static pomelo_packet_pong_t packet_pong;

// Connect token
static uint8_t connect_token[POMELO_CONNECT_TOKEN_BYTES];
static pomelo_connect_token_t token;
static pomelo_connect_token_t decoded_token;

// Temp variables
static pomelo_buffer_context_root_options_t buffer_ctx_options;

// Global variables
static uint64_t sequence;
static uint64_t token_sequence;
static uint64_t ping_sequence;


/// @brief Encode and encrypt packet
static int encode_and_encrypt_packet(pomelo_packet_t * packet) {
    int ret = pomelo_codec_encode_packet_header(packet);
    if (ret < 0) return ret;

    ret = pomelo_codec_encode_packet_body(packet);
    if (ret < 0) return ret;

    return pomelo_codec_encrypt_packet(&codec_ctx, packet);
}


/// @brief Decrypt and decode packet
static int decrypt_and_decode_packet(pomelo_packet_t * packet) {
    int ret = pomelo_codec_decrypt_packet(&codec_ctx, packet);
    if (ret < 0) return ret;

    return pomelo_codec_decode_packet_body(packet);
}


static uint64_t random_u64(void) {
    uint64_t value = 0;
    pomelo_codec_buffer_random((uint8_t *) &value, sizeof(value));
    return value;
}


static int64_t random_i64(void) {
    int64_t value = 0;
    pomelo_codec_buffer_random((uint8_t *) &value, sizeof(value));
    return value;
}


static int32_t random_i32(void) {
    int32_t value = 0;
    pomelo_codec_buffer_random((uint8_t *) &value, sizeof(value));
    return value;
}


static uint16_t random_u16(void) {
    uint16_t value = 0;
    pomelo_codec_buffer_random((uint8_t *) &value, sizeof(value));
    return value;
}


static uint8_t random_u8(void) {
    uint8_t value = 0;
    pomelo_codec_buffer_random((uint8_t *) &value, sizeof(value));
    return value;
}


/// @brief Test the connect token
static int pomelo_test_codec_connect_token(void) {
    int ret = 0;

    // Setup a connect token
    token.protocol_id = random_u64();
    token.create_timestamp = random_u64();
    token.expire_timestamp = random_u64();
    pomelo_codec_buffer_random(
        token.connect_token_nonce,
        sizeof(token.connect_token_nonce)
    );
    token.client_id = random_i64();
    token.timeout = random_i32();
    token.naddresses = random_u16() % POMELO_CONNECT_TOKEN_MAX_ADDRESSES;
    for (int i = 0; i < token.naddresses; i++) {
        token.addresses[i].type = POMELO_ADDRESS_IPV4;
        token.addresses[i].ip.v4[0] = random_u8();
        token.addresses[i].ip.v4[1] = random_u8();
        token.addresses[i].ip.v4[2] = random_u8();
        token.addresses[i].ip.v4[3] = random_u8();
        token.addresses[i].port = random_u16();
    }
    
    pomelo_codec_buffer_random(
        token.client_to_server_key,
        sizeof(token.client_to_server_key)
    );
    pomelo_codec_buffer_random(
        token.server_to_client_key,
        sizeof(token.server_to_client_key)
    );
    pomelo_codec_buffer_random(
        token.user_data,
        sizeof(token.user_data)
    );

    // Encode connect token to buffer
    ret = pomelo_connect_token_encode(connect_token, &token, private_key);
    pomelo_check(ret == 0);
 
    // Decode connect token from public part
    ret = pomelo_connect_token_decode_public(connect_token, &decoded_token);
    pomelo_check(ret == 0);

    // Decode private part
    ret = pomelo_connect_token_decode_private(
        connect_token + POMELO_CONNECT_TOKEN_PRIVATE_OFFSET,
        &decoded_token,
        private_key
    );
    pomelo_check(ret == 0);

    // Check the values
    pomelo_check(token.protocol_id == decoded_token.protocol_id);
    pomelo_check(token.create_timestamp == decoded_token.create_timestamp);
    pomelo_check(token.expire_timestamp == decoded_token.expire_timestamp);

    // Check the nonce
    ret = memcmp(
        token.connect_token_nonce,
        decoded_token.connect_token_nonce,
        sizeof(token.connect_token_nonce)
    );
    pomelo_check(ret == 0);
    pomelo_check(token.timeout == decoded_token.timeout);

    // Addresses
    pomelo_check(token.naddresses == decoded_token.naddresses);
    for (int i = 0; i < token.naddresses; i++) {
        bool cmp_ret = pomelo_address_compare(
            &token.addresses[i],
            &decoded_token.addresses[i]
        );
        pomelo_check(cmp_ret);
    }

    // Client to server key
    ret = memcmp(
        token.client_to_server_key,
        decoded_token.client_to_server_key,
        sizeof(token.client_to_server_key)
    );
    pomelo_check(ret == 0);

    // Server to client key
    ret = memcmp(
        token.server_to_client_key,
        decoded_token.server_to_client_key,
        sizeof(token.server_to_client_key)
    );
    pomelo_check(ret == 0);

    pomelo_check(token.client_id == decoded_token.client_id);

    // User data
    ret = memcmp(
        token.user_data,
        decoded_token.user_data,
        sizeof(token.user_data)
    );
    pomelo_check(ret == 0);

    return 0;
}


/// @brief Test codec request packet
static int pomelo_test_codec_request_packet(void) {
    int ret = 0;

    // Acquire a buffer to attach into it
    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    // Encode connect token again
    ret = pomelo_connect_token_encode(connect_token, &token, private_key);
    pomelo_check(ret == 0);

    // We reuse the connect token created in previous test
    pomelo_packet_request_init(&packet_request);
    pomelo_packet_attach_buffer(&packet_request.base, buffer);

    // Set the request data
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
        sizeof(packet_request.encrypted_token)
    );
    
    // Set private key before encoding
    packet_request.private_key = private_key;
    ret = encode_and_encrypt_packet(&packet_request.base);
    pomelo_check(ret == 0);

    // Save body length before reset packet
    size_t body_length = packet_request.base.body.position;
    memset(&packet_request, 0, sizeof(pomelo_packet_request_t));
    pomelo_packet_request_init(&packet_request);
    pomelo_packet_attach_buffer(&packet_request.base, buffer);
    packet_request.base.body.capacity = body_length;

    // Update the body length after attach the buffer

    pomelo_codec_packet_header_t header;
    pomelo_codec_decode_packet_header(&header, &packet_request.base.header);

    // Read header first
    pomelo_check(header.type == POMELO_PACKET_REQUEST);

    packet_request.private_key = private_key;
    ret = decrypt_and_decode_packet(&packet_request.base);
    pomelo_check(ret == 0);

    pomelo_check(packet_request.protocol_id == token.protocol_id);
    pomelo_check(packet_request.expire_timestamp == token.expire_timestamp);
    ret = memcmp(
        packet_request.connect_token_nonce,
        token.connect_token_nonce,
        sizeof(token.connect_token_nonce)
    );
    pomelo_check(ret == 0);
    pomelo_check(packet_request.token.client_id == token.client_id);
    
    // Free resources
    pomelo_buffer_unref(buffer);
    return 0;
}


/// @brief Test code challenge-response packet
static int pomelo_test_codec_challenge_response_packet(void) {
    int ret = 0;
    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    pomelo_packet_challenge_init(&packet_challenge);
    pomelo_packet_attach_buffer(&packet_challenge.base, buffer);

    packet_challenge.base.sequence = ++sequence;
    packet_challenge.challenge_key = challenge_key;
    packet_challenge.token_sequence = ++token_sequence;
    packet_challenge.challenge_token.client_id = token.client_id;
    memcpy(
        packet_challenge.challenge_token.user_data,
        token.user_data,
        sizeof(token.user_data)
    );

    // Encode the packet challenge
    ret = encode_and_encrypt_packet(&packet_challenge.base);
    pomelo_check(ret == 0);

    // Attach buffer to read again
    size_t body_length = packet_challenge.base.body.position;
    memset(&packet_challenge, 0, sizeof(pomelo_packet_challenge_t));
    pomelo_packet_challenge_init(&packet_challenge);
    pomelo_packet_attach_buffer(&packet_challenge.base, buffer);
    packet_challenge.base.body.capacity = body_length;

    pomelo_codec_packet_header_t header;
    ret = pomelo_codec_decode_packet_header(
        &header,
        &packet_challenge.base.header
    );

    pomelo_check(ret == 0);
    pomelo_check(header.type == POMELO_PACKET_CHALLENGE);
    pomelo_check(header.sequence == sequence);

    // Then decode the packet challenge
    packet_challenge.base.sequence = header.sequence;
    ret = decrypt_and_decode_packet(&packet_challenge.base);
    pomelo_check(ret == 0);

    // Build the packet response
    pomelo_packet_response_init(&packet_response);
    pomelo_packet_attach_buffer(&packet_response.base, buffer);

    // Clone the challenge token to response buffer
    packet_response.base.sequence = ++sequence;
    packet_response.token_sequence = packet_challenge.token_sequence;
    memcpy(
        packet_response.encrypted_challenge_token,
        packet_challenge.encrypted_challenge_token,
        sizeof(packet_response.encrypted_challenge_token)
    );

    // Encode the packet response
    ret = encode_and_encrypt_packet(&packet_response.base);
    pomelo_check(ret == 0);

    body_length = packet_response.base.body.position;
    memset(&packet_response, 0, sizeof(pomelo_packet_response_t));
    pomelo_packet_response_init(&packet_response);
    pomelo_packet_attach_buffer(&packet_response.base, buffer);
    packet_response.base.body.capacity = body_length;

    ret = pomelo_codec_decode_packet_header(
        &header,
        &packet_response.base.header
    );
    pomelo_check(ret == 0);
    pomelo_check(header.type == POMELO_PACKET_RESPONSE);
    pomelo_check(header.sequence == sequence);

    // Decode the packet challenge
    packet_response.base.sequence = header.sequence;
    packet_response.challenge_key = challenge_key;
    ret = decrypt_and_decode_packet(&packet_response.base);
    pomelo_check(ret == 0);

    // Check the data of response
    pomelo_check(packet_response.challenge_token.client_id == token.client_id);
    ret = memcmp(
        &packet_response.challenge_token.user_data,
        &token.user_data,
        sizeof(token.user_data)
    );
    pomelo_check(ret == 0);

    // Free resources
    pomelo_buffer_unref(buffer);
    return 0;
}


static int pomelo_test_codec_denied_packet(void) {
    int ret = 0;
    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    pomelo_packet_denied_init(&packet_denied);
    pomelo_packet_attach_buffer(&packet_denied, buffer);
    packet_denied.sequence = ++sequence;

    ret = encode_and_encrypt_packet(&packet_denied);
    pomelo_check(ret == 0);

    size_t body_length = packet_denied.body.position;
    memset(&packet_denied, 0, sizeof(pomelo_packet_denied_t));
    pomelo_packet_denied_init(&packet_denied);
    pomelo_packet_attach_buffer(&packet_denied, buffer);
    packet_denied.body.capacity = body_length;

    // Decode the packet header
    pomelo_codec_packet_header_t header = { 0 };
    ret = pomelo_codec_decode_packet_header(&header, &packet_denied.header);
    pomelo_check(ret == 0);

    pomelo_check(header.type == POMELO_PACKET_DENIED);
    pomelo_check(header.sequence == sequence);

    packet_denied.sequence = header.sequence;

    // Decode the packet
    ret = decrypt_and_decode_packet(&packet_denied);
    pomelo_check(ret == 0);

    pomelo_buffer_unref(buffer);
    return 0;
}


static int pomelo_test_codec_ping_packet(void) {
    int ret = 0;

    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    pomelo_packet_ping_init(&packet_ping);
    pomelo_packet_attach_buffer(&packet_ping.base, buffer);
    
    uint64_t time = random_u64();

    packet_ping.base.sequence = ++sequence;
    packet_ping.client_id = token.client_id;
    packet_ping.ping_sequence = ++ping_sequence;
    packet_ping.attach_time = true;
    packet_ping.time = time;

    ret = encode_and_encrypt_packet(&packet_ping.base);
    pomelo_check(ret == 0);

    size_t body_length = packet_ping.base.body.position;
    memset(&packet_ping, 0, sizeof(pomelo_packet_ping_t));
    pomelo_packet_ping_init(&packet_ping);
    pomelo_packet_attach_buffer(&packet_ping.base, buffer);
    packet_ping.base.body.capacity = body_length;

    pomelo_codec_packet_header_t header;
    ret = pomelo_codec_decode_packet_header(&header, &packet_ping.base.header);
    pomelo_check(ret == 0);

    pomelo_check(header.type == POMELO_PACKET_PING);
    pomelo_check(header.sequence == sequence);

    packet_ping.base.sequence = header.sequence;
    ret = decrypt_and_decode_packet(&packet_ping.base);
    pomelo_check(ret == 0);

    pomelo_check(packet_ping.ping_sequence == ping_sequence);
    pomelo_check(packet_ping.attach_time == true);
    pomelo_check(packet_ping.time == time);

    pomelo_buffer_unref(buffer);
    return 0;
}


static int pomelo_test_codec_payload_packet(void) {
    int ret = 0;

    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    // Setup data and attach the buffer
    pomelo_packet_payload_init(&packet_payload);
    pomelo_packet_attach_buffer(&packet_payload.base, buffer);
    packet_payload.base.sequence = ++sequence;

    pomelo_payload_t * body = &packet_payload.base.body;

    int32_t v1 = random_i32();
    uint64_t v2 = random_u64();
    pomelo_payload_write_int32(body, v1);
    pomelo_payload_write_uint64(body, v2);

    // Body & source share the same buffer
    ret = encode_and_encrypt_packet(&packet_payload.base);
    pomelo_check(ret == 0);
    size_t body_length = packet_payload.base.body.position;

    memset(&packet_payload, 0, sizeof(pomelo_packet_payload_t));
    pomelo_packet_payload_init(&packet_payload);
    pomelo_packet_attach_buffer(&packet_payload.base, buffer);
    packet_payload.base.body.capacity = body_length;

    pomelo_codec_packet_header_t header = { 0 };
    ret = pomelo_codec_decode_packet_header(
        &header,
        &packet_payload.base.header
    );
    pomelo_check(ret == 0);

    pomelo_check(header.type == POMELO_PACKET_PAYLOAD);
    pomelo_check(header.sequence == sequence);

    packet_payload.base.sequence = header.sequence;

    // Decode the packet
    ret = decrypt_and_decode_packet(&packet_payload.base);
    pomelo_check(ret == 0);

    // No need to update the payload offset here
    int32_t read_v1 = 0;
    uint64_t read_v2 = 0;
    
    ret = pomelo_payload_read_int32(body, &read_v1);
    pomelo_check(ret == 0);
    pomelo_check(read_v1 == v1);

    ret = pomelo_payload_read_uint64(body, &read_v2);
    pomelo_check(read_v2 == v2);

    pomelo_buffer_unref(buffer);
    return 0;
}


static int pomelo_test_codec_disconnect_packet(void) {
    int ret = 0;

    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    pomelo_packet_disconnect_init(&packet_disconnect);
    pomelo_packet_attach_buffer(&packet_disconnect, buffer);
    packet_disconnect.sequence = ++sequence;

    ret = encode_and_encrypt_packet(&packet_disconnect);
    pomelo_check(ret == 0);

    size_t body_length = packet_disconnect.body.position;
    memset(&packet_disconnect, 0, sizeof(pomelo_packet_disconnect_t));

    pomelo_packet_disconnect_init(&packet_disconnect);
    pomelo_packet_attach_buffer(&packet_disconnect, buffer);
    packet_disconnect.body.capacity = body_length;

    pomelo_codec_packet_header_t header = { 0 };
    ret = pomelo_codec_decode_packet_header(&header, &packet_disconnect.header);
    pomelo_check(ret == 0);

    pomelo_check(header.type == POMELO_PACKET_DISCONNECT);
    pomelo_check(header.sequence == sequence);

    packet_disconnect.sequence = header.sequence;

    // Decode the packet
    ret = decrypt_and_decode_packet(&packet_disconnect);
    pomelo_check(ret == 0);

    pomelo_buffer_unref(buffer);
    return 0;
}


static int pomelo_test_codec_pong_packet(void) {
    int ret = 0;

    pomelo_buffer_t * buffer = pomelo_buffer_context_root_acquire(buffer_ctx);
    pomelo_check(buffer != NULL);

    pomelo_packet_pong_init(&packet_pong);
    pomelo_packet_attach_buffer(&packet_pong.base, buffer);
    
    packet_pong.base.sequence = ++sequence;
    packet_pong.ping_sequence = ++ping_sequence;

    ret = encode_and_encrypt_packet(&packet_pong.base);
    pomelo_check(ret == 0);

    size_t body_length = packet_pong.base.body.position;
    memset(&packet_pong, 0, sizeof(pomelo_packet_pong_t));
    pomelo_packet_pong_init(&packet_pong);
    pomelo_packet_attach_buffer(&packet_pong.base, buffer);
    packet_pong.base.body.capacity = body_length;

    pomelo_codec_packet_header_t header;
    ret = pomelo_codec_decode_packet_header(&header, &packet_pong.base.header);
    pomelo_check(ret == 0);

    pomelo_check(header.type == POMELO_PACKET_PONG);
    pomelo_check(header.sequence == sequence);

    packet_pong.base.sequence = header.sequence;
    ret = decrypt_and_decode_packet(&packet_pong.base);
    pomelo_check(ret == 0);

    pomelo_check(packet_pong.ping_sequence == ping_sequence);

    pomelo_buffer_unref(buffer);
    return 0;
}


int pomelo_test_codec_packed(void) {
    uint64_t value = 196676118004994ULL;
    uint8_t buffer[8];
    pomelo_payload_t payload;
    payload.data = buffer;
    payload.capacity = 8;
    payload.position = 0;
    
    size_t bytes = pomelo_codec_calc_packed_uint64_bytes(value);
    pomelo_check(pomelo_codec_write_packed_uint64(&payload, bytes, value) == 0);
    pomelo_check(payload.position == bytes);

    // Read back
    payload.position = 0;
    uint64_t read_value = 0;
    int ret = pomelo_codec_read_packed_uint64(&payload, bytes, &read_value);
    pomelo_check(ret == 0);
    pomelo_check(read_value == value);
    return 0;
}


int pomelo_test_codec(void) {
    int ret;
    pomelo_check(pomelo_codec_init() == 0);

    allocator = pomelo_allocator_default();
    uint64_t alloc_bytes = pomelo_allocator_allocated_bytes(allocator);
    
    // Random values
    ret = pomelo_codec_buffer_random(private_key, sizeof(private_key));
    pomelo_check(ret == 0);
    ret = pomelo_codec_buffer_random(challenge_key, sizeof(challenge_key));
    pomelo_check(ret == 0);
    ret = pomelo_codec_buffer_random(codec_key, sizeof(codec_key));
    pomelo_check(ret == 0);
    
    codec_ctx.protocol_id = random_u64();
    codec_ctx.packet_decrypt_key = codec_key;
    codec_ctx.packet_encrypt_key = codec_key;

    // Create buffer context
    pomelo_buffer_context_root_options_init(&buffer_ctx_options);
    buffer_ctx_options.allocator = allocator;
    buffer_ctx_options.buffer_capacity = POMELO_PACKET_BUFFER_CAPACITY_DEFAULT;
    buffer_ctx = pomelo_buffer_context_root_create(&buffer_ctx_options);
    pomelo_check(buffer_ctx);

    // Run tests
    pomelo_check(pomelo_test_codec_connect_token() == 0);
    pomelo_check(pomelo_test_codec_request_packet() == 0);
    pomelo_check(pomelo_test_codec_challenge_response_packet() == 0);
    pomelo_check(pomelo_test_codec_ping_packet() == 0);
    pomelo_check(pomelo_test_codec_payload_packet() == 0);
    pomelo_check(pomelo_test_codec_disconnect_packet() == 0);
    pomelo_check(pomelo_test_codec_pong_packet() == 0);
    pomelo_check(pomelo_test_codec_denied_packet() == 0);
    pomelo_check(pomelo_test_codec_packed() == 0);

    // Cleanup
    pomelo_buffer_context_root_destroy(buffer_ctx);

    // Check memleak
    pomelo_check(alloc_bytes == pomelo_allocator_allocated_bytes(allocator));
    return 0;
}
