PROJECT( sack_deploy )

FILE( STRINGS ${CMAKE_SOURCE_DIR}/.hgtags TMP_tags )
LIST( GET TMP_tags -1 TMP_TAG )
STRING( REPLACE " " ";" NEWLIST ${TMP_TAG} )
LIST( GET NEWLIST -1 CURRENT_REPO_REVISION )
 
message( "Sack revision: "${CURRENT_REPO_REVISION} )

configure_file( deploy.cm.cm ${CMAKE_BINARY_DIR}/deploy.cm.tmp @ONLY )
include( ${CMAKE_BINARY_DIR}/deploy.cm.tmp )

if( NOT __ANDROID__ )
configure_file( deploy.cm.c ${CMAKE_BINARY_DIR}/deploy.c.tmp )
EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_BINARY_DIR}/deploy.c.tmp ${CMAKE_BINARY_DIR}/deploy.c )
ADD_EXECUTABLE_FORCE_SOURCE( ${PROJECT_NAME} ${CMAKE_BINARY_DIR}/deploy.c )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER install )

install_mode_dest( ${PROJECT_NAME} )
endif( NOT __ANDROID__ )

add_subdirectory( info )

