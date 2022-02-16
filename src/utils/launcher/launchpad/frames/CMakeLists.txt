

project( launchpad.frames )


SET( INTERSHELL_FRAMES
ConfigureLaunchpad.Frame
)




INSTALL( 
  FILES
    ${INTERSHELL_FRAMES}
  DESTINATION
    ${DATA_INSTALL_PREFIX}/frames
)




