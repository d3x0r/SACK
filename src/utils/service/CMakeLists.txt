
cmake_minimum_required(VERSION 2.8)

project( service_test )

if( NOT SACK_SDK_ROOT_PATH )
	GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
	include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
	include_directories( ${SACK_INCLUDE_DIR} )
	LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
	INSTALL_SACK( . )
endif( NOT SACK_SDK_ROOT_PATH )

if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
    SET( SERVICE_MORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/service.manifest )
    #SET( EXTRACTER_LINK_FLAGS "/MANIFEST:NO" )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
if( MSVC )
    SET( SERVICE_LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'" )
endif( MSVC )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    service_test.c
    ${SERVICE_MORE_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
		LINK_FLAGS "${SERVICE_LINK_FLAGS}"
                  FOLDER utils
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_default_dest( ${PROJECT_NAME} )

add_subdirectory( portable )
