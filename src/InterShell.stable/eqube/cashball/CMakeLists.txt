
set( PROJECT cashball.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
cashball
${LAST_GCC_LIBRARY_SOURCE} )

add_library_force_source(${PROJECT} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
				  COMPILE_DEFINITIONS "TARGETNAME=\"${PROJECT}\""
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

