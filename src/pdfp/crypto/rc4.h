
#ifndef H_PDFP_CRYPTO_RC4
#define H_PDFP_CRYPTO_RC4

#include <cstdint>

namespace pdfp {

struct RC4_KEY
{
	int x, y, m[256];
};

void RC4_set_key( RC4_KEY *, int, uint8_t * );
void RC4( RC4_KEY *, int, const uint8_t *, uint8_t * );

}

#endif
