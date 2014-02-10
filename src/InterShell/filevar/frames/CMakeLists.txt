
project( filevars.frames )

FILE( GLOB INTERSHELL_FRAMES  *.isFrame )

INSTALL( 
  FILES
    ${INTERSHELL_FRAMES}
  DESTINATION
    ${DATA_INSTALL_PREFIX}/frames
)


