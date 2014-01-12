
set( project test.listbox )

set(BASE_SOURCES
    ${FIRST_GCC_PROGRAM_SOURCE}
    listbox.c
    ${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${project} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}"
                  FOLDER tests/PSI
)
target_link_libraries( ${project} ${SACK_LIBRARIES})
install_mode_dest( ${project} )


