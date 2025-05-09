#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "crypto/checksum.h"
#include "parcel.h"
#include "context.h"


/// @brief The initial capacity of chunks
#define POMELO_DELIVERY_PARCEL_CHUNKS_INIT_CAPACITY 16


int pomelo_delivery_parcel_on_alloc(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_t * context
) {
    assert(parcel != NULL);
    assert(context != NULL);

    // Initialize the fragments list
    pomelo_array_options_t options = {
        .allocator = context->allocator,
        .element_size = sizeof(pomelo_buffer_view_t),
        .initial_capacity = POMELO_DELIVERY_PARCEL_CHUNKS_INIT_CAPACITY
    };
    parcel->chunks = pomelo_array_create(&options);
    if (!parcel->chunks) return -1;

    return 0;
}


void pomelo_delivery_parcel_on_free(pomelo_delivery_parcel_t * parcel) {
    assert(parcel != NULL);

    if (parcel->chunks) {
        pomelo_array_destroy(parcel->chunks);
        parcel->chunks = NULL;
    }
}


/// @brief The finalize callback for parcel
static void parcel_ref_finalize(pomelo_delivery_parcel_t * parcel) {
    assert(parcel != NULL);
    pomelo_delivery_context_t * context = parcel->context;
    context->release_parcel(context, parcel);
}


int pomelo_delivery_parcel_init(pomelo_delivery_parcel_t * parcel) {
    assert(parcel != NULL);
    pomelo_reference_init(
        &parcel->ref,
        (pomelo_ref_finalize_cb) parcel_ref_finalize
    );
    return 0;
}


void pomelo_delivery_parcel_cleanup(pomelo_delivery_parcel_t * parcel) {
    assert(parcel != NULL);
    pomelo_delivery_parcel_clear_all_chunks(parcel);
}


void pomelo_delivery_parcel_set_context(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_t * context
) {
    assert(parcel != NULL);
    assert(context != NULL);

    if (parcel->context == context) return; // Nothing to do

    // Change the context of parcel
    parcel->context = context;
    pomelo_buffer_context_t * buffer_context = context->buffer_context;

    // Change the context of chunks
    pomelo_array_t * chunks = parcel->chunks;
    size_t size = chunks->size;
    for (size_t i = 0; i < size; i++) {
        pomelo_buffer_view_t * chunk = pomelo_array_get_ptr(chunks, i);
        assert(chunk != NULL);
        if (!chunk->buffer) continue; // The chunk has not been arrived
        pomelo_buffer_set_context(chunk->buffer, buffer_context);
    }
}


pomelo_buffer_view_t * pomelo_delivery_parcel_append_chunk(
    pomelo_delivery_parcel_t * parcel
) {
    assert(parcel != NULL);
    pomelo_delivery_context_t * context = parcel->context;
    pomelo_buffer_context_t * buffer_context = context->buffer_context;

    // Acquire new buffer for the fragment
    pomelo_buffer_t * buffer = pomelo_buffer_context_acquire(buffer_context);
    if (!buffer) return NULL; // Failed to acquire new buffer

    pomelo_buffer_view_t * result =
        pomelo_array_append_ptr(parcel->chunks, NULL);
    if (!result) {
        // Failed to append fragment
        pomelo_buffer_unref(buffer);
        return NULL;
    }

    result->buffer = buffer;
    result->length = 0;
    result->offset = 0;

    return result;
}


int pomelo_delivery_parcel_set_fragments(
    pomelo_delivery_parcel_t * parcel,
    pomelo_array_t * fragments
) {
    assert(parcel != NULL);
    assert(fragments != NULL);

    size_t nfragments = fragments->size;
    // Check if the last fragment is empty
    pomelo_delivery_fragment_t * fragment =
        pomelo_array_get_ptr(fragments, nfragments - 1);
    assert(fragment != NULL);
    if (fragment->content.length == 0) {
        nfragments--;
    }

    int ret = pomelo_array_resize(parcel->chunks, nfragments);
    if (ret < 0) return ret;

    for (size_t i = 0; i < nfragments; i++) {
        fragment = pomelo_array_get_ptr(fragments, i);
        assert(fragment != NULL);

        pomelo_buffer_view_t * chunk = pomelo_array_get_ptr(parcel->chunks, i);
        assert(chunk != NULL);
        *chunk = fragment->content;
        pomelo_buffer_ref(chunk->buffer);
    }

    return 0;
}


void pomelo_delivery_parcel_clear_all_chunks(
    pomelo_delivery_parcel_t * parcel
) {
    assert(parcel != NULL);
    pomelo_array_t * chunks = parcel->chunks;
    size_t size = chunks->size;

    // Clear all chunks
    for (size_t i = 0; i < size; i++) {
        pomelo_buffer_view_t * chunk = pomelo_array_get_ptr(chunks, i);
        assert(chunk != NULL);

        if (!chunk->buffer) continue; // The chunk has not been arrived
        pomelo_buffer_unref(chunk->buffer);
        chunk->buffer = NULL;
    }

    // And clear the array
    pomelo_array_clear(chunks);
}


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

void pomelo_delivery_parcel_set_extra(
    pomelo_delivery_parcel_t * parcel,
    void * extra
) {
    assert(parcel != NULL);
    pomelo_extra_set(parcel->extra, extra);
}


void * pomelo_delivery_parcel_get_extra(pomelo_delivery_parcel_t * parcel) {
    assert(parcel != NULL);
    return pomelo_extra_get(parcel->extra);
}


bool pomelo_delivery_parcel_ref(pomelo_delivery_parcel_t * parcel) {
    assert(parcel != NULL);
    return pomelo_reference_ref(&parcel->ref);
}


void pomelo_delivery_parcel_unref(pomelo_delivery_parcel_t * parcel) {
    assert(parcel != NULL);
    pomelo_reference_unref(&parcel->ref);
}


void pomelo_delivery_parcel_reset(pomelo_delivery_parcel_t * parcel) {
    pomelo_delivery_parcel_clear_all_chunks(parcel);
}


void pomelo_delivery_reader_init(
    pomelo_delivery_reader_t * reader,
    pomelo_delivery_parcel_t * parcel
) {
    assert(reader != NULL);
    assert(parcel != NULL);

    reader->parcel = parcel;

    // Initialize the payload
    reader->payload.data = NULL;
    reader->payload.capacity = 0;
    reader->payload.position = 0;
    reader->index = 0;

    // Initialize the iterator
    size_t total_content_bytes = 0;
    pomelo_array_t * chunks = parcel->chunks;
    size_t size = chunks->size;
    for (size_t i = 0; i < size; i++) {
        pomelo_buffer_view_t * chunk = pomelo_array_get_ptr(chunks, i);
        assert(chunk != NULL);

        if (!chunk->buffer) continue; // Skip the empty chunk
        total_content_bytes += chunk->length;
    }

    reader->remain_bytes = total_content_bytes;
}


void pomelo_delivery_writer_init(
    pomelo_delivery_writer_t * writer,
    pomelo_delivery_parcel_t * parcel
) {
    assert(writer != NULL);
    assert(parcel != NULL);

    writer->parcel = parcel;

    // Update the written bytes of parcel
    size_t written_bytes = 0;
    pomelo_array_t * chunks = parcel->chunks;
    size_t size = chunks->size;
    for (size_t i = 0; i < size; i++) {
        pomelo_buffer_view_t * chunk = pomelo_array_get_ptr(chunks, i);
        assert(chunk != NULL);

        if (!chunk->buffer) continue; // Skip the empty chunk
        written_bytes += chunk->length;
    }

    writer->written_bytes = written_bytes;
}


int pomelo_delivery_writer_write(
    pomelo_delivery_writer_t * writer,
    const uint8_t * buffer,
    size_t length
) {
    assert(writer != NULL);
    assert(buffer != NULL);

    if (length == 0) return 0; // Nothing to do

    pomelo_delivery_parcel_t * parcel = writer->parcel;
    pomelo_delivery_context_t * context = parcel->context;
    pomelo_array_t * chunks = parcel->chunks;
    pomelo_buffer_view_t * chunk;
    
    // Get the last fragment
    pomelo_payload_t payload;
    if (chunks->size > 0) {
        chunk = pomelo_array_get_ptr(chunks, chunks->size - 1);
        assert(chunk != NULL);

        payload.data = chunk->buffer->data + chunk->offset;
        payload.capacity = POMELO_MIN(
            chunk->buffer->capacity - chunk->offset,
            context->fragment_content_capacity
        );
        payload.position = chunk->length;
    } else {
        chunk = NULL;
        payload.capacity = 0;
        payload.position = 0;
        payload.data = NULL;
    }

    const uint8_t * last = buffer + length; // The last of buffer
    size_t remain_bytes = length;

    while (remain_bytes > 0) {
        if (payload.capacity == payload.position) {
            if (chunks->size >= context->max_fragments) {
                return -1; // Maximum fragments
            }

            // Need more fragment
            chunk = pomelo_delivery_parcel_append_chunk(parcel);
            if (!chunk) return -1; // Cannot allocate more fragment

            payload.data = chunk->buffer->data + chunk->offset;
            payload.position = chunk->length;
            payload.capacity = POMELO_MIN(
                chunk->buffer->capacity - chunk->offset,
                context->fragment_content_capacity
            );
        }

        size_t remain_fragment_bytes = payload.capacity - payload.position;
        size_t writing_bytes = POMELO_MIN(remain_fragment_bytes, remain_bytes);
        assert(writing_bytes > 0);

        pomelo_payload_write_buffer_unsafe(
            &payload,
            last - remain_bytes,
            writing_bytes
        );

        remain_bytes -= writing_bytes;
        writer->written_bytes += writing_bytes;

        chunk->length += writing_bytes;
    }

    return 0;
}


size_t pomelo_delivery_writer_written_bytes(
    pomelo_delivery_writer_t * writer
) {
    assert(writer != NULL);
    return writer->written_bytes;
}


int pomelo_delivery_reader_read(
    pomelo_delivery_reader_t * reader,
    uint8_t * buffer,
    size_t length
) {
    assert(buffer != NULL);
    assert(reader != NULL);

    if (length == 0) return 0; // Nothing to do
    if (length > reader->remain_bytes) return -1; // Not enough data

    uint8_t * last = buffer + length; // Just for optimization
    pomelo_payload_t * payload = &reader->payload;

    // Total remain bytes to read
    size_t remain_bytes = length;
    while (remain_bytes > 0) {
        if (payload->position == payload->capacity) {
            // The current payload is out of capacity
            pomelo_buffer_view_t * chunk =
                pomelo_array_get_ptr(reader->parcel->chunks, reader->index);
            if (!chunk) return -1; // Not enough data

            payload->data = chunk->buffer->data + chunk->offset;
            payload->capacity = chunk->length;
            payload->position = 0;

            reader->index++;
        }

        size_t reading_bytes = POMELO_MIN(
            pomelo_payload_remain(payload),
            remain_bytes
        );
        assert(reading_bytes > 0);
        pomelo_payload_read_buffer_unsafe(
            payload,
            last - remain_bytes,
            reading_bytes
        );
        remain_bytes -= reading_bytes;
        reader->remain_bytes -= reading_bytes;
    }

    return 0;
}


size_t pomelo_delivery_reader_remain_bytes(
    pomelo_delivery_reader_t * reader
) {
    assert(reader != NULL);
    return reader->remain_bytes;
}
