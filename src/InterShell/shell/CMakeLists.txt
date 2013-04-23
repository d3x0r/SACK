


cmake_minimum_required(VERSION 2.8)

project( InterShell_console  )


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
add_definitions( 
	-DLOAD_LIBNAME="InterShell.core" -DMODE=1
)
endif( __CLR__ )

add_executable_force_source(${PROJECT_NAME} ${SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}"
                  FOLDER InterShell
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
if( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin ) 
endif( __CLR__ )
