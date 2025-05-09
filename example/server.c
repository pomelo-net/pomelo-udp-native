#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <stdlib.h>
#include "shared.h"


#define MAX_CLIENTS 20
#define TIMEOUT 1
#define EXPIRE 3600

#define HTTP_BACKLOG 128

static pomelo_example_env_t env;
static uint8_t private_key[POMELO_KEY_BYTES];
static uint64_t protocol_id = 0;
static int64_t client_id = 0;


/// @brief Serve the connect tokens
static void http_server_serve_connect_token(void);


int main(int argc, char * argv[]) {
    // Initialize Env
    pomelo_example_env_init(&env, argc > 1 ? argv[1] : NULL);

    // Random initial values
    pomelo_random_buffer(private_key, sizeof(private_key));
    pomelo_random_buffer((uint8_t *) &protocol_id, sizeof(protocol_id));

    // Parse address
    char address_str[POMELO_ADDRESS_STRING_BUFFER_CAPACITY];
    snprintf(
        address_str,
        sizeof(address_str),
        "%s:%d",
        ADDRESS_HOST,
        ADDRESS_PORT
    );

    pomelo_address_t address;
    int ret = pomelo_address_from_string(&address, address_str);
    pomelo_assert(ret == 0);

    // Start listening
    ret = pomelo_socket_listen(
        env.socket,
        private_key,
        protocol_id,
        MAX_CLIENTS,
        &address
    );
    pomelo_assert(ret == 0);
    printf("Socket is listening on %s\n", address_str);

    // Initialize HTTP server
    http_server_serve_connect_token();

    // Run UV loop
    uv_run(&env.loop, UV_RUN_DEFAULT);

    // Cleanup Env
    pomelo_example_env_finalize(&env);
    return 0;
}


/// @brief Generate connect token for clients
void pomelo_example_generate_token(uint8_t * connect_token) {
    (void) env;
    uint64_t now = time(NULL) * 1000ULL;

    pomelo_connect_token_t token;
    memset(&token, 0, sizeof(pomelo_connect_token_t));
    token.protocol_id = protocol_id;
    token.create_timestamp = now;
    token.expire_timestamp = now + EXPIRE * 1000ULL;
    pomelo_random_buffer(
        token.connect_token_nonce,
        sizeof(token.connect_token_nonce)
    );
    token.timeout = TIMEOUT;
    token.naddresses = 1;

    char address_str[POMELO_ADDRESS_STRING_BUFFER_CAPACITY];
    snprintf(
        address_str,
        sizeof(address_str),
        "%s:%d",
        ADDRESS_HOST,
        ADDRESS_PORT
    );
    int ret = pomelo_address_from_string(&token.addresses[0], address_str);
    pomelo_assert(ret == 0);
    pomelo_random_buffer(
        token.client_to_server_key,
        sizeof(token.client_to_server_key)
    );
    pomelo_random_buffer(
        token.server_to_client_key,
        sizeof(token.server_to_client_key)
    );

    token.client_id = ++client_id;
    memset(token.user_data, 0, sizeof(token.user_data));

    ret = pomelo_connect_token_encode(connect_token, &token, private_key);
    pomelo_assert(ret == 0);
    printf(
        "Generate connect token for client ID = %" PRId64 "\n",
        token.client_id
    );
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

    // Reply it
    pomelo_message_t * reply = pomelo_context_acquire_message(env.context);
    for (size_t i = 0; i < size; i++) {
        uint8_t value = 0;
        int ret = pomelo_message_read_uint8(message, &value);
        pomelo_assert(ret == 0);
        pomelo_message_write_uint8(reply, value);
    }

    pomelo_session_send(session, 0, reply, NULL);
    pomelo_message_unref(reply);
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
/*                          HTTP connect token server                         */
/* -------------------------------------------------------------------------- */

typedef struct {
    uv_tcp_t client;
    char last_char;
    bool header_end;
    uv_write_t write;
} http_server_state_t;


static uv_tcp_t http_server;


void http_server_on_closed(uv_handle_t * handle) {
    // handle is state
    pomelo_allocator_free(env.allocator, handle);
}


static void http_server_send_done(uv_write_t * req, int status) {
    // Close the connection
    pomelo_assert(status == 0);
    uv_close((uv_handle_t *) req->handle, http_server_on_closed);
}


void http_server_on_data(
    http_server_state_t * state,
    char * data,
    size_t length
) {
    // Skip the header
    if (state->header_end) return;
    size_t i = 0;
    while (!state->header_end && i < length) {
        char c = data[i++];
        if (c == '\r') continue;
        if (c == '\n' && state->last_char == '\n') {
            state->header_end = true;
        }
        state->last_char = c;
    }
    if (!state->header_end) return;

    // Send response
    time_t now = time(NULL);

    uint8_t connect_token[POMELO_CONNECT_TOKEN_BYTES];
    pomelo_example_generate_token(connect_token);

    char connect_token_b64[
        pomelo_base64_calc_encoded_length(POMELO_CONNECT_TOKEN_BYTES)
    ];
    connect_token_b64[0] = '\0';

    int ret = pomelo_base64_encode(
        connect_token_b64,
        sizeof(connect_token_b64),
        connect_token,
        sizeof(connect_token)
    );
    pomelo_assert(ret == 0);

    uv_buf_t buf[3];
    char header_0[128];
    char header_1[128];

    buf[0].base = header_0;
    buf[0].len = (BUFFER_LENGTH_TYPE) snprintf(
        header_0,
        sizeof(header_0),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *",
        sizeof(connect_token_b64) - 1
    );

    buf[1].base = header_1;
    buf[1].len = (BUFFER_LENGTH_TYPE) strftime(
        header_1,
        sizeof(header_1),
        "Date: %a, %d %h %Y %T GMT\r\n"
        "\r\n",
        gmtime(&now)
    );

    buf[2].base = connect_token_b64;
    buf[2].len = sizeof(connect_token_b64) - 1;

    ret = uv_write(
        &state->write,
        (uv_stream_t *) &state->client,
        buf,
        sizeof(buf) / sizeof(buf[0]),
        http_server_send_done
    );
    pomelo_assert(ret == 0);
}


static void http_server_on_alloc(
    uv_handle_t * handle,
    size_t suggested_size,
    uv_buf_t * buf
) {
    (void) handle;
    buf->base = pomelo_allocator_malloc(env.allocator, suggested_size);
    buf->len = (BUFFER_LENGTH_TYPE) suggested_size;
}


static void http_server_on_read(
    uv_stream_t * stream,
    ssize_t nread,
    const uv_buf_t * buf
) {
    if (nread >= 0) {
        // Call the callback
        http_server_on_data(
            (http_server_state_t *) stream,
            buf->base,
            (size_t) nread
        );
    } else if (nread == UV_EOF) {
        // Disconnected, free client
        uv_close((uv_handle_t *) stream, http_server_on_closed);
    }

    // Free the data
    pomelo_allocator_free(env.allocator, buf->base);
}


static void http_server_callback(uv_stream_t * server, int status) {
    if (status < 0) {
        uv_close((uv_handle_t *) &http_server, NULL);
        return;
    }
    http_server_state_t * state = pomelo_allocator_malloc_t(
        env.allocator,
        http_server_state_t
    );

    pomelo_assert(state != NULL);
    memset(state, 0, sizeof(http_server_state_t));

    uv_tcp_t * client = &state->client;
    int ret = uv_tcp_init(&env.loop, client);
    pomelo_assert(ret == 0);

    ret = uv_accept(server, (uv_stream_t *) client);
    pomelo_assert(ret == 0);

    ret = uv_read_start(
        (uv_stream_t *) client,
        http_server_on_alloc,
        http_server_on_read
    );
    pomelo_assert(ret == 0);
}


static void http_server_serve_connect_token(void) {
    // Start HTTP server
    int ret = uv_tcp_init(&env.loop, &http_server);
    pomelo_assert(ret == 0);

    struct sockaddr_in addr;
    ret = uv_ip4_addr(SERVICE_HOST, SERVICE_PORT, &addr);
    pomelo_assert(ret == 0);

    ret = uv_tcp_bind(&http_server, (struct sockaddr *) &addr, 0);
    pomelo_assert(ret == 0);

    ret = uv_listen(
        (uv_stream_t *) &http_server,
        HTTP_BACKLOG,
        http_server_callback
    );
    pomelo_assert(ret == 0);

    printf(
        "HTTP server is listening on http://%s:%d\n",
        SERVICE_HOST,
        SERVICE_PORT
    );
}
