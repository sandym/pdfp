/*
 *  ASCIIHex.h
 *  pdfp
 *
 *  Created by Sandy Martel on 25/08/10.
 *  Copyright 2010 by Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_ASCIIHEX
#define H_PDFP_ASCIIHEX

#include "Filter.h"

namespace pdfp {

class ASCIIHexDecode : public BufferedFilter
{
public:
	ASCIIHexDecode() = default;
	virtual ~ASCIIHexDecode() = default;

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	bool _eofReach = false;

	int getNextHexValue();
	int get();
};
}

#endif
