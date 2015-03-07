cmake_minimum_required(VERSION 3.0)
set_property(GLOBAL PROPERTY USE_FOLDERS On)

PROJECT( MatrixSSL )

if( WIN32 )
  set( OSDEP_DEFAULT "WIN32" )
  set( LIBFLAGS _USRDLL )
else( WIN32 )
  set( OSDEP_DEFAULT "POSIX" )
  set( LIBFLAGS _USRDLL )
endif( WIN32 )


SET( OSDEP ${OSDEP_DEFAULT} CACHE STRING "Which OS Extras to use" )
set_property(CACHE OSDEP PROPERTY STRINGS "POSIX" "Win32" )

INCLUDE_DIRECTORIES( . )

add_subdirectory( core )
add_subdirectory( crypto )
add_subdirectory( matrixssl )

