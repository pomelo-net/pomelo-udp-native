cmake_minimum_required(VERSION 3.10)

project(sodium)

set(LIB_SODIUM_PATH ../libsodium)
set(LIB_SODIUM_SRC_PATH ${LIB_SODIUM_PATH}/src/libsodium)

set(INCLUDE_SODIUM
    ${LIB_SODIUM_SRC_PATH}/include
    ${LIB_SODIUM_SRC_PATH}/include/sodium
)


set(SRC_SODIUM
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis128l/aead_aegis128l.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis128l/aegis128l_aesni.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis128l/aegis128l_armcrypto.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis128l/aegis128l_soft.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis256/aead_aegis256.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis256/aegis256_armcrypto.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis256/aegis256_aesni.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aegis256/aegis256_soft.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/aes256gcm/aesni/aead_aes256gcm_aesni.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/chacha20poly1305/aead_chacha20poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_aead/xchacha20poly1305/aead_xchacha20poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_auth/crypto_auth.c
    ${LIB_SODIUM_SRC_PATH}/crypto_auth/hmacsha256/auth_hmacsha256.c
    ${LIB_SODIUM_SRC_PATH}/crypto_auth/hmacsha512/auth_hmacsha512.c
    ${LIB_SODIUM_SRC_PATH}/crypto_auth/hmacsha512256/auth_hmacsha512256.c
    ${LIB_SODIUM_SRC_PATH}/crypto_box/crypto_box.c
    ${LIB_SODIUM_SRC_PATH}/crypto_box/crypto_box_easy.c
    ${LIB_SODIUM_SRC_PATH}/crypto_box/crypto_box_seal.c
    ${LIB_SODIUM_SRC_PATH}/crypto_box/curve25519xsalsa20poly1305/box_curve25519xsalsa20poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/ed25519/ref10/ed25519_ref10.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/hchacha20/core_hchacha20.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/hsalsa20/core_hsalsa20.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/hsalsa20/ref2/core_hsalsa20_ref2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/salsa/ref/core_salsa_ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_core/softaes/softaes.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/generichash_blake2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/blake2b-compress-avx2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/blake2b-compress-ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/blake2b-compress-sse41.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/blake2b-compress-ssse3.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/blake2b-ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/blake2b/ref/generichash_blake2b.c
    ${LIB_SODIUM_SRC_PATH}/crypto_generichash/crypto_generichash.c
    ${LIB_SODIUM_SRC_PATH}/crypto_hash/crypto_hash.c
    ${LIB_SODIUM_SRC_PATH}/crypto_hash/sha256/cp/hash_sha256_cp.c
    ${LIB_SODIUM_SRC_PATH}/crypto_hash/sha256/hash_sha256.c
    ${LIB_SODIUM_SRC_PATH}/crypto_hash/sha512/cp/hash_sha512_cp.c
    ${LIB_SODIUM_SRC_PATH}/crypto_hash/sha512/hash_sha512.c
    ${LIB_SODIUM_SRC_PATH}/crypto_kdf/blake2b/kdf_blake2b.c
    ${LIB_SODIUM_SRC_PATH}/crypto_kdf/crypto_kdf.c
    ${LIB_SODIUM_SRC_PATH}/crypto_kx/crypto_kx.c
    ${LIB_SODIUM_SRC_PATH}/crypto_onetimeauth/crypto_onetimeauth.c
    ${LIB_SODIUM_SRC_PATH}/crypto_onetimeauth/poly1305/donna/poly1305_donna.c
    ${LIB_SODIUM_SRC_PATH}/crypto_onetimeauth/poly1305/onetimeauth_poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_onetimeauth/poly1305/sse2/poly1305_sse2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-core.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-encoding.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-fill-block-avx2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-fill-block-avx512f.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-fill-block-ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2-fill-block-ssse3.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/argon2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/blake2b-long.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/pwhash_argon2i.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/argon2/pwhash_argon2id.c
    ${LIB_SODIUM_SRC_PATH}/crypto_pwhash/crypto_pwhash.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/crypto_scalarmult.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/curve25519/ref10/x25519_ref10.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/curve25519/sandy2x/curve25519_sandy2x.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/curve25519/sandy2x/fe51_invert.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/curve25519/sandy2x/fe_frombytes_sandy2x.c
    ${LIB_SODIUM_SRC_PATH}/crypto_scalarmult/curve25519/scalarmult_curve25519.c
    ${LIB_SODIUM_SRC_PATH}/crypto_secretbox/crypto_secretbox.c
    ${LIB_SODIUM_SRC_PATH}/crypto_secretbox/crypto_secretbox_easy.c
    ${LIB_SODIUM_SRC_PATH}/crypto_secretbox/xsalsa20poly1305/secretbox_xsalsa20poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_secretstream/xchacha20poly1305/secretstream_xchacha20poly1305.c
    ${LIB_SODIUM_SRC_PATH}/crypto_shorthash/crypto_shorthash.c
    ${LIB_SODIUM_SRC_PATH}/crypto_shorthash/siphash24/ref/shorthash_siphash24_ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_shorthash/siphash24/shorthash_siphash24.c
    ${LIB_SODIUM_SRC_PATH}/crypto_sign/crypto_sign.c
    ${LIB_SODIUM_SRC_PATH}/crypto_sign/ed25519/ref10/keypair.c
    ${LIB_SODIUM_SRC_PATH}/crypto_sign/ed25519/ref10/open.c
    ${LIB_SODIUM_SRC_PATH}/crypto_sign/ed25519/ref10/sign.c
    ${LIB_SODIUM_SRC_PATH}/crypto_sign/ed25519/sign_ed25519.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-avx2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-ssse3.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/chacha20/ref/chacha20_ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/chacha20/stream_chacha20.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/crypto_stream.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/salsa20/ref/salsa20_ref.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/salsa20/stream_salsa20.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/salsa20/xmm6/salsa20_xmm6.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-avx2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-sse2.c
    ${LIB_SODIUM_SRC_PATH}/crypto_stream/xsalsa20/stream_xsalsa20.c
    ${LIB_SODIUM_SRC_PATH}/crypto_verify/verify.c
    ${LIB_SODIUM_SRC_PATH}/randombytes/internal/randombytes_internal_random.c
    ${LIB_SODIUM_SRC_PATH}/randombytes/randombytes.c
    ${LIB_SODIUM_SRC_PATH}/randombytes/sysrandom/randombytes_sysrandom.c
    ${LIB_SODIUM_SRC_PATH}/sodium/codecs.c
    ${LIB_SODIUM_SRC_PATH}/sodium/core.c
    ${LIB_SODIUM_SRC_PATH}/sodium/runtime.c
    ${LIB_SODIUM_SRC_PATH}/sodium/utils.c
)


# Sodium
add_library(sodium STATIC ${SRC_SODIUM})
target_include_directories(sodium PUBLIC ${INCLUDE_SODIUM})
target_compile_definitions(sodium PUBLIC SODIUM_STATIC=1 DEV_MODE=0 CONFIGURED=1)
