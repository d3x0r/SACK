
set( PROJECT clock.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
clock_addon
${LAST_GCC_LIBRARY_SOURCE}
)

if( C++ )
set_source_files_properties( ${SOURCES} PROPERTIES LANGUAGE "CXX" )
endif()

add_library_force_source(${PROJECT} SHARED ${SOURCES} )



string( REPLACE "." "_" TARGET_LABEL ${PROJECT} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  FOLDER InterShell/plugins
)
if( NOT __ANDROID__ )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
)
endif( NOT __ANDROID__ )
my_target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
INSTALL_PLUGIN( ${PROJECT} ) 

add_subdirectory( frames )

