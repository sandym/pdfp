
#ifndef H_DUMPER_UTILS
#define H_DUMPER_UTILS

template<typename T>
int getNbOfColourComponents( T i_colourSpace )
{
	if ( isNull(i_colourSpace) )
		return 0;

	if ( type(i_colourSpace) == pdfp::Object::Type::k_name )
	{
		auto v = nameValue( i_colourSpace);
		if ( v == "DeviceGray" )
			return 1;
		if ( v == "DeviceRGB" )
			return 3;
		if ( v == "DeviceCMYK" )
			return 4;
		return 0;
	}
	else if ( type(i_colourSpace) == pdfp::Object::Type::k_array )
	{
		auto a = asArray(i_colourSpace);
		if ( arrayCount(a) > 0 )
		{
			auto obj = arrayValue(a, 0 );
			if ( type(obj) == pdfp::Object::Type::k_name )
			{
				auto cs = nameValue(obj);
				if ( cs == "CalGray" )
					return 1;
				else if ( cs == "CalRGB" or cs == "Lab" )
					return 3;
				else if ( cs == "ICCBased" and arrayCount(a) > 1 )
				{
					obj = arrayValue( a, 1 );
					if ( type(obj) == pdfp::Object::Type::k_stream )
					{
						auto d = streamDictionary( asStream(obj) );
						return getNbOfColourComponents(
						    dictValue( d, "Alternate" ) );
					}
				}
				else if ( cs == "Separation" and arrayCount(a) > 2 )
				{
					return getNbOfColourComponents( arrayValue( a, 2 ) );
				}
				else if ( cs == "DeviceN" and arrayCount(a) > 1 )
				{
					obj = arrayValue( a, 1 );
					if ( type(obj) == pdfp::Object::Type::k_array )
						return (int)arrayCount(asArray(obj));
				}
				else if ( cs == "Indexed" and arrayCount(a) > 1 )
				{
					return getNbOfColourComponents( arrayValue( a, 1 ) );
				}
			}
		}
	}
	return 0;
}
template<typename T>
bool getImageInfo( T i_dict,
                   bool isBitmap,
                   int &o_width,
                   int &o_height,
                   int &o_bpc,
                   int &o_nbOfComp )
{
	auto obj = dictValue( i_dict, "Subtype" );
	if ( isNull(obj) or type(obj) != pdfp::Object::Type::k_name or
	     nameValue( obj ) != "Image" )
		return false;

	obj = dictValue( i_dict, "Width" );
	if ( isNull(obj) or type(obj) != pdfp::Object::Type::k_number )
		return false;
	o_width = intValue(obj);

	obj = dictValue( i_dict, "Height" );
	if ( isNull(obj) or type(obj) != pdfp::Object::Type::k_number )
		return false;
	o_height = intValue(obj);

	if ( isBitmap )
	{
		o_nbOfComp = 1;
		o_bpc
		= 1;
	}
	else
	{
		o_nbOfComp = 0;

		obj = dictValue( i_dict, "BitsPerComponent" );
		if ( isNull(obj) or type(obj) != pdfp::Object::Type::k_number )
			return false;
		o_bpc = intValue(obj);

		obj = dictValue( i_dict, "DecodeParms" );
		if ( not isNull(obj) and type(obj) == pdfp::Object::Type::k_dictionary )
		{
			obj = dictValue( asDictionary(obj), "Colors" );
			if ( not isNull(obj) and type(obj) == pdfp::Object::Type::k_number )
				o_nbOfComp = intValue(obj);
		}
		if ( o_nbOfComp == 0 )
		{
			obj = dictValue( i_dict, "ColorSpace" );
			if ( not isNull(obj) )
				o_nbOfComp = getNbOfColourComponents( obj );
		}

		if ( o_nbOfComp == 0 )
			return false;
	}

	return true;
}

template<typename T>
std::streamoff computeStreamLimit( T o )
{
	std::streamoff limit = -1;

	auto dict = streamDictionary( o );
	bool needLimit = false, isBitmap = false;

	std::vector<std::string> filterList;
	auto obj = dictValue( dict, "Filter" );
	if ( not isNull(obj) )
	{
		if ( type(obj) == pdfp::Object::Type::k_name )
			filterList.push_back( nameValue(obj) );
		else if ( type(obj) == pdfp::Object::Type::k_array )
		{
			auto arr = asArray(obj);
			auto count = arrayCount(arr);
			for ( size_t i = 0; i < count; ++i )
			{
				obj = arrayValue( arr, i );
				if ( type(obj) == pdfp::Object::Type::k_name )
					filterList.push_back( nameValue(obj) );
			}
		}
	}
	for ( auto filter : filterList )
	{
		if ( filter == "FlateDecode" or filter == "Fl" )
			needLimit = true;
		if ( filter == "CCITTFaxDecode" or filter == "CCF" or
		     filter == "JBIG2Decode" )
			needLimit = isBitmap = true;
	}

	if ( needLimit )
	{
		int width, height;
		int bpc, cpp;
		if ( getImageInfo( dict, isBitmap, width, height, bpc, cpp ) )
		{
			size_t rowBytes = ( ( ( width * cpp * bpc ) + 7 ) & ( ~7 ) ) / 8;
			limit = rowBytes * height;
		}
	}
	return limit;
}

#endif

