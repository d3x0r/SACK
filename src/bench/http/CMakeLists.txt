
PROJECT( http_benchmarks )

#if( NOT SACK_SDK_ROOT_PATH )
#GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
#else( NOT SACK_SDK_ROOT_PATH )
#set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
#endif( NOT SACK_SDK_ROOT_PATH )

#include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

#include_directories( ${SACK_INCLUDE_DIR} )
#LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
#INSTALL_SACK( bin )


PROJECT( client_request )

set(SOURCES
  ${FIRST_GCC_PROGRAM_SOURCE}
   client.c
  ${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}"
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )


install( TARGETS ${PROJECT_NAME} DESTINATION bin )
