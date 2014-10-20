

set( SOURCES 	sqapi.cpp 
	sqbaselib.cpp 
	sqfuncstate.cpp 
	sqdebug.cpp 
	sqlexer.cpp 
	sqobject.cpp 
	sqcompiler.cpp 
	sqstate.cpp 
	sqtable.cpp 
	sqmem.cpp 
	sqvm.cpp 
sqclass.cpp )

INCLUDE_DIRECTORIES( ../include )

add_library( squirrel ${SOURCES} )

