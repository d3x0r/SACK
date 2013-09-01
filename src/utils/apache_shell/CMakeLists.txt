if( NOT __ANDROID__ )

set( PROJECT apache_shell )

if( __LINUX__ )
include_directories( /usr/include/apr-1 )
include_directories( /usr/include/apache )
include_directories( /usr/include/apr-util )
else( __LINUX__ )
include_directories( c:/wamp/bin/apache/apache2.4.4/include )
endif( __LINUX__ )


include_directories(  ${SACK_BASE}/include )

if( WIN32 )
set( MORE_DEFS "APR_DECLARE_EXPORT;WIN32" )
endif( WIN32 )

add_library( ${PROJECT} SHARED 
                ${FIRST_GCC_LIBRARY_SOURCE}
                apache_shell.c
                ${LAST_GCC_LIBRARY_SOURCE}
           )

my_target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
set( TARGET_LABEL ${PROJECT} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  COMPILE_DEFINITIONS "${MORE_DEFS}"
                  FOLDER "InterShell/plugins"
)
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  PREFIX "mod_"
                  SUFFIX ".so"
)
install_default_project( ${PROJECT} lib )

endif( NOT __ANDROID__ )
