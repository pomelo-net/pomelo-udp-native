#include <assert.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include "shared.h"


static pomelo_example_env_t env;
static uint8_t connect_token[POMELO_CONNECT_TOKEN_BYTES];


/// @brief Make a request to server to get connect token
static void http_client_request_connect_token(void);


int main(int argc, char * argv[]) {
    // Initialize Env
    pomelo_example_env_init(&env, argc > 1 ? argv[1] : NULL);
    
    // Run example
    http_client_request_connect_token();

    // Run UV loop
    uv_run(&env.loop, UV_RUN_DEFAULT);

    // Cleanup Env
    pomelo_example_env_finalize(&env);
    return 0;
}


/// @brief Start connect to server
void pomelo_example_connect_socket(void) {
    printf("Start connecting to server\n");
    // Connect to socket server
    int ret = pomelo_socket_connect(env.socket, connect_token);
    pomelo_assert(ret == 0);
}


/* -------------------------------------------------------------------------- */
/*                          Socket API implementation                         */
/* -------------------------------------------------------------------------- */


void pomelo_session_on_cleanup(pomelo_session_t * session) {
    (void) session;
}


void pomelo_channel_on_cleanup(pomelo_channel_t * channel) {
    (void) channel;
}


void pomelo_socket_on_connected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    (void) socket;

    int64_t client_id = pomelo_session_get_client_id(session);
    printf("On connected: %" PRId64 "\n", client_id);

    // Acquire new message and send it to server
    pomelo_message_t * message = pomelo_context_acquire_message(env.context);
    pomelo_assert(message != NULL);
    pomelo_message_write_int32(message, 12345);
    pomelo_message_write_uint64(message, 887722);

    pomelo_session_send(session, 0, message, NULL);
    pomelo_message_unref(message);
}


void pomelo_socket_on_disconnected(
    pomelo_socket_t * socket,
    pomelo_session_t * session
) {
    (void) socket;
    int64_t client_id = pomelo_session_get_client_id(session);
    printf("On disconnected: %" PRId64 "\n", client_id);
}


void pomelo_socket_on_received(
    pomelo_socket_t * socket,
    pomelo_session_t * session,
    pomelo_message_t * message
) {
    (void) socket;

    int64_t client_id = pomelo_session_get_client_id(session);
    size_t size = pomelo_message_size(message);
    printf("On received: %" PRId64 " message %zu bytes\n", client_id, size);
    int32_t v1 = 0;
    uint64_t v2 = 0;

    int ret = pomelo_message_read_int32(message, &v1);
    pomelo_assert(ret == 0);
    ret = pomelo_message_read_uint64(message, &v2);
    pomelo_assert(ret == 0);
    printf("Received: %" PRId32 " %" PRIu64 "\n", v1, v2);

    printf("Disconnecting...\n");
    pomelo_session_disconnect(session);
    // Stop socket here will cause crash
    // TODO: Shutdown the platform
}


void pomelo_socket_on_connect_result(
    pomelo_socket_t * socket,
    pomelo_socket_connect_result result
) {
    (void) socket;
    (void) result;
}


void pomelo_socket_on_send_result(
    pomelo_socket_t * socket,
    pomelo_message_t * message,
    void * data,
    size_t send_count
) {
    (void) socket;
    (void) message;
    (void) data;
    (void) send_count;
}

/* -------------------------------------------------------------------------- */
/*                          HTTP connect token client                         */
/* -------------------------------------------------------------------------- */

#define POMELO_CONNECT_TOKEN_BASE64_LENGTH                                     \
    pomelo_base64_calc_encoded_length(POMELO_CONNECT_TOKEN_BYTES)


typedef struct {
    char last_char;
    bool header_end;
    char body[POMELO_CONNECT_TOKEN_BASE64_LENGTH];
    size_t offset;
} http_client_state_t;


static uv_tcp_t http_client;
static uv_connect_t http_connect;
static http_client_state_t http_state;
static uv_write_t http_write;


static void http_client_on_data(char * data, size_t length) {
    size_t i = 0;
    // Skip the header

    while (!http_state.header_end && i < length) {
        char c = data[i++];
        if (c == '\r') continue; // Skip
        if (c == '\n' && http_state.last_char == '\n') {
            http_state.header_end = true;
        }
        http_state.last_char = c;
    }

    while (http_state.offset < sizeof(http_state.body) && i < length) {
        char c = data[i++];
        if (c == '\r') continue; // Skip '\r'
        http_state.body[http_state.offset++] = c;
    }
}


static void http_client_on_closed(uv_handle_t * handle) {
    (void) handle;

    // Got connect token b64, decode it
    int ret = pomelo_base64_decode(
        connect_token,
        sizeof(connect_token),
        http_state.body,
        strlen(http_state.body)
    );
    pomelo_assert(ret == 0);

    printf("Got connect token\n");
    pomelo_example_connect_socket();
}


static void http_client_on_alloc(
    uv_handle_t * handle,
    size_t suggested_size,
    uv_buf_t * buf
) {
    (void) handle;
    buf->base = pomelo_allocator_malloc(env.allocator, suggested_size);
    buf->len = (BUFFER_LENGTH_TYPE) suggested_size;
}


static void http_client_on_read(
    uv_stream_t * stream,
    ssize_t nread,
    const uv_buf_t * buf
) {
    if (nread >= 0) {
        // Call the callback
        http_client_on_data(buf->base, (size_t) nread);
    } else if (nread == UV_EOF) {
        // Disconnected, free client
        uv_close((uv_handle_t *) stream, http_client_on_closed);
    }

    // Free the data
    pomelo_allocator_free(env.allocator, buf->base);
}


static void http_client_callback(uv_connect_t * req, int status) {
    if (status < 0) return;

    memset(&http_state, 0, sizeof(http_client_state_t));
    uv_read_start(req->handle, http_client_on_alloc, http_client_on_read);

    // Send request
    uv_buf_t buf;
    char header[128];
    
    buf.base = header;
    buf.len = snprintf(
        header,
        sizeof(header),
        "GET / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: pomelo/1.0.0\r\n"
        "Accept: */*\r\n"
        "\r\n",
        ADDRESS_HOST
    );

    uv_write(&http_write, req->handle, &buf, 1, NULL);
}


static void http_client_request_connect_token(void) {
    printf(
        "Start getting connect token from http://%s:%d\n",
        SERVICE_HOST,
        SERVICE_PORT
    );

    // Start HTTP server
    int ret = uv_tcp_init(&env.loop, &http_client);
    pomelo_assert(ret == 0);

    struct sockaddr_in addr;
    ret = uv_ip4_addr(ADDRESS_HOST, ADDRESS_PORT, &addr);
    pomelo_assert(ret == 0);

    ret = uv_tcp_connect(
        &http_connect,
        &http_client,
        (struct sockaddr *) &addr,
        http_client_callback
    );
    pomelo_assert(ret == 0);
}
