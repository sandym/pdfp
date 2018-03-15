//
//  XrefTable.cpp
//  pdfp
//
//  Created by Sandy Martel on 2013/01/16.
//
//

#include "XrefTable.h"
#include <cassert>

namespace pdfp {

void XrefTable::clear()
{
	_table.clear();
}

void XrefTable::expand( size_t i_size )
{
	if ( _table.size() < i_size )
		_table.resize( i_size );
}

const Object &XrefTable::registerObject( int i_ref, const Object &i_obj ) const
{
	if ( i_ref < _table.size() )
	{
		_table[i_ref].setObject( i_obj );
		return _table[i_ref].object();
	}
	return kObjectNull;
}

void XrefTable::addNewObject( const Object &i_obj ) const
{
	_table.emplace_back( i_obj );
}

const Object &XrefTable::resolveRef( int i_ref, int i_gen ) const
{
	if ( i_ref >= _table.size() )
		return kObjectNull;
	if ( _table[i_ref].compressed() or _table[i_ref].generation() == i_gen )
		return _table[i_ref].object();
	else
		return kObjectNull;
}

bool XrefTable::refForObject( const Object &i_obj,
                                  int &o_ref,
                                  int &o_gen ) const
{
	o_ref = 0;
	for ( auto &it : _table )
	{
		if ( it.object() == i_obj )
		{
			o_gen = it.generation();
			return true;
		}
		++o_ref;
	}
	return false;
}
}
