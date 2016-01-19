
set( project get_display_position )


set(BASE_SOURCES
    get_display.c
)

add_executable(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
				  COMPILE_DEFINITIONS "CONSOLE"
)
INSTALL_DEFAULT_DEST( ${project} )

