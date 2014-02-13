
add_library( pscards SHARED ../cards ../poker )

SET_TARGET_PROPERTIES(pscards PROPERTIES
                  COMPILE_DEFINITIONS  "CARDS_SOURCE"
                  FOLDER Dekware/plugins/cards
)

my_target_link_libraries( pscards ${SACK_LIBRARIES} )

INSTALL( TARGETS pscards 
	RUNTIME DESTINATION bin/plugins
	LIBRARY DESTINATION bin/plugins
	ARCHIVE DESTINATION lib )


