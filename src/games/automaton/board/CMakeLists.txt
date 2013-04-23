cmake_minimum_required(VERSION 2.8)

PROJECT( automaton.board )
set( SOURCES 
  ${FIRST_GCC_LIBRARY_SOURCE}
  board.cpp
  peice.cpp
  layer.cpp
  toolbin.cpp
  board_db.c
  ${LAST_GCC_LIBRARY_SOURCE}
)

ADD_DEFINITIONS( -DBOARD_SOURCE -DPEICE_SOURCE )
ADD_LIBRARY( ${PROJECT_NAME} SHARED ${SOURCES} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} automaton.brain )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
)

if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin                     
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
else( WIN32 )
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )

PROJECT( automaton.brain.board.shell )

set( SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
  brainshell.cpp
  bs_dialog.cpp
  ${LAST_GCC_PROGRAM_SOURCE}
)

if( WIN32 )
set( MOREFLAGS "-DWINDOWS_MODE" )
endif( WIN32 )


ADD_EXECUTABLE( ${PROJECT_NAME} WIN32 ${SOURCES} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} automaton.board automaton.brain )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${MOREFLAGS}"
)

if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin                     
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
else( WIN32 )
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )

PROJECT( automaton.brain.board )

set( SOURCES 
  ${FIRST_GCC_LIBRARY_SOURCE}
  brainshell.cpp
  bs_dialog.cpp
  ${LAST_GCC_LIBRARY_SOURCE} 
)

ADD_LIBRARY( ${PROJECT_NAME} SHARED ${SOURCES} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} automaton.board automaton.brain )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
)

install( FILES brain.peices.txt DESTINATION Resources )

install( DIRECTORY frames DESTINATION Resources )
install( DIRECTORY images DESTINATION Resources/images/brainboard/ )

if( WIN32 )
	# On Windows platforms, the dynamic libs should
	# go in the same dir as the executables.
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin                     
        	LIBRARY DESTINATION bin
	        ARCHIVE DESTINATION lib )
else( WIN32 )
	install( TARGETS ${PROJECT_NAME}
	        RUNTIME DESTINATION bin 
        	LIBRARY DESTINATION lib
	        ARCHIVE DESTINATION lib )
endif( WIN32 )

