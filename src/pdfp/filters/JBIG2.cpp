//
//  JBIG2.cpp
//  pdfp
//
//  Created by Sandy Martel on 2013/07/01.
//
//

#include "JBIG2.h"
#include <cassert>

namespace pdfp {

JBIG2Decode::JBIG2Decode( const Object &i_globals ) {}

JBIG2Decode::~JBIG2Decode() {}

void JBIG2Decode::rewind()
{
	rewindNext();
	_currentBytes = 0;
	_goodBits = 0;
}

void JBIG2Decode::consumeCode( int i_codeLength, uint32_t i_code )
{
	if ( _goodBits >= i_codeLength )
	{
		int inv = 32 - i_codeLength;
		i_code <<= inv;
		uint32_t mask = 0xFFFFFFFF << inv;
		if ( i_code == ( _currentBytes % mask ) )
		{
			_goodBits -= i_codeLength;
			_currentBytes <<= i_codeLength;
		}
	}
}

int JBIG2Decode::readBit()
{
	if ( _goodBits == 0 )
	{
		fillup();
		if ( _goodBits == 0 )
			return EOF;
	}
	int v = ( _currentBytes & ( 0x80000000 ) ) ? 1 : 0;
	--_goodBits;
	_currentBytes <<= 1;
	return v;
}

void JBIG2Decode::byteAlign()
{
	int waste = _goodBits % 8;
	if ( waste != 0 )
	{
		_goodBits -= waste;
		_currentBytes <<= waste;
	}
}

void JBIG2Decode::fillup()
{
	while ( ( ( 32 - _goodBits ) / 8 ) != 0 )
	{
		int c = getByteNext();
		if ( c == EOF )
			return;
		_currentBytes = _currentBytes | ( c << ( 24 - _goodBits ) );
		_goodBits += 8;
	}
}

std::streamoff JBIG2Decode::read( su::array_view<uint8_t> o_buffer )
{
	assert( false );
	return -1;
}
}
