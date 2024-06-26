if( NOT BUILD_TESTS )

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

INSTALL_SACK( bin )
INSTALL_SACK_BINARY( systray_shell bin )

endif( NOT BUILD_TESTS )

PROJECT( field_tester )


ADD_LIBRARY( ${PROJECT_NAME} SHARED
${FIRST_GCC_LIBRARY_SOURCE} 
  fields.cpp
${LAST_GCC_LIBRARY_SOURCE} 
)
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} ${BULLET_LIBRARIES} winmm opengl32 glu32 )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} glew )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}\""
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DWINDOWS_MODE"
)
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER "tests/opengl plugin" )

configure_file( project.interface.conf.cm ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.interface.conf.tmp )
EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.interface.conf.tmp ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.interface.conf )
install( FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.interface.conf DESTINATION bin )

if( NOT BUILD_TESTS )
LIST( GET systray_shell_targets 0 shell_target )
install( FILES ${shell_target}  DESTINATION bin RENAME ${PROJECT_NAME}.exe )
endif( NOT BUILD_TESTS )

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


if( NOT BUILD_TESTS )
# get the system libraries..
SET( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP 1 )
if( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES 1 )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY 1 )
endif( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )

INCLUDE(InstallRequiredSystemLibraries)
INSTALL( FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin )
endif( NOT BUILD_TESTS )

