
cmake_minimum_required(VERSION 2.8)

project( frames )

FILE( GLOB MILK_FRAMES "*.frame" )

INSTALL( 
  FILES
    ${MILK_FRAMES}
  DESTINATION
    ${DATA_INSTALL_PREFIX}/frames
)




