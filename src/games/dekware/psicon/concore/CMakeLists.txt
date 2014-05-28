
set( project concore )

add_library( ${project} SHARED 
${FIRST_GCC_LIBRARY_SOURCE}
   ../history ../WinLogic ../keydefs
${LAST_GCC_LIBRARY_SOURCE} 
)

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} -DCORE_CON -DCORECON_SOURCE -D__DEKWARE_PLUGIN__"
                  FOLDER Dekware/plugins/devices
)

set( LINK_DIRECTORIES ${SACK_LIBRARY_DIR} )
my_target_link_libraries( ${project}  ${SACK_LIBRARIES} )

INSTALL( TARGETS ${project}
	RUNTIME DESTINATION bin/plugins
	LIBRARY DESTINATION bin/plugins
)

