add_subdirectory( pscards )

add_plugin( cards.nex cards ntlink ) 

my_target_link_libraries( cards.nex pscards )
