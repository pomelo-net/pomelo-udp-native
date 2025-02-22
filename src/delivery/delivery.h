#ifndef POMELO_DELIVERY_SRC_H
#define POMELO_DELIVERY_SRC_H
#include "pomelo/allocator.h"
#include "pomelo/statistic/statistic-delivery.h"
#include "pomelo/common.h"
#include "base/buffer.h"
#include "platform/platform.h"


#ifdef __cplusplus
extern "C" {
#endif


/// The maximum bytes of meta data of fragment
#define POMELO_MAX_FRAGMENT_META_DATA_BYTES 15

/// The maximum number of fragments this protocol can support
#define POMELO_DELIVERY_PARCEL_MAX_FRAGMENTS 65535

/// The maximum number of fragments in a parcel.
/// By the specs, each fragment can store (1200 - 15) bytes of data.
/// 222 fragments ~ 256KB.
#define POMELO_DELIVERY_PARCEL_MAX_FRAGMENTS_DEFAULT 222

/// The maximum number of buses
#define POMELO_DELIVERY_MAX_BUSES POMELO_MAX_CHANNELS


/// @brief Delivery mode
typedef enum pomelo_delivery_mode {
    /// @brief The unreliable mode
    POMELO_DELIVERY_MODE_UNRELIABLE,

    /// @brief The sequenced mode
    POMELO_DELIVERY_MODE_SEQUENCED,

    /// @brief The reliable mode
    POMELO_DELIVERY_MODE_RELIABLE
} pomelo_delivery_mode;


/// @brief The delivery context interface.
typedef struct pomelo_delivery_context_s pomelo_delivery_context_t;

/// @brief The thread-safe root delivery context
typedef struct pomelo_delivery_context_root_s pomelo_delivery_context_root_t;

#ifdef POMELO_MULTI_THREAD

/// @brief The non-threadsafe shared delivery context
typedef struct pomelo_delivery_context_shared_s
    pomelo_delivery_context_shared_t;

/// @brief The non-threadsafe delivery context
typedef struct pomelo_delivery_context_shared_options_s
    pomelo_delivery_context_shared_options_t;

#endif // POMELO_MULTI_THREAD

/// @brief The transporter.
/// Each system can have one or more transporters
typedef struct pomelo_delivery_transporter_s pomelo_delivery_transporter_t;

/// @brief The delivery endpoint.
/// This contains a group of buses and the some statistic values about
/// status of the connection.
typedef struct pomelo_delivery_endpoint_s pomelo_delivery_endpoint_t;

/// @brief The delivery bus
typedef struct pomelo_delivery_bus_s pomelo_delivery_bus_t;

/// @brief The delivery context creating options
typedef struct pomelo_delivery_context_root_options_s
    pomelo_delivery_context_root_options_t;

/// @brief The transporter creating options
typedef struct pomelo_delivery_transporter_options_s
    pomelo_delivery_transporter_options_t;

/// @brief The data for transmitting. It contains multiple fragments.
typedef struct pomelo_delivery_parcel_s pomelo_delivery_parcel_t;

/// @brief The fragment of a parcel
typedef struct pomelo_delivery_fragment_s pomelo_delivery_fragment_t;

/// @brief The parcel reader.
typedef struct pomelo_delivery_parcel_reader_s pomelo_delivery_parcel_reader_t;

/// @brief The parcel writer.
typedef struct pomelo_delivery_parcel_writer_s pomelo_delivery_parcel_writer_t;


/// @brief Fragment acquiring function.
/// If buffer is passed as NULL, new buffer will be acquired and attachted to
/// new fragment.
typedef pomelo_delivery_fragment_t *
(*pomelo_delivery_context_acquire_fragment_fn)(
    pomelo_delivery_context_t * context,
    pomelo_buffer_t * buffer
);

/// @brief Fragment releasing function
typedef void (*pomelo_delivery_context_release_fragment_fn)(
    pomelo_delivery_context_t * context,
    pomelo_delivery_fragment_t * fragment
);


/// @brief The statistic function of delivery context
typedef void (*pomelo_delivery_context_statistic_fn)(
    pomelo_delivery_context_t * context,
    pomelo_statistic_delivery_t * statistic
);


/// @brief Parcel acquiring function
typedef pomelo_delivery_parcel_t *
(*pomelo_delivery_context_acquire_parcel_fn)(
    pomelo_delivery_context_t * context
);

/// @brief Parcel releasing function
typedef void (*pomelo_delivery_context_release_parcel_fn)(
    pomelo_delivery_context_t * context,
    pomelo_delivery_parcel_t * parcel
);


struct pomelo_delivery_context_root_options_s {
    /// @brief The allocator of delivery context
    pomelo_allocator_t * allocator;

    /// @brief The global buffer context
    pomelo_buffer_context_root_t * buffer_context;

    /// @brief The total capacity of a packet
    size_t fragment_capacity;

    /// @brief The maximum number of fragments in a parcel
    size_t max_fragments;
};


struct pomelo_delivery_context_shared_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The root context
    pomelo_delivery_context_root_t * context;
};


struct pomelo_delivery_context_s {
    /// @brief The buffer context
    pomelo_buffer_context_t * buffer_context;

    /// @brief Acquire new fragment
    pomelo_delivery_context_acquire_fragment_fn acquire_fragment;

    /// @brief Release a fragment
    pomelo_delivery_context_release_fragment_fn release_fragment;

    /// @brief Acquire new parcel
    pomelo_delivery_context_acquire_parcel_fn acquire_parcel;

    /// @brief Release a parcel
    pomelo_delivery_context_release_parcel_fn release_parcel;

    /// @brief Get the statistic of delivery context
    pomelo_delivery_context_statistic_fn statistic;

    /// @brief The capacity of fragment payload
    size_t fragment_payload_capacity;

    /// @brief The maximum number of fragments in a parcel
    size_t max_fragments;
};


struct pomelo_delivery_transporter_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The delivery context
    pomelo_delivery_context_t * context;

    /// @brief The platform
    pomelo_platform_t * platform;

    /// @brief The number of buses
    size_t nbuses;
};


/* -------------------------------------------------------------------------- */
/*                           Delivery context APIs                            */
/* -------------------------------------------------------------------------- */

/// @brief Initialize the delivery context creating options
void pomelo_delivery_context_root_options_init(
    pomelo_delivery_context_root_options_t * options
);

/// @brief Create new delivery context
pomelo_delivery_context_root_t * pomelo_delivery_context_root_create(
    pomelo_delivery_context_root_options_t * options
);

/// @brief Destroy a delivery context
void pomelo_delivery_context_root_destroy(
    pomelo_delivery_context_root_t * context
);

#ifdef POMELO_MULTI_THREAD

/// @brief Initialize local-thread delivery context creating options
void pomelo_delivery_context_shared_options_init(
    pomelo_delivery_context_shared_options_t * options
);

/// @brief Create new local-thread delivery context
pomelo_delivery_context_shared_t * pomelo_delivery_context_shared_create(
    pomelo_delivery_context_shared_options_t * options
);

/// @brief Destroy a local-thread delivery context
void pomelo_delivery_context_shared_destroy(
    pomelo_delivery_context_shared_t * context
);

#endif // !POMELO_MULTI_THREAD


/* -------------------------------------------------------------------------- */
/*                              Transporter APIs                              */
/* -------------------------------------------------------------------------- */

/// @brief Initialize transporter options
void pomelo_delivery_transporter_options_init(
    pomelo_delivery_transporter_options_t * options
);

/// @brief Create new transporter
pomelo_delivery_transporter_t * pomelo_delivery_transporter_create(
    pomelo_delivery_transporter_options_t * options
);

/// @brief Destroy the transporter.
/// Destroying a transporter without stopping it may cause unexpected behaviors.
/// It is highly encouraged to stop and wait until the transporter completely
/// stops before destroying it.
void pomelo_delivery_transporter_destroy(
    pomelo_delivery_transporter_t * transporter
);

/// @brief Acquire an endpoint
pomelo_delivery_endpoint_t * pomelo_delivery_transporter_acquire_endpoint(
    pomelo_delivery_transporter_t * transporter
);

/// @brief Release an endpoint
void pomelo_delivery_transporter_release_endpoint(
    pomelo_delivery_transporter_t * transporter,
    pomelo_delivery_endpoint_t * endpoint
);

/// @brief Stop all doing works
void pomelo_delivery_transporter_stop(
    pomelo_delivery_transporter_t * transporter
);

/// @brief Callback when transporter has been stopped
void pomelo_delivery_transporter_on_stopped(
    pomelo_delivery_transporter_t * transporter
);

/// @brief Get the transporter extra data
#define pomelo_delivery_transporter_get_extra(transporter)                     \
    (*((void **) (transporter)))

/// @brief Set the extra data for transporter
#define pomelo_delivery_transporter_set_extra(transporter, data)               \
    (*((void **) (transporter)) = (data))

/// @brief Get the statistic of transporter
void pomelo_delivery_transporter_statistic(
    pomelo_delivery_transporter_t * transporter,
    pomelo_statistic_delivery_t * statistic
);

/* -------------------------------------------------------------------------- */
/*                               Endpoint APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Get the bus by index
pomelo_delivery_bus_t * pomelo_delivery_endpoint_get_bus(
    pomelo_delivery_endpoint_t * endpoint,
    size_t bus_index
);

/// @brief Process when enpoint has received a payload
int pomelo_delivery_endpoint_recv(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
);

/// @brief Process sending the fragment
/// [External linkage]
int pomelo_delivery_endpoint_send(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
);

/// @brief Get round trip time information of endpoint
/// [External linkage]
int pomelo_delivery_endpoint_rtt(
    pomelo_delivery_endpoint_t * endpoint,
    uint64_t * mean,
    uint64_t * variance
);


/// @brief Get the transporter extra data
#define pomelo_delivery_endpoint_get_extra(endpoint)                           \
    (*((void **) (endpoint)))

/// @brief Set the extra data for transporter
#define pomelo_delivery_endpoint_set_extra(endpoint, data)                     \
    (*((void **) (endpoint)) = (data))

/* -------------------------------------------------------------------------- */
/*                                 Bus APIs                                   */
/* -------------------------------------------------------------------------- */

/// @brief Get the endpoint from bus
#define pomelo_delivery_bus_get_endpoint(bus)                                 \
    *((pomelo_delivery_endpoint_t **) bus)

/// @brief Send a parcel through a bus.
int pomelo_delivery_bus_send(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel,
    pomelo_delivery_mode mode
);

/// @brief Process when bus receives a parcel.
/// After this callback is called, the parcel will immediately be released.
/// [External linkage]
void pomelo_delivery_bus_on_received(
    pomelo_delivery_bus_t * bus,
    pomelo_delivery_parcel_t * parcel
);



/* -------------------------------------------------------------------------- */
/*                                Parcel APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Increase the reference counter of this parcel.
/// @return 0 on success or -1 on failure
int pomelo_delivery_parcel_ref(pomelo_delivery_parcel_t * parcel);


/// @brief Decrease the reference counter of this parcel
/// @return 0 on success or -1 on failure
int pomelo_delivery_parcel_unref(pomelo_delivery_parcel_t * parcel);


/// @brief Get the reader of parcel.
/// Each parcel has one and only one reader. So that, this function always
/// returns the same pointer to the reader. This also resets the reader to
/// the beginning of parcel.
/// The reader always starts reading data at the beginning of parcel.
pomelo_delivery_parcel_reader_t * pomelo_delivery_parcel_get_reader(
    pomelo_delivery_parcel_t * parcel
);


/// @brief Get the writer of parcel.
/// Each parcel has one and only one writer. So that, this function always
/// returns the same pointer to the writer. This also resets the writer to
/// the end of parcel.
/// The writer always starts writing data at the end of parcel.
pomelo_delivery_parcel_writer_t * pomelo_delivery_parcel_get_writer(
    pomelo_delivery_parcel_t * parcel
);


/// @brief Clone the delivery parcel.
/// Currently, only writing parcel are supported.
pomelo_delivery_parcel_t * pomelo_delivery_parcel_clone(
    pomelo_delivery_parcel_t * parcel
);


/// @brief Write buffer to the parcel
int pomelo_delivery_parcel_writer_write_buffer(
    pomelo_delivery_parcel_writer_t * writer,
    size_t length,
    const uint8_t * buffer
);

/// @brief Get the written bytes of writing parcel
/// @return The number of written bytes
size_t pomelo_delivery_parcel_writer_written_bytes(
    pomelo_delivery_parcel_writer_t * writer
);

/// @brief Reader buffer from parcel
int pomelo_delivery_parcel_reader_read_buffer(
    pomelo_delivery_parcel_reader_t * reader,
    size_t length,
    uint8_t * buffer
);

/// @brief Get the remain available bytes of reading parcel
/// @param reader The reader of parcel
/// @return The remain bytes of parcel
size_t pomelo_relivery_parcel_reader_remain_bytes(
    pomelo_delivery_parcel_reader_t * reader
);


/// @brief Pack the parcel.
/// This will make the parcel ready to be sent to another endpoint
void pomelo_delivery_parcel_pack(pomelo_delivery_parcel_t * parcel);

#ifdef __cplusplus
}
#endif
#endif // POMELO_DELIVERY_SRC_H
