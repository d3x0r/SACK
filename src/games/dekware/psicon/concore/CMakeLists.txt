
set( project concore )

add_library( ${project} 
${FIRST_GCC_LIBRARY_SOURCE}
   ../history.c ../WinLogic.c ../keydefs.c
${LAST_GCC_LIBRARY_SOURCE} 
)

if( UNIX )
   set( MORE_OPTIONS  "-fPIC" )
endif( UNIX ) 

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "${MORE_OPTIONS} -DTARGET_LABEL=${TARGET_LABEL} -DCORE_CON -DCORECON_SOURCE -D__DEKWARE_PLUGIN__"
                  FOLDER plugins/devices
)

set( LINK_DIRECTORIES ${SACK_LIBRARY_DIR} )
my_target_link_libraries( ${project}  ${SACK_LIBRARIES} )

INSTALL( TARGETS ${project}
	RUNTIME DESTINATION share/SACK/plugins
	LIBRARY DESTINATION share/SACK/plugins
)

