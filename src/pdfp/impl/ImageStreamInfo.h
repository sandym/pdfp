//
//  ImageStreamInfo.h
//  pdfp
//
//  Created by Sandy Martel on 2013/01/16.
//
//

#ifndef H_PDFP_ImageStreamInfo
#define H_PDFP_ImageStreamInfo

namespace pdfp {

class Object;

int getNbOfColourComponents( const Object &i_colourSpace );

bool getImageInfo( const Object &i_dict,
                   bool isBitmap,
                   int &o_width,
                   int &o_height,
                   int &o_bpc,
                   int &o_nbOfComp );
}

#endif
