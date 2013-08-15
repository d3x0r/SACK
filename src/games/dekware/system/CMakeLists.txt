
cmake_minimum_required(VERSION 2.8)

project( system.nex )


set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
	ntlink process shutdown
${LAST_GCC_LIBRARY_SOURCE} )


add_library( ${PROJECT_NAME} SHARED ${SOURCES} )

string( REGEX REPLACE "(.*)\\.(.*)" "\\1_\\2" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  PREFIX ""
                  SUFFIX ""
                  FOLDER plugins
)

if( WIN32 )
set(WINMM winmm)
endif()

target_link_libraries( ${PROJECT_NAME} 
	${SACK_LIBRARIES}
        ${WINMM}
)

INSTALL( TARGETS ${PROJECT_NAME} 
	RUNTIME DESTINATION bin/plugins
	LIBRARY DESTINATION bin/plugins
)





