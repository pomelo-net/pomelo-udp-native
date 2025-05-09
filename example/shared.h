#ifndef POMELO_EXAMPLE_SHARED_H
#define POMELO_EXAMPLE_SHARED_H
#include "pomelo.h"
#include "pomelo/random.h"
#include "pomelo/base64.h"
#include "pomelo/platforms/platform-uv.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG
#define pomelo_assert(expression) if (!(expression)) abort()
#else
#define pomelo_assert assert
#endif

#ifdef _WIN32
#define BUFFER_LENGTH_TYPE ULONG
#else // Unix
#define BUFFER_LENGTH_TYPE size_t
#endif


#define ADDRESS_HOST "127.0.0.1"
#define ADDRESS_PORT 8888

#define SERVICE_HOST "127.0.0.1"
#define SERVICE_PORT 8889


/// @brief Environment of example
typedef struct pomelo_example_env_s pomelo_example_env_t;


struct pomelo_example_env_s {
    /// @brief UV loop  
    uv_loop_t loop;

    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief Platform
    pomelo_platform_t * platform;

    /// @brief Context
    pomelo_context_t * context;

    /// @brief Allocated bytes of allocator when this env is created
    uint64_t allocator_bytes_begin;

    /// @brief Socket
    pomelo_socket_t * socket;
};


/// @brief Initialize environment
void pomelo_example_env_init(
    pomelo_example_env_t * env,
    const char * plugin_path
);


/// @brief Finalize environment
void pomelo_example_env_finalize(pomelo_example_env_t * env);


#ifdef __cplusplus
}
#endif
#endif // POMELO_EXAMPLE_SHARED_H
