//
//  JBIG2.h
//  pdfp
//
//  Created by Sandy Martel on 2013/07/01.
//
//

#ifndef H_PDFP_JBIG2
#define H_PDFP_JBIG2

#include "Filter.h"

namespace pdfp {

class Object;

class JBIG2Decode : public Filter
{
public:
	JBIG2Decode( const Object &i_globals );
	virtual ~JBIG2Decode();

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	// bit reader
	uint32_t _currentBytes = 0;
	int _goodBits = 0;

	int readBit();
	void byteAlign();
	void fillup();
	void consumeCode( int i_codeLength, uint32_t i_code );
};
}

#endif
