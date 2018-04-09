#ifndef H_PDFP_TEST_DUMPER
#define H_PDFP_TEST_DUMPER

#include <iostream>
#include <unordered_set>
#include "pdfp/crypto/md5.h"

template<typename T>
class pdf_dumper
{
private:
	std::ostream &ostr;
	std::unordered_set<uintptr_t> _visitedDict;
	std::unordered_set<uintptr_t> _visitedArray;
	std::unordered_set<uintptr_t> _visitedStream;

	void dumpString( const std::string_view &v )
	{
		for ( auto &it : v )
		{
			uint8_t v = (uint8_t)it;
			int h = ( v & 0xF0 ) >> 4;
			int l = v & 0x0F;
			ostr << char( h < 10 ? h + '0' : 'A' + h - 10 );
			ostr << char( l < 10 ? l + '0' : 'A' + l - 10 );
		}
	}

	template<typename U>
	std::string extractFilters( const U &i_object )
	{
		std::string res;
		switch ( type( i_object ) )
		{
			case pdfp::Object::Type::k_name:
				res = nameValue( i_object );
				break;
			case pdfp::Object::Type::k_array:
			{
				auto v = asArray( i_object );
				for ( size_t i = 0; i < arrayCount( v ); ++i )
				{
					auto f = arrayValue( v, i );
					if ( not res.empty() )
						res += "-";
					res += extractFilters( f );
				}
				break;
			}
			default:
				break;
		}
		return res;
	}
	template<typename U>
	void dumpDict( U i_dict,
	               std::string *o_filter = nullptr,
	               ssize_t extraLength = -1 )
	{
		if ( isNull( i_dict ) )
			return;

		if ( _visitedDict.find( hashKey(i_dict) ) != _visitedDict.end() )
		{
			ostr << "visted dict\n";
			return;
		}
		_visitedDict.insert( hashKey(i_dict) );

		auto ks = keys( i_dict );

		bool useExtraLength = (extraLength >= 0);
		if ( useExtraLength and
		     std::find( ks.begin(), ks.end(), "Length" ) == ks.end() )
		{
			ks.emplace_back( "Length" );
			useExtraLength = true;
		}
		std::sort( ks.begin(), ks.end() );

		ostr << "<<\n";
		for ( auto &it : ks )
		{
			if ( useExtraLength and it == "Length" )
				ostr << it << "\ni: " << extraLength << "\n";
			else
			{
				auto v = dictValue( i_dict, it );
				if ( not isNull( v ) ) // some libs skip null entries
				{
					ostr << it << "\n";
					dumpObject( v );
					if ( o_filter != nullptr and it == "Filter" )
						*o_filter = extractFilters( v );
				}
			}
		}
		ostr << ">>\n";
	}
	template<typename U>
	void dumpArray( U i_array )
	{
		if ( _visitedArray.find( hashKey(i_array) ) != _visitedArray.end() )
		{
			ostr << "visted array\n";
			return;
		}
		_visitedArray.insert( hashKey(i_array) );

		ostr << "[\n";
		for ( size_t i = 0; i < arrayCount( i_array ); ++i )
		{
			auto v = arrayValue( i_array, i );
			dumpObject( v );
		}
		ostr << "]\n";
	}

	std::tuple<std::string, size_t> md5Checksum( const pdfp::Data &i_data )
	{
		pdfp::MD5_CTX c;
		pdfp::MD5_Init( &c );

		pdfp::MD5_Update( &c, i_data.buffer.get(), i_data.length );
		unsigned char md[pdfp::MD5_DIGEST_LENGTH];
		pdfp::MD5_Final( md, &c );

		auto res = std::to_string( i_data.length );
		res += ", ";

		for ( int i = 0; i < pdfp::MD5_DIGEST_LENGTH; ++i )
		{
			int hi = ( md[i] & 0xF0 ) >> 4;
			int lo = ( md[i] & 0x0F );
			res.push_back( hi < 10 ? hi + '0' : 'A' + hi - 10 );
			res.push_back( lo < 10 ? lo + '0' : 'A' + lo - 10 );
		}
		return {res, i_data.length};
	}
	std::string formatToString( pdfp::data_format_t i_format )
	{
		switch ( i_format )
		{
			case pdfp::data_format_t::kRaw:
				return "raw";
			case pdfp::data_format_t::kJPEGEncoded:
				return "jpeg";
			case pdfp::data_format_t::kJPEG2000:
				return "jp2000";
			default:
				break;
		}
		return "invalid";
	}
	template<typename U>
	void dumpStream( U i_stream )
	{
		if ( _visitedStream.find( hashKey(i_stream) ) !=
		     _visitedStream.end() )
		{
			ostr << "visted stream\n";
			return;
		}
		_visitedStream.insert( hashKey(i_stream) );

		ostr << "stream\n";
		auto dict = streamDictionary( i_stream );
		std::string filter;
		auto data = streamData( i_stream );
		auto sd = md5Checksum( data );

		dumpDict( dict, &filter, std::get<1>( sd ) );
		ostr << "filter: " << filter;
		ostr << " format: " << formatToString( data.format );
		ostr << " data: " << std::get<0>( sd ) << "\n";
		ostr << "endstream\n";
	}
	template<typename U>
	void dumpObject( U i_object )
	{
		if ( isNull( i_object ) )
		{
			ostr << "null\n";
			return;
		}
		switch ( type( i_object ) )
		{
			case pdfp::Object::Type::k_null:
				ostr << "null\n";
				break;
			case pdfp::Object::Type::k_boolean:
			{
				bool v = boolValue( i_object );
				ostr << ( v ? "true\n" : "false\n" );
				break;
			}
			case pdfp::Object::Type::k_number:
			{
				if ( numberIsInt( i_object ) )
				{
					int v = intValue( i_object );
					ostr << "i: " << v << "\n";
				}
				else
				{
					float v = floatValue( i_object );
					int i = (v * 100);
					ostr << "r: " << i << "\n";
				}
				break;
			}
			case pdfp::Object::Type::k_name:
			{
				auto v = nameValue( i_object );
				ostr << "/" << v << "\n";
				break;
			}
			case pdfp::Object::Type::k_string:
			{
				auto v = stringValue( i_object );
				ostr << "<";
				dumpString( v );
				ostr << ">\n";
				break;
			}
			case pdfp::Object::Type::k_array:
			{
				auto v = asArray( i_object );
				dumpArray( v );
				break;
			}
			case pdfp::Object::Type::k_dictionary:
			{
				auto v = asDictionary( i_object );
				dumpDict( v );
				break;
			}
			case pdfp::Object::Type::k_stream:
			{
				auto v = asStream( i_object );
				dumpStream( v );
				break;
			}
			default:
				throw std::runtime_error( "invalid object type" );
				break;
		}
	}

public:
	pdf_dumper( std::ostream &i_ostr ) : ostr( i_ostr ) {}

	void dump( T doc )
	{
		_visitedDict.clear();
		_visitedArray.clear();
		_visitedStream.clear();
		auto v = version( doc );
		ostr << "version: " << v.major << "." << v.minor << "\n";
		if ( isEncrypted( doc ) )
			ostr << "encrypted\n";
		if ( isUnlocked( doc ) )
		{
			auto nb = nbOfPages( doc );
			ostr << "nb of pages: " << nb << "\n";

			ostr << "info:\n";
			dumpDict( info( doc ) );

			ostr << "catalog:\n";
			dumpDict( catalog( doc ) );
			ostr << "end\n";
		}
		else
			ostr << "locked!\n";
	}
};

template<typename T>
void dumpDocument( T doc, std::ostream &ostr )
{
	pdf_dumper<T> d( ostr );
	d.dump( doc );
}

#endif
