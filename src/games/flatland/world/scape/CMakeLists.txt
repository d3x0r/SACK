
cmake_minimum_required(VERSION 2.8)



PROJECT( world_scape_client_library )

#ADD_DEFINITIONS( -DWORLD_CLIENT_LIBRARY ) 
include_directories( "../../include"  "../../../include")
ADD_LIBRARY( ${PROJECT_NAME} SHARED global.c lines.c names.c sector.c texture.c undo.c world.c )
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

