/*
 *  Parser.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on Sat Apr 12 2003.
 *  Copyright (c) 2003 Sandy Martel. All rights reserved.
 *
 */

#include "Parser.h"
#include "pdfp/PDFDocument.h"
#include "su/containers/flat_set.h"
#include "su/containers/stackarray.h"
#include "su/log/logger.h"
#include "su/streams/membuf.h"
#include "Utils.h"
#include <cassert>
#include <unordered_set>

namespace {
bool ArrayToVectorOfInt( const pdfp::Object &i_array,
                            std::vector<int> &o_vector )
{
	if ( not i_array.is_array() )
		return false;
	for ( auto &it : i_array.array_items_resolved() )
	{
		if ( it.is_number() )
			o_vector.push_back( it.int_value() );
		else
			return false;
	}
	return true;
}
struct SubSection
{
	int objNb, nbOfEntry;
};
bool ArrayToVectorOfRange( const pdfp::Object &i_array,
                              std::vector<SubSection> &o_vector )
{
	if ( not i_array.is_array() or (i_array.array_size()&1) )
		return false;
	auto arr = i_array.array_items_resolved();
	for ( auto it = arr.begin(); it != arr.end(); ++it )
	{
		auto &obj1 = *it;
		++it;
		auto &obj2 = *it;
		if ( obj1.is_number() and obj2.is_number() )
			o_vector.push_back( SubSection{obj1.int_value(),
			                               obj2.int_value()} );
		else
			return false;
	}
	return true;
}

bool isHeaderLine( const std::string &i_line, std::string &o_headerVersion )
{
	auto p = i_line.find( "%PDF-" );
	if ( p != std::string::npos )
	{
		if ( i_line.size() >= ( p + 8 ) and isdigit( i_line[p + 5] ) )
		{
			o_headerVersion = i_line.substr( p + 5 );
			return true;
		}
	}
	return false;
}

bool isObjDef( const std::string &i_line, int &num, int &gen )
{
	// search for ^\s*\d+\s+\d+\s+obj

	auto ptr = i_line.begin();

	// skip spaces
	while ( ptr != i_line.end() and std::isspace( *ptr ) )
		++ptr;

	// read int
	if ( ptr == i_line.end() or not std::isdigit( *ptr ) )
		return false;

	num = 0;
	while ( ptr != i_line.end() and std::isdigit( *ptr ) )
		num = ( num * 10 ) + ( *ptr++ - '0' );

	if ( ptr == i_line.end() or not std::isspace( *ptr ) )
		return false;

	// skip spaces
	while ( ptr != i_line.end() and std::isspace( *ptr ) )
		++ptr;

	// read int
	if ( ptr == i_line.end() or not std::isdigit( *ptr ) )
		return false;

	gen = 0;
	while ( ptr != i_line.end() and std::isdigit( *ptr ) )
		gen = ( gen * 10 ) + ( *ptr++ - '0' );

	if ( ptr == i_line.end() or not std::isspace( *ptr ) )
		return false;

	// skip spaces
	while ( ptr != i_line.end() and std::isspace( *ptr ) )
		++ptr;

	// read obj
	if ( ptr == i_line.end() or *ptr != 'o' )
		return false;
	++ptr;
	if ( ptr == i_line.end() or *ptr != 'b' )
		return false;
	++ptr;
	if ( ptr == i_line.end() or *ptr != 'j' )
		return false;
	++ptr;

	return ptr == i_line.end() or pdfp::isWhiteSpace( *ptr ) or pdfp::isDelimiter( *ptr );
}

}

namespace pdfp {

Parser::Parser( std::istream &i_str, Document *i_doc ) :
    _doc( i_doc ),
    _tokenizer( i_str )
{
}

void Parser::cleanup()
{
	_compressedObjects.clear();
}

Object Parser::readXRef( std::string &o_headerVersion )
{
	_tokenizer.seekg( 0, std::ios_base::beg );

	// get header version
	Token token, dummy;
	_tokenizer.nextTokenForced( token, Token::tok_version );
	o_headerVersion = token.value();

	// get xref and trailer
	const int kSearchSize = 512;
	const char startxref[] = "startxref";
	std::reverse_iterator<const char *> startxrefBeg( startxref +
	                                                  sizeof( startxref ) - 1 );
	std::reverse_iterator<const char *> startxrefEnd( startxref );

	char buffer[kSearchSize];
	std::istream::off_type startxrefPos = 0;
	_tokenizer.seekg( -kSearchSize, std::ios_base::end );
	if ( not _tokenizer )
	{
		// went back too far, probably a very small file
		_tokenizer.seekg( 0, std::ios_base::beg );
	}
	for ( int i = 0; i < 30 and _tokenizer; ++i )
	{
		size_t len = _tokenizer.read( buffer, kSearchSize );
		if ( len == 0 )
			break;
		std::reverse_iterator<const char *> beginRange( buffer + len );
		std::reverse_iterator<const char *> endRange( buffer );
		auto pos =
		    std::search( beginRange, endRange, startxrefBeg, startxrefEnd );
		if ( pos != endRange )
		{
			startxrefPos = pos.base() - ( buffer + len ) - sizeof( startxref );
			break;
		}
		_tokenizer.seekg( -( kSearchSize * 2 ) + (int)sizeof( startxref ),
		                  std::ios_base::cur );
	}
	if ( startxrefPos == 0 )
		throw std::runtime_error( "invalid PDF file" );

	_tokenizer.seekg( startxrefPos, std::ios_base::cur );
	_tokenizer.nextTokenForced( token, Token::tok_startxref );
	_tokenizer.nextTokenForced( token, Token::tok_int );
	_tokenizer.nextTokenForced( dummy, Token::tok_eof );

	std::stack<std::streamoff> xrefposStack;
	xrefposStack.push( token.intValue() );

	Object::dictionary trailerDict;

	std::unordered_set<std::streamoff> visitedXref;
	while ( not xrefposStack.empty() )
	{
		auto xrefpos = xrefposStack.top();
		xrefposStack.pop();
		if ( visitedXref.find( xrefpos ) != visitedXref.end() )
			continue;

		visitedXref.insert( xrefpos );
		_tokenizer.seekg( xrefpos, std::ios_base::beg );

		std::streamoff prev = -1;
		_tokenizer.nextTokenForced( dummy );
		if ( dummy.type() == Token::tok_xref )
			readCrossReferenceTable( trailerDict, prev );
		else if ( dummy.type() == Token::tok_int )
		{
			_tokenizer.putBack( dummy );
			readCrossReferenceStream( trailerDict, prev );
		}
		else
			_tokenizer.invalidToken( dummy );

		if ( prev != -1 )
			xrefposStack.push( prev );

		auto it = trailerDict.find( "XRefStm" );
		if ( it != trailerDict.end() and not it->second.is_null() )
		{
			auto &xrefStm = it->second;
			if ( xrefStm.is_number() )
				xrefposStack.push( xrefStm.int_value() );
			trailerDict.erase( "XRefStm" );
		}
	}

	if ( not verifyXRef() )
		throw std::runtime_error( "invalid PDF file" );

	return Object::create_dictionary( _doc, std::move(trailerDict) );
}

bool Parser::verifyXRef()
{
	int idCounter = 0;
	for ( auto &one_xref : _doc->xrefTable() )
	{
		if ( not one_xref.compressed() and one_xref.pos() != -1 )
		{
			if ( one_xref.pos() == 0 and one_xref.generation() == 0 )
			{
				// should have been an 'f' command
				//one_xref = XRef( 0, -1 );
			}
			else
			{
				_tokenizer.seekg( one_xref.pos(), std::ios_base::beg );
				Token token;
				_tokenizer.nextTokenForced( token, Token::tok_int );
				if ( token.intValue() != idCounter )
					return false;
			}
		}
		++idCounter;
	}
	return true;
}

void transfer( const Object::dictionary &src, Object::dictionary &dst )
{
	for ( auto &it : src )
		dst[it.first] = it.second;
}

void transferIfNotAlreadyThere( const Object::dictionary &src, Object::dictionary &dst,
							const std::unordered_set<std::string> &i_filter )
{
	for ( auto &it : src )
	{
		if ( i_filter.find( it.first ) == i_filter.end() and
		     dst.find( it.first ) == dst.end() )
		{
			dst[it.first] = it.second;
		}
	}
}


Object Parser::buildXRef( std::string &o_headerVersion )
{
	//! @todo: optimise, getLine is very slow

	_tokenizer.seekg( 0, std::ios_base::beg );
	o_headerVersion.clear();

	Object::dictionary trailerDict;

	std::string line;
	line.reserve( 1024 );
	std::streamoff pos = 0;
	
	while ( _tokenizer.getLine( line ) )
	{
		if ( line.compare( 0, 7, "trailer" ) == 0 )
		{
			//	read trailer
			_tokenizer.seekg( pos + 7, std::ios::beg );
			auto obj = readObject_priv( 0, 0 );
			transfer( obj.dictionary_items(), trailerDict );
		}
		else
		{
			int num, gen;
			if ( isObjDef( line, num, gen ) )
			{
				// make room in the xref table
				_doc->xrefTable().expand( num + 1 );

				int currentGen = _doc->xrefTable()[num].generation();
				if ( currentGen == -1 or currentGen <= gen )
					_doc->xrefTable()[num] = XRef( gen, (int)pos );
			}
			else if ( isHeaderLine( line, o_headerVersion ) )
			{
			}
		}
		pos = _tokenizer.tellg();
	}

	auto it = trailerDict.find( "XRefStm" );
	if ( it != trailerDict.end() and not it->second.is_null() )
	{
		auto &xrefStm = it->second;
		if ( xrefStm.is_number() )
		{
			_tokenizer.seekg( xrefStm.int_value(), std::ios_base::beg );
			std::streamoff xrefpos;
			readCrossReferenceStream( trailerDict, xrefpos );
			assert( xrefpos == -1 );
		}
	}

	return Object::create_dictionary( _doc, std::move(trailerDict) );
}

void Parser::readCrossReferenceTable( Object::dictionary &o_trailerDict,
                                          std::streamoff &o_xrefpos )
{
	// cross reference table should start with 2 integers: first object ID and
	// nb of objects
	Token token;
	_tokenizer.nextTokenForced( token );
	while ( token.type() == Token::tok_int )
	{
		Token token2, token3;
		_tokenizer.nextTokenForced( token2, Token::tok_int );
		int objRefNb = token.intValue();
		int nbOfObj = token2.intValue();

		// make room if our xref table is too small
		_doc->xrefTable().expand( objRefNb + nbOfObj );

		// for each entry
		for ( int i = objRefNb; i < ( objRefNb + nbOfObj ); ++i )
		{
			_tokenizer.nextTokenForced( token, Token::tok_int );
			_tokenizer.nextTokenForced( token2, Token::tok_int );
			_tokenizer.nextTokenForced( token3, Token::tok_command );
			if ( token3.value() == "n" )
			{
				size_t pos = token.intValue();
				int gen = token2.intValue();
				int currentGen = _doc->xrefTable()[i].generation();
				if ( currentGen == -1 or ( gen != 65535 and currentGen < gen ) )
					_doc->xrefTable()[i] = XRef( gen, (int)pos );
			}
			else if ( token3.value() == "f" )
			{
				int gen = token2.intValue();
				int currentGen = _doc->xrefTable()[i].generation();
				if ( currentGen == -1 or ( gen != 65535 and currentGen < gen ) )
					_doc->xrefTable()[i] = XRef( gen, -1 );
			}
			else
				_tokenizer.invalidToken( token3 );
		}
		_tokenizer.nextTokenForced( token );
	}
	if ( token.type() != Token::tok_trailer )
		_tokenizer.invalidToken( token );

	//	read trailer
	auto trailer = readObject_priv( 0, 0 );
	if ( trailer.is_dictionary() )
	{
		auto &v = trailer["Prev"];
		if ( v.is_number() )
			o_xrefpos = v.int_value();
		transferIfNotAlreadyThere( trailer.dictionary_items(), o_trailerDict,
												  {} );
	}
}

void Parser::readCrossReferenceStream( Object::dictionary &o_trailerDict,
                                           std::streamoff &o_xrefpos )
{
	// read object declaration: int int obj
	Token token;
	_tokenizer.nextTokenForced( token, Token::tok_int );
	int objID = token.intValue();
	_tokenizer.nextTokenForced( token, Token::tok_int );
	int objGen = token.intValue();
	_tokenizer.nextTokenForced( token, Token::tok_obj );

	// read the object, should be a stream
	auto streamObj = readObject_priv( objID, objGen );
	if ( not streamObj.is_stream() )
		throw std::runtime_error( "invalid cross reference stream" );

	auto &dict = streamObj.stream_dictionary();

	// check for a Prev
	auto &Prev = dict["Prev"];
	if ( Prev.is_number() )
		o_xrefpos = Prev.int_value();
	else
		o_xrefpos = -1;

	// parse the xref stream: Size, Index, W
	auto &Size = dict["Size"];
	if ( not Size.is_number() )
		throw std::runtime_error( "invalid cross reference stream" );
	int nbOfEntry = Size.int_value();

	std::vector<SubSection> index;
	auto &Index = dict["Index"];
	if ( Index.is_null() or not ArrayToVectorOfRange( Index, index ) )
	{
		index.push_back( SubSection{0, nbOfEntry} );
	}

	// W is an array that tell the width in bytes of each entry in the stream
	std::vector<int> w;
	auto &W = dict["W"];
	if ( not ArrayToVectorOfInt( W, w ) )
		throw std::runtime_error( "invalid cross reference stream" );

	auto data = streamObj.stream_data();

	size_t fieldsWidth = 0;
	for ( auto &it : w )
		fieldsWidth += it;

	su::stackarray<int, 4> fields( w.size() );
	su::stackarray<uint8_t, 16> buffer( fieldsWidth );
	for ( auto &range : index ) // for each range
	{
		for ( int i = range.objNb; i < range.objNb + range.nbOfEntry;
		      ++i ) // for each id in the range
		{
			if ( data->read( {buffer.data(), fieldsWidth} ) != fieldsWidth )
				break;
			uint8_t *ptr = buffer.data();

			// split the buffer
			size_t j = 0;
			for ( auto it = w.begin(); it != w.end(); ++it, ++j )
			{
				fields[j] = 0;
				for ( int k = 0; k < *it; ++k )
				{
					fields[j] = fields[j] << 8;
					fields[j] += *ptr++;
				}
			}

			switch ( fields[0] )
			{
				case 0: // not used
					break;
				case 1: // entry for a normal object
				{
					_doc->xrefTable().expand( i + 1 );
					if ( _doc->xrefTable()[i].generation() == -1 )
						_doc->xrefTable()[i] = XRef( fields[2], fields[1] );
					break;
				}
				case 2: // entry for an object in a compressed object stream
				{
					_doc->xrefTable().expand( i + 1 );
					if ( _doc->xrefTable()[i].generation() == -1 )
						_doc->xrefTable()[i] =
						    XRef( fields[1], fields[2], true );
					break;
				}
				default:
					log_warn() << "bad XRef stream";
					break;
			}
		}
	}

	// skip the stream specific entry
	transferIfNotAlreadyThere( dict.dictionary_items(), o_trailerDict,
		{
			"Length", "Filter", "DP", "DecodeParms",
			"Type", "W", "Index"
		} );
}

Object Parser::readObject( int i_id )
{
	try
	{
		if ( size_t( i_id ) < _doc->xrefTable().size() )
		{
			const XRef &one_xref = _doc->xrefTable()[i_id];
			if ( one_xref.pos() != -1 )
			{
				if ( one_xref.compressed() )
				{
					return readCompressedObject( one_xref.streamId(),
					                             one_xref.indexInStream() );
				}
				else
				{
					_tokenizer.seekg( one_xref.pos(), std::ios_base::beg );
					Token token;
					_tokenizer.nextTokenForced( token, Token::tok_int );
					if ( token.intValue() != i_id )
						_tokenizer.invalidToken( token );
					_tokenizer.nextTokenForced( token, Token::tok_int );
					if ( token.intValue() != one_xref.generation() )
						_tokenizer.invalidToken( token );
					_tokenizer.nextTokenForced( token, Token::tok_obj );
					auto obj = readObject_priv( i_id, one_xref.generation() );
					if ( not _tokenizer.nextTokenOptional( token,
					                                       Token::tok_endobj ) )
					{
						log_warn() << "missing endobj";
					}
					return obj;
				}
			}
		}
	}
	catch ( std::exception &ex )
	{
		log_warn() << ex.what();
	}
	return {};
}

Parser::ObjectStreamData Parser::readCompressedObjectStream(
    int i_streamId,
    std::vector<ObjectStreamIndex> &o_compressedObjectStreamIndex )
{
	auto compressedObjectStream = readObject( i_streamId );
	if ( not compressedObjectStream.is_stream() )
		return {};

	auto &compressedObjectStreamDict = compressedObjectStream.stream_dictionary();
	assert( not compressedObjectStreamDict.is_null() );

	auto &N = compressedObjectStreamDict["N"];
	if ( not N.is_number() )
		return {};
	int n = N.int_value();
	auto &First = compressedObjectStreamDict["First"];
	if ( not First.is_number() )
		return {};
	int first = First.int_value();

	auto data = compressedObjectStream.stream_data();
	std::unique_ptr<char[]> dataBuffer;
	size_t s = 0, cap = 0;
	for ( ;; )
	{
		uint8_t buffer[4096];
		auto len = data->read( {buffer, 4096} );
		if ( len <= 0 )
			break;

		if ( ( s + len ) > cap )
		{
			auto newCap = std::max<size_t>( s + len, cap * 2 );
			auto newData = std::make_unique<char[]>( newCap );
			memcpy( newData.get(), dataBuffer.get(), s );
			dataBuffer = std::move( newData );
			cap = newCap;
		}
		memcpy( dataBuffer.get() + s, buffer, len );
		s += len;
	}
	auto compressedObjectData = ObjectStreamData{std::move( dataBuffer ), s};

	su::membuf buf( compressedObjectData.data.get(),
	                compressedObjectData.data.get() + s );
	std::istream istr( &buf );
	Tokenizer tokenizer( istr );
	for ( int i = 0; i < n; ++i )
	{
		Token token1, token2;
		if ( tokenizer.nextTokenOptional( token1, Token::tok_int ) and
		     tokenizer.nextTokenOptional( token2, Token::tok_int ) )
			o_compressedObjectStreamIndex.push_back( ObjectStreamIndex{
			    token1.intValue(), (size_t)token2.intValue() + first} );
		else
			return ObjectStreamData{};
	}
	return compressedObjectData;
}

Object Parser::readCompressedObject( int i_streamId, int i_indexInStream )
{
	ObjectStreamData compressedObjectData;
	std::vector<ObjectStreamIndex>
	    compressedObjectStreamIndex; // maps ids -> offsets, use a vector
	                                 // because we need to preserve the order
	                                 // too

	auto compressedStream = _compressedObjects.find( i_streamId );
	if ( compressedStream == _compressedObjects.end() )
	{
		// init the stream
		compressedObjectData = readCompressedObjectStream(
		    i_streamId, compressedObjectStreamIndex );
		if ( compressedObjectData.data.get() == nullptr )
			return {};
		_compressedObjects[i_streamId].resize(
		    compressedObjectStreamIndex.size() );
		compressedStream = _compressedObjects.find( i_streamId );
	}

	if ( i_indexInStream >= compressedStream->second.size() )
		return {};

	auto obj = compressedStream->second[i_indexInStream];
	if ( obj.is_null() )
	{
		// missing object
		if ( compressedObjectData.data.get() != nullptr )
		{
			assert( compressedObjectStreamIndex.size() ==
			        compressedStream->second.size() );
			su::membuf buf(
			    compressedObjectData.data.get(),
			    compressedObjectData.data.get() + compressedObjectData.size );
			std::istream istr( &buf );
			Parser parser( istr, _doc ); // ?

			int index = 0;
			for ( auto &it : compressedObjectStreamIndex )
			{
				parser._tokenizer.seekg( it.offset, std::ios_base::beg );
				compressedStream->second[index++] =
				    parser.readObject_priv( 0, 0 );
			}
		}
	}

	return compressedStream->second[i_indexInStream];
}

Object Parser::readObject_priv( int i_id, int i_gen )
{
	Token token, token2;
	_tokenizer.nextTokenForced( token );
	switch ( token.type() )
	{
		case Token::tok_bool:
			return Object::create_boolean( token.boolValue() );
		case Token::tok_int:
		{
			int value = token.intValue();
			if ( _tokenizer.nextTokenOptional( token, Token::tok_int ) )
			{
				if ( _tokenizer.nextTokenOptional( token2,
				                                   Token::tok_command ) )
				{
					if ( token2.value() == "R" )
					{
						return Object::create_ref( value, token.intValue() );
					}
					_tokenizer.putBack( token2 );
				}
				_tokenizer.putBack( token );
			}
			return Object::create_number( value );
		}
		case Token::tok_float:
			return Object::create_number( token.floatValue() );
		case Token::tok_string:
			return Object::create_string(
			    _doc->decrypt( token.value(), i_id, i_gen ) );
		case Token::tok_name:
			return Object::create_name( token.value() );
		case Token::tok_openarray:
		{
			Object::array array;
			_tokenizer.nextTokenForced( token );
			while ( token.type() != Token::tok_closearray )
			{
				_tokenizer.putBack( token );
				array.push_back( readObject_priv( i_id, i_gen ) );
				_tokenizer.nextTokenForced( token );
			}
			return Object::create_array( _doc, std::move(array) );
		}
		case Token::tok_opendict:
		{
			Object::dictionary dict;
			_tokenizer.nextTokenForced( token );
			while ( token.type() != Token::tok_closedict )
			{
				_tokenizer.putBack( token );
				if ( not _tokenizer.nextTokenOptional( token,
				                                       Token::tok_name ) )
					break;
				dict[token.value()] = readObject_priv( i_id, i_gen );
				_tokenizer.nextTokenForced( token );
			}
			if ( _tokenizer.nextTokenOptional( token, Token::tok_stream ) )
			{
				size_t len, p = _tokenizer.tellg();
				try
				{
					auto it = dict.find( "Length" );
					len = resolveLength( it != dict.end() ? it->second : Object{} );
					_tokenizer.seekg( p + len, std::ios_base::beg );
					_tokenizer.nextTokenForced( token, Token::tok_endstream );
				}
				catch ( ... )
				{
					size_t originalLen = len;

					// search for endstream
					if ( not guessStreamLength( p, len ) )
						throw std::runtime_error( "invalid PDF file" );

					log_warn() << "invalid stream length of " << originalLen
					           << " for object " << i_id << " " << i_gen
					           << "; should be " << len;
					dict["Length"] = Object::create_number( (int)len );
				}
				return Object::create_stream(
				    Object::create_dictionary( _doc, std::move(dict) ), p, len, i_id, i_gen );
			}
			else
				return Object::create_dictionary( _doc, std::move(dict) );
		}
		case Token::tok_null:
			return {};
		default:
			break;
	}
	throw std::runtime_error( "invalid PDF file" );
	return {};
}

bool Parser::guessStreamLength( size_t i_streamStart, size_t &o_length )
{
	//! @todo: optimise, avoid reading one char at the time

	// look for [\r|\n]endstream[\r|\n]

	_tokenizer.seekg( i_streamStart, std::ios_base::beg );
	int state = 0; // 0 = start, 1 = [\r|\n], 2 = e, 3 = n, etc dstream,  11 =
	               // final [\r|\n]
	while ( _tokenizer and state != 11 )
	{
		char c = 0;
		_tokenizer.read( &c, 1 );
		if ( c == '\r' or c == '\n' )
		{
			state = state == 10 ? 11 : 1;
		}
		else
		{
			switch ( state )
			{
				case 0:
					break;
				case 1:
					state = c == 'e' ? 2 : 0;
					break;
				case 2:
					state = c == 'n' ? 3 : 0;
					break;
				case 3:
					state = c == 'd' ? 4 : 0;
					break;
				case 4:
					state = c == 's' ? 5 : 0;
					break;
				case 5:
					state = c == 't' ? 6 : 0;
					break;
				case 6:
					state = c == 'r' ? 7 : 0;
					break;
				case 7:
					state = c == 'e' ? 8 : 0;
					break;
				case 8:
					state = c == 'a' ? 9 : 0;
					break;
				case 9:
					state = c == 'm' ? 10 : 0;
					break;
				case 10:
					state = 0;
					break;
				default:
					break;
			}
		}
	}
	if ( state == 11 )
	{
		size_t current = _tokenizer.tellg();
		o_length = current - i_streamStart - 10;
		return true;
	}
	else
		return false;
}

size_t Parser::resolveLength( const Object &i_obj )
{
	try
	{
		if ( i_obj.is_null() )
			throw std::runtime_error( "missing length in stream dictionary" );

		auto obj = i_obj;
		if ( obj.is_ref() )
		{
			auto ref = obj.ref_value();

			if ( size_t( ref.ref ) >= _doc->xrefTable().size() or
			     _doc->xrefTable()[ref.ref].generation() != ref.gen )
				throw std::runtime_error( "invalid object ref for length in stream dictionary" );

			size_t p = _tokenizer.tellg();
			_tokenizer.seekg( _doc->xrefTable()[ref.ref].pos(),
			                  std::ios_base::beg );

			Token token;
			_tokenizer.nextTokenForced( token, Token::tok_int );
			if ( token.intValue() != ref.ref )
				throw std::runtime_error( "invalid object ref for length in stream dictionary" );
			_tokenizer.nextTokenForced( token, Token::tok_int );
			if ( token.intValue() != ref.gen )
				throw std::runtime_error( "invalid object ref for length in stream dictionary" );

			_tokenizer.nextTokenForced( token, Token::tok_obj );
			obj = readObject_priv( 0, 0 );
			_tokenizer.nextTokenForced( token, Token::tok_endobj );

			_tokenizer.seekg( p, std::ios_base::beg );
		}
		if ( obj.is_number() )
			return obj.int_value();
		else
			throw std::runtime_error( "invalid object ref for length in stream dictionary" );
	}
	catch ( std::exception &ex )
	{
		log_warn() << ex.what();
	}
	return 0;
}

std::streamoff Parser::read( size_t i_pos,
                                 su::array_view<uint8_t> o_buffer )
{
	_tokenizer.seekg( i_pos, std::ios_base::beg );
	return _tokenizer.read( (char *)o_buffer.data(), o_buffer.size() );
}

bool Parser::isEndOfStream( size_t i_pos )
{
	try
	{
		Token token;
		_tokenizer.seekg( i_pos, std::ios_base::beg );
		_tokenizer.nextTokenForced( token, Token::tok_endstream );
		_tokenizer.nextTokenForced( token, Token::tok_endobj );
	}
	catch ( ... )
	{
		return false;
	}
	return true;
}
}
