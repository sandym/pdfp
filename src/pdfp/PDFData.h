/*
 *  PDFData.h
 *  pdfp
 *
 *  Created by Sandy Martel on 06-12-26.
 *  Copyright 2006 Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_PDFDATA
#define H_PDFP_PDFDATA

#include "su/containers/array_view.h"
#include <memory>

namespace pdfp {

//! data format returned by PDF stream data
enum class data_format_t
{
	kInvalid,

	kRaw,
	kJPEGEncoded,
	kJPEG2000
};

struct Data
{
	size_t length{ 0 };
	std::unique_ptr<uint8_t[]> buffer;
	data_format_t format{ data_format_t::kInvalid };
};

/*!
 @brief Abstract interface to a data stream.

    forward iterator only
*/
class AbstractDataStream
{
public:
	AbstractDataStream( const AbstractDataStream & ) = delete;
	AbstractDataStream &operator=( const AbstractDataStream & ) = delete;

	virtual ~AbstractDataStream() = default;

	virtual std::streamoff read( su::array_view<uint8_t> o_buffer ) = 0;

	virtual data_format_t format() const = 0;
	
	Data readAll();

protected:
	AbstractDataStream() = default;
};

typedef std::unique_ptr<AbstractDataStream> DataStreamRef;
}

#endif
