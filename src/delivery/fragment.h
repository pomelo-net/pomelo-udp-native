#ifndef POMELO_DELIVERY_FRAGMENT_SRC_H
#define POMELO_DELIVERY_FRAGMENT_SRC_H
#include "base/payload.h"
#include "base/buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    The meta data is at the end of payload with the following layout:
        (1 - 2 Bytes) | <bus_index>        | [0 - 65535]
        (1 - 2 Bytes) | <fragment_index>   | [0 - 65535]
        (1 - 2 Bytes) | <total_fragments>  | [0 - 65535]
        (1 - 8 Bytes) | <sequence>         | [0 - (2^64 - 1)]
        (1 Byte)      | <meta_byte>        | [0 - 0xFF]

    The bit-layout of meta_byte (1 Byte):
        (2 b) | <fragment_type>
        (1 b) | <bus_index_bytes>
        (1 b) | <fragment_index_bytes>
        (1 b) | <total_fragments_bytes>
        (3 b) | <sequence_bytes>

    Note that: All the bits which store the number of bytes count from 1.
    For example:
        bus_index_bytes = 0 => The number of bytes storing bus_index is 1;
        bus_index_bytes = 1 => The number of bytes storing bus_index is 2;
*/

/// @brief The fragment type
typedef enum pomelo_delivery_fragment_type {
    /// @brief The data fragment with no ack required
    POMELO_FRAGMENT_TYPE_DATA_UNRELIABLE,

    /// @brief The fragment of sequenced parcel
    POMELO_FRAGMENT_TYPE_DATA_SEQUENCED,

    /// @brief The data fragment with ack required
    POMELO_FRAGMENT_TYPE_DATA_RELIABLE,

    /// @brief The ack fragment
    POMELO_FRAGMENT_TYPE_ACK,

    /// @brief Fragment type count
    POMELO_FRAGMENT_TYPE_COUNT
} pomelo_delivery_fragment_type;


/// @brief The meta data of fragment
typedef struct pomelo_delivery_fragment_meta_s pomelo_delivery_fragment_meta_t;


struct pomelo_delivery_fragment_s {
    /// @brief The fragment index
    size_t index;

    /// @brief The payload of this fragment
    pomelo_payload_t payload;

    /// @brief Attached buffer
    pomelo_buffer_t * buffer;

    /// @brief The acked flag of fragment
    uint8_t acked;
};


struct pomelo_delivery_fragment_meta_s {
    /// @brief The fragment type
    pomelo_delivery_fragment_type type;

    /// @brief The bus index
    size_t bus_index;

    /// @brief The fragment index
    size_t fragment_index;

    /// @brief The total number of fragments
    size_t total_fragments;

    /// @brief The sequence number of parcel
    uint64_t parcel_sequence;
};


/// @brief Decode the fragment meta data
int pomelo_delivery_fragment_meta_decode(
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_payload_t * payload
);


/// @brief Encode the fragment meta
int pomelo_delivery_fragment_meta_encode(
    pomelo_delivery_fragment_meta_t * meta,
    pomelo_payload_t * payload
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_FRAGMENT_SRC_H
