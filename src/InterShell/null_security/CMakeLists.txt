cmake_minimum_required(VERSION 2.8)

project( sql_password.isom )

set( SOURCES
${FIRST_GCC_LIBRARY_SOURCE} 
		reports.c
		sql_password.c
        comn_util.c
        password_frame.c
        password_window.c
${LAST_GCC_LIBRARY_SOURCE}
)

add_library(${PROJECT_NAME} SHARED ${SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  SUFFIX ""
                  PREFIX ""
                  FOLDER "MILK/plugins"
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} sack_widgets altanik_core comnprnt keypad.isp winmm )
install_default_project( ${PROJECT_NAME} bin/plugins ) 

add_subdirectory( frames )

