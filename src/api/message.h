#ifndef POMELO_API_MESSAGE_SRC_H
#define POMELO_API_MESSAGE_SRC_H
#include "pomelo/api.h"
#include "base/ref.h"
#include "delivery/delivery.h"
#include "base/extra.h"

#ifdef __cplusplus
extern "C" {
#endif


#define POMELO_MESSAGE_FLAG_BUSY         (1 << 0)


/// @brief The info of message
typedef struct pomelo_message_info_s pomelo_message_info_t;


typedef enum pomelo_message_mode {
    /// @brief The message is not set
    POMELO_MESSAGE_MODE_UNSET,

    /// @brief The message is reading
    POMELO_MESSAGE_MODE_READ,

    /// @brief The message is writing
    POMELO_MESSAGE_MODE_WRITE
} pomelo_message_mode;


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
    pomelo_delivery_writer_t writer;

    /// @brief The message reader
    pomelo_delivery_reader_t reader;

    /// @brief The mode of message
    pomelo_message_mode mode;

    /// @brief The data for send callback
    void * send_callback_data;

    /// @brief The number of sending sessions
    size_t nsent;

    /// @brief The flags of message
    uint32_t flags;
};


struct pomelo_message_info_s {
    /// @brief The delivery parcel
    pomelo_delivery_parcel_t * parcel;

    /// @brief The mode of message
    pomelo_message_mode mode;

    /// @brief The context of message
    pomelo_context_t * context;
};


/// @brief Initialize the message
int pomelo_message_init(
    pomelo_message_t * message,
    pomelo_message_info_t * info
);


/// @brief Cleanup the message
void pomelo_message_cleanup(pomelo_message_t * message);


/// @brief Make the writing message ready to be read
void pomelo_message_pack(pomelo_message_t * message);


/// @brief Make the reading message ready to be written
void pomelo_message_unpack(pomelo_message_t * message);


/// @brief Finalize callback for message reference
void pomelo_message_on_finalize(pomelo_message_t * message);


/// @brief Prepare the message for sending. This will lock the message, so it
/// cannot be written or read until the sending process is finished.
void pomelo_message_prepare_send(pomelo_message_t * message, void * data);


/// @brief Finish the sending process. This will unlock the message, so it
/// can be written or read again.
void pomelo_message_finish_send(pomelo_message_t * message);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_MESSAGE_SRC_H
