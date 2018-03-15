/*
 *  LZW.h
 *  pdfp
 *
 *  Created by Sandy Martel on 09-03-04.
 *  Copyright 2009 by Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_LZW
#define H_PDFP_LZW

#include "Filter.h"
#include <deque>
#include <vector>

namespace pdfp {

class LZWDecode : public BufferedFilter
{
public:
	LZWDecode( int i_EarlyChange );
	virtual ~LZWDecode() = default;

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	// bit reader
	uint32_t _currentBytes = 0;
	int _goodBits = 0;

	int readBit();
	void fillup();
	void consumeCode( int i_codeLength, uint32_t i_code );

	// Filter parameters
	int _EarlyChange;

	std::deque<uint8_t> _buffer;
	size_t _pos = 0;
	bool _endOfFile = false;

	// decoding
	struct LZWDicoEntry
	{
		int head, tail;
	};
	std::vector<LZWDicoEntry> _dico;
	int _codeLength = 9;
	int _newChar, _prevCode;
	bool _first = true;

	void clearTable();
	bool fillBuffer();
	int readCode();
};
}

#endif
