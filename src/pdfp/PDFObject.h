/*
 *  PDFObject.h
 *  pdfp
 *
 *  Created by Sandy Martel on Sat Apr 12 2003.
 *  Copyright (c) 2003 Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_PDFOBJECT
#define H_PDFP_PDFOBJECT

#include "pdfp/PDFData.h"
#include "su/containers/flat_map.h"
#include "su/base/platform.h"
#include "su/base/endian.h"
#include <vector>
#include <string>
#include <unordered_set>

namespace pdfp {

namespace details {
struct ObjectValue;
}
class Document;

class Object final
{
public:
	// Types
	enum Type
	{
		k_invalid,

		k_boolean,
		k_number,
		k_string,
		k_name,
		k_array,
		k_dictionary,
		k_stream,
		k_null,

		k_objectref
	};

	// Array, Dictionary and ref
	using array = std::vector<Object>;
	using dictionary = su::flat_map<std::string, Object, std::less<>>;
	struct ObjRef
	{
		int ref = -1;
		uint16_t gen = -1;

		bool operator==( const ObjRef &lhs ) const
		{
			return ref == lhs.ref and gen == lhs.gen;
		}
		bool operator<( const ObjRef &lhs ) const
		{
			return ref == lhs.ref ? gen < lhs.gen : ref < lhs.ref;
		}
	};

	~Object();

	Object( const Object &rhs ) noexcept;
	Object &operator=( const Object &rhs ) noexcept;
	Object( Object &&rhs ) noexcept;
	Object &operator=( Object &&rhs ) noexcept;

	// Constructors for the various types of Object.
	Object() noexcept {} // k_null

	static Object create_number( float value );
	static Object create_number( int value );
	static Object create_boolean( bool value );
	static Object create_name( const std::string_view &value );
	static Object create_string( const std::string_view &value );
	static Object create_array( Document *i_doc, const array &values );
	static Object create_array( Document *i_doc, array &&values );
	static Object create_dictionary( Document *i_doc,
	                                 const dictionary &values );
	static Object create_dictionary( Document *i_doc, dictionary &&values );
	static Object create_ref( int i_ref, uint16_t i_gen );

	// set to null
	void clear();

	Type type() const;

	bool is_null() const;
	bool is_boolean() const;
	bool is_number() const;
	bool is_name() const;
	bool is_string() const;
	bool is_array() const;
	bool is_dictionary() const;
	bool is_stream() const;
	bool is_ref() const;

	enum class NumberType
	{
		k_notANumber,
		k_integer,
		k_real
	};
	NumberType number_type() const;
	bool is_int() const;
	bool is_real() const;
	float real_value() const;
	int int_value() const;

	bool bool_value() const;
	std::string name_value() const;
	std::string string_value() const;
	std::string string_uvalue() const;

	const array &array_items() const;
	array array_items_resolved() const;

	const dictionary &dictionary_items() const;
	dictionary dictionary_items_resolved() const;

	const Object &stream_dictionary() const;
	DataStreamRef stream_data() const;

	ObjRef ref_value() const;

	// Return a reference to array[i] if this is an array, k_null otherwise.
	size_t array_size() const;
	const Object &operator[]( size_t i ) const;
	// Return a reference to dict[key] if this is a dictionary, k_null
	// otherwise.
	size_t dictionary_size() const;
	const Object &operator[]( const std::string_view &key ) const;

	bool operator==( const Object &rhs ) const;
	bool operator<( const Object &rhs ) const;
	bool operator!=( const Object &rhs ) const { return !( *this == rhs ); }
	bool operator<=( const Object &rhs ) const { return !( rhs < *this ); }
	bool operator>( const Object &rhs ) const { return ( rhs < *this ); }
	bool operator>=( const Object &rhs ) const { return !( *this < rhs ); }

	Document *document() const;

	void dump() const;
	
	size_t mem_size() const;

private:

#if defined( _WIN32 ) || defined( __LITTLE_ENDIAN__ )
#if UPLATFORM_64BIT
	using ObjectValuePtr = details::ObjectValue *;
#else
	// need to align the ptr at the other end
	struct ObjectValuePtr
	{
		uint32_t pad;
		details::ObjectValue *ptr;

		ObjectValuePtr &operator=( details::ObjectValue *p )
		{
			ptr = p;
			return *this;
		}
		details::ObjectValue *operator->() const { return ptr; }
	};
#endif

	// little endian
	struct split_storage
	{
		uint8_t tag;
		uint8_t u8;
		uint16_t u16;
		union
		{
			float r;
			int i;
			bool b;
		} u;
	};
	struct string_storage
	{
		uint8_t tag;
		char c[7];
	};
#else
	using ObjectValuePtr = details::ObjectValue *;

	// big endian
	struct split_storage
	{
		union
		{
			float r;
			int i;
			bool b;
		} u;
		uint16_t u16;
		uint8_t u8;
		uint8_t tag;
	};
	struct string_storage
	{
		char c[7];
		uint8_t tag;
	};

#endif

	union u
	{
		uint64_t data{0};
		split_storage split;
		string_storage string;
		ObjectValuePtr ptr;
	} _storage;
	static Object create_stream(
	    Object &&i_dict, size_t p, size_t len, int i_id, uint16_t i_gen );

	friend class Parser;
};
static_assert( sizeof( Object ) == sizeof( uint64_t ), "" );

extern Object kObjectNull;
}

#endif
