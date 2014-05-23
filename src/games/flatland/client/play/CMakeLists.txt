
cmake_minimum_required(VERSION 2.8)



PROJECT( flatland_player )

#ADD_DEFINITIONS( -D ) 
include_directories( "../../include"  "../../../include")
ADD_EXECUTABLE( ${PROJECT_NAME} 
${FIRST_GCC_PROGRAM_SOURCE}
	play.cpp 
${LAST_GCC_PROGRAM_SOURCE}
        )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} world_scape_client virtuality )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DUSE_WORLDSCAPE_INTERFACE"
)

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


