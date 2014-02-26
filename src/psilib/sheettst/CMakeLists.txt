
project( test.sheet.controls )

set(BASE_SOURCES
    sheettst.c
)

add_program(${PROJECT_NAME} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
install_mode_dest( ${PROJECT_NAME} )


