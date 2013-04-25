if( WIN32 )

include_directories( driver )

project( usbpulse100.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
usbpulse100
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source(${PROJECT_NAME} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  SUFFIX ""
                  PREFIX ""
                  FOLDER "InterShell/plugins"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} sack_widgets elan_driver_library )
install_literal_product( ${PROJECT_NAME} plugins ) 



project( elan_driver_library )

set( BASE_SOURCES
	${FIRST_GCC_LIBRARY_SOURCE} 
	    driver/usbpulse100drvr.cpp
	${LAST_GCC_LIBRARY_SOURCE}
)

add_library(${PROJECT_NAME} SHARED ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}" 
                  FOLDER InterShell/plugins
)

target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} version )
install_literal_product( ${PROJECT_NAME} plugins )
INSTALL( FILES driver/CP210xMan.dll DESTINATION bin/plugins )
INSTALL( DIRECTORY images DESTINATION resources )

endif( WIN32 )

