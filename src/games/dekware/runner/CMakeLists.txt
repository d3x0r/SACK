
set( PROJECT Dekware )

if( __ANDROID__ )
    add_definitions( -DLOAD_LIBNAME="${CMAKE_SHARED_LIBRARY_PREFIX}Dekware.core${CMAKE_SHARED_LIBRARY_SUFFIX}" -DMODE=0 )
else( __ANDROID__ )
    add_definitions( -DLOAD_LIBNAME="dekware.core" -DMODE=0 )
endif( __ANDROID__ )


ADD_PROGRAM( ${PROJECT} WIN32 
${FIRST_GCC_PROGRAM_SOURCE}
   ${SACK_BASE}/src/sack/runwin.c 
${LAST_GCC_PROGRAM_SOURCE}
   
)
SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES PROJECT_LABEL "Dekware.launcher" )


