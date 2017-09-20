
set( project launchcmd )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    launchcmd
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils/launcher
)

if( WIN32 )
set( project launchwin )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE}
    launchcmd
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils/launcher
)

endif( WIN32 )

string( REPLACE ";" " " PLATFORM_LIBRARIES_ARG "${SACK_PLATFORM_LIBRARIES}" )
string( REPLACE ";" " " PLATFORM_DEFINES_ARG "${PLATFORM_DEFINES}" )

ExternalProject_Add( ${project}_portable 
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/portable
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
		-DSACK_BASE=${SACK_BASE}
		-DPLATFORM_LIBRARIES=${PLATFORM_LIBRARIES_ARG}
		-DPLATFORM_DEFINES=${PLATFORM_DEFINES_ARG}
		-DSHA1_SOURCES=${SHA1_SOURCES}
		-DSHA2_SOURCES=${SHA2_SOURCES}
)

#add_subdirectory( portable )
