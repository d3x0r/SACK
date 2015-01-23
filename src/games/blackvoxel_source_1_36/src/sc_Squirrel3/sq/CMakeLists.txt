
set( SOURCES sq.c )
include_directories( ../include )
set_source_files_properties( sq.c PROPERTIES LANGUAGE CXX )
add_executable( sq  ${SOURCES} )
target_link_libraries( sq sqstdlib squirrel )
