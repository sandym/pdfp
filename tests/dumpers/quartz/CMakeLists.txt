cmake_minimum_required( VERSION 3.8 )

if(APPLE)
	target_compile_definitions( pdfp_tests PRIVATE PDFP_WANT_QUARTZ )
	target_sources( pdfp_tests PRIVATE ../dumpers/quartz/quartz.h ../dumpers/quartz/quartz.cpp )
	find_library( ApplicationServices ApplicationServices )
	find_library( COREFOUNDATION_LIBRARY CoreFoundation )
	set( TO_LINK
						${COREFOUNDATION_LIBRARY}
						${ApplicationServices}
						PARENT_SCOPE )
endif()
