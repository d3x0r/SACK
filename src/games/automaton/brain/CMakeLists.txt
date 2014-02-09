cmake_minimum_required(VERSION 2.8)

PROJECT( automaton.brain )
set( SOURCES 
	${FIRST_GCC_LIBRARY_SOURCE}
  brain.cpp
  convert.cpp
  neuron.cpp
  synapse.cpp
	${LAST_GCC_LIBRARY_SOURCE}
)

ADD_LIBRARY( ${PROJECT_NAME} SHARED ${SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DBRAIN_SOURCE"
				  FOLDER "Automaton"
)
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${SACK_LIBRARIES} ${SACK_LIBRARIES_PLUSPLUS} )

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



