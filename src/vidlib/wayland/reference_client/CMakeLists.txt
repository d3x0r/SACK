CMAKE_MINIMUM_required( VERSION 3.1 )

project( reference_client )

add_executable( connect connect.c )
target_link_libraries( connect wayland-client )

add_executable( registry registry.c )
target_link_libraries( registry wayland-client )

add_executable( surface surface.c )
target_link_libraries( surface wayland-client )

add_executable( client client.c keyboard_protocol.c pointer_protocol.c xdg-shell-protocol.c )
target_link_libraries( client wayland-client wayland-cursor)
