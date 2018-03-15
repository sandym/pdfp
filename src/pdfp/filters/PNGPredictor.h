/*
 *  PNGPredictor.h
 *  pdfptest
 *
 *  Created by Sandy Martel on 09-07-26.
 *  Copyright 2009 by Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_PNGPREDICTOR
#define H_PDFP_PNGPREDICTOR

#include "Filter.h"

namespace pdfp {

class PNGPredictor : public Filter
{
public:
	PNGPredictor( int i_width, int i_bitsPerComp, int i_nbOfComp );
	virtual ~PNGPredictor() = default;

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	int _width;
	size_t _pos, _rowBytes;
	int _bpp;
	std::unique_ptr<uint8_t[]> _buffer;
	uint8_t *_previousRow;
	uint8_t *_currentRow;

	bool fillBuffer();

	void decodeCurrentRow( int p );
};
}

#endif
