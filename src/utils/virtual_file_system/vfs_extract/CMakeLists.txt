set( PROJECT sack_vfs_extracter )


if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
    SET( EXTRACTER_MORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/extracter.manifest )
    STRING( REPLACE "/" "\\" MT_EXTRACTER_MORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/extracter.manifest )

    #SET( EXTRACTER_LINK_FLAGS "/MANIFEST:NO" )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )

if( MINGW OR MSVC )
    set( MORE_RESOURCES sfx.rc )
endif( MINGW OR MSVC )

if( MINGW )
	if( NOT HOST_UNIX )
		SET(CMAKE_RC_COMPILER_INIT "windres")
		SET(CMAKE_RC_COMPILER "windres")
		SET(CMAKE_RC_COMPILE_OBJECT "windres <FLAGS> <DEFINES> -O coff -o <OBJECT> <SOURCE>")
		ENABLE_LANGUAGE(RC)
	endif( NOT HOST_UNIX )
elseif( WATCOM )
    string( REPLACE ".rc" ".res" WATCOM_RESOURCES ${RESOURCES} )
    SET( CMAKE_RC_COMPILER "rc" )
    string( REPLACE "/" "\\" WATCOM_CMAKE_CURRENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} )
    string( REPLACE "/" "\\" WATCOM_CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
    add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${WATCOM_RESOURCES}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCES}
                     COMMAND ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_CMAKE_CURRENT_BINARY_DIR}\\${WATCOM_RESOURCES} ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\${RESOURCES}
     )
    ADD_CUSTOM_TARGET( "generate_foo${project}" DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${WATCOM_RESOURCES})

endif()

set(BASE_SOURCES
    vfs_extracter.c ${EXTRACTER_MORE_SOURCES} ${MORE_RESOURCES}
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

if( MSVC )
    SET( EXTRACTER_LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'" )
endif( MSVC )

if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
STRING ( REPLACE "/" "\\" THISDIR ${CMAKE_CURRENT_BINARY_DIR} )
add_custom_command(TARGET
                     ${PROJECT}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${MT_EXTRACTER_MORE_SOURCES}\" -outputresource:\"${THISDIR}\\${PROJECT}.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )


SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
		LINK_FLAGS "${EXTRACTER_LINK_FLAGS}"
                  FOLDER utils/vfs
)


target_link_libraries( ${PROJECT} sack_vfs.module ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )


ExternalProject_Add( ${PROJECT}_portable 
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/portable
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
		-DSACK_BASE=${SACK_BASE}
		-DPLATFORM_LIBRARIES=${PLATFORM_LIBRARIES}
		-DPLATFORM_DEFINES=${PLATFORM_DEFINES}
		-DSHA1_SOURCES=${SHA1_SOURCES}
		-DSHA2_SOURCES=${SHA2_SOURCES}
)
#add_subdirectory( portable )
