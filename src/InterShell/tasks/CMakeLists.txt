

cmake_minimum_required(VERSION 2.8)

project( tasks.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
tasks
${LAST_GCC_LIBRARY_SOURCE}
)

add_library_force_source(${PROJECT_NAME} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  SUFFIX ""
                  PREFIX ""
                  FOLDER "InterShell/plugins"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} sack_widgets )
install_literal_product( ${PROJECT_NAME} plugins ) 

add_subdirectory( frames )

