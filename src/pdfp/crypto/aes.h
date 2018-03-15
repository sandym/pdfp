
#ifndef H_PDFP_CRYPTO_AES
#define H_PDFP_CRYPTO_AES

#include <cstdint>

namespace pdfp {

const int AES_BLOCK_SIZE = 16;

struct AES_KEY
{
	uint32_t nr; // rounds
	uint32_t buf[68]; // store round_keys, each block is 4 bytes
};

void AES_set_decrypt_key( const unsigned char *, int, AES_KEY * );
void AES_decrypt( const uint8_t *in, uint8_t *out, const AES_KEY *key );
void AES_encrypt( const uint8_t *in, uint8_t *out, const AES_KEY *key );

}

#endif
