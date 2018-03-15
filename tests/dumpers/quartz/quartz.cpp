
#include "quartz.h"
#include "su/strings/str_ext.h"

su::cfauto<CGPDFDocumentRef> quartz_open( const std::string &i_path )
{
	su::cfauto<CFStringRef> filePath( su::CreateCFString( i_path ) );
	su::cfauto<CFURLRef> url( CFURLCreateWithFileSystemPath(
	    0, filePath, kCFURLPOSIXPathStyle, false ) );

	su::cfauto<CGDataProviderRef> provider(
	    CGDataProviderCreateWithURL( url ) );
	if ( provider == nullptr )
		throw std::runtime_error( "quartz-cannot create provider" );

	su::cfauto<CGPDFDocumentRef> doc(
	    CGPDFDocumentCreateWithProvider( provider ) );
	if ( doc == nullptr )
		throw std::runtime_error( "invalid PDF file" );
	return doc;
}

pdfp::Version version( CGPDFDocumentRef o )
{
	pdfp::Version v{};
	CGPDFDocumentGetVersion( o, &v.major, &v.minor );
	return v;
}
bool isEncrypted( CGPDFDocumentRef o )
{
	return CGPDFDocumentIsEncrypted( o );
}
bool isUnlocked( CGPDFDocumentRef o )
{
	return CGPDFDocumentIsUnlocked( o );
}
size_t nbOfPages( CGPDFDocumentRef o )
{
	return CGPDFDocumentGetNumberOfPages( o );
}
CGPDFDictionaryRef info( CGPDFDocumentRef o )
{
	return CGPDFDocumentGetInfo( o );
}
CGPDFDictionaryRef catalog( CGPDFDocumentRef o )
{
	return CGPDFDocumentGetCatalog( o );
}
bool isNull( CGPDFObjectRef o )
{
	return o == nullptr or CGPDFObjectGetType( o ) == kCGPDFObjectTypeNull;
}
bool isNull( CGPDFDictionaryRef o )
{
	return o == nullptr;
}
pdfp::Object::Type type( CGPDFObjectRef o )
{
	switch ( CGPDFObjectGetType( o ) )
	{
		case kCGPDFObjectTypeNull:
			return pdfp::Object::Type::k_null;
		case kCGPDFObjectTypeBoolean:
			return pdfp::Object::Type::k_boolean;
		case kCGPDFObjectTypeInteger:
		case kCGPDFObjectTypeReal:
			return pdfp::Object::Type::k_number;
		case kCGPDFObjectTypeName:
			return pdfp::Object::Type::k_name;
		case kCGPDFObjectTypeString:
			return pdfp::Object::Type::k_string;
		case kCGPDFObjectTypeArray:
			return pdfp::Object::Type::k_array;
		case kCGPDFObjectTypeDictionary:
			return pdfp::Object::Type::k_dictionary;
		case kCGPDFObjectTypeStream:
			return pdfp::Object::Type::k_stream;
		default:
			break;
	}
	assert( false );
	return pdfp::Object::Type::k_invalid;
}
bool boolValue( CGPDFObjectRef o )
{
	CGPDFBoolean v;
	bool res[[maybe_unused]] =
	    CGPDFObjectGetValue( o, kCGPDFObjectTypeBoolean, &v );
	assert( res );
	return v;
}
bool numberIsInt( CGPDFObjectRef o )
{
	return CGPDFObjectGetType( o ) == kCGPDFObjectTypeInteger;
}
int intValue( CGPDFObjectRef o )
{
	CGPDFInteger v;
	bool res[[maybe_unused]] =
	    CGPDFObjectGetValue( o, kCGPDFObjectTypeInteger, &v );
	assert( res );
	return v;
}
float floatValue( CGPDFObjectRef o )
{
	CGPDFReal v;
	bool res[[maybe_unused]] =
	    CGPDFObjectGetValue( o, kCGPDFObjectTypeReal, &v );
	assert( res );
	return v;
}
std::string nameValue( CGPDFObjectRef o )
{
	const char *v;
	bool res[[maybe_unused]] =
	    CGPDFObjectGetValue( o, kCGPDFObjectTypeName, &v );
	assert( res );
	return v;
}
std::string stringValue( CGPDFObjectRef o )
{
	CGPDFStringRef v;
	bool res[[maybe_unused]] =
	    CGPDFObjectGetValue( o, kCGPDFObjectTypeString, &v );
	assert( res );
	return {(const char *)CGPDFStringGetBytePtr( v ),
	        CGPDFStringGetLength( v )};
}
CGPDFArrayRef asArray( CGPDFObjectRef o )
{
	CGPDFArrayRef value;
	bool res[[maybe_unused]] =
	    CGPDFObjectGetValue( o, kCGPDFObjectTypeArray, &value );
	assert( res );
	return value;
}
CGPDFDictionaryRef asDictionary( CGPDFObjectRef o )
{
	CGPDFDictionaryRef value;
	bool res[[maybe_unused]] =
	    CGPDFObjectGetValue( o, kCGPDFObjectTypeDictionary, &value );
	assert( res );
	return value;
}
CGPDFStreamRef asStream( CGPDFObjectRef o )
{
	CGPDFStreamRef value;
	bool res[[maybe_unused]] =
	    CGPDFObjectGetValue( o, kCGPDFObjectTypeStream, &value );
	assert( res );
	return value;
}
size_t arrayCount( CGPDFArrayRef o )
{
	return CGPDFArrayGetCount( o );
}
CGPDFObjectRef arrayValue( CGPDFArrayRef o, int i )
{
	CGPDFObjectRef value;
	if ( CGPDFArrayGetObject( o, i, &value ) )
		return value;
	else
		return nullptr;
}
std::vector<std::string> keys( CGPDFDictionaryRef o )
{
	std::vector<std::string> keys;
	CGPDFDictionaryApplyFunction(
	    o,
	    []( const char *key, CGPDFObjectRef, void *info ) {
		    auto *keys = (std::vector<std::string> *)info;
		    if ( std::find( keys->begin(), keys->end(), key ) == keys->end() )
			    keys->push_back( key );
	    },
	    &keys );
	return keys;
}
CGPDFObjectRef dictValue( CGPDFDictionaryRef o, const std::string &k )
{
	CGPDFObjectRef value;
	if ( CGPDFDictionaryGetObject( o, k.c_str(), &value ) )
		return value;
	else
		return nullptr;
}
CGPDFDictionaryRef streamDictionary( CGPDFStreamRef o )
{
	return CGPDFStreamGetDictionary( o );
}

pdfp::Data streamData( CGPDFStreamRef o )
{
	auto limit = computeStreamLimit( o );

	pdfp::Data result;

	CGPDFDataFormat format;
	auto data = CGPDFStreamCopyData( o, &format );
	if ( limit == -1 )
		limit = CFDataGetLength( data );
	else
		limit = std::min<std::streamoff>( limit, CFDataGetLength( data ) );
	assert( data != nullptr );
	switch ( format )
	{
		case CGPDFDataFormatRaw:
			result.format = pdfp::data_format_t::kRaw;
			break;
		case CGPDFDataFormatJPEGEncoded:
			result.format = pdfp::data_format_t::kJPEGEncoded;
			break;
		case CGPDFDataFormatJPEG2000:
			result.format = pdfp::data_format_t::kJPEG2000;
			break;
	}

	result.length = limit;
	result.buffer.reset( new uint8_t[limit] );
	CFRange range;
	range.location = 0;
	range.length = limit;
	CFDataGetBytes( data, range, result.buffer.get() );

	CFRelease( data );

	return result;
}
