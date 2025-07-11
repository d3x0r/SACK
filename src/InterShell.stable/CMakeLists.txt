set_property(GLOBAL PROPERTY USE_FOLDERS On)
cmake_minimum_required(VERSION 3.15)
PROJECT( InterShell )

SET(CMAKE_SKIP_BUILD_RPATH  OFF)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
SET(CMAKE_INSTALL_RPATH "\$ORIGIN")

#set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)
#set( CMAKE_RULE_MESSAGES OFF )
set( CMAKE_INSTALL_MESSAGE "LAZY" )
set( CMAKE_INSTALL_REMOVE_ENVIRONMENT_RPATH OFF )

if( NOT MAKING_SACK_CORE )
  if( NOT SACK_SDK_ROOT_PATH )
    GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
  else( NOT SACK_SDK_ROOT_PATH )
    set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )

  endif( NOT SACK_SDK_ROOT_PATH )

  include( ${SACK_SDK_ROOT_PATH}/share/SACK/build/CMakePackage )

include( GNUInstallDirs )

if( NOT BUILD_INTO_SACK_INSTALL )
  if( __ANDROID__ )
    INSTALL_SACK( lib )
  else( __ANDROID__ )

		INSTALL_SACK( ${CMAKE_INSTALL_PREFIX} )
  endif( __ANDROID__ )

endif( NOT BUILD_INTO_SACK_INSTALL )

  include_directories( ${SACK_INCLUDE_DIR} )
  LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

else( NOT MAKING_SACK_CORE )
  ADD_DEFINITIONS( -DSACK_CORE_BUILD )
  set( CMAKE_MODULE_PATH ../../makefiles/cmake )
  include( ../../makefiles/cmake/DefaultInstall.cmake )
  set( SACK_INCLUDE_DIR ../../include )
  include_directories( ${SACK_INCLUDE_DIR} )
  include_directories( ../contrib )
  set( SACK_LIBRARIES bag bag.psi bag.externals )

endif( NOT MAKING_SACK_CORE )
if( WIN32 AND WATCOM )
add_definitions( -DWIN32 )
endif( WIN32 AND WATCOM )
add_definitions( -DINTERSHELL_CORE_BUILD )

set( BUILD_INTERSHELL 1 )
add_subdirectory( ../../data __/__/data )
add_subdirectory( widgets )
INSTALL( DIRECTORY config/ DESTINATION ${DATA_INSTALL_PREFIX}/conf )
INSTALL( DIRECTORY frames/ DESTINATION ${DATA_INSTALL_PREFIX}/frames )

set( PROJECT_OUTPUT InterShell.core )

set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
banner_button.c
fileopen.c     
fonts.c        
loadsave.c     
macros.c       
main.c         
intershell_plugins.c 
page_dialog.c  
pages.c        
resname.c      
security.c     
sprites.c      
text_label.c
env_value.c
${LAST_GCC_LIBRARY_SOURCE}
)

add_library( ${PROJECT_OUTPUT} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_OUTPUT} )
SET_TARGET_PROPERTIES(${PROJECT_OUTPUT} PROPERTIES
                  DEFINE_SYMBOL INTERSHELL_SOURCE
                  COMPILE_FLAGS  ""
                  COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL};_DEFINE_INTERFACE;__NO_ANIMATION__"
                  FOLDER InterShell
)

if( NOT __ANDROID__ )
       SET_TARGET_PROPERTIES(${PROJECT_OUTPUT} PROPERTIES
                  PREFIX ""
       )
   if( NOT __CLR__ )
      SET_TARGET_PROPERTIES(${PROJECT_OUTPUT} PROPERTIES
                  SUFFIX ""
      )
   endif( NOT __CLR__ )
endif( NOT __ANDROID__ )

my_target_link_libraries( ${PROJECT_OUTPUT} sack_widgets ${SACK_LIBRARIES} )

ADD_DEFINITIONS( -DCORE_PATH="${CMAKE_INSTALL_PREFIX}/${COMPILED_INSTALL_PREFIX}/applicationCore" )
install_literal_product( ${PROJECT_OUTPUT} applicationCore ) 


include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}
)


SET( INTERSHELL_SDK
  intershell_export.h
  intershell_registry.h
  intershell_button.h
  pages.h
)

INSTALL( DIRECTORY images/ DESTINATION ${DATA_INSTALL_PREFIX}/images )

INSTALL( 
  FILES
    ${INTERSHELL_SDK}
  DESTINATION
    include/SACK/InterShell
)

if( INSTALL_TO_SACK_SDK )
INSTALL( 
  FILES
    ${INTERSHELL_SDK}
  DESTINATION
    ${SACK_BASE}/include/SACK/InterShell )
endif( INSTALL_TO_SACK_SDK )

MACRO( INSTALL_PLUGIN target )
  SET_TARGET_PROPERTIES( ${target}
    PROPERTIES
      INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../.."
  )

   if( __ANDROID__ )
      install_literal_product( ${target} . )
   else( __ANDROID__ )
      install_literal_product( ${target} plugins )
   endif( __ANDROID__ )
ENDMACRO( INSTALL_PLUGIN target )

set( INSTALLED_TARGETS "" CACHE STRING "" FORCE )

add_subdirectory( ../utils/launcher/launchpad launchpad )
add_subdirectory( calendar )

if( NOT __ARM__ AND NOT __ANDROID__)
add_subdirectory( vlc_hook )
add_subdirectory( ../streamlib/InterShell __/streamlib )
  if( NOT __64__ AND NOT __CLR__ AND NOT BUILD_UNICODE )
    #add_subdirectory( usbpulse100 )
  endif( NOT __64__ AND NOT __CLR__ AND NOT BUILD_UNICODE )
endif()
if( NOT __ANDROID__ )
  add_subdirectory( set_config )
  add_subdirectory( wshell )
  add_subdirectory( service.shell )
  add_subdirectory( stop )
  add_subdirectory( restart )
  add_subdirectory( force_show )
endif( NOT __ANDROID__ )
add_subdirectory( tasks )
add_subdirectory( page_cycle )
add_subdirectory( clock )
add_subdirectory( calculator )
add_subdirectory( widgets/keypad/keypad )
add_subdirectory( graph )
add_subdirectory( filevar )
add_subdirectory( slider )
if( NOT EMSCRIPTEN )
   # emscripten didn't port ffmpeg yet (output is only opengl)
   add_subdirectory( video_player )
endif( NOT EMSCRIPTEN )
add_subdirectory( security.null )
add_subdirectory( security.sql )
#add_subdirectory( rpi.gpio )
if( NOT __ANDROID__ AND NOT EMSCRIPTEN )
add_subdirectory( games/Slots)
add_subdirectory( games/fantasy_football)
endif( NOT __ANDROID__ AND NOT EMSCRIPTEN )
if( NOT __CLR__ )
add_subdirectory( deploy )
endif( NOT __CLR__ )
if( WIN32 )
add_subdirectory( countdown_timer )
add_subdirectory( eqube/cashball )
add_subdirectory( app_mount )
add_subdirectory( windows_shell )
add_subdirectory( crossfade_support )
endif( WIN32 )
# this has to be set before the cmakelists.txt ends... but not before other cmakelists are loaded

if( NOT __ARM__ AND NOT __ANDROID__ )
# arm target has no console.
add_subdirectory( shell )
endif( NOT __ARM__ AND NOT __ANDROID__ )

MACRO( InstallConfig target )
 if( __ANDROID__ )
  INSTALL( FILES bin/${CMAKE_SHARED_LIBRARY_PREFIX}InterShell{CMAKE_SHARED_LIBRARY_SUFFIX}
	DESTINATION bin RENAME ${CMAKE_SHARED_LIBRARY_PREFIX}InterShell{CMAKE_SHARED_LIBRARY_SUFFIX} )
 else( __ANDROID__ )

  INSTALL( FILES bin/InterShell_Window.exe
	DESTINATION bin RENAME ${target}.exe )
  INSTALL( FILES bin/InterShell_Window.stop.exe
	DESTINATION bin RENAME ${target}.stop.exe )
  INSTALL( FILES ${CMAKE_SOURCE_DIR}/interface/${target}.interface.conf
	DESTINATION bin OPTIONAL)
 endif( __ANDROID__ )
ENDMACRO( InstallConfig target )

MACRO( InstallServiceConfig target )
 INSTALL( FILES bin/InterShell.Service.exe
	DESTINATION bin RENAME ${target}.exe )
ENDMACRO( InstallServiceConfig target )

#InstallConfig( usbpulse100 )



if( NOT MAKING_SACK_CORE )


SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SACK InterShell")
SET(CPACK_PACKAGE_VENDOR "Freedom Collective")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Description.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")

if( SUPPORTS_PARALLEL_BUILD_TYPE )
SET( VERSION_TYPE ${CMAKE_INSTALL_CONFIG_TYPE} )
else( SUPPORTS_PARALLEL_BUILD_TYPE )
SET( VERSION_TYPE ${CMAKE_BUILD_TYPE} )
endif( SUPPORTS_PARALLEL_BUILD_TYPE )

	SET(CPACK_PACKAGE_VERSION_PATCH "10-${CMAKE_GENERATOR}-${VERSION_TYPE}")

#SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/images\\\\icon1.png")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\sack_bag.dll")
  SET(CPACK_NSIS_DISPLAY_NAME "SACK ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\code.google.com\\\\p\\\\c-system-abstraction-component-gui")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\code.google.com\\\\p\\\\c-system-abstraction-component-gui")
  SET(CPACK_NSIS_CONTACT "d3ck0r@gmail.com")
  SET(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Set Intershell SDK.lnk' '\$INSTDIR\\\\bin\\\\intershell_deploy.exe'" )
  SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"\$INSTDIR\\\\intershell_deploy.exe\\\"'")
  SET(CPACK_NSIS_MODIFY_PATH OFF)
ELSE(WIN32 AND NOT UNIX)
#  SET(CPACK_STRIP_FILES "bin/MyExecutable")
#  SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)
#SET(CPACK_PACKAGE_EXECUTABLES "" "My Executable")
INCLUDE(CPack)

endif( NOT MAKING_SACK_CORE )
