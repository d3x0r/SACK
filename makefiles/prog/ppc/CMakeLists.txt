CMAKE_MINIMUM_REQUIRED( VERSION 2.4 )

STRING( REPLACE " " ";" PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} )
STRING( REPLACE " " ";" PLATFORM_DEFINES ${PLATFORM_DEFINES} )

add_definitions( ${PLATFORM_DEFINES} )

PROJECT( Panthers_pre_cpp )

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

INSTALL( TARGETS ppc DESTINATION bin )
