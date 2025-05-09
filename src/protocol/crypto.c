#include <assert.h>
#include "crypto/crypto.h"
#include "crypto.h"
#include "packet.h"
#include "context.h"


/// The size of associated data for packets:
/// version info + protocol id + prefix byte
#define ASSOCIATED_DATA_BYTES (sizeof(POMELO_VERSION_INFO) + 9)


int pomelo_protocol_crypto_context_on_alloc(
    pomelo_protocol_crypto_context_t * crypto_ctx,
    pomelo_protocol_context_t * context
) {
    assert(crypto_ctx != NULL);
    crypto_ctx->ref.data = context;
    return 0;
}


int pomelo_protocol_crypto_context_init(
    pomelo_protocol_crypto_context_t * crypto_ctx
) {
    assert(crypto_ctx != NULL);
    pomelo_reference_init(
        &crypto_ctx->ref,
        (pomelo_ref_finalize_cb) pomelo_protocol_crypto_context_on_finalize
    );
    return 0;
}


void pomelo_protocol_crypto_context_on_finalize(
    pomelo_protocol_crypto_context_t * crypto_ctx
) {
    assert(crypto_ctx != NULL);
    pomelo_protocol_context_t * context = crypto_ctx->ref.data;
    pomelo_protocol_context_release_crypto_context(context, crypto_ctx);
}


int pomelo_protocol_crypto_context_ref(
    pomelo_protocol_crypto_context_t * crypto_ctx
) {
    assert(crypto_ctx != NULL);
    return pomelo_reference_ref(&crypto_ctx->ref) ? 0 : -1;
}


int pomelo_protocol_crypto_context_unref(
    pomelo_protocol_crypto_context_t * crypto_ctx
) {
    assert(crypto_ctx != NULL);
    pomelo_reference_unref(&crypto_ctx->ref);
    return 0;
}


int pomelo_protocol_crypto_context_decrypt_packet(
    pomelo_protocol_crypto_context_t * crypto_ctx,
    pomelo_buffer_view_t * view,
    pomelo_protocol_packet_header_t * header
) {
    assert(crypto_ctx != NULL);
    assert(view != NULL);
    assert(header != NULL);

    if (header->type == POMELO_PROTOCOL_PACKET_REQUEST) {
        return 0; // No need to decrypt
    }

    if (view->length < POMELO_CRYPTO_AEAD_HMAC_BYTES) {
        return -1; // Invalid encrypted length
    }

    // Make nonce and associated data
    uint8_t nonce[POMELO_CRYPTO_AEAD_NONCE_BYTES];
    uint8_t associated[ASSOCIATED_DATA_BYTES];
    size_t sequence_bytes =
        pomelo_payload_calc_packed_uint64_bytes(header->sequence);
    uint8_t prefix = pomelo_protocol_prefix_encode(header->type, sequence_bytes);
    pomelo_protocol_crypto_context_make_associated_data(
        crypto_ctx, associated, prefix
    );
    pomelo_crypto_make_nonce(nonce, sizeof(nonce), header->sequence);

    uint8_t * data = view->buffer->data + view->offset;
    return pomelo_crypto_decrypt_aead(
        data,
        &view->length,
        data,
        view->length,
        crypto_ctx->packet_decrypt_key,
        nonce,
        associated,
        sizeof(associated)
    );
}


int pomelo_protocol_crypto_context_encrypt_packet(
    pomelo_protocol_crypto_context_t * crypto_ctx,
    pomelo_buffer_view_t * view,
    pomelo_protocol_packet_header_t * header
) {
    assert(crypto_ctx != NULL);
    assert(view != NULL);
    assert(header != NULL);

    if (header->type == POMELO_PROTOCOL_PACKET_REQUEST) {
        return 0; // No need to encrypt
    }

    size_t remain = view->buffer->capacity - (view->offset + view->length);
    if (remain < POMELO_CRYPTO_AEAD_HMAC_BYTES) {
        return -1; // Not enough space
    }

    // Make nonce and associated data
    uint8_t nonce[POMELO_CRYPTO_AEAD_NONCE_BYTES];
    uint8_t ad[ASSOCIATED_DATA_BYTES];
    size_t sequence_bytes =
        pomelo_payload_calc_packed_uint64_bytes(header->sequence);
    uint8_t prefix = pomelo_protocol_prefix_encode(header->type, sequence_bytes);
    pomelo_protocol_crypto_context_make_associated_data(
        crypto_ctx, ad, prefix
    );
    pomelo_crypto_make_nonce(nonce, sizeof(nonce), header->sequence);

    uint8_t * data = view->buffer->data + view->offset;
    return pomelo_crypto_encrypt_aead(
        data,
        &view->length,
        data,
        view->length,
        crypto_ctx->packet_encrypt_key,
        nonce,
        ad,
        sizeof(ad)
    );
}


void pomelo_protocol_crypto_context_make_associated_data(
    pomelo_protocol_crypto_context_t * crypto_ctx,
    uint8_t * ad,
    uint8_t prefix
) {
    assert(crypto_ctx != NULL);
    assert(ad != NULL);

    pomelo_payload_t payload;
    payload.data = ad;
    payload.position = 0;
    payload.capacity = ASSOCIATED_DATA_BYTES;

    // version info
    pomelo_payload_write_buffer_unsafe(
        &payload,
        (const uint8_t *) POMELO_VERSION_INFO,
        POMELO_VERSION_INFO_BYTES
    );

    // protocol id
    pomelo_payload_write_uint64_unsafe(&payload, crypto_ctx->protocol_id);

    // prefix byte
    pomelo_payload_write_uint8_unsafe(&payload, prefix);
}
