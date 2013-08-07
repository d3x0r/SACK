
set( project TouchPlotter )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    plotter.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER tests
)


