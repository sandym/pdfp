cmake_minimum_required( VERSION 3.8 )

if( EXISTS ${APDFL_PREFIX}/Include/Headers )
	target_compile_definitions( pdfp_tests PRIVATE PDFP_WANT_APDFL )
	target_include_directories( pdfp_tests PRIVATE ${APDFL_PREFIX}/Include/Headers
											${APDFL_PREFIX}/Include/Source )
	target_sources( pdfp_tests PRIVATE ../dumpers/apdfl/apdfl.h ../dumpers/apdfl/apdfl.cpp )
	find_library( DL100PDFL DL100PDFL ${APDFL_PREFIX}/Libs )
	set( TO_LINK ${DL100PDFL} PARENT_SCOPE )
endif()
