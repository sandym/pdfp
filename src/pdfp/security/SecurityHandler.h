//
//  SecurityHandler.h
//  pdfp
//
//  Created by Sandy Martel on 11/10/03.
//  Copyright 2011年 Sandy Martel. All rights reserved.
//

#ifndef H_PDFP_SecurityHandler
#define H_PDFP_SecurityHandler

#include "pdfp/filters/Filter.h"

namespace pdfp {

class Document;
class Object;
class Crypter;

class SecurityHandler
{
protected:
	SecurityHandler() = default;

public:
	SecurityHandler( const SecurityHandler & ) = delete;
	SecurityHandler &operator=( const SecurityHandler & ) = delete;

	static std::unique_ptr<SecurityHandler> create(
	    const std::string &i_filter, const Object &i_encryptDict );

	virtual ~SecurityHandler() = default;

	virtual bool unlock( Document *i_doc,
	                     const std::string &i_password ) = 0;
	virtual bool isUnlocked() const = 0;

	virtual std::string decryptString( const std::string &i_input,
	                            int i_id,
	                            int i_gen ) = 0;

	virtual std::unique_ptr<Crypter> createDefaultStreamCrypter(
	    bool i_isMetadata, int i_id, int i_gen ) const = 0;
};

/*!
Interface for an object that can decrypt
*/
class Crypter : public InputFilter
{
protected:
	Crypter() = default;

public:
	Crypter( const Crypter & ) = delete;
	Crypter &operator=( const Crypter & ) = delete;
	virtual ~Crypter() = default;

	virtual void rewind() = 0;
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer ) = 0;
};

}

#endif
