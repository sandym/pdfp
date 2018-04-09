/*
 *  Flate.h
 *  pdfp
 *
 *  Created by Sandy Martel on 25/08/10.
 *  Copyright 2010 by Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_FLATE
#define H_PDFP_FLATE

#include "Filter.h"
#include <zlib.h>

namespace pdfp {

const int kMaxWBits = 15;
const int kFlateWindowSize = ( 1 << ( kMaxWBits ) );

class FlateDecode : public InputFilter
{
public:
	FlateDecode() = default;
	virtual ~FlateDecode();

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	uint8_t _inputBuffer[kFlateWindowSize];
	z_stream _zstream;
	bool _inited = false;
	uLong _adler32;
};
}

#endif
