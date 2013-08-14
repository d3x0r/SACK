
project( InterShell.Service  )


set(SOURCES
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

if( __CLR__ )
add_definitions( 
	-DLOAD_LIBNAME="InterShell.core.dll" -DMODE=1 -DCONSOLE_SHELL
)
else( __CLR__ )
add_definitions( 
	-DLOAD_LIBNAME="InterShell.core" -DMODE=1 -DCONSOLE_SHELL
)
endif( __CLR__ )

add_executable_force_source(${PROJECT_NAME} ${SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGET_LABEL=Intershell_service;BUILD_SERVICE;BUILD_SERVICE_THREAD"
                  FOLDER InterShell
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )
if( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin ) 
endif( __CLR__ )

