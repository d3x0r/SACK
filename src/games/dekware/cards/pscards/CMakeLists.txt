
add_library( pscards SHARED ../cards.c ../poker.c )

SET_TARGET_PROPERTIES(pscards PROPERTIES
                  COMPILE_DEFINITIONS  "CARDS_SOURCE"
                  FOLDER plugins/cards
)

my_target_link_libraries( pscards ${SACK_LIBRARIES} )

INSTALL( TARGETS pscards 
	RUNTIME DESTINATION share/SACK/plugins
	LIBRARY DESTINATION share/SACK/plugins
	ARCHIVE DESTINATION lib )


