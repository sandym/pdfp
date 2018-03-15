/*
 *  Utils.h
 *  pdfp
 *
 *  Created by Sandy Martel on 06-03-26.
 *  Copyright 2006 Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_UTILS
#define H_PDFP_UTILS

namespace pdfp {

/*!
   @brief define a PDF white space.

        Return true if the input is a PDF white space as defined in the
   PDFReference.
   @param c a character, should be only 8 bits
   @return     true if the input is a PDF white space
*/
inline bool isWhiteSpace( int c )
{
	return ( c == 0 or c == 0x09 or c == 0x0A or c == 0x0C or c == 0x0D or
	         c == 0x20 );
}

/*!
   @brief define a PDF delimiter.

       Return true if the input is a PDF delimiter as defined in the
   PDFReference.
   @param c a character, should be only 8 bits
   @return     true if the input is a PDF delimiter
*/
inline bool isDelimiter( int c )
{
	return ( c == '(' or c == ')' or c == '<' or c == '>' or c == '[' or
	         c == ']' or c == '{' or c == '}' or c == '/' or c == '%' );
}
}

#endif
