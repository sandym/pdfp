/*
 *  PDFDocument.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 06-10-20.
 *  Copyright 2006 Sandy Martel. All rights reserved.
 *
 */

#include "pdfp/PDFDocument.h"
#include <string>
#include <regex>
#include <cassert>
#include "impl/Parser.h"
#include "security/SecurityHandler.h"
#include "su/log/logger.h"

namespace {

int iterateCountPages( const pdfp::Object &i_pages )
{
	int count = 0;
	if ( i_pages.is_null() )
		return count;

	auto &kids = i_pages["Kids"];
	if ( kids.is_null() )
		return 1;

	if ( not kids.is_array() )
		return count;

	for ( auto &kid : kids.array_items_resolved() )
	{
		if ( kid.is_null() )
			continue;
		if ( not kid.is_dictionary() )
			continue;
	}

	return count;
}

std::string parseName( const char *&ptr )
{
	const char *start = ptr;
	while ( *ptr != 0 and *ptr != '/' and *ptr != '>' and *ptr != '[' )
		++ptr;
	return std::string( start, ptr );
}

enum path_info_result
{
	pdfp_error,
	pdfp_object,
	pdfp_string,
	pdfp_int,
	pdfp_float,
};

path_info_result resolvePath( const pdfp::Document *i_doc,
                              pdfp::Object &o_object,
                              std::string *o_string,
                              int *o_int,
                              float *o_float,
                              const std::string &i_path )
{
	path_info_result res = pdfp_error;

	//	prefix:
	//		pages[d]
	//		info
	//		catalog or /
	//		version
	//	suffix:
	//		>count			for array and dictionary
	//		>keys[d]		for dictionary
	//		[d]				for array and dictionary
	//		>dict			for stream
	if ( i_path == "version" )
	{
		auto v = i_doc->version();
		if ( o_int != nullptr )
		{
			res = pdfp_int;
			*o_int = v.major;
		}
		if ( o_string != nullptr )
		{
			res = pdfp_string;
			char buf[32];
			sprintf( buf, "%d.%d", v.major, v.minor );
			o_string->assign( buf );
		}
		if ( o_float != nullptr )
		{
			res = pdfp_float;
			*o_float = float( v.major ) + ( float( v.minor ) / 10.0 );
		}
	}
	else if ( i_path == "encrypted" )
	{
		if ( o_int != nullptr )
		{
			res = pdfp_int;
			*o_int = i_doc->isEncrypted();
		}
	}
	else if ( i_path == "locked" )
	{
		if ( o_int != nullptr )
		{
			res = pdfp_int;
			*o_int = i_doc->isUnlocked();
		}
	}
	else if ( i_path == "pages>count" )
	{
		if ( o_float != nullptr )
		{
			res = pdfp_float;
			*o_float = i_doc->nbOfPages();
		}
		if ( o_int != nullptr )
		{
			res = pdfp_int;
			*o_int = (int)i_doc->nbOfPages();
		}
	}
	else
	{
		auto ptr = i_path.c_str();
		auto current = i_doc->catalog();
		if ( strncmp( ptr, "catalog", 7 ) == 0 )
		{
			current = i_doc->catalog();
			ptr += 7;
		}
		else if ( strncmp( ptr, "info", 4 ) == 0 )
		{
			current = i_doc->info();
			ptr += 4;
		}
		else if ( strncmp( ptr, "pages[", 6 ) == 0 )
		{
			ptr += 6;
			if ( isdigit( *ptr ) )
			{
				auto page = i_doc->page( std::stoi( ptr ) );
				if ( not page.is_null() )
				{
					while ( isdigit( *ptr ) )
						++ptr;
					if ( ptr[0] == ']' )
					{
						++ptr;
						current = page.dictionary();
					}
				}
			}
		}
		while ( ptr[0] != 0 and not current.is_null() )
		{
			if ( ptr[0] == '/' )
			{
				if ( current.is_stream() )
					current = current.stream_dictionary();
				if ( not current.is_dictionary() )
					current.clear();
				else
				{
					++ptr;
					auto name = parseName( ptr );
					current = current[name];
				}
			}
			else if ( ptr[0] == '[' )
			{
				if ( not current.is_array() )
					current.clear();
				else
				{
					++ptr;
					if ( not isdigit( *ptr ) )
						current.clear();
					else
					{
						long index = std::stol( ptr );
						while ( isdigit( *ptr ) )
							++ptr;
						if ( ptr[0] != ']' )
							current.clear();
						else
						{
							++ptr;
							current = current[index];
						}
					}
				}
			}
			else if ( ptr[0] == '>' )
			{
				++ptr;
				if ( strcmp( ptr, "count" ) == 0 )
				{
					if ( current.is_stream() )
						current = current.stream_dictionary();
					if ( current.is_dictionary() )
					{
						if ( o_int != nullptr )
						{
							res = pdfp_int;
							*o_int = (int)current.dictionary_size();
							current.clear();
						}
					}
					else if ( current.is_array() )
					{
						if ( o_int != nullptr )
						{
							res = pdfp_int;
							*o_int = (int)current.array_size();
							current.clear();
						}
					}
					else
						current.clear();
				}
				else if ( strncmp( ptr, "keys[", 5 ) == 0 )
				{
					if ( current.is_stream() )
						current = current.stream_dictionary();
					if ( not current.is_dictionary() )
						current.clear();
					else
					{
						ptr += 5;
						if ( not isdigit( *ptr ) )
							current.clear();
						else
						{
							long index = std::stol( ptr );
							while ( isdigit( *ptr ) )
								++ptr;
							if ( strcmp( ptr, "]" ) != 0 )
								current.clear();
							else
							{
								if ( index >= 0 and
								     ( (size_t)index ) < current.dictionary_size() )
								{
									res = pdfp_string;
									if ( o_string != nullptr )
									{
										auto &items = current.dictionary_items();
										auto it = items.begin();
										std::advance( it, index );
										*o_string = it->first;
									}
								}
								current.clear();
							}
						}
					}
				}
				else if ( strncmp( ptr, "dict", 4 ) == 0 )
				{
					if ( not current.is_stream() )
						current.clear();
					else
					{
						ptr += 4;
						current = current.stream_dictionary();
					}
				}
			}
		}
		if ( not current.is_null() )
		{
			res = pdfp_object;
			o_object = current;
		}
	}
	return res;
}
}

namespace pdfp {

Document::Document() {}
Document::~Document() {}

void Document::open( const std::string &i_path )
{
	_stream.open( i_path, std::ios_base::in | std::ios_base::binary );
	if ( not _stream )
	{
		// non-readable stream ?
		throw std::runtime_error( "cannot read PDF file" );
	}

	try
	{
		_parser = std::make_unique<Parser>( _stream, this );

		// try twice: first, proper parsing of the file, if that fails, try to
		// rebuild the file by scaning all of it
		for ( int i = 0; i < 2; ++i )
		{
			try
			{
				if ( i == 0 )
					_trailerDict = _parser->readXRef( _version );
				else
					_trailerDict = _parser->buildXRef( _version );
				break;
			}
			catch ( std::exception &ex )
			{
				// reset
				_trailerDict.clear();
				_xrefTable.clear();
				_parser->cleanup();

				if ( i == 0 )
					log_warn() << "damaged file, reconstructing xref table";
				else
				{
					log_warn() << ex.what();
					throw;
				}
			}
		}
		
		// check encryption
		auto &Encrypt = _trailerDict["Encrypt"];
		if ( not Encrypt.is_null() )
		{
			if ( Encrypt.is_dictionary() )
			{
				// required entries
				auto &obj = Encrypt["Filter"];
				if ( not obj.is_name() )
					throw std::runtime_error( "invalid Encrypt dictionary" );

				auto filter = obj.name_value();
				_securityHandler =
				    SecurityHandler::create( filter, Encrypt );
				if ( _securityHandler.get() != nullptr )
					_securityHandler->unlock( this, "" );
			}
		}

		if ( _securityHandler.get() == nullptr or
		     _securityHandler->isUnlocked() )
		{
			loadRoot();
		}
	}
	catch ( ... )
	{
		_parser.reset();
		_stream.close();
		throw;
	}
}

void Document::loadRoot()
{
	assert( _catalog.is_null() );

	auto &Root = _trailerDict["Root"];
	if ( not Root.is_dictionary() )
		throw std::runtime_error( "invalid PDF file" );

	_catalog = Root;

	auto &Version = _catalog["Version"];
	auto catalogVersion = Version.name_value();
	if ( not catalogVersion.empty() and
		     atof( catalogVersion.c_str() ) >
		         atof( _version.c_str() ) )
	{
		_version = catalogVersion;
	}

	auto &pages = _catalog["Pages"];
	if ( not pages.is_dictionary() )
		throw std::runtime_error( "invalid PDF file" );

	int pageCount = -1;
	auto &obj = pages["Count"];
	if ( obj.is_number() )
		pageCount = obj.int_value();
	else
		pageCount = iterateCountPages( pages );

	_pageRepository.resize( pageCount );
}

void Document::preload()
{
	std::vector<int> objectIndexes;
	objectIndexes.reserve( _xrefTable.size() );

	// the uncompressed objects
	// 1- list them
	int index = 0;
	for ( auto &it : _xrefTable )
	{
		if ( it.object().is_null() and not it.compressed() and it.pos() > 0 )
			objectIndexes.push_back( index );
		++index;
	}
	
	// 2- sort by offset in file
	std::sort( objectIndexes.begin(), objectIndexes.end(),
				[this]( int lhs, int rhs )
				{
					return _xrefTable[lhs].pos() < _xrefTable[rhs].pos();
				} );
	
	// 3- load all
	for ( auto it : objectIndexes )
		_xrefTable.registerObject( it, _parser->readObject( it ) );
	objectIndexes.clear();

	// the compressed objects
	// 1- list them
	index = 0;
	for ( auto &it : _xrefTable )
	{
		if ( it.object().is_null() and it.compressed() and it.streamId() > 0 )
			objectIndexes.push_back( index );
		++index;
	}

	// 2- sort by stream ID
	std::sort( objectIndexes.begin(), objectIndexes.end(),
				[this]( int lhs, int rhs )
				{
					return _xrefTable[lhs].streamId() < _xrefTable[rhs].streamId();
				} );

	// 3- load all
	for ( auto it : objectIndexes )
		_xrefTable.registerObject( it, _parser->readObject( it ) );
}

Version Document::version() const
{
	Version v{};
	std::regex parseVersion( "(\\d+)\\.(\\d+)" );
	std::smatch matches;
	if ( std::regex_search( _version, matches, parseVersion ) )
	{
		v.major = std::stoi( matches[1].str() );
		v.minor = std::stoi( matches[2].str() );
	}
	return v;
}

bool Document::isEncrypted() const
{
	return _securityHandler.get() != nullptr;
}

bool Document::unlock( const std::string &i_password )
{
	if ( not isUnlocked() )
	{
		if ( _securityHandler->unlock( this, i_password ) )
			loadRoot();
		else
			return false;
	}
	return true;
}

bool Document::isUnlocked() const
{
	if ( _securityHandler.get() != nullptr )
		return _securityHandler->isUnlocked();
	else
		return true;
}

size_t Document::nbOfPages() const
{
	return _pageRepository.size();
}

Page Document::page( size_t i_index ) const
{
	if ( i_index > _pageRepository.size() )
		return {};
	if ( _pageRepository[i_index].is_null() )
	{
		auto pages = _catalog["Pages"];
		auto page = navigateToPage( pages, i_index );
		_pageRepository[i_index] = page;
	}
	return { _pageRepository[i_index] };
}

const Object &Document::catalog() const
{
	return _catalog;
}

const Object &Document::info() const
{
	auto &obj = _trailerDict["Info"];
	if ( obj.is_dictionary() )
		return obj;
	return kObjectNull;
}

std::string Document::decrypt( const std::string &i_input,
                                   int i_id,
                                   int i_gen ) const
{
	if ( i_id != 0 and _securityHandler.get() != nullptr )
	{
		std::string o;
		_securityHandler->decryptString( i_input, o, i_id, i_gen );
		return o;
	}
	return i_input;
}

std::pair<std::string,std::string> Document::getFileID() const
{
	std::pair<std::string,std::string> ids;
	auto &fileID = _trailerDict["ID"].array_items();
	if ( fileID.size() > 0 )
		ids.first = fileID[0].string_value();
	if ( fileID.size() > 1 )
		ids.second = fileID[1].string_value();
	return ids;
}

Object Document::navigateToPage( const Object &pages,
                                                   size_t i_page ) const
{
	if ( not pages.is_dictionary() )
		return kObjectNull;
	auto kids = pages["Kids"];
	if ( not kids.is_array() )
		return kObjectNull;

	for ( auto &c : kids.array_items_resolved() )
	{
		if ( not c.is_dictionary() )
			return kObjectNull;
		auto &type = c["Type"];
		if ( not type.is_name() )
			return kObjectNull;
		if ( type.name_value() == "Page" )
		{
			if ( i_page == 0 )
				return c;
			else
				--i_page;
		}
		else
		{
			int pageCount = -1;
			auto count = c["Count"];
			if ( not count.is_null() )
			{
				if ( count.is_number() )
					pageCount = count.int_value();
			}
			if ( pageCount == -1 )
				pageCount = iterateCountPages( c );
			if ( pageCount <= i_page )
				i_page -= pageCount;
			else
				return navigateToPage( c, i_page );
		}
	}
	return kObjectNull;
}

const Object &Document::resolveIndirect( const Object &i_obj ) const
{
	if ( not i_obj.is_ref() )
		return i_obj;
	
	auto objRef = i_obj.ref_value();
	const auto &loaded = _xrefTable.resolveRef( objRef.ref, objRef.gen );
	if ( loaded.is_null() and _parser.get() != nullptr )
	{
		auto obj = _parser->readObject( objRef.ref );
		return _xrefTable.registerObject( objRef.ref, obj );
	}
	return loaded;
}

std::streamoff Document::read( size_t i_pos,
                                   su::array_view<uint8_t> o_buffer ) const
{
	return _parser->read( i_pos, o_buffer );
}

std::unique_ptr<Crypter> Document::createCrypter( bool i_isMetadata,
                                                         int i_id,
                                                         int i_gen ) const
{
	if ( _securityHandler.get() != nullptr )
		return _securityHandler->createDefaultStreamCrypter(
		    i_isMetadata, i_id, i_gen );
	else
		return nullptr;
}

Object::Type Document::get_type( const std::string &i_path ) const
{
	pdfp::Object object;
	auto res = resolvePath( this, object, nullptr, nullptr, nullptr, i_path );

	if ( res == pdfp_object )
		return object.type();
	else
		return Object::Type::k_invalid;
}

bool Document::get_boolean( const std::string &i_path ) const
{
	pdfp::Object object;
	int pdfint;
	float pdffloat;
	auto res = resolvePath( this, object, nullptr, &pdfint, &pdffloat, i_path );
	if ( res == pdfp_object )
	{
		if ( object.is_boolean() )
			return object.bool_value();
		else if ( object.is_number() )
			return object.int_value() != 0;
	}
	else if ( res == pdfp_int )
		return pdfint != 0;
	else if ( res == pdfp_float )
		return pdffloat != 0;
	return false;
}

int Document::get_int( const std::string &i_path ) const
{
	pdfp::Object object;
	int pdfint;
	float pdffloat;
	auto res = resolvePath( this, object, nullptr, &pdfint, &pdffloat, i_path );
	if ( res == pdfp_object )
	{
		if ( object.is_boolean() )
			return object.bool_value() ? 1 : 0;
		else if ( object.is_number() )
			return object.int_value();
	}
	else if ( res == pdfp_int )
		return pdfint;
	else if ( res == pdfp_float )
		return static_cast<int>( pdffloat );
	return 0;
}

float Document::get_real( const std::string &i_path ) const
{
	pdfp::Object object;
	int pdfint;
	float pdffloat;
	auto res = resolvePath( this, object, nullptr, &pdfint, &pdffloat, i_path );
	if ( res == pdfp_object )
	{
		if ( object.is_boolean() )
			return object.bool_value() ? 1 : 0;
		else if ( object.is_number() )
			return object.real_value();
	}
	else if ( res == pdfp_int )
		return static_cast<float>( pdfint );
	else if ( res == pdfp_float )
		return pdffloat;
	return 0;
}

std::string Document::get_string( const std::string &i_path ) const
{
	pdfp::Object object;
	std::string s;
	auto res = resolvePath( this, object, &s, nullptr, nullptr, i_path );
	if ( res == pdfp_object and object.is_string() )
		return object.string_value();
	if ( res == pdfp_object and object.is_name() )
		return object.name_value();
	else if ( res == pdfp_string )
		return s;
	else
		return std::string();
}

std::string Document::get_ustring( const std::string &i_path ) const
{
	pdfp::Object object;
	std::string s;
	auto res = resolvePath( this, object, &s, nullptr, nullptr, i_path );
	if ( res == pdfp_object and object.is_string() )
		return object.string_uvalue();
	if ( res == pdfp_object and object.is_name() )
		return object.name_value();
	else if ( res == pdfp_string )
		return std::string( s.c_str() ); // ok, it's utf8
	else
		return {};
}

DataStreamRef Document::get_streamData( const std::string &i_path ) const
{
	pdfp::Object object;
	auto res = resolvePath( this, object, nullptr, nullptr, nullptr, i_path );
	if ( res == pdfp_object and object.is_stream() )
		return object.stream_data();
	else
		return {};
}

Object Document::get_dictionary( const std::string &i_path ) const
{
	pdfp::Object object;
	auto res = resolvePath( this, object, nullptr, nullptr, nullptr, i_path );
	if ( res == pdfp_object )
		return object;
	else
		return kObjectNull;
}

size_t Document::mem_size() const
{
	size_t s = sizeof( Document );
	s += _trailerDict.mem_size();
	s += _catalog.mem_size();
	return s;
}

}
