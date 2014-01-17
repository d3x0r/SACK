
include_directories( "../../include" )

add_plugin( virtuality.nex 3D_World virtuality_interface.cpp macro_extension.cpp brain_extension.cpp )
my_target_link_libraries( virtuality.nex virtuality automaton.brain.board automaton.brain )
install( DIRECTORY images DESTINATION Resources )
