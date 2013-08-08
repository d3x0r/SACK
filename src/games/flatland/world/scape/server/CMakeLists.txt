
cmake_minimum_required(VERSION 2.8)


PROJECT( world_scape_service )

ADD_DEFINITIONS( -DWORLDSCAPE_SERVICE_PROGRAM ) 
include_directories( "../../../include"  "../../../../include")
ADD_EXECUTABLE( ${PROJECT_NAME} WIN32
  ${FIRST_GCC_PROGRAM_SOURCE}
global.c lines.c names.c sector.c service.c texture.c undo.c world.c
  ${LAST_GCC_PROGRAM_SOURCE}

)

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
)

SET( LINK_DIRECTORIES ${SACK_LIBRARY_DIR} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} )

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


PROJECT( world_scape_msg_server )

ADD_DEFINITIONS( -DWORLDSCAPE_SERVICE ) 
include_directories( "../../../include"  "../../../../include")
ADD_LIBRARY( ${PROJECT_NAME}  SHARED
  ${FIRST_GCC_LIBRARY_SOURCE}
global.c lines.c names.c sector.c service.c texture.c undo.c world.c
  ${LAST_GCC_LIBRARY_SOURCE}

)

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  PREFIX ""
)

SET( LINK_DIRECTORIES ${SACK_LIBRARY_DIR} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} )

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


