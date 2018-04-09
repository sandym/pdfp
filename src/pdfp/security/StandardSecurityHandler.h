//
//  StandardSecurityHandler.h
//  pdfp
//
//  Created by Sandy Martel on 11/10/03.
//  Copyright 2011年 Sandy Martel. All rights reserved.
//

#ifndef H_PDFP_StandardSecurityHandler
#define H_PDFP_StandardSecurityHandler

#include "SecurityHandler.h"
#include "pdfp/crypto/aes.h"
#include "pdfp/crypto/rc4.h"
#include <array>
#include <unordered_map>

namespace pdfp {

class StandardSecurityCrypter final : public Crypter
{
public:
	StandardSecurityCrypter( const std::array<uint8_t, 16> &i_encryptionKey,
	                             int i_length,
	                             int i_id,
	                             int i_gen );

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

	std::string decryptString( const std::string &i_input );

private:
	std::array<uint8_t, 16> _key;
	int _keyLength;

	RC4_KEY _rc4Key;

	void reset();
};

class StandardAESSecurityCrypter final : public Crypter
{
public:
	StandardAESSecurityCrypter( const std::array<uint8_t, 16> &i_encryptionKey,
	                             int i_length,
	                             int i_id,
	                             int i_gen );

	virtual void rewind();
	virtual std::streamoff read( su::array_view<uint8_t> o_buffer );

	std::string decryptString( const std::string &i_input );

private:
	std::array<uint8_t, 16> _key;
	int _keyLength;

	uint8_t _aesBuffer[3 * AES_BLOCK_SIZE]; // cbc, decoded and next block
	uint8_t *_cbc{ nullptr };
	uint8_t *_nextBlock{ nullptr };
	uint8_t *_decodedBlock{ nullptr };
	int _aesPos;
	AES_KEY _aesKey;

	void reset();
};

class StandardSecurityHandler : public SecurityHandler
{
public:
	StandardSecurityHandler( const Object &i_encryptDict );
	virtual ~StandardSecurityHandler() = default;

	virtual std::unique_ptr<Crypter> createDefaultStreamCrypter(
	    bool i_isMetadata, int i_id, int i_gen ) const;
	virtual std::string decryptString( const std::string &i_input,
	                            int i_id,
	                            int i_gen );

	virtual bool unlock( Document *i_doc, const std::string &i_password );
	virtual bool isUnlocked() const;

private:
	int V = 0, length = 40, R = 0;
	uint32_t P = 0;
	std::string O, U;
	bool EncryptMetadata = true;

	// for V4
	struct CFData
	{
		std::string CFM, AuthEvent;
		int length;
	};
	std::unordered_map<std::string, CFData> CF;
	std::string StmF, StrF, EFF;

	std::array<uint8_t, 16> _encryptionKey;
	bool _unlocked = false;

	void ComputeEncryptionKey( const std::string &i_password,
	                           const std::string &i_fileID );
};
}

#endif
