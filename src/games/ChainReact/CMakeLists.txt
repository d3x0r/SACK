set_property(GLOBAL PROPERTY USE_FOLDERS On)
set( CMAKE_INSTALL_MESSAGE "LAZY" )

cmake_minimum_required(VERSION 3.20)

if( NOT SACK_SDK_ROOT_PATH )
GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/share/SACK/build/CMakePackage )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

if( __ANDROID__ )
INSTALL_SACK( libs/${ANDROID_TARGET_CPU} )
else( __ANDROID__ )
INSTALL_SACK( bin )
endif( __ANDROID__ )

PROJECT( ChainReaction )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

# this is a bad thing to override.
set( BINARY_OUTPUT_DIR bin )
add_program( ${PROJECT_NAME} WIN32
${FIRST_GCC_PROGRAM_SOURCE} 
  chain.c logic.c player.c 
${LAST_GCC_PROGRAM_SOURCE} 
)


if( __ANDROID__ )
install( FILES 
	1atom.gif 357mag.wav atom2.gif atom2.png atom2-24.png grid1.jpg cursor.jpg plink.wav slwhist.wav
        	DESTINATION assets
                )
else( __ANDROID__ )
install( FILES 
	1atom.gif 357mag.wav atom2.gif atom2.png atom2-24.png grid1.jpg cursor.jpg plink.wav slwhist.wav
        	DESTINATION bin
                )
endif( __ANDROID__ )

  INSTALL( FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.interface.conf
	DESTINATION bin )

# get the libraries..
SET( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP 1 )
if( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES 1 )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY 1 )
endif( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )

INCLUDE(InstallRequiredSystemLibraries)
INSTALL( FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${BINARY_OUTPUT_DIR} )

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Chain Reaction")
SET(CPACK_PACKAGE_VENDOR "Freedom Collective")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/description.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/liscense.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "999")

if( SUPPORTS_PARALLEL_BUILD_TYPE )
SET( VERSION_TYPE ${CMAKE_INSTALL_CONFIG_TYPE} )
else( SUPPORTS_PARALLEL_BUILD_TYPE )
SET( VERSION_TYPE ${CMAKE_BUILD_TYPE} )
endif( SUPPORTS_PARALLEL_BUILD_TYPE )

	SET(CPACK_PACKAGE_VERSION_PATCH "485-${CMAKE_GENERATOR}-${VERSION_TYPE}")

#SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/doc\\\\chainreact-icon.jpg")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "${BINARY_OUTPUT_DIR}\\\\atom2.png")
  SET(CPACK_NSIS_DISPLAY_NAME "Chain Reaction ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\code.google.com\\\\p\\\\c-system-abstraction-component-gui")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\code.google.com\\\\p\\\\c-system-abstraction-component-gui")
  SET(CPACK_NSIS_CONTACT "d3ck0r@gmail.com")
  SET(CPACK_NSIS_CREATE_ICONS_EXTRA "SetOutPath \\\"\$INSTDIR\\\\${BINARY_OUTPUT_DIR}\\\"\n CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Chain Reaction.lnk' '\$INSTDIR\\\\${BINARY_OUTPUT_DIR}\\\\ChainReaction.exe'" )
  SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete \$INSTDIR\\\\*\nDelete \$INSTDIR\\\\bin\\\\*" )
  #SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"\$INSTDIR\\\\${BINARY_OUTPUT_DIR}\\\\ChainReaction.exe\\\"'")
  #SET(CPACK_NSIS_MODIFY_PATH ON)
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_STRIP_FILES "${BINARY_OUTPUT_DIR}/MyExecutable")
  SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)
#SET(CPACK_PACKAGE_EXECUTABLES "" "My Executable")
INCLUDE(CPack)


