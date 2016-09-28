
set( project rundelay )


add_program(${project} rundelay.c )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER utils )


set( project getPassword )
add_program(${project} getPass.c )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER utils )


add_subdirectory( portable )
