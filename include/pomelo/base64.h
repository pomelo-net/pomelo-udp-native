#ifndef POMELO_BASE64_H
#define POMELO_BASE64_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/// @brief Encoding variant of base64
/// Source: sodium_base64_VARIANT_URLSAFE
#define pomelo_base64_encoded_variant 5

/// @brief Encoding variant of base64 (no padding)
/// Source: sodium_base64_VARIANT_URLSAFE_NO_PADDING
#define pomelo_base64_encoded_variant_no_padding 7


/// @brief Calculate base64 encoded buffer length
/// Source: sodium_base64_ENCODED_LEN
#define pomelo_base64_calc_encoded_length(bin_len)                             \
    (((bin_len) / 3U) * 4U +                                                   \
    ((((bin_len) - ((bin_len) / 3U) * 3U) |                                    \
    (((bin_len) - ((bin_len) / 3U) * 3U) >> 1)) & 1U) *                        \
    (4U - (~((((pomelo_base64_encoded_variant) & 2U) >> 1) - 1U) &             \
    (3U - ((bin_len) - ((bin_len) / 3U) * 3U)))) + 1U)


/// @brief Calculate base64 encoded buffer length (no padding)
/// Source: sodium_base64_ENCODED_LEN
#define pomelo_base64_calc_encoded_no_padding_length(bin_len)                 \
    (((bin_len) / 3U) * 4U +                                                   \
    ((((bin_len) - ((bin_len) / 3U) * 3U) |                                    \
    (((bin_len) - ((bin_len) / 3U) * 3U) >> 1)) & 1U) *                        \
    (4U - (~((((pomelo_base64_encoded_variant_no_padding) & 2U) >> 1) - 1U) &             \
    (3U - ((bin_len) - ((bin_len) / 3U) * 3U)))) + 1U)


/// @brief Encode base64 with PADDING and URLSAFE variant
/// @param b64 Base64 output string
/// @param b64_capacity Capacity of base64 string buffer (include NULL trailing)
/// @param bin Input binary buffer
/// @param bin_size Size of binary buffer
/// @return 0 on success or an error code < 0 on failure
int pomelo_base64_encode(
    char * b64,
    size_t b64_capacity,
    const uint8_t * bin,
    size_t bin_size
);


/// @brief Decode base64 with URLSAFE variant (with or without PADDING)
/// @param bin Output binary buffer
/// @param bin_capacity Capacity of binary buffer
/// @param b64 Base64 string
/// @param b64_len Length of base64 string (Not include NULL trail)
/// @return 0 on success or an error code < 0 on failure
int pomelo_base64_decode(
    uint8_t * bin,
    size_t bin_capacity,
    const char * b64,
    size_t b64_len
);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POMELO_BASE64_H
