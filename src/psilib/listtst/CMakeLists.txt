
set( project test.listbox )

set(BASE_SOURCES
    listbox.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests/PSI
)
install_mode_dest( ${project} )


