#ifndef POMELO_RANDOM_H
#define POMELO_RANDOM_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Random value for a buffer
/// @param buffer Output buffer
/// @param length Length of buffer
void pomelo_random_buffer(void * buffer, size_t length);


/// @brief Random value for a buffer with a deterministic seed
/// @param buffer Output buffer
/// @param length Length of buffer
/// @param seed Seed value
void pomelo_random_buffer_deterministic(
    void * buffer,
    size_t length,
    uint64_t seed
);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POMELO_RANDOM_H
