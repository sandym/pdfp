//
//  XrefTable.h
//  pdfp
//
//  Created by Sandy Martel on 2013/01/16.
//
//

#ifndef H_PDFP_XrefTable
#define H_PDFP_XrefTable

#include <memory>
#include "pdfp/PDFObject.h"

namespace pdfp {

class XRef
{
public:
	XRef( int i_gen = -1, int i_pos = -1 ) :
	    _generationOrStreamID( i_gen ),
	    _posOrIndex( i_pos )
	{
	}
	XRef( int i_streamId, int i_indexInStream, bool i_compressed ) :
	    _compressed( i_compressed ),
	    _generationOrStreamID( i_streamId ),
	    _posOrIndex( i_indexInStream )
	{
	}
	XRef( const Object &i_obj ) :
	    _object( i_obj )
	{
	}

	int generation() const { return _generationOrStreamID; }
	int pos() const { return _posOrIndex; }

	bool compressed() const { return _compressed; }

	int streamId() const { return _generationOrStreamID; }
	int indexInStream() const { return _posOrIndex; }

	void setObject( const Object &i_obj )
	{
		_object = i_obj;
	}
	const Object &object() const { return _object; }

private:
	bool _compressed = false;
	int _generationOrStreamID = 1; //!< stream ID if _compressed
	int _posOrIndex = -1; //!< index if _compressed

	Object _object;
};
typedef std::vector<XRef> XRefVector_t;

class XrefTable
{
public:
	XrefTable() = default;
	~XrefTable() = default;

	void clear();

	size_t size() const { return _table.size(); }

	void expand( size_t i_size );

	const Object &registerObject( int i_ref, const Object &i_obj ) const;
	void addNewObject( const Object &i_obj ) const;

	const Object &resolveRef( int i_ref, int i_gen ) const;
	bool refForObject( const Object &i_obj, int &o_ref, int &o_gen ) const;

	XRefVector_t::const_iterator begin() const { return _table.begin(); }
	XRefVector_t::const_iterator end() const { return _table.end(); }

	const XRef &operator[]( size_t i ) const
	{
		return _table.operator[]( i );
	}
	XRef &operator[]( size_t i ) { return _table.operator[]( i ); }

private:
	mutable XRefVector_t _table;
};
}

#endif
