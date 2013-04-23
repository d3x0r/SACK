
cmake_minimum_required(VERSION 2.8)

PROJECT( "Dekware.Deploy" )

configure_file( deploy.cm.c ${CMAKE_BINARY_DIR}/deploy.c.tmp )
EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_BINARY_DIR}/deploy.c.tmp ${CMAKE_BINARY_DIR}/deploy.c )
ADD_EXECUTABLE( ${PROJECT_NAME} ${CMAKE_BINARY_DIR}/deploy.c )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER install )


install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . )


