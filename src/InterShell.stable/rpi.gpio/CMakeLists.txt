
set( PROJECT rpi.gpio.isp )

set( SOURCES
rpi.gpio.c
)

sack_add_library(${PROJECT} SHARED ${SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER "InterShell/plugins"
)
if( NOT __ANDROID__ )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT __ANDROID__ )
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_PLUGIN( ${PROJECT} ) 


