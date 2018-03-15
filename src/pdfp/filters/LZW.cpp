/*
 *  LZW.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 09-03-04.
 *  Copyright 2009 by Sandy Martel. All rights reserved.
 *
 */

#include "LZW.h"
#include <iostream>

namespace {
const int CLEARTABLE = 256;
const int EOD = 257;
}

namespace pdfp {

LZWDecode::LZWDecode( int i_EarlyChange ) :
    _EarlyChange( i_EarlyChange )
{
	_dico.reserve( 4097 );
}

void LZWDecode::rewind()
{
	rewindNext();
	_currentBytes = 0;
	_goodBits = 0;
	_pos = 0;
	_endOfFile = false;
	_codeLength = 9;
	_first = true;
}

std::streamoff LZWDecode::read( su::array_view<uint8_t> o_buffer )
{
	if ( _endOfFile )
		return EOF;

	size_t s = 0;
	while ( s < o_buffer.size() )
	{
		if ( _pos >= _buffer.size() and not fillBuffer() )
			return s == 0 ? EOF : s;

		size_t l =
		    std::min( _buffer.size() - _pos, size_t( o_buffer.size() - s ) );
		std::copy( _buffer.begin() + _pos,
		           _buffer.begin() + _pos + l,
		           o_buffer.begin() + s );

		s += l;
		_pos += l;
	}
	return s;
}

bool LZWDecode::fillBuffer()
{
	_pos = 0;

	bool done;
	do
	{
		done = true;
		int code = readCode();
		if ( code == EOF or code == EOD )
		{
			_buffer.clear();
			_endOfFile = true;
			return false;
		}
		else if ( code == CLEARTABLE )
		{
			clearTable();
			done = false;
		}
		else
		{
			if ( _dico.size() >= 4097 )
				clearTable();

			if ( code < 256 )
			{
				_buffer.clear();
				_buffer.push_back( code );
			}
			else if ( (size_t)code < ( _dico.size() + 258 ) )
			{
				_buffer.clear();
				int i;
				for ( i = code; i > 255; )
				{
					_buffer.push_front( _dico[i - 258].tail );
					i = _dico[i - 258].head;
				}
				_buffer.push_front( i );
			}
			else if ( (size_t)code == ( _dico.size() + 258 ) )
			{
				_buffer.push_back( _newChar );
			}
			else
				break;

			_newChar = _buffer[0];
			if ( _first )
			{
				_first = false;
			}
			else
			{
				_dico.push_back( LZWDicoEntry{_prevCode, _newChar} );
				if ( ( ( _dico.size() + 258 ) + _EarlyChange ) == 512 )
					_codeLength = 10;
				else if ( ( ( _dico.size() + 258 ) + _EarlyChange ) == 1024 )
					_codeLength = 11;
				else if ( ( ( _dico.size() + 258 ) + _EarlyChange ) == 2048 )
					_codeLength = 12;
			}
			_prevCode = code;
		}
	} while ( not done );
	return not _buffer.empty();
}

int LZWDecode::readCode()
{
	int code = 0;
	for ( int i = 0; i < _codeLength; ++i )
	{
		int bit = readBit();
		if ( bit == EOF )
			return EOF;
		code = ( code << 1 ) | bit;
	}
	return code;
}

void LZWDecode::clearTable()
{
	_first = true;
	_dico.clear();
	_codeLength = 9;
}

int LZWDecode::readBit()
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

void LZWDecode::fillup()
{
	while ( ( 32 - _goodBits ) >= 8 )
	{
		int c = getByteNext();
		if ( c == EOF )
			return;
		_currentBytes |= ( c << ( 24 - _goodBits ) );
		_goodBits += 8;
	}
}
}
