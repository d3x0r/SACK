cmake_minimum_required(VERSION 2.8)

project( route_loader.isp )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
   Loader.cpp
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
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} sack_widgets layout_board )
install_literal_product( ${PROJECT_NAME} plugins ) 

