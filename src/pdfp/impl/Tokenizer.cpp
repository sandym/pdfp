/*
 *  Tokenizer.h
 *  pdfp
 *
 *  Created by Sandy Martel on 06-03-26.
 *  Copyright 2003 Sandy Martel. All rights reserved.
 *
 */

#include "Tokenizer.h"
#include "Utils.h"
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace pdfp {

Token::Token() : _tokenType( tok_invalid ) {}

Token::Token( token_type tok, const std::string &val ) :
    _tokenType( tok ),
    _value( val )
{
	assert( _tokenType != tok_invalid and _tokenType != tok_bool and
	        _tokenType != tok_int and _tokenType != tok_float );
}

Token::Token( const std::string &val, float val2 ) :
    _tokenType( tok_float ),
    _value( val )
{
	u._floatValue = val2;
}

Token::Token( const std::string &val, int val2 ) :
    _tokenType( tok_int ),
    _value( val )
{
	u._intValue = val2;
}

Token::Token( const std::string &val, bool val2 ) :
    _tokenType( tok_bool ),
    _value( val )
{
	u._boolValue = val2;
}

float Token::floatValue() const
{
	if ( _tokenType == tok_float )
		return u._floatValue;
	else if ( _tokenType == tok_int )
		return float( u._intValue );
	else if ( _tokenType == tok_bool )
		return u._boolValue ? 1 : 0;
	else
		return 0;
}

int Token::intValue() const
{
	if ( _tokenType == tok_int )
		return u._intValue;
	else if ( _tokenType == tok_float )
		return int( u._floatValue );
	else if ( _tokenType == tok_bool )
		return u._boolValue ? 1 : 0;
	else
		return 0;
}

bool Token::boolValue() const
{
	if ( _tokenType == tok_bool )
		return u._boolValue;
	else if ( _tokenType == tok_int )
		return u._intValue != 0;
	else if ( _tokenType == tok_float )
		return u._floatValue != 0;
	else
		return false;
}

// MARK: -

Tokenizer::Tokenizer( std::istream &str ) : _stream( str ) {}

bool Tokenizer::nextToken( Token &o_token )
{
	// look first in the put back stack
	if ( not _putbackList.empty() )
	{
		o_token = _putbackList.top();
		_putbackList.pop();
		return true;
	}
	else
	{
		o_token = Token();
		bool res;
		do
		{
			// parse
			char aChar;
			res = nextNonSpaceChar( aChar );

			if ( aChar == '%' )
			{
				std::string s;
				readToWhiteSpace( s );
				s.insert( 0, 1, aChar );
				if ( s == "%%EOF" )
				{
					// %%EOF
					o_token = Token( Token::tok_eof, "%%EOF" );
					skipToNextLine();
				}
				else if ( s.substr( 0, 5 ) == "%PDF-" )
				{
					o_token = Token( Token::tok_version, s.substr( 5 ) );
					skipToNextLine();
				}
				else
				{
					// comment
					skipToNextLine();
				}
			}
			else if ( aChar == '+' or aChar == '-' or aChar == '.' or
			          isdigit( aChar ) )
			{
				std::string n;
				bool isInt;
				int intVal;
				float floatVal;
				if ( readNumber( aChar, n, isInt, intVal, floatVal ) )
				{
					if ( isInt )
						o_token = Token( n, intVal );
					else
						o_token = Token( n, floatVal );
				}
				else
					throw std::runtime_error( "invalid character" );
			}
			else if ( aChar == '(' )
			{
				std::string s;
				if ( readLiteralString( s ) )
				{
					o_token = Token( Token::tok_string, s );
				}
				else
					throw std::runtime_error( "invalid character" );
			}
			else if ( aChar == '<' )
			{
				res = nextChar( aChar );
				if ( aChar == '<' )
				{
					o_token = Token( Token::tok_opendict, "<<" );
				}
				else if ( res )
				{
					putBackChar( aChar );
					std::string s;
					if ( readHexString( s ) )
					{
						o_token = Token( Token::tok_string, s );
					}
					else
						throw std::runtime_error( "invalid character" );
				}
			}
			else if ( aChar == '/' )
			{
				std::string n;
				readName( n );
				o_token = Token( Token::tok_name, n );
			}
			else if ( aChar == '[' )
			{
				o_token = Token( Token::tok_openarray, "[" );
			}
			else if ( aChar == ']' )
			{
				o_token = Token( Token::tok_closearray, "]" );
			}
			else if ( aChar == '>' )
			{
				res = nextChar( aChar );
				if ( aChar == '>' )
					o_token = Token( Token::tok_closedict, ">>" );
				else
					throw std::runtime_error( "invalid character" );
			}
			else if ( isalpha( aChar ) )
			{
				std::string s;
				readToDelimiter( s );
				s.insert( 0, 1, aChar );

				if ( s == "true" )
					o_token = Token( "true", true );
				else if ( s == "false" )
					o_token = Token( "false", false );
				else if ( s == "stream" )
				{
					char c;
					/*res =*/nextChar( aChar );
					// skip spaces
					while ( aChar == ' ' )
						nextChar( aChar );
					
					res = nextChar( c );
					if ( aChar == 0x0A )
					{
						putBackChar( c );
						o_token = Token( Token::tok_stream, "stream" );
					}
					else if ( aChar == 0x0D )
					{
						if ( c == 0x0A )
							o_token = Token( Token::tok_stream, "stream" );
						else
						{
							putBackChar( c );
							o_token = Token( Token::tok_stream, "stream" );
						}
					}
					else
						throw std::runtime_error( "invalid character" );
				}
				else if ( s == "endstream" )
					o_token = Token( Token::tok_endstream, s );
				else if ( s == "obj" )
					o_token = Token( Token::tok_obj, s );
				else if ( s == "endobj" )
					o_token = Token( Token::tok_endobj, s );
				else if ( s == "null" )
					o_token = Token( Token::tok_null, s );
				else if ( s == "startxref" )
					o_token = Token( Token::tok_startxref, s );
				else if ( s == "xref" )
					o_token = Token( Token::tok_xref, s );
				else if ( s == "trailer" )
					o_token = Token( Token::tok_trailer, s );
				else
					o_token = Token( Token::tok_command, s );
			}
			else if ( res )
				throw std::runtime_error( "invalid character" );
		} while ( o_token.type() == Token::tok_invalid and res );
		return res;
	}
}

void Tokenizer::nextTokenForced( Token &o_token )
{
	bool res = nextToken( o_token );
	if ( not res )
		throw std::runtime_error( "error parsing file" );
}

void Tokenizer::nextTokenForced( Token &o_token,
                                    Token::token_type forcedToken )
{
	nextTokenForced( o_token );
	if ( o_token.type() != forcedToken )
		throw std::runtime_error( std::string( "invalid token : " ) +
		                          o_token.value() );
}

bool Tokenizer::nextTokenOptional( Token &o_token,
                                      Token::token_type optionalToken )
{
	if ( nextToken( o_token ) )
	{
		if ( o_token.type() != optionalToken )
		{
			putBack( o_token );
			o_token = Token();
		}
		else
			return true;
	}
	return false;
}

void Tokenizer::putBack( const Token &i_token )
{
	_putbackList.push( i_token );
}

void Tokenizer::invalidToken( const Token &i_token ) const
{
	throw std::runtime_error( std::string( "invalid token : " ) +
	                          i_token.value() );
}

void Tokenizer::putBackChar( char val )
{
	_putbackCharList.push( val );
}

bool Tokenizer::nextNonSpaceChar( char &o_char )
{
	bool res;
	do
	{
		res = nextChar( o_char );
	} while ( res and isWhiteSpace( o_char ) );
	return res;
}

bool Tokenizer::nextChar( char &o_char )
{
	if ( not _putbackCharList.empty() )
	{
		o_char = _putbackCharList.top();
		_putbackCharList.pop();
		return true;
	}

	o_char = 0;
	if ( _stream )
	{
		int val = _stream.get();
		if ( val == EOF )
			return false;
		o_char = val;

		return true;
	}
	else
	{
		if ( _stream.eof() )
			return false;
		else
			throw std::runtime_error( "unknown error" );
	}
	return false;
}

void Tokenizer::skipToNextLine()
{
	bool res;
	char aChar;
	do
	{
		res = nextChar( aChar );
	} while ( res and aChar != '\r' and aChar != '\n' );
}

bool Tokenizer::readName( std::string &o_str )
{
	for ( ;; )
	{
		char aChar;
		bool res = nextChar( aChar );
		if ( res and not isWhiteSpace( aChar ) and not isDelimiter( aChar ) )
		{
			if ( aChar == '#' )
			{
				char c1, c2;
				res = nextChar( c1 );
				if ( res and isxdigit( c1 ) )
				{
					res = nextChar( c2 );
					if ( res and isxdigit( c2 ) )
					{
						int v1 = c1 > '9' ? ( tolower( c1 ) - 'a' + 10 ) :
						                    ( c1 - '0' );
						int v2 = c2 > '9' ? ( tolower( c2 ) - 'a' + 10 ) :
						                    ( c2 - '0' );
						aChar = v1 * 16 + v2;
					}
					else
					{
						putBackChar( c2 );
						putBackChar( c1 );
					}
				}
				else
					putBackChar( c1 );
			}
			o_str.push_back( aChar );
		}
		else
		{
			putBackChar( aChar );
			break;
		}
	}
	return true;
}

bool Tokenizer::readToWhiteSpace( std::string &o_str )
{
	for ( ;; )
	{
		char aChar;
		bool res = nextChar( aChar );
		if ( res and not isWhiteSpace( aChar ) )
			o_str.push_back( aChar );
		else
		{
			putBackChar( aChar );
			break;
		}
	}

	return true;
}

bool Tokenizer::readToDelimiter( std::string &o_str )
{
	bool res;
	char aChar;

	for ( ;; )
	{
		res = nextChar( aChar );
		if ( res and not isWhiteSpace( aChar ) and not isDelimiter( aChar ) )
			o_str.push_back( aChar );
		else
		{
			putBackChar( aChar );
			break;
		}
	}

	return true;
}

bool Tokenizer::readNumber( char aChar,
                               std::string &o_str,
                               bool &o_isInt,
                               int &o_intVal,
                               float &o_floatVal )
{
	bool res = true;
	if ( aChar == '-' )
	{
		o_str.push_back( aChar );
		res = nextChar( aChar );
	}
	if ( res and ( aChar == '.' or isdigit( aChar ) ) )
	{
		bool isFloat = false;
		bool digitFound = false;
		while ( res and isdigit( aChar ) )
		{
			o_str.push_back( aChar );
			digitFound = true;
			res = nextChar( aChar );
		}
		if ( aChar == '.' )
		{
			o_str.push_back( aChar );
			isFloat = true;
			res = nextChar( aChar );
			while ( res and isdigit( aChar ) )
			{
				o_str.push_back( aChar );
				digitFound = true;
				res = nextChar( aChar );
			}
		}
		putBackChar( aChar );

		if ( digitFound )
		{
			if ( isFloat )
			{
				o_isInt = false;
				o_floatVal = atof( o_str.c_str() );
			}
			else
			{
				o_isInt = true;
				o_intVal = atoi( o_str.c_str() );
			}
			return true;
		}
	}
	return false;
}

bool Tokenizer::readLiteralString( std::string &o_str )
{
	int paren = 1;
	bool res = true;
	while ( res and paren != 0 )
	{
		char aChar;
		res = nextChar( aChar );
		if ( aChar == '(' )
		{
			++paren;
			o_str.push_back( aChar );
		}
		else if ( aChar == ')' )
		{
			--paren;
			if ( paren != 0 )
				o_str.push_back( aChar );
		}
		else if ( aChar == '\\' )
		{
			res = nextChar( aChar );
			if ( aChar == 'n' )
			{
				o_str.push_back( '\n' );
			}
			else if ( aChar == 'r' )
			{
				o_str.push_back( '\r' );
			}
			else if ( aChar == 't' )
			{
				o_str.push_back( '\t' );
			}
			else if ( aChar == 'b' )
			{
				o_str.push_back( '\b' );
			}
			else if ( aChar == 'f' )
			{
				o_str.push_back( '\f' );
			}
			else if ( aChar == '(' or aChar == ')' or aChar == '\\' )
			{
				o_str.push_back( aChar );
			}
			else if ( isdigit( aChar ) )
			{
				char c, c1 = '0', c2 = '0', c3 = aChar;
				res = nextChar( c );
				if ( isdigit( c ) )
				{
					c2 = c3;
					c3 = c;
					res = nextChar( c );
					if ( isdigit( c ) )
					{
						c1 = c2;
						c2 = c3;
						c3 = c;
					}
					else
						putBackChar( c );
				}
				else
					putBackChar( c );
				c = c1 - '0';
				c = ( c * 8 ) + ( c2 - '0' );
				c = ( c * 8 ) + ( c3 - '0' );
				o_str.push_back( c );
			}
			else if ( aChar == '\r' )
			{
				// skip
				nextChar( aChar );
				if ( aChar != '\n' )
					putBackChar( aChar );
			}
			else if ( aChar == '\n' )
			{
				// skip
			}
			else
			{
				o_str.push_back( aChar );
			}
		}
		else if ( aChar == '\r' )
		{
			// end-of-line shall be treated as a 0x0a byte.
			o_str.push_back( '\n' );
			nextChar( aChar );
			if ( aChar != '\n' )
				putBackChar( aChar );
		}
		else
			o_str.push_back( aChar );
	}
	return res;
}

bool Tokenizer::readHexString( std::string &o_str )
{
	bool res;
	do
	{
		char c1, c2;
		res = nextNonSpaceChar( c1 );
		if ( c1 == '>' )
			break;
		res = nextNonSpaceChar( c2 );
		if ( isxdigit( c1 ) )
		{
			int v1 = c1 > '9' ? ( tolower( c1 ) - 'a' + 10 ) : ( c1 - '0' );
			int v2 = 0;
			if ( isxdigit( c2 ) )
			{
				v2 = c2 > '9' ? ( tolower( c2 ) - 'a' + 10 ) : ( c2 - '0' );
			}
			char c = ( v1 << 4 ) | v2;
			o_str.push_back( c );
			if ( not isxdigit( c2 ) ) // c2 == '>'
				break;
		}
		else
			res = false;
	} while ( res );

	return res;
}

bool Tokenizer::getLine( std::string &o_line )
{
	o_line.clear();
	while ( not _putbackList.empty() )
		_putbackList.pop();

	char c;
	while ( nextChar( c ) )
	{
		if ( c == '\n' or c == '\r' )
			return true;
		else
			o_line.push_back( c );
	}
	return not o_line.empty();
}

std::istream::off_type Tokenizer::tellg()
{
	return std::istream::off_type( _stream.tellg() ) - _putbackCharList.size();
}

void Tokenizer::seekg( std::istream::off_type p, std::ios_base::seekdir s )
{
	while ( not _putbackList.empty() )
		_putbackList.pop();
	while ( not _putbackCharList.empty() )
		_putbackCharList.pop();
	_stream.clear();
	_stream.seekg( p, s );
}

std::streamsize Tokenizer::read( std::istream::char_type *p,
                                    std::streamsize len )
{
	std::streamsize s = 0;
	while ( not _putbackList.empty() )
		_putbackList.pop();
	while ( not _putbackCharList.empty() and s != len )
	{
		*p = _putbackCharList.top();
		_putbackCharList.pop();
		++p;
		++s;
	}
	_stream.read( p, len - s );
	return s + _stream.gcount();
}
}
