PROJECT( intershell_deploy )


if( EXISTS ${CMAKE_SOURCE_DIR}/../../.hgtags )
  FILE( STRINGS ${CMAKE_SOURCE_DIR}/../../.hgtags TMP_tags )
  LIST( GET TMP_tags -1 TMP_TAG )
  STRING( REPLACE " " ";" NEWLIST ${TMP_TAG} )
  LIST( GET NEWLIST -1 CURRENT_REPO_REVISION )
else()
  set( CURRENT_REPO_REVISION "??" )
endif()

message( "InterShell revision: ${CURRENT_REPO_REVISION}" )

  configure_file( deploy.cm.cm ${CMAKE_BINARY_DIR}/deploy.cm.tmp @ONLY )
  include( ${CMAKE_BINARY_DIR}/deploy.cm.tmp )


macro( build_package_include )
   file( STRINGS ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_PREFIX}/build/InterShell/CMakePackage LINES )
   #file( READ ${CMAKE_INSTALL_PREFIX}/CMakePackage LINES )
   file( WRITE ${CMAKE_BINARY_DIR}/deploy.package.tmp.h " char * package = \n" )
   foreach( LINE ${LINES} )
   	STRING( REPLACE "\\" "\\\\" LINE "${LINE}" )
   	STRING( REPLACE "\"" "\\\"" LINE "${LINE}" )
   	STRING( REPLACE "\;" "\\\;" LINE "${LINE}" )
        STRING( REPLACE "${CMAKE_INSTALL_PREFIX}" "@@@" LINE "${LINE}" )
   	file( APPEND ${CMAKE_BINARY_DIR}/deploy.package.tmp.h \t\" "${LINE}" \\n\"\n )
   endforeach( LINE )
   file( APPEND ${CMAKE_BINARY_DIR}/deploy.package.tmp.h \t\; )
   EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_BINARY_DIR}/deploy.package.tmp.h ${CMAKE_BINARY_DIR}/deploy.package.h )
endmacro( build_package_include )

build_package_include()
  
if( NOT __ANDROID__ AND NOT UNIX )
  configure_file( deploy.cm.c ${CMAKE_BINARY_DIR}/deploy.c.tmp )
  EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_BINARY_DIR}/deploy.c.tmp ${CMAKE_BINARY_DIR}/deploy.c )
  add_executable_force_source( ${PROJECT_NAME} ${CMAKE_BINARY_DIR}/deploy.c )
  target_link_libraries( ${PROJECT_NAME} advapi32 )
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Install and Deploy" )

  if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	#install( TARGETS ${PROJECT_NAME}
	#        RUNTIME DESTINATION .
   #)
  else( WIN32 )
  # should deploy differently for linux. otherwise we'll end up with bin in /
	#install( TARGETS ${PROJECT_NAME}
	#        RUNTIME DESTINATION . 
	#)
  endif( WIN32 )
endif( NOT __ANDROID__ AND NOT UNIX )

add_subdirectory( info )

