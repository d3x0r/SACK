
set( project test.palette )

set(BASE_SOURCES
    palette.c
)

add_program(${project} ${BASE_SOURCES} )
if( NOT MONOLITHIC )
	my_target_link_libraries( ${project} bag.psi )
endif( NOT MONOLITHIC )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests/PSI
)
install_default_dest( ${project} )


