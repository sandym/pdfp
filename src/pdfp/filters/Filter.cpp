/*
 *  Filter.cpp
 *  pdfptest
 *
 *  Created by Sandy Martel on 24/08/10.
 *  Copyright 2010 by Sandy Martel. All rights reserved.
 *
 */

#include "Filter.h"
#include <cassert>

namespace pdfp {

void Filter::setNext( std::unique_ptr<ReadSource> i_next )
{
	_next = std::move( i_next );
}

int Filter::getByteNext()
{
	uint8_t b;
	auto r = _next->read( {&b, 1} );
	return r != 1 ? EOF : b;
}

void BufferedFilter::rewindNext()
{
	Filter::rewindNext();
	_bufferSize = _bufferPos = 0;
}

std::streamoff BufferedFilter::readNext( su::array_view<uint8_t> o_buffer )
{
	size_t p = 0;
	// use the buffer
	if ( _bufferPos != _bufferSize )
	{
		auto len = std::min( o_buffer.size(), _bufferSize - _bufferPos );
		memcpy( o_buffer.data(), _buffer + _bufferPos, len );
		p += len;
		_bufferPos += len;
	}
	// read missing data
	if ( p < o_buffer.size() )
	{
		auto len =
		    Filter::readNext( o_buffer.subview( p, o_buffer.size() - p ) );
		if ( len > 0 )
			p += len;
	}
	return p;
}

int BufferedFilter::getByteNext()
{
	if ( _bufferPos == _bufferSize )
	{
		_bufferSize = _bufferPos = 0;
		auto r = Filter::readNext( {_buffer, 4096} );
		if ( r <= 0 )
			return EOF;
		_bufferSize = r;
	}
	return _buffer[_bufferPos++];
}
}
