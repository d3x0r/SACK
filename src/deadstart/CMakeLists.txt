
SET( DEADSTART_SOURCES  deadstart_list.c
deadstart_lib.c deadstart_prog.c deadstart_end.c )



INSTALL( 
  FILES
    ${DEADSTART_SOURCES}
  DESTINATION
    src/sack
)

install( DIRECTORY android DESTINATION src/sack )

add_library( deadstart SHARED deadstart_core.c )
install_mode_dest( deadstart )
