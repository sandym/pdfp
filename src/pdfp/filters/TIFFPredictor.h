//
//  TIFFPredictor.h
//  pdfp
//
//  Created by Sandy Martel on 11/10/03.
//  Copyright 2011年 Sandy Martel. All rights reserved.
//

#ifndef H_PDFP_TIFFPREDICTOR
#define H_PDFP_TIFFPREDICTOR

#include "Filter.h"

namespace pdfp {

class TIFFPredictor : public InputFilter
{
public:
	TIFFPredictor( size_t i_width, int i_bitsPerComp, int i_nbOfComp );
	virtual ~TIFFPredictor() = default;

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	[[maybe_unused]] const size_t _width;
	ssize_t _rowBytes;
	const int _bytesPerComp;
	const int _nbOfComp;
	const int _bitsPerComp;

	size_t _pos;
	std::unique_ptr<uint8_t[]> _buffer;

	bool fillBuffer();

	void decodeCurrentRow();
};
}

#endif
