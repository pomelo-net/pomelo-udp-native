#ifndef POMELO_PLATFORM_UV_SOCKET_SRC_H
#define POMELO_PLATFORM_UV_SOCKET_SRC_H
#include "utils/pool.h"
#include "utils/list.h"
#include "platform-uv.h"

#ifdef __cplusplus
extern "C" {
#endif


#define POMELO_CLIENT_SOCKET_SNDBUF_SIZE 256 * 1024
#define POMELO_CLIENT_SOCKET_RCVBUF_SIZE 256 * 1024
#define POMELO_SERVER_SOCKET_SNDBUF_SIZE 4 * 1024 * 1024
#define POMELO_SERVER_SOCKET_RCVBUF_SIZE 4 * 1024 * 1024

/// There are 2 vectors: header & body
#define POMELO_PLATFORM_UDP_MAX_NUMBER_BUF_VECTORS 2

#if UV_VERSION_HEX >= ((1 << 16) | (27 << 8) | 0)
#define UV_UV_CONNECT_AVAILABLE 1
#else
#define UV_UV_CONNECT_AVAILABLE 0
#endif


/// @brief The sending information
typedef struct pomelo_platform_send_s pomelo_platform_send_t;


struct pomelo_platform_udp_s {
    /// @brief The socket controller
    pomelo_platform_udp_controller_t * controller;

    /// @brief The UDP handle
    uv_udp_t uv_udp;

    /// @brief The context for callbacks
    void * context;

    /// @brief The pool of payload
    pomelo_platform_alloc_cb alloc_callback;

    /// @brief The payload receive callback
    pomelo_platform_recv_cb recv_callback;

    /// @brief The entry in sockets list
    pomelo_list_entry_t * entry;

    /// @brief Closing flag
    bool closing;

#if UV_UV_CONNECT_AVAILABLE == 0
    // In the case of unavailable connect function, we need to store the target
    // address

    /// @brief The target address for socket
    struct sockaddr_storage target_addr;
#endif
};


struct pomelo_platform_udp_controller_s {
    /// @brief Platform
    pomelo_platform_uv_t * platform;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief UV loop
    uv_loop_t * uv_loop;

    /// @brief The send information pool
    pomelo_pool_t * send_pool;

    /// @brief The pool of platform sockets
    pomelo_pool_t * socket_pool;

    /// @brief The list of running sockets
    pomelo_list_t * sockets;

    /// @brief Total sent bytes
    uint64_t send_bytes;

    /// @brief Total received bytes
    uint64_t recv_bytes;

    /// @brief The flag of running
    bool running;

    /// @brief The number of sending requests
    size_t sending_requests;
};


struct pomelo_platform_send_s {
    /// @brief The socket
    pomelo_platform_udp_t * socket;

    /// @brief The uv request
    uv_udp_send_t uv_req;

    /// @brief The callback
    pomelo_platform_send_cb callback;

    /// @brief The callback data
    void * callback_data;
};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Create platform socket controller
pomelo_platform_udp_controller_t * pomelo_platform_udp_controller_create(
    pomelo_platform_uv_t * platform,
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
);


/// @brief Destroy platform socket controller
void pomelo_platform_udp_controller_destroy(
    pomelo_platform_udp_controller_t * controller
);


/// @brief Get the socket controller statistic
void pomelo_platform_udp_controller_statistic(
    pomelo_platform_udp_controller_t * controller,
    pomelo_statistic_platform_uv_t * statistic
);


/// @brief Startup the UDP controller
void pomelo_platform_udp_controller_startup(
    pomelo_platform_udp_controller_t * controller
);


/// @brief Close all UDP handles
void pomelo_platform_udp_controller_shutdown(
    pomelo_platform_udp_controller_t * controller
);


/// @brief Callback when the UDP controller is completely shutdown
void pomelo_platform_udp_controller_on_shutdown(
    pomelo_platform_udp_controller_t * controller
);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Callback when udp socket is closed
void pomelo_platform_udp_on_closed(uv_udp_t * udp);


/// @brief Close the udp socket
void pomelo_platform_udp_close(pomelo_platform_udp_t * socket);


/// @brief The allocation callback for uv
void pomelo_platform_udp_alloc_callback(
    uv_handle_t * handle,
    size_t suggested_size,
    uv_buf_t * buf
);


/// @brief The receiving callback for uv
void pomelo_platform_udp_recv_callback(
    uv_udp_t * handle,
    ssize_t nread,
    const uv_buf_t * buf,
    const struct sockaddr * addr,
    unsigned flags
);


/// @brief Sending done callback
void pomelo_platform_send_done(uv_udp_send_t * req, int status);


/// @brief Check if the controller is shutdown
void pomelo_platform_udp_controller_check_shutdown(
    pomelo_platform_udp_controller_t * controller
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_SOCKET_SRC_H
