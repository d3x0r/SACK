
cmake_minimum_required(VERSION 2.8)

project( service_test.portable )

if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
    SET( EXTRACTER_MORE_SOURCES ${CURRENT_CMAKE_SOURCE_DIR}/extracter.manifest )
    #SET( EXTRACTER_LINK_FLAGS "/MANIFEST:NO" )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )


add_subdirectory( "../../../timerlib.build" minlib )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    ../service_stub.c
    ../service_test.c
${LAST_GCC_PROGRAM_SOURCE}
)

if( MSVC )
    SET( EXTRACTER_LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'" )
endif( MSVC )

sack_add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS "__STATIC__" )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} timerlib )

target_link_libraries( ${PROJECT_NAME} ws2_32 )
install_default_dest( ${PROJECT_NAME} )

