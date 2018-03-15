/*
 *  Flate.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 25/08/10.
 *  Copyright 2010 by Sandy Martel. All rights reserved.
 *
 */

#include "Flate.h"
#include "su/log/logger.h"
#include <cassert>

namespace {

void logZLibError( int err )
{
	switch ( err )
	{
		case Z_ERRNO:
			log_error() << "flate decode: error " << errno;
			break;
		case Z_STREAM_ERROR:
			log_error() << "flate decode: stream error";
			break;
		case Z_DATA_ERROR:
			log_error() << "flate decode: data error";
			break;
		case Z_MEM_ERROR:
			log_error() << "flate decode: memory error";
			break;
		case Z_BUF_ERROR:
			log_error() << "flate decode: buffer error";
			break;
		case Z_VERSION_ERROR:
			log_error() << "flate decode: version error";
			break;
		default:
			break;
	}
}
}

namespace pdfp {

FlateDecode::~FlateDecode()
{
	if ( _inited )
	{
		int err = inflateEnd( &_zstream );
		if ( err < 0 )
			logZLibError( err );
	}
}

void FlateDecode::rewind()
{
	rewindNext();
	if ( _inited )
	{
		int err = inflateEnd( &_zstream );
		if ( err < 0 )
			logZLibError( err );
		_inited = false;
	}
}

std::streamoff FlateDecode::read( su::array_view<uint8_t> o_buffer )
{
	int err;
	if ( not _inited )
	{
		memset( &_zstream, 0, sizeof( z_stream ) );
		err = inflateInit2( &_zstream, -kMaxWBits );
		assert( err == Z_OK );
		_adler32 = adler32( 0L, Z_NULL, 0 );

		_inited = true;

		// read the header
		int c1;
		do
		{
			c1 = getByteNext();
		} while ( c1 != EOF and isspace( c1 ) );
		int c2 = getByteNext();
		if ( c1 == EOF or c2 == EOF )
			return EOF;
		if ( ( c1 & 0x0f ) != 0x08 )
			log_error() << "wrong compression method";
		if ( ( ( ( c1 << 8 ) + c2 ) % 31 ) != 0 )
			log_error() << "bad FCHECK";
		if ( c2 & 0x20 )
			log_error() << "FDICT bit set";
	}

	_zstream.next_out = o_buffer.data();
	_zstream.avail_out = (uInt)o_buffer.size();
	while ( _zstream.avail_out > 0 )
	{
		if ( _zstream.avail_in == 0 )
		{
			auto len = readNext( {_inputBuffer, kFlateWindowSize} );
			if ( len <= 0 )
				break;
			_zstream.next_in = _inputBuffer;
			_zstream.avail_in = (uInt)len;
		}

		err = inflate( &_zstream, Z_NO_FLUSH );
		if ( err < 0 )
		{
			logZLibError( err );
			break;
		}
		else if ( err == Z_STREAM_END )
		{
			// read the 4 bytes trailer
			if ( _zstream.avail_in < 4 )
				readNext( {_inputBuffer, 4 - _zstream.avail_in} );
			break;
		}
		else
		{
			//_adler32 = adler32( _adler32, _zstream.next_out, len );
		}
	}
	size_t s = o_buffer.size() - _zstream.avail_out;
	return s == 0 ? EOF : s;
}
}
