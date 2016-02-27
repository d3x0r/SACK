
cmake_minimum_required(VERSION 2.8)



PROJECT( world_scape_client )

#ADD_DEFINITIONS( -DWORLD_CLIENT_LIBRARY ) 
include_directories( "../../../include"  "../../../../include")
ADD_LIBRARY( ${PROJECT_NAME} SHARED ${FIRST_GCC_LIBRARY_SOURCE}
  world_scape_client.cpp global.cpp lines.cpp names.cpp sector.cpp texture.cpp undo.cpp world.cpp 
  ${LAST_GCC_LIBRARY_SOURCE}
  )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" PREFIX ""
)

SET( LINK_DIRECTORIES ${SACK_LIBRARY_DIR} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} message_servicepp)

if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin                     
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
else( WIN32 )
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )

