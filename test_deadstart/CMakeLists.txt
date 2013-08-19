
set( CMAKE_VERBOSE_MAKEFILE  1 )

OPTION( C++ "Build with C++" OFF )

add_library( testlib SHARED lib.c )
add_executable( test main.c )
target_link_libraries( test testlib )


if( C++ )
  set( CMAKE_C_COMPILER "${CMAKE_CXX_COMPILER}" )
  set( CMAKE_C_COMPILE_OBJECT "${CMAKE_CXX_COMPILE_OBJECT}" )
endif()

