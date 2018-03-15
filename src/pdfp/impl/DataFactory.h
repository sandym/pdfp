/*
 *  DataFactory.h
 *  pdfp
 *
 *  Created by Sandy Martel on 09-02-16.
 *  Copyright 2009 by Sandy Martel. All rights reserved.
 *
 */

#ifndef H_PDFP_DATAFACTORY
#define H_PDFP_DATAFACTORY

#include "pdfp/PDFData.h"

namespace pdfp {

class Object;

DataStreamRef createDataStream( const Object &i_dict,
                                size_t i_offset,
                                size_t i_length,
                                int i_id,
                                int i_gen );

DataStreamRef createDataStream( const Object &i_dict,
                                const char *i_ptr,
                                size_t i_length );
}

#endif
