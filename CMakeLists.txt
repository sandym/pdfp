cmake_minimum_required( VERSION 3.8 )
project( pddfp )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# configure
execute_process( COMMAND git rev-parse HEAD
WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE )

set( PRODUCT_VERSION "0.0.1.1" )

add_subdirectory( ../sutils sutils )

add_library( pdfp STATIC
	src/pdfp/PDFData.h
	src/pdfp/PDFDocument.cpp
	src/pdfp/PDFDocument.h
	src/pdfp/PDFObject.cpp
	src/pdfp/PDFObject.h
	src/pdfp/PDFPage.cpp
	src/pdfp/PDFPage.h
	src/pdfp/PDFContentsParser.h
	src/pdfp/PDFContentsParser.cpp
	src/pdfp/crypto/aes.cpp
	src/pdfp/crypto/aes.h
	src/pdfp/crypto/md5.cpp
	src/pdfp/crypto/md5.h
	src/pdfp/crypto/rc4.cpp
	src/pdfp/crypto/rc4.h
	src/pdfp/filters/ASCII85.cpp
	src/pdfp/filters/ASCII85.h
	src/pdfp/filters/ASCIIHex.cpp
	src/pdfp/filters/ASCIIHex.h
	src/pdfp/filters/CCITTFax.cpp
	src/pdfp/filters/CCITTFax.h
	src/pdfp/filters/Filter.cpp
	src/pdfp/filters/Filter.h
	src/pdfp/filters/Flate.cpp
	src/pdfp/filters/Flate.h
	src/pdfp/filters/JBIG2.cpp
	src/pdfp/filters/JBIG2.h
	src/pdfp/filters/LZW.cpp
	src/pdfp/filters/LZW.h
	src/pdfp/filters/PNGPredictor.cpp
	src/pdfp/filters/PNGPredictor.h
	src/pdfp/filters/RunLength.cpp
	src/pdfp/filters/RunLength.h
	src/pdfp/filters/TIFFPredictor.cpp
	src/pdfp/filters/TIFFPredictor.h
	src/pdfp/impl/DataFactory.cpp
	src/pdfp/impl/DataFactory.h
	src/pdfp/impl/ImageStreamInfo.cpp
	src/pdfp/impl/ImageStreamInfo.h
	src/pdfp/impl/Parser.cpp
	src/pdfp/impl/Parser.h
	src/pdfp/impl/Tokenizer.cpp
	src/pdfp/impl/Tokenizer.h
	src/pdfp/impl/Utils.h
	src/pdfp/impl/XrefTable.cpp
	src/pdfp/impl/XrefTable.h
	src/pdfp/security/SecurityHandler.cpp
	src/pdfp/security/SecurityHandler.h
	src/pdfp/security/StandardSecurityHandler.cpp
	src/pdfp/security/StandardSecurityHandler.h
				)

target_include_directories( pdfp PUBLIC src )
target_link_libraries( pdfp sutils )

source_group( "src/pdfp" FILES
					src/pdfp/PDFData.h
					src/pdfp/PDFDocument.cpp
					src/pdfp/PDFDocument.h
					src/pdfp/PDFObject.cpp
					src/pdfp/PDFObject.h
					src/pdfp/PDFPage.cpp
					src/pdfp/PDFPage.h
					src/pdfp/PDFContentsParser.h
					src/pdfp/PDFContentsParser.cpp
		 )

source_group( "src/pdfp/imp" FILES
				src/pdfp/impl/DataFactory.cpp
				src/pdfp/impl/DataFactory.h
				src/pdfp/impl/ImageStreamInfo.cpp
				src/pdfp/impl/ImageStreamInfo.h
				src/pdfp/impl/Parser.cpp
				src/pdfp/impl/Parser.h
				src/pdfp/impl/Tokenizer.cpp
				src/pdfp/impl/Tokenizer.h
				src/pdfp/impl/Utils.h
				src/pdfp/impl/XrefTable.cpp
				src/pdfp/impl/XrefTable.h
			)
source_group( "src/pdfp/filters" FILES
				src/pdfp/filters/ASCII85.cpp
				src/pdfp/filters/ASCII85.h
				src/pdfp/filters/ASCIIHex.cpp
				src/pdfp/filters/ASCIIHex.h
				src/pdfp/filters/CCITTFax.cpp
				src/pdfp/filters/CCITTFax.h
				src/pdfp/filters/Filter.cpp
				src/pdfp/filters/Filter.h
				src/pdfp/filters/Flate.cpp
				src/pdfp/filters/Flate.h
				src/pdfp/filters/JBIG2.cpp
				src/pdfp/filters/JBIG2.h
				src/pdfp/filters/LZW.cpp
				src/pdfp/filters/LZW.h
				src/pdfp/filters/PNGPredictor.cpp
				src/pdfp/filters/PNGPredictor.h
				src/pdfp/filters/RunLength.cpp
				src/pdfp/filters/RunLength.h
				src/pdfp/filters/TIFFPredictor.cpp
				src/pdfp/filters/TIFFPredictor.h
		)
source_group( "src/pdfp/security" FILES
				src/pdfp/security/SecurityHandler.cpp
				src/pdfp/security/SecurityHandler.h
				src/pdfp/security/StandardSecurityHandler.cpp
				src/pdfp/security/StandardSecurityHandler.h
		)
source_group( "src/pdfp/crypto" FILES
				src/pdfp/crypto/aes.cpp
				src/pdfp/crypto/aes.h
				src/pdfp/crypto/md5.cpp
				src/pdfp/crypto/md5.h
				src/pdfp/crypto/rc4.cpp
				src/pdfp/crypto/rc4.h
	)