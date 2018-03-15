/*
 *  PDFPage.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 05-11-05.
 *  Copyright 2005 Sandy Martel. All rights reserved.
 *
 */

#include "PDFPage.h"
#include <string>

namespace {

pdfp::Rect extractRect( pdfp::Object i_dict, const std::string &i_key )
{
	auto obj = i_dict[i_key];
	while ( not i_dict.is_null() and obj.is_null() )
	{
		i_dict = i_dict["Parent"];
		obj = i_dict[i_key];
	}
	auto arr = obj.array_items_resolved();
	if ( arr.size() >= 4 and
			arr[0].is_number() and
			arr[1].is_number() and
			arr[2].is_number() and
			arr[3].is_number() )
	{
		auto a = arr[0].real_value();
		auto b = arr[1].real_value();
		auto c = arr[2].real_value();
		auto d = arr[3].real_value();
		return pdfp::Rect{a, b, c - a, d - b};
	}
	return {};
}
}

namespace pdfp {

Rect Page::mediaBox() const
{
	return extractRect( _dict, "MediaBox" );
}

Rect Page::cropBox() const
{
	return extractRect( _dict, "CropBox" );
}

Rect Page::bleedBox() const
{
	return extractRect( _dict, "BleedBox" );
}

Rect Page::trimBox() const
{
	return extractRect( _dict, "TrimBox" );
}

Rect Page::artBox() const
{
	return extractRect( _dict, "ArtBox" );
}

int Page::rotate() const
{
	return _dict["Rotate"].int_value();
}

Object Page::contents() const
{
	return _dict["Contents"];
}

}
