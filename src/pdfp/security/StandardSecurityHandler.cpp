//
//  StandardSecurityHandler.cpp
//  pdfp
//
//  Created by Sandy Martel on 11/10/03.
//  Copyright 2011年 Sandy Martel. All rights reserved.
//

#include "StandardSecurityHandler.h"
#include "pdfp/PDFDocument.h"
#include "pdfp/PDFObject.h"
#include "su/base/endian.h"
#include "su/log/logger.h"
#include <cassert>

#include "pdfp/crypto/md5.h"

namespace {
const uint8_t kPaddingString[] = {
    0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41, 0x64, 0x00, 0x4E,
    0x56, 0xFF, 0xFA, 0x01, 0x08, 0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68,
    0x3E, 0x80, 0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A};

// algorithm 1 from the PDF specs
std::tuple<std::array<uint8_t, 16>, int> Algorithm1(
    bool i_useAES,
    const std::array<uint8_t, 16> &i_encryptionKey,
    int i_length,
    int i_id,
    int i_gen )
{
	uint8_t extendKey[16 + 5 + 4];
	int l = i_length / 8;
	int extendKeyLength = l + 5;
	memcpy( extendKey, i_encryptionKey.data(), l );
	extendKey[l + 0] = i_id & 0xFF;
	extendKey[l + 1] = ( i_id >> 8 ) & 0xFF;
	extendKey[l + 2] = ( i_id >> 16 ) & 0xFF;
	extendKey[l + 3] = i_gen & 0xFF;
	extendKey[l + 4] = ( i_gen >> 8 ) & 0xFF;

	if ( i_useAES )
	{
		extendKey[l + 5] = 0x73; // s
		extendKey[l + 6] = 0x41; // A
		extendKey[l + 7] = 0x6C; // l
		extendKey[l + 8] = 0x54; // T
		extendKeyLength += 4;
	}

	pdfp::MD5_CTX c;
	pdfp::MD5_Init( &c );
	pdfp::MD5_Update( &c, extendKey, extendKeyLength );
	std::array<uint8_t, 16> key;
	pdfp::MD5_Final( key.data(), &c );

	return {key, std::min( 16, l + 5 )};
}
}

namespace pdfp {

StandardSecurityCrypter::StandardSecurityCrypter(
    const std::array<uint8_t, 16> &i_encryptionKey,
    int i_length,
    int i_id,
    int i_gen )
{
	std::tie( _key, _keyLength ) =
	    Algorithm1( false, i_encryptionKey, i_length, i_id, i_gen );
	reset();
}

void StandardSecurityCrypter::reset()
{
	RC4_set_key( &_rc4Key, _keyLength, _key.data() );
}

void StandardSecurityCrypter::rewind()
{
	rewindNext();
	reset();
}

std::streamoff StandardSecurityCrypter::read( su::array_view<uint8_t> o_buffer )
{
	ssize_t p = 0;
	while ( p < o_buffer.size() )
	{
		uint8_t buffer[4096];
		auto toRead = std::min<size_t>( 4096, o_buffer.size() - p );
		auto l = readNext( {buffer, toRead} );
		if ( l <= 0 )
			break;
		RC4( &_rc4Key, l, buffer, o_buffer.data() + p );
		p += l;
	}
	return p == 0 ? EOF : p;
}

std::string StandardSecurityCrypter::decryptString( const std::string &i_input )
{
	std::string output;
	output.resize( i_input.length() );
	RC4( &_rc4Key,
	     i_input.length(),
	     (uint8_t *)i_input.data(),
	     (uint8_t *)output.data() );
	return output;
}

StandardAESSecurityCrypter::StandardAESSecurityCrypter(
    const std::array<uint8_t, 16> &i_encryptionKey,
    int i_length,
    int i_id,
    int i_gen ) :
    _aesPos( AES_BLOCK_SIZE )
{
	std::tie( _key, _keyLength ) =
	    Algorithm1( true, i_encryptionKey, i_length, i_id, i_gen );
	reset();
}

void StandardAESSecurityCrypter::reset()
{
	AES_set_decrypt_key(
	    (const unsigned char *)_key.data(), _keyLength * 8, &_aesKey );
	_aesPos = AES_BLOCK_SIZE;
}

void StandardAESSecurityCrypter::rewind()
{
	rewindNext();
	reset();
	_cbc = nullptr;
}

std::streamoff StandardAESSecurityCrypter::read(
    su::array_view<uint8_t> o_buffer )
{
	// check if we read the initial cbc block
	if ( _cbc == nullptr )
	{
		_cbc = _aesBuffer;
		_nextBlock = _aesBuffer + AES_BLOCK_SIZE;
		_decodedBlock = _nextBlock + AES_BLOCK_SIZE;
		// initial cbc block
		auto l = readNext( {_cbc, AES_BLOCK_SIZE} );
		if ( l < AES_BLOCK_SIZE )
			log_error() << "AES encryption block too short";
		// read the next block
		l = readNext( {_nextBlock, AES_BLOCK_SIZE} );
		if ( l < AES_BLOCK_SIZE )
			log_error() << "AES encryption block too short";
	}

	size_t p = 0;
	while ( p < o_buffer.size() )
	{
		if ( _aesPos < AES_BLOCK_SIZE )
		{
			auto l = std::min( size_t( AES_BLOCK_SIZE - _aesPos ),
			                   o_buffer.size() - p );
			memcpy( o_buffer.data() + p, _decodedBlock + _aesPos, l );
			_aesPos += l;
			p += l;
		}
		else if ( _nextBlock != nullptr )
		{
			// decrypt the next block
			AES_decrypt( _nextBlock, _decodedBlock, &_aesKey );
			for ( int c = 0; c < AES_BLOCK_SIZE; ++c )
				_decodedBlock[c] = _decodedBlock[c] ^ _cbc[c];

			// setup the next cbc
			std::swap( _cbc, _nextBlock );

			// read the next block
			auto l = readNext( {_nextBlock, AES_BLOCK_SIZE} );
			if ( l < AES_BLOCK_SIZE )
			{
				// last block, clear the padding
				_aesPos = _decodedBlock[AES_BLOCK_SIZE - 1];
				memmove( _decodedBlock + _aesPos,
				         _decodedBlock,
				         AES_BLOCK_SIZE - _aesPos );
				_nextBlock = nullptr;
			}
			else
				_aesPos = 0;
		}
		else
			return p == 0 ? EOF : p;
	}
	return p;
}

std::string StandardAESSecurityCrypter::decryptString(
    const std::string &i_input )
{
	if ( i_input.length() < ( 2 * AES_BLOCK_SIZE ) )
	{
		log_error() << "AES encryption block too short";
		return {};
	}
	std::string output;
	output.reserve( i_input.length() - AES_BLOCK_SIZE );

	auto cbc = (uint8_t *)i_input.data();
	auto end = cbc + i_input.size();
	for ( ;; )
	{
		// decrypt the next block
		uint8_t decodedBlock[AES_BLOCK_SIZE];
		auto nextBlock = cbc + AES_BLOCK_SIZE;
		AES_decrypt( nextBlock, decodedBlock, &_aesKey );
		for ( int c = 0; c < AES_BLOCK_SIZE; ++c )
			decodedBlock[c] = decodedBlock[c] ^ cbc[c];

		// setup the next cbc
		cbc += AES_BLOCK_SIZE;

		if ( ( cbc + AES_BLOCK_SIZE ) >= end )
		{
			// last block, handle the padding
			output.append( (const char *)decodedBlock,
			               AES_BLOCK_SIZE - decodedBlock[AES_BLOCK_SIZE - 1] );
			break;
		}
		else
			output.append( (const char *)decodedBlock, AES_BLOCK_SIZE );
	}
	return output;
}

// MARK: -

StandardSecurityHandler::StandardSecurityHandler( const Object &i_encryptDict )
{
	const auto &VObj = i_encryptDict["V"];
	if ( not VObj.is_number() )
		throw std::runtime_error( "Encrypt dictionary: missing V entry" );
	V = VObj.int_value();
	if ( V != 1 and V != 2 and V != 4 ) // algorithm v == 3 is unpublished...
		throw std::runtime_error( "Encrypt dictionary: invalid V entry" );

	auto &Length = i_encryptDict["Length"];
	if ( Length.is_number() )
	{
		length = Length.int_value();
		if ( length < 40 or length > 128 or ( length & 7 ) != 0 )
		{
			throw std::runtime_error(
			    "Encrypt dictionary: invalid Length entry" );
		}
	}

	auto &RObj = i_encryptDict["R"];
	if ( RObj.is_number() )
		R = RObj.int_value();
	if ( V == 4 and R != 4 )
	{
		log_warn() << "Encrypt dictionary: R should be 4";
		R = 4;
	}

	auto &OObj = i_encryptDict["O"];
	if ( OObj.is_string() )
		O = OObj.string_value();
	if ( O.length() != 32 )
		throw std::runtime_error( "Encrypt dictionary: invalid O entry" );

	auto &UObj = i_encryptDict["U"];
	if ( UObj.is_string() )
		U = UObj.string_value();
	if ( U.length() != 32 )
		throw std::runtime_error( "Encrypt dictionary: invalid U entry" );

	auto &PObj = i_encryptDict["P"];
	if ( PObj.is_number() )
		P = (uint32_t)PObj.int_value();

	if ( V == 4 )
	{
		auto &EncryptMetadataObj = i_encryptDict["EncryptMetadata"];
		if ( EncryptMetadataObj.is_boolean() )
			EncryptMetadata = EncryptMetadataObj.bool_value();

		auto &CFObj = i_encryptDict["CF"];
		if ( CFObj.is_dictionary() )
		{
			for ( auto &it : CFObj.dictionary_items_resolved() )
			{
				auto &cryptDict = it.second;
				if ( not cryptDict.is_dictionary() )
					continue;

				CFData data;
				auto &CFM = cryptDict["CFM"];
				if ( not CFM.is_name() )
					data.CFM = "None";
				else
					data.CFM = CFM.name_value();

				auto &AuthEvent = cryptDict["AuthEvent"];
				if ( not AuthEvent.is_name() )
					data.AuthEvent = "DocOpen";
				else
					data.AuthEvent = AuthEvent.name_value();

				auto &Length = cryptDict["Length"];
				if ( not Length.is_number() )
					data.length = length;
				else
					data.length = Length.int_value();

				CF.insert( std::make_pair( it.first, data ) );
			}
			auto &StmFObj = i_encryptDict["StmF"];
			if ( not StmFObj.is_name() )
				StmF = "Identity";
			else
				StmF = StmFObj.name_value();

			auto &StrFObj = i_encryptDict["StrF"];
			if ( not StrFObj.is_name() )
				StrF = "Identity";
			else
				StrF = StrFObj.name_value();

			auto &EFFObj = i_encryptDict["EFF"];
			if ( EFFObj.is_name() )
				EFF = EFFObj.name_value();
		}
	}
}

std::unique_ptr<Crypter> StandardSecurityHandler::createDefaultStreamCrypter(
    bool i_isMetadata, int i_id, int i_gen ) const
{
	if ( _unlocked and ( not i_isMetadata or EncryptMetadata ) )
	{
		bool useAES = false;
		if ( V == 4 and R == 4 )
		{
			auto it = CF.find( StmF );
			if ( it != CF.end() )
				useAES = ( it->second.CFM == "AESV2" );
		}
		if ( useAES )
		{
			return std::make_unique<StandardAESSecurityCrypter>(
			    _encryptionKey, length, i_id, i_gen );
		}
		else
		{
			return std::make_unique<StandardSecurityCrypter>(
			    _encryptionKey, length, i_id, i_gen );
		}
	}
	else
		return nullptr;
}

std::string StandardSecurityHandler::decryptString( const std::string &i_input,
                                                    int i_id,
                                                    int i_gen )
{
	bool useAES = false;
	if ( V == 4 and R == 4 )
	{
		auto it = CF.find( StrF );
		if ( it != CF.end() )
			useAES = ( it->second.CFM == "AESV2" );
	}
	if ( useAES )
	{
		StandardAESSecurityCrypter crypter(
		    _encryptionKey, length, i_id, i_gen );
		return crypter.decryptString( i_input );
	}
	else
	{
		StandardSecurityCrypter crypter( _encryptionKey, length, i_id, i_gen );
		return crypter.decryptString( i_input );
	}
}

bool StandardSecurityHandler::unlock( Document *i_doc,
                                      const std::string &i_password )
{
	if ( not _unlocked )
	{
		// get file ID
		auto ids = i_doc->getFileID();
		ComputeEncryptionKey( i_password, ids.first );

		// algorithm 4 and 5 from the PDF specs

		std::array<uint8_t, 32> key;
		if ( R == 2 )
		{
			int l = length / 8;

			// b) c)
			RC4_KEY rc4Key;
			RC4_set_key( &rc4Key, l, _encryptionKey.data() );
			RC4( &rc4Key, 32, kPaddingString, key.data() );

			int cmpSize = 32;
			if ( memcmp( key.data(), U.data(), cmpSize ) == 0 )
				_unlocked = true;
		}
		else if ( R >= 3 )
		{
			// b)
			MD5_CTX c;
			MD5_Init( &c );
			MD5_Update( &c, kPaddingString, 32 );

			// c)
			MD5_Update( &c,
			            (const uint8_t *)ids.first.data(),
			            (uint32_t)ids.first.length() );

			uint8_t md[16];
			MD5_Final( md, &c );

			// d)
			memcpy( key.data(), U.data(), U.length() );

			// e)
			for ( int i = 1; i <= 20; ++i )
			{
				uint8_t xorEncryptionKey[16];
				for ( int j = 0; j < 16; ++j )
					xorEncryptionKey[j] = _encryptionKey[j] ^ ( 20 - i );

				RC4_KEY rc4Key;
				RC4_set_key( &rc4Key, 16, xorEncryptionKey );

				uint8_t tmp[32];
				memcpy( tmp, key.data(), 32 );
				RC4( &rc4Key, 32, tmp, key.data() );
			}

			int cmpSize = 16;
			if ( memcmp( key.data(), md, cmpSize ) == 0 )
				_unlocked = true;
		}
		else
		{
			log_error() << "unknown security handler revision";
			assert( false );
		}
	}
	return _unlocked;
}

bool StandardSecurityHandler::isUnlocked() const
{
	return _unlocked;
}

void StandardSecurityHandler::ComputeEncryptionKey(
    const std::string &i_password, const std::string &i_fileID )
{
	// algorithm 2 from the PDF specs

	// a)
	uint8_t password[32];
	size_t n = std::min( i_password.length(), size_t( 32 ) );
	memcpy( password, i_password.data(), n );
	memcpy( password + n, kPaddingString, 32 - n );

	// b) init and pass password
	MD5_CTX c;
	MD5_Init( &c );

	MD5_Update( &c, password, 32 );

	// c) O
	MD5_Update( &c, (const uint8_t *)O.data(), (uint32_t)O.length() );

	// d) P as 32 bits little endian
	uint32_t p = su::native_to_little( P );
	MD5_Update( &c, (const uint8_t *)&p, 4 );

	// e) first component of file id
	MD5_Update(
	    &c, (const uint8_t *)i_fileID.data(), (uint32_t)i_fileID.length() );

	// f)
	if ( R >= 4 and not EncryptMetadata )
	{
		const uint32_t kFFFFFFFF = 0xFFFFFFFF;
		MD5_Update( &c, (const uint8_t *)&kFFFFFFFF, 4 );
	}

	// g)
	uint8_t md[16];
	MD5_Final( md, &c );
	if ( R >= 3 )
	{
		uint8_t md5Key[16];
		for ( int i = 0; i < 50; ++i )
		{
			memcpy( md5Key, md, 16 );
			MD5_Init( &c );
			MD5_Update( &c, md5Key, 16 );
			MD5_Final( md, &c );
		}
	}

	// i)
	memcpy( _encryptionKey.data(), md, 16 );
}

}
