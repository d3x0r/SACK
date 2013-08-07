
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
                  SUFFIX ""
                  PREFIX ""
                  FOLDER "InterShell/plugins"
)
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_literal_product( ${PROJECT} plugins ) 


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
                  SUFFIX ""
                  PREFIX ""
                  FOLDER "InterShell/plugins"
)
if( WIN32 )
set( WINMM winmm )
endif( WIN32 )
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} ${WINMM} )
install_literal_product( ${PROJECT} plugins ) 


