

cmake_minimum_required(VERSION 2.8)

project( network_graph.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
global.c linegraph.c connect_stat.c packet_stat.c
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source(${PROJECT_NAME} SHARED ${SOURCES} )



string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  PREFIX ""
                  SUFFIX ""
                  FOLDER InterShell/plugins
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( WIN32 )
target_link_libraries( ${PROJECT_NAME} winmm )
endif( WIN32 )
install_literal_product( ${PROJECT_NAME} plugins ) 

add_subdirectory( frames )

