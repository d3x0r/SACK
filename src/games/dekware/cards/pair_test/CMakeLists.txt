cmake_minimum_required(VERSION 2.8)

project( pair_tester.nex )

set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
	pair_tester.c
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
	RUNTIME DESTINATION bin/plugins/test
	LIBRARY DESTINATION bin/plugins/test
)


