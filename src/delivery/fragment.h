#ifndef POMELO_DELIVERY_FRAGMENT_SRC_H
#define POMELO_DELIVERY_FRAGMENT_SRC_H
#include <stdbool.h>
#include "base/payload.h"
#include "base/buffer.h"
#include "delivery.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
    Fragment Metadata Layout
    -----------------------
    The metadata is stored at the beginning of the payload in the following
    format:

    Field Layout:
    ----------------------------------------------------------------------------
    Offset | Field Name          | Size          | Value Range
    -------|---------------------|---------------|------------------------------
    0      | meta_byte           | 1 byte        | [0-255]
    1      | bus_id              | 1-2 bytes     | [0-65535]
    +1     | fragment_index      | 1-2 bytes     | [0-65535]
    +1     | last_index          | 1-2 bytes     | [0-65535]
    +1     | sequence            | 1-8 bytes     | [0-2^64-1]

    Meta Byte Bit Layout (8 bits)
    ----------------------------------------------------------------------------
    Bits  | Field Name           | Description
    ------|----------------------|----------------------------------------------
    7-6   | fragment_type        | Type of fragment (2 bits)
    5     | bus_id_bytes         | Size of bus_id (0=1 byte, 1=2 bytes)
    4     | fragment_index_bytes | Size of fragment_index (0=1 byte, 1=2 bytes)
    3     | last_index_bytes     | Size of last_index (0=1 byte, 1=2 bytes)
    2-0   | sequence_bytes       | Size of sequence (value+1 = actual bytes)

    last_index = total_fragments - 1

    Note: All byte size fields indicate actual size by adding 1 to their value.
    Example: 
    - If bus_id_bytes = 0, bus_id uses 1 byte
    - If bus_id_bytes = 1, bus_id uses 2 bytes

    Bus ID is different from bus index. Bus index is the index of the bus in
    the bus array of the endpoint. Bus ID is the ID of the bus in the system.
    The ID 0 is reserved for the system bus. And the first user bus (index 0)
    has the ID 1.
*/


/// @brief The minimum size of fragment meta
#define POMELO_DELIVERY_FRAGMENT_META_MIN_SIZE 5

/// @brief The maximum size of fragment meta
#define POMELO_DELIVERY_FRAGMENT_META_MAX_SIZE 15


/// @brief The fragment type
typedef enum pomelo_delivery_fragment_type {
    /// @brief The data fragment with no ack required
    POMELO_FRAGMENT_TYPE_DATA_UNRELIABLE,

    /// @brief The fragment of sequenced parcel
    POMELO_FRAGMENT_TYPE_DATA_SEQUENCED,

    /// @brief The data fragment with ack required
    POMELO_FRAGMENT_TYPE_DATA_RELIABLE,

    /// @brief The ack fragment
    POMELO_FRAGMENT_TYPE_ACK

} pomelo_delivery_fragment_type;


/// @brief The meta data of fragment
typedef struct pomelo_delivery_fragment_meta_s pomelo_delivery_fragment_meta_t;


struct pomelo_delivery_fragment_s {
    /// @brief The content of fragment
    pomelo_buffer_view_t content;

    /// @brief The acked flag of fragment
    bool acked;
};


struct pomelo_delivery_fragment_meta_s {
    /// @brief The fragment type
    pomelo_delivery_fragment_type type;

    /// @brief The bus id
    size_t bus_id;

    /// @brief The fragment index
    size_t fragment_index;

    /// @brief The last index of fragments
    size_t last_index;

    /// @brief The sequence number of parcel
    uint64_t sequence;
};


/// @brief Convert the delivery mode to fragment type
#define pomelo_delivery_fragment_type_from_mode(delivery_mode) \
    (pomelo_delivery_fragment_type) (delivery_mode)


/// @brief Convert the fragment type to delivery mode
#define pomelo_delivery_mode_from_fragment_type(fragment_type) \
    (pomelo_delivery_mode) (fragment_type)


/// @brief Initialize the fragment
void pomelo_delivery_fragment_init(pomelo_delivery_fragment_t * fragment);


/// @brief Cleanup the fragment
void pomelo_delivery_fragment_cleanup(pomelo_delivery_fragment_t * fragment);


/// @brief Decode the fragment meta data
int pomelo_delivery_fragment_meta_decode(
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_view_t * view
);


/// @brief Encode the fragment meta
int pomelo_delivery_fragment_meta_encode(
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_buffer_view_t * view
);


/// @brief Attach the content to fragment
void pomelo_delivery_fragment_attach_content(
    pomelo_delivery_fragment_t * fragment,
    pomelo_buffer_view_t * view_content
);


/// @brief Attach a buffer to fragment
void pomelo_delivery_fragment_attach_buffer(
    pomelo_delivery_fragment_t * fragment,
    pomelo_buffer_t * buffer
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_FRAGMENT_SRC_H
