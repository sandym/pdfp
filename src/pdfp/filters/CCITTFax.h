/*
 *  CCITTFax.h
 *  pdfp
 *
 *  Created by Sandy Martel on 09-03-04.
 *  Copyright 2009 by Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_CCITTFAX
#define H_PDFP_CCITTFAX

#include "Filter.h"
#include <vector>

namespace pdfp {

class CCITTFaxDecode : public BufferedFilter
{
public:
	CCITTFaxDecode( int i_K,
	                    bool i_EndOfLine,
	                    bool i_EncodedByteAlign,
	                    int i_Columns,
	                    int i_Rows,
	                    bool i_EndOfBlock,
	                    bool i_BlackIs1,
	                    int i_DamagedRowsBeforeError );
	virtual ~CCITTFaxDecode();

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	// bit reader
	uint32_t _currentBytes = 0;
	int _goodBits = 0;

	int readBit()
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

	void byteAlign();
	void fillup();
	void consumeCode( int i_codeLength, uint32_t i_code );

	// Filter parameters
	int _K;
	bool _EndOfLine;
	bool _EncodedByteAlign;
	int _Columns;
	int _Rows;
	bool _EndOfBlock;
	bool _BlackIs1;
	int _DamagedRowsBeforeError;

	//	row reading
	std::vector<uint8_t> _currentRow;
	size_t _pos = std::numeric_limits<size_t>::max();

	// row decoding
	bool _nextRowIs1D;
	int _rowCounter = 0;
	std::vector<int> _refLine, _codingLine;
	bool fillNextRow();
	bool read1dSpan( bool i_wantWhite, int &o_span );
};
}

#endif
