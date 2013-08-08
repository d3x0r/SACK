
set( project calendar.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
DateButton.c
${LAST_GCC_LIBRARY_SOURCE}
)


add_library(${project} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  FOLDER InterShell/plugins
)
if( NOT __ANDROID__ )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT __ANDROID__ )

target_link_libraries( ${project} ${SACK_LIBRARIES} )
INSTALL_PLUGIN( ${project} ) 

