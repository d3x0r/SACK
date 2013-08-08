
cmake_minimum_required(VERSION 2.8)

project( pscards )

add_library( ${PROJECT_NAME} SHARED ../cards ../poker )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS  "CARDS_SOURCE"
                  FOLDER plugins
)

target_link_libraries( ${PROJECT_NAME} 
	${SACK_LIBRARIES}
)

INSTALL( TARGETS ${PROJECT_NAME} 
	RUNTIME DESTINATION bin/plugins
	LIBRARY DESTINATION bin/plugins
ARCHIVE DESTINATION lib )


