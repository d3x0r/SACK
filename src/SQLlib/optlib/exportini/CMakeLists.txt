
set( PROJECT exportini )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    exportini.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${PROJECT} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils
)

