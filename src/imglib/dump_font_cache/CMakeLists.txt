
set( project DumpFontCache )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    dump_font_cache.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)

