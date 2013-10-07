
cmake_minimum_required(VERSION 2.8)

project( clock.frames )


SET( INTERSHELL_FRAMES
ConfigurePingStatusGraph.isFrame 
)




INSTALL( 
  FILES
    ${INTERSHELL_FRAMES}
  DESTINATION
    ${DATA_INSTALL_PREFIX}/frames
)




