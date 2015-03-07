cmake_required_version( 3.0 )

PROJECT( MatrixSSL )

if( WIN32 )
set( OSDEP_DEFAULT "WIN32" )
else( WIN32 )
set( OSDEP_DEFAULT "POSIX" )
endif( WIN32 )


SET( OSDEP "POSIX" CACHE STRING "Which OS Extras to use" )
set_property(CACHE OSDEP PROPERTY STRINGS "POSIX" "Win32" )

add_subdirectory( core )
add_subdirectory( crypto )
add_subdirectory( matrixssl )

