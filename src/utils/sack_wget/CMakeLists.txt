
cmake_minimum_required(VERSION 2.8)

GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)

PROJECT( change_me )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )


PROJECT( change_me )
set( PROJECT_OUTPUT sack_wget )

ADD_EXECUTABLE( ${PROJECT_OUTPUT} 
${FIRST_GCC_PROGRAM_SOURCE} 
  sack_wget.c
${LAST_GCC_PROGRAM_SOURCE} 
)
TARGET_LINK_LIBRARIES( ${PROJECT_OUTPUT} ${SACK_LIBRARIES} winmm.lib )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_OUTPUT} )

SET_TARGET_PROPERTIES(${PROJECT_OUTPUT} PROPERTIES
                  COMPILE_FLAGS  ""
                  COMPILE_DEFINITIONS "WINDOWS_MODE;TARGET_LABEL=${TARGET_LABEL};TARGETNAME=\"${PROJECT_OUTPUT}${CMAKE_EXECUTABLE_SUFFIX}\""
                  FOLDER "Program"
)

INSTALL_SACK( . )

