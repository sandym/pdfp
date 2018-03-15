/*
 *  CCITTFax.cpp
 *  pdfp
 *
 *  Created by Sandy Martel on 09-03-04.
 *  Copyright 2009 by Sandy Martel. All rights reserved.
 *
 */

#include "CCITTFax.h"
#include <cassert>
#include <fstream>
#include <iomanip>

namespace {

const int EOL = -2;

struct CCITTCode
{
	int _runLength;
	int _codeLength;
	uint32_t _code;
};

//	terminating codes (white)
CCITTCode kWhiteTerminatingCode[] = {{2, /*      0111 */ 4, 0x0007},
                                     {3, /*      1000 */ 4, 0x0008},
                                     {4, /*      1011 */ 4, 0x000B},
                                     {5, /*      1100 */ 4, 0x000C},
                                     {6, /*      1110 */ 4, 0x000E},
                                     {7, /*      1111 */ 4, 0x000F},
                                     {8, /*    1 0011 */ 5, 0x0013},
                                     {9, /*    1 0100 */ 5, 0x0014},
                                     {10, /*    0 0111 */ 5, 0x0007},
                                     {11, /*    0 1000 */ 5, 0x0008},
                                     {1, /*   00 0111 */ 6, 0x0007},
                                     {12, /*   00 1000 */ 6, 0x0008},
                                     {13, /*   00 0011 */ 6, 0x0003},
                                     {14, /*   11 0100 */ 6, 0x0034},
                                     {15, /*   11 0101 */ 6, 0x0035},
                                     {16, /*   10 1010 */ 6, 0x002A},
                                     {17, /*   10 1011 */ 6, 0x002B},
                                     {18, /*  010 0111 */ 7, 0x0027},
                                     {19, /*  000 1100 */ 7, 0x000C},
                                     {20, /*  000 1000 */ 7, 0x0008},
                                     {21, /*  001 0111 */ 7, 0x0017},
                                     {22, /*  000 0011 */ 7, 0x0003},
                                     {23, /*  000 0100 */ 7, 0x0004},
                                     {24, /*  010 1000 */ 7, 0x0028},
                                     {25, /*  010 1011 */ 7, 0x002B},
                                     {26, /*  001 0011 */ 7, 0x0013},
                                     {27, /*  010 0100 */ 7, 0x0024},
                                     {28, /*  001 1000 */ 7, 0x0018},
                                     {0, /* 0011 0101 */ 8, 0x0035},
                                     {29, /* 0000 0010 */ 8, 0x0002},
                                     {30, /* 0000 0011 */ 8, 0x0003},
                                     {31, /* 0001 1010 */ 8, 0x001A},
                                     {32, /* 0001 1011 */ 8, 0x001B},
                                     {33, /* 0001 0010 */ 8, 0x0012},
                                     {34, /* 0001 0011 */ 8, 0x0013},
                                     {35, /* 0001 0100 */ 8, 0x0014},
                                     {36, /* 0001 0101 */ 8, 0x0015},
                                     {37, /* 0001 0110 */ 8, 0x0016},
                                     {38, /* 0001 0111 */ 8, 0x0017},
                                     {39, /* 0010 1000 */ 8, 0x0028},
                                     {40, /* 0010 1001 */ 8, 0x0029},
                                     {41, /* 0010 1010 */ 8, 0x002A},
                                     {42, /* 0010 1011 */ 8, 0x002B},
                                     {43, /* 0010 1100 */ 8, 0x002C},
                                     {44, /* 0010 1101 */ 8, 0x002D},
                                     {45, /* 0000 0100 */ 8, 0x0004},
                                     {46, /* 0000 0101 */ 8, 0x0005},
                                     {47, /* 0000 1010 */ 8, 0x000A},
                                     {48, /* 0000 1011 */ 8, 0x000B},
                                     {49, /* 0101 0010 */ 8, 0x0052},
                                     {50, /* 0101 0011 */ 8, 0x0053},
                                     {51, /* 0101 0100 */ 8, 0x0054},
                                     {52, /* 0101 0101 */ 8, 0x0055},
                                     {53, /* 0010 0100 */ 8, 0x0024},
                                     {54, /* 0010 0101 */ 8, 0x0025},
                                     {55, /* 0101 1000 */ 8, 0x0058},
                                     {56, /* 0101 1001 */ 8, 0x0059},
                                     {57, /* 0101 1010 */ 8, 0x005A},
                                     {58, /* 0101 1011 */ 8, 0x005B},
                                     {59, /* 0100 1010 */ 8, 0x004A},
                                     {60, /* 0100 1011 */ 8, 0x004B},
                                     {61, /* 0011 0010 */ 8, 0x0032},
                                     {62, /* 0011 0011 */ 8, 0x0033},
                                     {63, /* 0011 0100 */ 8, 0x0034},
                                     {0, 0, 0}};

//	terminating codes (black)
CCITTCode kBlackTerminatingCode[] = {{2, /*             11 */ 2, 0x0003},
                                     {3, /*             10 */ 2, 0x0002},
                                     {4, /*            011 */ 3, 0x0003},
                                     {1, /*            010 */ 3, 0x0002},
                                     {5, /*           0011 */ 4, 0x0003},
                                     {6, /*           0010 */ 4, 0x0002},
                                     {7, /*         0 0011 */ 5, 0x0003},
                                     {8, /*        00 0101 */ 6, 0x0005},
                                     {9, /*        00 0100 */ 6, 0x0004},
                                     {10, /*       000 0100 */ 7, 0x0004},
                                     {11, /*       000 0101 */ 7, 0x0005},
                                     {12, /*       000 0111 */ 7, 0x0007},
                                     {13, /*      0000 0100 */ 8, 0x0004},
                                     {14, /*      0000 0111 */ 8, 0x0007},
                                     {15, /*    0 0001 1000 */ 9, 0x0018},
                                     {0, /*   00 0011 0111 */ 10, 0x0037},
                                     {16, /*   00 0001 0111 */ 10, 0x0017},
                                     {17, /*   00 0001 1000 */ 10, 0x0018},
                                     {18, /*   00 0000 1000 */ 10, 0x0008},
                                     {19, /*  000 0110 0111 */ 11, 0x0067},
                                     {20, /*  000 0110 1000 */ 11, 0x0068},
                                     {21, /*  000 0110 1100 */ 11, 0x006C},
                                     {22, /*  000 0011 0111 */ 11, 0x0037},
                                     {23, /*  000 0010 1000 */ 11, 0x0028},
                                     {24, /*  000 0001 0111 */ 11, 0x0017},
                                     {25, /*  000 0001 1000 */ 11, 0x0018},
                                     {26, /* 0000 1100 1010 */ 12, 0x00CA},
                                     {27, /* 0000 1100 1011 */ 12, 0x00CB},
                                     {28, /* 0000 1100 1100 */ 12, 0x00CC},
                                     {29, /* 0000 1100 1101 */ 12, 0x00CD},
                                     {30, /* 0000 0110 1000 */ 12, 0x0068},
                                     {31, /* 0000 0110 1001 */ 12, 0x0069},
                                     {32, /* 0000 0110 1010 */ 12, 0x006A},
                                     {33, /* 0000 0110 1011 */ 12, 0x006B},
                                     {34, /* 0000 1101 0010 */ 12, 0x00D2},
                                     {35, /* 0000 1101 0011 */ 12, 0x00D3},
                                     {36, /* 0000 1101 0100 */ 12, 0x00D4},
                                     {37, /* 0000 1101 0101 */ 12, 0x00D5},
                                     {38, /* 0000 1101 0110 */ 12, 0x00D6},
                                     {39, /* 0000 1101 0111 */ 12, 0x00D7},
                                     {40, /* 0000 0110 1100 */ 12, 0x006C},
                                     {41, /* 0000 0110 1101 */ 12, 0x006D},
                                     {42, /* 0000 1101 1010 */ 12, 0x00DA},
                                     {43, /* 0000 1101 1011 */ 12, 0x00DB},
                                     {44, /* 0000 0101 0100 */ 12, 0x0054},
                                     {45, /* 0000 0101 0101 */ 12, 0x0055},
                                     {46, /* 0000 0101 0110 */ 12, 0x0056},
                                     {47, /* 0000 0101 0111 */ 12, 0x0057},
                                     {48, /* 0000 0110 0100 */ 12, 0x0064},
                                     {49, /* 0000 0110 0101 */ 12, 0x0065},
                                     {50, /* 0000 0101 0010 */ 12, 0x0052},
                                     {51, /* 0000 0101 0011 */ 12, 0x0053},
                                     {52, /* 0000 0010 0100 */ 12, 0x0024},
                                     {53, /* 0000 0011 0111 */ 12, 0x0037},
                                     {54, /* 0000 0011 1000 */ 12, 0x0038},
                                     {55, /* 0000 0010 0111 */ 12, 0x0027},
                                     {56, /* 0000 0010 1000 */ 12, 0x0028},
                                     {57, /* 0000 0101 1000 */ 12, 0x0058},
                                     {58, /* 0000 0101 1001 */ 12, 0x0059},
                                     {59, /* 0000 0010 1011 */ 12, 0x002B},
                                     {60, /* 0000 0010 1100 */ 12, 0x002C},
                                     {61, /* 0000 0101 1010 */ 12, 0x005A},
                                     {62, /* 0000 0110 0110 */ 12, 0x0066},
                                     {63, /* 0000 0110 0111 */ 12, 0x0067},
                                     {0, 0, 0}};

//	make-up codes (white)
CCITTCode kWhiteMakeUpCode[] = {{64, /*         1 1011 */ 5, 0x001B},
                                {128, /*         1 0010 */ 5, 0x0012},
                                {192, /*        01 0111 */ 6, 0x0017},
                                {1664, /*        01 1000 */ 6, 0x0018},
                                {256, /*       011 0111 */ 7, 0x0037},
                                {320, /*      0011 0110 */ 8, 0x0036},
                                {384, /*      0011 0111 */ 8, 0x0037},
                                {448, /*      0110 0100 */ 8, 0x0064},
                                {512, /*      0110 0101 */ 8, 0x0065},
                                {576, /*      0110 1000 */ 8, 0x0068},
                                {640, /*      0110 0111 */ 8, 0x0067},
                                {704, /*    0 1100 1100 */ 9, 0x00CC},
                                {768, /*    0 1100 1101 */ 9, 0x00CD},
                                {832, /*    0 1101 0010 */ 9, 0x00D2},
                                {896, /*    0 1101 0011 */ 9, 0x00D3},
                                {960, /*    0 1101 0100 */ 9, 0x00D4},
                                {1024, /*    0 1101 0101 */ 9, 0x00D5},
                                {1088, /*    0 1101 0110 */ 9, 0x00D6},
                                {1152, /*    0 1101 0111 */ 9, 0x00D7},
                                {1216, /*    0 1101 1000 */ 9, 0x00D8},
                                {1280, /*    0 1101 1001 */ 9, 0x00D9},
                                {1344, /*    0 1101 1010 */ 9, 0x00DA},
                                {1408, /*    0 1101 1011 */ 9, 0x00DB},
                                {1472, /*    0 1001 1000 */ 9, 0x0098},
                                {1536, /*    0 1001 1001 */ 9, 0x0099},
                                {1600, /*    0 1001 1010 */ 9, 0x009A},
                                {1728, /*    0 1001 1011 */ 9, 0x009B},
                                {1792, /*  000 0000 1000 */ 11, 0x0008},
                                {1856, /*  000 0000 1100 */ 11, 0x000C},
                                {1920, /*  000 0000 1101 */ 11, 0x000D},
                                {1984, /* 0000 0001 0010 */ 12, 0x0012},
                                {2048, /* 0000 0001 0011 */ 12, 0x0013},
                                {2112, /* 0000 0001 0100 */ 12, 0x0014},
                                {2176, /* 0000 0001 0101 */ 12, 0x0015},
                                {2240, /* 0000 0001 0110 */ 12, 0x0016},
                                {2304, /* 0000 0001 0111 */ 12, 0x0017},
                                {2368, /* 0000 0001 1100 */ 12, 0x001C},
                                {2432, /* 0000 0001 1101 */ 12, 0x001D},
                                {2496, /* 0000 0001 1110 */ 12, 0x001E},
                                {2560, /* 0000 0001 1111 */ 12, 0x001F},
                                {EOL, /* 0000 0000 0001 */ 12, 0x0001},
                                {0, 0, 0}};

//	{ make-up codes (black)
CCITTCode kBlackMakeUpCode[] = {{64, /*     00 0000 1111 */ 10, 0x000F},
                                {1792, /*    000 0000 1000 */ 11, 0x0008},
                                {1856, /*    000 0000 1100 */ 11, 0x000C},
                                {1920, /*    000 0000 1101 */ 11, 0x000D},
                                {128, /*   0000 1100 1000 */ 12, 0x00C8},
                                {192, /*   0000 1100 1001 */ 12, 0x00C9},
                                {256, /*   0000 0101 1011 */ 12, 0x005B},
                                {320, /*   0000 0011 0011 */ 12, 0x0033},
                                {384, /*   0000 0011 0100 */ 12, 0x0034},
                                {448, /*   0000 0011 0101 */ 12, 0x0035},
                                {1984, /*   0000 0001 0010 */ 12, 0x0012},
                                {2048, /*   0000 0001 0011 */ 12, 0x0013},
                                {2112, /*   0000 0001 0100 */ 12, 0x0014},
                                {2176, /*   0000 0001 0101 */ 12, 0X0015},
                                {2240, /*   0000 0001 0110 */ 12, 0x0016},
                                {2304, /*   0000 0001 0111 */ 12, 0x0017},
                                {2368, /*   0000 0001 1100 */ 12, 0x001C},
                                {2432, /*   0000 0001 1101 */ 12, 0x001D},
                                {2496, /*   0000 0001 1110 */ 12, 0x001E},
                                {2560, /*   0000 0001 1111 */ 12, 0x001F},
                                {512, /* 0 0000 0110 1100 */ 13, 0x006C},
                                {576, /* 0 0000 0110 1101 */ 13, 0x006D},
                                {640, /* 0 0000 0100 1010 */ 13, 0x004A},
                                {704, /* 0 0000 0100 1011 */ 13, 0x004B},
                                {768, /* 0 0000 0100 1100 */ 13, 0x004C},
                                {832, /* 0 0000 0100 1101 */ 13, 0x004D},
                                {896, /* 0 0000 0111 0010 */ 13, 0x0072},
                                {960, /* 0 0000 0111 0011 */ 13, 0x0073},
                                {1024, /* 0 0000 0111 0100 */ 13, 0x0074},
                                {1088, /* 0 0000 0111 0101 */ 13, 0x0075},
                                {1152, /* 0 0000 0111 0110 */ 13, 0x0076},
                                {1216, /* 0 0000 0111 0111 */ 13, 0x0077},
                                {1280, /* 0 0000 0101 0010 */ 13, 0x0052},
                                {1344, /* 0 0000 0101 0011 */ 13, 0x0053},
                                {1408, /* 0 0000 0101 0100 */ 13, 0x0054},
                                {1472, /* 0 0000 0101 0101 */ 13, 0x0055},
                                {1536, /* 0 0000 0101 1010 */ 13, 0x005A},
                                {1600, /* 0 0000 0101 1011 */ 13, 0x005B},
                                {1664, /* 0 0000 0110 0100 */ 13, 0x0064},
                                {1728, /* 0 0000 0110 0101 */ 13, 0x0065},
                                {EOL, /*    000 0000 0000 */ 11, 0x0000},
                                {0, 0, 0}};

enum
{
	kP = 1,
	kH,
	kV0,
	kVR1,
	kVR2,
	kVR3,
	kVL1,
	kVL2,
	kVL3
};

//	{ make-up codes (black)
CCITTCode k2dCode[] = {{kV0, /*                             1 */ 1, 0x0001},
                       {kH, /*                           001 */ 3, 0x000001},
                       {kVL1, /*                           010 */ 3, 0x000002},
                       {kVR1, /*                           011 */ 3, 0x000003},
                       {kP, /*                          0001 */ 4, 0x000001},
                       {kVL2, /*                       00 0010 */ 6, 0x000002},
                       {kVR2, /*                       00 0011 */ 6, 0x000003},
                       {kVL3, /*                      000 0010 */ 7, 0x000002},
                       {kVR3, /*                      000 0011 */ 7, 0x000003},
                       {EOF, /* 0000 0000 0001 0000 0000 0001 */ 24, 0x001001},
                       {0, 0, 0}};

bool find2DCode( uint32_t code, int codeSize, int &result )
{
	const CCITTCode *ptr = k2dCode;
	for ( ; ptr->_codeLength != 0; ++ptr )
	{
		if ( ptr->_codeLength == codeSize and ptr->_code == code )
		{
			result = ptr->_runLength;
			return true;
		}
	}
	return false;
}

bool findWhiteCode( uint32_t code, int codeSize, int &result )
{
	const CCITTCode *ptr = kWhiteTerminatingCode;
	for ( ; ptr->_codeLength != 0; ++ptr )
	{
		if ( ptr->_codeLength == codeSize and ptr->_code == code )
		{
			result = ptr->_runLength;
			return true;
		}
	}
	ptr = kWhiteMakeUpCode;
	for ( ; ptr->_codeLength != 0; ++ptr )
	{
		if ( ptr->_codeLength == codeSize and ptr->_code == code )
		{
			result = ptr->_runLength;
			return true;
		}
	}
	return false;
}

bool findBlackCode( uint32_t code, int codeSize, int &result )
{
	const CCITTCode *ptr = kBlackTerminatingCode;
	for ( ; ptr->_codeLength != 0; ++ptr )
	{
		if ( ptr->_codeLength == codeSize and ptr->_code == code )
		{
			result = ptr->_runLength;
			return true;
		}
	}
	ptr = kBlackMakeUpCode;
	for ( ; ptr->_codeLength != 0; ++ptr )
	{
		if ( ptr->_codeLength == codeSize and ptr->_code == code )
		{
			result = ptr->_runLength;
			return true;
		}
	}
	return false;
}

void setBits( std::vector<uint8_t> &i_row, int i_start, int i_span )
{
	if ( i_span == 0 )
		return;
	int bitToSet;
	uint8_t mask;
	int byte = i_start / 8;
	int bit = i_start % 8;
	if ( bit != 0 )
	{
		bitToSet = 8 - bit;
		mask = 0xFF >> bit;
		if ( i_span < bitToSet )
			mask &= ~( ( 1 << ( bitToSet - i_span ) ) - 1 );
		i_row[byte] |= mask;
		++byte;
		i_span -= bitToSet;
	}
	int lastFullByte = byte + i_span / 8;
	i_span -= ( lastFullByte - byte ) * 8;
	while ( byte < lastFullByte )
		i_row[byte++] = 0xFF;
	if ( i_span > 0 )
	{
		bitToSet = 8 - i_span;
		mask = ( 1 << bitToSet ) - 1;
		i_row[byte] |= ~mask;
	}
	return;
}

#if 0
#	define DUMP_REF_CODE_LINE
std::ofstream	g_str;
int	g_line = 0;
#endif
}

// MARK: -

namespace pdfp {

CCITTFaxDecode::CCITTFaxDecode( int i_K,
                                        bool i_EndOfLine,
                                        bool i_EncodedByteAlign,
                                        int i_Columns,
                                        int i_Rows,
                                        bool i_EndOfBlock,
                                        bool i_BlackIs1,
                                        int i_DamagedRowsBeforeError ) :
    _K( i_K ),
    _EndOfLine( i_EndOfLine ),
    _EncodedByteAlign( i_EncodedByteAlign ),
    _Columns( i_Columns ),
    _Rows( i_Rows ),
    _EndOfBlock( i_EndOfBlock ),
    _BlackIs1( i_BlackIs1 ),
    _DamagedRowsBeforeError( i_DamagedRowsBeforeError ),
    _nextRowIs1D( i_K >= 0 ),
    _rowCounter( 0 )
{
	int bytesPerRow( ( _Columns + 7 ) / 8 );
	_currentRow.resize( bytesPerRow, 0 );
	_codingLine.push_back( _Columns );

#ifdef DUMP_REF_CODE_LINE
	g_str.open( "/Users/sandy/pdfp.out" );
	g_line = 0;
#endif
}

CCITTFaxDecode::~CCITTFaxDecode()
{
#ifdef DUMP_REF_CODE_LINE
	g_str.close();
#endif
}

void CCITTFaxDecode::rewind()
{
	rewindNext();
	_currentBytes = 0;
	_goodBits = 0;
	_pos = std::numeric_limits<size_t>::max();
	_nextRowIs1D = ( _K >= 0 );
	_rowCounter = 0;
	int bytesPerRow( ( _Columns + 7 ) / 8 );
	_currentRow.clear();
	_currentRow.resize( bytesPerRow, 0 );
	_codingLine.clear();
	_codingLine.push_back( _Columns );
}

std::streamoff CCITTFaxDecode::read( su::array_view<uint8_t> o_buffer )
{
	size_t s = 0;
	while ( s < o_buffer.size() )
	{
		if ( _pos < _currentRow.size() )
		{
			size_t l = std::min( _currentRow.size() - _pos,
			                     size_t( o_buffer.size() - s ) );
			std::copy( _currentRow.begin() + _pos,
			           _currentRow.begin() + _pos + l,
			           o_buffer.begin() + s );
			s += l;
			_pos += l;
		}
		else if ( not fillNextRow() )
			break;
	}
	return s == 0 ? EOF : s;
}

void CCITTFaxDecode::consumeCode( int i_codeLength, uint32_t i_code )
{
	if ( _goodBits >= i_codeLength )
	{
		int inv = 32 - i_codeLength;
		i_code <<= inv;
		uint32_t mask = 0xFFFFFFFF << inv;
		if ( i_code == ( _currentBytes % mask ) )
		{
			_goodBits -= i_codeLength;
			_currentBytes <<= i_codeLength;
		}
	}
}

bool CCITTFaxDecode::read1dSpan( bool i_wantWhite, int &o_span )
{
	int result;
	o_span = 0;
	if ( i_wantWhite )
	{
		do
		{
			int bit = readBit();
			uint32_t code = bit;
			bit = readBit();
			code = ( code << 1 ) | bit;
			bit = readBit();
			code = ( code << 1 ) | bit;
			bit = readBit();
			if ( bit == EOF )
				return false;
			code = ( code << 1 ) | bit;

			int codeSize = 4;
			while ( not findWhiteCode( code, codeSize, result ) )
			{
				bit = readBit();
				if ( bit == EOF )
					return false;
				code = ( code << 1 ) | bit;
				++codeSize;
			}
			if ( result == EOL )
				return true;
			o_span += result;
		} while ( result > 63 );
	}
	else
	{
		do
		{
			int bit = readBit();
			uint32_t code = bit;
			bit = readBit();
			if ( bit == EOF )
				return false;
			code = ( code << 1 ) | bit;

			int codeSize = 2;
			while ( not findBlackCode( code, codeSize, result ) )
			{
				bit = readBit();
				if ( bit == EOF )
					return false;
				code = ( code << 1 ) | bit;
				++codeSize;
			}
			if ( result == EOL )
				return true;
			o_span += result;
		} while ( result > 63 );
	}
	return true;
}

bool CCITTFaxDecode::fillNextRow()
{
	bool wantWhite = true;
	int col = 0;

	_refLine.swap( _codingLine );
	_refLine.push_back( _Columns );
	_codingLine.clear();

	if ( _Rows != 0 and _rowCounter >= _Rows )
		return false;

	if ( _nextRowIs1D )
	{
		// Group 3 1D decoding
		do
		{
			int span;
			if ( not read1dSpan( wantWhite, span ) )
				return false;
			if ( span )
			{
				//				std::cout << span << " ";
				col += span;
				_codingLine.push_back( col );
				wantWhite = not wantWhite;
			}
		} while ( col < _Columns );
		//		std::cout << std::endl;

		// consume the end-of-line code if present
		const CCITTCode &endOfLine =
		    wantWhite ? kWhiteMakeUpCode[40] : kBlackMakeUpCode[40];
		if ( _goodBits <= endOfLine._codeLength )
			fillup();
		consumeCode( endOfLine._codeLength, endOfLine._code );
	}
	else
	{
#ifdef DUMP_REF_CODE_LINE
		{
			g_str << g_line << "-ref :";
			for ( auto it : _refLine )
			{
				if ( it < _Columns )
					g_str << it << " ";
			}
			g_str << std::endl;
		}
#endif

		// Group 4 2D decoding
		auto b1 = _refLine.begin();
		while ( col < _Columns )
		{
			uint32_t code = 0;
			int codeSize = 0;
			int bit, result = 0;
			do
			{
				bit = readBit();
				if ( bit == EOF )
					return false;
				code = ( code << 1 ) | bit;
				++codeSize;
				if ( find2DCode( code, codeSize, result ) )
					break;
			} while ( codeSize < 24 );
			switch ( result )
			{
				case kP:
					assert( b1 != _refLine.end() );
					if ( *( b1 + 1 ) < _Columns )
					{
						col = *( b1 + 1 );
						b1 += 2;
					}
					break;
				case kH:
				{
					int span1, span2;
					if ( not read1dSpan( wantWhite, span1 ) )
						return false;
					if ( not read1dSpan( not wantWhite, span2 ) )
						return false;
					col += span1;
					_codingLine.push_back( col );
					col += span2;
					_codingLine.push_back( col );
					while ( *b1 <= col and *b1 < _Columns )
						b1 += 2;
					break;
				}
				case kV0:
					assert( b1 != _refLine.end() );
					col = *b1;
					_codingLine.push_back( col );
					wantWhite = not wantWhite;
					if ( col < _Columns )
					{
						++b1;
						while ( *b1 <= col and *b1 < _Columns )
							b1 += 2;
					}
					break;
				case kVR1:
					assert( b1 != _refLine.end() );
					col = *b1 + 1;
					_codingLine.push_back( col );
					wantWhite = not wantWhite;
					if ( col < _Columns )
					{
						++b1;
						while ( *b1 <= col and *b1 < _Columns )
							b1 += 2;
					}
					break;
				case kVR2:
					assert( b1 != _refLine.end() );
					col = *b1 + 2;
					_codingLine.push_back( col );
					wantWhite = not wantWhite;
					if ( col < _Columns )
					{
						++b1;
						while ( *b1 <= col and *b1 < _Columns )
							b1 += 2;
					}
					break;
				case kVR3:
					assert( b1 != _refLine.end() );
					col = *b1 + 3;
					_codingLine.push_back( col );
					wantWhite = not wantWhite;
					if ( col < _Columns )
					{
						++b1;
						while ( *b1 <= col and *b1 < _Columns )
							b1 += 2;
					}
					break;
				case kVL1:
					assert( b1 != _refLine.end() );
					col = *b1 - 1;
					_codingLine.push_back( col );
					wantWhite = not wantWhite;
					if ( col < _Columns )
					{
						if ( b1 != _refLine.begin() )
							--b1;
						else
							++b1;
						while ( *b1 <= col and *b1 < _Columns )
							b1 += 2;
					}
					break;
				case kVL2:
					assert( b1 != _refLine.end() );
					col = *b1 - 2;
					_codingLine.push_back( col );
					wantWhite = not wantWhite;
					if ( col < _Columns )
					{
						if ( b1 != _refLine.begin() )
							--b1;
						else
							++b1;
						while ( *b1 <= col and *b1 < _Columns )
							b1 += 2;
					}
					break;
				case kVL3:
					assert( b1 != _refLine.end() );
					col = *b1 - 3;
					_codingLine.push_back( col );
					wantWhite = not wantWhite;
					if ( col < _Columns )
					{
						if ( b1 != _refLine.begin() )
							--b1;
						else
							++b1;
						while ( *b1 <= col and *b1 < _Columns )
							b1 += 2;
					}
					break;
				case EOF:
					col = _Columns;
					_codingLine.push_back( col );
					break;
				default:
					assert( false );
					return false;
					break;
			}
		}

#ifdef DUMP_REF_CODE_LINE
		{
			g_str << g_line << "-code:";
			for ( auto it : _codingLine )
			{
				if ( it < _Columns )
					g_str << it << " ";
			}
			g_str << std::endl;
			g_str.flush();
			++g_line;
		}
#endif
	}
	++_rowCounter;
	if ( _K > 0 )
		_nextRowIs1D = ( _rowCounter % _K ) == 0;
	if ( _EncodedByteAlign )
		byteAlign();

	//	fill _currentRow's bits with the span list
	std::fill( _currentRow.begin(), _currentRow.end(), 0 );
	if ( _codingLine.size() > 1 )
	{
		auto it = _codingLine.begin();
		int prev = 0;
		while ( it != _codingLine.end() )
		{
			// white
			prev = *it;
			++it;
			if ( it != _codingLine.end() )
			{
				setBits( _currentRow, prev, *it - prev );
				++it;
			}
		}
	}

	if ( not _BlackIs1 )
	{
		//	invert
		int remainingBits = _Columns % 8;
		std::vector<uint8_t>::iterator last;
		if ( remainingBits != 0 )
		{
			last = _currentRow.end() - 1;
			*last = ( ~( *last ) ) & ~( 0xFF >> remainingBits );
		}
		else
			last = _currentRow.end();

		for ( auto it = _currentRow.begin(); it != last; ++it )
			*it = ~( *it );
	}

	_pos = 0;
	return true;
}

void CCITTFaxDecode::byteAlign()
{
	int waste = _goodBits % 8;
	if ( waste != 0 )
	{
		_goodBits -= waste;
		_currentBytes <<= waste;
	}
}

void CCITTFaxDecode::fillup()
{
	while ( ( ( 32 - _goodBits ) / 8 ) != 0 )
	{
		int c = getByteNext();
		if ( c == EOF )
			return;
		_currentBytes = _currentBytes | ( c << ( 24 - _goodBits ) );
		_goodBits += 8;
	}
}
}
