//
//  SecurityHandler.cpp
//  pdfp
//
//  Created by Sandy Martel on 11/10/03.
//  Copyright 2011年 Sandy Martel. All rights reserved.
//

#include "SecurityHandler.h"
#include "StandardSecurityHandler.h"

class UnsupportedSecurityHandler : public pdfp::SecurityHandler
{
public:
	UnsupportedSecurityHandler() = default;
	virtual ~UnsupportedSecurityHandler() = default;

	virtual std::unique_ptr<pdfp::Crypter> createDefaultStreamCrypter(
	    bool, int, int ) const;
	virtual void decryptString( const std::string &, std::string &, int, int );

	virtual bool unlock( pdfp::Document *i_doc,
	                     const std::string &i_password );
	virtual bool isUnlocked() const;
};

std::unique_ptr<pdfp::Crypter>
UnsupportedSecurityHandler::createDefaultStreamCrypter( bool, int, int ) const
{
	return {};
}

void UnsupportedSecurityHandler::decryptString( const std::string &,
                                                std::string &,
                                                int,
                                                int )
{
}

bool UnsupportedSecurityHandler::unlock( pdfp::Document *,
                                         const std::string & )
{
	return false;
}

bool UnsupportedSecurityHandler::isUnlocked() const
{
	return false;
}

// MARK: -

std::unique_ptr<pdfp::SecurityHandler> pdfp::SecurityHandler::create(
    const std::string &i_filter, const Object &i_encryptDict )
{
	if ( i_filter == "Standard" )
		return std::make_unique<StandardSecurityHandler>( i_encryptDict );

	return {};
}

// MARK: -

void pdfp::IdentityCrypter::reset() {}

void pdfp::IdentityCrypter::decrypt( su::array_view<uint8_t> ) {}
