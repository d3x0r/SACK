
set( project TouchDiagnostic )

set(BASE_SOURCES
    touch_diag.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
        FOLDER tests
)


