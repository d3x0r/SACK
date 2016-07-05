cmake_minimum_required(VERSION 3.0)
set_property(GLOBAL PROPERTY USE_FOLDERS On)

PROJECT( MatrixSSL )

if( MINGW )
   #ADD_DEFINITIONS( -DMINGW_SUX )
endif( MINGW )

if( WIN32 )
  set( OSDEP_DEFAULT "WIN32" )
#  set( LIBFLAGS _USRDLL )
  set( SYS_C_FLAGS  )
else( WIN32 )
  set( OSDEP_DEFAULT "POSIX" )
  set( LIBFLAGS  )
  set( SYS_C_FLAGS -fPIC )
endif( WIN32 )


SET( OSDEP ${OSDEP_DEFAULT} CACHE STRING "Which OS Extras to use" )
set_property(CACHE OSDEP PROPERTY STRINGS "POSIX" "Win32" )

INCLUDE_DIRECTORIES( . )

add_subdirectory( core )
add_subdirectory( crypto )
add_subdirectory( matrixssl )
add_subdirectory( apps )

if( BUILD_TESTS )
add_subdirectory( crypto/test )
add_subdirectory( matrixssl/test )
endif( BUILD_TESTS )

