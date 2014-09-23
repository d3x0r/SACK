
set( PROJECT test_printer )

set(BASE_SOURCES test_printer.c )

add_program(${PROJECT} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES FOLDER tests )

