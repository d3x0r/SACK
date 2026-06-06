PROJECT( "Dekware.Deploy" )

if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../.git )
	EXECUTE_PROCESS(WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND git rev-list --max-count=1 HEAD OUTPUT_VARIABLE CURRENT_REPO_REVISION )
endif()

if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../.git/refs/tags )
	FILE( GLOB TMP_tags RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../.git/refs/tags/  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../.git/refs/tags/* )
	set( use_tag "0" )
	foreach( tag ${TMP_tags} )
		if( ${tag} GREATER ${use_tag} )
                	set( use_tag ${tag} )
		endif( ${tag} GREATER ${use_tag} )
	endforeach( )
      if( NOT use_tag STREQUAL "0" )
        	FILE( READ ${CMAKE_CURRENT_SOURCE_DIR}/../../../../.git/refs/tags/${use_tag} OTHER_REPO_REVISION )
		if( CURRENT_REPO_REVISION )
		        SET( CURRENT_REPO_REVISION "${CURRENT_REPO_REVISION} git:${use_tag}.${OTHER_REPO_REVISION}" )
	        else( CURRENT_REPO_REVISION )
		        SET( CURRENT_REPO_REVISION ${OTHER_REPO_REVISION} )
		endif()
        endif()
endif( )

message( "Dekware revision: ${CURRENT_REPO_REVISION}" )


configure_file( deploy.cm.cm ${CMAKE_BINARY_DIR}/deploy.cm.tmp @ONLY )
include( ${CMAKE_BINARY_DIR}/deploy.cm.tmp )


if( NOT __ANDROID__ )
  configure_file( deploy.cm.c ${CMAKE_BINARY_DIR}/deploy.c.tmp )
  EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_BINARY_DIR}/deploy.c.tmp ${CMAKE_BINARY_DIR}/deploy.c )
  ADD_EXECUTABLE( ${PROJECT_NAME} ${CMAKE_BINARY_DIR}/deploy.c )
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Install and Deploy" )


#  install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . )
endif( NOT __ANDROID__ )


