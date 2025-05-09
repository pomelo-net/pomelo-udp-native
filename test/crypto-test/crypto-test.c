#include <string.h>
#include "pomelo-test.h"
#include "pomelo/random.h"
#include "crypto/crypto.h"


static uint64_t sequence;
static uint8_t nonce[12];
static uint8_t key[32];
static uint8_t ad[16];


static char raw_msg[] = "Hello Crypto!";
static char decrypted_msg[sizeof(raw_msg)];
static char encrypted_msg[sizeof(raw_msg) + POMELO_CRYPTO_AEAD_HMAC_BYTES];


int main(void) {
    printf("Crypto test\n");
    pomelo_check(pomelo_crypto_init() == 0);

    pomelo_random_buffer(key, sizeof(key));
    pomelo_random_buffer(ad, sizeof(ad));
    pomelo_random_buffer(&sequence, sizeof(sequence));

    pomelo_crypto_make_nonce(nonce, sizeof(nonce), sequence);

    printf("Encrypting message\n");
    size_t output_length = 0;
    int ret = pomelo_crypto_encrypt_aead(
        (uint8_t *) encrypted_msg,
        &output_length,
        (uint8_t *) raw_msg,
        sizeof(raw_msg),
        key,
        nonce,
        ad,
        sizeof(ad)
    );
    pomelo_check(ret == 0);
    pomelo_check(
        output_length == sizeof(raw_msg) + POMELO_CRYPTO_AEAD_HMAC_BYTES
    );

    printf("Decrypting message\n");
    pomelo_crypto_decrypt_aead(
        (uint8_t *) decrypted_msg,
        &output_length,
        (uint8_t *) encrypted_msg,
        sizeof(encrypted_msg),
        key,
        nonce,
        ad,
        sizeof(ad)
    );
    pomelo_check(ret == 0);
    pomelo_check(output_length == sizeof(raw_msg));
    pomelo_check(memcmp(raw_msg, decrypted_msg, sizeof(raw_msg)) == 0);

    printf("*** All crypto tests passed ***\n");
    return 0;
}
