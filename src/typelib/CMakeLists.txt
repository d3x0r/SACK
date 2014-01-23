

set( project test.space.web )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    spaceweb.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${project} WIN32 ${BASE_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
           COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL};MAKE_RCOORD_SINGLE"
           FOLDER tests
)
target_link_libraries( ${project} ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )
if( UNIX )
target_link_libraries( ${project} GL)

endif( UNIX )

install_default_dest( ${project} )



