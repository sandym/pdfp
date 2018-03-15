/*
 *  PDFContentsParser.h
 *  pdfp
 *
 *  Created by Sandy Martel on 2018-03-04.
 *  Copyright 2006 Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_PDFContentsParser
#define H_PDFP_PDFContentsParser

#include "pdfp/PDFObject.h"
#include <stack>

namespace pdfp {

struct GState
{
	Object strokeColourSpace;
	Object fillColourSpace;
	// strokeColour;
	// fillColour;
	// renderingIntent;
	// ctm
	float w; // line width
	float M; // miter limit
	int j; // line join
	int J; // line cap
	bool OP; // overprint for stroking
	bool op; // overprint for filling
	int OPM; // overprint mode
	int halftoneType;
	// trSpecified
	// ca
	// CA
	// BM
	// SMask
	int tr; // text rendering mode
	float fs; // font size
	// font
	// font name
	float tl; // text leading
	float ws; // word spacing
	float cs; // character spacinf
	float r; // text rise
	float h; // horizontal scaling
	// float FL;
	// clip path
};

class ContentsParser
{
public:
	ContentsParser( const Object &i_graphic );

	void parse();

	virtual void b();
	virtual void B();
	virtual void b_star();
	virtual void B_star();
	virtual void BDC();
	virtual void inlineImage();
	virtual void BMC();
	virtual void BT();
	virtual void BX();
	virtual void c();
	virtual void cm();
	virtual void CS();
	virtual void cs();
	virtual void d();
	virtual void d0();
	virtual void d1();
	virtual void Do();
	virtual void DP();
	virtual void EMC();
	virtual void ET();
	virtual void EX();
	virtual void f();
	virtual void F();
	virtual void f_star();
	virtual void G();
	virtual void g();
	virtual void gs();
	virtual void h();
	virtual void i();
	virtual void j();
	virtual void J();
	virtual void K();
	virtual void k();
	virtual void l();
	virtual void m();
	virtual void M();
	virtual void MP();
	virtual void n();
	virtual void q();
	virtual void Q();
	virtual void re();
	virtual void RG();
	virtual void rg();
	virtual void ri();
	virtual void s();
	virtual void S();
	virtual void SC();
	virtual void sc();
	virtual void SCN();
	virtual void scn();
	virtual void T_star();
	virtual void Tc();
	virtual void Td();
	virtual void TD();
	virtual void Tf();
	virtual void Tj();
	virtual void TJ();
	virtual void TL();
	virtual void Tm();
	virtual void Tr();
	virtual void Ts();
	virtual void Tw();
	virtual void Tz();
	virtual void v();
	virtual void w();
	virtual void W();
	virtual void W_star();
	virtual void y();
	virtual void quote();
	virtual void double_quote();

protected:
	Object pop();
	float pop_real() { return pop().real_value(); }
	std::string pop_name() { return std::string{ pop().name_value() }; }
	std::string pop_string() { return std::string{ pop().string_value() }; }

	Object get_xobject( const std::string &i_name );
	Object get_font( const std::string &i_name );
	Object get_colourspace( const std::string &i_name );
	Object get_pattern( const std::string &i_name );
	Object get_shading( const std::string &i_name );

private:
	Object _graphic;
	std::vector<Object> _stack;
	std::stack<GState> _gstateStack;
	
	// path
	// text matrices
};

}

#endif
