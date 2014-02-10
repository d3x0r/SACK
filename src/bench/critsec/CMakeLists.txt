cmake_minimum_required(VERSION 2.8)


project( testcritsec )

GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
INSTALL_SACK( bin )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    testcritsec.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES})
install_default_dest( ${PROJECT_NAME} )

