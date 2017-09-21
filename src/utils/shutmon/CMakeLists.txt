
set( project shutdown.monitor )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    shutmon.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${project} WIN32 ${BASE_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
           COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}"
           FOLDER utils
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )


if( INTERSHELL_SDK_ROOT )
set( project shutdown.monitor.isp )


set(BASE_SOURCES
    ${FIRST_GCC_LIBRARY_SOURCE} 
        shutmon.c
    ${LAST_GCC_LIBRARY_SOURCE}
)

add_executable(${project} SHARED ${BASE_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
           COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}"
           FOLDER utils
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
INSTALL_PLUGIN( ${project} )
endif( INTERSHELL_SDK_ROOT )
