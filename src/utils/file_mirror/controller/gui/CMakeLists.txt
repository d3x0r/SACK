
set( project FileMirrorGuiRemote )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    guinet.c guiremote.c main.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER "utils/File Mirror"
)

