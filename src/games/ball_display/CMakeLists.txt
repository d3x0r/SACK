cmake_minimum_required(VERSION 2.8)

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )
if( NOT SACK_BINARY_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_BINARY_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\Sack.Binaries;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_BINARY_SDK_ROOT_PATH )
set(SACK_BINARY_SDK_ROOT_PATH ${SACK_BINARY_SDK_ROOT_PATH} CACHE STRING "SACK Binary SDK Root" )
endif( NOT SACK_BINARY_SDK_ROOT_PATH )


include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
include( ${SACK_BINARY_SDK_ROOT_PATH}/CMakePackage )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

INSTALL_SACK( bin )
INSTALL_SACK_BINARY( systray_shell bin )

PROJECT( ball_display )


include_directories( ../include )

ADD_LIBRARY( ${PROJECT_NAME} SHARED
${FIRST_GCC_LIBRARY_SOURCE} 
  land.cpp
${LAST_GCC_LIBRARY_SOURCE} 
)
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} winmm.lib opengl32.lib glu32.lib )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DWINDOWS_MODE"
)

install( FILES NewBallTextGrid.png DESTINATION bin )
LIST( GET systray_shell_targets 0 shell_target )
install( FILES ${shell_target}  DESTINATION bin RENAME ball_display.exe )
install( FILES ball_display.interface.conf DESTINATION bin )

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

add_subdirectory( ../virtuality virtuality )
