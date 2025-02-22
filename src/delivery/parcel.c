#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "codec/checksum.h"
#include "parcel.h"
#include "transporter.h"
#include "context.h"


/// The number of fragment per bucket in fragments list
#define POMELO_FRAGMENTS_PER_BUCKET 16

/* Check reference of parcel, only available in debug mode */
#ifndef NDEBUG
#define pomelo_delivery_parcel_check_alive(parcel) \
    assert(pomelo_reference_ref_count(&(parcel)->ref) > 0)
#else
#define pomelo_delivery_parcel_check_alive(parcel) (void) (parcel)
#endif


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Seek to the next fragment
pomelo_payload_t * pomelo_delivery_parcel_reader_next_payload(
    pomelo_delivery_parcel_reader_t * reader
) {
    assert(reader != NULL);

    pomelo_delivery_fragment_t * fragment = NULL;
    pomelo_unrolled_list_iterator_next(&reader->it, &fragment);

    if (!fragment) {
        // No more data
        reader->payload = NULL;
        return NULL;
    } else {
        // Get the next payload for reading
        reader->payload = &fragment->payload;
        return reader->payload;
    }
}


int pomelo_delivery_parcel_slice_checksum(
    pomelo_delivery_parcel_t * parcel,
    uint8_t * checksum
) {
    assert(parcel != NULL);

    pomelo_unrolled_list_t * fragments = parcel->fragments;

    if (fragments->size < 2) {
        // Only do checksum for parcels having at least 2 fragments
        return 0;
    }

    pomelo_delivery_parcel_reader_t * reader = &parcel->reader;
    pomelo_unrolled_list_iterator_t * it = &reader->it;
    pomelo_delivery_fragment_t * fragment;

    pomelo_unrolled_list_end(fragments, it);
    pomelo_unrolled_list_iterator_prev(it, &fragment);

    size_t remain_checksum_bytes = POMELO_CODEC_CHECKSUM_BYTES;
    while (remain_checksum_bytes > fragment->payload.capacity) {
        remain_checksum_bytes -= fragment->payload.capacity;

        if (!pomelo_unrolled_list_iterator_prev(it, &fragment)) {
            return -1;
        }
    }

    pomelo_payload_t * payload = &fragment->payload;

    // Start from here
    payload->position = payload->capacity - remain_checksum_bytes;
    size_t last_payload_capacity = payload->position;

    // Prepare a reader for reading the checksum data
    reader->parcel = parcel;
    reader->payload = payload;

    int ret = pomelo_delivery_parcel_reader_read_buffer(
        reader,
        POMELO_CODEC_CHECKSUM_BYTES,
        checksum
    );
    if (ret < 0) return ret;

    // Remove the fragments which contain only checksum data
    // Update the capacity of the payload which is the last one having data
    payload->capacity = last_payload_capacity;
    
    pomelo_delivery_fragment_t * last_fragment;
    ret = pomelo_unrolled_list_get_back(fragments, &last_fragment);
    if (ret < 0) return ret;

    pomelo_delivery_context_t * context = parcel->context;
    while (last_fragment != fragment) {
        // Release the fragment
        ret = pomelo_unrolled_list_pop_back(fragments, &last_fragment);
        if (ret < 0) return ret;

        // Release the fragment
        context->release_fragment(context, last_fragment);
        ret = pomelo_unrolled_list_get_back(fragments, &last_fragment);
        if (ret < 0) return ret;
    }

    return 0;
}


int pomelo_delivery_parcel_alloc(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_root_t * context
) {
    assert(parcel != NULL);
    assert(context != NULL);

    pomelo_unrolled_list_options_t options;
    options.allocator = context->allocator;
    options.bucket_elements = POMELO_FRAGMENTS_PER_BUCKET;
    options.element_size = sizeof(pomelo_delivery_fragment_t *);

    parcel->fragments = pomelo_unrolled_list_create(&options);
    if (!parcel->fragments) {
        return -1;
    }

    return 0;
}


int pomelo_delivery_parcel_dealloc(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_root_t * context
) {
    assert(parcel != NULL);
    (void) context;

    pomelo_unrolled_list_destroy(parcel->fragments);
    parcel->fragments = NULL;

    return 0;
}


int pomelo_delivery_parcel_init(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_root_t * context
) {
    assert(parcel != NULL);
    (void) context;

    // Initialize reference
    pomelo_reference_init(
        &parcel->ref,
        pomelo_delivery_parcel_reference_finalize
    );
    return 0;
}


int pomelo_delivery_parcel_finalize(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_root_t * context
) {
    assert(parcel != NULL);
    (void) context;

    pomelo_unrolled_list_clear(parcel->fragments);
    return 0;
}


void pomelo_delivery_parcel_set_context(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_t * context
) {
    assert(parcel != NULL);
    assert(context != NULL);

    if (parcel->context == context) {
        // Nothing to do
        return;
    }

    // Change the context of parcel
    parcel->context = context;

    // Change the context of buffers
    pomelo_buffer_context_t * buffer_context =
        context->buffer_context;

    pomelo_delivery_fragment_t * fragment;
    pomelo_unrolled_list_iterator_t it;

    pomelo_unrolled_list_begin(parcel->fragments, &it);
    while (pomelo_unrolled_list_iterator_next(&it, &fragment)) {
        // Change the buffer context
        pomelo_buffer_set_context(fragment->buffer, buffer_context);
    }
}


void pomelo_delivery_parcel_reference_finalize(pomelo_reference_t * reference) {
    assert(reference != NULL);
    pomelo_delivery_parcel_t * parcel = (pomelo_delivery_parcel_t *) reference;
    parcel->context->release_parcel(parcel->context, parcel);
}

/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


int pomelo_delivery_parcel_ref(pomelo_delivery_parcel_t * parcel) {
    assert(parcel != NULL);
    pomelo_delivery_parcel_check_alive(parcel);
    bool result = pomelo_reference_ref(&parcel->ref);
    return result ? 0 : -1;
}


int pomelo_delivery_parcel_unref(pomelo_delivery_parcel_t * parcel) {
    assert(parcel != NULL);
    pomelo_delivery_parcel_check_alive(parcel);
    pomelo_reference_unref(&parcel->ref);
    return 0;
}


pomelo_delivery_parcel_reader_t * pomelo_delivery_parcel_get_reader(
    pomelo_delivery_parcel_t * parcel
) {
    assert(parcel != NULL);
    pomelo_delivery_parcel_reader_t * reader = &parcel->reader;

    reader->parcel = parcel;

    pomelo_unrolled_list_begin(parcel->fragments, &reader->it);
    pomelo_delivery_parcel_reader_next_payload(reader);

    // Update the remain bytes of parcel
    size_t remain_bytes = 0;
    pomelo_delivery_fragment_t * fragment;
    pomelo_unrolled_list_iterator_t it;
    pomelo_payload_t * payload;

    pomelo_unrolled_list_begin(parcel->fragments, &it);
    while (pomelo_unrolled_list_iterator_next(&it, &fragment)) {
        payload = &fragment->payload;
        remain_bytes += (payload->capacity - payload->position);
    }

    reader->remain_bytes = remain_bytes;

    return reader;
}


pomelo_delivery_parcel_writer_t * pomelo_delivery_parcel_get_writer(
    pomelo_delivery_parcel_t * parcel
) {
    assert(parcel != NULL);
    int ret;
    pomelo_delivery_parcel_writer_t * writer = &parcel->writer;

    writer->parcel = parcel;
    if (parcel->fragments->size == 0) {
        // The parcel is empty
        // Allocate the first fragment for writing

        pomelo_delivery_context_t * context = parcel->context;

        // New buffer will be acquired
        pomelo_delivery_fragment_t * fragment =
            context->acquire_fragment(context, NULL);
        if (!fragment) {
            return NULL;
        }

        ret = pomelo_unrolled_list_push_back(parcel->fragments, fragment);
        if (ret != 0) {
            return NULL;
        }
    }

    pomelo_delivery_fragment_t * fragment = NULL;

    // Try to get the last fragment
    ret = pomelo_unrolled_list_get_back(parcel->fragments, &fragment);
    if (ret != 0) {
        return NULL;
    }

    writer->payload = &fragment->payload;

    // Update the written bytes of parcel
    size_t written_bytes = 0;
    pomelo_unrolled_list_iterator_t it;
    pomelo_payload_t * payload;

    pomelo_unrolled_list_begin(parcel->fragments, &it);
    while (pomelo_unrolled_list_iterator_next(&it, &fragment)) {
        payload = &fragment->payload;
        written_bytes += payload->position;
    }

    writer->written_bytes = written_bytes;

    return writer;
}


pomelo_delivery_parcel_t * pomelo_delivery_parcel_clone(
    pomelo_delivery_parcel_t * parcel
) {
    assert(parcel != NULL);

    pomelo_delivery_context_t * context = parcel->context;
    pomelo_delivery_parcel_t * cloned_parcel = context->acquire_parcel(context);

    if (!cloned_parcel) {
        // Failed to acquire new parcel
        return NULL;
    }

    // Clone all fragments here
    pomelo_unrolled_list_iterator_t it;
    pomelo_delivery_fragment_t * fragment;

    pomelo_unrolled_list_begin(parcel->fragments, &it);
    while (pomelo_unrolled_list_iterator_next(&it, &fragment)) {
        pomelo_delivery_fragment_t * cloned_fragment = 
            pomelo_delivery_context_clone_writing_fragment(context, fragment);
        
        if (!cloned_fragment) {
            // Failed to clone the fragments
            pomelo_delivery_parcel_unref(cloned_parcel);
            return NULL;
        }

        int ret = pomelo_unrolled_list_push_back(
            cloned_parcel->fragments,
            cloned_fragment
        );
        if (ret < 0) {
            context->release_fragment(context, cloned_fragment);
            pomelo_delivery_parcel_unref(cloned_parcel);
            return NULL;
        }
    }
    
    return cloned_parcel;
}


int pomelo_delivery_parcel_writer_write_buffer(
    pomelo_delivery_parcel_writer_t * writer,
    size_t length,
    const uint8_t * buffer
) {
    assert(writer != NULL);
    assert(buffer != NULL);

    if (length == 0) {
        return 0; // Nothing to do
    }

    pomelo_delivery_context_t * context = writer->parcel->context;
    pomelo_payload_t * payload = writer->payload;

    const uint8_t * last = buffer + length; // The last of buffer
    size_t remain_bytes = length;
    size_t writing_bytes, remain_fragment_bytes;
    size_t written_bytes = 0;

    pomelo_delivery_fragment_t * fragment;
    bool ok = true;

    while (remain_bytes > 0) {
        if (!payload) {
            ok = false;
            break;
        }

        remain_fragment_bytes = payload->capacity - payload->position;
        writing_bytes = POMELO_MIN(remain_fragment_bytes, remain_bytes);

        if (writing_bytes > 0) {
            memcpy(
                payload->data + payload->position,
                last - remain_bytes, 
                writing_bytes
            );

            written_bytes += writing_bytes;
        }

        payload->position += writing_bytes;
        remain_bytes -= writing_bytes;

        if (payload->capacity == payload->position) {
            size_t fragment_index = writer->parcel->fragments->size;
            if (fragment_index >= context->max_fragments) {
                // Maximum fragments
                ok = false;
                break;
            }

            // Need more fragment
            fragment = context->acquire_fragment(context, NULL);
            if (!fragment) {
                // Cannot allocate more fragment
                ok = false;
                break;
            }
            fragment->index = fragment_index;
            int ret = pomelo_unrolled_list_push_back(
                writer->parcel->fragments,
                fragment
            );
            if (ret < 0) {
                ok = false;
                break;
            }

            payload = &fragment->payload;
            writer->payload = payload;
        }
    }

    // Update written bytes
    writer->written_bytes += written_bytes;
    return ok ? 0 : -1;
}


size_t pomelo_delivery_parcel_writer_written_bytes(
    pomelo_delivery_parcel_writer_t * writer
) {
    assert(writer != NULL);
    return writer->written_bytes;
}


int pomelo_delivery_parcel_reader_read_buffer(
    pomelo_delivery_parcel_reader_t * reader,
    size_t length,
    uint8_t * buffer
) {
    assert(reader != NULL);
    assert(buffer != NULL);

    if (length == 0) {
        return 0; // Nothing to do
    }

    pomelo_payload_t * payload = reader->payload;

    // The last address of buffer
    uint8_t * last = buffer + length;

    // Total remain bytes to read
    size_t remain_bytes = length;
    size_t reading_bytes;

    while (remain_bytes > 0) {
        if (!payload) {
            // Not enough space for reader
            return -1;
        }

        reading_bytes = POMELO_MIN(
            payload->capacity - payload->position, // Remain bytes of payload
            remain_bytes
        );

        if (reading_bytes > 0) {
            memcpy(
                last - remain_bytes,
                payload->data + payload->position,
                reading_bytes
            );
        }

        remain_bytes -= reading_bytes;
        payload->position += reading_bytes;

        if (payload->position == payload->capacity) {
            // The current payload is out of capacity
            payload = pomelo_delivery_parcel_reader_next_payload(reader);
        }
    }

    // Update remain bytes
    reader->remain_bytes -= length;

    return 0;
}


size_t pomelo_relivery_parcel_reader_remain_bytes(
    pomelo_delivery_parcel_reader_t * reader
) {
    assert(reader != NULL);
    return reader->remain_bytes;
}


void pomelo_delivery_parcel_pack(pomelo_delivery_parcel_t * parcel) {
    assert(parcel != NULL);

    pomelo_delivery_fragment_t * fragment;
    pomelo_unrolled_list_iterator_t it;
    pomelo_payload_t * payload;

    pomelo_unrolled_list_begin(parcel->fragments, &it);
    while (pomelo_unrolled_list_iterator_next(&it, &fragment)) {
        payload = &fragment->payload;
        // Pack the payload
        pomelo_payload_pack(payload);
    }
}
