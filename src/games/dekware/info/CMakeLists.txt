# Get current source repository Revision:
execute_process( COMMAND hg id -i WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE CURRENT_REPO_REVISION )
message( "Source revision: " ${CURRENT_REPO_REVISION} )
# output build info to files
file(WRITE ${CMAKE_BINARY_DIR}/info_out.tmp/ver.txt "${CURRENT_REPO_REVISION}" )
file(WRITE ${CMAKE_BINARY_DIR}/info_out.tmp/type.txt "${CMAKE_BUILD_TYPE}" )
file(WRITE ${CMAKE_BINARY_DIR}/info_out.tmp/generator.txt "${CMAKE_GENERATOR}" )
file(WRITE ${CMAKE_BINARY_DIR}/info_out.tmp/project.txt "${CMAKE_PROJECT_NAME}" )

foreach( FILE ver.txt type.txt generator.txt project.txt )
	EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${CMAKE_BINARY_DIR}/info_out.tmp/${FILE} ${CMAKE_BINARY_DIR}/info_out/${FILE} )
endforeach( FILE ver.txt type.txt genrator.txt )

#install the build info so it can be referred to after deployment
install (
	DIRECTORY ${CMAKE_BINARY_DIR}/info_out/
	DESTINATION ./info
	PATTERN "CMakeLists.txt" EXCLUDE)
	#FILES_MATCHING REGEX "^.*\\.txt$" )

