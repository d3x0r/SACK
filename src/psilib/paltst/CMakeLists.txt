
set( project test.palette )

set(BASE_SOURCES
    palette.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests/PSI
)
install_default_dest( ${project} )


