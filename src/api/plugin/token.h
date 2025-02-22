#ifndef POMELO_API_PLUGIN_TOKEN_SRC_H
#define POMELO_API_PLUGIN_TOKEN_SRC_H
#include "plugin.h"


#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */


int POMELO_PLUGIN_CALL pomelo_plugin_token_connect_token_decode(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    uint8_t * connect_token,
    pomelo_plugin_token_info_t * token_info
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_PLUGIN_TOKEN_SRC_H
