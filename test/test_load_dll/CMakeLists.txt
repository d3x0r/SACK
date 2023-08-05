cmake_minimum_required(VERSION 3.0)

PROJECT( test_dll_load )

add_executable( main main.c )

add_library( dll SHARED dll.c )
add_library( dll_dep SHARED dll_dep.c )
target_link_libraries( dll dll_dep )

set( CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} )

INSTALL( TARGETS main DESTINATION bin )
INSTALL( TARGETS dll DESTINATION bin/plugin )
INSTALL( TARGETS dll_dep DESTINATION bin/plugin )
