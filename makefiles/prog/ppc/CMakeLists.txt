CMAKE_MINIMUM_REQUIRED( VERSION 3.15 )

# Host-platform plumbing forwarded by the root project's ExternalProject_Add( ppc_portable ).
# Unset when this directory is configured standalone, which is fine.
if( PLATFORM_LIBRARIES )
	STRING( REPLACE " " ";" PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} )
	STRING( REPLACE " " ";" PLATFORM_DEFINES ${PLATFORM_DEFINES} )

	add_definitions( ${PLATFORM_DEFINES} )

	if( PLATFORM_CMAKE_DEFINES )
		STRING( REPLACE "," ";" PLATFORM_CMAKE_DEFINES ${PLATFORM_CMAKE_DEFINES} )
		foreach( DEF IN LISTS PLATFORM_CMAKE_DEFINES)
			STRING( REPLACE " " ";" SETDEF ${DEF} )
			set( ${SETDEF} )
		endforeach()
	endif( PLATFORM_CMAKE_DEFINES )
endif( PLATFORM_LIBRARIES )

PROJECT( Panthers_pre_cpp C )

if( UNIX )
	ADD_DEFINITIONS( -D__LINUX__ )
endif( UNIX )

set( MYSOURCES
	args.c
	define.c
	links.c
	text.c
	input.c
	cppmain.c
	mem.c
	fileio.c
	expr.c )

add_executable( ppc ${MYSOURCES} )

# Static CRT, so the installed ppc.exe carries no redist dependency.  The old
# /MD->/MT CMAKE_*_FLAGS rewriting this replaces had been inert since the minimum
# was raised to 3.15 and CMP0091 went NEW - it was building /MD.  Delete this line
# if a dynamic CRT is wanted.
set_property( TARGET ppc PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" )

INSTALL( TARGETS ppc DESTINATION bin )
