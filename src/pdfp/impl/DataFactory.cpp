/*
 *  DataFactory.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 09-02-16.
 *  Copyright 2009 by Sandy Martel. All rights reserved.
 *
 */

#include "DataFactory.h"
#include "ImageStreamInfo.h"
#include "pdfp/PDFDocument.h"
#include "pdfp/filters/ASCII85.h"
#include "pdfp/filters/ASCIIHex.h"
#include "pdfp/filters/CCITTFax.h"
#include "pdfp/filters/Filter.h"
#include "pdfp/filters/Flate.h"
#include "pdfp/filters/JBIG2.h"
#include "pdfp/filters/LZW.h"
#include "pdfp/filters/PNGPredictor.h"
#include "pdfp/filters/RunLength.h"
#include "pdfp/filters/TIFFPredictor.h"
#include "pdfp/security/SecurityHandler.h"
#include "su/log/logger.h"
#include <cassert>

namespace pdfp {

class DocSource : public ReadSource
{
public:
	DocSource( const Document *i_doc,
	              size_t i_offset,
	              size_t i_len );
	virtual ~DocSource() = default;

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	const Document *_doc;
	size_t _offset, _len;
	size_t _pos = 0;
};

DocSource::DocSource( const Document *i_doc,
                            size_t i_offset,
                            size_t i_len ) :
    _doc( i_doc ),
    _offset( i_offset ),
    _len( i_len )
{
}

void DocSource::rewind()
{
	_pos = 0;
}

std::streamoff DocSource::read( su::array_view<uint8_t> o_buffer )
{
	if ( _pos >= _len )
		return -1;
	size_t l = std::min( o_buffer.size(), _len - _pos );
	auto result = _doc->read( _offset + _pos, o_buffer.subview( 0, l ) );
	if ( result <= 0 )
		return EOF;
	else
	{
		_pos += result;
		return result;
	}
}

// MARK: -

class BufferSource : public ReadSource
{
public:
	BufferSource( const char *i_ptr, size_t i_len );
	virtual ~BufferSource() = default;

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	const char *_ptr;
	size_t _len, _pos;
};

BufferSource::BufferSource( const char *i_ptr, size_t i_len ) :
    _ptr( i_ptr ),
    _len( i_len ),
    _pos( 0 )
{
}

void BufferSource::rewind()
{
	_pos = 0;
}

std::streamoff BufferSource::read( su::array_view<uint8_t> o_buffer )
{
	if ( _pos >= _len )
		return -1;
	auto result = std::min( o_buffer.size(), _len - _pos );
	if ( result <= 0 )
		return EOF;
	else
	{
		memcpy( o_buffer.data(), _ptr + _pos, result );
		_pos += result;
		return result;
	}
}

// MARK: -

int getParamInt( const Object &i_decodeParams,
                 const std::string &i_param,
                 int i_value )
{
	auto &obj = i_decodeParams[i_param];
	if ( obj.is_number() )
		return obj.int_value();
	else
		return i_value;
}

std::string getParamName( const Object &i_decodeParams,
                          const std::string &i_param )
{
	auto &obj = i_decodeParams[i_param];
	if ( obj.is_name() )
		return obj.name_value();
	else
		return {};
}

bool getParamBool( const Object &i_decodeParams,
                   const std::string &i_param,
                   bool i_value )
{
	auto &obj = i_decodeParams[i_param];
	if ( obj.is_boolean() )
		return obj.bool_value();
	else
		return i_value;
}

Object getParamStream(
    const Object &i_decodeParams, const std::string &i_param )
{
	auto &obj = i_decodeParams[i_param];
	if ( obj.is_stream() )
		return obj;
	return {};
}

// MARK: -

class DataStream : public AbstractDataStream
{
public:
	DataStream( std::unique_ptr<ReadSource> i_input );
	virtual ~DataStream() = default;

	void pushFilter( std::unique_ptr<Filter> i_filter );
	bool pushFilter( const std::string &i_name,
	                 const Object &i_decodeParams );
	void pushPredictor( const Object &i_decodeParams );

	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );
	virtual data_format_t format() const;

private:
	data_format_t _format;

	std::unique_ptr<ReadSource> _input;

	template<typename T, int NC>
	bool choose_predictor_format( size_t width, int bpp, int c );

	// forbidden
	DataStream( const DataStream & ) = delete;
	DataStream &operator=( const DataStream & ) = delete;
};

DataStream::DataStream( std::unique_ptr<ReadSource> i_input ) :
    _format( data_format_t::kRaw ),
    _input( std::move( i_input ) )
{
}

std::streamoff DataStream::read( su::array_view<uint8_t> o_buffer )
{
	return _input->read( o_buffer );
}

data_format_t DataStream::format() const
{
	return _format;
}

void DataStream::pushFilter( std::unique_ptr<Filter> i_filter )
{
	i_filter->setNext( std::move( _input ) );
	_input = std::move( i_filter );
}

bool DataStream::pushFilter( const std::string &i_name,
                           const Object &i_decodeParams )
{
	if ( i_name == "FlateDecode" or i_name == "Fl" )
	{
		pushFilter( std::make_unique<FlateDecode>() );
		if ( not i_decodeParams.is_null() )
			pushPredictor( i_decodeParams );
	}
	else if ( i_name == "CCITTFaxDecode" or i_name == "CCF" )
	{
		int K = getParamInt( i_decodeParams, "K", 0 );
		bool EndOfLine = getParamBool( i_decodeParams, "EndOfLine", false );
		bool EncodedByteAlign =
		    getParamBool( i_decodeParams, "EncodedByteAlign", false );
		int Columns = getParamInt( i_decodeParams, "Columns", 1728 );
		int Rows = getParamInt( i_decodeParams, "Rows", 0 );
		bool EndOfBlock = getParamBool( i_decodeParams, "EndOfBlock", true );
		bool BlackIs1 = getParamBool( i_decodeParams, "BlackIs1", false );
		int DamagedRowsBeforeError =
		    getParamInt( i_decodeParams, "DamagedRowsBeforeError", 0 );
		pushFilter( std::make_unique<CCITTFaxDecode>(
		    K,
		    EndOfLine,
		    EncodedByteAlign,
		    Columns,
		    Rows,
		    EndOfBlock,
		    BlackIs1,
		    DamagedRowsBeforeError ) );
	}
	else if ( i_name == "ASCIIHexDecode" or i_name == "AHx" )
	{
		pushFilter( std::make_unique<ASCIIHexDecode>() );
	}
	else if ( i_name == "ASCII85Decode" or i_name == "A85" )
	{
		pushFilter( std::make_unique<ASCII85Decode>() );
	}
	else if ( i_name == "LZWDecode" or i_name == "LZW" )
	{
		int EarlyChange = getParamInt( i_decodeParams, "EarlyChange", 1 );
		pushFilter( std::make_unique<LZWDecode>( EarlyChange ) );
	}
	else if ( i_name == "RunLengthDecode" or i_name == "RL" )
	{
		pushFilter( std::make_unique<RunLengthDecode>() );
	}
	else if ( i_name == "DCTDecode" or i_name == "DCT" )
	{
		_format = data_format_t::kJPEGEncoded;
		return false;
	}
	else if ( i_name == "JPXDecode" )
	{
		_format = data_format_t::kJPEG2000;
		return false;
	}
	else if ( i_name == "JBIG2Decode" )
	{
		auto globals = getParamStream( i_decodeParams, "JBIG2Globals" );
		pushFilter( std::make_unique<JBIG2Decode>( globals ) );
	}
	else if ( i_name == "Crypt" )
	{
		std::string name = getParamName( i_decodeParams, "Name" );
		if ( not name.empty() and name != "Identity" )
		{
			log_error() << "unkown filter: " << i_name << " - " << name;
			assert( false );
			return false;
		}
	}
	else
	{
		log_error() << "unkown filter: " << i_name;
		assert( false );
		return false;
	}
	return true;
}

void DataStream::pushPredictor( const Object &i_decodeParams )
{
	auto &Predictor = i_decodeParams["Predictor"];
	int p = 1;
	if ( Predictor.is_number() )
		p = Predictor.int_value();

	auto &Columns = i_decodeParams["Columns"];
	int width = 1;
	if ( Columns.is_number() )
		width = Columns.int_value();

	int bpc = 8;
	auto &bpcObj = i_decodeParams["BitsPerComponent"];
	if ( bpcObj.is_number() )
		bpc = bpcObj.int_value();

	int c = 1;
	auto &Colors = i_decodeParams["Colors"];
	if ( Colors.is_number() )
		c = Colors.int_value();

	switch ( p )
	{
		case 1:
			break; // no predictor
		case 2:
		{
			auto n = std::make_unique<TIFFPredictor>( width, bpc, c );
			n->setNext( std::move( _input ) );
			_input = std::move( n );
			break;
		}
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		{
			auto n = std::make_unique<PNGPredictor>( width, bpc, c );
			n->setNext( std::move( _input ) );
			_input = std::move( n );
			break;
		}
		default:
			assert( false );
			break;
	}
}

// MARK: -

class DataStreamLimiter : public Filter
{
public:
	DataStreamLimiter( size_t i_limit );
	virtual ~DataStreamLimiter() = default;

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

private:
	size_t _limit, _left;
};

DataStreamLimiter::DataStreamLimiter( size_t i_limit ) :
    _limit( i_limit ),
    _left( i_limit )
{
}

void DataStreamLimiter::rewind()
{
	rewindNext();
	_left = _limit;
}

std::streamoff DataStreamLimiter::read( su::array_view<uint8_t> o_buffer )
{
	if ( _left == 0 )
		return 0;
	auto s = o_buffer.size();
	if ( s > _left )
		s = _left;
	auto r = readNext( o_buffer.subview( 0, s ) );
	if ( r > 0 )
	{
		_left -= r;
		return r;
	}
	else
		return 0;
}

// MARK: -

std::vector<std::pair<std::string, const Object>> collectFilters(
    const Object &i_dictionary )
{
	std::vector<std::pair<std::string, const Object>> filterList;

	auto &filter = i_dictionary["Filter"];
	if ( not filter.is_null() )
	{
		std::vector<std::string> nameList;

		if ( filter.is_array() )
		{
			// array of filters
			for ( auto &it : filter.array_items_resolved() )
				nameList.push_back( it.name_value() );
		}
		else
			nameList.push_back( filter.name_value() );

		std::vector<const Object> paramList;
		paramList.reserve( nameList.size() );

		auto decodeParams = i_dictionary["DecodeParms"];
		if ( decodeParams.is_null() )
			decodeParams = i_dictionary["DP"];
		if ( not decodeParams.is_null() )
		{
			if ( decodeParams.is_array() )
			{
				// array of params
				for ( auto &it : decodeParams.array_items_resolved() )
					paramList.push_back( it );
			}
			else
				paramList.push_back( decodeParams );
		}
		while ( paramList.size() < nameList.size() )
			paramList.push_back( {} );

		filterList.reserve( nameList.size() );
		auto param = paramList.begin();
		for ( auto name = nameList.begin(); name != nameList.end();
		      ++name, ++param )
			filterList.push_back( std::make_pair( *name, *param ) );
	}
	return filterList;
}

// MARK: -

DataStreamRef createDataStream( const Object &i_dict,
                                      size_t i_offset,
                                      size_t i_length,
                                      int i_id,
                                      int i_gen )
{
	auto filterList = collectFilters( i_dict );

	auto data = std::make_unique<DataStream>(
	    std::make_unique<DocSource>( i_dict.document(), i_offset, i_length ) );

	bool isMetadata = false;
	auto mdtype = i_dict["Type"];
	if ( not mdtype.is_null() )
	{
		if ( mdtype.name_value() == "Metadata" )
			isMetadata = true;
	}

	auto crypter = i_dict.document()->createCrypter( isMetadata, i_id, i_gen );
	if ( crypter.get() != nullptr )
		data->pushFilter( std::move( crypter ) );

	bool needLimit = false, isBitmap = false;
	auto filter = filterList.begin();
	for ( ; filter != filterList.end(); ++filter )
	{
		if ( not data->pushFilter( filter->first, filter->second ) )
			break;
		else
		{
			if ( filter->first == "FlateDecode" or filter->first == "Fl" )
				needLimit = true;
			if ( filter->first == "CCITTFaxDecode" or filter->first == "CCF" or
			     filter->first == "JBIG2Decode" )
				needLimit = isBitmap = true;
		}
	}
	if ( needLimit )
	{
		int width, height;
		int bpc, cpp;
		if ( getImageInfo( i_dict, isBitmap, width, height, bpc, cpp ) )
		{
			size_t rowBytes = ( ( ( width * cpp * bpc ) + 7 ) & ( ~7 ) ) / 8;
			data->pushFilter(
			    std::make_unique<DataStreamLimiter>( rowBytes * height ) );
		}
	}
	return data;
}

// MARK: -

DataStreamRef createDataStream( const Object &i_dict,
                                      const char *i_ptr,
                                      size_t i_length )
{
	auto filterList = collectFilters( i_dict );

	auto data = std::make_unique<DataStream>(
	    std::make_unique<BufferSource>( i_ptr, i_length ) );

	bool needLimit = false, isBitmap = false;
	auto filter = filterList.begin();
	for ( ; filter != filterList.end(); ++filter )
	{
		if ( not data->pushFilter( filter->first, filter->second ) )
			break;
		else
		{
			if ( filter->first == "FlateDecode" or filter->first == "Fl" )
				needLimit = true;
			if ( filter->first == "CCITTFaxDecode" or filter->first == "CCF" or
			     filter->first == "JBIG2Decode" )
				needLimit = isBitmap = true;
		}
	}
	if ( needLimit )
	{
		int width, height;
		int bpc, cpp;
		if ( getImageInfo( i_dict, isBitmap, width, height, bpc, cpp ) )
		{
			size_t rowBytes = ( ( ( width * cpp * bpc ) + 7 ) & ( ~7 ) ) / 8;
			data->pushFilter(
			    std::make_unique<DataStreamLimiter>( rowBytes * height ) );
		}
	}
	return data;
}

Data AbstractDataStream::readAll()
{
	Data result;
	
	size_t capacity = 0;
	for ( ;; )
	{
		uint8_t buffer[4096];
		auto l = read( buffer );
		if ( l <= 0 )
			break;
		if ( ( result.length + l ) > capacity )
		{
			capacity =
			    std::max<size_t>( std::max<size_t>( result.length + l, capacity * 2 ), 128 );
			auto newData = std::make_unique<uint8_t[]>( capacity );
			memcpy( newData.get(), result.buffer.get(), result.length );
			result.buffer = std::move( newData );
		}
		memcpy( result.buffer.get() + result.length, buffer, l );
		result.length += l;
	}
	result.format = format();

	return result;
}

}

