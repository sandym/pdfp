

#ifndef H_QUARTZ_DUMPER
#define H_QUARTZ_DUMPER

#include <ApplicationServices/ApplicationServices.h>
#include "pdfp/PDFDocument.h"
#include "su/base/cfauto.h"
#include "dumper_utils.h"

su::cfauto<CGPDFDocumentRef> quartz_open( const std::string &i_path );

pdfp::Version version( CGPDFDocumentRef o );
bool isEncrypted( CGPDFDocumentRef o );
bool isUnlocked( CGPDFDocumentRef o );
size_t nbOfPages( CGPDFDocumentRef o );
CGPDFDictionaryRef info( CGPDFDocumentRef o );
CGPDFDictionaryRef catalog( CGPDFDocumentRef o );
bool isNull( CGPDFObjectRef o );
bool isNull( CGPDFDictionaryRef o );
pdfp::Object::Type type( CGPDFObjectRef o );
bool boolValue( CGPDFObjectRef o );
bool numberIsInt( CGPDFObjectRef o );
int intValue( CGPDFObjectRef o );
float floatValue( CGPDFObjectRef o );
std::string nameValue( CGPDFObjectRef o );
std::string stringValue( CGPDFObjectRef o );
CGPDFArrayRef asArray( CGPDFObjectRef o );
CGPDFDictionaryRef asDictionary( CGPDFObjectRef o );
CGPDFStreamRef asStream( CGPDFObjectRef o );
size_t arrayCount( CGPDFArrayRef o );
CGPDFObjectRef arrayValue( CGPDFArrayRef o, int i );
std::vector<std::string> keys( CGPDFDictionaryRef o );
CGPDFObjectRef dictValue( CGPDFDictionaryRef o, const std::string &k );
CGPDFDictionaryRef streamDictionary( CGPDFStreamRef o );

pdfp::Data streamData( CGPDFStreamRef o );

inline uintptr_t hashKey( CGPDFDictionaryRef o )
{
	return (uintptr_t)o;
}
inline uintptr_t hashKey( CGPDFArrayRef o )
{
	return (uintptr_t)o;
}
inline uintptr_t hashKey( CGPDFStreamRef o )
{
	return (uintptr_t)o;
}

#endif
