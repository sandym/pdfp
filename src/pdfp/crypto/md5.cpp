
#include "md5.h"

namespace {

uint8_t PADDING[64] = {
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* F, G, H and I are basic MD5 functions */
#define F( x, y, z ) ( ( ( x ) & ( y ) ) | ( ( ~x ) & ( z ) ) )
#define G( x, y, z ) ( ( ( x ) & ( z ) ) | ( ( y ) & ( ~z ) ) )
#define H( x, y, z ) ( ( x ) ^ ( y ) ^ ( z ) )
#define I( x, y, z ) ( ( y ) ^ ( ( x ) | ( ~z ) ) )

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT( x, n ) ( ( ( x ) << ( n ) ) | ( ( x ) >> ( 32 - ( n ) ) ) )

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF( a, b, c, d, x, s, ac )                                      \
	{                                                                   \
		( a ) += F( ( b ), ( c ), ( d ) ) + ( x ) + ( uint32_t )( ac ); \
		( a ) = ROTATE_LEFT( ( a ), ( s ) );                            \
		( a ) += ( b );                                                 \
	}
#define GG( a, b, c, d, x, s, ac )                                      \
	{                                                                   \
		( a ) += G( ( b ), ( c ), ( d ) ) + ( x ) + ( uint32_t )( ac ); \
		( a ) = ROTATE_LEFT( ( a ), ( s ) );                            \
		( a ) += ( b );                                                 \
	}
#define HH( a, b, c, d, x, s, ac )                                      \
	{                                                                   \
		( a ) += H( ( b ), ( c ), ( d ) ) + ( x ) + ( uint32_t )( ac ); \
		( a ) = ROTATE_LEFT( ( a ), ( s ) );                            \
		( a ) += ( b );                                                 \
	}
#define II( a, b, c, d, x, s, ac )                                      \
	{                                                                   \
		( a ) += I( ( b ), ( c ), ( d ) ) + ( x ) + ( uint32_t )( ac ); \
		( a ) = ROTATE_LEFT( ( a ), ( s ) );                            \
		( a ) += ( b );                                                 \
	}

void Transform( uint32_t *buf, uint32_t *in )
{
	uint32_t a = buf[0], b = buf[1], c = buf[2], d = buf[3];

// Round 1
#define S11 7
#define S12 12
#define S13 17
#define S14 22
	FF( a, b, c, d, in[0], S11, 0xD76AA478L ); /* 1 */
	FF( d, a, b, c, in[1], S12, 0xE8C7B756L ); /* 2 */
	FF( c, d, a, b, in[2], S13, 0x242070DBL ); /* 3 */
	FF( b, c, d, a, in[3], S14, 0xC1BDCEEEL ); /* 4 */
	FF( a, b, c, d, in[4], S11, 0xF57C0FAFL ); /* 5 */
	FF( d, a, b, c, in[5], S12, 0x4787C62AL ); /* 6 */
	FF( c, d, a, b, in[6], S13, 0xA8304613L ); /* 7 */
	FF( b, c, d, a, in[7], S14, 0xFD469501L ); /* 8 */
	FF( a, b, c, d, in[8], S11, 0x698098D8L ); /* 9 */
	FF( d, a, b, c, in[9], S12, 0x8B44F7AFL ); /* 10 */
	FF( c, d, a, b, in[10], S13, 0xFFFF5BB1L ); /* 11 */
	FF( b, c, d, a, in[11], S14, 0x895CD7BEL ); /* 12 */
	FF( a, b, c, d, in[12], S11, 0x6B901122L ); /* 13 */
	FF( d, a, b, c, in[13], S12, 0xFD987193L ); /* 14 */
	FF( c, d, a, b, in[14], S13, 0xA679438EL ); /* 15 */
	FF( b, c, d, a, in[15], S14, 0x49B40821L ); /* 16 */

// Round 2
#define S21 5
#define S22 9
#define S23 14
#define S24 20
	GG( a, b, c, d, in[1], S21, 0xF61E2562L ); /* 17 */
	GG( d, a, b, c, in[6], S22, 0xC040B340L ); /* 18 */
	GG( c, d, a, b, in[11], S23, 0x265E5A51L ); /* 19 */
	GG( b, c, d, a, in[0], S24, 0xE9B6C7AAL ); /* 20 */
	GG( a, b, c, d, in[5], S21, 0xD62F105DL ); /* 21 */
	GG( d, a, b, c, in[10], S22, 0x02441453L ); /* 22 */
	GG( c, d, a, b, in[15], S23, 0xD8A1E681L ); /* 23 */
	GG( b, c, d, a, in[4], S24, 0xE7D3FBC8L ); /* 24 */
	GG( a, b, c, d, in[9], S21, 0x21E1CDE6L ); /* 25 */
	GG( d, a, b, c, in[14], S22, 0xC33707D6L ); /* 26 */
	GG( c, d, a, b, in[3], S23, 0xF4D50D87L ); /* 27 */
	GG( b, c, d, a, in[8], S24, 0x455A14EDL ); /* 28 */
	GG( a, b, c, d, in[13], S21, 0xA9E3E905L ); /* 29 */
	GG( d, a, b, c, in[2], S22, 0xFCEFA3F8L ); /* 30 */
	GG( c, d, a, b, in[7], S23, 0x676F02D9L ); /* 31 */
	GG( b, c, d, a, in[12], S24, 0x8D2A4C8AL ); /* 32 */

// Round 3
#define S31 4
#define S32 11
#define S33 16
#define S34 23
	HH( a, b, c, d, in[5], S31, 0xFFFA3942L ); /* 33 */
	HH( d, a, b, c, in[8], S32, 0x8771F681L ); /* 34 */
	HH( c, d, a, b, in[11], S33, 0x6D9D6122L ); /* 35 */
	HH( b, c, d, a, in[14], S34, 0xFDE5380CL ); /* 36 */
	HH( a, b, c, d, in[1], S31, 0xA4BEEA44L ); /* 37 */
	HH( d, a, b, c, in[4], S32, 0x4BDECFA9L ); /* 38 */
	HH( c, d, a, b, in[7], S33, 0xF6BB4B60L ); /* 39 */
	HH( b, c, d, a, in[10], S34, 0xBEBFBC70L ); /* 40 */
	HH( a, b, c, d, in[13], S31, 0x289B7EC6L ); /* 41 */
	HH( d, a, b, c, in[0], S32, 0xEAA127FAL ); /* 42 */
	HH( c, d, a, b, in[3], S33, 0xD4EF3085L ); /* 43 */
	HH( b, c, d, a, in[6], S34, 0x04881D05L ); /* 44 */
	HH( a, b, c, d, in[9], S31, 0xD9D4D039L ); /* 45 */
	HH( d, a, b, c, in[12], S32, 0xE6DB99E5L ); /* 46 */
	HH( c, d, a, b, in[15], S33, 0x1FA27CF8L ); /* 47 */
	HH( b, c, d, a, in[2], S34, 0xC4AC5665L ); /* 48 */

// Round 4
#define S41 6
#define S42 10
#define S43 15
#define S44 21
	II( a, b, c, d, in[0], S41, 0xF4292244L ); /* 49 */
	II( d, a, b, c, in[7], S42, 0x432AFF97L ); /* 50 */
	II( c, d, a, b, in[14], S43, 0xAB9423A7L ); /* 51 */
	II( b, c, d, a, in[5], S44, 0xFC93A039L ); /* 52 */
	II( a, b, c, d, in[12], S41, 0x655B59C3L ); /* 53 */
	II( d, a, b, c, in[3], S42, 0x8F0CCC92L ); /* 54 */
	II( c, d, a, b, in[10], S43, 0xFFEFF47DL ); /* 55 */
	II( b, c, d, a, in[1], S44, 0x85845DD1L ); /* 56 */
	II( a, b, c, d, in[8], S41, 0x6FA87E4FL ); /* 57 */
	II( d, a, b, c, in[15], S42, 0xFE2CE6E0L ); /* 58 */
	II( c, d, a, b, in[6], S43, 0xA3014314L ); /* 59 */
	II( b, c, d, a, in[13], S44, 0x4E0811A1L ); /* 60 */
	II( a, b, c, d, in[4], S41, 0xF7537E82L ); /* 61 */
	II( d, a, b, c, in[11], S42, 0xBD3AF235L ); /* 62 */
	II( c, d, a, b, in[2], S43, 0x2AD7D2BBL ); /* 63 */
	II( b, c, d, a, in[9], S44, 0xEB86D391L ); /* 64 */

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}
}

namespace pdfp {

void MD5_Init( MD5_CTX *ctx )
{
	ctx->i[0] = ctx->i[1] = 0;

	/* Load magic initialization constants.
	 */
	ctx->buf[0] = (uint32_t)0x67452301L;
	ctx->buf[1] = (uint32_t)0xefcdab89L;
	ctx->buf[2] = (uint32_t)0x98badcfeL;
	ctx->buf[3] = (uint32_t)0x10325476L;
}

void MD5_Update( MD5_CTX *ctx, const uint8_t *i_buf, int i_len )
{
	// compute number of bytes mod 64
	int mdi = (int)( ( ctx->i[0] >> 3 ) & 0x3F );

	// update number of bits
	if ( ( ctx->i[0] + ( (uint32_t)i_len << 3 ) ) < ctx->i[0] )
		ctx->i[1]++;
	ctx->i[0] += ( (uint32_t)i_len << 3 );
	ctx->i[1] += ( (uint32_t)i_len >> 29 );

	uint32_t in[16];
	while ( i_len-- )
	{
		// add new character to buffer, increment mdi
		ctx->in[mdi++] = *i_buf++;

		// transform if necessary
		if ( mdi == 0x40 )
		{
			for ( int i = 0, ii = 0; i < 16; i++, ii += 4 )
			{
				in[i] = ( ( (uint32_t)ctx->in[ii + 3] ) << 24 ) |
				        ( ( (uint32_t)ctx->in[ii + 2] ) << 16 ) |
				        ( ( (uint32_t)ctx->in[ii + 1] ) << 8 ) |
				        ( (uint32_t)ctx->in[ii] );
			}
			Transform( ctx->buf, in );
			mdi = 0;
		}
	}
}

void MD5_Final( uint8_t *digest, MD5_CTX *ctx )
{
	// save number of bits
	uint32_t in[16];
	in[14] = ctx->i[0];
	in[15] = ctx->i[1];

	// compute number of bytes mod 64
	int mdi = (int)( ( ctx->i[0] >> 3 ) & 0x3F );

	/* pad out to 56 mod 64 */
	unsigned int padLen = ( mdi < 56 ) ? ( 56 - mdi ) : ( 120 - mdi );
	MD5_Update( ctx, PADDING, padLen );

	// append length in bits and transform
	for ( int i = 0, ii = 0; i < 14; i++, ii += 4 )
	{
		in[i] = ( ( (uint32_t)ctx->in[ii + 3] ) << 24 ) |
		        ( ( (uint32_t)ctx->in[ii + 2] ) << 16 ) |
		        ( ( (uint32_t)ctx->in[ii + 1] ) << 8 ) |
		        ( (uint32_t)ctx->in[ii] );
	}
	Transform( ctx->buf, in );

	// store buffer in digest
	for ( int i = 0, ii = 0; i < 4; i++, ii += 4 )
	{
		digest[ii] = ( uint8_t )( ctx->buf[i] & 0xFF );
		digest[ii + 1] = ( uint8_t )( ( ctx->buf[i] >> 8 ) & 0xFF );
		digest[ii + 2] = ( uint8_t )( ( ctx->buf[i] >> 16 ) & 0xFF );
		digest[ii + 3] = ( uint8_t )( ( ctx->buf[i] >> 24 ) & 0xFF );
	}
}

}
