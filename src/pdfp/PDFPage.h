/*
 *  PDFPage.h
 *  pdfp
 *
 *  Created by Sandy Martel on 05-11-05.
 *  Copyright 2005 Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_PDFPAGGE
#define H_PDFP_PDFPAGGE

#include "PDFObject.h"

namespace pdfp {

struct Rect
{
	float x{ 0 };
	float y{ 0 };
	float width{ std::numeric_limits<float>::infinity() };
	float height{ std::numeric_limits<float>::infinity() };
	
	bool isNull() const
	{
		return x == 0 and y == 0 and
				width == std::numeric_limits<float>::infinity() and
				height == std::numeric_limits<float>::infinity();
	}
};

class Page final
{
public:
	Page() = default;
	
	Page( const Object &i_dict ) : _dict( i_dict ) {}
	~Page() = default;

	bool is_null() const { return _dict.is_null(); }
	
	Rect mediaBox() const;
	Rect cropBox() const;
	Rect bleedBox() const;
	Rect trimBox() const;
	Rect artBox() const;
	int rotate() const;
	Object contents() const;
	Object dictionary() const { return _dict; }

	void dump() const { dictionary().dump(); }

private:
	Object _dict;
};
}

#endif
