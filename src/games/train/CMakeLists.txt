
cmake_minimum_required(VERSION 3.20)

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/share/SACK/build/CMakePackage )

INSTALL_SACK( bin )
include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
		
PROJECT( train )

ADD_EXECUTABLE( ${PROJECT_NAME} WIN32
${FIRST_GCC_PROGRAM_SOURCE} 
  editor.c key.c object.c plane.c train.c view.c
${LAST_GCC_PROGRAM_SOURCE} 
)
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} winmm.lib opengl32.lib glu32.lib )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DWINDOWS_MODE"
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

