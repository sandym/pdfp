
#include "aes.h"
#include <string.h>
#include <cassert>

namespace {

const uint8_t S_BOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
    0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
    0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed,
    0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
    0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
    0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
    0xb0, 0x54, 0xbb, 0x16};

const uint8_t INV_S_BOX[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e,
    0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
    0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32,
    0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49,
    0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50,
    0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05,
    0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
    0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91, 0x11, 0x41,
    0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8,
    0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
    0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b,
    0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59,
    0x27, 0x80, 0xec, 0x5f, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
    0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d,
    0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63,
    0x55, 0x21, 0x0c, 0x7d};

const uint8_t INV_MIX[4][4] = {{0x0e, 0x0b, 0x0d, 0x09},
                               {0x09, 0x0e, 0x0b, 0x0d},
                               {0x0d, 0x09, 0x0e, 0x0b},
                               {0x0b, 0x0d, 0x09, 0x0e}};
const uint32_t RCON[10] = {0x00000001,
                           0x00000002,
                           0x00000004,
                           0x00000008,
                           0x00000010,
                           0x00000020,
                           0x00000040,
                           0x00000080,
                           0x0000001B,
                           0x00000036};

inline uint32_t GET_UINT32( const uint8_t *b, uint32_t i )
{
	return ( ( uint32_t )( b )[i] ) | ( ( uint32_t )( b )[i + 1] << 8 ) |
	       ( ( uint32_t )( b )[i + 2] << 16 ) |
	       ( ( uint32_t )( b )[i + 3] << 24 );
}
inline uint32_t ROTL8( uint32_t x )
{
	return ( ( x << 24 ) | ( x >> 8 ) );
}
inline uint32_t ROTL16( uint32_t x )
{
	return ( ( x << 16 ) | ( x >> 16 ) );
}
inline uint32_t ROTL24( uint32_t x )
{
	return ( ( x << 8 ) | ( x >> 24 ) );
}

inline uint32_t SUB_WORD( uint32_t x )
{
	return ( ( (uint32_t)S_BOX[x & 0xFF] ) |
	         ( (uint32_t)S_BOX[( x >> 8 ) & 0xFF] << 8 ) |
	         ( (uint32_t)S_BOX[( x >> 16 ) & 0xFF] << 16 ) |
	         ( (uint32_t)S_BOX[( x >> 24 ) & 0xFF] << 24 ) );
}

void transport( uint8_t state[pdfp::AES_BLOCK_SIZE] )
{
	uint8_t _state[4][4];
	for ( int r = 0; r < 4; ++r )
		for ( int c = 0; c < 4; ++c )
			_state[r][c] = state[( c << 2 ) + r];
	memcpy( state, _state, sizeof( _state ) );
}

void key_expansion( pdfp::AES_KEY *ctx, const uint8_t *key )
{
	uint32_t Nk = ctx->nr - 6;
	uint32_t Ek = ( ctx->nr + 1 ) << 2;
	uint32_t *RK = ctx->buf;

	uint32_t i = 0;
	do
	{
		RK[i] = GET_UINT32( key, i << 2 );
	} while ( ++i < Nk );
	do
	{
		uint32_t t = RK[i - 1];
		if ( ( i % Nk ) == 0 )
			t = SUB_WORD( ROTL8( t ) ) ^ RCON[i / Nk - 1];
		else if ( Nk == 8 && ( i % Nk ) == 4 )
			t = SUB_WORD( t );
		RK[i] = RK[i - Nk] ^ t;
	} while ( ++i < Ek );
}
inline void add_round_key( uint8_t state[pdfp::AES_BLOCK_SIZE],
                           const uint8_t key[pdfp::AES_BLOCK_SIZE] )
{
	for ( int i = 0; i < pdfp::AES_BLOCK_SIZE; ++i )
		state[i] ^= key[i];
}

void inv_sub_bytes( uint8_t state[pdfp::AES_BLOCK_SIZE] )
{
	for ( int i = 0; i < pdfp::AES_BLOCK_SIZE; ++i )
		state[i] = INV_S_BOX[state[i]];
}
inline void inv_shift_rows( uint8_t state[pdfp::AES_BLOCK_SIZE] )
{
	transport( state );
	*(uint32_t *)( state + 4 ) = ROTL24( *(uint32_t *)( state + 4 ) );
	*(uint32_t *)( state + 8 ) = ROTL16( *(uint32_t *)( state + 8 ) );
	*(uint32_t *)( state + 12 ) = ROTL8( *(uint32_t *)( state + 12 ) );
	transport( state );
}
uint8_t GF_256_multiply( uint8_t a, uint8_t b )
{
	uint8_t t[8] = {a};
	uint8_t ret = 0x00;
	for ( int i = 1; i < 8; ++i )
	{
		t[i] = t[i - 1] << 1;
		if ( t[i - 1] & 0x80 )
			t[i] ^= 0x1b;
	}
	for ( int i = 0; i < 8; ++i )
		ret ^= ( ( ( b >> i ) & 0x01 ) * t[i] );
	return ret;
}
void inv_mix_columns( uint8_t state[pdfp::AES_BLOCK_SIZE] )
{
	uint8_t _state[pdfp::AES_BLOCK_SIZE] = {0};
	for ( int r = 0; r < 4; ++r )
		for ( int c = 0; c < 4; ++c )
			for ( int i = 0; i < 4; ++i )
				_state[( c << 2 ) + r] ^=
				    GF_256_multiply( INV_MIX[r][i], state[( c << 2 ) + i] );
	memcpy( state, _state, sizeof( _state ) );
}

void aes_inv_round( uint8_t state[pdfp::AES_BLOCK_SIZE],
                    const uint8_t inv_rk[pdfp::AES_BLOCK_SIZE] )
{
	inv_shift_rows( state );
	inv_sub_bytes( state );
	add_round_key( state, inv_rk );
	inv_mix_columns( state );
}

void inv_final_round( uint8_t state[pdfp::AES_BLOCK_SIZE],
                      const uint8_t inv_rk[pdfp::AES_BLOCK_SIZE] )
{
	inv_shift_rows( state );
	inv_sub_bytes( state );
	add_round_key( state, inv_rk );
}
}

namespace pdfp {

void AES_set_decrypt_key( const unsigned char *key, int key_bit, AES_KEY *ctx )
{
	assert( ctx != nullptr );
	assert( key != nullptr );

	switch ( key_bit )
	{
		case 128:
			ctx->nr = 10;
			break;
		case 192:
			ctx->nr = 12;
			break;
		case 256:
			ctx->nr = 14;
			break;
		default:
			return;
	}
	key_expansion( ctx, key );
}

void AES_decrypt( const uint8_t *in, uint8_t *out, const AES_KEY *key )
{
	assert( key != nullptr );
	assert( out != nullptr );
	assert( in != nullptr );

	auto Nr = key->nr;
	auto INV_RK = key->buf;
	auto state = out;
	memcpy( state, in, AES_BLOCK_SIZE );

	add_round_key( state, (const uint8_t *)( INV_RK + ( Nr << 2 ) ) );
	for ( auto i = Nr - 1; i > 0; --i )
		aes_inv_round( state, (const uint8_t *)( INV_RK + ( i << 2 ) ) );
	inv_final_round( state, (const uint8_t *)INV_RK );
}

void AES_encrypt( const uint8_t *in, uint8_t *out, const AES_KEY *key )
{
	assert( key != nullptr );
	assert( out != nullptr );
	assert( in != nullptr );

	auto Nr = key->nr;
	auto INV_RK = key->buf;
	auto state = out;
	memcpy( state, in, AES_BLOCK_SIZE );

	add_round_key( state, (const uint8_t *)( INV_RK + ( Nr << 2 ) ) );
	for ( auto i = Nr - 1; i > 0; --i )
		aes_inv_round( state, (const uint8_t *)( INV_RK + ( i << 2 ) ) );
	inv_final_round( state, (const uint8_t *)INV_RK );
}

}

