
set( project BubbleScreenSaver )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    bubbles2.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)

message( "project ${project} needs better install path" )
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION "${DEFAULT_WORKING_DIRECTORY}"
    FILES_MATCHING PATTERN "*.png" )
