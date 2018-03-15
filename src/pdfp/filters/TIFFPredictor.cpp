//
//  TIFFPredictor.cpp
//  pdfp
//
//  Created by Sandy Martel on 11/10/03.
//  Copyright 2011年 Sandy Martel. All rights reserved.
//

#include "TIFFPredictor.h"
#include "su/base/endian.h"
#include <cassert>
#include <iostream>

namespace pdfp {

TIFFPredictor::TIFFPredictor( size_t i_width,
                                      int i_bitsPerComp,
                                      int i_nbOfComp ) :
    _width( i_width ),
    _rowBytes( ( ( i_width * i_nbOfComp * i_bitsPerComp + 7 ) >> 3 ) ),
    _bytesPerComp( ( i_nbOfComp * i_bitsPerComp + 7 ) >> 3 ),
    _nbOfComp( i_nbOfComp ),
    _bitsPerComp( i_bitsPerComp )
{
	assert( _bytesPerComp > 0 );

	_buffer = std::make_unique<uint8_t[]>( _rowBytes );
	memset( _buffer.get(), 0, _rowBytes );

	_pos = _rowBytes;
}

void TIFFPredictor::rewind()
{
	rewindNext();
	memset( _buffer.get(), 0, _rowBytes );
	_pos = _rowBytes;
}

std::streamoff TIFFPredictor::read( su::array_view<uint8_t> o_buffer )
{
	size_t s = 0;
	while ( s < o_buffer.size() )
	{
		if ( _pos < _rowBytes )
		{
			auto ptr = _buffer.get() + _pos;
			auto l = std::min( _rowBytes - _pos, o_buffer.size() - s );
			std::copy( ptr, ptr + l, o_buffer.begin() + s );

			s += l;
			_pos += l;
		}
		else if ( not fillBuffer() )
		{
			if ( s == 0 )
				return EOF;
			break;
		}
	}
	return s;
}

bool TIFFPredictor::fillBuffer()
{
	// read a row
	_rowBytes = readNext( {_buffer.get(), (size_t)_rowBytes} );

	// decode
	decodeCurrentRow();
	_pos = 0;

	return _rowBytes > 0;
}

void TIFFPredictor::decodeCurrentRow()
{
	auto ptr = _buffer.get();

	switch ( _bitsPerComp )
	{
		case 1:
		{
			assert( false );
			auto row = (uint8_t *)ptr;
			auto b = row[_bytesPerComp - 1];
			for ( auto i = _bytesPerComp; i < _rowBytes; i += 8 )
			{
				// 1-bit add is just xor
				b = ( b << 8 ) | row[i];
				row[i] ^= b >> _nbOfComp;
			}
			break;
		}
		case 8:
		{
			auto row = (uint8_t *)ptr;
			auto width = _rowBytes;
			for ( auto i = _nbOfComp; i < width; ++i )
				row[i] += row[i - _nbOfComp];
			break;
		}
		case 16:
		{
			auto row = (uint16_t *)ptr;
			auto width = _rowBytes / 2;
			for ( auto i = 0; i < width; ++i )
				row[i] = su::big_to_native( row[i] );
			for ( auto i = _nbOfComp; i < width; ++i )
				row[i] += row[i - _nbOfComp];
			for ( auto i = 0; i < width; ++i )
				row[i] = su::native_to_big( row[i] );
			break;
		}
		default:
		{
			assert( false );
			break;
		}
	}
}
}
