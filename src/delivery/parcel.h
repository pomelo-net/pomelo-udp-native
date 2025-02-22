#ifndef POMELO_DELIVERY_PARCEL_SRC_H
#define POMELO_DELIVERY_PARCEL_SRC_H
#include "platform/platform.h"
#include "utils/list.h"
#include "base/ref.h"
#include "fragment.h"
#include "delivery.h"


#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_delivery_parcel_reader_s {
    /// @brief The parcel
    pomelo_delivery_parcel_t * parcel;

    /// @brief The current iterator or parcel fragments
    pomelo_unrolled_list_iterator_t it;

    /// @brief The current reading payload. If payload is NULL, the reader has
    /// finished its work.
    pomelo_payload_t * payload;

    /// @brief The number of remain bytes
    size_t remain_bytes;
};


struct pomelo_delivery_parcel_writer_s {
    /// @brief The parcel
    pomelo_delivery_parcel_t * parcel;

    /// @brief The current writing payload
    pomelo_payload_t * payload;

    /// @brief The number of written bytes of this parcel
    size_t written_bytes;
};


struct pomelo_delivery_parcel_s {
    /// @brief Reference
    pomelo_reference_t ref;

    /// @brief The delivery context
    pomelo_delivery_context_t * context;

    /// @brief The list of fragments of this parcel
    pomelo_unrolled_list_t * fragments;

    /// @brief The writer of parcel
    pomelo_delivery_parcel_writer_t writer;

    /// @brief The reader of parcel
    pomelo_delivery_parcel_reader_t reader;
};


/// @brief Seek to the next payload
pomelo_payload_t * pomelo_delivery_parcel_reader_next_payload(
    pomelo_delivery_parcel_reader_t * reader
);


/// @brief Extract and remove the checksum part from parcel.
/// The removed fragments and payloads will be released.
int pomelo_delivery_parcel_slice_checksum(
    pomelo_delivery_parcel_t * parcel,
    uint8_t * checksum
);


/// @brief Allocating callback for parcel
int pomelo_delivery_parcel_alloc(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_root_t * context
);


/// @brief Deallocating callback for parcel
int pomelo_delivery_parcel_dealloc(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_root_t * context
);


/// @brief The acquire callback for delivery parcel pool
int pomelo_delivery_parcel_init(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_root_t * context
);


/// @brief The release callback for delivery parcel pool
int pomelo_delivery_parcel_finalize(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_root_t * context
);


/// @brief Change the context
void pomelo_delivery_parcel_set_context(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_t * context
);


/// @brief Finalize callback for parcel
void pomelo_delivery_parcel_reference_finalize(pomelo_reference_t * reference);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_PARCEL_SRC_H
