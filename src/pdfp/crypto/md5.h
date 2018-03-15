
#ifndef H_PDFP_CRYPTO_MD5
#define H_PDFP_CRYPTO_MD5

#include <cstdint>

namespace pdfp {

const int MD5_DIGEST_LENGTH = 16;

struct MD5_CTX
{
	uint32_t buf[4];
	uint32_t i[2];
	uint8_t in[64];
};

void MD5_Init( MD5_CTX * );
void MD5_Update( MD5_CTX *, const uint8_t *, int );
void MD5_Final( uint8_t *, MD5_CTX * );

}

#endif
