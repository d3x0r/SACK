
include_directories( "../../include" )

add_plugin( virtuality.nex 3D_World virtuality_interface.cpp )
my_target_link_libraries( virtuality.nex virtuality )
