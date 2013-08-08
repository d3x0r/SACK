
set( project Slider.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
       slider
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source(${project} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
				  COMPILE_DEFINITIONS "TARGETNAME=\"${project}\"" 
                  FOLDER "InterShell/plugins"
)
if( NOT__ANDROID__ )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT__ANDROID__ )
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_plugin( ${project}  ) 

add_subdirectory( frames )

