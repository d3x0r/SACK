
set( project security.null.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
		null_password.c
${LAST_GCC_LIBRARY_SOURCE}
)

add_library(${project} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  FOLDER "InterShell/plugins"
)
if( NOT __ANDROID__ )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT __ANDROID__ )
my_target_link_libraries( ${project} ${SACK_LIBRARIES} sack_widgets keypad.isp ${PLATFORM_LIBRARIES} )
install_plugin( ${project}  ) 

INSTALL( 
  DIRECTORY frames
  DESTINATION
    ${DATA_INSTALL_PREFIX}/frames
)

