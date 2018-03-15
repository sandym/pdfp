/*
 *  PNGPredictor.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 09-07-26.
 *  Copyright 2009 by Sandy Martel. All rights reserved.
 *
 */

#include "PNGPredictor.h"
#include <cassert>
#include <cstdlib>

namespace {

inline uint8_t PaethPredictor( int a, int b, int c )
{
	// a = left, b = above, c = upper left
	int p = a + b - c; // initial estimate
	int pa = std::abs( p - a ); // distances to a, b, c
	int pb = std::abs( p - b );
	int pc = std::abs( p - c );
	// return nearest of a,b,c,
	// breaking ties in order a,b,c.
	if ( pa <= pb and pa <= pc )
		return a;
	else if ( pb <= pc )
		return b;
	else
		return c;
}
}

namespace pdfp {

PNGPredictor::PNGPredictor( int i_width,
                                    int i_bitsPerComp,
                                    int i_nbOfComp ) :
    _width( i_width )
{
	_bpp = ( i_nbOfComp * i_bitsPerComp + 7 ) >> 3;
	assert( _bpp > 0 );
	_rowBytes = ( ( _width * i_nbOfComp * i_bitsPerComp + 7 ) >> 3 );
	_buffer = std::make_unique<uint8_t[]>( _rowBytes * 2 );
	memset( _buffer.get(), 0, _rowBytes * 2 );

	_previousRow = _buffer.get();
	_currentRow = _previousRow + _rowBytes;

	_pos = _rowBytes;
}

void PNGPredictor::rewind()
{
	rewindNext();
	memset( _buffer.get(), 0, _rowBytes * 2 );
	_previousRow = _buffer.get();
	_currentRow = _previousRow + _rowBytes;
	_pos = _rowBytes;
}

std::streamoff PNGPredictor::read( su::array_view<uint8_t> o_buffer )
{
	size_t s = 0;
	while ( s < o_buffer.size() )
	{
		if ( _pos < _rowBytes )
		{
			auto ptr = _currentRow + _pos;
			size_t l = std::min( _rowBytes - _pos, o_buffer.size() - s );
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

bool PNGPredictor::fillBuffer()
{
	// read a predictor
	int p = getByteNext();
	if ( p == EOF )
		return false;

	// read a row
	std::swap( _previousRow, _currentRow );
	_rowBytes = readNext( {_currentRow, _rowBytes} );

	// decode
	decodeCurrentRow( p );
	_pos = 0;

	return _rowBytes > 0;
}

void PNGPredictor::decodeCurrentRow( int p )
{
	auto ptr = _currentRow;
	auto end = _currentRow + _rowBytes;
	auto ptrUp = _previousRow;
	switch ( p )
	{
		case 1:
		{
			for ( int i = 0; i < _bpp; ++i )
				++ptr;
			for ( ; ptr < end; ++ptr )
				*ptr = *ptr + *( ptr - _bpp );
			break;
		}
		case 2:
		{
			for ( ; ptr < end; ++ptr, ++ptrUp )
				*ptr = *ptr + *ptrUp;
			break;
		}
		case 3:
		{
			for ( int i = 0; i < _bpp; ++i, ++ptr, ++ptrUp )
				*ptr = *ptr + ( ( *ptrUp ) >> 1 );
			for ( ; ptr < end; ++ptr, ++ptrUp )
				*ptr =
				    *ptr + ( ( int( *ptrUp ) + int( *( ptr - _bpp ) ) ) >> 1 );
			break;
		}
		case 4:
		{
			for ( int i = 0; i < _bpp; ++i, ++ptr, ++ptrUp )
				*ptr = *ptr + PaethPredictor( 0, *ptrUp, 0 );

			for ( ; ptr < end; ++ptr, ++ptrUp )
				*ptr = *ptr + PaethPredictor(
				                  *( ptr - _bpp ), *ptrUp, *( ptrUp - _bpp ) );
			break;
		}
		default:
			break;
	}
}
}
