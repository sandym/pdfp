/*
 *  RunLength.h
 *  pdfp
 *
 *  Created by Sandy Martel on 25/08/10.
 *  Copyright 2010 by Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_RUNLENGTH
#define H_PDFP_RUNLENGTH

#include "Filter.h"

namespace pdfp {

class RunLengthDecode : public BufferedFilter
{
public:
	RunLengthDecode() = default;
	virtual ~RunLengthDecode() = default;

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	uint8_t _buffer[128];
	size_t _bufValid = 0, _pos = 0;

	bool fillBuffer();
};
}

#endif
