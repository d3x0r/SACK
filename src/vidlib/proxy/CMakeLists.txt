
ADD_DEFINITIONS( -DMAKE_RCOORD_SINGLE )

set( PROJECT vidlib.proxy.server )
sack_add_library( ${PROJECT} SHARED server.c )
install_default_dest( ${PROJECT} )

set( PROJECT vidlib.proxy.client )
add_program( ${PROJECT} WIN32 client.c )
install_default_dest( ${PROJECT} )

