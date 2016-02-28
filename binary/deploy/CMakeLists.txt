set( PROJECT "Sack.Binary.Deploy" )

if( EXISTS ${CMAKE_SOURCE_DIR}/../.hgtags )
  FILE( STRINGS ${CMAKE_SOURCE_DIR}/../.hgtags TMP_tags )
  LIST( GET TMP_tags -1 TMP_TAG )
  STRING( REPLACE " " ";" NEWLIST ${TMP_TAG} )
  LIST( GET NEWLIST -1 CURRENT_REPO_REVISION )
endif()

configure_file( deploy.cm.cm ${CMAKE_BINARY_DIR}/deploy.cm.tmp @ONLY )
include( ${CMAKE_BINARY_DIR}/deploy.cm.tmp )



macro( build_package_include )
   file( STRINGS ${CMAKE_INSTALL_PREFIX}/CMakePackage LINES )
   #file( READ ${CMAKE_INSTALL_PREFIX}/CMakePackage LINES )
   file( WRITE ${CMAKE_BINARY_DIR}/deploy.package.h " char * package = \n" )
   foreach( LINE ${LINES} )
   	STRING( REPLACE "\\" "\\\\" LINE "${LINE}" )
   	STRING( REPLACE "\"" "\\\"" LINE "${LINE}" )
   	STRING( REPLACE "\;" "\\\;" LINE "${LINE}" )
        STRING( REPLACE "${CMAKE_INSTALL_PREFIX}" "@@@" LINE "${LINE}" )
   	file( APPEND ${CMAKE_BINARY_DIR}/deploy.package.h \t\" "${LINE}" \\n\"\n )
   endforeach( LINE )
   file( APPEND ${CMAKE_BINARY_DIR}/deploy.package.h \t\; )
endmacro( build_package_include )

build_package_include()
  

if( __ANDROID__ OR UNIX )
  configure_file( deploy.cm.cm ${CMAKE_BINARY_DIR}/deploy.cm.tmp @ONLY )
  include( ${CMAKE_BINARY_DIR}/deploy.cm.tmp )
else( __ANDROID__ OR UNIX )
  configure_file( deploy.cm.c ${CMAKE_BINARY_DIR}/deploy.c.tmp )
  EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_BINARY_DIR}/deploy.c.tmp ${CMAKE_BINARY_DIR}/deploy.c )
  sack_add_executable( ${PROJECT} DEPLOY ${CMAKE_BINARY_DIR}/deploy.c )
  SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES FOLDER install )

  if( WIN32 )
  	# On Windows platforms, the dynamic libs should
  	# go in the same dir as the executables.
  	install( TARGETS ${PROJECT}
  	        RUNTIME DESTINATION .
  )
  else( WIN32 )
  # should deploy differently for linux. otherwise we'll end up with bin in /
  	install( TARGETS ${PROJECT}
  	        RUNTIME DESTINATION .
  	)
  endif( WIN32 )
endif( __ANDROID__ OR UNIX )

add_subdirectory( info )
