
cmake_minimum_required(VERSION 2.8)

project( dialog.nex )


set(SOURCES 
${FIRST_GCC_LIBRARY_SOURCE}
	ctlbutton.nex.c ctledit.nex.c ctllist.nex.c dialog.c
${LAST_GCC_LIBRARY_SOURCE} )


add_library( ${PROJECT_NAME} SHARED ${SOURCES} )

string( REGEX REPLACE "(.*)\\.(.*)" "\\1_\\2" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  DEFINE_SYMBOL CHARACTER_SOURCE
                  PREFIX ""
                  SUFFIX ""
                  COMPILE_DEFINITIONS "TARGET_LABEL=${TARGET_LABEL}"
                  FOLDER plugins
)


target_link_libraries( ${PROJECT_NAME}  ${SACK_LIBRARIES} )

INSTALL( TARGETS ${PROJECT_NAME} 
	RUNTIME DESTINATION bin/plugins
	LIBRARY DESTINATION bin/plugins
)



