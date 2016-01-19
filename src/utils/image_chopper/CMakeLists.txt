set( PROJECT image_chopper )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    image_chopper.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_program( ${PROJECT} ${BASE_SOURCES} )
install_default_dest( ${PROJECT} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils
)

