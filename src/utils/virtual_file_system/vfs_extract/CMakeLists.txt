set( PROJECT sack_vfs_extracter )


if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
    SET( EXTRACTER_MORE_SOURCES ${CURRENT_CMAKE_SOURCE_DIR}/extracter.manifest )
    #SET( EXTRACTER_LINK_FLAGS "/MANIFEST:NO" )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )


set(BASE_SOURCES
    vfs_extracter.c memory_dll_loader.c ${EXTRACTER_MORE_SORUCES}
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

if( MSVC )
    SET( EXTRACTER_LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'" )
endif( MSVC )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
		LINK_FLAGS "${EXTRACTER_LINK_FLAGS}"
                  FOLDER utils/vfs
)

if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

message( "something is ${CMAKE_CURRENT_BINARY_DIR}\\sack_vfs_extracter.exe" )
add_custom_command(TARGET
                     ${PROJECT}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\extracter.manifest\" -outputresource:\"${CMAKE_CURRENT_BINARY_DIR}\\sack_vfs_extracter.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

target_link_libraries( ${PROJECT} sack_vfs.module ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )


add_subdirectory( portable )
