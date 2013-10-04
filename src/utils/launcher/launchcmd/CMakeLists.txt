
set( project launchcmd )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    launchcmd
${LAST_GCC_PROGRAM_SOURCE}
)

sack_add_executable(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${project}" 
                  FOLDER utils/launcher
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )

if( WIN32 )
set( project launchwin )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE}
    launchcmd
${LAST_GCC_PROGRAM_SOURCE}
)

sack_add_executable(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${project} -DWINDOWS_MODE" 
                  FOLDER utils/launcher
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )

endif( WIN32 )

add_subdirectory( portable )
