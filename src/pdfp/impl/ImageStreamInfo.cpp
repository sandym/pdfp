//
//  ImageStreamInfo.cpp
//  pdfp
//
//  Created by Sandy Martel on 2013/01/16.
//
//

#include "ImageStreamInfo.h"
#include "pdfp/PDFObject.h"
#include <string>

namespace pdfp {

int getNbOfColourComponents( const Object &i_colourSpace )
{
	if ( i_colourSpace.is_null() )
		return 0;

	if ( i_colourSpace.is_name() )
	{
		auto v = i_colourSpace.name_value();
		if ( v == "DeviceGray" )
			return 1;
		if ( v == "DeviceRGB" )
			return 3;
		if ( v == "DeviceCMYK" )
			return 4;
		return 0;
	}
	else if ( i_colourSpace.is_array() )
	{
		auto a = i_colourSpace.array_items_resolved();
		if ( not a.empty() )
		{
			auto &obj = a[0];
			if ( obj.is_name() )
			{
				auto cs = obj.name_value();
				if ( cs == "CalGray" )
					return 1;
				else if ( cs == "CalRGB" or cs == "Lab" )
					return 3;
				else if ( cs == "ICCBased" and a.size() > 1 )
				{
					auto &obj = a[1];
					if ( obj.is_stream() )
					{
						auto &d = obj.stream_dictionary();
						return getNbOfColourComponents( d["Alternate"] );
					}
				}
				else if ( cs == "Separation" and a.size() > 2 )
				{
					return getNbOfColourComponents( a[2] );
				}
				else if ( cs == "DeviceN" and a.size() > 1 )
				{
					auto &obj = a[1];
					if ( obj.is_array() )
						return (int)obj.array_size();
				}
				else if ( cs == "Indexed" and a.size() > 1 )
				{
					return getNbOfColourComponents( a[1] );
				}
			}
		}
	}
	return 0;
}

bool getImageInfo( const Object &i_dict,
                   bool isBitmap,
                   int &o_width,
                   int &o_height,
                   int &o_bpc,
                   int &o_nbOfComp )
{
	auto &subtype = i_dict["Subtype"];
	if ( subtype.is_null() or not subtype.is_name() or
	     subtype.name_value() != "Image" )
	{
		return false;
	}
	
	auto &w = i_dict["Width"];
	if ( w.is_null() or not w.is_number() )
		return false;
	o_width = w.int_value();

	auto &h = i_dict["Height"];
	if ( h.is_null() or not h.is_number() )
		return false;
	o_height = h.int_value();

	if ( isBitmap )
	{
		o_nbOfComp = 1;
		o_bpc = 1;
	}
	else
	{
		o_nbOfComp = 0;

		auto &bpc = i_dict["BitsPerComponent"];
		if ( bpc.is_null() or not bpc.is_number() )
			return false;
		o_bpc = bpc.int_value();

		auto &dc = i_dict["DecodeParms"];
		if ( dc.is_dictionary() )
		{
			auto &c = dc["Colors"];
			if ( c.is_number() )
				o_nbOfComp = c.int_value();
		}
		if ( o_nbOfComp == 0 )
		{
			auto &cs = i_dict["ColorSpace"];
			if ( not cs.is_null() )
				o_nbOfComp = getNbOfColourComponents( cs );
		}

		if ( o_nbOfComp == 0 )
			return false;
	}

	return true;
}
}
