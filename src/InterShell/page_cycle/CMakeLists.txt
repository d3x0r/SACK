
set( PROJECT page_cycle.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
page_cycle
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source(${PROJECT} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
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


set( PROJECT net_page_changer.isp  )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
net_page_change
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source(${PROJECT} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  FOLDER "InterShell/plugins"
)
if( NOT __ANDROID__ )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT __ANDROID__ )
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} ${SACK_PLATFORM_LIBRARIES} )
install_PLUGIN( ${PROJECT} ) 


