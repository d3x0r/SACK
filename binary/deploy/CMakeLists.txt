PROJECT( "Sack.Binary.Deploy" )

FILE( STRINGS ${CMAKE_SOURCE_DIR}/../.hgtags TMP_tags )
LIST( GET TMP_tags -1 TMP_TAG )
STRING( REPLACE " " ";" NEWLIST ${TMP_TAG} )
LIST( GET NEWLIST -1 CURRENT_REPO_REVISION )
message( "Binary revision: "${CURRENT_REPO_REVISION} )

if( __ANDROID__ )
  configure_file( deploy.cm.cm ${CMAKE_BINARY_DIR}/deploy.cm.tmp @ONLY )
  include( ${CMAKE_BINARY_DIR}/deploy.cm.tmp )
else( __ANDROID__ )
  configure_file( deploy.cm.c ${CMAKE_BINARY_DIR}/deploy.c.tmp )
  EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_BINARY_DIR}/deploy.c.tmp ${CMAKE_BINARY_DIR}/deploy.c )
  ADD_EXECUTABLE( ${PROJECT_NAME} ${CMAKE_BINARY_DIR}/deploy.c )
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER install )

  if( WIN32 )
  	# On Windows platforms, the dynamic libs should
  	# go in the same dir as the executables.
  	install( TARGETS ${PROJECT_NAME}
  	        RUNTIME DESTINATION .
  )
  else( WIN32 )
  # should deploy differently for linux. otherwise we'll end up with bin in /
  	install( TARGETS ${PROJECT_NAME}
  	        RUNTIME DESTINATION .
  	)
  endif( WIN32 )
endif( __ANDROID__ )

add_subdirectory( info )
