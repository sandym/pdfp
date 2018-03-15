/*
 *  Parser.h
 *  pdfp
 *
 *  Created by Sandy Martel on Sat Apr 12 2003.
 *  Copyright (c) 2003 Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_PARSER
#define H_PDFP_PARSER

#include "Tokenizer.h"
#include "pdfp/PDFObject.h"
#include "su/containers/array_view.h"
#include "su/containers/flat_map.h"

namespace pdfp {

class Object;
class Document;

/*!
   @brief syntaxic analyser.

       Do the syntaxic analisys of a PDF file, reading and parsing object
   definitions.
*/
class Parser
{
public:
	Parser( std::istream &i_str, Document *i_doc );
	~Parser() = default;

	Object readXRef( std::string &o_headerVersion );
	Object buildXRef( std::string &o_headerVersion );

	Object readObject( int i_id );

	void cleanup();

	std::streamoff read( size_t i_pos, su::array_view<uint8_t> o_buffer );
	bool isEndOfStream( size_t i_pos );

private:
	Document *_doc;
	Tokenizer _tokenizer;

	Object readObject_priv( int i_id, int i_gen );
	size_t resolveLength( const Object &i_obj );

	bool verifyXRef();

	void readCrossReferenceTable( Object::dictionary &o_trailerDict,
	                              std::streamoff &o_xrefpos );
	void readCrossReferenceStream( Object::dictionary &o_trailerDict,
	                               std::streamoff &o_xrefpos );

	Object readCompressedObject( int i_streamId,
	                                                 int i_indexInStream );

	bool guessStreamLength( size_t i_streamStart, size_t &o_length );

	// compressed objects
	struct ObjectStreamIndex
	{
		int objNb;
		size_t offset;
	};
	struct ObjectStreamData
	{
		std::unique_ptr<char[]> data;
		size_t size = 0;
	};
	ObjectStreamData readCompressedObjectStream(
	    int i_streamId,
	    std::vector<ObjectStreamIndex> &o_compressedObjectStreamIndex );

	su::flat_map<int, std::vector<Object>> _compressedObjects;
};
}

#endif
