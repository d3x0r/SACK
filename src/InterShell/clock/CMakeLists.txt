

cmake_minimum_required(VERSION 2.8)

project( clock.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
clock_addon
${LAST_GCC_LIBRARY_SOURCE}
)

if( C++ )
set_source_files_properties( ${SOURCES} PROPERTIES LANGUAGE "CXX" )
endif()

add_library_force_source(${PROJECT_NAME} SHARED ${SOURCES} )



string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  PREFIX ""
                  SUFFIX ""
                  FOLDER InterShell/plugins
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
install_literal_product( ${PROJECT_NAME} plugins ) 

add_subdirectory( frames )

