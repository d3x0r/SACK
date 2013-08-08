
set( PROJECT dekware )

add_definitions( -DLOAD_LIBNAME="dekware.core" -DMODE=0 )

ADD_PROGRAM( ${PROJECT} WIN32 
${FIRST_GCC_PROGRAM_SOURCE}
   ${SACK_BASE}/src/sack/runwin.c 
${LAST_GCC_PROGRAM_SOURCE}
   
)


