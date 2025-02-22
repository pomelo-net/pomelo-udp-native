#include <assert.h>
#include <string.h>
#include "codec/packed.h"
#include "delivery.h"
#include "fragment.h"


int pomelo_delivery_fragment_meta_decode(
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_payload_t * payload
) {
    assert(meta != NULL);
    assert(payload != NULL);

    if (payload->capacity < 1) {
        return -1;
    }

    // Save the current position for later restoring
    size_t position = payload->position;

    // Seek to the last byte
    size_t meta_byte = payload->data[payload->capacity - 1];
    size_t sequence_bytes = (meta_byte & 0x07) + 1;
    size_t total_fragments_bytes = ((meta_byte >> 3) & 0x01) + 1;
    size_t fragment_index_bytes = ((meta_byte >> 4) & 0x01) + 1;
    size_t bus_index_bytes = ((meta_byte >> 5) & 0x01) + 1;
    size_t fragment_type = meta_byte >> 6;

    if (fragment_type >= POMELO_FRAGMENT_TYPE_COUNT) {
        // Invalid fragment type
        return -1;
    }

    size_t meta_bytes = 1 +
        total_fragments_bytes +
        fragment_index_bytes +
        bus_index_bytes +
        sequence_bytes;

    if (payload->capacity < meta_bytes) {
        return -1;
    }

    // The new capacity
    size_t capacity = payload->capacity - meta_bytes;
    payload->position = capacity;

    uint64_t value = 0;  // buffered value
    
    // Read bus index
    pomelo_codec_read_packed_uint64(payload, bus_index_bytes, &value);
    meta->bus_index = (size_t) value;

    // Read fragment index
    pomelo_codec_read_packed_uint64(payload, fragment_index_bytes, &value);
    meta->fragment_index = (size_t) value;

    // Read total fragments
    pomelo_codec_read_packed_uint64(payload, total_fragments_bytes, &value);
    meta->total_fragments = (size_t) value;

    // Read sequence
    pomelo_codec_read_packed_uint64(payload, sequence_bytes, &value);
    meta->parcel_sequence = value;

    // The type
    meta->type = (pomelo_delivery_fragment_type) fragment_type;

    // Limit the capacity and reset position
    payload->position = position;
    payload->capacity = capacity;
    
    return 0;
}


int pomelo_delivery_fragment_meta_encode(
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_payload_t * payload
) {
    assert(meta != NULL);
    assert(payload != NULL);

    size_t bus_index_bytes = 
        pomelo_codec_calc_packed_uint64_bytes(meta->bus_index);
    size_t fragment_index_bytes =
        pomelo_codec_calc_packed_uint64_bytes(meta->fragment_index);
    size_t total_fragments_bytes =
        pomelo_codec_calc_packed_uint64_bytes(meta->total_fragments);
    size_t sequence_bytes =
        pomelo_codec_calc_packed_uint64_bytes(meta->parcel_sequence);

    if (
        bus_index_bytes > 2 ||
        fragment_index_bytes > 2 ||
        total_fragments_bytes > 2
    ) {
        return -1;
    }

    // Extend the payload capacity
    payload->capacity += POMELO_MAX_FRAGMENT_META_DATA_BYTES;

    // Write bus index
    pomelo_codec_write_packed_uint64(payload, bus_index_bytes, meta->bus_index);

    // Write fragment index
    pomelo_codec_write_packed_uint64(
        payload,
        fragment_index_bytes,
        meta->fragment_index
    );

    // Total fragments
    pomelo_codec_write_packed_uint64(
        payload,
        total_fragments_bytes,
        meta->total_fragments
    );

    // Sequence
    pomelo_codec_write_packed_uint64(
        payload,
        sequence_bytes,
        meta->parcel_sequence
    );

    size_t meta_byte =
        (meta->type << 6) |
        ((bus_index_bytes - 1) << 5) |
        ((fragment_index_bytes - 1) << 4) |
        ((total_fragments_bytes - 1) << 3) |
        (sequence_bytes - 1);

    // Encode the meta of fragment
    pomelo_payload_write_uint8(payload, (uint8_t) meta_byte);

    return 0;
}
