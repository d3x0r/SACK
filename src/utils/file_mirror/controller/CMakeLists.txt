
set( project FileMirrorRemote )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    rcontrol.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER "utils/File Mirror"
)

add_subdirectory( gui )

