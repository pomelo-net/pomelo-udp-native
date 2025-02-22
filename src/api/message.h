#ifndef POMELO_API_MESSAGE_SRC_H
#define POMELO_API_MESSAGE_SRC_H
#include "pomelo/api.h"
#include "base/ref.h"
#include "delivery/delivery.h"
#include "base/extra.h"

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_message_s {
    /// @brief Reference of message
    pomelo_reference_t ref;

    /// @brief The extra data of message
    pomelo_extra_t extra;

    /// @brief The context of message
    pomelo_context_t * context;

    /// @brief The delivery parcel
    pomelo_delivery_parcel_t * parcel;

    /// @brief The message writer
    pomelo_delivery_parcel_writer_t * writer;

    /// @brief The message reader
    pomelo_delivery_parcel_reader_t * reader;
};


/// @brief Wrap a delivery parcel
pomelo_message_t * pomelo_message_wrap(
    pomelo_context_t * context,
    pomelo_delivery_parcel_t * parcel
);


/// @brief Make the writing message ready to be read
int pomelo_message_pack(pomelo_message_t * message);


/// @brief Finalize callback for message reference
void pomelo_message_reference_finalize(pomelo_reference_t * reference);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_MESSAGE_SRC_H
