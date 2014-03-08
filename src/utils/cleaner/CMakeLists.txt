
set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    cleaner.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(file_cleaner ${BASE_SOURCES} )
