

#ifndef H_APDFL_DUMPER
#define H_APDFL_DUMPER

#include "pdfp/PDFDocument.h"
#include "dumper_utils.h"

#include "APDFL_config.h"
#include "PDFInit.h"
#include "PDFLCalls.h"
#include "PDCalls.h"

void apdfl_init();

std::unique_ptr<std::remove_pointer_t<PDDoc>, std::function<void( PDDoc )>>
apdfl_open( const std::string &i_path );

struct APDFLObject
{
	CosObj obj;
};
struct APDFLArray
{
	CosObj array;
};
struct APDFLDict
{
	CosObj dict;
};
struct APDFLStream
{
	CosObj stream;
};

pdfp::Version version( PDDoc o );
bool isEncrypted( PDDoc o );
bool isUnlocked( PDDoc o );
size_t nbOfPages( PDDoc o );
APDFLDict info( PDDoc o );
APDFLDict catalog( PDDoc o );
bool isNull( APDFLObject o );
bool isNull( APDFLDict o );
pdfp::Object::Type type( APDFLObject o );
bool boolValue( APDFLObject o );
bool numberIsInt( APDFLObject o );
int intValue( APDFLObject o );
float floatValue( APDFLObject o );
std::string nameValue( APDFLObject o );
std::string stringValue( APDFLObject o );
APDFLArray asArray( APDFLObject o );
APDFLDict asDictionary( APDFLObject o );
APDFLStream asStream( APDFLObject o );
size_t arrayCount( APDFLArray o );
APDFLObject arrayValue( APDFLArray o, int i );
std::vector<std::string> keys( APDFLDict o );
APDFLObject dictValue( APDFLDict o, const std::string &k );
APDFLDict streamDictionary( APDFLStream o );

pdfp::Data streamData( APDFLStream o );

inline uintptr_t hashKey( const APDFLArray &v )
{
	return *( (uintptr_t *)&v.array );
}
inline uintptr_t hashKey( const APDFLDict &v )
{
	return *( (uintptr_t *)&v.dict );
}
inline uintptr_t hashKey( const APDFLStream &v )
{
	return *( (uintptr_t *)&v.stream );
}
#endif
