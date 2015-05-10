set( CMAKE_INSTALL_MESSAGE "LAZY" )

set_property(GLOBAL PROPERTY USE_FOLDERS On)

cmake_minimum_required(VERSION 2.8)

if( NOT SACK_SDK_ROOT_PATH )
  GET_FILENAME_COMPONENT(SACK_SDK_ROOT_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Freedom Collective\\SACK;Install_Dir]" ABSOLUTE CACHE)
else( NOT SACK_SDK_ROOT_PATH )
  set(SACK_SDK_ROOT_PATH ${SACK_SDK_ROOT_PATH} CACHE STRING "SACK SDK Root" )
endif( NOT SACK_SDK_ROOT_PATH )

include( ${SACK_SDK_ROOT_PATH}/CMakePackage )
INSTALL_SACK( . )

PROJECT( CrackTheCodeLoader )

include_directories( ${SACK_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SACK_LIBRARY_DIR} )

sack_add_executable( crack_the_code_loader WIN32 program.c resources.rc  )
target_link_libraries( crack_the_code_loader ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )
INSTALL( TARGETS crack_the_code_loader RUNTIME DESTINATION . )

	
INSTALL( FILES interface.conf noswipes.jpg standby.jpg sql.config tnsnames.ora DESTINATION . )

INSTALL( FILES c:/tools/pkzip25.exe 
	M:/c-system-abstraction-component-gui.external/mysql/mysql-5.4-minimal.zip  
	M:/c-system-abstraction-component-gui.external/mysql/mysql-connector-odbc-5.1.8-win32.msi 
        DESTINATION mysql )
INSTALL( FILES 	setup.bat
	setup.sql
	DESTINATION . )


# get the libraries..
SET( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP 1 )
if( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES 1 )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY 1 )
endif( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )

INCLUDE(InstallRequiredSystemLibraries)
INSTALL( FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${BINARY_OUTPUT_DIR} )


SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Crack The Code")
SET(CPACK_PACKAGE_VENDOR "Freedom Collective")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/description.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/liscense.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_NSIS_PACKAGE_NAME "Crack The Code" )
if( SUPPORTS_PARALLEL_BUILD_TYPE )
SET( VERSION_TYPE ${CMAKE_INSTALL_CONFIG_TYPE} )
else( SUPPORTS_PARALLEL_BUILD_TYPE )
SET( VERSION_TYPE ${CMAKE_BUILD_TYPE} )
endif( SUPPORTS_PARALLEL_BUILD_TYPE )

#SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons\\\\128.png")
  set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\ico100.ico")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "\$INSTDIR\\\\crack_the_code_loader.exe" )
  SET(CPACK_NSIS_DISPLAY_NAME "Crack The Code Loader")
  SET(CPACK_NSIS_HELP_LINK "https:\\\\\\\\nosupport\\\\")
  SET(CPACK_NSIS_URL_INFO_ABOUT "https:\\\\\\\\nosupport\\\\")
  SET(CPACK_NSIS_CONTACT "support@nosupport.com")
  SET(CPACK_NSIS_CREATE_ICONS_EXTRA "SetOutPath \$INSTDIR\nCreateShortCut '\$DESKTOP\\\\CrackTheCode.lnk' '\$INSTDIR\\\\crack_the_code_loader.exe'" )
  SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
  		SetOutPath \$INSTDIR\\\\MySQL 
  		ExecWait '\\\"\$INSTDIR\\\\MySQL\\\\pkzip25\\\" -ext -dir mysql-5.4-minimal.zip'
		SetOutPath \$INSTDIR\\\\MySQL\\\\mysql\\\\bin
  		ExecWait '\\\"mysqld\\\" --install'
  		ExecWait '\\\"net\\\" start mysql'
  		ExecWait '\\\"cmd\\\" /c \\\"\$INSTDIR\\\\MySQL\\\\mysql-connector-odbc-5.1.8-win32.msi\\\"'
  		ExecWait '\\\"cmd\\\" /c \\\"\$INSTDIR\\\\setup.bat\\\"'

		#ExecWait '\\\"cmd\\\"'
                ")
  #SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "")
  #SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "RMDir /r \\\"\$APPDATA\\\\Karaway Entertainment\\\\chatment\\\"
  #                                        RMDir /r \\\"\$INSTDIR\\\\${BINARY_OUTPUT_DIR}\\\\tmp\\\"")
  SET(CPACK_NSIS_MODIFY_PATH OFF)
ELSE(WIN32 AND NOT UNIX)
#  SET(CPACK_STRIP_FILES "${BINARY_OUTPUT_DIR}/MyExecutable")
#  SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)
#SET(CPACK_PACKAGE_EXECUTABLES "" "My Executable")
INCLUDE(CPack)


