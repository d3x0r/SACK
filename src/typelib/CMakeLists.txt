

set( project test.space.web )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    spaceweb.c
${LAST_GCC_PROGRAM_SOURCE}
)

ADD_PROGRAM(${project} WIN32 ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${project} PROPERTIES
           FOLDER tests
)
SET_PROPERTY(TARGET ${project} APPEND PROPERTY
           COMPILE_DEFINITIONS  "MAKE_RCOORD_SINGLE"
)

install_default_dest( ${project} )



