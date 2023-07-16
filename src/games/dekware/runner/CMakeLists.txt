
set( PROJECT Dekware )

if( __ANDROID__ )
    add_definitions( -DLOAD_LIBNAME="${CMAKE_SHARED_LIBRARY_PREFIX}Dekware.core${CMAKE_SHARED_LIBRARY_SUFFIX}" -DMODE=0 )
else( __ANDROID__ )
    add_definitions( -DLOAD_LIBNAME="@/../${COMPILED_INSTALL_PREFIX}/applicationCore/Dekware.core" -DMODE=0 )
endif( __ANDROID__ )


if( MAKING_SACK_CORE )

ADD_PROGRAM( ${PROJECT} WIN32 
${FIRST_GCC_PROGRAM_SOURCE}
   ../../../utils/runner/runwin.c 
${LAST_GCC_PROGRAM_SOURCE}
   
)

else( MAKING_SACK_CORE )

ADD_PROGRAM( ${PROJECT} WIN32 
${FIRST_GCC_PROGRAM_SOURCE}
   ${SACK_BASE}/src/SACK/runwin.c 
${LAST_GCC_PROGRAM_SOURCE}
   
)

endif( MAKING_SACK_CORE )

SET_TARGET_PROPERTIES( ${PROJECT} PROPERTIES PROJECT_LABEL "Dekware.launcher" )


