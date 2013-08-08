
if( NOT BUILD_INTERSHELL )

set( project launchpad )

set( RESOURCES launchpad.rc )
if( MINGW OR MSVC )
set( MORE_SOURCES ${RESOURCES} )
endif( MINGW OR MSVC )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    launchpad ${MORE_SOURCES}
${LAST_GCC_PROGRAM_SOURCE}
)

if( MINGW )

SET(CMAKE_RC_COMPILER_INIT "windres")
SET(CMAKE_RC_COMPILER "windres")
SET(CMAKE_RC_COMPILE_OBJECT "windres <FLAGS> <DEFINES> -O coff -o <OBJECT> <SOURCE>")
ENABLE_LANGUAGE(RC)
elseif( WATCOM )
string( REPLACE ".rc" ".res" WATCOM_RESOURCES ${RESOURCES} )
SET( CMAKE_RC_COMPILER "rc" )
string( REPLACE "/" "\\" WATCOM_PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR} )
string( REPLACE "/" "\\" WATCOM_CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/${WATCOM_RESOURCES}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCES}
                     COMMAND ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_PROJECT_BINARY_DIR}\\${WATCOM_RESOURCES} ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\${RESOURCES}
 )
ADD_CUSTOM_TARGET( "generate_foo${project}" DEPENDS ${PROJECT_BINARY_DIR}/${WATCOM_RESOURCES})

endif()


add_executable_force_source(${project} WIN32 ${BASE_SOURCES} )

if( WATCOM )
ADD_DEPENDENCIES( ${project} "generate_foo${project}" )
SET_TARGET_PROPERTIES(${project} PROPERTIES 
          LINK_FLAGS "option resource=${WATCOM_RESOURCES}")
endif( WATCOM )

SET_TARGET_PROPERTIES(${project} PROPERTIES
          COMPILE_FLAGS  "-DTARGET_LABEL=${project}" 
          COMPILE_DEFINITIONS "TARGETNAME=\"${project}${CMAKE_EXECUTABLE_SUFFIX}\""
                  FOLDER utils/launcher
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest_binary( ${project} )

add_subdirectory( portable )

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

if( altanik_is_built )
set( project launchpad.isom ) 

set(BASE_SOURCES3
${FIRST_GCC_PROGRAM_SOURCE} 
    launchpad milk_plugin
${LAST_GCC_PROGRAM_SOURCE}
)

add_library_force_source(${project} SHARED ${BASE_SOURCES3} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags} -DISHELL_PLUGIN"
                  PREFIX ""
                  SUFFIX ""
                  FOLDER utils/launcher
)
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )
add_subdirectory( frames )

endif()

