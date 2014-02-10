
project( test_file_monitor )

set(BASE_SOURCES
 ${FIRST_GCC_PROGRAM_SOURCE}
    test_file_monitor.c
 ${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )

string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL} ${LibraryExtraFlags}"
                  FOLDER tests
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( WIN32 )
target_link_libraries( ${PROJECT_NAME} winmm )
endif( WIN32 )

install_mode_dest( ${PROJECT_NAME} )


