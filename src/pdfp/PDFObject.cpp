/*
 *  PDFObject.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 06-10-19.
 *  Copyright 2006 Sandy Martel. All rights reserved.
 *
 */

#include "pdfp/PDFObject.h"
#include "su/base/endian.h"
#include "su/strings/str_ext.h"
#include "pdfp/PDFDocument.h"
#include "impl/DataFactory.h"
#include <cassert>
#include <iostream>

namespace pdfp {

namespace details {

const char16_t PDFDocEncoding[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008,
    0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 0x0010, 0x0011,
    0x0012, 0x0013, 0x0014, 0x0015, 0x0017, 0x0017, 0x02D8, 0x02C7, 0x02C6,
    0x02D9, 0x02DD, 0x02DB, 0x02DA, 0x02DC, 0x0020, 0x0021, 0x0022, 0x0023,
    0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C,
    0x002D, 0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
    0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E,
    0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050,
    0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059,
    0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 0x0060, 0x0061, 0x0062,
    0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B,
    0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074,
    0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D,
    0x007E, 0x0000, 0x2022, 0x2020, 0x2021, 0x2026, 0x2014, 0x2013, 0x0192,
    0x2044, 0x2039, 0x203A, 0x2212, 0x2030, 0x201E, 0x201C, 0x201D, 0x2018,
    0x2019, 0x201A, 0x2122, 0xFB01, 0xFB02, 0x0141, 0x0152, 0x0160, 0x0178,
    0x017D, 0x0131, 0x0142, 0x0153, 0x0161, 0x017E, 0x0000, 0x20AC, 0x00A1,
    0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA,
    0x00AB, 0x00AC, 0x0255, 0x00AE, 0x00AF, 0x00B0, 0x00B1, 0x00B2, 0x00B3,
    0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC,
    0x00BD, 0x00BE, 0x00BF, 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5,
    0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE,
    0x00CF, 0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
    0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF, 0x00E0,
    0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9,
    0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x00F0, 0x00F1, 0x00F2,
    0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB,
    0x00FC, 0x00FD, 0x00FE, 0x00FF};

std::string decodeString( const std::string_view &i_data )
{
	auto i_len = i_data.size();
	// do I have the BOM UTF16-BE?
	std::u16string s16;
	if ( i_len >= 2 and uint8_t( i_data[0] ) == 0xFE and
	     uint8_t( i_data[1] ) == 0xFF )
	{
		auto ptr = (const char16_t *)i_data.data();
		++ptr;
		i_len = ( ( i_len - 2 ) / 2 );
		auto end = ptr + i_len;

		s16.reserve( i_len );
		bool inEscape = false;
		for ( ; ptr < end; ++ptr )
		{
			char16_t c = su::big_to_native( *ptr );
			if ( c == 0x001B )
			{
				// escape sequence for language
				inEscape = not inEscape;
				assert( false );
			}
			else if ( not inEscape )
				s16.push_back( c );
		}
	}
	else // PDFDocEncoding
	{
		auto ptr = (const uint8_t *)i_data.data();
		auto end = ptr + i_len;
		s16.reserve( i_len );
		for ( ; ptr < end; ++ptr )
			s16.push_back( PDFDocEncoding[*ptr] );
	}
	return su::to_string( s16 );
}

inline bool isPtr( uint64_t tag )
{
	return ( tag & 0x7 ) == 0 and tag != 0;
}

struct Statics
{
	const Object::array empty_array;
	const Object::dictionary empty_dictionary{};
	Statics() {}
};

const Statics &statics()
{
	static const Statics s{};
	return s;
}

struct alignas(8) ObjectValue
{
	ObjectValue( Object::Type i_type ) : type( i_type ) {}

	virtual ~ObjectValue() = default;

	mutable size_t refCount{1};
	void inc() const { ++refCount; }
	void dec() const
	{
		--refCount;
		if ( refCount == 0 )
			delete this;
	}

	const Object::Type type;

	void *operator new( std::size_t len ) { return ::malloc( len ); }
	void *operator new( std::size_t count, void *ptr ) { return ptr; }
	void operator delete( void *ptr )
	{
		if ( ptr != nullptr )
			::free( ptr );
	}
	
	virtual size_t mem_size() const = 0;
};
struct StringStorage final : ObjectValue
{
private:
	StringStorage( Object::Type i_type ) : ObjectValue( i_type ) {}

public:
	static StringStorage *alloc( Object::Type i_type,
	                             const std::string_view &i_value );

	size_t len;
	char buf[1];

	virtual size_t mem_size() const;
};
StringStorage *StringStorage::alloc( Object::Type i_type,
                                     const std::string_view &i_value )
{
	auto needed = sizeof( StringStorage ) + i_value.size();

	auto ptr = ::malloc( needed );
	auto ss = new ( ptr ) StringStorage( i_type );

	ss->len = i_value.size();
	std::copy( i_value.begin(), i_value.end(), ss->buf );
	ss->buf[ss->len] = 0;
	return ss;
}
size_t StringStorage::mem_size() const
{
	return sizeof( StringStorage ) + len;
}
struct Array : ObjectValue
{
	Document *_doc;
	Object::array value;
	Array( Document *i_doc, const Object::array &i_value ) :
	    ObjectValue( Object::Type::k_array ),
	    _doc( i_doc ),
	    value( i_value )
	{
	}
	Array( Document *i_doc, Object::array &&i_value ) :
	    ObjectValue( Object::Type::k_array ),
	    _doc( i_doc ),
	    value( std::move( i_value ) )
	{
	}
	virtual size_t mem_size() const;
};

size_t Array::mem_size() const
{
	size_t s = sizeof( Array );
	for ( auto &it : value )
		s += it.mem_size();
	return s;
}

struct Dictionary : ObjectValue
{
	Document *_doc;
	Object::dictionary value;
	Dictionary( Document *i_doc, const Object::dictionary &i_value ) :
	    ObjectValue( Object::Type::k_dictionary ),
	    _doc( i_doc ),
	    value( i_value )
	{
	}
	Dictionary( Document *i_doc, Object::dictionary &&i_value ) :
	    ObjectValue( Object::Type::k_dictionary ),
	    _doc( i_doc ),
	    value( std::move( i_value ) )
	{
	}
	virtual size_t mem_size() const;
};
size_t Dictionary::mem_size() const
{
	size_t s = sizeof( Dictionary );
	for ( auto &it : value )
		s += it.first.size() + it.second.mem_size();
	return s;
}

struct Stream : ObjectValue
{
	Object _dict;
	size_t _offset, _length;
	int _id;
	uint16_t _gen;
	Stream( Object &&i_dict, size_t p, size_t len, int i_id, uint16_t i_gen ) :
	    ObjectValue( Object::Type::k_stream ),
	    _dict( std::move( i_dict ) ),
	    _offset( p ),
	    _length( len ),
	    _id( i_id ),
	    _gen( i_gen )
	{
	}
	virtual size_t mem_size() const;
};
size_t Stream::mem_size() const
{
	return sizeof( Stream ) + _dict.mem_size();
}

const int kBoolTag = 1;
const int kIntTag = 2;
const int kRealTag = 3;
const int kRefTag = 4;
const int kNameTag = 5;
const int kStringTag = 6;
}

using namespace details;

Object kObjectNull;

Object Object::create_number( float value )
{
	Object obj;
	obj._storage.split.tag = kRealTag;
	obj._storage.split.u.r = value;
	return obj;
}
Object Object::create_number( int value )
{
	Object obj;
	obj._storage.split.tag = kIntTag;
	obj._storage.split.u.i = value;
	return obj;
}
Object Object::create_boolean( bool value )
{
	Object obj;
	obj._storage.split.tag = kBoolTag;
	obj._storage.split.u.b = value;
	return obj;
}
Object Object::create_name( const std::string_view &value )
{
	Object obj;
	if ( value.size() < 7 and value.find( '\0' ) == std::string_view::npos )
	{
		obj._storage.string.tag = kNameTag;
		std::copy( value.begin(), value.end(), obj._storage.string.c );
		obj._storage.string.c[value.size()] = 0;
	}
	else
	{
		obj._storage.ptr = StringStorage::alloc( Type::k_name, value );
	}
	return obj;
}
Object Object::create_string( const std::string_view &value )
{
	Object obj;
	if ( value.size() < 7 and value.find( '\0' ) == std::string_view::npos )
	{
		obj._storage.string.tag = kStringTag;
		std::copy( value.begin(), value.end(), obj._storage.string.c );
		obj._storage.string.c[value.size()] = 0;
	}
	else
	{
		obj._storage.ptr = StringStorage::alloc( Type::k_string, value );
	}
	return obj;
}
Object Object::create_array( Document *i_doc, const array &values )
{
	Object obj;
	obj._storage.ptr = new Array( i_doc, values );
	return obj;
}
Object Object::create_array( Document *i_doc, array &&values )
{
	Object obj;
	obj._storage.ptr = new Array( i_doc, std::move( values ) );
	return obj;
}
Object Object::create_dictionary( Document *i_doc, const dictionary &values )
{
	Object obj;
	obj._storage.ptr = new Dictionary( i_doc, values );
	return obj;
}
Object Object::create_dictionary( Document *i_doc, dictionary &&values )
{
	Object obj;
	obj._storage.ptr = new Dictionary( i_doc, std::move( values ) );
	return obj;
}
Object Object::create_ref( int i_ref, uint16_t i_gen )
{
	Object obj;
	obj._storage.split.tag = kRefTag;
	obj._storage.split.u.i = i_ref;
	obj._storage.split.u16 = i_gen;
	return obj;
}
Object Object::create_stream(
    Object &&i_dict, size_t p, size_t len, int i_id, uint16_t i_gen )
{
	Object obj;
	obj._storage.ptr = new Stream( std::move( i_dict ), p, len, i_id, i_gen );
	return obj;
}

Object::~Object()
{
	if ( isPtr( _storage.data ) )
		_storage.ptr->dec();
}

Object::Object( const Object &rhs ) noexcept
{
	_storage.data = rhs._storage.data;
	if ( isPtr( _storage.data ) )
		_storage.ptr->inc();
}
Object &Object::operator=( const Object &rhs ) noexcept
{
	if ( this != &rhs )
	{
		if ( isPtr( rhs._storage.data ) )
			rhs._storage.ptr->inc();
		if ( isPtr( _storage.data ) )
			_storage.ptr->dec();
		_storage.data = rhs._storage.data;
	}
	return *this;
}
Object::Object( Object &&rhs ) noexcept
{
	_storage.data = std::exchange( rhs._storage.data, 0 );
}
Object &Object::operator=( Object &&rhs ) noexcept
{
	if ( this != &rhs )
	{
		if ( _storage.data != rhs._storage.data )
		{
			// different storage
			std::swap( _storage.data, rhs._storage.data );
		}
		rhs.clear();
	}
	return *this;
}

Object::Type Object::type() const
{
	if ( _storage.data == 0 )
		return Type::k_null;
	if ( isPtr( _storage.data ) )
		return _storage.ptr->type;
	switch ( _storage.data & 0x7 )
	{
		case kBoolTag:
			return Type::k_boolean;
		case kIntTag:
		case kRealTag:
			return Type::k_number;
		case kRefTag:
			return Type::k_objectref;
		case kNameTag:
			return Type::k_name;
		case kStringTag:
			return Type::k_string;
		default:
			break;
	}
	assert( false );
	return Type::k_null;
}

bool Object::is_null() const
{
	return _storage.data == 0;
}
bool Object::is_boolean() const
{
	return ( _storage.data & 0x7 ) == kBoolTag;
}
bool Object::is_number() const
{
	return ( _storage.data & 0x7 ) == kIntTag or
	       ( _storage.data & 0x7 ) == kRealTag;
}
bool Object::is_name() const
{
	if ( isPtr( _storage.data ) )
		return _storage.ptr->type == Type::k_name;
	return ( _storage.data & 0x7 ) == kNameTag;
}
bool Object::is_string() const
{
	if ( isPtr( _storage.data ) )
		return _storage.ptr->type == Type::k_string;
	return ( _storage.data & 0x7 ) == kStringTag;
}
bool Object::is_array() const
{
	return isPtr( _storage.data ) and _storage.ptr->type == Type::k_array;
}
bool Object::is_dictionary() const
{
	return isPtr( _storage.data ) and _storage.ptr->type == Type::k_dictionary;
}
bool Object::is_stream() const
{
	return isPtr( _storage.data ) and _storage.ptr->type == Type::k_stream;
}
bool Object::is_ref() const
{
	return ( _storage.data & 0x7 ) == kRefTag;
}

Object::NumberType Object::number_type() const
{
	switch ( _storage.data & 0x7 )
	{
		case kIntTag:
			return NumberType::k_integer;
		case kRealTag:
			return NumberType::k_real;
		default:
			break;
	}
	return NumberType::k_notANumber;
}
bool Object::is_int() const
{
	return ( _storage.data & 0x7 ) == kIntTag;
}
bool Object::is_real() const
{
	return ( _storage.data & 0x7 ) == kRealTag;
}

void Object::clear()
{
	if ( isPtr( _storage.data ) )
		_storage.ptr->dec();
	_storage.data = 0;
}

float Object::real_value() const
{
	switch ( _storage.data & 0x7 )
	{
		case kIntTag:
			return (float)_storage.split.u.i;
		case kRealTag:
			return _storage.split.u.r;
		default:
			break;
	}
	return 0;
}
int Object::int_value() const
{
	switch ( _storage.data & 0x7 )
	{
		case kIntTag:
			return _storage.split.u.i;
		case kRealTag:
			return (int)_storage.split.u.r;
		default:
			break;
	}
	return 0;
}

bool Object::bool_value() const
{
	if ( ( _storage.data & 0x7 ) == kBoolTag )
		return _storage.split.u.b;
	return false;
}

std::string Object::name_value() const
{
	if ( isPtr( _storage.data ) and _storage.ptr->type == Type::k_name )
	{
		return {( (StringStorage *)_storage.ptr )->buf,
		        ( (StringStorage *)_storage.ptr )->len};
	}
	else if ( ( _storage.data & 0x7 ) == kNameTag )
	{
		return _storage.string.c;
	}
	return {};
}
std::string Object::string_value() const
{
	if ( isPtr( _storage.data ) and _storage.ptr->type == Type::k_string )
	{
		return {( (StringStorage *)_storage.ptr )->buf,
		        ( (StringStorage *)_storage.ptr )->len};
	}
	else if ( ( _storage.data & 0x7 ) == kStringTag )
	{
		return _storage.string.c;
	}
	return {};
}
std::string Object::string_uvalue() const
{
	if ( isPtr( _storage.data ) and _storage.ptr->type == Type::k_string )
	{
		return decodeString( {( (StringStorage *)_storage.ptr )->buf,
		                      ( (StringStorage *)_storage.ptr )->len} );
	}
	else if ( ( _storage.data & 0x7 ) == kStringTag )
	{
		return decodeString( _storage.string.c );
	}
	return {};
}
const Object::array &Object::array_items() const
{
	if ( is_array() )
		return ( (Array *)_storage.ptr )->value;
	return statics().empty_array;
}
Object::array Object::array_items_resolved() const
{
	Object::array r;
	if ( is_array() )
	{
		auto arr = (Array *)_storage.ptr;
		for ( auto &it : arr->value )
			r.push_back( arr->_doc->resolveIndirect( it ) );
	}
	return r;
}
const Object::dictionary &Object::dictionary_items() const
{
	switch ( type() )
	{
		case Type::k_dictionary:
			return ( (Dictionary *)_storage.ptr )->value;
			break;
		case Type::k_stream:
			return ( (Stream *)_storage.ptr )->_dict.dictionary_items();
			break;
		default:
			break;
	}
	return statics().empty_dictionary;
}
Object::dictionary Object::dictionary_items_resolved() const
{
	Object::dictionary r;
	switch ( type() )
	{
		case Type::k_dictionary:
		{
			auto dict = (Dictionary *)_storage.ptr;
			r.reserve( dict->value.size() );
			for ( auto &it : dict->value )
				r[it.first] = dict->_doc->resolveIndirect( it.second );
			break;
		}
		case Type::k_stream:
			return ( (Stream *)_storage.ptr )
			    ->_dict.dictionary_items_resolved();
			break;
		default:
			break;
	}
	return r;
}
const Object &Object::stream_dictionary() const
{
	if ( is_stream() )
		return ( (Stream *)_storage.ptr )->_dict;
	return kObjectNull;
}
DataStreamRef Object::stream_data() const
{
	if ( is_stream() )
	{
		auto v = (Stream *)_storage.ptr;
		return createDataStream(
		    v->_dict, v->_offset, v->_length, v->_id, v->_gen );
	}
	return {};
}

Document *Object::document() const
{
	switch ( type() )
	{
		case Type::k_array:
			return ( (Array *)_storage.ptr )->_doc;
		case Type::k_dictionary:
			return ( (Dictionary *)_storage.ptr )->_doc;
		case Type::k_stream:
			return ( (Stream *)_storage.ptr )->_dict.document();
		default:
			break;
	}
	return nullptr;
}

Object::ObjRef Object::ref_value() const
{
	if ( is_ref() )
		return {_storage.split.u.i, _storage.split.u16};
	return {};
}

size_t Object::array_size() const
{
	if ( is_array() )
	{
		auto arr = (Array *)_storage.ptr;
		return arr->value.size();
	}
	return 0;
}
const Object &Object::operator[]( size_t i ) const
{
	if ( is_array() )
	{
		auto arr = (Array *)_storage.ptr;
		if ( i < arr->value.size() )
			return arr->_doc->resolveIndirect( arr->value[i] );
	}
	return kObjectNull;
}
size_t Object::dictionary_size() const
{
	switch ( type() )
	{
		case Type::k_dictionary:
			return ( (Dictionary *)_storage.ptr )->value.size();
			break;
		case Type::k_stream:
			return ( (Stream *)_storage.ptr )->_dict.dictionary_size();
			break;
		default:
			break;
	}
	return 0;
}
const Object &Object::operator[]( const std::string_view &key ) const
{
	switch ( type() )
	{
		case Type::k_dictionary:
		{
			auto dict = (Dictionary *)_storage.ptr;
			auto it = dict->value.find( key );
			if ( it != dict->value.end() )
				return dict->_doc->resolveIndirect( it->second );
			break;
		}
		case Type::k_stream:
			return ( (Stream *)_storage.ptr )->_dict[key];
			break;
		default:
			break;
	}
	return kObjectNull;
}

bool Object::operator==( const Object &rhs ) const
{
	if ( type() == rhs.type() )
	{
		switch ( type() )
		{
			case Type::k_null:
				return true;
			case Type::k_number:
				switch ( number_type() )
				{
					case NumberType::k_real:
						switch ( rhs.number_type() )
						{
							case NumberType::k_real:
								return _storage.split.u.r ==
								       rhs._storage.split.u.r;
							case NumberType::k_integer:
								return _storage.split.u.r ==
								       rhs._storage.split.u.i;
							default:
								assert( false );
								break;
						}
						break;
					case NumberType::k_integer:
						switch ( rhs.number_type() )
						{
							case NumberType::k_real:
								return _storage.split.u.i ==
								       rhs._storage.split.u.r;
							case NumberType::k_integer:
								return _storage.split.u.i ==
								       rhs._storage.split.u.i;
							default:
								assert( false );
								break;
						}
						break;
					default:
						assert( false );
						break;
				}
				break;
			case Type::k_boolean:
				return _storage.split.u.b == rhs._storage.split.u.b;
			case Type::k_name:
				return name_value() == rhs.name_value();
			case Type::k_string:
				return string_value() == rhs.string_value();
			case Type::k_array:
				return array_items() == rhs.array_items();
			case Type::k_dictionary:
				return dictionary_items() == rhs.dictionary_items();
			case Type::k_stream:
			{
				auto lhss = (Stream *)_storage.ptr;
				auto rhss = (Stream *)rhs._storage.ptr;
				return lhss->_offset == rhss->_offset and
				       lhss->_length == rhss->_length and
				       lhss->_id == rhss->_id and lhss->_gen == rhss->_gen and
				       lhss->_dict == rhss->_dict;
			}
			break;
			case Type::k_objectref:
				return ref_value() == rhs.ref_value();
			default:
				assert( false );
				break;
		}
	}
	return false;
}

bool Object::operator<( const Object &rhs ) const
{
	if ( type() == rhs.type() )
	{
		switch ( type() )
		{
			case Type::k_null:
				return false;
			case Type::k_number:
				switch ( number_type() )
				{
					case NumberType::k_real:
						switch ( rhs.number_type() )
						{
							case NumberType::k_real:
								return _storage.split.u.r <
								       rhs._storage.split.u.r;
							case NumberType::k_integer:
								return _storage.split.u.r <
								       rhs._storage.split.u.i;
							default:
								assert( false );
								break;
						}
						break;
					case NumberType::k_integer:
						switch ( rhs.number_type() )
						{
							case NumberType::k_real:
								return _storage.split.u.i <
								       rhs._storage.split.u.r;
							case NumberType::k_integer:
								return _storage.split.u.i <
								       rhs._storage.split.u.i;
							default:
								assert( false );
								break;
						}
						break;
					default:
						assert( false );
						break;
				}
				break;
			case Type::k_boolean:
				return _storage.split.u.b < rhs._storage.split.u.b;
			case Type::k_name:
				return name_value() < rhs.name_value();
			case Type::k_string:
				return string_value() < rhs.string_value();
			case Type::k_array:
				return array_items() < rhs.array_items();
			case Type::k_dictionary:
				return dictionary_items() < rhs.dictionary_items();
			case Type::k_stream:
			{
				auto lhss = (Stream *)_storage.ptr;
				auto rhss = (Stream *)rhs._storage.ptr;
				return std::tie( lhss->_offset,
				                 lhss->_length,
				                 lhss->_id,
				                 lhss->_gen,
				                 lhss->_dict ) < std::tie( rhss->_offset,
				                                           rhss->_length,
				                                           rhss->_id,
				                                           rhss->_gen,
				                                           rhss->_dict );
				break;
			}
			case Type::k_objectref:
				return ref_value() < rhs.ref_value();
			default:
				assert( false );
				break;
		}
	}
	return type() < rhs.type();
}

void Object::dump() const
{
	switch ( type() )
	{
		case Type::k_null:
			std::cout << "null";
			break;
		case Type::k_number:
			switch ( number_type() )
			{
				case NumberType::k_real:
					std::cout << real_value();
					break;
				case NumberType::k_integer:
					std::cout << int_value();
					break;
				default:
					assert( false );
					break;
			}
			break;
		case Type::k_boolean:
			std::cout << ( bool_value() ? "true" : "false" );
			break;
		case Type::k_name:
			std::cout << name_value();
			break;
		case Type::k_string:
			std::cout << string_value();
			break;
		case Type::k_array:
			std::cout << "[ ";
			for ( auto &it : array_items() )
			{
				it.dump();
				std::cout << " ";
			}
			std::cout << "]";
			break;
		case Type::k_dictionary:
			std::cout << "<<\n";
			for ( auto &it : dictionary_items() )
			{
				std::cout << it.first << ":";
				it.second.dump();
				std::cout << "\n";
			}
			std::cout << ">>";
			break;
		case Type::k_objectref:
		{
			auto ref = ref_value();
			std::cout << "(" << ref.ref << "," << ref.gen << ")";
			break;
		}
		case Type::k_stream:
		{
			std::cout << "stream\n";
			stream_dictionary().dump();
			auto s = (Stream *)_storage.ptr;
			std::cout << "ref: " << s->_id << " " << s->_gen << "\n";
			std::cout << "pos: " << s->_offset << " " << s->_length << "\n";
			std::cout << "endstream\n";
			break;
		}
		default:
			assert( false );
			break;
	}
}

size_t Object::mem_size() const
{
	if ( isPtr( _storage.data ) )
		return sizeof( Object ) + _storage.ptr->mem_size();
	return sizeof( Object );
}

}
