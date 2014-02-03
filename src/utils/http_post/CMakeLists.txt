
set( project HttpUtils )

GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
INSTALL_SACK( bin )



project( test_http_post )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    http_post.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER tests
)


