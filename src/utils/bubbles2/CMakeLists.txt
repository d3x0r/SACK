
set( project BubbleScreenSaver )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    bubbles2.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)

message( "project ${project} needs better install path" )
install( FILES *.png DESTINATION ${BINARY_OUTPUT_DIR} )

