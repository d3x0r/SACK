
cmake_minimum_required(VERSION 2.8)

GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SACK;Install_Dir]" ABSOLUTE CACHE)

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )



PROJECT( virtuality )

ADD_DEFINITIONS( -DVIRTUALITY_LIBRARY_SOURCE ) 
include_directories( "../include" )
ADD_LIBRARY( ${PROJECT_NAME} SHARED editor.cpp key.cpp object.cpp plane.cpp shapes.cpp view.cpp )

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} opengl32
glu32 winmm )

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


