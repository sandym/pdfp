/*
 *  ASCII85.h
 *  pdfp
 *
 *  Created by Sandy Martel on 08-04-19.
 *  Copyright 2008 Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_ASCII85
#define H_PDFP_ASCII85

#include "Filter.h"

namespace pdfp {

class ASCII85Decode : public BufferedInputFilter
{
public:
	ASCII85Decode() = default;
	virtual ~ASCII85Decode() = default;

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	bool _eofReach = false;
	uint8_t _leftOver[4];
	size_t _validSize = 0, _index = 0;

	int getNonSpace();
	int get();
};
}

#endif
