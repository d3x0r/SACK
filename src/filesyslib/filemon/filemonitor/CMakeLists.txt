
project( test_file_monitor )

set(BASE_SOURCES
 ${FIRST_GCC_PROGRAM_SOURCE}
    test_file_monitor.c
 ${LAST_GCC_PROGRAM_SOURCE}
)

ADD_PROGRAM(${PROJECT_NAME} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
SET_PROPERTY(TARGET ${project} APPEND PROPERTY
                  COMPILE_FLAGS  "${LibraryExtraFlags}"
      )

install_mode_dest( ${PROJECT_NAME} )


