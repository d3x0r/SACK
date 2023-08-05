CMAKE_MINIMUM_required( VERSION 3.1 )

project( reference_client )

add_executable( client client.c keyboard_protocol.c pointer_protocol.c xdg-shell-protocol.c )
target_link_libraries( client wayland-client wayland-cursor)
