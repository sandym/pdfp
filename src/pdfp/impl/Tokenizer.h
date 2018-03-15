/*
 *  Tokenizer.h
 *  pdfp
 *
 *  Created by Sandy Martel on 06-03-26.
 *  Copyright 2003 Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_TOKENIZER
#define H_PDFP_TOKENIZER

#include <iostream>
#include <stack>

namespace pdfp {

/*!
   @brief a lexical token read in a PDF file.

       Lexical token, used during the lexical analysis of a PDF file.
*/
class Token
{
public:
	enum token_type
	{
		tok_invalid,
		tok_bool, // bool
		tok_int, // int
		tok_float, // float
		tok_string, // pdf string
		tok_name, // pdf name
		tok_openarray, // [
		tok_closearray, // ]
		tok_opendict, // <<
		tok_closedict, // >>
		tok_stream, // "stream"
		tok_endstream, // "endstream"
		tok_obj, // "obj"
		tok_endobj, // "endobj"
		tok_null, // "null"
		tok_version, // string
		tok_eof, // string
		tok_startxref, // "startxref"
		tok_xref, // "xref"
		tok_trailer, // "trailer"
		tok_command // string
	};

	Token();
	Token( token_type tok, const std::string &val );
	Token( const std::string &val, float val2 );
	Token( const std::string &val, int val2 );
	Token( const std::string &val, bool val2 );
	~Token() = default;

	token_type type() const { return _tokenType; }
	const std::string &value() const { return _value; }
	float floatValue() const;
	int intValue() const;
	bool boolValue() const;

private:
	token_type _tokenType; //!< tells which member of the union is valid
	std::string _value;
	union
	{
		float _floatValue;
		int _intValue;
		bool _boolValue;
	} u;
};

/*!
   @brief The lexical analyser.

       Read a PDF file and return a sequence of lexical token.
*/
class Tokenizer
{
public:
	Tokenizer( std::istream &str );
	~Tokenizer() = default;

	//! get the next token of the stream, return false if no token found
	bool nextToken( Token &o_token );

	//! get the next token of the stream, throw a parse error if no token found
	void nextTokenForced( Token &o_token );

	//! get the next token of the stream, throw a parse error if no token found
	//! or the token is not of the specified type
	void nextTokenForced( Token &o_token, Token::token_type forcedToken );

	//! get the next token of the stream, if the token is not of the specified
	//! type return an empty Token and leave the stream is unchanged
	bool nextTokenOptional( Token &o_token, Token::token_type optionalToken );

	//! Put back a token in the stream
	void putBack( const Token &i_token );

	//! throw a parse error
	void invalidToken( const Token &i_token ) const;

	bool getLine( std::string &o_line );
	std::istream::off_type tellg();
	void seekg( std::istream::off_type p, std::ios_base::seekdir s );
	std::streamsize read( std::istream::char_type *p, std::streamsize len );
	operator bool() const { return (bool)_stream; }

private:
	std::istream &_stream;

	std::stack<Token> _putbackList;
	std::stack<char> _putbackCharList;

	bool nextNonSpaceChar( char &o_char );
	bool nextChar( char &o_char );
	void putBackChar( char val );
	bool readToWhiteSpace( std::string &o_str );
	bool readToDelimiter( std::string &o_str );
	bool readName( std::string &o_str );
	bool readNumber( char aChar,
	                 std::string &o_str,
	                 bool &o_isInt,
	                 int &o_intVal,
	                 float &o_floatVal );
	bool readHexString( std::string &o_str );
	bool readLiteralString( std::string &o_str );
	void skipToNextLine();
};
}

#endif
