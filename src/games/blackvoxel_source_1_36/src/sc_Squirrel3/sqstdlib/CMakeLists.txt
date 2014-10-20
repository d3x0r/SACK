

set( SOURCES 	sqstdblob.cpp 
	sqstdio.cpp 
	sqstdstream.cpp 
	sqstdmath.cpp 
	sqstdsystem.cpp 
	sqstdstring.cpp 
	sqstdaux.cpp 
sqstdrex.cpp
)

include_directories( ../include )

add_library( sqstdlib ${SOURCES} ) 
target_link_libraries( sqstdlib squirrel )
