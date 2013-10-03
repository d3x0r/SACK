cmake_minimum_required(VERSION 2.8)

set( project security.sql.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
		sql_password.c
		comn_util.c
		password_frame.c
		password_window.c
		loginmonitor.c
${LAST_GCC_LIBRARY_SOURCE}
)

add_library(${project} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  COMPILE_DEFINITIONS "TARGETNAME=\"${project}\""
                  SUFFIX ""
                  PREFIX ""
                  FOLDER "InterShell/plugins"
)
target_link_libraries( ${project} ${SACK_LIBRARIES} sack_widgets keypad.isp )
# used for EnumProcesses
target_link_libraries( ${project} Psapi )
install_default_project( ${project} bin/plugins ) 

INSTALL( DIRECTORY frames DESTINATION Resources ) 

