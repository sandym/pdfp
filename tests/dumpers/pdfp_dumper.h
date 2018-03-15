
#ifndef H_PDFP_DUMPER
#define H_PDFP_DUMPER

#include "pdfp/PDFDocument.h"
#include "dumper_utils.h"

inline pdfp::Version version( const pdfp::Document *o )
{
	return o->version();
}
inline bool isEncrypted( const pdfp::Document *o )
{
	return o->isEncrypted();
}
inline bool isUnlocked( const pdfp::Document *o )
{
	return o->isUnlocked();
}
inline size_t nbOfPages( const pdfp::Document *o )
{
	return o->nbOfPages();
}
inline pdfp::Object info( const pdfp::Document *o )
{
	return o->info();
}
inline pdfp::Object catalog( const pdfp::Document *o )
{
	return o->catalog();
}
inline bool isNull( const pdfp::Object &o )
{
	return o.is_null();
}
inline pdfp::Object::Type type( const pdfp::Object &o )
{
	return o.type();
}
inline bool boolValue( const pdfp::Object &o )
{
	return o.bool_value();
}
inline bool numberIsInt( const pdfp::Object &o )
{
	return o.is_int();
}
inline int intValue( const pdfp::Object &o )
{
	return o.int_value();
}
inline float floatValue( const pdfp::Object &o )
{
	return o.real_value();
}
inline std::string nameValue( const pdfp::Object &o )
{
	return o.name_value();
}
inline std::string stringValue( const pdfp::Object &o )
{
	return o.string_value();
}
inline pdfp::Object asArray( const pdfp::Object &o )
{
	return o;
}
inline pdfp::Object asDictionary( const pdfp::Object &o )
{
	return o;
}
inline pdfp::Object asStream( const pdfp::Object &o )
{
	return o;
}
inline size_t arrayCount( const pdfp::Object &o )
{
	return o.array_size();
}
inline pdfp::Object arrayValue( const pdfp::Object &o, int i )
{
	return o[i];
}
inline std::vector<std::string> keys( const pdfp::Object &o )
{
	std::vector<std::string> ks;
	for ( auto &it : o.dictionary_items() )
		ks.push_back( it.first );
	return ks;
}
inline pdfp::Object dictValue( const pdfp::Object &o, const std::string &k )
{
	return o[k];
}
inline pdfp::Object streamDictionary( const pdfp::Object &o )
{
	return o.stream_dictionary();
}
inline pdfp::Data streamData( const pdfp::Object &o )
{
	return o.stream_data()->readAll();
}

uintptr_t hashKey( const pdfp::Object &o )
{
	return *((uintptr_t*)&o);
}

#endif
