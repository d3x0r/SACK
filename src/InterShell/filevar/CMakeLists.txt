
set( PROJECT  filevars.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
   filevars.c
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source(${PROJECT} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  FOLDER "InterShell/plugins"
)
if( NOT__ANDROID__ )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT__ANDROID__ )
target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_plugin( ${PROJECT} )

add_subdirectory( frames )

