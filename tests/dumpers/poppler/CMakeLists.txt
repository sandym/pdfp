cmake_minimum_required( VERSION 3.8 )

if( EXISTS /opt/local/include/poppler )
	target_compile_definitions( pdfp_tests PRIVATE PDFP_WANT_POPPLER )
	target_sources( pdfp_tests PRIVATE ../dumpers/poppler/poppler.h ../dumpers/poppler/poppler.cpp )
	target_include_directories( pdfp_tests PRIVATE /opt/local/include/poppler )
	find_library( poppler poppler /opt/local/lib )
	set( TO_LINK ${poppler} PARENT_SCOPE )
endif()
