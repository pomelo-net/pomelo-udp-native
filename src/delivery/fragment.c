#include <assert.h>
#include <string.h>
#include "delivery.h"
#include "fragment.h"
#include "context.h"


void pomelo_delivery_fragment_init(pomelo_delivery_fragment_t * fragment) {
    assert(fragment != NULL);
    memset(fragment, 0, sizeof(pomelo_delivery_fragment_t));
}


void pomelo_delivery_fragment_cleanup(pomelo_delivery_fragment_t * fragment) {
    assert(fragment != NULL);

    if (fragment->content.buffer) {
        pomelo_buffer_unref(fragment->content.buffer);
        fragment->content.buffer = NULL;
    }
}


int pomelo_delivery_fragment_meta_decode(
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_view_t * view
) {
    assert(meta != NULL);
    assert(view != NULL);

    if (view->length < POMELO_DELIVERY_FRAGMENT_META_MIN_SIZE) {
        return -1; // Not enough space for meta
    }

    pomelo_payload_t payload;
    payload.data = view->buffer->data + view->offset;
    payload.position = 0;
    payload.capacity = view->length;

    uint8_t meta_byte = 0;
    pomelo_payload_read_uint8_unsafe(&payload, &meta_byte);

    size_t sequence_bytes = (meta_byte & 0x07) + 1;
    size_t last_index_bytes = ((meta_byte >> 3) & 0x01) + 1;
    size_t fragment_index_bytes = ((meta_byte >> 4) & 0x01) + 1;
    size_t bus_id_bytes = ((meta_byte >> 5) & 0x01) + 1;
    size_t fragment_type = meta_byte >> 6;

    size_t meta_length = 1 +
        last_index_bytes +
        fragment_index_bytes +
        bus_id_bytes +
        sequence_bytes;

    if (view->length < meta_length) return -1; // Not enough space for meta

    // Temporary value
    uint64_t value = 0;
    
    // Read bus id
    pomelo_payload_read_packed_uint64_unsafe(&payload, bus_id_bytes, &value);
    meta->bus_id = (size_t) value;

    // Read fragment index
    pomelo_payload_read_packed_uint64_unsafe(
        &payload,
        fragment_index_bytes,
        &value
    );
    meta->fragment_index = (size_t) value;

    // Read last index
    pomelo_payload_read_packed_uint64_unsafe(
        &payload,
        last_index_bytes,
        &value
    );
    // total_fragments = last_index + 1
    meta->last_index = (size_t) value;

    // Read sequence
    pomelo_payload_read_packed_uint64_unsafe(&payload, sequence_bytes, &value);
    meta->sequence = value;

    // The type
    meta->type = (pomelo_delivery_fragment_type) fragment_type;

    // Update the view length and offset
    view->length -= meta_length;
    view->offset += meta_length;
    return 0;
}


int pomelo_delivery_fragment_meta_encode(
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_view_t * view
) {
    assert(meta != NULL);
    assert(view != NULL);
    assert(meta->last_index >= 0);

    size_t bus_id_bytes = 
        pomelo_payload_calc_packed_uint64_bytes(meta->bus_id);
    size_t fragment_index_bytes =
        pomelo_payload_calc_packed_uint64_bytes(meta->fragment_index);
    size_t last_index_bytes =
        pomelo_payload_calc_packed_uint64_bytes(meta->last_index);
    size_t sequence_bytes =
        pomelo_payload_calc_packed_uint64_bytes(meta->sequence);

    assert(bus_id_bytes <= 2);
    assert(fragment_index_bytes <= 2);
    assert(last_index_bytes <= 2);

    // Set meta length
    size_t meta_length = 1 +
        last_index_bytes +
        fragment_index_bytes +
        bus_id_bytes +
        sequence_bytes;

    // Check if there is enough space for meta
    size_t remain = view->buffer->capacity - (view->offset + view->length);
    if (remain < meta_length) return -1; // Not enough space for meta

    // Write at the end of the view
    pomelo_payload_t payload;
    payload.data = view->buffer->data + view->offset;
    payload.position = view->length;
    payload.capacity = view->buffer->capacity - view->offset;

    size_t meta_byte =
        (meta->type << 6)                  |
        ((bus_id_bytes - 1) << 5)          |
        ((fragment_index_bytes - 1) << 4)  |
        ((last_index_bytes - 1) << 3)      |
        (sequence_bytes - 1);

    // Encode the meta of fragment
    pomelo_payload_write_uint8_unsafe(&payload, (uint8_t) meta_byte);

    // Write bus index
    pomelo_payload_write_packed_uint64_unsafe(
        &payload,
        bus_id_bytes,
        meta->bus_id
    );

    // Write fragment index
    pomelo_payload_write_packed_uint64_unsafe(
        &payload,
        fragment_index_bytes,
        meta->fragment_index
    );

    // Last index
    pomelo_payload_write_packed_uint64_unsafe(
        &payload,
        last_index_bytes,
        meta->last_index
    );

    // Sequence
    pomelo_payload_write_packed_uint64_unsafe(
        &payload,
        sequence_bytes,
        meta->sequence
    );

    view->length += meta_length;
    return 0;
}


void pomelo_delivery_fragment_set_context(
    pomelo_delivery_fragment_t * fragment,
    pomelo_delivery_context_t * context
) {
    assert(fragment != NULL);
    assert(context != NULL);

    if (fragment->content.buffer) {
        pomelo_buffer_set_context(
            fragment->content.buffer,
            context->buffer_context
        );
    }
}


void pomelo_delivery_fragment_attach_content(
    pomelo_delivery_fragment_t * fragment,
    pomelo_buffer_view_t * view_content
) {
    assert(fragment != NULL);
    assert(view_content != NULL);
    assert(fragment->content.buffer == NULL);

    // Set the view content
    fragment->content = *view_content;
    pomelo_buffer_ref(view_content->buffer);
}


void pomelo_delivery_fragment_attach_buffer(
    pomelo_delivery_fragment_t * fragment,
    pomelo_buffer_t * buffer
) {
    assert(fragment != NULL);
    assert(buffer != NULL);
    assert(fragment->content.buffer == NULL);

    // Attach the buffer to content view
    pomelo_buffer_view_t * view = &fragment->content;
    view->buffer = buffer;
    view->offset = 0;
    view->length = 0;
    pomelo_buffer_ref(buffer);
}
