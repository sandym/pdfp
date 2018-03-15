
#include "rc4.h"

namespace pdfp {

void RC4_set_key( RC4_KEY *s, int length, uint8_t *key )
{
	s->x = 0;
	s->y = 0;

	auto m = s->m;
	for ( int i = 0; i < 256; ++i )
		m[i] = i;

	int j = 0, k = 0;

	for ( int i = 0; i < 256; ++i )
	{
		int a = m[i];
		j = (unsigned char)( j + a + key[k] );
		m[i] = m[j];
		m[j] = a;
		if ( ++k >= length )
			k = 0;
	}
}

void RC4( RC4_KEY *s, int length, const uint8_t *i_data, uint8_t *o_data )
{
	int x = s->x;
	int y = s->y;
	auto m = s->m;

	for ( int i = 0; i < length; ++i )
	{
		x = (unsigned char)( x + 1 );
		int a = m[x];
		y = (unsigned char)( y + a );
		int b = m[y];
		m[x] = b;
		m[y] = a;
		o_data[i] = i_data[i] ^ m[(unsigned char)( a + b )];
	}

	s->x = x;
	s->y = y;
}

}

