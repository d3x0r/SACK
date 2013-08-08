
cmake_minimum_required(VERSION 2.8)

add_subdirectory( pscards )

project( cards.nex )

set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
	ntlink
${LAST_GCC_LIBRARY_SOURCE} )


add_library( ${PROJECT_NAME} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  SUFFIX ""
                  PREFIX ""
                  FOLDER plugins
)


target_link_libraries( ${PROJECT_NAME} 
	${SACK_LIBRARIES}
	pscards
)

INSTALL( TARGETS ${PROJECT_NAME} 
	RUNTIME DESTINATION bin/plugins
	LIBRARY DESTINATION bin/plugins
)

INSTALL( FILES cards.h DESTINATION include/dekware/cards )
INSTALL( DIRECTORY images DESTINATION resources )

add_subdirectory( decktest )
add_subdirectory( pair_test )
