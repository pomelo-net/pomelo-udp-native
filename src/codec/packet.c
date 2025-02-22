#include <assert.h>
#include <string.h>
#include "sodium/crypto_aead_chacha20poly1305.h"
#include "base/payload.h"
#include "token.h"
#include "packet.h"
#include "packed.h"


/// The size of associated data for packets:
/// version info + protocol id + prefix byte
#define POMELO_PACKET_ASSOCIATED_DATA_BYTES (sizeof(POMELO_VERSION_INFO) + 9)


int pomelo_codec_encode_packet_header(pomelo_packet_t * packet) {
    assert(packet != NULL);

    pomelo_payload_t * header = &packet->header;
    if (packet->type == POMELO_PACKET_REQUEST) {
        // Only prefix zero for request packet
        return pomelo_payload_write_uint8(header, 0);
    }

    uint64_t sequence = packet->sequence;
    size_t sequence_bytes = pomelo_codec_calc_packed_uint64_bytes(sequence);

    // Low 4 bits of prefix contain the packet type
    // High 4 bits contain the number of bytes for sequence number
    uint8_t prefix = pomelo_codec_encode_prefix(packet->type, sequence_bytes);

    // prefix
    int ret = pomelo_payload_write_uint8(header, prefix);
    if (ret < 0) {
        return ret;
    }

    // sequence bytes
    return pomelo_codec_write_packed_uint64(header, sequence_bytes, sequence);;
}


int pomelo_codec_decode_packet_header(
    pomelo_codec_packet_header_t * header,
    pomelo_payload_t * payload
) {
    assert(header != NULL);
    assert(payload != NULL);

    // Read and decode prefix
    uint8_t prefix = 0;
    int ret = pomelo_payload_read_uint8(payload, &prefix);
    if (ret < 0) return ret;

    if (prefix == 0) {
        header->type = POMELO_PACKET_REQUEST;
        header->sequence = 0;
        header->sequence_bytes = 0;
        return 0;
    }

    pomelo_packet_type type = pomelo_codec_decode_prefix_packet_type(prefix);
    if (type >= POMELO_PACKET_TYPE_COUNT) {
        return -1; // Invalid packet type
    }

    uint8_t sequence_bytes = pomelo_codec_decode_prefix_sequence_bytes(prefix);
    if (sequence_bytes > POMELO_SEQUENCE_BYTES_MAX ||
        sequence_bytes < POMELO_SEQUENCE_BYTES_MIN
    ) {
        return -1; // Invalid sequence bytes
    }

    header->type = type;
    header->sequence_bytes = sequence_bytes;
    return pomelo_codec_read_packed_uint64(
        payload,
        header->sequence_bytes,
        &header->sequence
    );
}


/// @brief Encrypt the packet
int pomelo_codec_encrypt_packet(
    pomelo_codec_packet_context_t * context,
    pomelo_packet_t * packet
) {
    assert(context != NULL);
    assert(packet != NULL);

    if (packet->type == POMELO_PACKET_REQUEST) {
        return 0; // No encryption is performed on packet request
    }

    uint8_t prefix = pomelo_packet_prefix(packet);
    uint8_t nonce[crypto_aead_chacha20poly1305_IETF_NPUBBYTES];
    uint8_t associated_data[POMELO_PACKET_ASSOCIATED_DATA_BYTES];
    
    int ret = pomelo_codec_make_associated_data(
        context,
        associated_data,
        prefix
    );
    if (ret < 0) return ret;

    ret = pomelo_codec_packet_sequence_to_nonce(nonce, packet->sequence);
    if (ret < 0) return ret;

    // Length of per-packet data
    pomelo_payload_t * body = &packet->body;
    unsigned long long encrypted_length = 0;

    // This will encrypt the data from current position to the end of source.
    ret = crypto_aead_chacha20poly1305_ietf_encrypt(
        body->data,
        &encrypted_length,
        body->data,
        body->position,
        associated_data,
        POMELO_PACKET_ASSOCIATED_DATA_BYTES,
        NULL,
        nonce,
        context->packet_encrypt_key
    );
    if (ret < 0) return ret;

    // Update the position
    body->position = (size_t) encrypted_length;
    return 0;
}


int pomelo_codec_decrypt_packet(
    pomelo_codec_packet_context_t * context,
    pomelo_packet_t * packet
) {
    assert(context != NULL);
    assert(packet != NULL);

    uint8_t prefix = pomelo_packet_prefix(packet);
    if (prefix == 0) {
        return 0; // No decrypt is needed for packet request
    }

    pomelo_payload_t * body = &packet->body;

    // Length of per-packet data
    uint8_t nonce[crypto_aead_chacha20poly1305_IETF_NPUBBYTES];
    uint8_t associated_data[POMELO_PACKET_ASSOCIATED_DATA_BYTES];

    int ret = pomelo_codec_make_associated_data(
        context,
        associated_data,
        prefix
    );
    if (ret < 0) return ret;

    ret = pomelo_codec_packet_sequence_to_nonce(nonce, packet->sequence);
    if (ret < 0) return ret;

    unsigned long long decrypted_length;
    ret = crypto_aead_chacha20poly1305_ietf_decrypt(
        body->data,
        &decrypted_length,
        NULL,
        body->data,
        body->capacity,
        associated_data,
        POMELO_PACKET_ASSOCIATED_DATA_BYTES,
        nonce,
        context->packet_decrypt_key
    );
    if (ret < 0) return ret;

    // Update the body capacity
    body->capacity = (size_t) decrypted_length;
    return 0;
}


int pomelo_codec_decode_packet_body(pomelo_packet_t * packet) {
    assert(packet != NULL);

    switch (packet->type) {
        case POMELO_PACKET_REQUEST:
            return pomelo_codec_decode_packet_request_body(
                (pomelo_packet_request_t *) packet
            );

        case POMELO_PACKET_DENIED:
            return pomelo_codec_decode_packet_denied_body(
                (pomelo_packet_denied_t *) packet
            );

        case POMELO_PACKET_CHALLENGE:
            return pomelo_codec_decode_packet_challenge_body(
                (pomelo_packet_challenge_t *) packet
            );

        case POMELO_PACKET_RESPONSE:
            return pomelo_codec_decode_packet_response_body(
                (pomelo_packet_response_t *) packet
            );

        case POMELO_PACKET_PING:
            return pomelo_codec_decode_packet_ping_body(
                (pomelo_packet_ping_t *) packet
            );

        case POMELO_PACKET_PAYLOAD:
            return pomelo_codec_decode_packet_payload_body(
                (pomelo_packet_payload_t *) packet
            );

        case POMELO_PACKET_DISCONNECT:
            return pomelo_codec_decode_packet_disconnect_body(
                (pomelo_packet_disconnect_t *) packet
            );

        case POMELO_PACKET_PONG:
            return pomelo_codec_decode_packet_pong(
                (pomelo_packet_pong_t *) packet
            );

        default:
            return -1; // Invalid packet type
    }
}


int pomelo_codec_encode_packet_body(pomelo_packet_t * packet) {
    assert(packet != NULL);

    switch (packet->type) {
        case POMELO_PACKET_REQUEST:
            return pomelo_codec_encode_packet_request_body(
                (pomelo_packet_request_t *) packet
            );
            break;
        
        case POMELO_PACKET_DENIED:
            return pomelo_codec_encode_packet_denied_body(
                (pomelo_packet_denied_t *) packet
            );
            break;
        
        case POMELO_PACKET_CHALLENGE:
            return pomelo_codec_encode_packet_challenge_body(
                (pomelo_packet_challenge_t *) packet
            );
            break;
        
        case POMELO_PACKET_RESPONSE:
            return pomelo_codec_encode_packet_response_body(
                (pomelo_packet_response_t *) packet
            );
            break;
        
        case POMELO_PACKET_PING:
            return pomelo_codec_encode_packet_ping_body(
                (pomelo_packet_ping_t *) packet
            );
            break;
        
        case POMELO_PACKET_PAYLOAD:
            return pomelo_codec_encode_packet_payload_body(
                (pomelo_packet_payload_t *) packet
            );
            break;
        
        case POMELO_PACKET_DISCONNECT:
            return pomelo_codec_encode_packet_disconnect_body(
                (pomelo_packet_disconnect_t *) packet
            );
            break;
        
        case POMELO_PACKET_PONG:
            return pomelo_codec_encode_packet_pong_body(
                (pomelo_packet_pong_t *) packet
            );
            break;
        
        default:
            return -1; // Invalid packet type
    }
}


/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

int pomelo_codec_make_associated_data(
    pomelo_codec_packet_context_t * context,
    uint8_t * buffer,
    uint8_t prefix
) {
    assert(context != NULL);
    assert(buffer != NULL);

    pomelo_payload_t payload;
    payload.data = buffer;
    payload.position = 0;
    payload.capacity = POMELO_PACKET_ASSOCIATED_DATA_BYTES;

    // version info
    pomelo_payload_write_buffer(
        &payload,
        (const uint8_t *) POMELO_VERSION_INFO,
        POMELO_VERSION_INFO_BYTES
    );

    // protocol id
    pomelo_payload_write_uint64(&payload, context->protocol_id);

    // prefix byte
    pomelo_payload_write_uint8(&payload, prefix);

    return 0;
}

/* -------------------------------------------------------------------------- */
/*                               Packet Request                               */
/* -------------------------------------------------------------------------- */

int pomelo_codec_encode_packet_request_body(pomelo_packet_request_t * packet) {
    // This function is for client only
    assert(packet != NULL);
    pomelo_payload_t * body = &packet->base.body;

    // version info
    pomelo_payload_write_buffer(
        body,
        (const uint8_t *) POMELO_VERSION_INFO,
        POMELO_VERSION_INFO_BYTES
    );

    // protocol id
    pomelo_payload_write_uint64(body, packet->protocol_id);

    // expire timestamp
    pomelo_payload_write_uint64(body, packet->expire_timestamp);

    // connect token nonce
    pomelo_payload_write_buffer(
        body,
        packet->connect_token_nonce,
        POMELO_CONNECT_TOKEN_NONCE_BYTES
    );

    // encrypted priviate connect token data
    pomelo_payload_write_buffer(
        body,
        packet->encrypted_token,
        POMELO_CONNECT_TOKEN_PRIVATE_BYTES
    );
    return 0;
}


#define pomelo_codec_compare_version_info(buffer)                              \
    (memcmp(buffer, POMELO_VERSION_INFO, POMELO_VERSION_INFO_BYTES) == 0)

int pomelo_codec_decode_packet_request_body(pomelo_packet_request_t * packet) {
    // This is for server only
    assert(packet != NULL);
    
    pomelo_payload_t * body = &packet->base.body;

    char buffer[POMELO_VERSION_INFO_BYTES];
    pomelo_payload_read_buffer(
        body,
        (uint8_t * ) buffer,
        POMELO_VERSION_INFO_BYTES
    );

    // Compare the version info first
    if (!pomelo_codec_compare_version_info(buffer)) {
        return -1;
    }

    // protocol id
    pomelo_payload_read_uint64(body, &packet->protocol_id);

    // expire timestamp
    pomelo_payload_read_uint64(body, &packet->expire_timestamp);

    // token nonce
    pomelo_payload_read_buffer(
        body,
        packet->connect_token_nonce,
        POMELO_CONNECT_TOKEN_NONCE_BYTES
    );

    packet->token.protocol_id = packet->protocol_id;
    packet->token.expire_timestamp = packet->expire_timestamp;
    memcpy(
        packet->token.connect_token_nonce,
        packet->connect_token_nonce,
        POMELO_CONNECT_TOKEN_NONCE_BYTES
    );

    // Decrypt encrypted private connect token data
    return pomelo_connect_token_decode_private(
        body->data + body->position,
        &packet->token,
        packet->private_key
    );
}


/* -------------------------------------------------------------------------- */
/*                              Packet Denied                                 */
/* -------------------------------------------------------------------------- */

int pomelo_codec_encode_packet_denied_body(pomelo_packet_denied_t * packet) {
    (void) packet; // Ignore
    return 0;
}


int pomelo_codec_decode_packet_denied_body(pomelo_packet_denied_t * packet) {
    (void) packet; // Ignore
    return 0;
}

/* -------------------------------------------------------------------------- */
/*                             Packet Challenge                               */
/* -------------------------------------------------------------------------- */


int pomelo_codec_encode_packet_challenge_body(
    pomelo_packet_challenge_t * packet
) {
    assert(packet != NULL);
    pomelo_payload_t * body = &packet->base.body;

    // Challenge token sequence
    pomelo_payload_write_uint64(body, packet->token_sequence);

    // Challenge token data
    return pomelo_codec_encode_challenge_token(
        body,
        &packet->challenge_token,
        packet->challenge_key,
        packet->token_sequence // as a nonce
    );
}


int pomelo_codec_decode_packet_challenge_body(
    pomelo_packet_challenge_t * packet
) {
    assert(packet != NULL);

    // Decode the challenge packet (for client)
    // No challenge token decryption will be performed
    pomelo_payload_t * body = &packet->base.body;

    // Token sequence
    pomelo_payload_read_uint64(body, &packet->token_sequence);

    // Encrypted challenge token
    pomelo_payload_read_buffer(
        body,
        packet->encrypted_challenge_token,
        POMELO_CHALLENGE_TOKEN_BYTES
    );

    return 0;
}


/* -------------------------------------------------------------------------- */
/*                             Packet Response                                */
/* -------------------------------------------------------------------------- */

int pomelo_codec_encode_packet_response_body(
    pomelo_packet_response_t * packet
) {
    assert(packet != NULL);
    pomelo_payload_t * body = &packet->base.body;

    // Token sequence
    pomelo_payload_write_uint64(body, packet->token_sequence);

    // Encrypted challenge token
    pomelo_payload_write_buffer(
        body,
        packet->encrypted_challenge_token,
        POMELO_CHALLENGE_TOKEN_BYTES
    );

    return 0;
}


int pomelo_codec_decode_packet_response_body(pomelo_packet_response_t * packet) {
    assert(packet != NULL);
    pomelo_payload_t * body = &packet->base.body;

    // Token sequence
    pomelo_payload_read_uint64(body, &packet->token_sequence);

    // Decrypt challenge token
    return pomelo_codec_decode_challenge_token(
        body,
        &packet->challenge_token,
        packet->challenge_key,
        packet->token_sequence
    );
}


/* -------------------------------------------------------------------------- */
/*                               Packet Ping                                  */
/* -------------------------------------------------------------------------- */


int pomelo_codec_encode_packet_ping_body(pomelo_packet_ping_t * packet) {
    assert(packet != NULL);

    size_t time_bytes = 0; // 0 - 8
    if (packet->attach_time) {
        time_bytes = pomelo_codec_calc_packed_uint64_bytes(packet->time);
    }
    size_t client_id_bytes =     // 1 - 8
        pomelo_codec_calc_packed_int64_bytes(packet->client_id);
    size_t ping_sequence_bytes = // 1 - 2
        pomelo_codec_calc_packed_uint64_bytes(packet->ping_sequence);
    assert(ping_sequence_bytes >= 1 && ping_sequence_bytes <= 2);

    // Meta: <time_bytes(4)|client_id_bytes(3)|ping_sequence_bytes(1)>
    uint8_t meta_byte = (uint8_t) (
        ((time_bytes & 0x0F) << 4)            | // 4 bits
        (((client_id_bytes - 1) & 0x07) << 1) | // 3 bits
        ((ping_sequence_bytes - 1) & 0x01)      // 1 bit
    );

    pomelo_payload_t * body = &packet->base.body;

    // Meta (1 byte)
    pomelo_payload_write_uint8(body, meta_byte); // Meta byte

    // Time (0 to 8 bytes)
    if (time_bytes > 0) {
        pomelo_codec_write_packed_uint64(body, time_bytes, packet->time);
    }
    
    // Client ID (1 to 8 bytes)
    pomelo_codec_write_packed_int64(body, client_id_bytes, packet->client_id);
    
    // Ping sequence (1 to 2 bytes)
    pomelo_codec_write_packed_uint64(
        body,
        ping_sequence_bytes,
        packet->ping_sequence
    );

    return 0;
}


int pomelo_codec_decode_packet_ping_body(pomelo_packet_ping_t * packet) {
    assert(packet != NULL);
    pomelo_payload_t * body = &packet->base.body;

    // Read and extract meta
    uint8_t meta = 0;
    pomelo_payload_read_uint8(body, &meta);

    size_t time_bytes = (meta >> 4) & 0x0F;
    size_t client_id_bytes = ((meta >> 1) & 0x07) + 1;
    size_t ping_sequence_bytes = (meta & 0x01) + 1;

    // Time (0 to 8 bytes)
    int ret;
    if (time_bytes > 0) {
        packet->attach_time = true;
        ret = pomelo_codec_read_packed_uint64(body, time_bytes, &packet->time);
        if (ret < 0) {
            return ret;
        }
    } else {
        packet->attach_time = false;
        packet->time = 0;
    }

    // Client ID
    ret = pomelo_codec_read_packed_int64(
        body,
        client_id_bytes,
        &packet->client_id
    );
    if (ret < 0) return ret;

    // ping sequence
    return pomelo_codec_read_packed_uint64(
        body,
        ping_sequence_bytes,
        &packet->ping_sequence
    );
}


/* -------------------------------------------------------------------------- */
/*                              Packet Payload                                */
/* -------------------------------------------------------------------------- */


int pomelo_codec_encode_packet_payload_body(pomelo_packet_payload_t * packet) {
    (void) packet; // Ignore
    return 0;
}


int pomelo_codec_decode_packet_payload_body(pomelo_packet_payload_t * packet) {
    (void) packet; // Ignore
    return 0;
}


/* -------------------------------------------------------------------------- */
/*                             Packet Disconnect                              */
/* -------------------------------------------------------------------------- */


int pomelo_codec_encode_packet_disconnect_body(
    pomelo_packet_disconnect_t * packet
) {
    (void) packet; // Ignore
    return 0;
}


int pomelo_codec_decode_packet_disconnect_body(
    pomelo_packet_disconnect_t * packet
) {
    (void) packet; // Ignore
    return 0;
}


/* -------------------------------------------------------------------------- */
/*                               Packet Pong                                  */
/* -------------------------------------------------------------------------- */

int pomelo_codec_encode_packet_pong_body(pomelo_packet_pong_t * packet) {
    assert(packet != NULL);

    pomelo_payload_t * body = &packet->base.body;
    size_t ping_sequence_bytes =
        pomelo_codec_calc_packed_uint64_bytes(packet->ping_sequence);
    size_t ping_recv_time_bytes =
        pomelo_codec_calc_packed_uint64_bytes(packet->ping_recv_time);
    size_t pong_delta_time_bytes =
        pomelo_codec_calc_packed_uint64_bytes(packet->pong_delta_time);

    // Meta:
    // <ping_sequence_bytes(2)|ping_recv_time_bytes(3)|pong_delta_time_bytes(3)>
    uint8_t meta = (
        (((ping_sequence_bytes - 1) & 0x03) << 6)  | // 2 bits
        (((ping_recv_time_bytes - 1) & 0x07) << 3) | // 3 bits
        ((pong_delta_time_bytes - 1) & 0x07)         // 3 bits
    ) ;
    pomelo_payload_write_uint8(body, meta & 0xFF);

    // Packed ping sequence
    pomelo_codec_write_packed_uint64(
        body,
        ping_sequence_bytes,
        packet->ping_sequence
    );

    // Packed recv time
    pomelo_codec_write_packed_uint64(
        body,
        ping_recv_time_bytes,
        packet->ping_recv_time
    );
    
    // Packed delta time
    pomelo_codec_write_packed_uint64(
        body,
        pong_delta_time_bytes,
        packet->pong_delta_time
    );

    return 0;
}


int pomelo_codec_decode_packet_pong(pomelo_packet_pong_t * packet) {
    assert(packet != NULL);
    pomelo_payload_t * body = &packet->base.body;

    // Read & decode meta
    uint8_t meta = 0;
    pomelo_payload_read_uint8(body, &meta);

    size_t ping_sequence_bytes = ((meta >> 6) & 0x01) + 1;
    size_t ping_recv_time_bytes = ((meta >> 3) & 0x07) + 1;
    size_t pong_delta_time_bytes = (meta & 0x07) + 1;

    // Packed ping sequence
    int ret = pomelo_codec_read_packed_uint64(
        body,
        ping_sequence_bytes,
        &packet->ping_sequence
    );
    if (ret < 0) return ret;

    // Packed ping recv time
    ret = pomelo_codec_read_packed_uint64(
        body,
        ping_recv_time_bytes,
        &packet->ping_recv_time
    );
    if (ret < 0) return ret;

    // Packed pong delta time
    return pomelo_codec_read_packed_uint64(
        body,
        pong_delta_time_bytes,
        &packet->pong_delta_time
    );
}


int pomelo_codec_packet_sequence_to_nonce(uint8_t * nonce, uint64_t sequence) {
    assert(nonce != NULL);

    pomelo_payload_t payload;
    payload.data = nonce;
    payload.position = 0;
    payload.capacity = crypto_aead_chacha20poly1305_IETF_NPUBBYTES;

    // padding high bits with zero
    pomelo_payload_zero_pad(
        &payload,
        crypto_aead_chacha20poly1305_IETF_NPUBBYTES - sizeof(uint64_t)
    );

    // write the token sequence
    pomelo_payload_write_uint64(&payload, sequence);
    return 0;
}
