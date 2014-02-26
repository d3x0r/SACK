
set( project TouchPlotter )

set(BASE_SOURCES
    plotter.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER tests
)


