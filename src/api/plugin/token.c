#include <assert.h>
#include <string.h>
#include "pomelo/errno.h"
#include "pomelo/token.h"
#include "api/socket.h"
#include "codec/packet.h"
#include "token.h"


int POMELO_PLUGIN_CALL pomelo_plugin_token_connect_token_decode(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    uint8_t * connect_token,
    pomelo_plugin_token_info_t * token_info
) {
    assert(plugin != NULL);
    assert(socket != NULL);
    assert(connect_token != NULL);
    assert(token_info != NULL);

    if (!plugin || !socket || !connect_token || !token_info) {
        return POMELO_ERR_PLUGIN_INVALID_ARG;
    }

    pomelo_connect_token_t token;
    memset(&token, 0, sizeof(pomelo_connect_token_t));
    int ret = pomelo_connect_token_decode_public(connect_token, &token);
    if (ret < 0) return ret;

    ret = pomelo_connect_token_decode_private(
        connect_token + POMELO_CONNECT_TOKEN_PRIVATE_OFFSET,
        &token,
        socket->private_key
    );
    if (ret < 0) return ret;

    if (token_info->protocol_id) {
        *token_info->protocol_id = token.protocol_id;
    }

    if (token_info->create_timestamp) {
        *token_info->create_timestamp = token.create_timestamp;
    }

    if (token_info->expire_timestamp) {
        *token_info->expire_timestamp = token.expire_timestamp;
    }

    if (token_info->connect_token_nonce) {
        memcpy(
            token_info->connect_token_nonce,
            token.connect_token_nonce,
            POMELO_CONNECT_TOKEN_NONCE_BYTES
        );
    }
    
    if (token_info->timeout) {
        *token_info->timeout = token.timeout;
    }

    if (token_info->naddresses) {
        *token_info->naddresses = token.naddresses;
    }

    if (token_info->addresses) {
        for (int i = 0; i < token.naddresses; i++) {
            token_info->addresses[i] = token.addresses[i];
        }
    }

    if (token_info->client_to_server_key) {
        memcpy(
            token_info->client_to_server_key,
            token.client_to_server_key,
            POMELO_KEY_BYTES
        );
    }

    if (token_info->server_to_client_key) {
        memcpy(
            token_info->server_to_client_key,
            token.server_to_client_key,
            POMELO_KEY_BYTES
        );
    }

    if (token_info->client_id) {
        *token_info->client_id = token.client_id;
    }

    if (token_info->user_data) {
        memcpy(
            token_info->user_data,
            token.user_data,
            POMELO_USER_DATA_BYTES
        );
    }

    return 0;
}
