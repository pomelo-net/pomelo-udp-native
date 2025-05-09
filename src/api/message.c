#include <assert.h>
#include <string.h>
#include "pomelo/errno.h"
#include "message.h"
#include "context.h"
#include "delivery/context.h"
#include "delivery/parcel.h"


/* Check reference of message, only available in debug mode */
#ifndef NDEBUG
#define pomelo_message_check_alive(message) \
    assert(pomelo_reference_ref_count(&((message)->ref)) > 0)
#else
#define pomelo_message_check_alive(message) (void) (message)
#endif


void pomelo_message_set_extra(pomelo_message_t * message, void * data) {
    assert(message != NULL);
    pomelo_extra_set(message->extra, data);
}


void * pomelo_message_get_extra(pomelo_message_t * message) {
    assert(message != NULL);
    return pomelo_extra_get(message->extra);
}


int pomelo_message_ref(pomelo_message_t * message) {
    assert(message != NULL);
    bool result = pomelo_reference_ref(&message->ref);
    assert(result == true);
    return result ? POMELO_ERR_OK : POMELO_ERR_FAILURE;
}


void pomelo_message_unref(pomelo_message_t * message) {
    assert(message != NULL);
    pomelo_reference_unref(&message->ref);
}


void pomelo_message_set_context(
    pomelo_message_t * message,
    pomelo_context_t * context
) {
    assert(message != NULL);
    assert(context != NULL);

    if (context == message->context) return; // The same context, nothing to do
    message->context = context;

    // Change the context of delivery parcel
    pomelo_delivery_parcel_set_context(
        message->parcel,
        context->delivery_context
    );
}


pomelo_context_t * pomelo_message_get_context(pomelo_message_t * message) {
    assert(message != NULL);
    return message->context;
}


int pomelo_message_reset(pomelo_message_t * message) {
    assert(message != NULL);
    pomelo_message_check_alive(message);

    // Reset the parcel
    pomelo_delivery_parcel_reset(message->parcel);
    pomelo_delivery_writer_init(&message->writer, message->parcel);
    message->mode = POMELO_MESSAGE_MODE_WRITE;

    return 0;
}


size_t pomelo_message_size(pomelo_message_t * message) {
    assert(message != NULL);
    pomelo_message_check_alive(message);

    if (message->mode == POMELO_MESSAGE_MODE_READ) {
        return pomelo_delivery_reader_remain_bytes(&message->reader);
    } else if (message->mode == POMELO_MESSAGE_MODE_WRITE) {
        return pomelo_delivery_writer_written_bytes(&message->writer);
    }

    return 0;
}


int pomelo_message_init(
    pomelo_message_t * message,
    pomelo_message_info_t * info
) {
    assert(message != NULL);
    assert(info != NULL);
    assert(info->parcel != NULL);
    assert(info->context != NULL);

    // Initialize message values
    pomelo_extra_set(message->extra, NULL);
    message->context = info->context;
    message->mode = info->mode;

    // Attach the delivery parcel
    pomelo_delivery_parcel_t * parcel = info->parcel;
    message->parcel = parcel;
    pomelo_delivery_parcel_ref(parcel);

    // Set the extra of parcel
    pomelo_delivery_parcel_set_extra(parcel, message);

    // Initialize the writer or reader
    switch (info->mode) {
        case POMELO_MESSAGE_MODE_WRITE:
            pomelo_delivery_writer_init(&message->writer, parcel);
            break;

        case POMELO_MESSAGE_MODE_READ:
            pomelo_delivery_reader_init(&message->reader, parcel);
            break;

        default:
            assert(false);
            return -1;
    }

    // Initialize the reference
    pomelo_reference_init(
        &message->ref,
        (pomelo_ref_finalize_cb) pomelo_message_on_finalize
    );
    return 0;
}


void pomelo_message_cleanup(pomelo_message_t * message) {
    assert(message != NULL);
    message->mode = POMELO_MESSAGE_MODE_UNSET;

    if (message->parcel) {
        pomelo_delivery_parcel_unref(message->parcel);
        message->parcel = NULL;
    }
}


void pomelo_message_pack(pomelo_message_t * message) {
    assert(message != NULL);
    assert(message->parcel != NULL);

    // Pack the parcel
    pomelo_delivery_reader_init(&message->reader, message->parcel);
    message->mode = POMELO_MESSAGE_MODE_READ;
}


void pomelo_message_unpack(pomelo_message_t * message) {
    assert(message != NULL);
    assert(message->parcel != NULL);

    // Unpack the parcel
    pomelo_delivery_writer_init(&message->writer, message->parcel);
    message->mode = POMELO_MESSAGE_MODE_WRITE;
}


void pomelo_message_on_finalize(pomelo_message_t * message) {
    assert(message != NULL);
    pomelo_context_release_message(message->context, message);
}


void pomelo_message_prepare_send(pomelo_message_t * message, void * data) {
    assert(message != NULL);

    message->send_callback_data = data;
    message->flags |= POMELO_MESSAGE_FLAG_BUSY;
    message->nsent = 0;
    pomelo_message_ref(message);
}


void pomelo_message_finish_send(pomelo_message_t * message) {
    assert(message != NULL);
    message->flags &= ~POMELO_MESSAGE_FLAG_BUSY;
    pomelo_message_unref(message);
}


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


int pomelo_message_write_buffer(
    pomelo_message_t * message,
    const uint8_t * buffer,
    size_t length
) {
    assert(message != NULL);
    assert(buffer != NULL);

    if (message->mode != POMELO_MESSAGE_MODE_WRITE) {
        return POMELO_ERR_MESSAGE_WRITE; // This message is read-only
    }

    if (message->flags & POMELO_MESSAGE_FLAG_BUSY) {
        return POMELO_ERR_MESSAGE_BUSY; // This message is busy
    }

    pomelo_delivery_writer_t * writer = &message->writer;
    size_t bytes = pomelo_delivery_writer_written_bytes(writer);
    if (bytes + length > message->context->message_capacity) {
        return POMELO_ERR_MESSAGE_OVERFLOW; // Out of capacity
    }

    return pomelo_delivery_writer_write(writer, buffer, length);
}


int pomelo_message_write_uint8(pomelo_message_t * message, uint8_t value) {
    assert(message != NULL);
    return pomelo_message_write_buffer(message, &value, sizeof(uint8_t));
}


int pomelo_message_write_uint16(pomelo_message_t * message, uint16_t value) {
    assert(message != NULL);
    uint8_t buffer[sizeof(uint16_t)];
    pomelo_payload_t payload;

    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_write_uint16_unsafe(&payload, value);
    return pomelo_message_write_buffer(message, buffer, sizeof(buffer));
}


int pomelo_message_write_uint32(pomelo_message_t * message, uint32_t value) {
    assert(message != NULL);
    uint8_t buffer[sizeof(uint32_t)];
    pomelo_payload_t payload;

    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_write_uint32_unsafe(&payload, value);
    return pomelo_message_write_buffer(message, buffer, sizeof(buffer));
}


int pomelo_message_write_uint64(pomelo_message_t * message, uint64_t value) {
    assert(message != NULL);
    uint8_t buffer[sizeof(uint64_t)];
    pomelo_payload_t payload;

    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_write_uint64_unsafe(&payload, value);
    return pomelo_message_write_buffer(message, buffer, sizeof(buffer));
}


int pomelo_message_write_float32(pomelo_message_t * message, float value) {
    assert(message != NULL);
    uint8_t buffer[sizeof(float)];
    pomelo_payload_t payload;

    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_write_float32_unsafe(&payload, value);
    return pomelo_message_write_buffer(message, buffer, sizeof(buffer));
}


int pomelo_message_write_float64(pomelo_message_t * message, double value) {
    assert(message != NULL);
    uint8_t buffer[sizeof(double)];
    pomelo_payload_t payload;

    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_write_float64_unsafe(&payload, value);
    return pomelo_message_write_buffer(message, buffer, sizeof(buffer));
}


int pomelo_message_write_int8(pomelo_message_t * message, int8_t value) {
    assert(message != NULL);
    uint8_t buffer[sizeof(int8_t)];
    pomelo_payload_t payload;

    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_write_int8_unsafe(&payload, value);
    return pomelo_message_write_buffer(message, buffer, sizeof(buffer));
}


int pomelo_message_write_int16(pomelo_message_t * message, int16_t value) {
    assert(message != NULL);
    uint8_t buffer[sizeof(int16_t)];
    pomelo_payload_t payload;

    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_write_int16_unsafe(&payload, value);
    return pomelo_message_write_buffer(message, buffer, sizeof(buffer));
}


int pomelo_message_write_int32(pomelo_message_t * message, int32_t value) {
    assert(message != NULL);
    uint8_t buffer[sizeof(int32_t)];
    pomelo_payload_t payload;

    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_write_int32_unsafe(&payload, value);
    return pomelo_message_write_buffer(message, buffer, sizeof(buffer));
}


int pomelo_message_write_int64(pomelo_message_t * message, int64_t value) {
    assert(message != NULL);
    uint8_t buffer[sizeof(int64_t)];
    pomelo_payload_t payload;

    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_write_int64_unsafe(&payload, value);
    return pomelo_message_write_buffer(message, buffer, sizeof(buffer));
}


int pomelo_message_read_buffer(
    pomelo_message_t * message,
    uint8_t * buffer,
    size_t length
) {
    assert(message != NULL);
    assert(buffer != NULL);

    if (message->mode != POMELO_MESSAGE_MODE_READ) {
        return POMELO_ERR_MESSAGE_READ; // This message is write-only
    }

    int ret = pomelo_delivery_reader_read(
        &message->reader, buffer, length
    );

    return (ret < 0) ? POMELO_ERR_MESSAGE_UNDERFLOW : 0;
}


int pomelo_message_read_uint8(pomelo_message_t * message, uint8_t * value) {
    assert(message != NULL);
    assert(value != NULL);
    return pomelo_message_read_buffer(message, value, sizeof(uint8_t));
}


int pomelo_message_read_uint16(pomelo_message_t * message, uint16_t * value) {
    assert(message != NULL);
    assert(value != NULL);

    uint8_t buffer[sizeof(uint16_t)];
    int ret = pomelo_message_read_buffer(message, buffer, sizeof(buffer));
    if (ret < 0) return ret;

    pomelo_payload_t payload;
    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_read_uint16_unsafe(&payload, value);
    return 0;
}


int pomelo_message_read_uint32(pomelo_message_t * message, uint32_t * value) {
    assert(message != NULL);
    assert(value != NULL);

    uint8_t buffer[sizeof(uint32_t)];
    int ret = pomelo_message_read_buffer(message, buffer, sizeof(buffer));
    if (ret < 0) return ret;

    pomelo_payload_t payload;
    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_read_uint32_unsafe(&payload, value);
    return 0;
}


int pomelo_message_read_uint64(pomelo_message_t * message, uint64_t * value) {
    assert(message != NULL);
    assert(value != NULL);

    uint8_t buffer[sizeof(uint64_t)];
    int ret = pomelo_message_read_buffer(message, buffer, sizeof(buffer));
    if (ret < 0) return ret;

    pomelo_payload_t payload;
    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_read_uint64_unsafe(&payload, value);
    return 0;
}


int pomelo_message_read_float32(pomelo_message_t * message, float * value) {
    assert(message != NULL);
    assert(value != NULL);
    uint8_t buffer[sizeof(float)];
    int ret = pomelo_message_read_buffer(message, buffer, sizeof(buffer));
    if (ret < 0) return ret;

    pomelo_payload_t payload;
    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_read_float32_unsafe(&payload, value);
    return 0;
}


int pomelo_message_read_float64(pomelo_message_t * message, double * value) {
    assert(message != NULL);
    assert(value != NULL);
    uint8_t buffer[sizeof(double)];
    int ret = pomelo_message_read_buffer(message, buffer, sizeof(buffer));
    if (ret < 0) return ret;

    pomelo_payload_t payload;
    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_read_float64_unsafe(&payload, value);
    return 0;
}


int pomelo_message_read_int8(pomelo_message_t * message, int8_t * value) {
    assert(message != NULL);
    assert(value != NULL);
    uint8_t buffer[sizeof(int8_t)];
    int ret = pomelo_message_read_buffer(message, buffer, sizeof(buffer));
    if (ret < 0) return ret;

    pomelo_payload_t payload;
    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_read_int8_unsafe(&payload, value);
    return 0;
}


int pomelo_message_read_int16(pomelo_message_t * message, int16_t * value) {
    assert(message != NULL);
    assert(value != NULL);
    uint8_t buffer[sizeof(int16_t)];
    int ret = pomelo_message_read_buffer(message, buffer, sizeof(buffer));
    if (ret < 0) return ret;

    pomelo_payload_t payload;
    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_read_int16_unsafe(&payload, value);
    return 0;
}


int pomelo_message_read_int32(pomelo_message_t * message, int32_t * value) {
    assert(message != NULL);
    assert(value != NULL);
    uint8_t buffer[sizeof(int32_t)];
    int ret = pomelo_message_read_buffer(message, buffer, sizeof(buffer));
    if (ret < 0) return ret;

    pomelo_payload_t payload;
    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_read_int32_unsafe(&payload, value);
    return 0;
}


int pomelo_message_read_int64(pomelo_message_t * message, int64_t * value) {    
    assert(message != NULL);
    assert(value != NULL);
    uint8_t buffer[sizeof(int64_t)];
    int ret = pomelo_message_read_buffer(message, buffer, sizeof(buffer));
    if (ret < 0) return ret;

    pomelo_payload_t payload;
    payload.data = buffer;
    payload.position = 0;
    payload.capacity = sizeof(buffer);

    pomelo_payload_read_int64_unsafe(&payload, value);
    return 0;
}
