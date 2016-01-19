cmake_minimum_required(VERSION 2.8)

set( project security.sql.isp )

if( NOT __ANDROID__ AND NOT UNIX )
  set( TASK_MONITOR_SOURCES 		loginmonitor.c )
endif( NOT __ANDROID__ AND NOT UNIX )


set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
		sql_password.c
		comn_util.c
		password_frame.c
		${TASK_MONITOR_SOURCES}
${LAST_GCC_LIBRARY_SOURCE}
)

add_library(${project} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  COMPILE_DEFINITIONS "TARGETNAME=\"${project}\""
                  FOLDER "InterShell/plugins"
)
if( NOT __ANDROID__ )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT __ANDROID__ )
my_target_link_libraries( ${project} ${SACK_LIBRARIES} sack_widgets keypad.isp )

if( WIN32 )
	# used for EnumProcesses
	target_link_libraries( ${project} psapi )
endif( WIN32 )

install_plugin( ${project}  ) 

INSTALL( DIRECTORY frames DESTINATION resources ) 

