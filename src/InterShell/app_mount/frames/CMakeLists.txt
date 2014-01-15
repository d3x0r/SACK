cmake_minimum_required(VERSION 2.8)

project( application_mount.frames )

FILE( GLOB INTERSHELL_FRAMES  *.isFrame )

INSTALL( 
  FILES
    ${INTERSHELL_FRAMES}
  DESTINATION
    ${DATA_INSTALL_PREFIX}/frames
)


