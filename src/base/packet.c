#include <assert.h>
#include <string.h>
#include <stddef.h>
#include "packet.h"


void pomelo_packet_request_init(pomelo_packet_request_t * packet) {
    assert(packet != NULL);
    memset(packet, 0, sizeof(pomelo_packet_request_t));
    packet->base.type = POMELO_PACKET_REQUEST;
}


void pomelo_packet_response_init(pomelo_packet_response_t * packet) {
    assert(packet != NULL);
    memset(packet, 0, sizeof(pomelo_packet_response_t));
    packet->base.type = POMELO_PACKET_RESPONSE;
}


void pomelo_packet_denied_init(pomelo_packet_denied_t * packet) {
    assert(packet != NULL);
    memset(packet, 0, sizeof(pomelo_packet_denied_t));
    packet->type = POMELO_PACKET_DENIED;
}


void pomelo_packet_challenge_init(pomelo_packet_challenge_t * packet) {
    assert(packet != NULL);
    memset(packet, 0, sizeof(pomelo_packet_challenge_t));
    packet->base.type = POMELO_PACKET_CHALLENGE;
}


void pomelo_packet_payload_init(pomelo_packet_payload_t * packet) {
    assert(packet != NULL);
    memset(packet, 0, sizeof(pomelo_packet_payload_t));
    packet->base.type = POMELO_PACKET_PAYLOAD;
}


void pomelo_packet_ping_init(pomelo_packet_ping_t * packet) {
    assert(packet != NULL);
    memset(packet, 0, sizeof(pomelo_packet_ping_t));
    packet->base.type = POMELO_PACKET_PING;
}


void pomelo_packet_disconnect_init(pomelo_packet_disconnect_t * packet) {
    assert(packet != NULL);
    memset(packet, 0, sizeof(pomelo_packet_disconnect_t));
    packet->type = POMELO_PACKET_DISCONNECT;
}


void pomelo_packet_pong_init(pomelo_packet_pong_t * packet) {
    assert(packet != NULL);
    memset(packet, 0, sizeof(pomelo_packet_pong_t));
    packet->base.type = POMELO_PACKET_PONG;
}


void pomelo_packet_reset(pomelo_packet_t * packet) {
    assert(packet != NULL);
    packet->sequence = 0;
    packet->buffer = NULL;
}


void pomelo_packet_attach_buffer(
    pomelo_packet_t * packet,
    pomelo_buffer_t * buffer
) {
    assert(packet != NULL);
    assert(buffer != NULL);
    packet->buffer = buffer;

    // Header
    packet->header.data = buffer->data;
    packet->header.position = 0;
    packet->header.capacity = POMELO_PACKET_HEADER_CAPACITY;

    // Body
    packet->body.data = buffer->data + POMELO_PACKET_HEADER_CAPACITY;
    packet->body.position = 0;
    packet->body.capacity = buffer->capacity - POMELO_PACKET_HEADER_CAPACITY;
}


bool pomelo_packet_validate_packet_body_size(
    pomelo_packet_type type,
    size_t body_size
) {
    switch (type) {
        case POMELO_PACKET_REQUEST:
            return (body_size == POMELO_PACKET_REQUEST_BODY_SIZE);

        case POMELO_PACKET_DENIED:
            return (body_size == POMELO_PACKET_DENIED_BODY_SIZE);

        case POMELO_PACKET_CHALLENGE:
            return (body_size == POMELO_PACKET_CHALLENGE_BODY_SIZE);

        case POMELO_PACKET_RESPONSE:
            return (body_size == POMELO_PACKET_RESPONSE_BODY_SIZE);

        case POMELO_PACKET_PING:
            return (
                body_size >= POMELO_PACKET_PING_BODY_MIN_SIZE &&
                body_size <= POMELO_PACKET_PING_BODY_MAX_SIZE
            );

        case POMELO_PACKET_PAYLOAD:
            return (
                body_size > 0 &&
                body_size <= POMELO_PACKET_PAYLOAD_BODY_CAPACITY_DEFAULT
            );

        case POMELO_PACKET_DISCONNECT:
            return (body_size == POMELO_PACKET_DISCONNECT_BODY_SIZE);

        case POMELO_PACKET_PONG:
            return (
                body_size >= POMELO_PACKET_PONG_BODY_MIN_SIZE &&
                body_size <= POMELO_PACKET_PONG_BODY_MAX_SIZE
            );

        default:
            return false;
    }
}
