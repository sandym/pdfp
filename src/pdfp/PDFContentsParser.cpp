/*
 *  PDFContentsParser.h
 *  pdfp
 *
 *  Created by Sandy Martel on 2018-03-04.
 *  Copyright 2006 Sandy Martel. All rights reserved.
 *
 */

#include "PDFContentsParser.h"
#include <cassert>

namespace pdfp {

ContentsParser::ContentsParser( const Object &i_graphic )
	: _graphic( i_graphic )
{
	_stack.reserve( 8 );
}

void ContentsParser::parse()
{
	assert( false );
}

Object ContentsParser::pop()
{
	if ( not _stack.empty() )
	{
		auto o = std::move(_stack.back());
		_stack.pop_back();
	}
	return kObjectNull;
}

Object ContentsParser::get_xobject( const std::string &i_name )
{
	assert( false );
	return {};
}
Object ContentsParser::get_font( const std::string &i_name )
{
	assert( false );
	return {};
}
Object ContentsParser::get_colourspace( const std::string &i_name )
{
	assert( false );
	return {};
}
Object ContentsParser::get_pattern( const std::string &i_name )
{
	assert( false );
	return {};
}
Object ContentsParser::get_shading( const std::string &i_name )
{
	assert( false );
	return {};
}

void ContentsParser::b(){}
void ContentsParser::B(){}
void ContentsParser::b_star(){}
void ContentsParser::B_star(){}
void ContentsParser::BDC()
{
	pop();
	pop();
}
void ContentsParser::inlineImage()
{
	pop();
}
void ContentsParser::BMC()
{
	pop();
}
void ContentsParser::BT(){}
void ContentsParser::BX(){}
void ContentsParser::c()
{
	pop();
	pop();
	pop();
	pop();
	pop();
	pop();
}
void ContentsParser::cm()
{
	pop();
	pop();
	pop();
	pop();
	pop();
	pop();
}
void ContentsParser::CS()
{
	pop();
}
void ContentsParser::cs()
{
	pop();
}
void ContentsParser::d()
{
	pop();
	pop();
}
void ContentsParser::d0()
{
	pop();
	pop();
}
void ContentsParser::d1()
{
	pop();
	pop();
	pop();
	pop();
	pop();
	pop();
}
void ContentsParser::Do()
{
	pop();
}
void ContentsParser::DP()
{
	pop();
	pop();
}
void ContentsParser::EMC(){}
void ContentsParser::ET(){}
void ContentsParser::EX(){}
void ContentsParser::f(){}
void ContentsParser::F(){}
void ContentsParser::f_star(){}
void ContentsParser::G()
{
	pop();
}
void ContentsParser::g()
{
	pop();
}
void ContentsParser::gs()
{
	pop();
}
void ContentsParser::h(){}
void ContentsParser::i()
{
	pop();
}
void ContentsParser::j()
{
	pop();
}
void ContentsParser::J()
{
	pop();
}
void ContentsParser::K()
{
	pop();
	pop();
	pop();
	pop();
}
void ContentsParser::k()
{
	pop();
	pop();
	pop();
	pop();
}
void ContentsParser::l()
{
	pop();
	pop();
}
void ContentsParser::m()
{
	pop();
	pop();
}
void ContentsParser::M()
{
	pop();
}
void ContentsParser::MP()
{
	pop();
}
void ContentsParser::n(){}
void ContentsParser::q(){}
void ContentsParser::Q(){}
void ContentsParser::re()
{
	pop();
	pop();
	pop();
	pop();
}
void ContentsParser::RG()
{
	pop();
	pop();
	pop();
}
void ContentsParser::rg()
{
	pop();
	pop();
	pop();
}
void ContentsParser::ri()
{
	pop();
}
void ContentsParser::s(){}
void ContentsParser::S(){}
void ContentsParser::SC()
{
	assert( false );
}
void ContentsParser::sc()
{
	assert( false );
}
void ContentsParser::SCN()
{
	assert( false );
}
void ContentsParser::scn()
{
	assert( false );
}
void ContentsParser::T_star(){}
void ContentsParser::Tc()
{
	pop();
}
void ContentsParser::Td()
{
	pop();
	pop();
}
void ContentsParser::TD()
{
	pop();
	pop();
}
void ContentsParser::Tf()
{
	pop();
}
void ContentsParser::Tj()
{
	pop();
}
void ContentsParser::TJ()
{
	pop();
}
void ContentsParser::TL()
{
	pop();
}
void ContentsParser::Tm()
{
	pop();
	pop();
	pop();
	pop();
	pop();
	pop();
}
void ContentsParser::Tr()
{
	pop();
}
void ContentsParser::Ts()
{
	pop();
}
void ContentsParser::Tw()
{
	pop();
}
void ContentsParser::Tz()
{
	pop();
}
void ContentsParser::v()
{
	pop();
	pop();
	pop();
	pop();
}
void ContentsParser::w(){}
void ContentsParser::W(){}
void ContentsParser::W_star(){}
void ContentsParser::y()
{
	pop();
	pop();
	pop();
	pop();
}
void ContentsParser::quote()
{
	pop();
}
void ContentsParser::double_quote()
{
	pop();
	pop();
	pop();
}

}
