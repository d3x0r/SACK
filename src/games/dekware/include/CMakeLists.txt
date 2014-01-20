
# something like, 
# if !(MAKE_DEV)

SET( DEKWARE_HEADERS commands.h datapath.h input.h interface.h my_ver.h plugin.h space.h text.h )


INSTALL( 
  FILES
    ${DEKWARE_HEADERS}
  DESTINATION
    include/dekware
)

