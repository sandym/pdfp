/*
 *  ASCII85.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 08-04-19.
 *  Copyright 2008 Sandy Martel. All rights reserved.
 *
 */

#include "ASCII85.h"
#include "su/base/endian.h"

namespace pdfp {

void ASCII85Decode::rewind()
{
	rewindNext();
	_eofReach = false;
	_validSize = 0;
	_index = 0;
}

std::streamoff ASCII85Decode::read( su::array_view<uint8_t> o_buffer )
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

int ASCII85Decode::getNonSpace()
{
	if ( _eofReach )
		return EOF;

	int c;
	do
	{
		c = getByteNext();
	} while ( c > 0 and isspace( c ) );
	if ( c == '~' or c == EOF )
	{
		_eofReach = true;
		c = EOF;
	}
	return c;
}

int ASCII85Decode::get()
{
	if ( _index >= _validSize )
	{
		_index = _validSize = *( (uint32_t *)_leftOver ) = 0;
		int c = getNonSpace();
		if ( c == EOF )
			return c;
		if ( c == 'z' )
		{
			_validSize = 4;
		}
		else if ( c >= 33 and c <= 117 )
		{
			uint32_t group[5] = {
			    'u' - '!', 'u' - '!', 'u' - '!', 'u' - '!', 'u' - '!'};
			size_t groupSize = 0;
			for ( ;; )
			{
				if ( c == '~' or c < 33 or c > 117 ) // eof or invalid
					break;
				group[groupSize++] = c - '!';
				if ( groupSize == 5 )
					break;
				c = getNonSpace();
			}
			if ( groupSize > 1 )
			{
				_validSize = groupSize - 1;
				uint32_t v = ( group[0] * 85 * 85 * 85 * 85 ) +
				             ( group[1] * 85 * 85 * 85 ) +
				             ( group[2] * 85 * 85 ) + ( group[3] * 85 ) +
				             group[4];
				*( (uint32_t *)_leftOver ) = su::native_to_big<uint32_t>( v );
			}
		}
	}

	if ( _index < _validSize )
	{
		return int( _leftOver[_index++] );
	}
	return EOF;
}
}
