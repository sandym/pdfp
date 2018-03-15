
#include "poppler.h"
#include <cassert>

std::unique_ptr<PDFDoc> poppler_open( const std::string &i_path )
{
	auto doc = std::make_unique<PDFDoc>( new GooString( i_path.c_str() ) );
	if ( not doc->isOk() )
		throw std::runtime_error( "invalid PDF file" );
	return doc;
}

pdfp::Version version( PDFDoc *o )
{
	pdfp::Version v{};
	v.major = o->getPDFMajorVersion();
	v.minor = o->getPDFMinorVersion();
	auto cat = catalog( o );
	if ( cat.dict != nullptr )
	{
		auto obj = dictValue( cat, "Version" );
		if ( not isNull( obj ) and type( obj ) == pdfp::Object::Type::k_name )
		{
			auto vers = nameValue( obj );
			if ( vers.size() == 3 and isdigit( vers[0] ) and vers[1] == '.' and
			     isdigit( vers[2] ) )
			{
				v.major = vers[0] - '0';
				v.minor = vers[2] - '0';
			}
		}
	}
	return v;
}

bool isEncrypted( PDFDoc *o )
{
	return o->isEncrypted();
}
bool isUnlocked( PDFDoc *o )
{
	return true; // ?
}
size_t nbOfPages( PDFDoc *o )
{
	return o->getNumPages();
}
PopplerDict info( PDFDoc *o )
{
	Object obj;
	o->getDocInfo( &obj );
	if ( obj.isDict() )
		return {o, obj.getDict(), 0};
	return {};
}
PopplerDict catalog( PDFDoc *o )
{
	Object obj;
	o->getXRef()->getCatalog( &obj );
	return {o, obj.getDict(), 0};
}
bool isNull( const PopplerObject &o )
{
	return const_cast<PopplerObject *>( &o )->obj.isNull();
}
bool isNull( const PopplerDict &o )
{
	return o.dict == nullptr;
}

pdfp::Object::Type type( const PopplerObject &o )
{
	switch ( const_cast<PopplerObject *>( &o )->obj.getType() )
	{
		case objNull:
			return pdfp::Object::Type::k_null;
		case objBool:
			return pdfp::Object::Type::k_boolean;
		case objInt:
		case objReal:
		case objInt64:
			return pdfp::Object::Type::k_number;
		case objName:
			return pdfp::Object::Type::k_name;
		case objString:
			return pdfp::Object::Type::k_string;
		case objArray:
			return pdfp::Object::Type::k_array;
		case objDict:
			return pdfp::Object::Type::k_dictionary;
		case objStream:
			return pdfp::Object::Type::k_stream;
		default:
			break;
	}
	assert( false );
	return pdfp::Object::Type::k_invalid;
}
bool boolValue( const PopplerObject &o )
{
	return const_cast<PopplerObject *>( &o )->obj.getBool();
}
bool numberIsInt( const PopplerObject &o )
{
	return const_cast<PopplerObject *>( &o )->obj.isInt();
}
int intValue( const PopplerObject &o )
{
	return const_cast<PopplerObject *>( &o )->obj.getInt();
}
float floatValue( const PopplerObject &o )
{
	return const_cast<PopplerObject *>( &o )->obj.getReal();
}
std::string nameValue( const PopplerObject &o )
{
	return const_cast<PopplerObject *>( &o )->obj.getName();
}
std::string stringValue( const PopplerObject &o )
{
	auto s = const_cast<PopplerObject *>( &o )->obj.getString();
	return {s->getCString(), (size_t)s->getLength()};
}
PopplerArray asArray( const PopplerObject &o )
{
	return {o.doc, const_cast<PopplerObject *>( &o )->obj.getArray(), o.h};
}
PopplerDict asDictionary( const PopplerObject &o )
{
	return {o.doc, const_cast<PopplerObject *>( &o )->obj.getDict(), o.h};
}
PopplerStream asStream( const PopplerObject &o )
{
	return {o.doc, const_cast<PopplerObject *>( &o )->obj.getStream(), o.h};
}
size_t arrayCount( PopplerArray o )
{
	return o.array->getLength();
}
PopplerObject arrayValue( PopplerArray o, int i )
{
	Object obj;
	o.array->getNF( i, &obj );
	if ( obj.isRef() )
	{
		Object obj2;
		o.doc->getXRef()->fetch( obj.getRef().num, obj.getRef().gen, &obj2 );
		uintptr_t h = obj.getRef().num;
		h = ( h << 32 ) + obj.getRef().gen;
		return {o.doc, obj2, h};
	}
	return {o.doc, obj, 0};
}
std::vector<std::string> keys( PopplerDict o )
{
	std::vector<std::string> ks;
	for ( int i = 0; i < o.dict->getLength(); ++i )
		ks.emplace_back( o.dict->getKey( i ) );
	return ks;
}

PopplerObject dictValue( PopplerDict o, const std::string &k )
{
	Object obj;
	o.dict->lookupNF( k.c_str(), &obj );
	if ( obj.isRef() )
	{
		Object obj2;
		o.doc->getXRef()->fetch( obj.getRef().num, obj.getRef().gen, &obj2 );
		uintptr_t h = obj.getRef().num;
		h = ( h << 32 ) + obj.getRef().gen;
		return {o.doc, obj2, h};
	}
	return {o.doc, obj, 0};
}
PopplerDict streamDictionary( PopplerStream o )
{
	return {o.doc, o.stream->getDict()};
}

pdfp::Data streamData( PopplerStream o )
{
	auto limit = computeStreamLimit( o );

	pdfp::Data result;

	auto i_stream = o.stream;
	result.format = pdfp::data_format_t::kRaw;
	if ( i_stream->getKind() == strDCT )
	{
		result.format = pdfp::data_format_t::kJPEGEncoded;
		i_stream = i_stream->getNextStream();
	}
	if ( i_stream->getKind() == strJPX )
	{
		result.format = pdfp::data_format_t::kJPEG2000;
		i_stream = i_stream->getNextStream();
	}
	i_stream->reset();

	int c;
	size_t capacity = 0;
	while ( ( c = i_stream->getChar() ) != -1 )
	{
		if ( ( result.length + 1 ) > capacity )
		{
			capacity = std::max( std::max( result.length + 1, capacity * 2 ),
			                     size_t( 128 ) );
			auto newData = std::make_unique<uint8_t[]>( capacity );
			memcpy( newData.get(), result.buffer.get(), result.length );
			result.buffer = std::move( newData );
		}
		result.buffer[result.length] = c;
		++result.length;
	}
	if ( i_stream->getKind() == strASCIIHex )
	{
		--result.length;
	}
	else if ( i_stream->getKind() == strFile )
	{
		int wanted = -1;
		if ( i_stream->getDict()->lookupInt( "Length", "L", &wanted ) )
		{
			if ( ( wanted + 1 ) == result.length and
			     ( result.buffer[wanted] == 10 or
			       result.buffer[wanted] == 13 ) )
			{
				--result.length;
			}
			else if ( ( wanted + 2 ) == result.length and
			          result.buffer[wanted] == 13 and
			          result.buffer[wanted + 1] == 10 )
			{
				result.length -= 2;
			}
		}
	}
	if ( limit != -1 )
		result.length = std::min( size_t( limit ), result.length );

	return result;
}
