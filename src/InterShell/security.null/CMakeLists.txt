
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
                  SUFFIX ""
                  PREFIX ""
                  FOLDER "InterShell/plugins"
)
target_link_libraries( ${project} ${SACK_LIBRARIES} sack_widgets keypad.isp ${PLATFORM_LIBRARIES} )
install_default_project( ${project} bin/plugins ) 

INSTALL( 
  DIRECTORY frames
  DESTINATION
    ${DATA_INSTALL_PREFIX}/frames
)

