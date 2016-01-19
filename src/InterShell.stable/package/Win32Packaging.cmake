

set( vfs_arg_index 1 )
set( vfs_arg_list vfs_arg1 )
set( strip_arg_list strip_arg1 )
set( strip_arg1 ${INTERSHELL_SDK_ROOT_PATH}/bin/InterShell.core )

set( CORE_NAME InterShell.core )

macro( append_vfs_arg a b c )
    LIST( LENGTH vfs_arg${vfs_arg_index} len )
    #message( "len of vfs_arg${vfs_arg_index} is ${len}" )
    if( len GREATER 100 )
        math( EXPR vfs_arg_index ${vfs_arg_index}+1 )
        set( vfs_arg_list ${vfs_arg_list} vfs_arg${vfs_arg_index} )
        set( strip_arg_list ${strip_arg_list} strip_arg${vfs_arg_index} )
    endif( len GREATER 100 )
    LIST( APPEND vfs_arg${vfs_arg_index} storeas ${a} ${b} )
	if( ${c} )
        LIST( APPEND strip_arg${vfs_arg_index} ${a} )
	endif( ${c} )
endmacro( append_vfs_arg )
                                              	
FILE( GLOB sack_images ${SACK_BASE}/bin/images/* )
FILE( GLOB these_images images/* )
set( images ${sack_images} ${these_images} )
#message( "images ${images}" )
foreach( image ${images} )
  get_filename_component( image_name ${image} NAME )
  if( ${image_name} STREQUAL "dragon.vfs" )
       	#append_vfs_arg(  ${image} Images/${image_name} 0  )
  else()
	  if( ${image_name} STREQUAL "promo.wmv" )
		#append_vfs_arg(  ${image} Images/${image_name} 0  )
	  else()
	  if( ${image_name} MATCHES ".*pdn" )
		#append_vfs_arg(  ${image} Images/${image_name} 0  )
	  else()
	    get_filename_component( image_path ${image} DIRECTORY )
		append_vfs_arg( ${image} Images/${image_name} 0 )
	    #set( vfs_args ${vfs_args} storeas ${image} Images/${image_name} )
          endif()
          endif()
  endif()
endforeach( image ${images} )


FILE( GLOB fonts fonts/* )
	foreach( image ${fonts} )
  get_filename_component( image_name ${image} NAME )
    get_filename_component( image_path ${image} DIRECTORY )
	append_vfs_arg( ${image} fonts/${image_name} 0 )
    #set( vfs_args ${vfs_args} storeas ${image} fonts/${image_name} )
endforeach(  )

FILE( GLOB sounds sounds/* )
foreach( image ${sounds} )
  get_filename_component( image_name ${image} NAME )
    get_filename_component( image_path ${image} DIRECTORY )
	append_vfs_arg( ${image} sounds/${image_name} 0 )
    #set( vfs_args ${vfs_args} storeas ${image} sounds/${image_name} )
endforeach(  )

if( WIN32 )
set( MORELIBS ${SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}dl${CMAKE_SHARED_LIBRARY_SUFFIX} )
endif( WIN32 )
foreach( lib ${sack_binary_list} ${MORELIBS} )
	get_filename_component( image_name ${lib} NAME )
    get_filename_component( image_path ${lib} DIRECTORY )
	set( STOREFILE 1 )
	if( ${image_name} STREQUAL "sack_vfs.module" )
		set( STOREFILE 0 )
	endif()
	if( ${image_name} MATCHES ".pure" )
		set( STOREFILE 0 )
	endif()
	if( ${image_name} STREQUAL "glew.dll" )
		set( STOREFILE 0 )
	endif()

	if( ${STOREFILE} )
		append_vfs_arg( ${lib} ${image_name} 1 )
	endif()
endforeach()
#append_vfs_arg( $<TARGET_FILE:kwei> $<TARGET_FILE_NAME:kwei> 1 )
#append_vfs_arg( mingw/libgcc_s_dw2-1.dll libgcc_s_dw2-1.dll 0 )
append_vfs_arg( mingw/oalinst.exe oalinst.exe 0 )
if( __ANDROID__ )
	set( BINARY_PATH lib )
	append_vfs_arg( ${INTERSHELL_BASE}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}sack_widgets${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_SHARED_LIBRARY_PREFIX}sack_widgets${CMAKE_SHARED_LIBRARY_SUFFIX} 1 )
else( __ANDROID__ )
	set( BINARY_PATH bin )
	append_vfs_arg( ${INTERSHELL_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}sack_widgets${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_SHARED_LIBRARY_PREFIX}sack_widgets${CMAKE_SHARED_LIBRARY_SUFFIX} 1 )
endif( __ANDROID__ )
#append_vfs_arg( ${CMAKE_CURRENT_SOURCE_DIR}/chatment.standalone.sql.config program.sql.config 0 )
#append_vfs_arg( ${CMAKE_CURRENT_SOURCE_DIR}/chatment.standalone.interface.conf program.interface.conf 0 )
append_vfs_arg( ${SACK_BASE}/bin/interface.conf core.interface.conf 0 )
append_vfs_arg( ${CMAKE_CURRENT_SOURCE_DIR}/interface.conf interface.conf 0 )
append_vfs_arg( ${CMAKE_CURRENT_SOURCE_DIR}/sql.config sql.config 0 )


foreach( EXTRA ${SECURITY_LIBRARY_LIST} )
    get_filename_component( libname ${EXTRA} NAME )
	append_vfs_arg( ${EXTRA} ${libname} 1 )
endforeach()

FILE( GLOB PLUGIN_LIST ${INTERSHELL_SDK_ROOT_PATH}/bin/plugins/* )
foreach( EXTRA ${PLUGIN_LIST} )
    get_filename_component( libname ${EXTRA} NAME )
	append_vfs_arg( ${EXTRA} ${libname} 1 )
endforeach()

FILE( GLOB PLUGIN_LIST ${INTERSHELL_SDK_ROOT_PATH}/Resources/Images/* )
foreach( EXTRA ${PLUGIN_LIST} )
    get_filename_component( libname ${EXTRA} NAME )
	append_vfs_arg( ${EXTRA} Resources/Images/${libname} 1 )
endforeach()

FILE( GLOB PLUGIN_LIST ${INTERSHELL_SDK_ROOT_PATH}/Resources/frames/* )
foreach( EXTRA ${PLUGIN_LIST} )
    get_filename_component( libname ${EXTRA} NAME )
	append_vfs_arg( ${EXTRA} Resources/frames/${libname} 1 )
endforeach()


# get the system libraries..
SET( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP 1 )
if( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
	SET( CMAKE_INSTALL_DEBUG_LIBRARIES 1 )
	SET( CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY 1 )
    SET( LOADER \$<TARGET_FILE:sack_vfs_runner_sfx_win> )
	if( WATCOM )
		set( STRIP_COMMAND wstrip ${LOADER} )
	endif()
else()
    SET( LOADER \$<TARGET_FILE:sack_vfs_runner_sfx_win> )
    #SET( LOADER \$<TARGET_FILE:sack_vfs_runner_sfx> )
endif( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )

INCLUDE(InstallRequiredSystemLibraries)
#message( "Resulting install: ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}" )
foreach( EXTRA ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} )
    get_filename_component( libname ${EXTRA} NAME )
	append_vfs_arg( ${EXTRA} ${libname} 0 )
endforeach()


add_subdirectory( loader )

set( app_vfs vfs ${CMAKE_BINARY_DIR}/application.dat )

foreach( strip_arg ${strip_arg_list} )
	if( GCC )
    if( NOT "" STREQUAL "${${strip_arg}}" )
	    set( PRE_MORE_COMMAND ${PRE_MORE_COMMAND} COMMAND strip.exe ${${strip_arg}} "\n" )
    endif(  )
	endif( GCC )
endforeach()

foreach( vfs_arg ${vfs_arg_list} )
	set( MORE_COMMAND ${MORE_COMMAND} COMMAND ${SACK_BASE}/bin/sack_vfs_command.exe ${app_vfs} ${${vfs_arg}} "\n" )
endforeach()

if( MINGW )
	set( STRIP_COMMAND c:/tools/unix/mingw/bin/strip -s ${LOADER} )
endif()

add_custom_target( generate_application_dat #ALL
                   #OUTPUT ${CMAKE_BINARY_DIR}/application.dat
                   COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/application.dat
				   ${PRE_MORE_COMMAND} 
                   COMMAND ${CMAKE_COMMAND} -E echo ${SACK_BASE}/bin/sack_vfs_command.exe ${app_vfs} storeas ${INTERSHELL_SDK_ROOT_PATH}/bin/InterShell.core ${CORE_NAME}
                   COMMAND ${SACK_BASE}/bin/sack_vfs_command.exe ${app_vfs} storeas ${INTERSHELL_SDK_ROOT_PATH}/bin/InterShell.core ${CORE_NAME}
				   ${MORE_COMMAND} 
				   COMMAND ${SACK_BASE}/bin/sack_vfs_command.exe ${app_vfs} shrink
				   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/application.dat ${CMAKE_BINARY_DIR}/application.bak
				   #COMMAND ${SACK_BASE}/bin/sack_vfs_command.exe ${app_vfs} sign-to-header ${CMAKE_BINARY_DIR}/app-signature.h app_signature
				   #COMMAND ${SACK_BASE}/bin/sack_vfs_command.exe ${app_vfs} sign-encrypt ${CMAKE_BUILD_TYPE}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
				                   
				   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                                   
				   DEPENDS ${sack_binary_list} ${MORELIBS} 
					       # ${CMAKE_CURRENT_SOURCE_DIR}/chatment.standalone.sql.config 
					       # ${CMAKE_CURRENT_SOURCE_DIR}/chatment.standalone.interface.conf
						 ${INTERSHELL_BASE}/${BINARY_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}sack_widgets${CMAKE_SHARED_LIBRARY_SUFFIX}
						 ${SACK_BASE}/bin/interface.conf
						${SECURITY_LIBRARY_LIST}
                   )

add_custom_target( generate_application ALL
                   #OUTPUT ${CMAKE_BINARY_DIR}/application.exe
				   #COMMAND ${CMAKE_COMMAND} -E echo ${SACK_BASE}/bin/sack_vfs_command.exe append ${LOADER} ${CMAKE_BINARY_DIR}/application.dat ${CMAKE_BINARY_DIR}/InterShell.exe
   				   COMMAND ${STRIP_COMMAND}
                   COMMAND ${SACK_BASE}/bin/sack_vfs_command.exe append ${LOADER} ${CMAKE_BINARY_DIR}/application.dat ${CMAKE_BINARY_DIR}/InterShell.exe
                                   
				   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                                   
				   DEPENDS $<TARGET_FILE:sack_vfs_runner_sfx> 
                                   		$<TARGET_FILE:sack_vfs_runner_sfx_win> 
                                                ${CMAKE_BINARY_DIR}/application.dat 
                   )
#ADD_CUSTOM_TARGET( generate_application DEPENDS ${CMAKE_BINARY_DIR}/application.dat )
ADD_DEPENDENCIES( sack_vfs_runner_win_min generate_application_dat )
ADD_DEPENDENCIES( sack_vfs_runner_min generate_application_dat )
ADD_DEPENDENCIES( sack_vfs_runner_sfx generate_application_dat )
ADD_DEPENDENCIES( sack_vfs_runner_sfx_win generate_application_dat )

INSTALL( FILES ${CMAKE_BINARY_DIR}/InterShell.exe DESTINATION bin )


#INSTALL( PROGRAMS mingw/libgcc_s_dw2-1.dll DESTINATION bin )
#INSTALL( PROGRAMS mingw/oalinst.exe DESTINATION bin )
 
INSTALL( FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin )
#INSTALL( DIRECTORY ${SACK_BASE}/bin/images DESTINATION bin )


SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "InterShell")
SET(CPACK_PACKAGE_VENDOR "Karaway Entertainment")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/description.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/liscense.txt")
SET(CPACK_NSIS_PACKAGE_NAME "InterShell" )
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_NSIS_PACKAGE_NAME} )
if( SUPPORTS_PARALLEL_BUILD_TYPE )
SET( VERSION_TYPE ${CMAKE_INSTALL_CONFIG_TYPE} )
else( SUPPORTS_PARALLEL_BUILD_TYPE )
SET( VERSION_TYPE ${CMAKE_BUILD_TYPE} )
endif( SUPPORTS_PARALLEL_BUILD_TYPE )

#SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/images\\\\InterShell_Logo.png")
  set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/images\\\\InterShellLogo.ico")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "\$INSTDIR\\\\bin\\\\Intershell_Package.exe" )
  SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON )
  SET(CPACK_NSIS_DISPLAY_NAME "InterShell")
  SET(CPACK_NSIS_HELP_LINK "https:\\\\\\\\no-webpage.com\\\\")
  SET(CPACK_NSIS_URL_INFO_ABOUT "https:\\\\\\\\no-webpage.com\\\\")
  SET(CPACK_NSIS_CONTACT "support@no-webpage.com")
  SET(CPACK_NSIS_CREATE_ICONS_EXTRA "SetOutPath \$INSTDIR\\\\bin\nCreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\InterShell.lnk' '\$INSTDIR\\\\${BINARY_OUTPUT_DIR}\\\\Intershell.exe'"
                                   "\nCreateShortCut '\$DESKTOP\\\\InterShell.lnk' '\$INSTDIR\\\\${BINARY_OUTPUT_DIR}\\\\InterShell.exe'"
								 )
  SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"\$INSTDIR\\\\${BINARY_OUTPUT_DIR}\\\\oalinst.exe /silent\\\"'")
  #SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "")
  SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "RMDir /r \\\"\$APPDATA\\\\Freedom Collective\\\\InterShell\\\"
                                          RMDir /r \\\"\$INSTDIR\\\\${BINARY_OUTPUT_DIR}\\\\tmp\\\"
                                          RMDir /r \\\"\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\"
										  Delete \\\"\$DESKTOP\\\\InterShell.lnk\\\"
										  "
										  )
  SET(CPACK_NSIS_MODIFY_PATH OFF)
ELSE(WIN32 AND NOT UNIX)
#  SET(CPACK_STRIP_FILES "${BINARY_OUTPUT_DIR}/MyExecutable")
#  SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)
#SET(CPACK_PACKAGE_EXECUTABLES "" "My Executable")
INCLUDE(CPack)


