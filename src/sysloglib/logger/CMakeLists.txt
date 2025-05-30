
set( project syslog.logger )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    logger.c
${LAST_GCC_PROGRAM_SOURCE}
)

sack_add_executable(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )

string( REPLACE ";" " " PLATFORM_LIBRARIES_ARG "${SACK_PLATFORM_LIBRARIES}" )
string( REPLACE ";" " " PLATFORM_DEFINES_ARG "${PLATFORM_DEFINES}" )
string( REPLACE ";" "," PLATFORM_CMAKE_DEFINES_ARG "${PLATFORM_CMAKE_DEFINES}" )

ExternalProject_Add( ${project}_portable 
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/portable
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
		-DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE} 
		-DSACK_BASE=${SACK_BASE}
		-DSACK_SDK_ROOT_PATH=${SACK_SDK_ROOT_PATH}
		-DPLATFORM_LIBRARIES=${PLATFORM_LIBRARIES_ARG}
		-DPLATFORM_DEFINES=${PLATFORM_DEFINES_ARG}
		-DPLATFORM_CMAKE_DEFINES=${PLATFORM_CMAKE_DEFINES_ARG}
)
#add_subdirectory( portable )
