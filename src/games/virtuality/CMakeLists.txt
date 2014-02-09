

GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SACK;Install_Dir]" ABSOLUTE CACHE)

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

if( __ANDROID__ )
ADD_DEFINITIONS( -DUSE_GLES2 )
endif( __ANDROID__ )


set( PROJECT virtuality )
ADD_DEFINITIONS( -DMAKE_RCOORD_SINGLE )

ADD_DEFINITIONS( -DVIRTUALITY_LIBRARY_SOURCE ) 
include_directories( "../include" )
ADD_LIBRARY( ${PROJECT} SHARED editor.cpp key.cpp object.cpp plane.cpp shapes.cpp view.cpp )

if( WIN32 )
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} opengl32
glu32 winmm )
else( WIN32 )
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )
endif( WIN32 )

if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${PROJECT}
	        RUNTIME DESTINATION bin                     
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
else( WIN32 )
	install( TARGETS ${PROJECT}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )


