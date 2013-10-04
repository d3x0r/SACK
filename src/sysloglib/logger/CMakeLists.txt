
set( project syslog.logger )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    logger.c
${LAST_GCC_PROGRAM_SOURCE}
)

sack_add_executable(${project} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
	COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}" 
                  FOLDER utils
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )

add_subdirectory( portable )
