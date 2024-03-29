cmake_minimum_required(VERSION 3.20)

if( NOT MAKING_SACK_CORE )
if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

if( NOT INTERSHELL_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(INTERSHELL_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\InterShell;Install_Dir]" ABSOLUTE CACHE)
else( NOT INTERSHELL_SDK_ROOT_PATH )
set(INTERSHELL_SDK_ROOT_PATH ${INTERSHELL_SDK_ROOT_PATH} CACHE STRING "SACK InterShell SDK Root" )
endif( NOT INTERSHELL_SDK_ROOT_PATH )

else( NOT MAKING_SACK_CORE )
  include( ../../../makefiles/cmake/DefaultInstall.cmake )
  set( SACK_INCLUDE_DIR ../../../include )
  set( INTERSHELL_INCLUDE_DIR ../../InterShell.stable )
  set( SACK_LIBRARIES bag bag.psi bag.externals )
endif( NOT MAKING_SACK_CORE )



project( Dekware )

SET(CMAKE_SKIP_BUILD_RPATH  OFF)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
SET(CMAKE_INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../..")

#set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)
#set( CMAKE_RULE_MESSAGES OFF )
set( CMAKE_INSTALL_REMOVE_ENVIRONMENT_RPATH OFF )

set_property(GLOBAL PROPERTY USE_FOLDERS On)
set( CMAKE_INSTALL_MESSAGE "LAZY" )

if( NOT MAKING_SACK_CORE )
  include( ${SACK_SDK_ROOT_PATH}/share/SACK/build/CMakePackage )
  include( ${INTERSHELL_SDK_ROOT_PATH}/share/SACK/build/InterShell/CMakePackage )

  if( NOT BUILD_INTO_SACK_INSTALL )
    INSTALL_SACK( ${CMAKE_INSTALL_PREFIX} )
    INSTALL_INTERSHELL( ${CMAKE_INSTALL_PREFIX} ) # includes a bin and resources at ./
  endif( NOT BUILD_INTO_SACK_INSTALL )

endif( NOT MAKING_SACK_CORE )


if( NOT DATA_INSTALL_PREFIX )
	SET( DATA_INSTALL_PREFIX share/SACK )
endif()

if( CMAKE_BUILD_TYPE MATCHES "[dD]ebug"  )
  add_definitions( -D_DEBUG )
endif()

#include( makefiles/cmake/DefaultInstall.cmake )

include_directories( ${SACK_INCLUDE_DIR} )
include_directories( ${INTERSHELL_INCLUDE_DIR} )

LINK_DIRECTORIES( ${INTERSHELL_LIBRARY_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

# add dekware common includes
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )


install(DIRECTORY scripts DESTINATION ${DATA_INSTALL_PREFIX}/dekware )

macro( ADD_PLUGIN target group  )
	add_library( ${target} SHARED ${FIRST_GCC_LIBRARY_SOURCE} ${ARGN} ${LAST_GCC_LIBRARY_SOURCE} )
	string( REPLACE "." "_" TARGET_LABEL ${target} )

  if( __ANDROID__ )
        set( MOREDEFS TARGETNAME=\"${target}\" )
  else( __ANDROID__ )
        set( MOREDEFS TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${target}${CMAKE_SHARED_LIBRARY_SUFFIX}\" )
  endif( __ANDROID__ )

	SET_TARGET_PROPERTIES(${target} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags} -D__DEKWARE_PLUGIN__"
                  COMPILE_DEFINITIONS ${MOREDEFS}
                  INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../..:$ORIGIN/../applicationCore"
                  FOLDER plugins/${group}
	)
	if( NOT __ANDROID__ )
		SET_TARGET_PROPERTIES(${target} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
		)
	endif( NOT __ANDROID__ )
	target_link_libraries( ${target} ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )
	INSTALL( TARGETS ${target}
		RUNTIME DESTINATION ${COMPILED_INSTALL_PREFIX}/plugins
		LIBRARY DESTINATION ${COMPILED_INSTALL_PREFIX}/plugins
	)

endmacro( ADD_PLUGIN )


add_subdirectory( deploy )
add_subdirectory( runner )
add_subdirectory( syscore )
add_subdirectory( character )
add_subdirectory( term )
add_subdirectory( trigger )
add_subdirectory( file )
add_subdirectory( timer )
add_subdirectory( system )
add_subdirectory( filters )
add_subdirectory( psicon )
add_subdirectory( dialog )
add_subdirectory( math )
add_subdirectory( cards )
add_subdirectory( include )
add_subdirectory( intershell_plugin )
#add_subdirectory( odbc )
#add_subdirectory( xml )

#add_subdirectory( spider )
#add_subdirectory( video )

# extend to get brains and 3D objects again...
if( NOT __ANDROID__ AND NOT MINGW )
add_subdirectory( ../automaton automaton)
include_directories( ../automaton/brain )
include_directories( ../automaton/board )
add_subdirectory( ../virtuality virtuality_library )
add_subdirectory( virtuality )
endif( NOT __ANDROID__ AND NOT MINGW )

install( DIRECTORY defaults/ DESTINATION ${DATA_INSTALL_PREFIX}/conf )

# get the libraries..
#SET( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP 1 )
#if( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
#SET( CMAKE_INSTALL_DEBUG_LIBRARIES 1 )
#SET( CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY 1 )
#endif( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )

#INCLUDE(InstallRequiredSystemLibraries)
##message( "Resulting install: ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}" )
#INSTALL( FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin )

#add_subdirectory( info )
