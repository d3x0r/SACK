

GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SACK;Install_Dir]" ABSOLUTE CACHE)

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

if( __ANDROID__ )
ADD_DEFINITIONS( -DUSE_GLES2 )
endif( __ANDROID__ )


ADD_DEFINITIONS( -DVIRTUALITY_LIBRARY_SOURCE ) 
include_directories( "../include" )

set( PROJECT virtuality )
ADD_LIBRARY( ${PROJECT} SHARED editor.cpp key.cpp object.cpp plane.cpp shapes.cpp view.cpp )
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_DEFINITIONS "MAKE_RCOORD_SINGLE"
                  #LINK_FLAGS "${extra_link_flags}"
)

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


set( PROJECT virtualityd )
ADD_LIBRARY( ${PROJECT} SHARED editor.cpp key.cpp object.cpp plane.cpp shapes.cpp view.cpp )
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )

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

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_DEFINITIONS ""
                  #LINK_FLAGS "${extra_link_flags}"
)

