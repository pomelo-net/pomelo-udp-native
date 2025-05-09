#ifndef POMELO_GENERATOR_ARGS_SRC_H
#define POMELO_GENERATOR_ARGS_SRC_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

struct pomelo_arg_descriptor_s {
    /// @brief Short version of argument
    const char * arg_short;

    /// @brief Long version of argument
    const char * arg_long;
};

struct pomelo_arg_vector_s {
    /// @brief True if argument is present
    bool present;

    /// @brief Begin index of argument chain (inclusive)
    int begin;

    /// @brief End index of argument chain (inclusive)
    int end;
};

/// @brief Argument descriptor. This describes the argument chain
typedef struct pomelo_arg_descriptor_s pomelo_arg_descriptor_t;

/// @brief Argument vector. This defines the argument chain
typedef struct pomelo_arg_vector_s pomelo_arg_vector_t;


/// @brief Process the arguments
void pomelo_arg_process(
    int argc,
    char * argv[],
    pomelo_arg_descriptor_t * descriptors,
    pomelo_arg_vector_t * vectors,
    int ndescriptors
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_UTILS_ARGS_SRC_H
