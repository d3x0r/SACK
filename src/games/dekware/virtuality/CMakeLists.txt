
include_directories( "../../include" )

ADD_DEFINITIONS( -DMAKE_RCOORD_SINGLE )

add_plugin( virtuality.nex 3D_World virtuality_interface.cpp macro_extension.cpp brain_extension.cpp )
my_target_link_libraries( virtuality.nex virtuality automaton.brain.board automaton.brain )
install( DIRECTORY images DESTINATION ${DATA_INSTALL_PREFIX}/images )
