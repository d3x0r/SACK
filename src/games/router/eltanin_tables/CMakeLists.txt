cmake_minimum_required(VERSION 2.8)

project( game_tables.plugin )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
	table.cpp
${LAST_GCC_LIBRARY_SOURCE}
)


add_library(${PROJECT_NAME} SHARED ${SOURCES} )


string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  SUFFIX ""
                  PREFIX ""
                  FOLDER "InterShell/plugins"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} sack_widgets )
install_literal_product( ${PROJECT_NAME} plugins ) 
install( DIRECTORY images DESTINATION resources )
