/*
 *  RunLength.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 25/08/10.
 *  Copyright 2010 by Sandy Martel. All rights reserved.
 *
 */

#include "RunLength.h"

namespace pdfp {

void RunLengthDecode::rewind()
{
	rewindNext();
	_bufValid = _pos = 0;
}

std::streamoff RunLengthDecode::read( su::array_view<uint8_t> o_buffer )
{
	size_t s = 0;
	while ( s < o_buffer.size() )
	{
		if ( _pos < _bufValid )
		{
			size_t l =
			    std::min( _bufValid - _pos, size_t( o_buffer.size() - s ) );
			std::copy(
			    _buffer + _pos, _buffer + _pos + l, o_buffer.begin() + s );
			s += l;
			_pos += l;
		}
		else if ( not fillBuffer() )
			return s == 0 ? EOF : s;
	}
	return s;
}

bool RunLengthDecode::fillBuffer()
{
	int length = getByteNext();
	if ( length < 0 or length == 128 )
		return false;

	if ( length < 128 )
	{
		_bufValid = readNext( {_buffer, size_t( length + 1 )} );
	}
	else
	{
		int b = getByteNext();
		if ( b == EOF )
			return false;
		_bufValid = 257 - length;
		memset( _buffer, b, _bufValid );
	}
	_pos = 0;
	return true;
}
}
