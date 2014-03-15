
set( PROJECT systray_shell )


set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    systray_shell.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${PROJECT} WIN32 ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES FOLDER utils )

if( NOT WAS_MONOLITHIC )
# only build in modular mode; static video library is not supported.
add_subdirectory( portable )
endif( NOT WAS_MONOLITHIC )
