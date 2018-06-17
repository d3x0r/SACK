
if( NOT BUILD_INTERSHELL )

set( project launchpad )

if( NOT __CLR__ )
    # have to do better resources I guess
    set( RESOURCES launchpad.rc )
endif( NOT __CLR__ )

if( MINGW OR MSVC )
set( MORE_SOURCES ${RESOURCES} )
endif( MINGW OR MSVC )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    launchpad ${MORE_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
)

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


add_program(${project} WIN32 ${BASE_SOURCES} )

if( WATCOM )
ADD_DEPENDENCIES( ${project} "generate_foo${project}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES 
          LINK_FLAGS "option resource=${WATCOM_RESOURCES}")
endif( WATCOM )

SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils/launcher
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest_binary( ${project} )

string( REPLACE ";" " " PLATFORM_LIBRARIES_ARG "${SACK_PLATFORM_LIBRARIES}" )
string( REPLACE ";" " " PLATFORM_DEFINES_ARG "${PLATFORM_DEFINES}" )
string( REPLACE ";" "," PLATFORM_CMAKE_DEFINES_ARG "${PLATFORM_CMAKE_DEFINES}" )

ExternalProject_Add( ${project}_portable 
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/portable
	BINARY_DIR ${project}_portable
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS 1
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
		-DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE} 
		-DSACK_BASE=${SACK_BASE}
		-DPLATFORM_LIBRARIES=${PLATFORM_LIBRARIES_ARG}
		-DPLATFORM_DEFINES=${PLATFORM_DEFINES_ARG}
		-DPLATFORM_CMAKE_DEFINES=${PLATFORM_CMAKE_DEFINES_ARG}
		-DSHA1_SOURCES=${SHA1_SOURCES}
		-DSHA2_SOURCES=${SHA2_SOURCES}
		-DSHA3_SOURCES=${SHA3_SOURCES}
)
#add_subdirectory( portable )

else( NOT BUILD_INTERSHELL )

set( project launchpad.isp )

set(BASE_SOURCES2
${FIRST_GCC_LIBRARY_SOURCE}
    launchpad ishell_plugin
${LAST_GCC_LIBRARY_SOURCE}
)

include_directories( ${SACK_BASE}/src/InterShell )
add_library_force_source(${project} SHARED ${BASE_SOURCES2} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags} -DISHELL_PLUGIN"
                  FOLDER utils/launcher
)
if( NOT __ANDROID__ )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT __ANDROID__ )
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_PLUGIN( ${project} ) 
add_subdirectory( frames )

endif( NOT BUILD_INTERSHELL )


