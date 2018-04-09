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

void InputFilter::setNext( std::unique_ptr<InputSource> i_next )
{
	_next = std::move( i_next );
}

int InputFilter::getByteNext()
{
	uint8_t b;
	auto r = _next->read( {&b, 1} );
	return r != 1 ? EOF : b;
}

void BufferedInputFilter::rewindNext()
{
	InputFilter::rewindNext();
	_bufferSize = _bufferPos = 0;
}

std::streamoff BufferedInputFilter::readNext( su::array_view<uint8_t> o_buffer )
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
		    InputFilter::readNext( o_buffer.subview( p, o_buffer.size() - p ) );
		if ( len > 0 )
			p += len;
	}
	return p;
}

int BufferedInputFilter::getByteNext()
{
	if ( _bufferPos == _bufferSize )
	{
		_bufferSize = _bufferPos = 0;
		auto r = InputFilter::readNext( {_buffer, 4096} );
		if ( r <= 0 )
			return EOF;
		_bufferSize = r;
	}
	return _buffer[_bufferPos++];
}
}
