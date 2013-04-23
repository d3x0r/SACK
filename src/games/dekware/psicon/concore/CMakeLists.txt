
cmake_minimum_required(VERSION 2.8)

project( concore )

add_library( ${PROJECT_NAME} SHARED 
${FIRST_GCC_LIBRARY_SOURCE}
   ../history ../WinLogic ../keydefs
${LAST_GCC_LIBRARY_SOURCE} 
)

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DCORE_CON -DCORECON_SOURCE -D__DEKWARE_PLUGIN__"
                  FOLDER plugins/devices
)

set( LINK_DIRECTORIES ${SACK_LIBRARY_DIR} )
target_link_libraries( ${PROJECT_NAME}  ${SACK_LIBRARIES} )

INSTALL( TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin/plugins
	LIBRARY DESTINATION bin/plugins
)

