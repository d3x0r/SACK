
cmake_minimum_required(VERSION 2.8)

project( file.nex )


set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
	file
${LAST_GCC_LIBRARY_SOURCE} )

add_library( ${PROJECT_NAME} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  PREFIX ""
                  SUFFIX ""
                  FOLDER plugins/devices
)


target_link_libraries( ${PROJECT_NAME} 	${SACK_LIBRARIES} )

INSTALL( TARGETS ${PROJECT_NAME} 
	RUNTIME DESTINATION bin/plugins
	LIBRARY DESTINATION bin/plugins
)


