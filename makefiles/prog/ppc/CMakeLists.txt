CMAKE_MINIMUM_REQUIRED( VERSION 3.15 )

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

PROJECT( Panthers_pre_cpp )

if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
  # MESSAGE( "THIS FIXES COMPILER IS GNUCC - ${CMAKE_CXX_COMPILER_ID}" )
  ADD_DEFINITIONS( -Wno-parentheses )
  ADD_DEFINITIONS( -Wno-null-dereference )
  ADD_DEFINITIONS( -Wno-self-assign ) # so what if a variable is assigned to itself - that's what optimizers are for?
  ADD_DEFINITIONS( -Wno-null-dereference )
  set( GCC 1 )
  ADD_DEFINITIONS( -D_GNU_SOURCE )
  set( CMAKE_COMPILER_IS_GNUCC 0 )
endif()

if( WIN32 )
   ADD_DEFINITIONS( -D_CRT_NONSTDC_NO_DEPRECATE )
endif()

if(MSVC)
# We statically link to reduce dependencies
foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
                 CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
    if(${flag_var} MATCHES "/MDd")
        string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MDd")
    set( ${flag_var} "${${flag_var}}" CACHE STRING "cached." FORCE )
endforeach(flag_var)
endif(MSVC)


if( UNIX) 
  ADD_DEFINITIONS( -D__LINUX__ )
endif( UNIX) 

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


  ADD_DEFINITIONS( -D_GNU_SOURCE )

add_executable( ppc ${MYSOURCES} )

INSTALL( TARGETS ppc DESTINATION bin )
