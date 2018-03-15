cmake_minimum_required( VERSION 3.8 )
project( pdfp_tests )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory( .. pdfp )

add_executable( pdfp_tests main.cpp
						dumpers/pdfp_dumper.h
						dumpers/dumper.h
						dumpers/dumper_utils.h )
target_include_directories( pdfp_tests PRIVATE dumpers )

target_link_libraries( pdfp_tests pdfp )

find_package( Threads )
target_link_libraries( pdfp_tests ${CMAKE_THREAD_LIBS_INIT} )

file( GLOB dumpers dumpers/* )
foreach( dumper ${dumpers} )
	if( IS_DIRECTORY ${dumper} )
		unset( TO_LINK )
		add_subdirectory(${dumper})
		if( DEFINED TO_LINK )
			target_link_libraries( pdfp_tests ${TO_LINK} )
		endif()
	endif()
endforeach()

find_library( ZLIB_LIBRARY z )
target_link_libraries( pdfp_tests ${ZLIB_LIBRARY} )
