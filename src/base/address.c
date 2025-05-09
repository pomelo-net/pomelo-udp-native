#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "pomelo/address.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif


#define ADDRESS_V6_OPEN_BRACKET '['
#define ADDRESS_V6_CLOSE_BRACKET ']'
#define ADDRESS_PORT_SEPARATOR ':'

#define MIN_ADDRESS_V6_LEN 4    // Includes port number, a colon and 2 brackets
#define MAX_ADDRESS_V6_LEN 47   // Includes port number, a colon and 2 brackets
#define MIN_ADDRESS_V4_LEN 9    // Includes port number and a colon
#define MAX_ADDRESS_V4_LEN 21   // Includes port number and a colon

#define MAX_PORT_STR_LENGTH 5
#define BUF_ADDRESS_LENGTH 40      // Includes zero trail


/// @brief Convert string to port number
static int pomelo_address_parse_port(const char * str, uint16_t * port) {
    uint16_t result = 0;
    char c;
    while ((c = (*str)) != 0) {
        if (c < '0' || c > '9') {
            return -1;
        }

        result = result * 10 + (c - '0');
        str++;
    }

    *port = htons(result);
    return 0;
}


/// @brief Analyze address string
static int pomelo_address_analyze_string(
    const char * str,
    int * str_length,
    int * colon_count,
    int * last_colon_index,
    int max_length
) {
    char c;
    int i = 0;
    int count = 0;
    int last_index = -1;

    while ((c = (str[i])) != 0) {
        if (c == ':') {
            count++;
            last_index = i;
        }
        i++;
        if (i > max_length) {
            return -1; // Exceed max length
        }
    }

    if (str_length) {
        *str_length = i;
    }

    if (colon_count) {
        *colon_count = count;
    }

    if (last_colon_index) {
        *last_colon_index = last_index;
    }

    return 0;
}


int pomelo_address_from_string(pomelo_address_t * address, const char * str) {
    assert(address != NULL);
    assert(str != NULL);

    int length = 0;
    int colon_count = 0;
    int last_colon_index = -1;
    int ret = pomelo_address_analyze_string(
        str,
        &length,
        &colon_count,
        &last_colon_index,
        MAX_ADDRESS_V6_LEN
    );

    // Detect if str is IPv4 or IPv6
    if (
        ret < 0 ||
        length < MIN_ADDRESS_V6_LEN || // :::1
        length > MAX_ADDRESS_V6_LEN ||
        last_colon_index < 0
    ) {
        return -1;
    }

    /* Parse port */
    int port_str_length = length - last_colon_index - 1;
    if (port_str_length == 0 || port_str_length > MAX_PORT_STR_LENGTH) {
        return -1;
    }
    // Find the colon from last of string
    const char * port_str = str + (last_colon_index + 1); // Skip colon
    uint16_t port = 0;
    ret = pomelo_address_parse_port(port_str, &port);
    if (ret < 0) return ret;

    char ip_str[BUF_ADDRESS_LENGTH];
    if (colon_count == 1) {
        memcpy(ip_str, str, last_colon_index);
        ip_str[last_colon_index] = '\0';

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = port;

        int success = inet_pton(AF_INET, ip_str, &addr.sin_addr);
        if (!success) return -1;

        // Just 1 colon: IPv4
        return pomelo_address_from_sockaddr(
            address,
            (const struct sockaddr *) &addr
        );
    }

    // More than 1: IPv6
    if (str[0] == '[' && str[last_colon_index - 1] == ']') {
        int ip_length = last_colon_index - 2;
        memcpy(ip_str, str + 1, ip_length);
        ip_str[ip_length] = '\0';
    } else {
        memcpy(ip_str, str, last_colon_index);
        ip_str[last_colon_index] = '\0';
    }

    struct sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = port;

    int success = inet_pton(AF_INET6, ip_str, &addr.sin6_addr);
    if (!success) return -1;

    return pomelo_address_from_sockaddr(
        address,
        (const struct sockaddr *) &addr
    );
}


int pomelo_address_from_string_ex(
    pomelo_address_t * address,
    const char * ip,
    uint16_t port
) {
    assert(address != NULL);
    assert(ip != NULL);

    int colon_count = 0;
    int ret = pomelo_address_analyze_string(
        ip, NULL,
        &colon_count,
        NULL,
        MAX_ADDRESS_V6_LEN
    );
    if (ret < 0) return ret;

    if (colon_count > 0) {
        // IPv6
        address->type = POMELO_ADDRESS_IPV6;
        struct sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(port);

        int success = inet_pton(AF_INET6, ip, &addr.sin6_addr);
        if (!success) return -1;

        return pomelo_address_from_sockaddr(
            address,
            (const struct sockaddr *) &addr
        );
    } else {
        // IPv4
        address->type = POMELO_ADDRESS_IPV4;
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        int success = inet_pton(AF_INET, ip, &addr.sin_addr);
        if (!success) return -1;

        return pomelo_address_from_sockaddr(
            address,
            (const struct sockaddr *) &addr
        );
    }
}


int pomelo_address_to_string(
    pomelo_address_t * address,
    char * str,
    size_t capacity
) {
    assert(address != NULL);
    assert(str != NULL);

    if (capacity == 0) {
        return 0;
    }
    if (capacity == 1) {
        str[0] = '\0';
        return 0;
    }

    size_t max_length = capacity - 1; // At least 1 (min capacity = 2)

    struct sockaddr_storage sockaddr;
    int ret = pomelo_address_to_sockaddr(address, &sockaddr);
    if (ret < 0) {
        return ret;
    }

    size_t length = 0;
    if (address->type == POMELO_ADDRESS_IPV4) {
        const char * success = inet_ntop(
            AF_INET,
            &((struct sockaddr_in *) &sockaddr)->sin_addr,
            str,
            // Avoid warning
            #ifdef _WIN32
            capacity
            #else
            (socklen_t) capacity
            #endif
        );
        if (!success) {
            return -1;
        }
        length = strlen(str);
    } else {
        // Prepend open bracket
        str[0] = '[';
        if (max_length == 1) {
            str[1] = '\0';
            length = 1;
            // Not enough space to append IPv6
        } else {
            const char * success = inet_ntop(
                AF_INET6,
                &((struct sockaddr_in6 *) &sockaddr)->sin6_addr,
                str + 1,
                #ifdef _WIN32
                max_length
                #else
                (socklen_t) max_length
                #endif
            );
            if (!success) {
                return -1;
            }
            length = strlen(str);
            if (length < max_length) {
                str[length] = ']';
                str[length + 1] = '\0';
                length += 1;
            }
        }
    }

    size_t remain_length = max_length - length;
    if (remain_length == 0) {
        return 0;
    }

    // Port buffer
    char port_buffer[7] = { 0 };
    size_t port_len = (size_t) snprintf(
        port_buffer,
        sizeof(port_buffer),
        ":%" PRIu16,
        ntohs(address->port)
    );
    size_t copy_len = port_len < remain_length ? port_len : remain_length;
    memcpy(str + length, port_buffer, copy_len);
    str[length + copy_len] = '\0';

    return 0;
}


int pomelo_address_to_ip_string(
    pomelo_address_t * address,
    char * str,
    size_t capacity
) {
    assert(address != NULL);
    assert(str != NULL);

    if (capacity == 0) {
        return 0;
    }

    if (capacity == 1) {
        str[0] = '\0';
        return 0;
    }

    struct sockaddr_storage sockaddr;
    int ret = pomelo_address_to_sockaddr(address, &sockaddr);
    if (ret < 0) return ret;

    if (address->type == POMELO_ADDRESS_IPV4) {
        const char * success = inet_ntop(
            AF_INET,
            &((struct sockaddr_in *) &sockaddr)->sin_addr,
            str,
            #ifdef _WIN32
            capacity
            #else
            (socklen_t) capacity
            #endif
        );
        return success ? 0 : -1;
    } else {
        const char * success = inet_ntop(
            AF_INET6,
            &((struct sockaddr_in6 *) &sockaddr)->sin6_addr,
            str,
            #ifdef _WIN32
            capacity
            #else
            (socklen_t) capacity
            #endif
        );
        return success ? 0 : -1;
    }
}


int pomelo_address_from_sockaddr(
    pomelo_address_t * address,
    const sockaddr_t * sockaddr
) {
    assert(address != NULL);
    assert(sockaddr != NULL);

    const struct sockaddr_in * addr_v4;
    const struct sockaddr_in6 * addr_v6;

    switch (sockaddr->sa_family) {
        case AF_INET:
            address->type = POMELO_ADDRESS_IPV4;
            addr_v4 = (const struct sockaddr_in *) sockaddr;
            
            memcpy(&address->ip, &addr_v4->sin_addr, 4); // ip - 4 bytes
            address->port = addr_v4->sin_port;           // port
            break;

        case AF_INET6:
            address->type = POMELO_ADDRESS_IPV6;
            addr_v6 = (const struct sockaddr_in6 *) sockaddr;

            memcpy(&address->ip, &addr_v6->sin6_addr, 16); // ip - 16 bytes
            address->port = addr_v6->sin6_port;            // port
            break;

        default:
            return -1;
    }

    return 0;
}


int pomelo_address_to_sockaddr(
    pomelo_address_t * address,
    sockaddr_storage_t * sockaddr
) {
    assert(address != NULL);
    assert(sockaddr != NULL);

    struct sockaddr_in * addr_v4;
    struct sockaddr_in6 * addr_v6;

    switch (address->type) {
        case POMELO_ADDRESS_IPV4:
            sockaddr->ss_family = AF_INET;
            addr_v4 = (struct sockaddr_in *) sockaddr;

            memcpy(&addr_v4->sin_addr, &address->ip, 4); // ip - 4 bytes
            addr_v4->sin_port = address->port;           // port
            break;

        case POMELO_ADDRESS_IPV6:
            sockaddr->ss_family = AF_INET6;
            addr_v6 = (struct sockaddr_in6 *) sockaddr;

            memcpy(&addr_v6->sin6_addr, &address->ip, 16); // ip - 16 bytes
            addr_v6->sin6_port = address->port;            // port
            break;

        default:
            return -1;
    }

    return 0;
}


bool pomelo_address_compare(
    pomelo_address_t * first,
    pomelo_address_t * second
) {
    assert(first != NULL);
    assert(second != NULL);

    if (first->port != second->port) {
        return 0;
    }

    if (first->type != second->type) {
        return 0;
    }

    int cmp = (first->type == POMELO_ADDRESS_IPV4)
        ? memcmp(first->ip.v4, second->ip.v4, sizeof(first->ip.v4))
        : memcmp(first->ip.v6, second->ip.v6, sizeof(first->ip.v6));

    return cmp == 0;
}


int64_t pomelo_address_hash(pomelo_address_t * address) {
    assert(address != NULL);

    // IPv4 layout: 4 bytes
    // IPv6 layout: 16 bytes

    int32_t * values = (int32_t *) (&address->ip);
    return (address->type == POMELO_ADDRESS_IPV4)
        ? (values[0] ^ address->port)
        : (values[0] ^ values[1] ^ values[2] ^ values[3] ^ address->port);
}


uint16_t pomelo_address_port(pomelo_address_t * address) {
    assert(address != NULL);
    return ntohs(address->port);
}


void pomelo_address_ip(pomelo_address_t * address, pomelo_address_ip_t * ip) {
    assert(address != NULL);
    assert(ip != NULL);

    if (address->type == POMELO_ADDRESS_IPV4) {
        // Just copy with IPv4
        memcpy(ip->v4, &address->ip.v4, sizeof(address->ip.v4));
    } else {
        // Reverse byte order
        uint16_t * dst = ip->v6;
        uint16_t * src = address->ip.v6;
        for (int i = 0; i < 8; i++) {
            dst[i] = ntohs(src[i]);
        }
    }
}


void pomelo_address_set(
    pomelo_address_t * address,
    pomelo_address_type type,
    pomelo_address_ip_t * ip,
    uint16_t port
) {
    assert(address != NULL);
    assert(ip != NULL);
    if (type != POMELO_ADDRESS_IPV4 && type != POMELO_ADDRESS_IPV6) {
        return; // Invalid type
    }

    if (type == POMELO_ADDRESS_IPV4) {
        // Just copy
        memcpy(address->ip.v4, ip->v4, sizeof(ip->v4));
    } else {
        address->ip.v6[0] = htons(ip->v6[0]);
        address->ip.v6[1] = htons(ip->v6[1]);
        address->ip.v6[2] = htons(ip->v6[2]);
        address->ip.v6[3] = htons(ip->v6[3]);
        address->ip.v6[4] = htons(ip->v6[4]);
        address->ip.v6[5] = htons(ip->v6[5]);
        address->ip.v6[6] = htons(ip->v6[6]);
        address->ip.v6[7] = htons(ip->v6[7]);
    }

    address->type = type;
    address->port = htons(port);
}
