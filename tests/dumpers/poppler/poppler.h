
#ifndef H_POPPLER_DUMPER
#define H_POPPLER_DUMPER

#include <PDFDoc.h>
#include <memory>
#include "pdfp/PDFDocument.h"
#include "dumper_utils.h"

std::unique_ptr<PDFDoc> poppler_open( const std::string &i_path );

struct PopplerObject
{
	PDFDoc *doc{nullptr};
	Object obj;
	uintptr_t h{0};
};
struct PopplerArray
{
	PDFDoc *doc{nullptr};
	Array *array{nullptr};
	uintptr_t h{0};
};
struct PopplerDict
{
	PDFDoc *doc{nullptr};
	Dict *dict{nullptr};
	uintptr_t h{0};
};
struct PopplerStream
{
	PDFDoc *doc{nullptr};
	Stream *stream{nullptr};
	uintptr_t h{0};
};
inline uintptr_t hashKey( PopplerDict o )
{
	return o.h != 0 ? o.h : (uintptr_t)o.dict;
}
inline uintptr_t hashKey( PopplerArray o )
{
	return o.h != 0 ? o.h : (uintptr_t)o.array;
}
inline uintptr_t hashKey( PopplerStream o )
{
	return o.h != 0 ? o.h : (uintptr_t)o.stream;
}

pdfp::Version version( PDFDoc *o );
bool isEncrypted( PDFDoc *o );
bool isUnlocked( PDFDoc *o );
size_t nbOfPages( PDFDoc *o );
PopplerDict info( PDFDoc *o );
PopplerDict catalog( PDFDoc *o );
bool isNull( const PopplerObject &o );
bool isNull( const PopplerDict &o );
pdfp::Object::Type type( const PopplerObject &o );
bool boolValue( const PopplerObject &o );
bool numberIsInt( const PopplerObject &o );
int intValue( const PopplerObject &o );
float floatValue( const PopplerObject &o );
std::string nameValue( const PopplerObject &o );
std::string stringValue( const PopplerObject &o );
PopplerArray asArray( const PopplerObject &o );
PopplerDict asDictionary( const PopplerObject &o );
PopplerStream asStream( const PopplerObject &o );
size_t arrayCount( PopplerArray o );
PopplerObject arrayValue( PopplerArray o, int i );
std::vector<std::string> keys( PopplerDict o );
PopplerObject dictValue( PopplerDict o, const std::string &k );
PopplerDict streamDictionary( PopplerStream o );

pdfp::Data streamData( PopplerStream o );

#endif
