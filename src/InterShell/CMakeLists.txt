set_property(GLOBAL PROPERTY USE_FOLDERS On)
cmake_minimum_required(VERSION 2.8)

PROJECT( InterShell )

if( MINGW ) 
# trigger no-touch
add_definitions( -DMINGW_SUX ) 
endif()

if( NOT MAKING_SACK_CORE )
  if( NOT SACK_SDK_ROOT_PATH )
    GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
  else( NOT SACK_SDK_ROOT_PATH )
    set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )

  endif( NOT SACK_SDK_ROOT_PATH )

  include( ${SACK_SDK_ROOT_PATH}/CMakePackage )

if( __ANDROID__ )
  INSTALL_SACK( lib )
else( __ANDROID__ )
  INSTALL_SACK( bin )
endif( __ANDROID__ )

  include_directories( ${SACK_INCLUDE_DIR} )
  LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )
else( NOT MAKING_SACK_CORE )
  ADD_DEFINITIONS( -DSACK_CORE_BUILD )

endif( NOT MAKING_SACK_CORE )


add_definitions( -DINTERSHELL_CORE_BUILD )

set( BUILD_INTERSHELL 1 )

add_subdirectory( ../../data __/__/data )
add_subdirectory( widgets )
INSTALL( DIRECTORY config/ DESTINATION Resources )
INSTALL( DIRECTORY frames DESTINATION Resources )

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

install_literal_product( ${PROJECT_OUTPUT} . ) 


include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}
)


SET( INTERSHELL_SDK
  intershell_export.h
  intershell_registry.h
  intershell_button.h
  pages.h
)

INSTALL( DIRECTORY images DESTINATION Resources )

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
   if( __ANDROID__ )
      install_literal_product( ${target} . )
   else( __ANDROID__ )
      install_literal_product( ${target} plugins )
   endif( __ANDROID__ )
ENDMACRO( INSTALL_PLUGIN target )

set( INSTALLED_TARGETS "" CACHE STRING "" FORCE )

add_subdirectory( ../utils/launcher/launchpad launchpad )
add_subdirectory( calendar )

if( NOT __ARM__ )
add_subdirectory( vlc_hook )
add_subdirectory( ../streamlib/InterShell __/streamlib )
  if( NOT __64__ AND NOT __CLR__ AND NOT BUILD_UNICODE )
    add_subdirectory( USBPulse100 )
  endif( NOT __64__ AND NOT __CLR__ AND NOT BUILD_UNICODE )
endif()
add_subdirectory( set_config )
if( NOT __ANDROID__ )
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
add_subdirectory( security.null )
add_subdirectory( security.sql )
add_subdirectory( games/slots)
if( NOT __CLR__ )
add_subdirectory( deploy )
endif( NOT __CLR__ )
if( WIN32 )
add_subdirectory( app_mount )
add_subdirectory( windows_shell )
add_subdirectory( crossfade_support )
endif( WIN32 )

# this has to be set before the cmakelists.txt ends... but not before other cmakelists are loaded

if( NOT __ARM__ OR __ANDROID__ )
# arm target has no console.
add_subdirectory( shell )
endif( NOT __ARM__ OR __ANDROID__ )

MACRO( InstallConfig target )
 if( __ANDROID__ )
  INSTALL( FILES ${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}InterShell{CMAKE_SHARED_LIBRARY_SUFFIX}
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin RENAME ${CMAKE_SHARED_LIBRARY_PREFIX}InterShell{CMAKE_SHARED_LIBRARY_SUFFIX} )
 else( __ANDROID__ )

  INSTALL( FILES ${CMAKE_INSTALL_PREFIX}/bin/InterShell_Window.exe
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin RENAME ${target}.exe )
  INSTALL( FILES ${CMAKE_INSTALL_PREFIX}/bin/InterShell_Window.stop.exe
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin RENAME ${target}.stop.exe )
  INSTALL( FILES ${CMAKE_SOURCE_DIR}/interface/${target}.interface.conf
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin OPTIONAL)
 endif( __ANDROID__ )
ENDMACRO( InstallConfig target )

MACRO( InstallServiceConfig target )
 INSTALL( FILES ${CMAKE_INSTALL_PREFIX}/bin/InterShell.Service.exe
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin RENAME ${target}.exe )
ENDMACRO( InstallServiceConfig target )

#InstallConfig( usbpulse100 )



if( NOT MAKING_SACK_CORE )

SET( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP 1 )
if( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES 1 )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY 1 )
endif( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )

INCLUDE(InstallRequiredSystemLibraries)
#message( "Resulting install: ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}" )
INSTALL( FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin )

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
