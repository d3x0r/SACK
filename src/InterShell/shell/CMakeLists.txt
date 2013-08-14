

##########################################################################3
################ Console ######################
##########################################################################3

set( project InterShell_console  )


set(SOURCES
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

if( __CLR__ )
add_definitions( 
	-DLOAD_LIBNAME="InterShell.core.dll" -DMODE=1
)
else( __CLR__ )
  if( __ANDROID__ )
    add_definitions( 
	-DLOAD_LIBNAME="libInterShell.core.so" -DMODE=1
     )
  else( __ANDROID__ )

    add_definitions( 
	-DLOAD_LIBNAME="InterShell.core" -DMODE=1
    )
  endif( __ANDROID__ )
endif( __CLR__ )

add_program(${project} ${SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER InterShell
)
if( __ANDROID__ )
install( TARGETS ${project} RUNTIME DESTINATION lib
	LIBRARY DESTINATION lib) 
else( __ANDROID__ )
if( __CLR__ )
install( TARGETS ${project} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${project} RUNTIME DESTINATION bin ) 
endif( __CLR__ )
endif( __ANDROID__ )
