PROJECT( "Dekware.Deploy" )

configure_file( deploy.cm.cm ${CMAKE_BINARY_DIR}/deploy.cm.tmp @ONLY )
include( ${CMAKE_BINARY_DIR}/deploy.cm.tmp )


if( NOT __ANDROID__ )
  configure_file( deploy.cm.c ${CMAKE_BINARY_DIR}/deploy.c.tmp )
  EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_BINARY_DIR}/deploy.c.tmp ${CMAKE_BINARY_DIR}/deploy.c )
  ADD_EXECUTABLE( ${PROJECT_NAME} ${CMAKE_BINARY_DIR}/deploy.c )
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER install )


  install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . )
endif( NOT __ANDROID__ )


