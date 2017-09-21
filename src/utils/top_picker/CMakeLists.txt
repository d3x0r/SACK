
set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    top_picker.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(top_picker ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(top_picker PROPERTIES    FOLDER utils   )
