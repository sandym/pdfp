/*
 *  ASCIIHex.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 25/08/10.
 *  Copyright 2010 by Sandy Martel. All rights reserved.
 *
 */

#include "ASCIIHex.h"
#include "su/log/logger.h"

namespace pdfp {

void ASCIIHexDecode::rewind()
{
	rewindNext();
	_eofReach = false;
}

std::streamoff ASCIIHexDecode::read( su::array_view<uint8_t> o_buffer )
{
	auto ptr = o_buffer.begin();
	auto e = o_buffer.end();
	while ( ptr < e )
	{
		int c = get();
		if ( c == EOF )
			break;
		*ptr++ = c;
	}
	return ptr - o_buffer.begin();
}

int ASCIIHexDecode::getNextHexValue()
{
	if ( _eofReach )
		return EOF;

	int c;
	do
	{
		c = getByteNext();
	} while ( c > 0 and isspace( c ) );
	if ( c == '>' or c == EOF )
	{
		_eofReach = true;
		return EOF;
	}
	if ( c >= '0' and c <= '9' )
		return c - '0';
	else if ( c >= 'a' and c <= 'f' )
		return c - 'a' + 10;
	else if ( c >= 'A' and c <= 'F' )
		return c - 'A' + 10;
	else
	{
		log_error() << "illegal character in ASCIIHexDecode filter";
		_eofReach = true;
		return EOF;
	}
}

int ASCIIHexDecode::get()
{
	int c1 = getNextHexValue();
	if ( c1 == EOF )
		return c1;
	int c2 = getNextHexValue();
	if ( c2 == EOF )
		c2 = 0;
	return ( c1 << 4 ) + c2;
}
}
