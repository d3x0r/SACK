
set( project rundelay )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    rundelay.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)


add_subdirectory( portable )
