/*
 *  Filter.h
 *  pdfp
 *
 *  Created by Sandy Martel on 24/08/10.
 *  Copyright 2010 by Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_FILTER
#define H_PDFP_FILTER

#include "su/containers/array_view.h"
#include <ios>

namespace pdfp {

class InputSource
{
protected:
	InputSource() = default;

public:
	virtual ~InputSource() = default;
	InputSource( const InputSource & ) = delete;
	InputSource &operator=( const InputSource & ) = delete;

	virtual void rewind() = 0;
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer ) = 0;
};

class InputFilter : public InputSource
{
public:
	virtual ~InputFilter() = default;

	virtual void rewind() = 0;
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer ) = 0;

	void setNext( std::unique_ptr<InputSource> i_next );

private:
	std::unique_ptr<InputSource> _next;

protected:
	InputFilter() = default;

	void rewindNext() { _next->rewind(); }
	std::streamoff readNext( su::array_view<uint8_t> o_buffer )
	{
		return _next->read( o_buffer );
	}
	int getByteNext();
};

class BufferedInputFilter : public InputFilter
{
public:
	virtual ~BufferedInputFilter() = default;

private:
	uint8_t _buffer[4096];
	size_t _bufferSize = 0, _bufferPos = 0;

protected:
	void rewindNext();
	std::streamoff readNext( su::array_view<uint8_t> o_buffer );
	int getByteNext();
};
}

#endif
