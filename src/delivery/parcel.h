#ifndef POMELO_DELIVERY_PARCEL_SRC_H
#define POMELO_DELIVERY_PARCEL_SRC_H
#include "platform/platform.h"
#include "base/ref.h"
#include "base/extra.h"
#include "utils/array.h"
#include "fragment.h"
#include "delivery.h"

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_delivery_parcel_s {
    /// @brief Reference
    pomelo_reference_t ref;

    /// @brief The extra data of parcel
    pomelo_extra_t extra;

    /// @brief The delivery context
    pomelo_delivery_context_t * context;

    /// @brief The chunks of this parcel
    pomelo_array_t * chunks;
};


/// @brief Allocating callback for parcel
int pomelo_delivery_parcel_on_alloc(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_t * context
);


/// @brief Freeing callback for parcel
void pomelo_delivery_parcel_on_free(pomelo_delivery_parcel_t * parcel);


/// @brief The acquire callback for delivery parcel pool
int pomelo_delivery_parcel_init(pomelo_delivery_parcel_t * parcel);


/// @brief The release callback for delivery parcel pool
void pomelo_delivery_parcel_cleanup(pomelo_delivery_parcel_t * parcel);


/// @brief Change the context
void pomelo_delivery_parcel_set_context(
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_context_t * context
);


/// @brief Append new fragment to parcel
pomelo_buffer_view_t * pomelo_delivery_parcel_append_chunk(
    pomelo_delivery_parcel_t * parcel
);


/// @brief Set the fragments
int pomelo_delivery_parcel_set_fragments(
    pomelo_delivery_parcel_t * parcel,
    pomelo_array_t * fragments
);


/// @brief Clear all chunks
void pomelo_delivery_parcel_clear_all_chunks(pomelo_delivery_parcel_t * parcel);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_PARCEL_SRC_H
