
#include "apdfl.h"
#include "su/log/logger.h"

#if UPLATFORM_MAC
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "PDFLInitCommon.c"
#include "PDFLInitHFT.c"
//#include "CosExpT.h"

#if UPLATFORM_MAC
#	pragma GCC diagnostic pop
#endif

struct APDFLInit
{
	APDFLInit()
	{
		// init APDFL
		PDFLDataRec pdflData;
		memset( &pdflData, 0, sizeof( pdflData ) );
		pdflData.size = sizeof( pdflData );
		auto err = ::PDFLInitHFT( &pdflData );
		if ( err != 0 )
		{
			char buffer[1024];
			log_error() << "PDFLInitHFT: "
			            << ASGetErrorString( err, buffer, 1024 );
		}
	}
};
std::unique_ptr<APDFLInit> g_apdflInit;

void apdfl_init()
{
	if ( g_apdflInit.get() == nullptr )
		g_apdflInit = std::make_unique<APDFLInit>();
}

std::unique_ptr<std::remove_pointer_t<PDDoc>, std::function<void( PDDoc )>>
apdfl_open( const std::string &i_path )
{
	if ( g_apdflInit.get() == nullptr )
		g_apdflInit = std::make_unique<APDFLInit>();

	ASFileSys asFileSys = ASGetDefaultFileSys();

	ASPathName asPathName = nullptr;
	PDDoc doc = nullptr;

	DURING
	asPathName = ASFileSysCreatePathName(
	    asFileSys, ASAtomFromString( "POSIXPath" ), i_path.c_str(), 0 );

	doc = PDDocOpen( asPathName, asFileSys, nullptr, true );
	HANDLER
	log_error() << "Unable to open " << i_path;
	END_HANDLER

	// Release asPathName
	if ( asPathName )
		ASFileSysReleasePath( asFileSys, asPathName );

	if ( doc == nullptr )
		throw std::runtime_error( "invalid PDF file" );

	return {doc, []( PDDoc d ) { PDDocClose( d ); }};
}

pdfp::Version version( PDDoc o )
{
	ASInt16 major, minor;
	PDDocGetVersion( o, &major, &minor );
	return {major, minor};
}
bool isEncrypted( PDDoc o )
{
	auto v = PDDocGetCryptHandler( o );
	return v != ASAtomNull;
}
bool isUnlocked( PDDoc o )
{
	return true; // ?
}
size_t nbOfPages( PDDoc o )
{
	return PDDocGetNumPages( o );
}
APDFLDict info( PDDoc o )
{
	return {CosDocGetInfoDict( PDDocGetCosDoc( o ) )};
}
APDFLDict catalog( PDDoc o )
{
	return {CosDocGetRoot( PDDocGetCosDoc( o ) )};
}

bool isNull( APDFLObject o )
{
	return CosObjGetType( o.obj ) == CosNull;
}
bool isNull( APDFLDict o )
{
	return CosObjGetType( o.dict ) == CosNull;
}
pdfp::Object::Type type( APDFLObject o )
{
	switch ( CosObjGetType( o.obj ) )
	{
		case CosNull:
			return pdfp::Object::Type::k_null;
		case CosBoolean:
			return pdfp::Object::Type::k_boolean;
		case CosInteger:
		case CosReal:
			return pdfp::Object::Type::k_number;
		case CosName:
			return pdfp::Object::Type::k_name;
		case CosString:
			return pdfp::Object::Type::k_string;
		case CosArray:
			return pdfp::Object::Type::k_array;
		case CosDict:
			return pdfp::Object::Type::k_dictionary;
		case CosStream:
			return pdfp::Object::Type::k_stream;
		default:
			break;
	}
	assert( false );
	return pdfp::Object::Type::k_invalid;
}
bool boolValue( APDFLObject o )
{
	return CosBooleanValue( o.obj );
}
bool numberIsInt( APDFLObject o )
{
	return CosObjGetType( o.obj ) == CosInteger;
}
int intValue( APDFLObject o )
{
	return CosIntegerValue( o.obj );
}
float floatValue( APDFLObject o )
{
	return CosFloatValue( o.obj );
}
std::string nameValue( APDFLObject o )
{
	return ASAtomGetString( CosNameValue( o.obj ) );
}
std::string stringValue( APDFLObject o )
{
	ASTCount c = 0;
	auto p = CosStringValue( o.obj, &c );
	return {p, (size_t)c};
}
APDFLArray asArray( APDFLObject o )
{
	return {o.obj};
}
APDFLDict asDictionary( APDFLObject o )
{
	return {o.obj};
}
APDFLStream asStream( APDFLObject o )
{
	return {o.obj};
}
size_t arrayCount( APDFLArray o )
{
	return CosArrayLength( o.array );
}
APDFLObject arrayValue( APDFLArray o, int i )
{
	return {CosArrayGet( o.array, i )};
}
std::vector<std::string> keys( APDFLDict o )
{
	std::vector<std::string> ks;

	CosObjEnum( o.dict,
	            []( CosObj obj, CosObj value, void *clientData ) -> ASBool {
		            ( (std::vector<std::string> *)clientData )
		                ->push_back( ASAtomGetString( CosNameValue( obj ) ) );
		            return true;
	            },
	            &ks );

	return ks;
}
APDFLObject dictValue( APDFLDict o, const std::string &k )
{
	return {CosDictGetKeyString( o.dict, k.c_str() )};
}
APDFLDict streamDictionary( APDFLStream o )
{
	return {CosStreamDict( o.stream )};
}

pdfp::Data streamData( APDFLStream o )
{
	auto limit = computeStreamLimit( o );
	auto i_stream = o.stream;

	pdfp::Data result;

	result.format = pdfp::data_format_t::kRaw;

	auto dict = CosStreamDict( i_stream );
	auto filter = CosDictGetKeyString( dict, "Filter" );
	auto decodeParms = CosDictGetKeyString( dict, "DecodeParms" );

	CosObj filterName{};
	CosObj filterArray{};
	if ( CosObjGetType( filter ) == CosName )
	{
		std::string n( ASAtomGetString( CosNameValue( filter ) ) );
		if ( n == "JPXDecode" or n == "DCTDecode" )
		{
			filterName = filter;
			CosDictRemoveKeyString( dict, "Filter" );
			result.format = n == "JPXDecode" ?
			                    pdfp::data_format_t::kJPEG2000 :
			                    pdfp::data_format_t::kJPEGEncoded;
		}
	}
	else if ( CosObjGetType( filter ) == CosArray )
	{
		std::vector<std::string> arr;
		CosObjEnum(
		    filter,
		    []( CosObj obj, CosObj value, void *clientData ) -> ASBool {
			    ( (std::vector<std::string> *)clientData )
			        ->push_back( ASAtomGetString( CosNameValue( obj ) ) );
			    return true;
		    },
		    &arr );
		auto it = std::find( arr.begin(), arr.end(), "DCTDecode" );
		if ( it != arr.end() )
		{
			result.format = pdfp::data_format_t::kJPEGEncoded;
			if ( arr.size() == 1 )
			{
				filterArray = filter;
				CosDictRemoveKeyString( dict, "Filter" );
				CosDictRemoveKeyString( dict, "DecodeParms" );
			}
			else
			{
				assert( false );
			}
		}
		else
		{
			auto it = std::find( arr.begin(), arr.end(), "JPXDecode" );
			if ( it != arr.end() )
			{
				result.format = pdfp::data_format_t::kJPEG2000;
				filterArray = filter;
				assert( false );
			}
		}
	}

	auto stm =
	    CosStreamOpenStm( i_stream, cosOpenFiltered ); // CosOpenUnfiltered

	size_t capacity = 0;
	for ( ;; )
	{
		char buffer[4096];
		auto l = ASStmRead( buffer, 1, 4096, stm );
		if ( ( result.length + l ) > capacity )
		{
			capacity = std::max( std::max( result.length + l, capacity * 2 ),
			                     size_t( 128 ) );
			auto newData = std::make_unique<uint8_t[]>( capacity );
			memcpy( newData.get(), result.buffer.get(), result.length );
			result.buffer = std::move( newData );
		}
		if ( l <= 0 )
			break;
		memcpy( result.buffer.get() + result.length, buffer, l );
		result.length += l;
	}
	ASStmClose( stm );
	if ( limit != -1 )
		result.length = std::min( size_t( limit ), result.length );

	if ( CosObjGetType( filterName ) != CosNull )
	{
		CosDictPutKeyString( dict, "Filter", filterName );
	}
	else if ( CosObjGetType( filterArray ) != CosNull )
	{
		CosDictPutKeyString( dict, "Filter", filterArray );
		if ( CosObjGetType( decodeParms ) != CosNull )
			CosDictPutKeyString( dict, "DecodeParms", decodeParms );
	}

	return result;
}
