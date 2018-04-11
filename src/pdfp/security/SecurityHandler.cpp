//
//  SecurityHandler.cpp
//  pdfp
//
//  Created by Sandy Martel on 11/10/03.
//  Copyright 2011年 Sandy Martel. All rights reserved.
//

#include "SecurityHandler.h"
#include "StandardSecurityHandler.h"

std::unique_ptr<pdfp::SecurityHandler> pdfp::SecurityHandler::create(
    const std::string &i_filter, const Object &i_encryptDict )
{
	if ( i_filter == "Standard" )
		return std::make_unique<StandardSecurityHandler>( i_encryptDict );

	return {};
}
