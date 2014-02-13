cmake_minimum_required(VERSION 2.8)

project( HttpUtils )

GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
INSTALL_SACK( bin )



project( http_server )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    http_server.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
        FOLDER servers
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES})
install_default_dest( ${PROJECT_NAME} )


