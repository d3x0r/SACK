cmake_minimum_required(VERSION 2.8)

if( __ANDROID__ )
ADD_DEFINITIONS( "-fno-rtti" )
endif( __ANDROID__ )

set( PROJECT automaton.board )
set( SOURCES 
  ${FIRST_GCC_LIBRARY_SOURCE}
  board.cpp
  peice.cpp
  layer.cpp
  toolbin.cpp
  board_db.c
  ${LAST_GCC_LIBRARY_SOURCE}
)

if( __ANDROID__ )
set( MOREFLAGS "-fno-rtti" )
endif( __ANDROID__ )
ADD_DEFINITIONS( -DBOARD_SOURCE -DPEICE_SOURCE )
ADD_LIBRARY( ${PROJECT} SHARED ${SOURCES} )
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} ${SACK_LIBRARIES_PLUSPLUS} automaton.brain )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
				  FOLDER "Automaton"
)

if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${PROJECT}
	        RUNTIME DESTINATION bin                     
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
else( WIN32 )
	install( TARGETS ${PROJECT}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )

set( PROJECT automaton.brain.board.shell )

set( SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
  brainshell.cpp
  bs_dialog.cpp
  ${LAST_GCC_PROGRAM_SOURCE}
)

if( __ANDROID__ )
set( MOREFLAGS "-fno-rtti" )
endif( __ANDROID__ )

if( WIN32 )
set( MOREFLAGS ${MOREFLAGS} "-DWINDOWS_MODE" )
endif( WIN32 )


ADD_PROGRAM( ${PROJECT} WIN32 ${SOURCES} )
TARGET_LINK_LIBRARIES( ${PROJECT} automaton.board automaton.brain )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
				  FOLDER "Automaton"
)


set( PROJECT automaton.brain.board )

set( SOURCES 
  ${FIRST_GCC_LIBRARY_SOURCE}
  brainshell.cpp
  bs_dialog.cpp
  ${LAST_GCC_LIBRARY_SOURCE} 
)

if( __ANDROID__ )
ADD_DEFINITIONS( "-fno-rtti" )
endif( __ANDROID__ )
ADD_LIBRARY( ${PROJECT} SHARED ${SOURCES} )
TARGET_LINK_LIBRARIES( ${PROJECT} ${SACK_LIBRARIES} ${SACK_LIBRARIES_PLUSPLUS} automaton.board automaton.brain )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
				  FOLDER "Automaton"
)

install( FILES brain.peices.txt DESTINATION Resources )

install( DIRECTORY frames DESTINATION Resources )
install( DIRECTORY images/ DESTINATION Resources/images/brainboard )

if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${PROJECT}
	        RUNTIME DESTINATION bin                     
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
else( WIN32 )
	install( TARGETS ${PROJECT}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )

