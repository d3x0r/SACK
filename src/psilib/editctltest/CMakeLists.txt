
project( test.edit.control )

set(BASE_SOURCES
    edittst.c
)

ADD_PROGRAM(${PROJECT_NAME} WIN32 ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
      FOLDER tests/PSI
)
install_mode_dest( ${PROJECT_NAME} )


