
set( project waitwin )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    waitwin.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)


set( project waitproc )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    waitproc.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)

add_subdirectory( portable )
