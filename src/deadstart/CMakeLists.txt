
SET( DEADSTART_SOURCES  deadstart_list.c
deadstart_lib.c deadstart_prog.c deadstart_end.c )



INSTALL( 
  FILES
    ${DEADSTART_SOURCES}
  DESTINATION
    src/sack
)


