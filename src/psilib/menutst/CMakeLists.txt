
project( test.menu.popups )

set(BASE_SOURCES
    menutest.c
)

add_program(${PROJECT_NAME} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests/PSI
)
install_mode_dest( ${PROJECT_NAME} )


