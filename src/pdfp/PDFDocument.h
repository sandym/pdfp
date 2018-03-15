/*
 *  PDFDocument.h
 *  pdfp
 *
 *  Created by Sandy Martel on Sat Apr 19 2003.
 *  Copyright (c) 2003 Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_PDFDOCUMENT
#define H_PDFP_PDFDOCUMENT

#include "PDFObject.h"
#include "PDFPage.h"
#include "su/containers/flat_map.h"
#include "impl/XrefTable.h"
#include <fstream>

namespace pdfp {

struct Version
{
	int major{0}, minor{0};
};

class DocSource;
class Crypter;
class SecurityHandler;
class Parser;

/*!
   PDF document class
*/
class Document final
{
public:
	Document( const Document & ) = delete;
	Document &operator=( const Document & ) = delete;

	Document();
	~Document();

	void open( const std::string &i_path );
	void preload();

	// interface
	Version version() const;
	bool isEncrypted() const;
	bool unlock( const std::string &i_password );
	bool isUnlocked() const;
	size_t nbOfPages() const;
	Page page( size_t i_index ) const;
	const Object &catalog() const;
	const Object &info() const;

	std::pair<std::string,std::string> getFileID() const;

	// path access:
	// prefix:
	//    pages[d]
	//    info
	//    catalog or /
	//    version
	// suffix:
	//    >count   for array and dictionary
	//    >keys[index] for dictionary
	//    [index]      for array and dictionary
	//    >dict    for stream
	Object::Type get_type( const std::string &i_path ) const;
	bool get_boolean( const std::string &i_path ) const;
	int get_int( const std::string &i_path ) const;
	float get_real( const std::string &i_path ) const;
	std::string get_string( const std::string &i_path ) const;
	std::string get_ustring( const std::string &i_path ) const;
	DataStreamRef get_streamData( const std::string &i_path ) const;
	Object get_dictionary( const std::string &i_path ) const;

	// edition, todo

	const Object &resolveIndirect( const Object &i_obj ) const;

	size_t mem_size() const;
	
private:
	std::ifstream _stream;
	//! the xref table, all indirect objects are stored here
	XrefTable _xrefTable;
	std::unique_ptr<Parser> _parser;

	Object _trailerDict;
	Object _catalog;

	std::unique_ptr<SecurityHandler> _securityHandler;

	std::string _version;

	mutable std::vector<Object> _pageRepository;

	void loadRoot();

	std::streamoff read( size_t i_pos, su::array_view<uint8_t> o_buffer ) const;
	
	std::string decrypt( const std::string &i_input,
	                     int i_id,
	                     int i_gen ) const;
	std::unique_ptr<Crypter> createCrypter( bool i_isMetadata,
	                                           int i_id,
	                                           int i_gen ) const;

	XrefTable &xrefTable() { return _xrefTable; }

	Object navigateToPage( const Object &pages,
	                                     size_t i_page ) const;

	friend class DocSource;
	friend class Parser;
	friend DataStreamRef createDataStream( const Object &,
                                size_t,
                                size_t,
                                int,
                                int );

};
}

#endif
