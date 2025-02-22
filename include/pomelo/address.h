#ifndef POMELO_ADDRESS_H
#define POMELO_ADDRESS_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

#define POMELO_ADDRESS_STRING_BUFFER_CAPACITY 48

typedef struct sockaddr sockaddr_t;
typedef struct sockaddr_storage sockaddr_storage_t;


/// @brief The IP address
typedef union pomelo_address_ip_u {
    /// @brief v4 type
    uint8_t v4[4];

    /// @brief v6 type
    uint16_t v6[8];
} pomelo_address_ip_t;


/// @brief Address type
typedef enum pomelo_address_type_e {
    POMELO_ADDRESS_IPV4 = 1,
    POMELO_ADDRESS_IPV6 = 2
} pomelo_address_type;


struct pomelo_address_s {
    /// @brief The address type
    pomelo_address_type type;

    /// @brief The socket IP (With network byte order - BigEndian)
    pomelo_address_ip_t ip;

    /// @brief The socket address port (With network byte order - BigEndian)
    uint16_t port;
};

/// @brief Address
typedef struct pomelo_address_s pomelo_address_t;


/// @brief Convert string to address
/// @param address The output address
/// @param str The input string
/// @return 0 on success, or an error code < 0 on failure.
int pomelo_address_from_string(pomelo_address_t * address, const char * str);


/// @brief Convert address to string
/// @param address Input address
/// @param str Output string
/// @param len Output string capacity buffer (Include zero)
int pomelo_address_to_string(
    pomelo_address_t * address,
    char * str,
    size_t capacity
);


/// @brief Convert sockaddr to address
/// @param address The address
/// @param sockaddr The socket address
/// @return 0 on success, or an error code < 0 on failure.
int pomelo_address_from_sockaddr(
    pomelo_address_t * address,
    const sockaddr_t * sockaddr
);

/// @brief Convert address to sockaddr
/// @param address The address
/// @param sockaddr The socket address
/// @return 0 on success, or an error code < 0 on failure.
int pomelo_address_to_sockaddr(
    pomelo_address_t * address,
    sockaddr_storage_t * sockaddr
);


/// @brief Compare two addresses
/// @param first The first address
/// @param second The second address
/// @returns Return true if two addresses are equal or false if they are not
bool pomelo_address_compare(
    pomelo_address_t * first,
    pomelo_address_t * second
);


/// @brief The address hash
int64_t pomelo_address_hash(pomelo_address_t * address);


/// @brief Get the address port as byte-order of host machine
uint16_t pomelo_address_port(pomelo_address_t * address);


/// @brief Get the address ip as byte-order of host machine
void pomelo_address_ip(pomelo_address_t * address, pomelo_address_ip_t * ip);


/// @brief Set the value of address
void pomelo_address_set(
    pomelo_address_t * address,
    pomelo_address_type type,
    pomelo_address_ip_t * ip,
    uint16_t port
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_ADDRESS_H
