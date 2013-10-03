if( WIN32 )

include_directories( driver )

set( project usbpulse100.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
usbpulse100
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source(${project} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  SUFFIX ""
                  PREFIX ""
                  FOLDER "InterShell/plugins"
)
target_link_libraries( ${project} ${SACK_LIBRARIES} ${SACK_LIBRARIES_PLUSPLUS} sack_widgets elan_driver_library )
install_literal_product( ${project} plugins ) 



set( project elan_driver_library )

set( BASE_SOURCES
	${FIRST_GCC_LIBRARY_SOURCE} 
	    driver/usbpulse100drvr.cpp
	${LAST_GCC_LIBRARY_SOURCE}
)

add_library(${project} SHARED ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${project}" 
                  FOLDER InterShell/plugins
)

target_link_libraries( ${project} ${SACK_LIBRARIES} version )
install_literal_product( ${project} plugins )
INSTALL( FILES driver/CP210xMan.dll DESTINATION bin/plugins )
INSTALL( DIRECTORY images DESTINATION resources )

endif( WIN32 )

