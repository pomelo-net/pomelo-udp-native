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


pomelo_message_t * pomelo_message_new(pomelo_context_t * context) {
    assert(context != NULL);
    pomelo_message_t * message = context->acquire_message(context);
    if (!message) {
        // Cannot acquire new message
        return NULL;
    }

    // After acquiring the message, the ref counter will be set to 1

    // Acquire new delivery parcel
    pomelo_delivery_parcel_t * parcel =
        context->delivery_context->acquire_parcel(context->delivery_context);
    if (!parcel) {
        // Cannot acquire new delivery parcel
        context->release_message(context, message);
        return NULL;
    }

    // Attach the delivery parcel and set the wriable mode
    message->parcel = parcel;
    message->writer = pomelo_delivery_parcel_get_writer(parcel);
    if (!message->writer) {
        // Failed to get writer
        pomelo_message_unref(message);
        return NULL;
    }

    pomelo_extra_set(message->extra, NULL);
    return message;
}


int pomelo_message_ref(pomelo_message_t * message) {
    assert(message != NULL);
    bool result = pomelo_reference_ref(&message->ref);
    assert(result == true);
    return result ? POMELO_ERR_OK : POMELO_ERR_FAILURE;
}


int pomelo_message_unref(pomelo_message_t * message) {
    assert(message != NULL);
    pomelo_reference_unref(&message->ref);
    return 0;
}


void pomelo_message_set_context(
    pomelo_message_t * message,
    pomelo_context_t * context
) {
    assert(message != NULL);
    assert(context != NULL);

    if (context == message->context) {
        // The same context, nothing to do
        return;
    }

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

    // Release current delivery parcel
    if (message->parcel) {
        pomelo_delivery_parcel_unref(message->parcel);
        message->parcel = NULL;
    }
    message->writer = NULL;
    message->reader = NULL;

    // Acquire new delivery parcel
    pomelo_delivery_context_t * delivery_context =
        message->context->delivery_context;
    pomelo_delivery_parcel_t * parcel =
        delivery_context->acquire_parcel(delivery_context);
    if (!parcel) {
        // Cannot acquire new delivery parcel
        return POMELO_ERR_MESSAGE_RESET;
    }

    // Attach the delivery parcel and set the wriable mode
    message->parcel = parcel;
    message->writer = pomelo_delivery_parcel_get_writer(parcel);
    if (!message->writer) {
        return POMELO_ERR_MESSAGE_RESET;
    }

    return 0;
}


size_t pomelo_message_size(pomelo_message_t * message) {
    assert(message != NULL);
    pomelo_message_check_alive(message);

    if (message->reader) {
        // Reading message
        return pomelo_relivery_parcel_reader_remain_bytes(message->reader);
    } else if (message->writer) {
        // Writing message
        return pomelo_delivery_parcel_writer_written_bytes(message->writer);
    } else {
        // Invalid case
        return 0;
    }
}


pomelo_message_t * pomelo_message_wrap(
    pomelo_context_t * context,
    pomelo_delivery_parcel_t * parcel
) {
    assert(context != NULL);
    assert(parcel != NULL);

    pomelo_message_t * message = context->acquire_message(context);
    if (!message) {
        // Failed to allocate message
        return NULL;
    }

    // After acquiring the message, the ref counter will be set to 1

    // Attach delivery parcel & its reader
    message->parcel = parcel;
    message->reader = pomelo_delivery_parcel_get_reader(parcel);
    if (!message->reader) {
        // Failed to get reader
        pomelo_message_unref(message);
        return NULL;
    }

    // Ref the delivery parcel
    pomelo_delivery_parcel_ref(parcel);

    // Change the delivery parcel context
    pomelo_delivery_parcel_set_context(
        parcel,
        context->delivery_context
    );

    pomelo_extra_set(message->extra, NULL);
    return message;
}


int pomelo_message_pack(pomelo_message_t * message) {
    assert(message != NULL);
    pomelo_delivery_parcel_t * parcel = message->parcel;
    if (!parcel) {
        return POMELO_ERR_MESSAGE_PACK;
    }

    // Pack the parcel
    pomelo_delivery_parcel_pack(parcel);
    message->writer = NULL;
    message->reader = pomelo_delivery_parcel_get_reader(parcel);

    return 0;
}


void pomelo_message_reference_finalize(pomelo_reference_t * reference) {
    assert(reference != NULL);
    pomelo_message_t * message = (pomelo_message_t *) reference;
    if (message->parcel) {
        pomelo_delivery_parcel_unref(message->parcel);
        message->parcel = NULL;
    }

    // Clear the reader & writer
    message->reader = NULL;
    message->writer = NULL;

    // Call the callback first
    pomelo_message_on_released(message);

    // Release the message
    pomelo_context_t * context = message->context;
    context->release_message(context, message);
}


pomelo_message_t * pomelo_message_clone(pomelo_message_t * message) {
    assert(message != NULL);

    if (!message->writer || !message->parcel) {
        // This message is not writable, return NULL
        return NULL;
    }

    // First, acquire a message
    pomelo_context_t * context = message->context;
    pomelo_message_t * cloned_message = context->acquire_message(context);

    if (!cloned_message) {
        // Failed to acquire new message
        return NULL;
    }

    // Everything will be cloned but extra data
    cloned_message->parcel = pomelo_delivery_parcel_clone(message->parcel);

    // Create writer
    cloned_message->writer =
        pomelo_delivery_parcel_get_writer(cloned_message->parcel);

    if (!cloned_message->writer) {
        pomelo_message_unref(cloned_message);
        return NULL;
    }

    return cloned_message;
}


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


int pomelo_message_write_buffer(
    pomelo_message_t * message,
    size_t length,
    const uint8_t * buffer
) {
    assert(message != NULL);
    assert(buffer != NULL);
    pomelo_delivery_parcel_writer_t * writer = message->writer;

    if (!writer) {
        // This message is read-only
        return POMELO_ERR_MESSAGE_WRITE;
    }

    size_t bytes = pomelo_delivery_parcel_writer_written_bytes(writer);
    if (bytes + length > message->context->message_capacity) {
        // Out of capacity
        return POMELO_ERR_MESSAGE_OVERFLOW;
    }

    return pomelo_delivery_parcel_writer_write_buffer(writer, length, buffer);
}


int pomelo_message_write_uint8(
    pomelo_message_t * message,
    uint8_t value
) {
    return pomelo_message_write_buffer(message, 1, &value);
}


int pomelo_message_write_uint16(
    pomelo_message_t * message,
    uint16_t value
) {
    uint8_t buffer[2];

    buffer[0] = (uint8_t) value;
    buffer[1] = (uint8_t) (value >> 8);

    return pomelo_message_write_buffer(message, 2, buffer);
}


int pomelo_message_write_uint32(
    pomelo_message_t * message,
    uint32_t value
) {
    uint8_t buffer[4];

    buffer[0] = (uint8_t) value;
    buffer[1] = (uint8_t) (value >> 8);
    buffer[2] = (uint8_t) (value >> 16);
    buffer[3] = (uint8_t) (value >> 24);

    return pomelo_message_write_buffer(message, 4, buffer);
}


int pomelo_message_write_uint64(
    pomelo_message_t * message,
    uint64_t value
) {
    uint8_t buffer[8];

    buffer[0] = (uint8_t) value;
    buffer[1] = (uint8_t) (value >> 8);
    buffer[2] = (uint8_t) (value >> 16);
    buffer[3] = (uint8_t) (value >> 24);
    buffer[4] = (uint8_t) (value >> 32);
    buffer[5] = (uint8_t) (value >> 40);
    buffer[6] = (uint8_t) (value >> 48);
    buffer[7] = (uint8_t) (value >> 56);

    return pomelo_message_write_buffer(message, 8, buffer);
}


int pomelo_message_write_float32(
    pomelo_message_t * message,
    float value
) {
    return pomelo_message_write_uint32(message, *((uint32_t *) &value));
}


int pomelo_message_write_float64(
    pomelo_message_t * message,
    double value
) {
    return pomelo_message_write_uint64(message, *((uint64_t *) &value));
}


int pomelo_message_write_int8(
    pomelo_message_t * message,
    int8_t value
) {
    return pomelo_message_write_uint8(message, value);
}


int pomelo_message_write_int16(
    pomelo_message_t * message,
    int16_t value
) {
    return pomelo_message_write_uint16(message, value);
}


int pomelo_message_write_int32(
    pomelo_message_t * message,
    int32_t value
) {
    return pomelo_message_write_uint32(message, value);
}


int pomelo_message_write_int64(
    pomelo_message_t * message,
    int64_t value
) {
    return pomelo_message_write_uint64(message, value);
}


int pomelo_message_read_buffer(
    pomelo_message_t * message,
    size_t length,
    uint8_t * buffer
) {
    assert(message != NULL);
    assert(buffer != NULL);

    if (!message->reader) {
        // This message is write-only
        return POMELO_ERR_MESSAGE_READ;
    }

    pomelo_delivery_parcel_reader_t * reader = message->reader;
    int ret = pomelo_delivery_parcel_reader_read_buffer(reader, length, buffer);
    if (ret < 0) {
        return POMELO_ERR_MESSAGE_UNDERFLOW;
    }

    return 0;
}


int pomelo_message_read_uint8(
    pomelo_message_t * message,
    uint8_t * value
) {
    return pomelo_message_read_buffer(message, 1, value);
}


int pomelo_message_read_uint16(
    pomelo_message_t * message,
    uint16_t * value
) {
    uint8_t buffer[2];
    int ret = pomelo_message_read_buffer(message, 2, buffer);
    if (ret < 0) {
        return ret;
    }

    *value = buffer[0] | buffer[1] << 8;
    return 0;
}


int pomelo_message_read_uint32(
    pomelo_message_t * message,
    uint32_t * value
) {
    uint8_t buffer[4];
    int ret = pomelo_message_read_buffer(message, 4, buffer);
    if (ret < 0) {
        return ret;
    }

    *value = (
        buffer[0] |
        buffer[1] << 8 |
        buffer[2] << 16 |
        buffer[3] << 24
    );

    return 0;
}


int pomelo_message_read_uint64(
    pomelo_message_t * message,
    uint64_t * value
) {
    uint8_t buffer[8];
    int ret = pomelo_message_read_buffer(message, 8, buffer);
    if (ret < 0) {
        return ret;
    }

    *value = (
        (uint64_t) buffer[0] |
        (uint64_t) buffer[1] << 8 |
        (uint64_t) buffer[2] << 16 |
        (uint64_t) buffer[3] << 24 |
        (uint64_t) buffer[4] << 32 |
        (uint64_t) buffer[5] << 40 |
        (uint64_t) buffer[6] << 48 |
        (uint64_t) buffer[7] << 56
    );

    return 0;
}


int pomelo_message_read_float32(
    pomelo_message_t * message,
    float * value
) {
    return pomelo_message_read_uint32(message, (uint32_t *) value);
}


int pomelo_message_read_float64(
    pomelo_message_t * message,
    double * value
) {
    return pomelo_message_read_uint64(message, (uint64_t *) value);
}


int pomelo_message_read_int8(
    pomelo_message_t * message,
    int8_t * value
) {
    return pomelo_message_read_uint8(message, (uint8_t *) value);
}


int pomelo_message_read_int16(
    pomelo_message_t * message,
    int16_t * value
) {
    return pomelo_message_read_uint16(message, (uint16_t *) value);
}


int pomelo_message_read_int32(
    pomelo_message_t * message,
    int32_t * value
) {
    return pomelo_message_read_uint32(message, (uint32_t *) value);
}


int pomelo_message_read_int64(
    pomelo_message_t * message,
    int64_t * value
) {
    return pomelo_message_read_uint64(message, (uint64_t *) value);
}
